#!/usr/bin/env python

"""camcops_server/cc_modules/cc_redcap.py

===============================================================================

    Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).

    This file is part of CamCOPS.

    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

**Implements communication with REDCap.**

- For general information about REDCap, see https://www.project-redcap.org/.

- The API documentation is not provided there, but is available from
  your local REDCap server. Pick a project. Choose "API" from the left-hand
  menu. Follow the "REDCap API documentation" link.

- We use PyCap (https://pycap.readthedocs.io/ or
  https://github.com/redcap-tools/PyCap). See also
  https://redcap-tools.github.io/projects/. PyCap is no longer being actively
  developed though the author is still responding to issues and pull requests.

We use an XML fieldmap to describe how the rows in CamCOPS task tables are
translated into REDCap records. See :ref:`REDCap export <redcap>`.

REDCap does not assign instance IDs for repeating instruments so we need to
query the database in order to determine the next instance ID. It is possible
to create a race condition if more than one client is trying to update the same
record at the same time.

"""

from enum import Enum
import io
import logging
from typing import (
    Any,
    Dict,
    Iterable,
    List,
    Optional,
    TYPE_CHECKING,
    Union,
)
import xml.etree.cElementTree as ElementTree

from asteval import Interpreter, make_symbol_table
from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.logs import BraceStyleAdapter
from pandas import DataFrame
from pandas.errors import EmptyDataError
import redcap

from camcops_server.cc_modules.cc_constants import (
    ConfigParamExportRecipient,
    DateFormat,
)
from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_exportmodels import ExportedTaskRedcap
    from camcops_server.cc_modules.cc_request import CamcopsRequest
    from camcops_server.cc_modules.cc_task import Task

log = BraceStyleAdapter(logging.getLogger(__name__))

MISSING_EVENT_TAG_OR_ATTRIBUTE = (
    "The REDCap project has events but there is no 'event' tag "
    "in the fieldmap or an instrument is missing an 'event' "
    "attribute"
)


class RedcapExportException(Exception):
    pass


class RedcapFieldmap(object):
    """
    Internal representation of the fieldmap XML file.
    This describes how the task fields should be translated to
    the REDCap record.
    """

    def __init__(self, filename: str) -> None:
        """
        Args:
            filename:
                Name of an XML file telling CamCOPS how to map task fields
                to REDCap. See :ref:`REDCap export <redcap>`.
        """
        self.filename = filename
        self.fields = {}  # type: Dict[str, Dict[str, str]]
        # ... {task: {name: formula}}
        self.files = {}  # type: Dict[str, Dict[str, str]]
        # ... {task: {name: formula}}
        self.instruments = {}  # type: Dict[str, str]
        # ... {task: instrument_name}
        self.events = {}  # type: Dict[str, str]
        # ... {task: event_name}

        parser = ElementTree.XMLParser(encoding="UTF-8")
        try:
            tree = ElementTree.parse(filename, parser=parser)
        except FileNotFoundError:
            raise RedcapExportException(
                f"Unable to open fieldmap file '{filename}'"
            )
        except ElementTree.ParseError as e:
            raise RedcapExportException(
                f"There was a problem parsing {filename}: {str(e)}"
            ) from e

        root = tree.getroot()
        if root.tag != "fieldmap":
            raise RedcapExportException(
                (f"Expected the root tag to be 'fieldmap' instead of "
                 f"'{root.tag}' in {filename}")
            )

        patient_element = root.find("patient")
        if patient_element is None:
            raise RedcapExportException(
                f"'patient' is missing from {filename}"
            )

        self.patient = self._validate_and_return_attributes(
            patient_element, ("instrument", "redcap_field")
        )

        record_element = root.find("record")
        if record_element is None:
            raise RedcapExportException(
                f"'record' is missing from {filename}"
            )

        self.record = self._validate_and_return_attributes(
            record_element, ("instrument", "redcap_field")
        )

        default_event = None
        event_element = root.find("event")
        if event_element is not None:
            event_attributes = self._validate_and_return_attributes(
                event_element, ("name",)
            )
            default_event = event_attributes['name']

        instrument_elements = root.find("instruments")
        if instrument_elements is None:
            raise RedcapExportException(
                f"'instruments' tag is missing from {filename}"
            )

        for instrument_element in instrument_elements:
            instrument_attributes = self._validate_and_return_attributes(
                instrument_element, ("name", "task")
            )

            task = instrument_attributes["task"]
            instrument_name = instrument_attributes["name"]
            self.fields[task] = {}
            self.files[task] = {}
            self.events[task] = instrument_attributes.get("event",
                                                          default_event)
            self.instruments[task] = instrument_name

            field_elements = instrument_element.find("fields") or []

            for field_element in field_elements:
                field_attributes = self._validate_and_return_attributes(
                    field_element, ("name", "formula")
                )
                name = field_attributes["name"]
                formula = field_attributes["formula"]

                self.fields[task][name] = formula

            file_elements = instrument_element.find("files") or []
            for file_element in file_elements:
                file_attributes = self._validate_and_return_attributes(
                    file_element, ("name", "formula")
                )

                name = file_attributes["name"]
                formula = file_attributes["formula"]
                self.files[task][name] = formula

    def _validate_and_return_attributes(
            self, element: ElementTree.Element,
            expected_attributes: Iterable[str]) -> Dict[str, str]:
        """
        Checks that all the expected attributes are present in the XML element
        (from the fieldmap XML file), or raises :exc:`RedcapExportException`.
        """
        attributes = element.attrib

        if not all(a in attributes.keys() for a in expected_attributes):
            raise RedcapExportException(
                (f"'{element.tag}' must have attributes: "
                 f"{', '.join(expected_attributes)} in {self.filename}")
            )

        return attributes

    def instrument_names(self) -> List[str]:
        """
        Returns the names of all REDCap instruments.
        """
        return list(self.instruments.values())


class RedcapTaskExporter(object):
    """
    Main entry point for task export to REDCap. Works out which record needs
    updating or creating. Creates the fieldmap and initiates upload.
    """
    def export_task(self,
                    req: "CamcopsRequest",
                    exported_task_redcap: "ExportedTaskRedcap") -> None:
        """
        Exports a specific task.

        Args:
            req:
                a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            exported_task_redcap:
                a :class:`camcops_server.cc_modules.cc_exportmodels.ExportedTaskRedcap`
        """  # noqa
        exported_task = exported_task_redcap.exported_task
        recipient = exported_task.recipient
        task = exported_task.task

        if task.is_anonymous:
            raise RedcapExportException(
                f"Skipping anonymous task '{task.tablename}'"
            )

        which_idnum = recipient.primary_idnum
        idnum_object = task.patient.get_idnum_object(which_idnum)

        project = self.get_project(recipient)
        fieldmap = self.get_fieldmap(recipient)

        if project.is_longitudinal():
            if not all(fieldmap.events.values()):
                raise RedcapExportException(MISSING_EVENT_TAG_OR_ATTRIBUTE)

        existing_records = self._get_existing_records(project, fieldmap)
        existing_record_id = self._get_existing_record_id(
            existing_records,
            fieldmap,
            idnum_object.idnum_value
        )

        if existing_record_id is None:
            uploader_class = RedcapNewRecordUploader
        else:
            uploader_class = RedcapUpdatedRecordUploader

        try:
            instrument_name = fieldmap.instruments[task.tablename]
        except KeyError:
            raise RedcapExportException(
                (f"Instrument for task '{task.tablename}' is missing from the "
                 f"fieldmap")
            )

        record_id_fieldname = fieldmap.record["redcap_field"]

        next_instance_id = self._get_next_instance_id(existing_records,
                                                      instrument_name,
                                                      record_id_fieldname,
                                                      existing_record_id)

        uploader = uploader_class(req, project)

        new_record_id = uploader.upload(task, existing_record_id,
                                        next_instance_id,
                                        fieldmap, idnum_object.idnum_value)

        exported_task_redcap.redcap_record_id = new_record_id
        exported_task_redcap.redcap_instrument_name = instrument_name
        exported_task_redcap.redcap_instance_id = next_instance_id

    @staticmethod
    def _get_existing_records(project: redcap.project.Project,
                              fieldmap: RedcapFieldmap) -> "DataFrame":
        """
        Returns a Pandas data frame containing existing REDCap records for this
        project, for instruments we are interested in.

        Args:
            project:
                a :class:`redcap.project.Project`
            fieldmap:
                a :class:`RedcapFieldmap`
        """
        # Arguments to pandas read_csv()

        type_dict = {
            # otherwise pandas may infer as int or str
            fieldmap.record["redcap_field"]: str,
        }

        df_kwargs = {
            "index_col": None,  # don't index by record_id
            "dtype": type_dict,
        }

        forms = (fieldmap.instrument_names() +
                 [fieldmap.patient["instrument"]] +
                 [fieldmap.record["instrument"]])

        try:
            records = project.export_records(format="df", forms=forms,
                                             df_kwargs=df_kwargs)
        except EmptyDataError:
            # Should not happen, but in case of PyCap failing to catch this...
            return DataFrame()
        except redcap.RedcapError as e:
            raise RedcapExportException(str(e))

        return records

    @staticmethod
    def _get_existing_record_id(records: "DataFrame",
                                fieldmap: RedcapFieldmap,
                                idnum_value: int) -> Optional[str]:
        """
        Returns the ID of an existing record that matches a specific
        patient, if one can be found.

        Args:
            records:
                records retrieved from REDCap; Pandas data frame from
                :meth:`_get_existing_records`
            fieldmap:
                :class:`RedcapFieldmap`
            idnum_value:
                CamCOPS patient ID number

        Returns:
            REDCap record ID or ``None``
        """

        if records.empty:
            return None

        patient_id_fieldname = fieldmap.patient["redcap_field"]

        if patient_id_fieldname not in records:
            raise RedcapExportException(
                (f"Field '{patient_id_fieldname}' does not exist in REDCap. "
                 f"Is the 'patient' tag in the fieldmap correct?")
            )

        with_identifier = records[patient_id_fieldname] == idnum_value

        if len(records[with_identifier]) == 0:
            return None

        return records[with_identifier].iat[0, 0]

    @staticmethod
    def _get_next_instance_id(records: "DataFrame",
                              instrument: str,
                              record_id_fieldname: str,
                              existing_record_id: Optional[str]) -> int:
        """
        Returns the next REDCap record ID to use for a particular instrument,
        including for a repeating instrument (the previous highest ID plus 1,
        or 1 if none can be found).

        Args:
            records:
                records retrieved from REDCap; Pandas data frame from
                :meth:`_get_existing_records`
            instrument:
                instrument name
            existing_record_id:
                ID of existing record
        """
        if existing_record_id is None:
            return 1

        if record_id_fieldname not in records:
            raise RedcapExportException(
                (f"Field '{record_id_fieldname}' does not exist in REDCap. "
                 f"Is the 'record' tag in the fieldmap correct?")
            )

        previous_instances = records[
            (records["redcap_repeat_instrument"] == instrument) &
            (records[record_id_fieldname] == existing_record_id)
        ]

        if len(previous_instances) == 0:
            return 1

        return int(previous_instances.max()["redcap_repeat_instance"] + 1)

    def get_fieldmap(self, recipient: ExportRecipient) -> RedcapFieldmap:
        """
        Returns the relevant :class:`RedcapFieldmap`.

        Args:
            recipient:
                an
                :class:`camcops_server.cc_modules.cc_exportmodels.ExportRecipient`
        """  # noqa
        fieldmap = RedcapFieldmap(self.get_fieldmap_filename(recipient))

        return fieldmap

    @staticmethod
    def get_fieldmap_filename(recipient: ExportRecipient) -> str:
        """
        Returns the name of the XML file containing our fieldmap details, or
        raises :exc:`RedcapExportException`.

        Args:
            recipient:
                an
                :class:`camcops_server.cc_modules.cc_exportmodels.ExportRecipient`
        """  # noqa
        filename = recipient.redcap_fieldmap_filename
        if filename is None:
            raise RedcapExportException(
                f"{ConfigParamExportRecipient.REDCAP_FIELDMAP_FILENAME} "
                f"is not set in the config file"
            )

        if filename == "":
            raise RedcapExportException(
                f"{ConfigParamExportRecipient.REDCAP_FIELDMAP_FILENAME} "
                f"is empty in the config file"
            )

        return filename

    @staticmethod
    def get_project(recipient: ExportRecipient) -> redcap.project.Project:
        """
        Returns the :class:`redcap.project.Project`.

        Args:
            recipient:
                an
                :class:`camcops_server.cc_modules.cc_exportmodels.ExportRecipient`
        """
        try:
            project = redcap.project.Project(
                recipient.redcap_api_url, recipient.redcap_api_key
            )
        except redcap.RedcapError as e:
            raise RedcapExportException(str(e))

        return project


class RedcapRecordStatus(Enum):
    """
    Corresponds to valid values of Form Status -> Complete? field in REDCap
    """
    INCOMPLETE = 0
    UNVERIFIED = 1
    COMPLETE = 2


class RedcapUploader(object):
    """
    Uploads records and files into REDCap, transforming the fields via the
    fieldmap.

    Abstract base class.

    Knows nothing about ExportedTaskRedcap, ExportedTask, ExportRecipient
    """
    def __init__(self,
                 req: "CamcopsRequest",
                 project: "redcap.project.Project") -> None:
        """

        Args:
            req:
                a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            project:
                a :class:`redcap.project.Project`
        """
        self.req = req
        self.project = project
        self.project_info = project.export_project_info()

    def get_record_id(self, existing_record_id: Optional[str]) -> str:
        """
        Returns the REDCap record ID to use.

        Args:
            existing_record_id: highest existing record ID, if known
        """
        raise NotImplementedError("implement in subclass")

    @property
    def return_content(self) -> str:
        """
        The ``return_content`` argument to be passed to
        :meth:`redcap.project.Project.import_records`. Can be:

        - ``count`` [default] - the number of records imported
        - ``ids`` - a list of all record IDs that were imported
        - ``auto_ids`` = (used only when ``forceAutoNumber=true``) a list of
           pairs of all record IDs that were imported, includes the new ID
           created and the ID value that was sent in the API request
           (e.g., 323,10).

        Note (2020-01-27) that it can return e.g. ``15-30,0``, i.e. the ID
        values can be non-integer.
        """
        raise NotImplementedError("implement in subclass")

    @property
    def force_auto_number(self) -> bool:
        """
        Should we force auto-numbering of records in REDCap?
        """
        raise NotImplementedError("implement in subclass")

    def get_new_record_id(self, record_id: str, response: List[str]) -> str:
        """
        Returns the ID of the new (or updated) REDCap record.

        Args:
            record_id:
                existing record ID
            response:
                response from :meth:`redcap.project.Project.import_records`
        """
        raise NotImplementedError("implement in subclass")

    @staticmethod
    def log_success(record_id: str) -> None:
        """
        Report upload success to the Python log.

        Args:
            record_id: REDCap record ID
        """
        raise NotImplementedError("implement in subclass")

    @property
    def autonumbering_enabled(self) -> bool:
        """
        Does this REDCap project have record autonumbering enabled?
        """
        return self.project_info['record_autonumbering_enabled']

    def upload(self, task: "Task", existing_record_id: Optional[str],
               next_instance_id: int, fieldmap: RedcapFieldmap,
               idnum_value: int) -> str:
        """
        Uploads a CamCOPS task to REDCap.

        Args:
            task:
                :class:`camcops_server.cc_modules.cc_task.Task` to be uploaded
            existing_record_id:
                REDCap ID of the existing record, if there is one
            next_instance_id:
                REDCap instance ID to be used for a repeating instrument
            fieldmap:
                :class:`RedcapFieldmap`
            idnum_value:
                CamCOPS patient ID number

        Returns:
            str: REDCap record ID of the record that was created or updated

        """
        complete_status = RedcapRecordStatus.INCOMPLETE

        if task.is_complete():
            complete_status = RedcapRecordStatus.COMPLETE
        instrument_name = fieldmap.instruments[task.tablename]
        record_id_fieldname = fieldmap.record["redcap_field"]

        record_id = self.get_record_id(existing_record_id)

        record = {
            record_id_fieldname: record_id,
            "redcap_repeat_instrument": instrument_name,
            # https://community.projectredcap.org/questions/74561/unexpected-behaviour-with-import-records-repeat-in.html  # noqa
            # REDCap won't create instance IDs automatically so we have to
            # assume no one else is writing to this record
            "redcap_repeat_instance": next_instance_id,
            f"{instrument_name}_complete": complete_status.value,
            "redcap_event_name": fieldmap.events[task.tablename]
        }

        self.transform_fields(record, task, fieldmap.fields[task.tablename])

        import_kwargs = {
            "return_content": self.return_content,
            "force_auto_number": self.force_auto_number,
        }

        response = self.upload_record(record, **import_kwargs)

        new_record_id = self.get_new_record_id(record_id, response)

        # We don't mark the patient record as complete - it could be part of
        # a larger form. We don't require it to be complete.
        patient_record = {
            record_id_fieldname: new_record_id,
            fieldmap.patient["redcap_field"]: idnum_value,
        }
        self.upload_record(patient_record)

        file_dict = {}
        self.transform_fields(file_dict, task, fieldmap.files[task.tablename])

        self.upload_files(task,
                          new_record_id,
                          next_instance_id,
                          file_dict,
                          event=fieldmap.events[task.tablename])

        self.log_success(new_record_id)

        return new_record_id

    def upload_record(self, record: Dict[str, Any],
                      **kwargs) -> Union[Dict, List, str]:
        """
        Uploads a REDCap record via the pycap
        :func:`redcap.project.Project.import_record` function. Returns its
        response.
        """
        try:
            response = self.project.import_records(
                [record],
                **kwargs
            )
        except redcap.RedcapError as e:
            raise RedcapExportException(str(e))

        return response

    def upload_files(self, task: "Task", record_id: Union[int, str],
                     repeat_instance: int,
                     file_dict: Dict[str, bytes],
                     event: Optional[str] = None) -> None:
        """
        Uploads files attached to a task (e.g. a PDF of the CamCOPS task).

        Args:
            task:
                the :class:`camcops_server.cc_modules.cc_task.Task`
            record_id:
                the REDCap record ID
            repeat_instance:
                instance number for repeating instruments
            file_dict:
                dictionary mapping filename to file contents
            event:
                for longitudinal projects, specify the unique event here

        Raises:
            :exc:`RedcapExportException`
        """
        for fieldname, value in file_dict.items():
            with io.BytesIO(value) as file_obj:
                filename = f"{task.tablename}_{record_id}_{fieldname}"

                try:
                    self.project.import_file(
                        record_id, fieldname, filename, file_obj,
                        event=event,
                        repeat_instance=repeat_instance
                    )
                # ValueError if the field does not exist or is not
                # a file field
                except (redcap.RedcapError, ValueError) as e:
                    raise RedcapExportException(str(e))

    def transform_fields(self, field_dict: Dict[str, Any], task: "Task",
                         formula_dict: Dict[str, str]) -> None:
        """
        Uses the definitions from the fieldmap XML to set up field values to be
        exported to REDCap.

        Args:
            field_dict:
                Exported field values go here (the dictionary is modified).
            task:
                the :class:`camcops_server.cc_modules.cc_task.Task`
            formula_dict:
                dictionary (from the XML information) mapping REDCap field
                name to a "formula". The formula is applied to extract data
                from the task in a flexible way.
        """
        extra_symbols = self.get_extra_symbols()

        symbol_table = make_symbol_table(
            task=task,
            **extra_symbols
        )
        interpreter = Interpreter(symtable=symbol_table)

        for redcap_field, formula in formula_dict.items():
            v = interpreter(f"{formula}", show_errors=True)
            if interpreter.error:
                message = "\n".join([e.msg for e in interpreter.error])
                raise RedcapExportException(
                    (
                        f"Fieldmap:\n"
                        f"Error in formula '{formula}': {message}\n"
                        f"Task: '{task.tablename}'\n"
                        f"REDCap field: '{redcap_field}'\n"
                    )
                )
            field_dict[redcap_field] = v

    def get_extra_symbols(self) -> Dict[str, Any]:
        """
        Returns a dictionary made available to the ``asteval`` interpreter.
        These become variables that the system administrator can refer to in
        their fieldmap XML; see :ref:`REDCap export <redcap>`.
        """
        return dict(
            format_datetime=format_datetime,
            DateFormat=DateFormat,
            request=self.req
        )


class RedcapNewRecordUploader(RedcapUploader):
    """
    Creates a new REDCap record.
    """

    @property
    def force_auto_number(self) -> bool:
        return self.autonumbering_enabled

    @property
    def return_content(self) -> str:
        if self.autonumbering_enabled:
            # import_records returns ["<redcap record id>, 0"]
            return "auto_ids"

        # import_records returns {'count': 1}
        return "count"

    # noinspection PyUnusedLocal
    def get_record_id(self, existing_record_id: str) -> str:
        """
        Get the record ID to send to REDCap when importing records
        """
        if self.autonumbering_enabled:
            # Is ignored but we still need to set this to something
            return "0"

        return self.project.generate_next_record_name()

    def get_new_record_id(self, record_id: str, response: List[str]) -> str:
        """
        For autonumbering, read the generated record ID from the
        response. Otherwise we already have it.
        """
        if not self.autonumbering_enabled:
            return record_id

        id_pair = response[0]

        record_id = id_pair.rsplit(",")[0]

        return record_id

    @staticmethod
    def log_success(record_id: str) -> None:
        log.info(f"Created new REDCap record {record_id}")


class RedcapUpdatedRecordUploader(RedcapUploader):
    """
    Updates an existing REDCap record.
    """
    force_auto_number = False
    # import_records returns {'count': 1}
    return_content = "count"

    # noinspection PyMethodMayBeStatic
    def get_record_id(self, existing_record_id: str) -> str:
        return existing_record_id

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def get_new_record_id(self, old_record_id: str, response: Any) -> str:
        return old_record_id

    @staticmethod
    def log_success(record_id: str) -> None:
        log.info(f"Updated REDCap record {record_id}")
