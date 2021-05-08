#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_task.py

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

**Represents CamCOPS tasks.**

Core task export methods:

======= =======================================================================
Format  Comment
======= =======================================================================
HTML    The task in a user-friendly format.
PDF     Essentially the HTML output, but with page headers and (for clinician
        tasks) a signature block, and without additional HTML administrative
        hyperlinks.
XML     Centres on the task with its subdata integrated.
TSV     Tab-separated value format.
SQL     As part of an SQL or SQLite download.
======= =======================================================================

"""

from collections import OrderedDict
import datetime
import logging
import statistics
from typing import (Any, Dict, Iterable, Generator, List, Optional,
                    Tuple, Type, TYPE_CHECKING, Union)

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.datetimefunc import (
    convert_datetime_to_utc,
    format_datetime,
    pendulum_to_utc_datetime_without_tz,
)
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.sqlalchemy.orm_inspect import (
    gen_columns,
    gen_orm_classes_from_base,
)
from cardinal_pythonlib.sqlalchemy.schema import is_sqlatype_string
from cardinal_pythonlib.stringfunc import mangle_unicode_to_ascii
import hl7
from pendulum import Date, DateTime as Pendulum
from pyramid.renderers import render
from semantic_version import Version
from sqlalchemy.ext.declarative import declared_attr
from sqlalchemy.orm import relationship
from sqlalchemy.orm.relationships import RelationshipProperty
from sqlalchemy.sql.expression import not_, update
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, DateTime, Float, Integer, Text

# from camcops_server.cc_modules.cc_anon import get_cris_dd_rows_from_fieldspecs
from camcops_server.cc_modules.cc_audit import audit
from camcops_server.cc_modules.cc_blob import Blob, get_blob_img_html
from camcops_server.cc_modules.cc_cache import cache_region_static, fkg
from camcops_server.cc_modules.cc_constants import (
    CssClass,
    CSS_PAGED_MEDIA,
    DateFormat,
    ERA_NOW,
    INVALID_VALUE,
)
from camcops_server.cc_modules.cc_db import (
    GenericTabletRecordMixin,
    TFN_EDITING_TIME_S,
    TFN_FIRSTEXIT_IS_ABORT,
    TFN_FIRSTEXIT_IS_FINISH,
    TFN_WHEN_CREATED,
    TFN_WHEN_FIRSTEXIT,
)
from camcops_server.cc_modules.cc_filename import get_export_filename
from camcops_server.cc_modules.cc_hl7 import make_obr_segment, make_obx_segment
from camcops_server.cc_modules.cc_html import (
    get_present_absent_none,
    get_true_false_none,
    get_yes_no,
    get_yes_no_none,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_pdf import pdf_from_html
from camcops_server.cc_modules.cc_pyramid import ViewArg
from camcops_server.cc_modules.cc_simpleobjects import TaskExportOptions
from camcops_server.cc_modules.cc_specialnote import SpecialNote
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    gen_ancillary_relationships,
    get_camcops_blob_column_attr_names,
    get_column_attr_names,
    PendulumDateTimeAsIsoTextColType,
    permitted_value_failure_msgs,
    permitted_values_ok,
    SemanticVersionColType,
    TableNameColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_summaryelement import (
    ExtraSummaryTable,
    SummaryElement,
)
from camcops_server.cc_modules.cc_version import (
    CAMCOPS_SERVER_VERSION,
    MINIMUM_TABLET_VERSION,
)
from camcops_server.cc_modules.cc_xml import (
    get_xml_document,
    XML_COMMENT_ANCILLARY,
    XML_COMMENT_ANONYMOUS,
    XML_COMMENT_BLOBS,
    XML_COMMENT_CALCULATED,
    XML_COMMENT_PATIENT,
    XML_COMMENT_SNOMED_CT,
    XML_COMMENT_SPECIAL_NOTES,
    XML_NAME_SNOMED_CODES,
    XmlElement,
    XmlLiteral,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_ctvinfo import CtvInfo  # noqa: F401
    from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient  # noqa: E501,F401
    from camcops_server.cc_modules.cc_patient import Patient  # noqa: F401
    from camcops_server.cc_modules.cc_patientidnum import PatientIdNum  # noqa: E501,F401
    from camcops_server.cc_modules.cc_request import CamcopsRequest  # noqa: E501,F401
    from camcops_server.cc_modules.cc_snomed import SnomedExpression  # noqa: E501,F401
    from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo  # noqa: E501,F401
    from camcops_server.cc_modules.cc_tsv import TsvPage  # noqa: F401

log = BraceStyleAdapter(logging.getLogger(__name__))

ANCILLARY_FWD_REF = "Ancillary"
TASK_FWD_REF = "Task"

SNOMED_TABLENAME = "_snomed_ct"
SNOMED_COLNAME_TASKTABLE = "task_tablename"
SNOMED_COLNAME_TASKPK = "task_pk"
SNOMED_COLNAME_WHENCREATED_UTC = "when_created"
SNOMED_COLNAME_EXPRESSION = "snomed_expression"
UNUSED_SNOMED_XML_NAME = "snomed_ct_expressions"


# =============================================================================
# Patient mixin
# =============================================================================

class TaskHasPatientMixin(object):
    """
    Mixin for tasks that have a patient (aren't anonymous).
    """
    # http://docs.sqlalchemy.org/en/latest/orm/extensions/declarative/mixins.html#using-advanced-relationship-arguments-e-g-primaryjoin-etc  # noqa

    # noinspection PyMethodParameters
    @declared_attr
    def patient_id(cls) -> Column:
        """
        SQLAlchemy :class:`Column` that is a foreign key to the patient table.
        """
        return Column(
            "patient_id", Integer,
            nullable=False, index=True,
            comment="(TASK) Foreign key to patient.id (for this device/era)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def patient(cls) -> RelationshipProperty:
        """
        SQLAlchemy relationship: "the patient for this task".

        Note that this refers to the CURRENT version of the patient. If there
        is an editing chain, older patient versions are not retrieved.

        Compare :func:`camcops_server.cc_modules.cc_blob.blob_relationship`,
        which uses the same strategy, as do several other similar functions.

        """
        return relationship(
            "Patient",
            primaryjoin=(
                "and_("
                " remote(Patient.id) == foreign({task}.patient_id), "
                " remote(Patient._device_id) == foreign({task}._device_id), "
                " remote(Patient._era) == foreign({task}._era), "
                " remote(Patient._current) == True "
                ")".format(
                    task=cls.__name__,
                )
            ),
            uselist=False,
            viewonly=True,
            # Profiling results 2019-10-14 exporting 4185 phq9 records with
            # unique patients to xlsx
            # lazy="select"  : 59.7s
            # lazy="joined"  : 44.3s
            # lazy="subquery": 36.9s
            # lazy="selectin": 35.3s
            # See also idnums relationship on Patient class (cc_patient.py)
            lazy="selectin"
        )
        # NOTE: this retrieves the most recent (i.e. the current) information
        # on that patient. Consequently, task version history doesn't show the
        # history of patient edits. This is consistent with our relationship
        # strategy throughout for the web front-end viewer.

    # noinspection PyMethodParameters
    @classproperty
    def has_patient(cls) -> bool:
        """
        Does this task have a patient? (Yes.)
        """
        return True


# =============================================================================
# Clinician mixin
# =============================================================================

class TaskHasClinicianMixin(object):
    """
    Mixin to add clinician columns and override clinician-related methods.

    Must be to the LEFT of ``Task`` in the class's base class list, i.e.
    must have higher precedence than ``Task`` in the method resolution order.
    """
    # noinspection PyMethodParameters
    @declared_attr
    def clinician_specialty(cls) -> Column:
        return CamcopsColumn(
            "clinician_specialty", Text,
            exempt_from_anonymisation=True,
            comment="(CLINICIAN) Clinician's specialty "
                    "(e.g. Liaison Psychiatry)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def clinician_name(cls) -> Column:
        return CamcopsColumn(
            "clinician_name", Text,
            exempt_from_anonymisation=True,
            comment="(CLINICIAN) Clinician's name (e.g. Dr X)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def clinician_professional_registration(cls) -> Column:
        return CamcopsColumn(
            "clinician_professional_registration", Text,
            exempt_from_anonymisation=True,
            comment="(CLINICIAN) Clinician's professional registration (e.g. "
                    "GMC# 12345)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def clinician_post(cls) -> Column:
        return CamcopsColumn(
            "clinician_post", Text,
            exempt_from_anonymisation=True,
            comment="(CLINICIAN) Clinician's post (e.g. Consultant)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def clinician_service(cls) -> Column:
        return CamcopsColumn(
            "clinician_service", Text,
            exempt_from_anonymisation=True,
            comment="(CLINICIAN) Clinician's service (e.g. Liaison Psychiatry "
                    "Service)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def clinician_contact_details(cls) -> Column:
        return CamcopsColumn(
            "clinician_contact_details", Text,
            exempt_from_anonymisation=True,
            comment="(CLINICIAN) Clinician's contact details (e.g. bleep, "
                    "extension)"
        )

    # For field order, see also:
    # https://stackoverflow.com/questions/3923910/sqlalchemy-move-mixin-columns-to-end  # noqa

    # noinspection PyMethodParameters
    @classproperty
    def has_clinician(cls) -> bool:
        """
        Does the task have a clinician? (Yes.)
        """
        return True

    def get_clinician_name(self) -> str:
        """
        Returns the clinician's name.
        """
        return self.clinician_name or ""


# =============================================================================
# Respondent mixin
# =============================================================================

class TaskHasRespondentMixin(object):
    """
    Mixin to add respondent columns and override respondent-related methods.

    A respondent is someone who isn't the patient and isn't a clinician, such
    as a family member or carer.

    Must be to the LEFT of ``Task`` in the class's base class list, i.e.
    must have higher precedence than ``Task`` in the method resolution order.

    Notes:

    - If you don't use ``@declared_attr``, the ``comment`` property on columns
      doesn't work.
    """

    # noinspection PyMethodParameters
    @declared_attr
    def respondent_name(cls) -> Column:
        return CamcopsColumn(
            "respondent_name", Text,
            identifies_patient=True,
            comment="(RESPONDENT) Respondent's name"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def respondent_relationship(cls) -> Column:
        return Column(
            "respondent_relationship", Text,
            comment="(RESPONDENT) Respondent's relationship to patient"
        )

    # noinspection PyMethodParameters
    @classproperty
    def has_respondent(cls) -> bool:
        """
        Does the class have a respondent? (Yes.)
        """
        return True

    def is_respondent_complete(self) -> bool:
        """
        Do we have sufficient information about the respondent?
        (That means: name, relationship to the patient.)
        """
        return all([self.respondent_name, self.respondent_relationship])


# =============================================================================
# Task base class
# =============================================================================

class Task(GenericTabletRecordMixin, Base):
    """
    Abstract base class for all tasks.

    Note:

    - For column definitions: use
      :class:`camcops_server.cc_modules.cc_sqla_coltypes.CamcopsColumn`, not
      :class:`Column`, if you have fields that need to define permitted values,
      mark them as BLOB-referencing fields, or do other CamCOPS-specific
      things.

    """
    __abstract__ = True

    # noinspection PyMethodParameters
    @declared_attr
    def __mapper_args__(cls):
        return {
            'polymorphic_identity': cls.__name__,
            'concrete': True,
        }

    # =========================================================================
    # PART 0: COLUMNS COMMON TO ALL TASKS
    # =========================================================================

    # Columns

    # noinspection PyMethodParameters
    @declared_attr
    def when_created(cls) -> Column:
        """
        Column representing the task's creation time.
        """
        return Column(
            TFN_WHEN_CREATED, PendulumDateTimeAsIsoTextColType,
            nullable=False,
            comment="(TASK) Date/time this task instance was created (ISO 8601)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def when_firstexit(cls) -> Column:
        """
        Column representing when the user first exited the task's editor
        (i.e. first "finish" or first "abort").
        """
        return Column(
            TFN_WHEN_FIRSTEXIT, PendulumDateTimeAsIsoTextColType,
            comment="(TASK) Date/time of the first exit from this task "
                    "(ISO 8601)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def firstexit_is_finish(cls) -> Column:
        """
        Was the first exit from the task's editor a successful "finish"?
        """
        return Column(
            TFN_FIRSTEXIT_IS_FINISH, Boolean,
            comment="(TASK) Was the first exit from the task because it was "
                    "finished (1)?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def firstexit_is_abort(cls) -> Column:
        """
        Was the first exit from the task's editor an "abort"?
        """
        return Column(
            TFN_FIRSTEXIT_IS_ABORT, Boolean,
            comment="(TASK) Was the first exit from this task because it was "
                    "aborted (1)?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def editing_time_s(cls) -> Column:
        """
        How long has the user spent editing the task?
        (Calculated by the CamCOPS client.)
        """
        return Column(
            TFN_EDITING_TIME_S, Float,
            comment="(TASK) Time spent editing (s)"
        )

    # Relationships

    # noinspection PyMethodParameters
    @declared_attr
    def special_notes(cls) -> RelationshipProperty:
        """
        List-style SQLAlchemy relationship to any :class:`SpecialNote` objects
        attached to this class. Skips hidden (quasi-deleted) notes.
        """
        return relationship(
            SpecialNote,
            primaryjoin=(
                "and_("
                " remote(SpecialNote.basetable) == literal({repr_task_tablename}), "  # noqa
                " remote(SpecialNote.task_id) == foreign({task}.id), "
                " remote(SpecialNote.device_id) == foreign({task}._device_id), "  # noqa
                " remote(SpecialNote.era) == foreign({task}._era), "
                " not_(SpecialNote.hidden)"
                ")".format(
                    task=cls.__name__,
                    repr_task_tablename=repr(cls.__tablename__),
                )
            ),
            uselist=True,
            order_by="SpecialNote.note_at",
            viewonly=True,  # for now!
        )

    # =========================================================================
    # PART 1: THINGS THAT DERIVED CLASSES MAY CARE ABOUT
    # =========================================================================
    #
    # Notes:
    #
    # - for summaries, see GenericTabletRecordMixin.get_summaries

    # -------------------------------------------------------------------------
    # Attributes that must be provided
    # -------------------------------------------------------------------------
    __tablename__ = None  # type: str  # also the SQLAlchemy table name
    shortname = None  # type: str

    # -------------------------------------------------------------------------
    # Attributes that can be overridden
    # -------------------------------------------------------------------------
    extrastring_taskname = None  # type: str  # if None, tablename is used instead  # noqa
    provides_trackers = False
    use_landscape_for_pdf = False
    dependent_classes = []

    prohibits_clinical = False
    prohibits_commercial = False
    prohibits_educational = False
    prohibits_research = False

    @classmethod
    def prohibits_anything(cls) -> bool:
        return any([cls.prohibits_clinical,
                    cls.prohibits_commercial,
                    cls.prohibits_educational,
                    cls.prohibits_research])

    # -------------------------------------------------------------------------
    # Methods always overridden by the actual task
    # -------------------------------------------------------------------------

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        """
        Long name (in the relevant language).
        """
        raise NotImplementedError("Task.longname must be overridden")

    def is_complete(self) -> bool:
        """
        Is the task instance complete?

        Must be overridden.
        """
        raise NotImplementedError("Task.is_complete must be overridden")

    def get_task_html(self, req: "CamcopsRequest") -> str:
        """
        HTML for the main task content.

        Must be overridden by derived classes.
        """
        raise NotImplementedError(
            "No get_task_html() HTML generator for this task class!")

    # -------------------------------------------------------------------------
    # Implement if you provide trackers
    # -------------------------------------------------------------------------

    def get_trackers(self, req: "CamcopsRequest") -> List["TrackerInfo"]:
        """
        Tasks that provide quantitative information for tracking over time
        should override this and return a list of
        :class:`camcops_server.cc_modules.cc_trackerhelpers.TrackerInfo`
        objects, one per tracker.

        The information is read by
        :meth:`camcops_server.cc_modules.cc_tracker.Tracker.get_all_plots_for_one_task_html`.

        Time information will be retrieved using :func:`get_creation_datetime`.
        """  # noqa
        return []

    # -------------------------------------------------------------------------
    # Override to provide clinical text
    # -------------------------------------------------------------------------

    # noinspection PyMethodMayBeStatic
    def get_clinical_text(self, req: "CamcopsRequest") \
            -> Optional[List["CtvInfo"]]:
        """
        Tasks that provide clinical text information should override this
        to provide a list of
        :class:`camcops_server.cc_modules.cc_ctvinfo.CtvInfo` objects.

        Return ``None`` (default) for a task that doesn't provide clinical
        text, or ``[]`` for one that does in general but has no information for
        this particular instance, or a list of
        :class:`camcops_server.cc_modules.cc_ctvinfo.CtvInfo` objects.
        """
        return None

    # -------------------------------------------------------------------------
    # Override some of these if you provide summaries
    # -------------------------------------------------------------------------

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def get_extra_summary_tables(
            self, req: "CamcopsRequest") -> List[ExtraSummaryTable]:
        """
        Override if you wish to create extra summary tables, not just add
        summary columns to task/ancillary tables.

        Return a list of
        :class:`camcops_server.cc_modules.cc_summaryelement.ExtraSummaryTable`
        objects.
        """
        return []

    # -------------------------------------------------------------------------
    # Implement if you provide SNOMED-CT codes
    # -------------------------------------------------------------------------

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def get_snomed_codes(self,
                         req: "CamcopsRequest") -> List["SnomedExpression"]:
        """
        Returns all SNOMED-CT codes for this task.

        Args:
            req: the
                :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`

        Returns:
            a list of
            :class:`camcops_server.cc_modules.cc_snomed.SnomedExpression`
            objects

        """
        return []

    # =========================================================================
    # PART 2: INTERNALS
    # =========================================================================

    # -------------------------------------------------------------------------
    # Representations
    # -------------------------------------------------------------------------

    def __str__(self) -> str:
        if self.is_anonymous:
            patient_str = ""
        else:
            patient_str = f", patient={self.patient}"
        return "{t} (_pk={pk}, when_created={wc}{patient})".format(
            t=self.tablename,
            pk=self.pk,
            wc=(
                format_datetime(self.when_created, DateFormat.ERA)
                if self.when_created else "None"
            ),
            patient=patient_str,
        )

    def __repr__(self) -> str:
        return "<{classname}(_pk={pk}, when_created={wc})>".format(
            classname=self.__class__.__qualname__,
            pk=self.pk,
            wc=(
                format_datetime(self.when_created, DateFormat.ERA)
                if self.when_created else "None"
            ),
        )

    # -------------------------------------------------------------------------
    # Way to fetch all task types
    # -------------------------------------------------------------------------

    @classmethod
    def gen_all_subclasses(cls) -> Generator[Type[TASK_FWD_REF], None, None]:
        """
        Generate all non-abstract SQLAlchemy ORM subclasses of :class:`Task` --
        that is, all task classes.

        We require that actual tasks are subclasses of both :class:`Task` and
        :class:`camcops_server.cc_modules.cc_sqlalchemy.Base`.

        OLD WAY (ignore): this means we can (a) inherit from Task to make an
        abstract base class for actual tasks, as with PCL, HADS, HoNOS, etc.;
        and (b) not have those intermediate classes appear in the task list.
        Since all actual classes must be SQLAlchemy ORM objects inheriting from
        Base, that common inheritance is an excellent way to define them.

        NEW WAY: things now inherit from Base/Task without necessarily
        being actual tasks; we discriminate using ``__abstract__`` and/or
        ``__tablename__``. See
        https://docs.sqlalchemy.org/en/latest/orm/inheritance.html#abstract-concrete-classes
        """  # noqa
        # noinspection PyTypeChecker
        return gen_orm_classes_from_base(cls)

    @classmethod
    @cache_region_static.cache_on_arguments(function_key_generator=fkg)
    def all_subclasses_by_tablename(cls) -> List[Type[TASK_FWD_REF]]:
        """
        Return all task classes, ordered by table name.
        """
        classes = list(cls.gen_all_subclasses())
        classes.sort(key=lambda c: c.tablename)
        return classes

    @classmethod
    @cache_region_static.cache_on_arguments(function_key_generator=fkg)
    def all_subclasses_by_shortname(cls) -> List[Type[TASK_FWD_REF]]:
        """
        Return all task classes, ordered by short name.
        """
        classes = list(cls.gen_all_subclasses())
        classes.sort(key=lambda c: c.shortname)
        return classes

    @classmethod
    def all_subclasses_by_longname(
            cls, req: "CamcopsRequest") -> List[Type[TASK_FWD_REF]]:
        """
        Return all task classes, ordered by long name.
        """
        classes = cls.all_subclasses_by_shortname()
        classes.sort(key=lambda c: c.longname(req))
        return classes

    # -------------------------------------------------------------------------
    # Methods that may be overridden by mixins
    # -------------------------------------------------------------------------

    # noinspection PyMethodParameters
    @classproperty
    def has_patient(cls) -> bool:
        """
        Does the task have a patient? (No.)

        May be overridden by :class:`TaskHasPatientMixin`.
        """
        return False

    # noinspection PyMethodParameters
    @classproperty
    def is_anonymous(cls) -> bool:
        """
        Antonym for :attr:`has_patient`.
        """
        return not cls.has_patient

    # noinspection PyMethodParameters
    @classproperty
    def has_clinician(cls) -> bool:
        """
        Does the task have a clinician? (No.)

        May be overridden by :class:`TaskHasClinicianMixin`.
        """
        return False

    # noinspection PyMethodParameters
    @classproperty
    def has_respondent(cls) -> bool:
        """
        Does the task have a respondent? (No.)

        May be overridden by :class:`TaskHasRespondentMixin`.
        """
        return False

    # -------------------------------------------------------------------------
    # Other classmethods
    # -------------------------------------------------------------------------

    # noinspection PyMethodParameters
    @classproperty
    def tablename(cls) -> str:
        """
        Returns the database table name for the task's primary table.
        """
        return cls.__tablename__

    # noinspection PyMethodParameters
    @classproperty
    def minimum_client_version(cls) -> Version:
        """
        Returns the minimum client version that provides this task.

        Override this as you add tasks.

        Used by
        :func:`camcops_server.cc_modules.client_api.ensure_valid_table_name`.

        (There are some pre-C++ client versions for which the default is not
        exactly accurate, and the tasks do not override, but this is of no
        consequence and the version numbering system also changed, from
        something legible as a float -- e.g. ``1.2 > 1.14`` -- to something
        interpreted as a semantic version -- e.g. ``1.2 < 1.14``. So we ignore
        that.)
        """
        return MINIMUM_TABLET_VERSION

    # noinspection PyMethodParameters
    @classmethod
    def all_tables_with_min_client_version(cls) -> Dict[str, Version]:
        """
        Returns a dictionary mapping all this task's tables (primary and
        ancillary) to the corresponding minimum client version.
        """
        v = cls.minimum_client_version
        d = {cls.__tablename__: v}  # type: Dict[str, Version]
        for _, _, rel_cls in gen_ancillary_relationships(cls):
            d[rel_cls.__tablename__] = v
        return d

    # -------------------------------------------------------------------------
    # More on fields
    # -------------------------------------------------------------------------

    @classmethod
    def get_fieldnames(cls) -> List[str]:
        """
        Returns all field (column) names for this task's primary table.
        """
        return get_column_attr_names(cls)

    def field_contents_valid(self) -> bool:
        """
        Checks field contents validity.

        This is a high-speed function that doesn't bother with explanations,
        since we use it for lots of task :func:`is_complete` calculations.
        """
        return permitted_values_ok(self)

    def field_contents_invalid_because(self) -> List[str]:
        """
        Explains why contents are invalid.
        """
        return permitted_value_failure_msgs(self)

    def get_blob_fields(self) -> List[str]:
        """
        Returns field (column) names for all BLOB fields in this class.
        """
        return get_camcops_blob_column_attr_names(self)

    # -------------------------------------------------------------------------
    # Server field calculations
    # -------------------------------------------------------------------------

    def is_preserved(self) -> bool:
        """
        Is the task preserved and erased from the tablet?
        """
        return self._pk is not None and self._era != ERA_NOW

    def was_forcibly_preserved(self) -> bool:
        """
        Was this task forcibly preserved?
        """
        return self._forcibly_preserved and self.is_preserved()

    def get_creation_datetime(self) -> Optional[Pendulum]:
        """
        Creation datetime, or None.
        """
        return self.when_created

    def get_creation_datetime_utc(self) -> Optional[Pendulum]:
        """
        Creation datetime in UTC, or None.
        """
        localtime = self.get_creation_datetime()
        if localtime is None:
            return None
        return convert_datetime_to_utc(localtime)

    def get_creation_datetime_utc_tz_unaware(self) -> \
            Optional[datetime.datetime]:
        """
        Creation time as a :class:`datetime.datetime` object on UTC with no
        timezone (i.e. an "offset-naive" datetime), or None.
        """
        localtime = self.get_creation_datetime()
        if localtime is None:
            return None
        return pendulum_to_utc_datetime_without_tz(localtime)

    def get_seconds_from_creation_to_first_finish(self) -> Optional[float]:
        """
        Time in seconds from creation time to first finish (i.e. first exit
        if the first exit was a finish rather than an abort), or None.
        """
        if not self.firstexit_is_finish:
            return None
        start = self.get_creation_datetime()
        end = self.when_firstexit
        if not start or not end:
            return None
        diff = end - start
        return diff.total_seconds()

    def get_adding_user_id(self) -> Optional[int]:
        """
        Returns the user ID of the user who uploaded this task.
        """
        # noinspection PyTypeChecker
        return self._adding_user_id

    def get_adding_user_username(self) -> str:
        """
        Returns the username of the user who uploaded this task.
        """
        return self._adding_user.username if self._adding_user else ""

    def get_removing_user_username(self) -> str:
        """
        Returns the username of the user who deleted this task (by removing it
        on the client and re-uploading).
        """
        return self._removing_user.username if self._removing_user else ""

    def get_preserving_user_username(self) -> str:
        """
        Returns the username of the user who "preserved" this task (marking it
        to be saved on the server and then deleting it from the client).
        """
        return self._preserving_user.username if self._preserving_user else ""

    def get_manually_erasing_user_username(self) -> str:
        """
        Returns the username of the user who erased this task manually on the
        server.
        """
        return self._manually_erasing_user.username if self._manually_erasing_user else ""  # noqa

    # -------------------------------------------------------------------------
    # Summary tables
    # -------------------------------------------------------------------------

    def standard_task_summary_fields(self) -> List[SummaryElement]:
        """
        Returns summary fields/values provided by all tasks.
        """
        return [
            SummaryElement(
                name="is_complete",
                coltype=Boolean(),
                value=self.is_complete(),
                comment="(GENERIC) Task complete?"
            ),
            SummaryElement(
                name="seconds_from_creation_to_first_finish",
                coltype=Float(),
                value=self.get_seconds_from_creation_to_first_finish(),
                comment="(GENERIC) Time (in seconds) from record creation to "
                        "first exit, if that was a finish not an abort",
            ),
            SummaryElement(
                name="camcops_server_version",
                coltype=SemanticVersionColType(),
                value=CAMCOPS_SERVER_VERSION,
                comment="(GENERIC) CamCOPS server version that created the "
                        "summary information",
            ),
        ]

    def get_all_summary_tables(self, req: "CamcopsRequest") \
            -> List[ExtraSummaryTable]:
        """
        Returns all
        :class:`camcops_server.cc_modules.cc_summaryelement.ExtraSummaryTable`
        objects for this class, including any provided by subclasses, plus
        SNOMED CT codes if enabled.
        """
        tables = self.get_extra_summary_tables(req)
        if req.snomed_supported:
            tables.append(self._get_snomed_extra_summary_table(req))
        return tables

    def _get_snomed_extra_summary_table(self, req: "CamcopsRequest") \
            -> ExtraSummaryTable:
        """
        Returns a
        :class:`camcops_server.cc_modules.cc_summaryelement.ExtraSummaryTable`
        for this task's SNOMED CT codes.
        """
        codes = self.get_snomed_codes(req)
        columns = [
            Column(SNOMED_COLNAME_TASKTABLE, TableNameColType,
                   comment="Task's base table name"),
            Column(SNOMED_COLNAME_TASKPK, Integer,
                   comment="Task's server primary key"),
            Column(SNOMED_COLNAME_WHENCREATED_UTC, DateTime,
                   comment="Task's creation date/time (UTC)"),
            CamcopsColumn(SNOMED_COLNAME_EXPRESSION, Text,
                          exempt_from_anonymisation=True,
                          comment="SNOMED CT expression"),
        ]
        rows = []  # type: List[Dict[str, Any]]
        for code in codes:
            d = OrderedDict([
                (SNOMED_COLNAME_TASKTABLE, self.tablename),
                (SNOMED_COLNAME_TASKPK, self.pk),
                (SNOMED_COLNAME_WHENCREATED_UTC,
                 self.get_creation_datetime_utc_tz_unaware()),
                (SNOMED_COLNAME_EXPRESSION, code.as_string()),
            ])
            rows.append(d)
        return ExtraSummaryTable(
            tablename=SNOMED_TABLENAME,
            xmlname=UNUSED_SNOMED_XML_NAME,  # though actual XML doesn't use this route  # noqa
            columns=columns,
            rows=rows,
            task=self
        )

    # -------------------------------------------------------------------------
    # Testing
    # -------------------------------------------------------------------------

    def dump(self) -> None:
        """
        Dump a description of the task instance to the Python log, for
        debugging.
        """
        line_equals = "=" * 79
        lines = ["", line_equals]
        for f in self.get_fieldnames():
            lines.append(f"{f}: {getattr(self, f)!r}")
        lines.append(line_equals)
        log.info("\n".join(lines))

    # -------------------------------------------------------------------------
    # Special notes
    # -------------------------------------------------------------------------

    def apply_special_note(self,
                           req: "CamcopsRequest",
                           note: str,
                           from_console: bool = False) -> None:
        """
        Manually applies a special note to a task.

        Applies it to all predecessor/successor versions as well.
        WRITES TO THE DATABASE.
        """
        sn = SpecialNote()
        sn.basetable = self.tablename
        sn.task_id = self.id
        sn.device_id = self._device_id
        sn.era = self._era
        sn.note_at = req.now
        sn.user_id = req.user_id
        sn.note = note
        dbsession = req.dbsession
        dbsession.add(sn)
        self.audit(req, "Special note applied manually", from_console)
        self.cancel_from_export_log(req, from_console)

    # -------------------------------------------------------------------------
    # Clinician
    # -------------------------------------------------------------------------

    # noinspection PyMethodMayBeStatic
    def get_clinician_name(self) -> str:
        """
        Get the clinician's name.

        May be overridden by :class:`TaskHasClinicianMixin`.
        """
        return ""

    # -------------------------------------------------------------------------
    # Respondent
    # -------------------------------------------------------------------------

    # noinspection PyMethodMayBeStatic
    def is_respondent_complete(self) -> bool:
        """
        Is the respondent information complete?

        May be overridden by :class:`TaskHasRespondentMixin`.
        """
        return False

    # -------------------------------------------------------------------------
    # About the associated patient
    # -------------------------------------------------------------------------

    @property
    def patient(self) -> Optional["Patient"]:
        """
        Returns the :class:`camcops_server.cc_modules.cc_patient.Patient` for
        this task.

        Overridden by :class:`TaskHasPatientMixin`.
        """
        return None

    def is_female(self) -> bool:
        """
        Is the patient female?
        """
        return self.patient.is_female() if self.patient else False

    def is_male(self) -> bool:
        """
        Is the patient male?
        """
        return self.patient.is_male() if self.patient else False

    def get_patient_server_pk(self) -> Optional[int]:
        """
        Get the server PK of the patient, or None.
        """
        return self.patient.pk if self.patient else None

    def get_patient_forename(self) -> str:
        """
        Get the patient's forename, in upper case, or "".
        """
        return self.patient.get_forename() if self.patient else ""

    def get_patient_surname(self) -> str:
        """
        Get the patient's surname, in upper case, or "".
        """
        return self.patient.get_surname() if self.patient else ""

    def get_patient_dob(self) -> Optional[Date]:
        """
        Get the patient's DOB, or None.
        """
        return self.patient.get_dob() if self.patient else None

    def get_patient_dob_first11chars(self) -> Optional[str]:
        """
        Gets the patient's date of birth in an 11-character human-readable
        short format. For example: ``29 Dec 1999``.
        """
        if not self.patient:
            return None
        dob_str = self.patient.get_dob_str()
        if not dob_str:
            return None
        return dob_str[:11]

    def get_patient_sex(self) -> str:
        """
        Get the patient's sex, or "".
        """
        return self.patient.get_sex() if self.patient else ""

    def get_patient_address(self) -> str:
        """
        Get the patient's address, or "".
        """
        return self.patient.get_address() if self.patient else ""

    def get_patient_idnum_objects(self) -> List["PatientIdNum"]:
        """
        Gets all
        :class:`camcops_server.cc_modules.cc_patientidnum.PatientIdNum` objects
        for the patient.
        """
        return self.patient.get_idnum_objects() if self.patient else []

    def get_patient_idnum_object(self,
                                 which_idnum: int) -> Optional["PatientIdNum"]:
        """
        Get the patient's
        :class:`camcops_server.cc_modules.cc_patientidnum.PatientIdNum` for the
        specified ID number type (``which_idnum``), or None.
        """
        return (self.patient.get_idnum_object(which_idnum) if self.patient
                else None)

    def any_patient_idnums_invalid(self, req: "CamcopsRequest") -> bool:
        """
        Do we have a patient who has any invalid ID numbers?

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        """
        idnums = self.get_patient_idnum_objects()
        for idnum in idnums:
            if not idnum.is_fully_valid(req):
                return True
        return False

    def get_patient_idnum_value(self, which_idnum: int) -> Optional[int]:
        """
        Get the patient's ID number value for the specified ID number
        type (``which_idnum``), or None.
        """
        idobj = self.get_patient_idnum_object(which_idnum=which_idnum)
        return idobj.idnum_value if idobj else None

    def get_patient_hl7_pid_segment(self,
                                    req: "CamcopsRequest",
                                    recipient_def: "ExportRecipient") \
            -> Union[hl7.Segment, str]:
        """
        Get an HL7 PID segment for the patient, or "".
        """
        return (self.patient.get_hl7_pid_segment(req, recipient_def)
                if self.patient else "")

    # -------------------------------------------------------------------------
    # HL7
    # -------------------------------------------------------------------------

    def get_hl7_data_segments(self, req: "CamcopsRequest",
                              recipient_def: "ExportRecipient") \
            -> List[hl7.Segment]:
        """
        Returns a list of HL7 data segments.

        These will be:

        - OBR segment
        - OBX segment
        - any extra ones offered by the task
        """
        obr_segment = make_obr_segment(self)
        export_options = recipient_def.get_task_export_options()
        obx_segment = make_obx_segment(
            req,
            self,
            task_format=recipient_def.task_format,
            observation_identifier=self.tablename + "_" + str(self._pk),
            observation_datetime=self.get_creation_datetime(),
            responsible_observer=self.get_clinician_name(),
            export_options=export_options,
        )
        return [
            obr_segment,
            obx_segment
        ] + self.get_hl7_extra_data_segments(recipient_def)

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def get_hl7_extra_data_segments(self, recipient_def: "ExportRecipient") \
            -> List[hl7.Segment]:
        """
        Return a list of any extra HL7 data segments. (See
        :func:`get_hl7_data_segments`.)

        May be overridden.
        """
        return []

    def cancel_from_export_log(self, req: "CamcopsRequest",
                               from_console: bool = False) -> None:
        """
        Marks all instances of this task as "cancelled" in the export log, so
        it will be resent.
        """
        if self._pk is None:
            return
        from camcops_server.cc_modules.cc_exportmodels import ExportedTask  # delayed import  # noqa
        # noinspection PyUnresolvedReferences
        statement = (
            update(ExportedTask.__table__)
            .where(ExportedTask.basetable == self.tablename)
            .where(ExportedTask.task_server_pk == self._pk)
            .where(not_(ExportedTask.cancelled) |
                   ExportedTask.cancelled.is_(None))
            .values(cancelled=1,
                    cancelled_at_utc=req.now_utc)
        )
        # ... this bit: ... AND (NOT cancelled OR cancelled IS NULL) ...:
        # https://stackoverflow.com/questions/37445041/sqlalchemy-how-to-filter-column-which-contains-both-null-and-integer-values  # noqa
        req.dbsession.execute(statement)
        self.audit(
            req,
            "Task cancelled in export log (may trigger resending)",
            from_console
        )

    # -------------------------------------------------------------------------
    # Audit
    # -------------------------------------------------------------------------

    def audit(self, req: "CamcopsRequest", details: str,
              from_console: bool = False) -> None:
        """
        Audits actions to this task.
        """
        audit(req,
              details,
              patient_server_pk=self.get_patient_server_pk(),
              table=self.tablename,
              server_pk=self._pk,
              from_console=from_console)

    # -------------------------------------------------------------------------
    # Erasure (wiping, leaving record as placeholder)
    # -------------------------------------------------------------------------

    def manually_erase(self, req: "CamcopsRequest") -> None:
        """
        Manually erases a task (including sub-tables).
        Also erases linked non-current records.
        This WIPES THE CONTENTS but LEAVES THE RECORD AS A PLACEHOLDER.

        Audits the erasure. Propagates erase through to the HL7 log, so those
        records will be re-sent. WRITES TO DATABASE.
        """
        # Erase ourself and any other in our "family"
        for task in self.get_lineage():
            task.manually_erase_with_dependants(req)
        # Audit and clear HL7 message log
        self.audit(req, "Task details erased manually")
        self.cancel_from_export_log(req)

    def is_erased(self) -> bool:
        """
        Has the task been manually erased? See :func:`manually_erase`.
        """
        return self._manually_erased

    # -------------------------------------------------------------------------
    # Complete deletion
    # -------------------------------------------------------------------------

    def delete_entirely(self, req: "CamcopsRequest") -> None:
        """
        Completely delete this task, its lineage, and its dependants.
        """
        for task in self.get_lineage():
            task.delete_with_dependants(req)
        self.audit(req, "Task deleted")

    # -------------------------------------------------------------------------
    # Viewing the task in the list of tasks
    # -------------------------------------------------------------------------

    def is_live_on_tablet(self) -> bool:
        """
        Is the task instance live on a tablet?
        """
        return self._era == ERA_NOW

    # -------------------------------------------------------------------------
    # Filtering tasks for the task list
    # -------------------------------------------------------------------------

    @classmethod
    def gen_text_filter_columns(cls) -> Generator[Tuple[str, Column], None,
                                                  None]:
        """
        Yields tuples of ``attrname, column``, for columns that are suitable
        for text filtering.
        """
        for attrname, column in gen_columns(cls):
            if attrname.startswith("_"):  # system field
                continue
            if not is_sqlatype_string(column.type):
                continue
            yield attrname, column

    @classmethod
    @cache_region_static.cache_on_arguments(function_key_generator=fkg)
    def get_text_filter_columns(cls) -> List[Column]:
        """
        Cached function to return a list of SQLAlchemy Column objects suitable
        for text filtering.
        """
        return [col for _, col in cls.gen_text_filter_columns()]

    def contains_text(self, text: str) -> bool:
        """
        Does this task contain the specified text?

        Args:
            text:
                string that must be present in at least one of our text
                columns

        Returns:
            is the strings present?
        """
        text = text.lower()
        for attrname, _ in self.gen_text_filter_columns():
            value = getattr(self, attrname)
            if value is None:
                continue
            assert isinstance(value, str), "Internal bug in contains_text"
            if text in value.lower():
                return True
        return False

    def contains_all_strings(self, strings: List[str]) -> bool:
        """
        Does this task contain all of the specified strings?

        Args:
            strings:
                list of strings; each string must be present in at least
                one of our text columns

        Returns:
            are all strings present?
        """
        return all(self.contains_text(text) for text in strings)

    # -------------------------------------------------------------------------
    # TSV export for basic research dump
    # -------------------------------------------------------------------------

    def get_tsv_pages(self, req: "CamcopsRequest") -> List["TsvPage"]:
        """
        Returns information used for the basic research dump in TSV format.
        """
        # 1. Our core fields, plus summary information

        main_page = self._get_core_tsv_page(req)
        # 2. Patient details.

        if self.patient:
            main_page.add_or_set_columns_from_page(
                self.patient.get_tsv_page(req))
        tsv_pages = [main_page]
        # 3. +/- Ancillary objects
        for ancillary in self.gen_ancillary_instances():  # type: GenericTabletRecordMixin  # noqa
            page = ancillary._get_core_tsv_page(req)
            tsv_pages.append(page)
        # 4. +/- Extra summary tables (inc. SNOMED)
        for est in self.get_all_summary_tables(req):
            tsv_pages.append(est.get_tsv_page())
        # Done
        return tsv_pages

    # -------------------------------------------------------------------------
    # XML view
    # -------------------------------------------------------------------------

    def get_xml(self,
                req: "CamcopsRequest",
                options: TaskExportOptions = None,
                indent_spaces: int = 4,
                eol: str = '\n') -> str:
        """
        Returns XML describing the task.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            options: a :class:`camcops_server.cc_modules.cc_simpleobjects.TaskExportOptions`

            indent_spaces: number of spaces to indent formatted XML
            eol: end-of-line string

        Returns:
            an XML UTF-8 document representing the task.

        """  # noqa
        options = options or TaskExportOptions()
        tree = self.get_xml_root(req=req, options=options)
        return get_xml_document(
            tree,
            indent_spaces=indent_spaces,
            eol=eol,
            include_comments=options.xml_include_comments,
        )

    def get_xml_root(self,
                     req: "CamcopsRequest",
                     options: TaskExportOptions) -> XmlElement:
        """
        Returns an XML tree. The return value is the root
        :class:`camcops_server.cc_modules.cc_xml.XmlElement`.

        Override to include other tables, or to deal with BLOBs, if the default
        methods are insufficient.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            options: a :class:`camcops_server.cc_modules.cc_simpleobjects.TaskExportOptions`
        """  # noqa
        # Core (inc. core BLOBs)
        branches = self._get_xml_core_branches(req=req, options=options)
        tree = XmlElement(name=self.tablename, value=branches)
        return tree

    def _get_xml_core_branches(
            self,
            req: "CamcopsRequest",
            options: TaskExportOptions) -> List[XmlElement]:
        """
        Returns a list of :class:`camcops_server.cc_modules.cc_xml.XmlElement`
        elements representing stored, calculated, patient, and/or BLOB fields,
        depending on the options.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            options: a :class:`camcops_server.cc_modules.cc_simpleobjects.TaskExportOptions`
        """  # noqa
        def add_comment(comment: XmlLiteral) -> None:
            if options.xml_with_header_comments:
                branches.append(comment)

        options = options or TaskExportOptions(xml_include_plain_columns=True,
                                               xml_include_ancillary=True,
                                               include_blobs=False,
                                               xml_include_calculated=True,
                                               xml_include_patient=True,
                                               xml_include_snomed=True)

        # Stored values +/- calculated values
        core_options = options.clone()
        core_options.include_blobs = False
        branches = self._get_xml_branches(req=req, options=core_options)

        # SNOMED-CT codes
        if options.xml_include_snomed and req.snomed_supported:
            add_comment(XML_COMMENT_SNOMED_CT)
            snomed_codes = self.get_snomed_codes(req)
            snomed_branches = []  # type: List[XmlElement]
            for code in snomed_codes:
                snomed_branches.append(code.xml_element())
            branches.append(XmlElement(name=XML_NAME_SNOMED_CODES,
                                       value=snomed_branches))

        # Special notes
        add_comment(XML_COMMENT_SPECIAL_NOTES)
        for sn in self.special_notes:
            branches.append(sn.get_xml_root())

        # Patient details
        if self.is_anonymous:
            add_comment(XML_COMMENT_ANONYMOUS)
        elif options.xml_include_patient:
            add_comment(XML_COMMENT_PATIENT)
            patient_options = TaskExportOptions(
                xml_include_plain_columns=True,
                xml_with_header_comments=options.xml_with_header_comments)
            if self.patient:
                branches.append(self.patient.get_xml_root(
                    req, patient_options))

        # BLOBs
        if options.include_blobs:
            add_comment(XML_COMMENT_BLOBS)
            blob_options = TaskExportOptions(
                include_blobs=True,
                xml_skip_fields=options.xml_skip_fields,
                xml_sort_by_name=True,
                xml_with_header_comments=False,
            )
            branches += self._get_xml_branches(req=req, options=blob_options)

        # Ancillary objects
        if options.xml_include_ancillary:
            ancillary_options = TaskExportOptions(
                xml_include_plain_columns=True,
                xml_include_ancillary=True,
                include_blobs=options.include_blobs,
                xml_include_calculated=options.xml_include_calculated,
                xml_sort_by_name=True,
                xml_with_header_comments=options.xml_with_header_comments,
            )
            item_collections = []  # type: List[XmlElement]
            found_ancillary = False
            # We use a slightly more manual iteration process here so that
            # we iterate through individual ancillaries but clustered by their
            # name (e.g. if we have 50 trials and 5 groups, we do them in
            # collections).
            for attrname, rel_prop, rel_cls in gen_ancillary_relationships(self):  # noqa
                if not found_ancillary:
                    add_comment(XML_COMMENT_ANCILLARY)
                    found_ancillary = True
                itembranches = []  # type: List[XmlElement]
                if rel_prop.uselist:
                    ancillaries = getattr(self, attrname)  # type: List[GenericTabletRecordMixin]  # noqa
                else:
                    ancillaries = [getattr(self, attrname)]  # type: List[GenericTabletRecordMixin]  # noqa
                for ancillary in ancillaries:
                    itembranches.append(
                        ancillary._get_xml_root(req=req,
                                                options=ancillary_options)
                    )
                itemcollection = XmlElement(name=attrname, value=itembranches)
                item_collections.append(itemcollection)
            item_collections.sort(key=lambda el: el.name)
            branches += item_collections

        # Completely separate additional summary tables
        if options.xml_include_calculated:
            item_collections = []  # type: List[XmlElement]
            found_est = False
            for est in self.get_extra_summary_tables(req):
                # ... not get_all_summary_tables(); we handled SNOMED
                # differently, above
                if not found_est and est.rows:
                    add_comment(XML_COMMENT_CALCULATED)
                    found_est = True
                item_collections.append(est.get_xml_element())
            item_collections.sort(key=lambda el: el.name)
            branches += item_collections

        return branches

    # -------------------------------------------------------------------------
    # HTML view
    # -------------------------------------------------------------------------

    def get_html(self, req: "CamcopsRequest", anonymise: bool = False) -> str:
        """
        Returns HTML representing the task, for our HTML view.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            anonymise: hide patient identifying details?
        """
        req.prepare_for_html_figures()
        return render("task.mako",
                      dict(task=self,
                           anonymise=anonymise,
                           signature=False,
                           viewtype=ViewArg.HTML),
                      request=req)

    def title_for_html(self, req: "CamcopsRequest",
                       anonymise: bool = False) -> str:
        """
        Returns the plain text used for the HTML ``<title>`` block (by
        ``task.mako``), and also for the PDF title for PDF exports.

        Should be plain text only.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            anonymise: hide patient identifying details?
        """
        if anonymise:
            patient = "?"
        elif self.patient:
            patient = self.patient.prettystr(req)
        else:
            _ = req.gettext
            patient = _("Anonymous")
        tasktype = self.tablename
        when = format_datetime(self.get_creation_datetime(),
                               DateFormat.ISO8601_HUMANIZED_TO_MINUTES, "")
        return f"CamCOPS: {patient}; {tasktype}; {when}"

    # -------------------------------------------------------------------------
    # PDF view
    # -------------------------------------------------------------------------

    def get_pdf(self, req: "CamcopsRequest", anonymise: bool = False) -> bytes:
        """
        Returns a PDF representing the task.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            anonymise: hide patient identifying details?
        """
        html = self.get_pdf_html(req, anonymise=anonymise)  # main content
        if CSS_PAGED_MEDIA:
            return pdf_from_html(req, html=html)
        else:
            return pdf_from_html(
                req,
                html=html,
                header_html=render(
                    "wkhtmltopdf_header.mako",
                    dict(inner_text=render("task_page_header.mako",
                                           dict(task=self, anonymise=anonymise),
                                           request=req)),
                    request=req
                ),
                footer_html=render(
                    "wkhtmltopdf_footer.mako",
                    dict(inner_text=render("task_page_footer.mako",
                                           dict(task=self),
                                           request=req)),
                    request=req
                ),
                extra_wkhtmltopdf_options={
                    "orientation": ("Landscape" if self.use_landscape_for_pdf
                                    else "Portrait")
                }
            )

    def get_pdf_html(self, req: "CamcopsRequest",
                     anonymise: bool = False) -> str:
        """
        Gets the HTML used to make the PDF (slightly different from the HTML
        used for the HTML view).
        """
        req.prepare_for_pdf_figures()
        return render("task.mako",
                      dict(task=self,
                           anonymise=anonymise,
                           pdf_landscape=self.use_landscape_for_pdf,
                           signature=self.has_clinician,
                           viewtype=ViewArg.PDF),
                      request=req)

    def suggested_pdf_filename(self, req: "CamcopsRequest",
                               anonymise: bool = False) -> str:
        """
        Suggested filename for the PDF copy (for downloads).

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            anonymise: hide patient identifying details?
        """
        cfg = req.config
        if anonymise:
            is_anonymous = True
        else:
            is_anonymous = self.is_anonymous
        patient = self.patient
        return get_export_filename(
            req=req,
            patient_spec_if_anonymous=cfg.patient_spec_if_anonymous,
            patient_spec=cfg.patient_spec,
            filename_spec=cfg.task_filename_spec,
            filetype=ViewArg.PDF,
            is_anonymous=is_anonymous,
            surname=patient.get_surname() if patient else "",
            forename=patient.get_forename() if patient else "",
            dob=patient.get_dob() if patient else None,
            sex=patient.get_sex() if patient else None,
            idnum_objects=patient.get_idnum_objects() if patient else None,
            creation_datetime=self.get_creation_datetime(),
            basetable=self.tablename,
            serverpk=self._pk
        )

    def write_pdf_to_disk(self, req: "CamcopsRequest", filename: str) -> None:
        """
        Writes the PDF to disk, using ``filename``.
        """
        pdffile = open(filename, "wb")
        pdffile.write(self.get_pdf(req))

    # -------------------------------------------------------------------------
    # Metadata for e.g. RiO
    # -------------------------------------------------------------------------

    def get_rio_metadata(self,
                         req: "CamcopsRequest",
                         which_idnum: int,
                         uploading_user_id: str,
                         document_type: str) -> str:
        """
        Returns metadata for the task that Servelec's RiO electronic patient
        record may want.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            which_idnum: which CamCOPS ID number type corresponds to the RiO
                client ID?
            uploading_user_id: RiO user ID (string) of the user who will
                be recorded as uploading this information; see below
            document_type: a string indicating the RiO-defined document type
                (this is system-specific); see below

        Returns:
            a newline-terminated single line of CSV values; see below

        Called by
        :meth:`camcops_server.cc_modules.cc_exportmodels.ExportedTaskFileGroup.export_task`.

        From Servelec (Lee Meredith) to Rudolf Cardinal, 2014-12-04:

        .. code-block:: none

            Batch Document Upload

            The RiO batch document upload function can be used to upload
            documents in bulk automatically.  RiO includes a Batch Upload
            windows service which monitors a designated folder for new files.
            Each file which is scanned must be placed in the designated folder
            along with a meta-data file which describes the document.  So
            essentially if a document had been scanned in and was called
            ‘ThisIsANewReferralLetterForAPatient.pdf’ then there would also
            need to be a meta file in the same folder called
            ‘ThisIsANewReferralLetterForAPatient.metadata’.  The contents of
            the meta file would need to include the following:

                Field Order; Field Name; Description; Data Mandatory (Y/N);
                Format

                1; ClientID; RiO Client ID which identifies the patient in RiO
                against which the document will be uploaded.; Y; 15
                Alphanumeric Characters

                2; UserID; User ID of the uploaded document, this is any user
                defined within the RiO system and can be a single system user
                called ‘AutomaticDocumentUploadUser’ for example.; Y; 10
                Alphanumeric Characters

                    [NB example longer than that!]

                3; DocumentType; The RiO defined document type eg: APT; Y; 80
                Alphanumeric Characters

                4; Title; The title of the document; N; 40 Alphanumeric
                Characters

                5; Description; The document description.; N; 500 Alphanumeric
                Characters

                6; Author; The author of the document; N; 80 Alphanumeric
                Characters

                7; DocumentDate; The date of the document; N; dd/MM/yyyy HH:mm

                8; FinalRevision; The revision values are 0 Draft or 1 Final,
                this is defaulted to 1 which is Final revision.; N; 0 or 1

            As an example, this is what would be needed in a meta file:

                “1000001”,”TRUST1”,”APT”,”A title”, “A description of the
                    document”, “An author”,”01/12/2012 09:45”,”1”

            (on one line)

        Clarification, from Lee Meredith to Rudolf Cardinal, 2015-02-18:

        - metadata files must be plain ASCII, not UTF-8

          - ... here and
            :meth:`camcops_server.cc_modules.cc_exportmodels.ExportedTaskFileGroup.export_task`

        - line terminator is <CR>

          - BUT see
            :meth:`camcops_server.cc_modules.cc_exportmodels.ExportedTaskFileGroup.export_task`

        - user name limit is 10 characters, despite incorrect example

          - search for ``RIO_MAX_USER_LEN``

        - DocumentType is a code that maps to a human-readable document
          type; for example, "APT" might map to "Appointment Letter". These
          mappings are specific to the local system. (We will probably want
          one that maps to "Clinical Correspondence" in the absence of
          anything more specific.)

        - RiO will delete the files after it's processed them.

        - Filenames should avoid spaces, but otherwise any other standard
          ASCII code is fine within filenames.

          - see
            :meth:`camcops_server.cc_modules.cc_exportmodels.ExportedTaskFileGroup.export_task`

        """  # noqa

        try:
            client_id = self.patient.get_idnum_value(which_idnum)
        except AttributeError:
            client_id = ""
        title = "CamCOPS_" + self.shortname
        description = self.longname(req)
        author = self.get_clinician_name()  # may be blank
        document_date = format_datetime(self.when_created,
                                        DateFormat.RIO_EXPORT_UK)
        # This STRIPS the timezone information; i.e. it is in the local
        # timezone but doesn't tell you which timezone that is. (That's fine;
        # it should be local or users would be confused.)
        final_revision = (0 if self.is_live_on_tablet() else 1)

        item_list = [
            client_id,
            uploading_user_id,
            document_type,
            title,
            description,
            author,
            document_date,
            final_revision
        ]
        # UTF-8 is NOT supported by RiO for metadata. So:
        csv_line = ",".join([f'"{mangle_unicode_to_ascii(x)}"'
                             for x in item_list])
        return csv_line + "\n"

    # -------------------------------------------------------------------------
    # HTML elements used by tasks
    # -------------------------------------------------------------------------

    # noinspection PyMethodMayBeStatic
    def get_standard_clinician_comments_block(self,
                                              req: "CamcopsRequest",
                                              comments: str) -> str:
        """
        HTML DIV for clinician's comments.
        """
        return render("clinician_comments.mako",
                      dict(comment=comments),
                      request=req)

    def get_is_complete_td_pair(self, req: "CamcopsRequest") -> str:
        """
        HTML to indicate whether task is complete or not, and to make it
        very obvious visually when it isn't.
        """
        c = self.is_complete()
        td_class = "" if c else f' class="{CssClass.INCOMPLETE}"'
        return (
            f"<td>Completed?</td>"
            f"<td{td_class}><b>{get_yes_no(req, c)}</b></td>"
        )

    def get_is_complete_tr(self, req: "CamcopsRequest") -> str:
        """
        HTML table row to indicate whether task is complete or not, and to
        make it very obvious visually when it isn't.
        """
        return f"<tr>{self.get_is_complete_td_pair(req)}</tr>"

    def get_twocol_val_row(self,
                           fieldname: str,
                           default: str = None,
                           label: str = None) -> str:
        """
        HTML table row, two columns, without web-safing of value.

        Args:
            fieldname: field (attribute) name; the value will be retrieved
                from this attribute
            default: default to show if the value is ``None``
            label: descriptive label

        Returns:
            two-column HTML table row (label, value)

        """
        val = getattr(self, fieldname)
        if val is None:
            val = default
        if label is None:
            label = fieldname
        return tr_qa(label, val)

    def get_twocol_string_row(self,
                              fieldname: str,
                              label: str = None) -> str:
        """
        HTML table row, two columns, with web-safing of value.

        Args:
            fieldname: field (attribute) name; the value will be retrieved
                from this attribute
            label: descriptive label

        Returns:
            two-column HTML table row (label, value)
        """
        if label is None:
            label = fieldname
        return tr_qa(label, getattr(self, fieldname))

    def get_twocol_bool_row(self,
                            req: "CamcopsRequest",
                            fieldname: str,
                            label: str = None) -> str:
        """
        HTML table row, two columns, with Boolean Y/N formatter for value.

        Args:
            req: :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            fieldname: field (attribute) name; the value will be retrieved
                from this attribute
            label: descriptive label

        Returns:
            two-column HTML table row (label, value)
        """
        if label is None:
            label = fieldname
        return tr_qa(label, get_yes_no_none(req, getattr(self, fieldname)))

    def get_twocol_bool_row_true_false(self,
                                       req: "CamcopsRequest",
                                       fieldname: str,
                                       label: str = None) -> str:
        """
        HTML table row, two columns, with Boolean true/false formatter for
        value.

        Args:
            req: :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            fieldname: field (attribute) name; the value will be retrieved
                from this attribute
            label: descriptive label

        Returns:
            two-column HTML table row (label, value)
        """
        if label is None:
            label = fieldname
        return tr_qa(label, get_true_false_none(req, getattr(self, fieldname)))

    def get_twocol_bool_row_present_absent(self,
                                           req: "CamcopsRequest",
                                           fieldname: str,
                                           label: str = None) -> str:
        """
        HTML table row, two columns, with Boolean present/absent formatter for
        value.

        Args:
            req: :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            fieldname: field (attribute) name; the value will be retrieved
                from this attribute
            label: descriptive label

        Returns:
            two-column HTML table row (label, value)
        """
        if label is None:
            label = fieldname
        return tr_qa(label, get_present_absent_none(req,
                                                    getattr(self, fieldname)))

    @staticmethod
    def get_twocol_picture_row(blob: Optional[Blob], label: str) -> str:
        """
        HTML table row, two columns, with PNG on right.

        Args:
            blob: the :class:`camcops_server.cc_modules.cc_blob.Blob` object
            label: descriptive label

        Returns:
            two-column HTML table row (label, picture)
        """
        return tr(label, get_blob_img_html(blob))

    # -------------------------------------------------------------------------
    # Field helper functions for subclasses
    # -------------------------------------------------------------------------

    def get_values(self, fields: List[str]) -> List:
        """
        Get list of object's values from list of field names.
        """
        return [getattr(self, f) for f in fields]

    def is_field_not_none(self, field: str) -> bool:
        """
        Is the field not None?
        """
        return getattr(self, field) is not None

    def any_fields_none(self, fields: List[str]) -> bool:
        """
        Are any specified fields None?
        """
        for f in fields:
            if getattr(self, f) is None:
                return True
        return False

    def all_fields_not_none(self, fields: List[str]) -> bool:
        """
        Are all specified fields not None?
        """
        return not self.any_fields_none(fields)

    def any_fields_null_or_empty_str(self, fields: List[str]) -> bool:
        """
        Are any specified fields either None or the empty string?
        """
        for f in fields:
            v = getattr(self, f)
            if v is None or v == "":
                return True
        return False

    def are_all_fields_not_null_or_empty_str(self, fields: List[str]) -> bool:
        """
        Are all specified fields neither None nor the empty string?
        """
        return not self.any_fields_null_or_empty_str(fields)

    def n_fields_not_none(self, fields: List[str]) -> int:
        """
        How many of the specified fields are not None?
        """
        total = 0
        for f in fields:
            if getattr(self, f) is not None:
                total += 1
        return total

    def n_fields_none(self, fields: List[str]) -> int:
        """
        How many of the specified fields are None?
        """
        total = 0
        for f in fields:
            if getattr(self, f) is None:
                total += 1
        return total

    def count_booleans(self, fields: List[str]) -> int:
        """
        How many of the specified fields evaluate to True (are truthy)?
        """
        total = 0
        for f in fields:
            value = getattr(self, f)
            if value:
                total += 1
        return total

    def all_truthy(self, fields: List[str]) -> bool:
        """
        Do all the specified fields evaluate to True (are they all truthy)?
        """
        for f in fields:
            value = getattr(self, f)
            if not value:
                return False
        return True

    def count_where(self,
                    fields: List[str],
                    wherevalues: List[Any]) -> int:
        """
        Count how many values for the specified fields are in ``wherevalues``.
        """
        return sum(1 for x in self.get_values(fields) if x in wherevalues)

    def count_wherenot(self,
                       fields: List[str],
                       notvalues: List[Any]) -> int:
        """
        Count how many values for the specified fields are NOT in
        ``notvalues``.
        """
        return sum(1 for x in self.get_values(fields) if x not in notvalues)

    def sum_fields(self,
                   fields: List[str],
                   ignorevalue: Any = None) -> Union[int, float]:
        """
        Sum values stored in all specified fields (skipping any whose value is
        ``ignorevalue``; treating fields containing ``None`` as zero).
        """
        total = 0
        for f in fields:
            value = getattr(self, f)
            if value == ignorevalue:
                continue
            total += value if value is not None else 0
        return total

    def mean_fields(self,
                    fields: List[str],
                    ignorevalue: Any = None) -> Union[int, float, None]:
        """
        Return the mean of the values stored in all specified fields (skipping
        any whose value is ``ignorevalue``).
        """
        values = []
        for f in fields:
            value = getattr(self, f)
            if value != ignorevalue:
                values.append(value)
        try:
            return statistics.mean(values)
        except (TypeError, statistics.StatisticsError):
            return None

    @staticmethod
    def fieldnames_from_prefix(prefix: str, start: int, end: int) -> List[str]:
        """
        Returns a list of field (column, attribute) names from a prefix.
        For example, ``fieldnames_from_prefix("q", 1, 5)`` produces
        ``["q1", "q2", "q3", "q4", "q5"]``.

        Args:
            prefix: string prefix
            start: first value (inclusive)
            end: last value (inclusive

        Returns:
            list of fieldnames, as above

        """
        return [prefix + str(x) for x in range(start, end + 1)]

    @staticmethod
    def fieldnames_from_list(prefix: str,
                             suffixes: Iterable[Any]) -> List[str]:
        """
        Returns a list of fieldnames made by appending each suffix to the
        prefix.

        Args:
            prefix: string prefix
            suffixes: list of suffixes, which will be coerced to ``str``

        Returns:
            list of fieldnames, as above

        """
        return [prefix + str(x) for x in suffixes]

    # -------------------------------------------------------------------------
    # Extra strings
    # -------------------------------------------------------------------------

    def get_extrastring_taskname(self) -> str:
        """
        Get the taskname used as the top-level key for this task's extra
        strings (loaded by the server from XML files). By default this is the
        task's primary tablename, but tasks may override that via
        ``extrastring_taskname``.
        """
        return self.extrastring_taskname or self.tablename

    def extrastrings_exist(self, req: "CamcopsRequest") -> bool:
        """
        Does the server have any extra strings for this task?
        """
        return req.task_extrastrings_exist(self.get_extrastring_taskname())

    def wxstring(self,
                 req: "CamcopsRequest",
                 name: str,
                 defaultvalue: str = None,
                 provide_default_if_none: bool = True) -> str:
        """
        Return a web-safe version of an extra string for this task.

        Args:
            req: :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            name: name (second-level key) of the string, within the set of
                this task's extra strings
            defaultvalue: default to return if the string is not found
            provide_default_if_none: if ``True`` and ``default is None``,
                return a helpful missing-string message in the style
                "string x.y not found"
        """
        if defaultvalue is None and provide_default_if_none:
            defaultvalue = f"[{self.get_extrastring_taskname()}: {name}]"
        return req.wxstring(
            self.get_extrastring_taskname(),
            name,
            defaultvalue,
            provide_default_if_none=provide_default_if_none)

    def xstring(self,
                req: "CamcopsRequest",
                name: str,
                defaultvalue: str = None,
                provide_default_if_none: bool = True) -> str:
        """
        Return a raw (not necessarily web-safe) version of an extra string for
        this task.

        Args:
            req: :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            name: name (second-level key) of the string, within the set of
                this task's extra strings
            defaultvalue: default to return if the string is not found
            provide_default_if_none: if ``True`` and ``default is None``,
                return a helpful missing-string message in the style
                "string x.y not found"
        """
        if defaultvalue is None and provide_default_if_none:
            defaultvalue = f"[{self.get_extrastring_taskname()}: {name}]"
        return req.xstring(
            self.get_extrastring_taskname(),
            name,
            defaultvalue,
            provide_default_if_none=provide_default_if_none)

    def make_options_from_xstrings(self,
                                   req: "CamcopsRequest",
                                   prefix: str, first: int, last: int,
                                   suffix: str = "") -> Dict[int, str]:
        """
        Creates a lookup dictionary from xstrings.

        Args:
            req: :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            prefix: prefix for xstring
            first: first value
            last: last value
            suffix: optional suffix

        Returns:
            dict: Each entry maps ``value`` to an xstring named
            ``<PREFIX><VALUE><SUFFIX>``.

        """
        d = {}  # type: Dict[int, str]
        if first > last:  # descending order
            for i in range(first, last - 1, -1):
                d[i] = self.xstring(req, f"{prefix}{i}{suffix}")
        else:  # ascending order
            for i in range(first, last + 1):
                d[i] = self.xstring(req, f"{prefix}{i}{suffix}")
        return d

    @staticmethod
    def make_options_from_numbers(first: int, last: int) -> Dict[int, str]:
        """
        Creates a simple dictionary mapping numbers to string versions of those
        numbers. Usually for subsequent (more interesting) processing!

        Args:
            first: first value
            last: last value

        Returns:
            dict

        """
        d = {}  # type: Dict[int, str]
        if first > last:  # descending order
            for i in range(first, last - 1, -1):
                d[i] = str(i)
        else:  # ascending order
            for i in range(first, last + 1):
                d[i] = str(i)
        return d


# =============================================================================
# Collating all task tables for specific purposes
# =============================================================================
# Function, staticmethod, classmethod?
# https://stackoverflow.com/questions/8108688/in-python-when-should-i-use-a-function-instead-of-a-method  # noqa
# https://stackoverflow.com/questions/11788195/module-function-vs-staticmethod-vs-classmethod-vs-no-decorators-which-idiom-is  # noqa
# https://stackoverflow.com/questions/15017734/using-static-methods-in-python-best-practice  # noqa

def all_task_tables_with_min_client_version() -> Dict[str, Version]:
    """
    Across all tasks, return a mapping from each of their tables to the
    minimum client version.

    Used by
    :func:`camcops_server.cc_modules.client_api.all_tables_with_min_client_version`.

    """  # noqa
    d = {}  # type: Dict[str, Version]
    classes = list(Task.gen_all_subclasses())
    for cls in classes:
        d.update(cls.all_tables_with_min_client_version())
    return d


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def tablename_to_task_class_dict() -> Dict[str, Type[Task]]:
    """
    Returns a mapping from task base tablenames to task classes.
    """
    d = {}  # type: Dict[str, Type[Task]]
    for cls in Task.gen_all_subclasses():
        d[cls.tablename] = cls
    return d


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def all_task_tablenames() -> List[str]:
    """
    Returns all task base table names.
    """
    d = tablename_to_task_class_dict()
    return list(d.keys())


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def all_task_classes() -> List[Type[Task]]:
    """
    Returns all task base table names.
    """
    d = tablename_to_task_class_dict()
    return list(d.values())


# =============================================================================
# Support functions
# =============================================================================

def get_from_dict(d: Dict, key: Any, default: Any = INVALID_VALUE) -> Any:
    """
    Returns a value from a dictionary. This is not a very complex function...
    all it really does in practice is provide a default for ``default``.

    Args:
        d: the dictionary
        key: the key
        default: value to return if none is provided
    """
    return d.get(key, default)
