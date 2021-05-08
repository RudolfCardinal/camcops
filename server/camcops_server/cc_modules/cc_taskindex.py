#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_taskindex.py

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

**Server-side task index.**

Note in particular that if you, as a developer, change the ``is_complete()``
criteria for a task, you should cause the server index to be rebuilt (because
it caches ``is_complete()`` information).

"""

import logging
from typing import List, Optional, Type, TYPE_CHECKING

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import simple_repr
from cardinal_pythonlib.sqlalchemy.session import get_engine_from_session
from cardinal_pythonlib.sqlalchemy.schema import table_exists
from cardinal_pythonlib.sqlalchemy.sqlserver import (
    if_sqlserver_disable_constraints_triggers,
)
from pendulum import DateTime as Pendulum
import pyramid.httpexceptions as exc
from sqlalchemy.orm import relationship, Session as SqlASession
from sqlalchemy.sql.expression import and_, exists, join, literal, select
from sqlalchemy.sql.schema import Column, ForeignKey, Table
from sqlalchemy.sql.sqltypes import BigInteger, Boolean, DateTime, Integer

from camcops_server.cc_modules.cc_client_api_core import (
    BatchDetails,
    fail_user_error,
    UploadTableChanges,
)
from camcops_server.cc_modules.cc_constants import ERA_NOW
from camcops_server.cc_modules.cc_idnumdef import IdNumDefinition
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_sqla_coltypes import (
    EraColType,
    isotzdatetime_to_utcdatetime,
    PendulumDateTimeAsIsoTextColType,
    TableNameColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_task import (
    all_task_tablenames,
    tablename_to_task_class_dict,
    Task,
)
from camcops_server.cc_modules.cc_user import User

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Helper functions
# =============================================================================

def task_factory_unfiltered(dbsession: SqlASession,
                            basetable: str,
                            serverpk: int) -> Optional[Task]:
    """
    Load a task from the database and return it.
    No permission filtering is performed. (Used by
    :class:`camcops_server.cc_modules.cc_taskindex.TaskIndexEntry`.)

    Args:
        dbsession: a :class:`sqlalchemy.orm.session.Session`
        basetable: name of the task's base table
        serverpk: server PK of the task

    Returns:
        the task, or ``None`` if the PK doesn't exist

    Raises:
        :exc:`HTTPBadRequest` if the table doesn't exist
    """
    d = tablename_to_task_class_dict()
    try:
        cls = d[basetable]  # may raise KeyError
    except KeyError:
        raise exc.HTTPBadRequest(f"No such task table: {basetable!r}")
    # noinspection PyProtectedMember
    q = dbsession.query(cls).filter(cls._pk == serverpk)
    return q.first()


# =============================================================================
# PatientIdNumIndexEntry
# =============================================================================

class PatientIdNumIndexEntry(Base):
    """
    Represents a server index entry for a
    :class:`camcops_server.cc_modules.cc_patientidnum.PatientIdNum`.

    - Only current ID numbers are indexed.
    """
    __tablename__ = "_idnum_index"

    idnum_pk = Column(
        "idnum_pk", Integer,
        primary_key=True, index=True,
        comment="Server primary key of the PatientIdNum "
                "(and of the PatientIdNumIndexEntry)"
    )
    indexed_at_utc = Column(
        "indexed_at_utc", DateTime, nullable=False,
        comment="When this index entry was created"
    )

    # noinspection PyProtectedMember
    patient_pk = Column(
        "patient_pk", Integer, ForeignKey(Patient._pk),
        index=True,
        comment="Server primary key of the Patient"
    )
    which_idnum = Column(
        "which_idnum", Integer, ForeignKey(IdNumDefinition.which_idnum),
        nullable=False,
        index=True,
        comment="Which of the server's ID numbers is this?"
    )
    idnum_value = Column(
        "idnum_value", BigInteger,
        comment="The value of the ID number"
    )

    # Relationships:
    patient = relationship(Patient)

    def __repr__(self) -> str:
        return simple_repr(self, ["idnum_pk", "patient_pk",
                                  "which_idnum", "idnum_value"])

    # -------------------------------------------------------------------------
    # Create
    # -------------------------------------------------------------------------

    @classmethod
    def make_from_idnum(cls, idnum: PatientIdNum) -> "PatientIdNumIndexEntry":
        """
        Returns an ID index entry for the specified
        :class:`camcops_server.cc_modules.cc_patientidnum.PatientIdNum`. The
        returned index requires inserting into a database session.
        """
        # noinspection PyProtectedMember
        assert idnum._current, "Only index current PatientIdNum objects"
        index = cls()
        index.idnum_pk = idnum.pk
        index.patient_pk = idnum.get_patient_server_pk()
        index.which_idnum = idnum.which_idnum
        index.idnum_value = idnum.idnum_value
        index.indexed_at_utc = Pendulum.now()
        return index

    @classmethod
    def index_idnum(cls, idnum: PatientIdNum, session: SqlASession) -> None:
        """
        Indexes an ID number and inserts the index into the database.

        Args:
            idnum: a
                :class:`camcops_server.cc_modules.cc_patientidnum.PatientIdNum`
            session:
                an SQLAlchemy Session
        """
        index = cls.make_from_idnum(idnum)
        session.add(index)

    @classmethod
    def unindex_patient(cls, patient: Patient,
                        session: SqlASession) -> None:
        """
        Removes all ID number indexes from the database for a patient.

        Args:
            patient:
                :class:`camcops_server.cc_modules.cc_patient.Patient`
            session:
                an SQLAlchemy Session
        """

        # noinspection PyUnresolvedReferences
        idxtable = cls.__table__  # type: Table
        idxcols = idxtable.columns
        # noinspection PyProtectedMember
        session.execute(
            idxtable.delete()
            .where(idxcols.patient_pk == patient._pk)
        )

    # -------------------------------------------------------------------------
    # Regenerate index
    # -------------------------------------------------------------------------

    @classmethod
    def rebuild_idnum_index(cls, session: SqlASession,
                            indexed_at_utc: Pendulum) -> None:
        """
        Rebuilds the index entirely. Uses SQLAlchemy Core (not ORM) for speed.

        Args:
            session: an SQLAlchemy Session
            indexed_at_utc: current time in UTC
        """
        log.info("Rebuilding patient ID number index")
        # noinspection PyUnresolvedReferences
        indextable = PatientIdNumIndexEntry.__table__  # type: Table
        indexcols = indextable.columns
        # noinspection PyUnresolvedReferences
        idnumtable = PatientIdNum.__table__  # type: Table
        idnumcols = idnumtable.columns
        # noinspection PyUnresolvedReferences
        patienttable = Patient.__table__  # type: Table
        patientcols = patienttable.columns

        # Delete all entries
        with if_sqlserver_disable_constraints_triggers(session,
                                                       indextable.name):
            session.execute(
                indextable.delete()
            )

        # Create new ones
        # noinspection PyProtectedMember,PyPep8
        session.execute(
            indextable.insert().from_select(
                # Target:
                [indexcols.idnum_pk,
                 indexcols.indexed_at_utc,
                 indexcols.patient_pk,
                 indexcols.which_idnum,
                 indexcols.idnum_value],
                # Source:
                (
                    select([idnumcols._pk,
                            literal(indexed_at_utc),
                            patientcols._pk,
                            idnumcols.which_idnum,
                            idnumcols.idnum_value])
                    .select_from(
                        join(
                            idnumtable,
                            patienttable,
                            and_(
                                idnumcols._device_id == patientcols._device_id,
                                idnumcols._era == patientcols._era,
                                idnumcols.patient_id == patientcols.id,
                            )
                        )
                    )
                    .where(idnumcols._current == True)  # noqa: E712
                    .where(idnumcols.idnum_value.isnot(None))
                    .where(patientcols._current == True)  # noqa: E712
                )
            )
        )

    # -------------------------------------------------------------------------
    # Check index
    # -------------------------------------------------------------------------
    @classmethod
    def check_index(cls, session: SqlASession,
                    show_all_bad: bool = False) -> bool:
        """
        Checks the index.

        Args:
            session:
                an SQLAlchemy Session
            show_all_bad:
                show all bad entries? (If false, return upon the first)

        Returns:
            bool: is the index OK?
        """
        ok = True

        log.info(
            "Checking all patient ID number indexes represent valid entries")
        # noinspection PyUnresolvedReferences,PyProtectedMember
        q_idx_without_original = session.query(PatientIdNumIndexEntry).filter(
            ~exists()
            .select_from(
                PatientIdNum.__table__.join(
                    Patient.__table__,
                    Patient.id == PatientIdNum.patient_id,
                    Patient._device_id == PatientIdNum._device_id,
                    Patient._era == PatientIdNum._era,
                )
            ).where(and_(
                PatientIdNum._pk == PatientIdNumIndexEntry.idnum_pk,
                PatientIdNum._current == True,  # noqa: E712
                PatientIdNum.which_idnum == PatientIdNumIndexEntry.which_idnum,
                PatientIdNum.idnum_value == PatientIdNumIndexEntry.idnum_value,
                Patient._pk == PatientIdNumIndexEntry.patient_pk,
                Patient._current == True,  # noqa: E712
            ))
        )
        for index in q_idx_without_original:
            log.error("Patient ID number index without matching "
                      "original: {!r}", index)
            ok = False
            if not show_all_bad:
                return ok

        log.info("Checking all patient ID numbers have an index")
        # noinspection PyUnresolvedReferences,PyProtectedMember
        q_original_with_idx = session.query(PatientIdNum).filter(
            PatientIdNum._current == True,  # noqa: E712
            PatientIdNum.idnum_value.isnot(None),
            ~exists()
            .select_from(
                PatientIdNumIndexEntry.__table__
            ).where(and_(
                PatientIdNum._pk == PatientIdNumIndexEntry.idnum_pk,
                PatientIdNum.which_idnum == PatientIdNumIndexEntry.which_idnum,  # noqa
                PatientIdNum.idnum_value == PatientIdNumIndexEntry.idnum_value,  # noqa
            ))
        )
        for orig in q_original_with_idx:
            log.error("ID number without index entry: {!r}", orig)
            ok = False
            if not show_all_bad:
                return ok

        return ok

    # -------------------------------------------------------------------------
    # Update index at the point of upload from a device
    # -------------------------------------------------------------------------

    @classmethod
    def update_idnum_index_for_upload(
            cls,
            session: SqlASession,
            indexed_at_utc: Pendulum,
            tablechanges: UploadTableChanges) -> None:
        """
        Updates the index for a device's upload.

        - Deletes index entries for records that are on the way out.
        - Creates index entries for records that are on the way in.
        - Should be called after both the Patient and PatientIdNum tables are
          committed; see special ordering in
          :func:`camcops_server.cc_modules.client_api.commit_all`.

        Args:
            session:
                an SQLAlchemy Session
            indexed_at_utc:
                current time in UTC
            tablechanges:
                a :class:`camcops_server.cc_modules.cc_client_api_core.UploadTableChanges`
                object describing the changes to a table
        """  # noqa
        # noinspection PyUnresolvedReferences
        indextable = PatientIdNumIndexEntry.__table__  # type: Table
        indexcols = indextable.columns
        # noinspection PyUnresolvedReferences
        idnumtable = PatientIdNum.__table__  # type: Table
        idnumcols = idnumtable.columns
        # noinspection PyUnresolvedReferences
        patienttable = Patient.__table__  # type: Table
        patientcols = patienttable.columns

        # Delete the old
        removal_pks = tablechanges.idnum_delete_index_pks
        if removal_pks:
            log.debug("Deleting old ID number indexes: server PKs {}",
                      removal_pks)
            session.execute(
                indextable.delete()
                .where(indextable.c.idnum_pk.in_(removal_pks))
            )

        # Create the new
        addition_pks = tablechanges.idnum_add_index_pks
        if addition_pks:
            log.debug("Adding ID number indexes: server PKs {}", addition_pks)
            # noinspection PyPep8,PyProtectedMember
            session.execute(
                indextable.insert().from_select(
                    # Target:
                    [indexcols.idnum_pk,
                     indexcols.indexed_at_utc,
                     indexcols.patient_pk,
                     indexcols.which_idnum,
                     indexcols.idnum_value],
                    # Source:
                    (
                        select([idnumcols._pk,
                                literal(indexed_at_utc),
                                patientcols._pk,
                                idnumcols.which_idnum,
                                idnumcols.idnum_value])
                        .select_from(
                            join(
                                idnumtable,
                                patienttable,
                                and_(
                                    idnumcols._device_id == patientcols._device_id,  # noqa
                                    idnumcols._era == patientcols._era,
                                    idnumcols.patient_id == patientcols.id,
                                )
                            )
                        )
                        .where(idnumcols._pk.in_(addition_pks))
                        .where(patientcols._current == True)  # noqa: E712
                    )
                )
            )


# =============================================================================
# TaskIndexEntry
# =============================================================================

class TaskIndexEntry(Base):
    """
    Represents a server index entry for a
    :class:`camcops_server.cc_modules.cc_task.Task`.

    - Only current tasks are indexed. This simplifies direct linking to patient
      PKs.
    """
    __tablename__ = "_task_index"

    index_entry_pk = Column(
        "index_entry_pk", Integer,
        primary_key=True, autoincrement=True,
        comment="Arbitrary primary key of this index entry"
    )
    indexed_at_utc = Column(
        "indexed_at_utc", DateTime, nullable=False,
        comment="When this index entry was created"
    )

    # The next two fields link to our task:
    task_table_name = Column(
        "task_table_name", TableNameColType,
        index=True,
        comment="Table name of the task's base table"
    )
    task_pk = Column(
        "task_pk", Integer,
        index=True,
        comment="Server primary key of the task"
    )
    # We can probably even represent this with an SQLAlchemy ORM relationship.
    # This is polymorphic loading (we'll return objects of different types)
    # based on concrete table inheritance (each type of object -- each task --
    # has its own standalone table).
    # However, there are warnings about the inefficiency of this; see
    # https://docs.sqlalchemy.org/en/latest/orm/inheritance.html#concrete-table-inheritance
    # and we are trying to be efficient. So let's do via task() below.

    # This links to the task's patient, if there is one:
    # noinspection PyProtectedMember
    patient_pk = Column(
        "patient_pk", Integer, ForeignKey(Patient._pk),
        index=True,
        comment="Server primary key of the patient (if applicable)"
    )

    # These fields allow us to filter tasks efficiently:
    device_id = Column(
        "device_id", Integer, ForeignKey("_security_devices.id"),
        nullable=False,
        index=True,
        comment="ID of the source tablet device"
    )
    era = Column(
        "era", EraColType, nullable=False,
        index=True,
        comment="Era (_era) field of the source record",
    )
    when_created_utc = Column(
        "when_created_utc", DateTime, nullable=False,
        index=True,
        comment="Date/time this task instance was created (UTC)"
    )
    when_created_iso = Column(
        "when_created_iso", PendulumDateTimeAsIsoTextColType, nullable=False,
        index=True,
        comment="Date/time this task instance was created (ISO 8601)"
    )  # Pendulum on the Python side
    when_added_batch_utc = Column(
        "when_added_batch_utc", DateTime, nullable=False,
        index=True,
        comment="Date/time this task index was uploaded (UTC)"
    )
    adding_user_id = Column(
        "adding_user_id", Integer, ForeignKey("_security_users.id"),
        comment="ID of user that added this task",
    )
    group_id = Column(
        "group_id", Integer, ForeignKey("_security_groups.id"),
        nullable=False, index=True,
        comment="ID of group to which this task belongs"
    )
    task_is_complete = Column(
        "task_is_complete", Boolean, nullable=False,
        comment="Is the task complete (as judged by the server when the index "
                "entry was created)?"
    )

    # Relationships:
    patient = relationship(Patient)
    _adding_user = relationship(User)

    def __repr__(self) -> str:
        return simple_repr(self, [
            "index_entry_pk", "task_table_name", "task_pk", "patient_pk",
            "device_id", "era", "when_created_utc", "when_created_iso",
            "when_added_batch_utc",
            "adding_user_id", "group_id", "task_is_complete",
        ])

    # -------------------------------------------------------------------------
    # Fetch the task
    # -------------------------------------------------------------------------

    @property
    def task(self) -> Optional[Task]:
        """
        Returns:
            the associated :class:`camcops_server.cc_modules.cc_task.Task`, or
            ``None`` if none exists.

        Raises:
            :exc:`HTTPBadRequest` if the table doesn't exist
        """
        dbsession = SqlASession.object_session(self)
        assert dbsession, (
            "TaskIndexEntry.task called on a TaskIndexEntry "
            "that's not yet in a database session")
        return task_factory_unfiltered(
            dbsession, self.task_table_name, self.task_pk)

    # -------------------------------------------------------------------------
    # Other properties mirroring those of Task, for duck typing
    # -------------------------------------------------------------------------

    @property
    def is_anonymous(self) -> bool:
        """
        Is the task anonymous?
        """
        return self.patient_pk is None

    def is_complete(self) -> bool:
        """
        Is the task complete?
        """
        return self.task_is_complete

    @property
    def _current(self) -> bool:
        """
        All task index entries represent complete tasks, so this always returns
        ``True``.
        """
        return True

    @property
    def pk(self) -> int:
        """
        Return's the task's server PK.
        """
        return self.task_pk

    @property
    def tablename(self) -> str:
        """
        Returns the base table name of the task.
        """
        return self.task_table_name

    @property
    def shortname(self) -> str:
        """
        Returns the task's shortname.
        """
        d = tablename_to_task_class_dict()
        taskclass = d[self.task_table_name]
        return taskclass.shortname

    def is_live_on_tablet(self) -> bool:
        """
        Is the task live on the source device (e.g. tablet)?
        """
        return self.era == ERA_NOW

    @property
    def when_created(self) -> Pendulum:
        """
        Returns the creation date/time as a Pendulum DateTime object.
        """
        return self.when_created_iso

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

    def get_patient_idnum_objects(self) -> List[PatientIdNum]:
        """
        Gets all :class:`PatientIdNum` objects for the patient.
        """
        return self.patient.get_idnum_objects() if self.patient else []

    # -------------------------------------------------------------------------
    # Create
    # -------------------------------------------------------------------------

    @classmethod
    def make_from_task(cls, task: Task,
                       indexed_at_utc: Pendulum) -> "TaskIndexEntry":
        """
        Returns a task index entry for the specified
        :class:`camcops_server.cc_modules.cc_task.Task`. The
        returned index requires inserting into a database session.

        Args:
            task:
                a :class:`camcops_server.cc_modules.cc_task.Task`
            indexed_at_utc:
                current time in UTC
        """
        assert indexed_at_utc is not None, "Missing indexed_at_utc"

        index = cls()

        index.indexed_at_utc = indexed_at_utc

        index.task_table_name = task.tablename
        index.task_pk = task.pk

        patient = task.patient
        index.patient_pk = patient.pk if patient else None

        index.device_id = task.device_id
        index.era = task.era
        index.when_created_utc = task.get_creation_datetime_utc()
        index.when_created_iso = task.when_created
        # noinspection PyProtectedMember
        index.when_added_batch_utc = task._when_added_batch_utc
        index.adding_user_id = task.get_adding_user_id()
        index.group_id = task.group_id
        index.task_is_complete = task.is_complete()

        return index

    @classmethod
    def index_task(cls, task: Task, session: SqlASession,
                   indexed_at_utc: Pendulum) -> None:
        """
        Indexes a task and inserts the index into the database.

        Args:
            task:
                a :class:`camcops_server.cc_modules.cc_task.Task`
            session:
                an SQLAlchemy Session
            indexed_at_utc:
                current time in UTC
        """
        index = cls.make_from_task(task,
                                   indexed_at_utc=indexed_at_utc)
        session.add(index)

    @classmethod
    def unindex_task(cls, task: Task, session: SqlASession) -> None:
        """
        Removes a task index from the database.

        Args:
            task:
                a :class:`camcops_server.cc_modules.cc_task.Task`
            session:
                an SQLAlchemy Session
        """

        # noinspection PyUnresolvedReferences
        idxtable = cls.__table__  # type: Table
        idxcols = idxtable.columns
        tasktablename = task.__class__.tablename
        session.execute(
            idxtable.delete()
            .where(idxcols.task_table_name == tasktablename)
            .where(idxcols.task_pk == task.pk)
        )

    # -------------------------------------------------------------------------
    # Regenerate index
    # -------------------------------------------------------------------------

    @classmethod
    def rebuild_index_for_task_type(cls, session: SqlASession,
                                    taskclass: Type[Task],
                                    indexed_at_utc: Pendulum,
                                    delete_first: bool = True) -> None:
        """
        Rebuilds the index for a particular task type.

        Args:
            session: an SQLAlchemy Session
            taskclass: a subclass of
                :class:`camcops_server.cc_modules.cc_task.Task`
            indexed_at_utc: current time in UTC
            delete_first: delete old index entries first? Should always be True
                unless called as part of a master rebuild that deletes
                everything first.
        """
        # noinspection PyUnresolvedReferences
        idxtable = cls.__table__  # type: Table
        idxcols = idxtable.columns
        tasktablename = taskclass.tablename
        log.info("Rebuilding task index for {}", tasktablename)
        # Delete all entries for this task
        if delete_first:
            session.execute(
                idxtable.delete()
                .where(idxcols.table_name == tasktablename)
            )
        # Create new entries
        # noinspection PyPep8,PyUnresolvedReferences,PyProtectedMember
        q = (
            session.query(taskclass)
            .filter(taskclass._current == True)  # noqa: E712
            .order_by(isotzdatetime_to_utcdatetime(taskclass.when_created))
        )
        for task in q:
            cls.index_task(task, session, indexed_at_utc)

    @classmethod
    def rebuild_entire_task_index(
            cls, session: SqlASession,
            indexed_at_utc: Pendulum,
            skip_tasks_with_missing_tables: bool = False) -> None:
        """
        Rebuilds the entire index.

        Args:
            session: an SQLAlchemy Session
            indexed_at_utc: current time in UTC
            skip_tasks_with_missing_tables: should we skip over tasks if their
                tables are not in the database? (This is so we can rebuild an
                index from a database upgrade, but not crash because newer
                tasks haven't had their tables created yet.)
        """
        log.info("Rebuilding entire task index")
        # noinspection PyUnresolvedReferences
        idxtable = cls.__table__  # type: Table

        # Delete all entries
        with if_sqlserver_disable_constraints_triggers(session,
                                                       idxtable.name):
            session.execute(
                idxtable.delete()
            )

        # Now rebuild:
        for taskclass in Task.all_subclasses_by_tablename():
            if skip_tasks_with_missing_tables:
                basetable = taskclass.tablename
                engine = get_engine_from_session(session)
                if not table_exists(engine, basetable):
                    continue
            cls.rebuild_index_for_task_type(session, taskclass,
                                            indexed_at_utc,
                                            delete_first=False)

    # -------------------------------------------------------------------------
    # Update index at the point of upload from a device
    # -------------------------------------------------------------------------

    @classmethod
    def update_task_index_for_upload(cls,
                                     session: SqlASession,
                                     tablechanges: UploadTableChanges,
                                     indexed_at_utc: Pendulum) -> None:
        """
        Updates the index for a device's upload.

        - Deletes index entries for records that are on the way out.
        - Creates index entries for records that are on the way in.
        - Deletes/recreates index entries for records being preserved.

        Args:
            session:
                an SQLAlchemy Session
            tablechanges:
                a :class:`camcops_server.cc_modules.cc_client_api_core.UploadTableChanges`
                object describing the changes to a table
            indexed_at_utc:
                current time in UTC
        """  # noqa
        tasktablename = tablechanges.tablename
        d = tablename_to_task_class_dict()
        try:
            taskclass = d[tasktablename]  # may raise KeyError
        except KeyError:
            fail_user_error(f"Bug: no such task table: {tasktablename!r}")

        # noinspection PyUnresolvedReferences
        idxtable = cls.__table__  # type: Table
        idxcols = idxtable.columns

        # Delete the old.
        delete_index_pks = tablechanges.task_delete_index_pks
        if delete_index_pks:
            log.debug("Deleting old task indexes: {}, server PKs {}",
                      tasktablename, delete_index_pks)
            # noinspection PyProtectedMember
            session.execute(
                idxtable.delete()
                .where(idxcols.task_table_name == tasktablename)
                .where(idxcols.task_pk.in_(delete_index_pks))
            )

        # Create the new.
        reindex_pks = tablechanges.task_reindex_pks
        if reindex_pks:
            log.debug("Recreating task indexes: {}, server PKs {}",
                      tasktablename, reindex_pks)
            # noinspection PyUnboundLocalVariable,PyProtectedMember
            q = (
                session.query(taskclass)
                .filter(taskclass._pk.in_(reindex_pks))
            )
            for task in q:
                cls.index_task(task, session,
                               indexed_at_utc=indexed_at_utc)

    # -------------------------------------------------------------------------
    # Check index
    # -------------------------------------------------------------------------
    @classmethod
    def check_index(cls, session: SqlASession,
                    show_all_bad: bool = False) -> bool:
        """
        Checks the index.

        Args:
            session:
                an SQLAlchemy Session
            show_all_bad:
                show all bad entries? (If false, return upon the first)

        Returns:
            bool: is the index OK?
        """
        ok = True

        log.info("Checking all task indexes represent valid entries")
        for taskclass in Task.all_subclasses_by_tablename():
            tasktablename = taskclass.tablename
            log.debug("Checking {}", tasktablename)
            # noinspection PyUnresolvedReferences,PyProtectedMember
            q_idx_without_original = session.query(TaskIndexEntry).filter(
                TaskIndexEntry.task_table_name == tasktablename,
                ~exists()
                .select_from(taskclass.__table__)
                .where(and_(
                    TaskIndexEntry.task_pk == taskclass._pk,
                    taskclass._current == True,  # noqa: E712
                ))
            )
            # No check for a valid patient at this time.
            for index in q_idx_without_original:
                log.error("Task index without matching original: {!r}", index)
                ok = False
                if not show_all_bad:
                    return ok

        log.info("Checking all tasks have an index")
        for taskclass in Task.all_subclasses_by_tablename():
            tasktablename = taskclass.tablename
            log.debug("Checking {}", tasktablename)
            # noinspection PyUnresolvedReferences,PyProtectedMember
            q_original_with_idx = session.query(taskclass).filter(
                taskclass._current == True,  # noqa: E712
                ~exists().select_from(
                    TaskIndexEntry.__table__
                ).where(and_(
                    TaskIndexEntry.task_pk == taskclass._pk,
                    TaskIndexEntry.task_table_name == tasktablename,
                ))
            )
            for orig in q_original_with_idx:
                log.error("Task without index entry: {!r}", orig)
                ok = False
                if not show_all_bad:
                    return ok

        return ok


# =============================================================================
# Wide-ranging index update functions
# =============================================================================

def reindex_everything(session: SqlASession,
                       skip_tasks_with_missing_tables: bool = False) -> None:
    """
    Deletes from and rebuilds all server index tables.

    Args:
        session: an SQLAlchemy Session
        skip_tasks_with_missing_tables: should we skip over tasks if their
            tables are not in the database? (This is so we can rebuild an index
            from a database upgrade, but not crash because newer tasks haven't
            had their tables created yet.)
    """
    now = Pendulum.utcnow()
    log.info("Reindexing database; indexed_at_utc = {}", now)
    PatientIdNumIndexEntry.rebuild_idnum_index(session, now)
    TaskIndexEntry.rebuild_entire_task_index(
        session, now,
        skip_tasks_with_missing_tables=skip_tasks_with_missing_tables)


def update_indexes_and_push_exports(req: "CamcopsRequest",
                                    batchdetails: BatchDetails,
                                    tablechanges: UploadTableChanges) -> None:
    """
    Update server indexes, if required.

    Also triggers background jobs to export "new arrivals", if required.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        tablechanges:
            a :class:`camcops_server.cc_modules.cc_client_api_core.UploadTableChanges`
            object describing the changes to a table
    """  # noqa
    tablename = tablechanges.tablename
    if tablename == PatientIdNum.__tablename__:
        # Update idnum index
        PatientIdNumIndexEntry.update_idnum_index_for_upload(
            session=req.dbsession,
            indexed_at_utc=batchdetails.batchtime,
            tablechanges=tablechanges,
        )
    elif tablename in all_task_tablenames():
        # Update task index
        TaskIndexEntry.update_task_index_for_upload(
            session=req.dbsession,
            tablechanges=tablechanges,
            indexed_at_utc=batchdetails.batchtime
        )
        # Push exports
        recipients = req.all_push_recipients
        uploading_group_id = req.user.upload_group_id
        for recipient in recipients:
            recipient_name = recipient.recipient_name
            for pk in tablechanges.get_task_push_export_pks(
                    recipient=recipient,
                    uploading_group_id=uploading_group_id):
                req.add_export_push_request(recipient_name, tablename, pk)
                # ... will be transmitted *after* the request performs COMMIT


def check_indexes(session: SqlASession, show_all_bad: bool = False) -> bool:
    """
    Checks all server index tables.

    Args:
        session:
            an SQLAlchemy Session
        show_all_bad:
            show all bad entries? (If false, return upon the first)

    Returns:
        bool: are the indexes OK?
    """
    p_ok = PatientIdNumIndexEntry.check_index(session, show_all_bad)
    if p_ok:
        log.info("Patient ID number index is good")
    else:
        log.error("Patient ID number index is bad")
        if not show_all_bad:
            return False
    t_ok = TaskIndexEntry.check_index(session, show_all_bad)
    if t_ok:
        log.info("Task index is good")
    else:
        log.error("Task index is bad")
    return p_ok and t_ok
