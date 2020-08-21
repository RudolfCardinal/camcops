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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

**Implements communication with REDCap.**

- For general information about REDCap, see https://www.project-redcap.org/.

- The API documentation is not provided there, but is available from
  your local REDCap server. Pick a project. Choose "API" from the left-hand
  menu. Follow the "REDCap API documentation" link.

- We use PyCap (https://pycap.readthedocs.io/ or
  https://github.com/redcap-tools/PyCap). See also
  http://redcap-tools.github.io/projects/. PyCap is no longer being actively
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
import os
import tempfile
from typing import (
    Any,
    Dict,
    Generator,
    Iterable,
    List,
    Optional,
    TYPE_CHECKING,
    Union,
)
from unittest import mock, TestCase
import xml.etree.cElementTree as ElementTree

from asteval import Interpreter, make_symbol_table
from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.logs import BraceStyleAdapter
from pandas import DataFrame
from pandas.errors import EmptyDataError
import pendulum
import redcap

from camcops_server.cc_modules.cc_constants import (
    ConfigParamExportRecipient,
    DateFormat,
)
from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient
from camcops_server.cc_modules.cc_exportrecipientinfo import ExportRecipientInfo
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_exportmodels import ExportedTaskRedcap
    from camcops_server.cc_modules.cc_request import CamcopsRequest
    from camcops_server.cc_modules.cc_patient import Patient
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


# =============================================================================
# Unit testing
# =============================================================================

class MockProject(mock.Mock):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

        self.export_project_info = mock.Mock()
        self.export_records = mock.Mock()
        self.generate_next_record_name = mock.Mock()
        self.import_file = mock.Mock()
        self.import_records = mock.Mock()
        self.is_longitudinal = mock.Mock(return_value=False)


class MockRedcapTaskExporter(RedcapTaskExporter):
    def __init__(self) -> None:
        mock_project = MockProject()
        self.get_project = mock.Mock(return_value=mock_project)

        config = mock.Mock()
        self.req = mock.Mock(config=config)


class MockRedcapNewRecordUploader(RedcapNewRecordUploader):
    # noinspection PyMissingConstructor
    def __init__(self) -> None:
        self.req = mock.Mock()
        self.project = MockProject()
        self.task = mock.Mock(tablename="mock_task")


class RedcapExporterTests(TestCase):
    def test_next_instance_id_converted_to_int(self) -> None:
        import numpy

        records = DataFrame({
            "record_id": ["1", "1", "1", "1", "1"],
            "redcap_repeat_instrument": ["bmi", "bmi", "bmi", "bmi", "bmi"],
            "redcap_repeat_instance": [
                numpy.float64(1.0),
                numpy.float64(2.0),
                numpy.float64(3.0),
                numpy.float64(4.0),
                numpy.float64(5.0),
            ],

        })

        next_instance_id = RedcapTaskExporter._get_next_instance_id(
            records, "bmi", "record_id", "1"
        )

        self.assertEqual(next_instance_id, 6)
        self.assertEqual(type(next_instance_id), int)


class RedcapExportErrorTests(TestCase):
    def test_raises_when_fieldmap_has_unknown_symbols(self) -> None:
        exporter = MockRedcapNewRecordUploader()

        task = mock.Mock(tablename="bmi")
        fieldmap = {"pa_height": "sys.platform"}

        field_dict = {}

        with self.assertRaises(RedcapExportException) as cm:
            exporter.transform_fields(field_dict, task, fieldmap)

        message = str(cm.exception)
        self.assertIn("Error in formula 'sys.platform':", message)
        self.assertIn("Task: 'bmi'", message)
        self.assertIn("REDCap field: 'pa_height'", message)
        self.assertIn("'sys' is not defined", message)

    def test_raises_when_fieldmap_empty_in_config(self) -> None:

        exporter = MockRedcapTaskExporter()

        recipient = mock.Mock(redcap_fieldmap_filename="")
        with self.assertRaises(RedcapExportException) as cm:
            exporter.get_fieldmap_filename(recipient)

        message = str(cm.exception)
        self.assertIn(f"{ConfigParamExportRecipient.REDCAP_FIELDMAP_FILENAME} "
                      f"is empty in the config file", message)

    def test_raises_when_fieldmap_not_set_in_config(self) -> None:

        exporter = MockRedcapTaskExporter()

        recipient = mock.Mock(redcap_fieldmap_filename=None)
        with self.assertRaises(RedcapExportException) as cm:
            exporter.get_fieldmap_filename(recipient)

        message = str(cm.exception)
        self.assertIn(f"{ConfigParamExportRecipient.REDCAP_FIELDMAP_FILENAME} "
                      f"is not set in the config file", message)

    def test_raises_when_error_from_redcap_on_import(self) -> None:
        exporter = MockRedcapNewRecordUploader()
        exporter.project.import_records.side_effect = redcap.RedcapError(
            "Something went wrong"
        )

        with self.assertRaises(RedcapExportException) as cm:
            record = {}
            exporter.upload_record(record)
        message = str(cm.exception)

        self.assertIn("Something went wrong", message)

    def test_raises_when_error_from_redcap_on_init(self) -> None:
        with mock.patch("redcap.project.Project.__init__") as mock_init:
            mock_init.side_effect = redcap.RedcapError(
                "Something went wrong"
            )

            with self.assertRaises(RedcapExportException) as cm:
                exporter = RedcapTaskExporter()
                recipient = mock.Mock()
                exporter.get_project(recipient)

            message = str(cm.exception)

            self.assertIn("Something went wrong", message)

    def test_raises_when_field_not_a_file_field(self) -> None:
        exporter = MockRedcapNewRecordUploader()
        exporter.project.import_file.side_effect = ValueError(
            "Error with file field"
        )

        task = mock.Mock()

        with self.assertRaises(RedcapExportException) as cm:
            record_id = 1
            repeat_instance = 1
            file_dict = {"medication_items": b"not a real file"}
            exporter.upload_files(task, record_id, repeat_instance, file_dict)
        message = str(cm.exception)

        self.assertIn("Error with file field", message)

    def test_raises_when_error_from_redcap_on_import_file(self) -> None:
        exporter = MockRedcapNewRecordUploader()
        exporter.project.import_file.side_effect = redcap.RedcapError(
            "Something went wrong"
        )

        task = mock.Mock()

        with self.assertRaises(RedcapExportException) as cm:
            record_id = 1
            repeat_instance = 1
            file_dict = {"medication_items": b"not a real file"}
            exporter.upload_files(task, record_id, repeat_instance, file_dict)
        message = str(cm.exception)

        self.assertIn("Something went wrong", message)


class RedcapFieldmapTests(TestCase):
    def test_raises_when_xml_file_missing(self) -> None:
        with self.assertRaises(RedcapExportException) as cm:
            RedcapFieldmap("/does/not/exist/bmi.xml")

        message = str(cm.exception)

        self.assertIn("Unable to open fieldmap file", message)
        self.assertIn("bmi.xml", message)

    def test_raises_when_fieldmap_missing(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write("""<?xml version="1.0" encoding="UTF-8"?>
<someothertag></someothertag>
""")
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn(("Expected the root tag to be 'fieldmap' instead of "
                       "'someothertag'"), message)
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_root_tag_missing(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write("""<?xml version="1.0" encoding="UTF-8"?>
""")
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn("There was a problem parsing", message)
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_patient_missing(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                </fieldmap>
                """)
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn("'patient' is missing from", message)
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_patient_missing_attributes(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                <patient />
                </fieldmap>
                """)
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn(
            "'patient' must have attributes: instrument, redcap_field",
            message
        )
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_record_missing(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                    <patient instrument="patient_record" redcap_field="patient_id" />
                </fieldmap>
                """)  # noqa: E501
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn("'record' is missing from", message)
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_record_missing_attributes(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                   <patient instrument="patient_record" redcap_field="patient_id" />
                   <record />
                </fieldmap>
                """)  # noqa: E501
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn(
            "'record' must have attributes: instrument, redcap_field",
            message
        )
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_instruments_missing(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                    <patient instrument="patient_record" redcap_field="patient_id" />
                    <record instrument="patient_record" redcap_field="record_id" />
                </fieldmap>
                """)  # noqa: E501
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn("'instruments' tag is missing from", message)
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_instruments_missing_attributes(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                    <patient instrument="patient_record" redcap_field="patient_id" />
                    <record instrument="patient_record" redcap_field="record_id" />
                    <instruments>
                        <instrument />
                    </instruments>
                </fieldmap>
                """)  # noqa: E501
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn(
            "'instrument' must have attributes: name, task",
            message
        )
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_file_fields_missing_attributes(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                    <patient instrument="patient_record" redcap_field="patient_id" />
                    <record instrument="patient_record" redcap_field="record_id" />
                    <instruments>
                        <instrument name="bmi" task="bmi">
                            <files>
                                <field />
                            </files>
                        </instrument>
                    </instruments>
                </fieldmap>
                """)  # noqa: E501
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn(
            "'field' must have attributes: name, formula",
            message
        )
        self.assertIn(fieldmap_file.name, message)

    def test_raises_when_fields_missing_attributes(self) -> None:
        with tempfile.NamedTemporaryFile(
                mode="w", suffix="xml") as fieldmap_file:
            fieldmap_file.write(
                """<?xml version="1.0" encoding="UTF-8"?>
                <fieldmap>
                    <patient instrument="patient_record" redcap_field="patient_id" />
                    <record instrument="patient_record" redcap_field="record_id" />
                    <instruments>
                        <instrument name="bmi" task="bmi">
                            <fields>
                                <field />
                            </fields>
                        </instrument>
                    </instruments>
                </fieldmap>
                """)  # noqa: E501
            fieldmap_file.flush()

            with self.assertRaises(RedcapExportException) as cm:
                RedcapFieldmap(fieldmap_file.name)

        message = str(cm.exception)
        self.assertIn(
            "'field' must have attributes: name, formula",
            message
        )
        self.assertIn(fieldmap_file.name, message)


# =============================================================================
# Integration testing
# =============================================================================

class RedcapExportTestCase(DemoDatabaseTestCase):
    fieldmap = ""

    def setUp(self) -> None:
        recipientinfo = ExportRecipientInfo()

        self.recipient = ExportRecipient(recipientinfo)
        self.recipient.primary_idnum = 1001

        # auto increment doesn't work for BigInteger with SQLite
        self.recipient.id = 1
        self.recipient.recipient_name = "test"
        self.recipient.redcap_fieldmap_filename = os.path.join(
            self.tmpdir_obj.name, "redcap_fieldmap.xml"
        )
        self.write_fieldmaps(self.recipient.redcap_fieldmap_filename)

        super().setUp()

    def write_fieldmaps(self, filename: str) -> None:
        with open(filename, "w") as f:
            f.write(self.fieldmap)

    def create_patient_with_idnum_1001(self) -> "Patient":
        from camcops_server.cc_modules.cc_patient import Patient
        from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
        patient = Patient()
        patient.id = 2
        self._apply_standard_db_fields(patient)
        patient.forename = "Forename2"
        patient.surname = "Surname2"
        patient.dob = pendulum.parse("1975-12-12")
        self.dbsession.add(patient)
        patient_idnum1 = PatientIdNum()
        patient_idnum1.id = 3
        self._apply_standard_db_fields(patient_idnum1)
        patient_idnum1.patient_id = patient.id
        patient_idnum1.which_idnum = 1001
        patient_idnum1.idnum_value = 555
        self.dbsession.add(patient_idnum1)
        self.dbsession.commit()

        return patient


class BmiRedcapExportTestCase(RedcapExportTestCase):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1


class BmiRedcapValidFieldmapTestCase(BmiRedcapExportTestCase):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="patient_id" />
  <record instrument="instrument_with_record_id" redcap_field="record_id" />
  <instruments>
    <instrument task="bmi" name="bmi">
      <fields>
        <field name="pa_height" formula="format(task.height_m, '.1f')" />
        <field name="pa_weight" formula="format(task.mass_kg, '.1f')" />
        <field name="bmi_date" formula="format_datetime(task.when_created, DateFormat.ISO8601_DATE_ONLY)" />
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501


class BmiRedcapExportTests(BmiRedcapValidFieldmapTestCase):
    """
    These are more of a test of the fieldmap code than anything
    related to the BMI task
    """

    def create_tasks(self) -> None:
        from camcops_server.tasks.bmi import Bmi

        patient = self.create_patient_with_idnum_1001()
        self.task = Bmi()
        self.apply_standard_task_fields(self.task)
        self.task.id = next(self.id_sequence)
        self.task.height_m = 1.83
        self.task.mass_kg = 67.57
        self.task.patient_id = patient.id
        self.dbsession.add(self.task)
        self.dbsession.commit()

    def test_record_exported(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        exporter.export_task(self.req, exported_task_redcap)
        self.assertEquals(exported_task_redcap.redcap_record_id, "123")
        self.assertEquals(exported_task_redcap.redcap_instrument_name, "bmi")
        self.assertEquals(exported_task_redcap.redcap_instance_id, 1)

        args, kwargs = project.export_records.call_args

        self.assertIn("bmi", kwargs['forms'])
        self.assertIn("patient_record", kwargs['forms'])
        self.assertIn("instrument_with_record_id", kwargs['forms'])

        # Initial call with original record
        args, kwargs = project.import_records.call_args_list[0]

        rows = args[0]
        record = rows[0]

        self.assertEquals(record["redcap_repeat_instrument"], "bmi")
        self.assertEquals(record["redcap_repeat_instance"], 1)
        self.assertEquals(record["record_id"], "0")
        self.assertEquals(record["bmi_complete"],
                          RedcapRecordStatus.COMPLETE.value)
        self.assertEquals(record["bmi_date"], "2010-07-07")

        self.assertEquals(record["pa_height"], "1.8")
        self.assertEquals(record["pa_weight"], "67.6")

        self.assertEquals(kwargs["return_content"], "auto_ids")
        self.assertTrue(kwargs["force_auto_number"])

        # Second call with updated patient ID
        args, kwargs = project.import_records.call_args_list[1]
        rows = args[0]
        record = rows[0]

        self.assertEquals(record["patient_id"], 555)

    def test_record_exported_with_non_integer_id(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["15-123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        exporter.export_task(self.req, exported_task_redcap)
        self.assertEquals(exported_task_redcap.redcap_record_id, "15-123")

    def test_record_id_generated_when_no_autonumbering(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = {"count": 1}
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 0
        }
        project.generate_next_record_name.return_value = "15-29"

        exporter.export_task(self.req, exported_task_redcap)

        # Initial call with original record
        args, kwargs = project.import_records.call_args_list[0]

        rows = args[0]
        record = rows[0]

        self.assertEquals(record["record_id"], "15-29")
        self.assertEquals(kwargs["return_content"], "count")
        self.assertFalse(kwargs["force_auto_number"])

    def test_record_imported_when_no_existing_records(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap,
        )

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame()
        project.import_records.return_value = ["1,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)
        exporter.export_task(self.req, exported_task_redcap)

        self.assertEquals(exported_task_redcap.redcap_record_id, "1")
        self.assertEquals(exported_task_redcap.redcap_instrument_name, "bmi")
        self.assertEquals(exported_task_redcap.redcap_instance_id, 1)


class BmiRedcapUpdateTests(BmiRedcapValidFieldmapTestCase):
    def create_tasks(self) -> None:
        from camcops_server.tasks.bmi import Bmi
        patient = self.create_patient_with_idnum_1001()
        self.task1 = Bmi()
        self.apply_standard_task_fields(self.task1)
        self.task1.id = next(self.id_sequence)
        self.task1.height_m = 1.83
        self.task1.mass_kg = 67.57
        self.task1.patient_id = patient.id
        self.dbsession.add(self.task1)

        self.task2 = Bmi()
        self.apply_standard_task_fields(self.task2)
        self.task2.id = next(self.id_sequence)
        self.task2.height_m = 1.83
        self.task2.mass_kg = 68.5
        self.task2.patient_id = patient.id
        self.dbsession.add(self.task2)
        self.dbsession.commit()

    def test_existing_record_id_used_for_update(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap,
        )

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        exported_task1 = ExportedTask(task=self.task1, recipient=self.recipient)
        exported_task_redcap1 = ExportedTaskRedcap(exported_task1)
        exporter.export_task(self.req, exported_task_redcap1)
        self.assertEquals(exported_task_redcap1.redcap_record_id, "123")
        self.assertEquals(exported_task_redcap1.redcap_instrument_name, "bmi")
        self.assertEquals(exported_task_redcap1.redcap_instance_id, 1)

        project.export_records.return_value = DataFrame({
            "record_id": ["123"],
            "patient_id": [555],
            "redcap_repeat_instrument": ["bmi"],
            "redcap_repeat_instance": [1],
        })
        exported_task2 = ExportedTask(task=self.task2, recipient=self.recipient)
        exported_task_redcap2 = ExportedTaskRedcap(exported_task2)

        exporter.export_task(self.req, exported_task_redcap2)
        self.assertEquals(exported_task_redcap2.redcap_record_id, "123")
        self.assertEquals(exported_task_redcap2.redcap_instrument_name, "bmi")
        self.assertEquals(exported_task_redcap2.redcap_instance_id, 2)

        # Third call (after initial record and patient ID)
        args, kwargs = project.import_records.call_args_list[2]

        rows = args[0]
        record = rows[0]

        self.assertEquals(record["record_id"], "123")
        self.assertEquals(record["redcap_repeat_instance"], 2)
        self.assertEquals(kwargs["return_content"], "count")
        self.assertFalse(kwargs["force_auto_number"])


class Phq9RedcapExportTests(RedcapExportTestCase):
    """
    These are more of a test of the fieldmap code than anything
    related to the PHQ9 task. For these we have also renamed the record_id
    field.
    """
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="patient_id" />
  <record instrument="patient_record" redcap_field="my_record_id" />
  <instruments>
    <instrument task="phq9" name="patient_health_questionnaire_9">
      <fields>
        <field name="phq9_how_difficult" formula="task.q10 + 1" />
        <field name="phq9_total_score" formula="task.total_score()" />
        <field name="phq9_first_name" formula="task.patient.forename" />
        <field name="phq9_last_name" formula="task.patient.surname" />
        <field name="phq9_date_enrolled" formula="format_datetime(task.when_created,DateFormat.ISO8601_DATE_ONLY)" />
        <field name="phq9_1" formula="task.q1" />
        <field name="phq9_2" formula="task.q2" />
        <field name="phq9_3" formula="task.q3" />
        <field name="phq9_4" formula="task.q4" />
        <field name="phq9_5" formula="task.q5" />
        <field name="phq9_6" formula="task.q6" />
        <field name="phq9_7" formula="task.q7" />
        <field name="phq9_8" formula="task.q8" />
        <field name="phq9_9" formula="task.q9" />
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1

    def create_tasks(self) -> None:
        from camcops_server.tasks.phq9 import Phq9
        patient = self.create_patient_with_idnum_1001()
        self.task = Phq9()
        self.apply_standard_task_fields(self.task)
        self.task.id = next(self.id_sequence)
        self.task.q1 = 0
        self.task.q2 = 1
        self.task.q3 = 2
        self.task.q4 = 3
        self.task.q5 = 0
        self.task.q6 = 1
        self.task.q7 = 2
        self.task.q8 = 3
        self.task.q9 = 0
        self.task.q10 = 3
        self.task.patient_id = patient.id
        self.dbsession.add(self.task)
        self.dbsession.commit()

    def test_record_exported(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap,
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        exporter.export_task(self.req, exported_task_redcap)
        self.assertEquals(exported_task_redcap.redcap_record_id, "123")
        self.assertEquals(exported_task_redcap.redcap_instrument_name,
                          "patient_health_questionnaire_9")
        self.assertEquals(exported_task_redcap.redcap_instance_id, 1)

        # Initial call with new record
        args, kwargs = project.import_records.call_args_list[0]

        rows = args[0]
        record = rows[0]

        self.assertEquals(record["redcap_repeat_instrument"],
                          "patient_health_questionnaire_9")
        self.assertEquals(record["my_record_id"], "0")
        self.assertEquals(record["patient_health_questionnaire_9_complete"],
                          RedcapRecordStatus.COMPLETE.value)
        self.assertEquals(record["phq9_how_difficult"], 4)
        self.assertEquals(record["phq9_total_score"], 12)
        self.assertEquals(record["phq9_first_name"], "Forename2")
        self.assertEquals(record["phq9_last_name"], "Surname2")
        self.assertEquals(record["phq9_date_enrolled"], "2010-07-07")

        self.assertEquals(record["phq9_1"], 0)
        self.assertEquals(record["phq9_2"], 1)
        self.assertEquals(record["phq9_3"], 2)
        self.assertEquals(record["phq9_4"], 3)
        self.assertEquals(record["phq9_5"], 0)
        self.assertEquals(record["phq9_6"], 1)
        self.assertEquals(record["phq9_7"], 2)
        self.assertEquals(record["phq9_8"], 3)
        self.assertEquals(record["phq9_9"], 0)

        self.assertEquals(kwargs["return_content"], "auto_ids")
        self.assertTrue(kwargs["force_auto_number"])

        # Second call with patient ID
        args, kwargs = project.import_records.call_args_list[1]

        rows = args[0]
        record = rows[0]
        self.assertEquals(record["patient_id"], 555)


class MedicationTherapyRedcapExportTests(RedcapExportTestCase):
    """
    These are more of a test of the file upload code than anything
    related to the KhandakerMojoMedicationTherapy task
    """
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <event name="event_1_arm_1" />
  <patient instrument="patient_record" redcap_field="patient_id" />
  <record instrument="patient_record" redcap_field="record_id" />
  <instruments>
    <instrument task="khandaker_mojo_medicationtherapy" name="medication_table">
      <files>
        <field name="medtbl_medication_items" formula="task.get_pdf(request)" />
      </files>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1

    def create_tasks(self) -> None:
        from camcops_server.tasks.khandaker_mojo_medicationtherapy import (
            KhandakerMojoMedicationTherapy,
        )

        patient = self.create_patient_with_idnum_1001()
        self.task = KhandakerMojoMedicationTherapy()
        self.apply_standard_task_fields(self.task)
        self.task.id = next(self.id_sequence)
        self.task.patient_id = patient.id
        self.dbsession.add(self.task)
        self.dbsession.commit()

    def test_record_exported(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        # We can't just look at the call_args on the mock object because
        # the file will already have been closed by then
        # noinspection PyUnusedLocal
        def read_pdf_bytes(*import_file_args, **import_file_kwargs) -> None:
            # record, field, fname, fobj
            file_obj = import_file_args[3]
            read_pdf_bytes.pdf_header = file_obj.read(5)

        project.import_file.side_effect = read_pdf_bytes

        exporter.export_task(self.req, exported_task_redcap)
        self.assertEquals(exported_task_redcap.redcap_record_id, "123")
        self.assertEquals(exported_task_redcap.redcap_instrument_name,
                          "medication_table")
        self.assertEquals(exported_task_redcap.redcap_instance_id, 1)

        args, kwargs = project.import_file.call_args

        record_id = args[0]
        fieldname = args[1]
        filename = args[2]

        self.assertEquals(record_id, "123")
        self.assertEquals(fieldname, "medtbl_medication_items")
        self.assertEquals(
            filename,
            "khandaker_mojo_medicationtherapy_123_medtbl_medication_items"
        )

        self.assertEquals(kwargs["repeat_instance"], 1)
        # noinspection PyUnresolvedReferences
        self.assertEquals(read_pdf_bytes.pdf_header, b"%PDF-")
        self.assertEquals(kwargs["event"], "event_1_arm_1")


class MultipleTaskRedcapExportTests(RedcapExportTestCase):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="patient_id" />
  <record instrument="patient_record" redcap_field="record_id" />
  <instruments>
    <instrument task="bmi" name="bmi" event="bmi_event">
      <fields>
        <field name="pa_height" formula="format(task.height_m, '.1f')" />
        <field name="pa_weight" formula="format(task.mass_kg, '.1f')" />
        <field name="bmi_date" formula="format_datetime(task.when_created, DateFormat.ISO8601_DATE_ONLY)" />
      </fields>
    </instrument>
    <instrument task="khandaker_mojo_medicationtherapy" name="medication_table" event="mojo_event">
      <files>
        <field name="medtbl_medication_items" formula="task.get_pdf(request)" />
      </files>
    </instrument>
  </instruments>
</fieldmap>
"""  # noqa: E501

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1

    def create_tasks(self) -> None:
        from camcops_server.tasks.khandaker_mojo_medicationtherapy import (
            KhandakerMojoMedicationTherapy,
        )

        patient = self.create_patient_with_idnum_1001()
        self.mojo_task = KhandakerMojoMedicationTherapy()
        self.apply_standard_task_fields(self.mojo_task)
        self.mojo_task.id = next(self.id_sequence)
        self.mojo_task.patient_id = patient.id
        self.dbsession.add(self.mojo_task)
        self.dbsession.commit()

        from camcops_server.tasks.bmi import Bmi
        self.bmi_task = Bmi()
        self.apply_standard_task_fields(self.bmi_task)
        self.bmi_task.id = next(self.id_sequence)
        self.bmi_task.height_m = 1.83
        self.bmi_task.mass_kg = 67.57
        self.bmi_task.patient_id = patient.id
        self.dbsession.add(self.bmi_task)
        self.dbsession.commit()

    def test_instance_ids_on_different_tasks_in_same_record(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap,
        )
        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        exported_task_mojo = ExportedTask(task=self.mojo_task,
                                          recipient=self.recipient)
        exported_task_redcap_mojo = ExportedTaskRedcap(exported_task_mojo)
        exporter.export_task(self.req, exported_task_redcap_mojo)
        self.assertEquals(exported_task_redcap_mojo.redcap_record_id, "123")
        args, kwargs = project.import_file.call_args

        self.assertEquals(kwargs["repeat_instance"], 1)

        project.export_records.return_value = DataFrame({
            "record_id": ["123"],
            "patient_id": [555],
            "redcap_repeat_instrument": ["khandaker_mojo_medicationtherapy"],
            "redcap_repeat_instance": [1],
        })
        exported_task_bmi = ExportedTask(task=self.bmi_task,
                                         recipient=self.recipient)
        exported_task_redcap_bmi = ExportedTaskRedcap(exported_task_bmi)

        exporter.export_task(self.req, exported_task_redcap_bmi)

        # Import of second task, but is first instance
        # (third call to import_records)
        args, kwargs = project.import_records.call_args_list[2]

        rows = args[0]
        record = rows[0]

        self.assertEquals(record["redcap_repeat_instance"], 1)

    def test_imported_into_different_events(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap,
        )
        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()

        project.is_longitudinal = mock.Mock(return_value=True)
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        exported_task_mojo = ExportedTask(task=self.mojo_task,
                                          recipient=self.recipient)
        exported_task_redcap_mojo = ExportedTaskRedcap(exported_task_mojo)

        exporter.export_task(self.req, exported_task_redcap_mojo)

        args, kwargs = project.import_records.call_args_list[0]
        rows = args[0]
        record = rows[0]

        self.assertEquals(record["redcap_event_name"], "mojo_event")
        args, kwargs = project.import_file.call_args

        self.assertEquals(kwargs["event"], "mojo_event")

        exported_task_bmi = ExportedTask(task=self.bmi_task,
                                         recipient=self.recipient)
        exported_task_redcap_bmi = ExportedTaskRedcap(exported_task_bmi)

        exporter.export_task(self.req, exported_task_redcap_bmi)

        # Import of second task (third call to import_records)
        args, kwargs = project.import_records.call_args_list[2]
        rows = args[0]
        record = rows[0]
        self.assertEquals(record["redcap_event_name"], "bmi_event")


class BadConfigurationRedcapTests(RedcapExportTestCase):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1

    def create_tasks(self) -> None:
        from camcops_server.tasks.bmi import Bmi
        patient = self.create_patient_with_idnum_1001()
        self.task = Bmi()
        self.apply_standard_task_fields(self.task)
        self.task.id = next(self.id_sequence)
        self.task.height_m = 1.83
        self.task.mass_kg = 67.57
        self.task.patient_id = patient.id
        self.dbsession.add(self.task)
        self.dbsession.commit()


class MissingInstrumentRedcapTests(BadConfigurationRedcapTests):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="patient_id" />
  <record instrument="patient_record" redcap_field="record_id" />
  <instruments>
    <instrument task="phq9" name="patient_health_questionnaire_9">
      <fields>
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def test_raises_when_instrument_missing_from_fieldmap(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({"patient_id": []})
        project.import_records.return_value = ["123,0"]

        with self.assertRaises(RedcapExportException) as cm:
            exporter.export_task(self.req, exported_task_redcap)

        message = str(cm.exception)
        self.assertIn("Instrument for task 'bmi' is missing from the fieldmap",
                      message)


class IncorrectRecordIdRedcapTests(BadConfigurationRedcapTests):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="patient_id" />
  <record instrument="patient_record" redcap_field="my_record_id" />
  <instruments>
    <instrument task="bmi" name="bmi">
      <fields>
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def test_raises_when_record_id_is_incorrect(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({
            "record_id": ["123"],
            "patient_id": [555],
            "redcap_repeat_instrument": ["bmi"],
            "redcap_repeat_instance": [1],
        })
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        with self.assertRaises(RedcapExportException) as cm:
            exporter.export_task(self.req, exported_task_redcap)

        message = str(cm.exception)
        self.assertIn("Field 'my_record_id' does not exist in REDCap",
                      message)


class IncorrectPatientIdRedcapTests(BadConfigurationRedcapTests):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="my_patient_id" />
  <record instrument="patient_record" redcap_field="record_id" />
  <instruments>
    <instrument task="bmi" name="bmi">
      <fields>
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def test_raises_when_patient_id_is_incorrect(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.return_value = DataFrame({
            "record_id": ["123"],
            "patient_id": [555],
            "redcap_repeat_instrument": ["bmi"],
            "redcap_repeat_instance": [1],
        })
        project.import_records.return_value = ["123,0"]
        project.export_project_info.return_value = {
            "record_autonumbering_enabled": 1
        }

        with self.assertRaises(RedcapExportException) as cm:
            exporter.export_task(self.req, exported_task_redcap)

        message = str(cm.exception)
        self.assertIn("Field 'my_patient_id' does not exist in REDCap",
                      message)


class MissingPatientInstrumentRedcapTests(BadConfigurationRedcapTests):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="my_patient_id" />
  <record instrument="patient_record" redcap_field="record_id" />
  <instruments>
    <instrument task="bmi" name="bmi">
      <fields>
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def test_raises_when_instrument_is_missing(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()
        project.export_records.side_effect = redcap.RedcapError(
            "Something went wrong"
        )

        with self.assertRaises(RedcapExportException) as cm:
            exporter.export_task(self.req, exported_task_redcap)

        message = str(cm.exception)
        self.assertIn("Something went wrong", message)


class MissingEventRedcapTests(BadConfigurationRedcapTests):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="my_patient_id" />
  <record instrument="patient_record" redcap_field="record_id" />
  <instruments>
    <instrument task="bmi" name="bmi">
      <fields>
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def test_raises_for_longitudinal_project(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()

        project.is_longitudinal = mock.Mock(return_value=True)

        with self.assertRaises(RedcapExportException) as cm:
            exporter.export_task(self.req, exported_task_redcap)

        message = str(cm.exception)
        self.assertEqual(MISSING_EVENT_TAG_OR_ATTRIBUTE, message)


class MissingInstrumentEventRedcapTests(BadConfigurationRedcapTests):
    fieldmap = """<?xml version="1.0" encoding="UTF-8"?>
<fieldmap>
  <patient instrument="patient_record" redcap_field="my_patient_id" />
  <record instrument="patient_record" redcap_field="record_id" />
  <instruments>
    <instrument task="bmi" name="bmi">
      <fields>
      </fields>
    </instrument>
    <instrument task="phq9" name="phq9" event="phq9_event">
      <fields>
      </fields>
    </instrument>
  </instruments>
</fieldmap>"""  # noqa: E501

    def test_raises_when_instrument_missing_event(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()
        project = exporter.get_project()

        project.is_longitudinal = mock.Mock(return_value=True)

        with self.assertRaises(RedcapExportException) as cm:
            exporter.export_task(self.req, exported_task_redcap)

        message = str(cm.exception)
        self.assertEqual(MISSING_EVENT_TAG_OR_ATTRIBUTE, message)


class AnonymousTaskRedcapTests(RedcapExportTestCase):
    def create_tasks(self) -> None:
        from camcops_server.tasks.apeq_cpft_perinatal import APEQCPFTPerinatal
        self.task = APEQCPFTPerinatal()
        self.apply_standard_task_fields(self.task)
        self.task.id = 1
        self.dbsession.add(self.task)
        self.dbsession.commit()

    def test_raises_when_task_is_anonymous(self) -> None:
        from camcops_server.cc_modules.cc_exportmodels import (
            ExportedTask,
            ExportedTaskRedcap
        )

        exported_task = ExportedTask(task=self.task, recipient=self.recipient)
        exported_task_redcap = ExportedTaskRedcap(exported_task)

        exporter = MockRedcapTaskExporter()

        with self.assertRaises(RedcapExportException) as cm:
            exporter.export_task(self.req, exported_task_redcap)

        message = str(cm.exception)
        self.assertIn("Skipping anonymous task 'apeq_cpft_perinatal'", message)
