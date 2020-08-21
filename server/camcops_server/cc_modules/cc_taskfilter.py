#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_taskfilter.py

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

**Representation of filtering criteria for tasks.**

"""

import datetime
from enum import Enum
import logging
from typing import Dict, List, Optional, Type, TYPE_CHECKING, Union

from cardinal_pythonlib.datetimefunc import convert_datetime_to_utc
from cardinal_pythonlib.json.serialize import register_class_for_json
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import auto_repr
from cardinal_pythonlib.sqlalchemy.list_types import (
    IntListType,
    StringListType,
)
from pendulum import DateTime as Pendulum
from sqlalchemy.orm import Query, reconstructor
from sqlalchemy.sql.functions import func
from sqlalchemy.sql.expression import and_, or_
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Date, Integer

from camcops_server.cc_modules.cc_cache import cache_region_static, fkg
from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_group import Group
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_sqla_coltypes import (
    PendulumDateTimeAsIsoTextColType,
    IdNumReferenceListColType,
    PatientNameColType,
    SexColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_task import (
    tablename_to_task_class_dict,
    Task,
)
from camcops_server.cc_modules.cc_taskindex import PatientIdNumIndexEntry
from camcops_server.cc_modules.cc_user import User

if TYPE_CHECKING:
    from sqlalchemy.sql.elements import ColumnElement
    from camcops_server.cc_modules.cc_request import CamcopsRequest
    from camcops_server.cc_modules.cc_simpleobjects import IdNumReference

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Sorting helpers
# =============================================================================

class TaskClassSortMethod(Enum):
    """
    Enum to represent ways to sort task types (classes).
    """
    NONE = 0
    TABLENAME = 1
    SHORTNAME = 2
    LONGNAME = 3


def sort_task_classes_in_place(classlist: List[Type[Task]],
                               sortmethod: TaskClassSortMethod,
                               req: "CamcopsRequest" = None) -> None:
    """
    Sort a list of task classes in place.

    Args:
        classlist: the list of task classes
        sortmethod: a :class:`TaskClassSortMethod` enum
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
    """
    if sortmethod == TaskClassSortMethod.TABLENAME:
        classlist.sort(key=lambda c: c.tablename)
    elif sortmethod == TaskClassSortMethod.SHORTNAME:
        classlist.sort(key=lambda c: c.shortname)
    elif sortmethod == TaskClassSortMethod.LONGNAME:
        assert req is not None
        classlist.sort(key=lambda c: c.longname(req))


# =============================================================================
# Cache task class mapping
# =============================================================================
# Function, staticmethod, classmethod?
# https://stackoverflow.com/questions/8108688/in-python-when-should-i-use-a-function-instead-of-a-method  # noqa
# https://stackoverflow.com/questions/11788195/module-function-vs-staticmethod-vs-classmethod-vs-no-decorators-which-idiom-is  # noqa
# https://stackoverflow.com/questions/15017734/using-static-methods-in-python-best-practice  # noqa

def task_classes_from_table_names(
        tablenames: List[str],
        sortmethod: TaskClassSortMethod = TaskClassSortMethod.NONE) \
        -> List[Type[Task]]:
    """
    Transforms a list of task base tablenames into a list of task classes,
    appropriately sorted.

    Args:
        tablenames: list of task base table names
        sortmethod: a :class:`TaskClassSortMethod` enum

    Returns:
        a list of task classes, in the order requested

    Raises:
        :exc:`KeyError` if a table name is invalid

    """
    assert sortmethod != TaskClassSortMethod.LONGNAME
    d = tablename_to_task_class_dict()
    classes = []  # type: List[Type[Task]]
    for tablename in tablenames:
        cls = d[tablename]
        classes.append(cls)
    sort_task_classes_in_place(classes, sortmethod)
    return classes


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def all_tracker_task_classes() -> List[Type[Task]]:
    """
    Returns a list of all task classes that provide tracker information.
    """
    return [cls for cls in Task.all_subclasses_by_shortname()
            if cls.provides_trackers]


# =============================================================================
# Define a filter to apply to tasks
# =============================================================================

class TaskFilter(Base):
    """
    SQLAlchemy ORM object representing task filter criteria.
    """
    __tablename__ = "_task_filters"

    # Lots of these could be changed into lists; for example, filtering to
    # multiple devices, multiple users, multiple text patterns. For
    # AND-joining, there is little clear benefit (one could always AND-join
    # multiple filters with SQL). For OR-joining, this is more useful.
    # - surname: use ID numbers instead; not very likely to have >1 surname
    # - forename: ditto
    # - DOB: ditto
    # - sex: just eliminate the filter if you don't care about sex
    # - task_types: needs a list
    # - device_id: might as well make it a list
    # - user_id: might as well make it a list
    # - group_id: might as well make it a list
    # - start_datetime: single only
    # - end_datetime: single only
    # - text_contents: might as well make it a list
    # - ID numbers: a list, joined with OR.
    id = Column(
        "id", Integer,
        primary_key=True, autoincrement=True, index=True,
        comment="Task filter ID (arbitrary integer)"
    )
    # Task type filters
    task_types = Column(
        "task_types", StringListType,
        comment="Task filter: task type(s), as CSV list of table names"
    )
    tasks_offering_trackers_only = Column(
        "tasks_offering_trackers_only", Boolean,
        comment="Task filter: restrict to tasks offering trackers only?"
    )
    tasks_with_patient_only = Column(
        "tasks_with_patient_only", Boolean,
        comment="Task filter: restrict to tasks with a patient (non-anonymous "
                "tasks) only?"
    )
    # Patient-related filters
    surname = Column(
        "surname", PatientNameColType,
        comment="Task filter: surname"
    )
    forename = Column(
        "forename", PatientNameColType,
        comment="Task filter: forename"
    )
    dob = Column(
        "dob", Date,
        comment="Task filter: DOB"
    )  # type: Optional[datetime.date]
    sex = Column(
        "sex", SexColType,
        comment="Task filter: sex"
    )
    idnum_criteria = Column(  # new in v2.0.1
        "idnum_criteria", IdNumReferenceListColType,
        comment="ID filters as JSON; the ID number definitions are joined "
                "with OR"
    )
    # Other filters
    device_ids = Column(
        "device_ids", IntListType,
        comment="Task filter: source device ID(s), as CSV"
    )
    adding_user_ids = Column(
        "user_ids", IntListType,
        comment="Task filter: adding (uploading) user ID(s), as CSV"
    )
    group_ids = Column(
        "group_ids", IntListType,
        comment="Task filter: group ID(s), as CSV"
    )
    start_datetime = Column(
        "start_datetime_iso8601", PendulumDateTimeAsIsoTextColType,
        comment="Task filter: start date/time (UTC as ISO8601)"
    )  # type: Union[None, Pendulum, datetime.datetime]
    end_datetime = Column(
        "end_datetime_iso8601", PendulumDateTimeAsIsoTextColType,
        comment="Task filter: end date/time (UTC as ISO8601)"
    )  # type: Union[None, Pendulum, datetime.datetime]
    # Implemented on the Python side for indexed lookup:
    text_contents = Column(
        "text_contents", StringListType,
        comment="Task filter: filter text fields"
    )  # task must contain ALL the strings in AT LEAST ONE of its text columns
    # Implemented on the Python side for non-indexed lookup:
    complete_only = Column(
        "complete_only", Boolean,
        comment="Task filter: task complete?"
    )

    def __init__(self) -> None:
        # We need to initialize these explicitly, because if we create an
        # instance via "x = TaskFilter()", they will be initialized to None,
        # without any recourse to our database to-and-fro conversion code for
        # each fieldtype.
        # (If we load from a database, things will be fine.)
        self.idnum_criteria = []  # type: List[IdNumReference]
        self.device_ids = []  # type: List[int]
        self.adding_user_ids = []  # type: List[int]
        self.group_ids = []  # type: List[int]
        self.text_contents = []  # type: List[str]

        # ANYTHING YOU ADD BELOW HERE MUST ALSO BE IN init_on_load().
        # Or call it, of course, but we like to keep on the happy side of the
        # PyCharm type checker.

        # Python-only filtering attributes (i.e. not saved to database)
        self.era = None  # type: Optional[str]
        self.finalized_only = False  # used for exports
        self.must_have_idnum_type = None  # type: Optional[int]

        # Other Python-only attributes
        self._sort_method = TaskClassSortMethod.NONE
        self._task_classes = None  # type: Optional[List[Type[Task]]]

    @reconstructor
    def init_on_load(self):
        """
        SQLAlchemy function to recreate after loading from the database.
        """
        self.era = None  # type: Optional[str]
        self.finalized_only = False
        self.must_have_idnum_type = None  # type: Optional[int]

        self._sort_method = TaskClassSortMethod.NONE
        self._task_classes = None  # type: Optional[List[Type[Task]]]

    def __repr__(self) -> str:
        return auto_repr(self, with_addr=True)

    def set_sort_method(self, sort_method: TaskClassSortMethod) -> None:
        """
        Sets the sorting method for task types.
        """
        assert sort_method != TaskClassSortMethod.LONGNAME, (
            "If you want to use that sorting method, you need to save a "
            "request object, because long task names use translation"
        )
        self._sort_method = sort_method

    @property
    def task_classes(self) -> List[Type[Task]]:
        """
        Return a list of task classes permitted by the filter.

        Uses caching, since the filter will be called repeatedly.
        """
        if self._task_classes is None:
            self._task_classes = []  # type: List[Type[Task]]
            if self.task_types:
                starting_classes = task_classes_from_table_names(
                    self.task_types)
            else:
                starting_classes = Task.all_subclasses_by_shortname()
            skip_anonymous_tasks = self.skip_anonymous_tasks()
            for cls in starting_classes:
                if (self.tasks_offering_trackers_only and
                        not cls.provides_trackers):
                    # Class doesn't provide trackers; skip
                    continue
                if skip_anonymous_tasks and not cls.has_patient:
                    # Anonymous task; skip
                    continue
                if self.text_contents and not cls.get_text_filter_columns():
                    # Text filter and task has no text columns; skip
                    continue
                self._task_classes.append(cls)
            sort_task_classes_in_place(self._task_classes, self._sort_method)
        return self._task_classes

    def skip_anonymous_tasks(self) -> bool:
        """
        Should we skip anonymous tasks?
        """
        return self.tasks_with_patient_only or self.any_patient_filtering()

    def offers_all_task_types(self) -> bool:
        """
        Does this filter offer every single task class? Used for efficiency
        when using indexes. (Since ignored.)
        """
        if self.tasks_offering_trackers_only:
            return False
        if self.skip_anonymous_tasks():
            return False
        if not self.task_types:
            return True
        return set(self.task_classes) == set(Task.all_subclasses_by_shortname)

    def offers_all_non_anonymous_task_types(self) -> bool:
        """
        Does this filter offer every single non-anonymous task class? Used for
        efficiency when using indexes.
        """
        offered_task_classes = self.task_classes
        for taskclass in Task.all_subclasses_by_shortname():
            if taskclass.is_anonymous:
                continue
            if taskclass not in offered_task_classes:
                return False
        return True

    @property
    def task_tablename_list(self) -> List[str]:
        """
        Returns the base table names for all task types permitted by the
        filter.
        """
        return [cls.__tablename__ for cls in self.task_classes]

    def any_patient_filtering(self) -> bool:
        """
        Is some sort of patient filtering being applied?
        """
        return (
            bool(self.surname) or
            bool(self.forename) or
            (self.dob is not None) or
            bool(self.sex) or
            bool(self.idnum_criteria)
        )

    def any_specific_patient_filtering(self) -> bool:
        """
        Are there filters that would restrict to one or a few patients?

        (Differs from :func:`any_patient_filtering` with respect to sex.)
        """
        return (
            bool(self.surname) or
            bool(self.forename) or
            self.dob is not None or
            bool(self.idnum_criteria)
        )

    def get_only_iddef(self) -> Optional["IdNumReference"]:
        """
        If a single ID number type/value restriction is being applied, return
        it, as an
        :class:`camcops_server.cc_modules.cc_simpleobjects.IdNumReference`.
        Otherwise, return ``None``.
        """
        if len(self.idnum_criteria) != 1:
            return None
        return self.idnum_criteria[0]

    def get_group_names(self, req: "CamcopsRequest") -> List[str]:
        """
        Get the names of any groups to which we are restricting.
        """
        groups = (
            req.dbsession.query(Group)
            .filter(Group.id.in_(self.group_ids))
            .all()
        )  # type: List[Group]
        return [g.name if g and g.name else "" for g in groups]

    def get_user_names(self, req: "CamcopsRequest") -> List[str]:
        """
        Get the usernames of any uploading users to which we are restricting.
        """
        users = (
            req.dbsession.query(User)
            .filter(User.id.in_(self.adding_user_ids))
            .all()
        )  # type: List[User]
        return [u.username if u and u.username else "" for u in users]

    def get_device_names(self, req: "CamcopsRequest") -> List[str]:
        """
        Get the names of any devices to which we are restricting.
        """
        devices = (
            req.dbsession.query(Device)
            .filter(Device.id.in_(self.device_ids))
            .all()
        )  # type: List[Device]
        return [d.name if d and d.name else "" for d in devices]

    def clear(self) -> None:
        """
        Clear all parts of the filter.
        """
        self.task_types = []  # type: List[str]

        self.surname = None
        self.forename = None
        self.dob = None  # type: Optional[datetime.date]
        self.sex = None
        self.idnum_criteria = []  # type: List[IdNumReference]

        self.device_ids = []  # type: List[int]
        self.adding_user_ids = []  # type: List[int]
        self.group_ids = []  # type: List[int]
        self.start_datetime = None  # type: Union[None, Pendulum, datetime.datetime]  # noqa
        self.end_datetime = None  # type: Union[None, Pendulum, datetime.datetime]  # noqa
        self.text_contents = []  # type: List[str]

        self.complete_only = None  # type: Optional[bool]

    def dates_inconsistent(self) -> bool:
        """
        Are inconsistent dates specified, such that no tasks should be
        returned?
        """
        return (self.start_datetime and self.end_datetime and
                self.end_datetime < self.start_datetime)

    def filter_query_by_patient(self, q: Query,
                                via_index: bool) -> Query:
        """
        Restricts an query that has *already been joined* to the
        :class:`camcops_server.cc_modules.cc_patient.Patient` class, according
        to the patient filtering criteria.

        Args:
            q: the starting SQLAlchemy ORM Query
            via_index:
                If ``True``, the query relates to a
                :class:`camcops_server.cc_modules.cc_taskindex.TaskIndexEntry`
                and we should restrict it according to the
                :class:`camcops_server.cc_modules.cc_taskindex.PatientIdNumIndexEntry`
                class. If ``False``, the query relates to a
                :class:`camcops_server.cc_modules.cc_taskindex.Task` and we
                should restrict according to
                :class:`camcops_server.cc_modules.cc_patientidnum.PatientIdNum`.

        Returns:
            a revised Query

        """
        if self.surname:
            q = q.filter(func.upper(Patient.surname) ==
                         self.surname.upper())
        if self.forename:
            q = q.filter(func.upper(Patient.forename) ==
                         self.forename.upper())
        if self.dob is not None:
            q = q.filter(Patient.dob == self.dob)
        if self.sex:
            q = q.filter(func.upper(Patient.sex) == self.sex.upper())

        if self.idnum_criteria:
            id_filter_parts = []  # type: List[ColumnElement]
            if via_index:
                q = q.join(PatientIdNumIndexEntry)
                # "Specify possible ID number values"
                for iddef in self.idnum_criteria:
                    id_filter_parts.append(and_(
                        PatientIdNumIndexEntry.which_idnum == iddef.which_idnum,  # noqa
                        PatientIdNumIndexEntry.idnum_value == iddef.idnum_value
                    ))
                # Use OR (disjunction) of the specified values:
                q = q.filter(or_(*id_filter_parts))
                # "Must have a value for a given ID number type"
                if self.must_have_idnum_type:
                    # noinspection PyComparisonWithNone,PyPep8
                    q = q.filter(and_(
                        PatientIdNumIndexEntry.which_idnum == self.must_have_idnum_type,  # noqa
                        PatientIdNumIndexEntry.idnum_value != None  # noqa: E711
                    ))
            else:
                # q = q.join(PatientIdNum) # fails
                q = q.join(Patient.idnums)
                # "Specify possible ID number values"
                for iddef in self.idnum_criteria:
                    id_filter_parts.append(and_(
                        PatientIdNum.which_idnum == iddef.which_idnum,
                        PatientIdNum.idnum_value == iddef.idnum_value
                    ))
                # Use OR (disjunction) of the specified values:
                q = q.filter(or_(*id_filter_parts))
                # "Must have a value for a given ID number type"
                if self.must_have_idnum_type:
                    # noinspection PyComparisonWithNone,PyPep8
                    q = q.filter(and_(
                        PatientIdNum.which_idnum == self.must_have_idnum_type,
                        PatientIdNum.idnum_value != None  # noqa: E711
                    ))

        return q

    @property
    def start_datetime_utc(self) -> Optional[Pendulum]:
        if not self.start_datetime:
            return None
        return convert_datetime_to_utc(self.start_datetime)

    @property
    def end_datetime_utc(self) -> Optional[Pendulum]:
        if not self.end_datetime:
            return None
        return convert_datetime_to_utc(self.end_datetime)


def encode_task_filter(taskfilter: TaskFilter) -> Dict:
    return {
        "task_types": taskfilter.task_types,
        "group_ids": taskfilter.group_ids,
    }


# noinspection PyUnusedLocal
def decode_task_filter(d: Dict, cls: Type) -> TaskFilter:
    taskfilter = TaskFilter()
    taskfilter.task_types = d["task_types"]
    taskfilter.group_ids = d["group_ids"]

    return taskfilter


register_class_for_json(
    cls=TaskFilter,
    obj_to_dict_fn=encode_task_filter,
    dict_to_obj_fn=decode_task_filter
)
