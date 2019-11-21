#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_dummy_database.py

===============================================================================

    Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).

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

**Functions for dummy database creation for manual testing.**

"""
import logging
import random
from typing import TYPE_CHECKING

from cardinal_pythonlib.datetimefunc import (
    convert_datetime_to_utc,
    format_datetime,
)
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.nhs import generate_random_nhs_number
import pendulum
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm.session import sessionmaker
from sqlalchemy.sql.expression import func
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Float, Integer

from camcops_server.cc_modules.cc_constants import DateFormat
from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_group import Group
from camcops_server.cc_modules.cc_idnumdef import IdNumDefinition
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_task import Task
from camcops_server.cc_modules.cc_user import User


if TYPE_CHECKING:
    from sqlalchemy.orm import Session as SqlASession
    from camcops_server.cc_modules.cc_config import CamcopsConfig
    from camcops_server.cc_modules.cc_db import GenericTabletRecordMixin

log = BraceStyleAdapter(logging.getLogger(__name__))


def add_dummy_data(cfg: "CamcopsConfig",
                   confirm_add_dummy_data: bool = False) -> None:
    if not confirm_add_dummy_data:
        log.critical("Destructive action not confirmed! Refusing.")
        return

    factory = DummyDataFactory(cfg)
    factory.add_data()


class DummyDataFactory(object):
    FIRST_PATIENT_ID = 10001
    NUM_PATIENTS = 5

    DEFAULT_MIN_FLOAT = 0
    DEFAULT_MAX_FLOAT = 1000

    DEFAULT_MIN_INTEGER = 0
    DEFAULT_MAX_INTEGER = 1000

    def __init__(self, cfg: "CamcopsConfig"):
        engine = cfg.get_sqla_engine()
        self.dbsession = sessionmaker()(bind=engine)  # type: SqlASession

    def add_data(self):
        next_id = self.next_id(Group.id)

        self.group = Group()
        self.group.name = f"dummygroup {next_id}"
        self.group.description = "Dummy group"
        self.group.upload_policy = "sex AND anyidnum"
        self.group.finalize_policy = "sex AND idnum1"
        self.dbsession.add(self.group)
        self.dbsession.commit()  # sets PK fields

        self.user = User.get_system_user(self.dbsession)
        self.user.upload_group_id = self.group.id

        self.era_time = pendulum.now()
        self.era_time_utc = convert_datetime_to_utc(self.era_time)
        self.era = format_datetime(self.era_time, DateFormat.ISO8601)
        self.server_device = Device.get_server_device(self.dbsession)

        self.nhs_iddef = IdNumDefinition(which_idnum="1001",
                                         description="NHS number (TEST)",
                                         short_description="NHS#",
                                         hl7_assigning_authority="NHS",
                                         hl7_id_type="NHSN")
        self.dbsession.add(self.nhs_iddef)
        try:
            self.dbsession.commit()
        except IntegrityError:
            self.dbsession.rollback()

        for patient_id in range(self.FIRST_PATIENT_ID,
                                self.FIRST_PATIENT_ID + self.NUM_PATIENTS):
            self.add_patient(patient_id)
            self.add_tasks(patient_id)

    def add_patient(self, patient_id: int) -> Patient:
        log.info(f"Adding patient {patient_id}")

        patient = Patient()

        patient.id = patient_id
        self.apply_standard_db_fields(patient)
        patient.forename = f"Forename {patient_id}"
        patient.surname = f"Surname {patient_id}"
        patient.dob = pendulum.parse("1950-01-01")
        self.dbsession.add(patient)

        self.add_patient_idnum(patient_id)
        try:
            self.dbsession.commit()
        except IntegrityError:
            self.dbsession.rollback()

        return patient

    def add_patient_idnum(self, patient_id: int) -> PatientIdNum:
        next_id = self.next_id(PatientIdNum.id)

        patient_idnum = PatientIdNum()
        patient_idnum.id = next_id
        self.apply_standard_db_fields(patient_idnum)
        patient_idnum.patient_id = patient_id
        patient_idnum.which_idnum = self.nhs_iddef.which_idnum
        patient_idnum.idnum_value = generate_random_nhs_number()

        self.dbsession.add(patient_idnum)

    def add_tasks(self, patient_id) -> Task:
        for cls in Task.all_subclasses_by_tablename():
            task = cls()
            task.id = self.next_id(cls.id)
            self.apply_standard_task_fields(task)
            if task.has_patient:
                task.patient_id = patient_id

            self.fill_in_task_fields(task)

            self.dbsession.add(task)
            self.dbsession.commit()

    def fill_in_task_fields(self, task: "Task"):
        for column in task.__table__.columns:
            if not self.column_is_q_field(column):
                continue

            if isinstance(column.type, Integer):
                self.set_integer_field(task, column)
                continue

            if isinstance(column.type, Float):
                self.set_float_field(task, column)

    def set_integer_field(self, task: "Task", column: Column):
        setattr(task, column.name, self.get_valid_integer_for_field(column))

    def set_float_field(self, task: "Task", column: Column):
        setattr(task, column.name, self.get_valid_float_for_field(column))

    def get_valid_integer_for_field(self, column: Column):
        min_value = self.DEFAULT_MIN_INTEGER
        max_value = self.DEFAULT_MAX_INTEGER

        value_checker = getattr(column, "permitted_value_checker", None)

        if value_checker is not None:
            if value_checker.permitted_values is not None:
                return random.choice(value_checker.permitted_values)

            if value_checker.minimum is not None:
                min_value = value_checker.minimum

            if value_checker.maximum is not None:
                max_value = value_checker.maximum

        return random.randint(min_value, max_value)

    def get_valid_float_for_field(self, column: Column):
        min_value = self.DEFAULT_MIN_FLOAT
        max_value = self.DEFAULT_MAX_FLOAT

        value_checker = getattr(column, "permitted_value_checker", None)

        if value_checker is not None:
            if value_checker.permitted_values is not None:
                return random.choice(value_checker.permitted_values)

            if value_checker.minimum is not None:
                min_value = value_checker.minimum

            if value_checker.maximum is not None:
                max_value = value_checker.maximum

        return random.uniform(min_value, max_value)

    def column_is_q_field(self, column: Column):
        if column.name.startswith("_"):
            return False

        if column.name in [
            'editing_time_s',
            'firstexit_is_abort',
            'firstexit_is_finish',
            'id',
            'patient_id',
            'when_created',
            'when_firstexit',
            'when_last_modified',
        ]:
            return False

        return True

    def next_id(self, column: Column):
        max_id = self.dbsession.query(func.max(column)).scalar()
        if max_id is None:
            return 1

        return max_id + 1

    def apply_standard_task_fields(self, task: "Task") -> None:
        """
        Writes some default values to an SQLAlchemy ORM object representing
        a task.
        """
        self.apply_standard_db_fields(task)
        task.when_created = self.era_time

    def apply_standard_db_fields(self,
                                 obj: "GenericTabletRecordMixin") -> None:
        """
        Writes some default values to an SQLAlchemy ORM object representing a
        record uploaded from a client (tablet) device.
        """
        obj._device_id = self.server_device.id
        obj._era = self.era
        obj._group_id = self.group.id
        obj._current = True
        obj._adding_user_id = self.user.id
        obj._when_added_batch_utc = self.era_time_utc