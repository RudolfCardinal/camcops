#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_taskschedule.py

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

"""

import logging
from typing import List, Iterable, Optional, Tuple, TYPE_CHECKING
from urllib.parse import quote, urlencode

from pendulum import DateTime as Pendulum, Duration

from sqlalchemy import cast, Numeric
from sqlalchemy.orm import relationship
from sqlalchemy.sql.functions import func
from sqlalchemy.sql.schema import Column, ForeignKey
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_formatter import SafeFormatter
from camcops_server.cc_modules.cc_group import Group
from camcops_server.cc_modules.cc_pyramid import Routes
from camcops_server.cc_modules.cc_simpleobjects import IdNumReference
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_sqla_coltypes import (
    JsonColType,
    PendulumDateTimeAsIsoTextColType,
    PendulumDurationAsIsoTextColType,
    TableNameColType,
)
from camcops_server.cc_modules.cc_task import Task, tablename_to_task_class_dict
from camcops_server.cc_modules.cc_taskcollection import (
    TaskFilter,
    TaskCollection,
    TaskSortMethod,
)


if TYPE_CHECKING:
    from sqlalchemy.sql.elements import Cast
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = logging.getLogger(__name__)


# =============================================================================
# ScheduledTaskInfo
# =============================================================================

class ScheduledTaskInfo(object):
    """
    Simple representation of a scheduled task (which may also contain the
    actual completed task, in its ``task`` member, if there is one).
    """
    def __init__(self,
                 shortname: str,
                 tablename: str,
                 is_anonymous: bool,
                 task: Optional[Task] = None,
                 start_datetime: Optional[Pendulum] = None,
                 end_datetime: Optional[Pendulum] = None) -> None:
        self.shortname = shortname
        self.tablename = tablename
        self.is_anonymous = is_anonymous
        self.task = task
        self.start_datetime = start_datetime
        self.end_datetime = end_datetime


# =============================================================================
# PatientTaskSchedule
# =============================================================================

class PatientTaskSchedule(Base):
    """
    Joining table that associates a patient with a task schedule
    """
    __tablename__ = "_patient_task_schedule"

    id = Column("id", Integer, primary_key=True, autoincrement=True)
    patient_pk = Column(
        "patient_pk", Integer,
        ForeignKey("patient._pk", ondelete="CASCADE")
    )
    schedule_id = Column(
        "schedule_id", Integer,
        ForeignKey("_task_schedule.id", ondelete="CASCADE")
    )
    start_datetime = Column(
        "start_datetime", PendulumDateTimeAsIsoTextColType,
        comment=(
            "Schedule start date for the patient. Due from/within "
            "durations for a task schedule item are relative to this."
        )
    )
    settings = Column(
        "settings", JsonColType,
        comment="Task-specific settings for this patient"
    )

    patient = relationship("Patient", back_populates="task_schedules")
    task_schedule = relationship("TaskSchedule", back_populates="patients")

    def get_list_of_scheduled_tasks(self, req: "CamcopsRequest") \
            -> List[ScheduledTaskInfo]:

        task_list = []

        task_class_lookup = tablename_to_task_class_dict()

        for tsi in self.task_schedule.items:
            start_datetime = None
            end_datetime = None
            task = None

            if self.start_datetime is not None:
                start_datetime = self.start_datetime.add(
                    days=tsi.due_from.days
                )
                end_datetime = self.start_datetime.add(
                    days=tsi.due_by.days
                )

                task = self.find_scheduled_task(
                    req, tsi, start_datetime, end_datetime
                )

            task_class = task_class_lookup[tsi.task_table_name]

            task_list.append(
                ScheduledTaskInfo(
                    task_class.shortname,
                    tsi.task_table_name,
                    is_anonymous=task_class.is_anonymous,
                    task=task,
                    start_datetime=start_datetime,
                    end_datetime=end_datetime,
                )
            )

        return task_list

    def find_scheduled_task(self,
                            req: "CamcopsRequest",
                            tsi: "TaskScheduleItem",
                            start_datetime: Pendulum,
                            end_datetime: Pendulum) -> Optional[Task]:
        """
        Returns the most recently uploaded task that matches the patient (by
        any ID number, i.e. via OR), task type and timeframe
        """
        taskfilter = TaskFilter()
        for idnum in self.patient.idnums:
            idnum_ref = IdNumReference(which_idnum=idnum.which_idnum,
                                       idnum_value=idnum.idnum_value)
            taskfilter.idnum_criteria.append(idnum_ref)

        taskfilter.task_types = [tsi.task_table_name]

        taskfilter.start_datetime = start_datetime
        taskfilter.end_datetime = end_datetime

        # TODO: Improve error reporting
        # Shouldn't happen in normal operation as the task schedule item form
        # validation will ensure the dates are correct.
        # However, it's quite easy to write tests with unintentionally
        # inconsistent dates.
        # If we don't assert this here, we get a more cryptic assertion
        # failure later:
        #
        # cc_taskcollection.py _fetch_tasks_from_indexes()
        # assert self._all_indexes is not None
        assert not taskfilter.dates_inconsistent()

        collection = TaskCollection(
            req=req,
            taskfilter=taskfilter,
            sort_method_global=TaskSortMethod.CREATION_DATE_DESC
        )

        if len(collection.all_tasks) > 0:
            return collection.all_tasks[0]

        return None

    def mailto_url(self, req: "CamcopsRequest") -> str:
        template_dict = dict(
            access_key=self.patient.uuid_as_proquint,
            server_url=req.route_url(Routes.CLIENT_API)
        )

        formatter = TaskScheduleEmailTemplateFormatter()
        email_body = formatter.format(self.task_schedule.email_template,
                                      **template_dict)

        mailto_params = urlencode({
            "subject": self.task_schedule.email_subject,
            "body": email_body,
        }, quote_via=quote)

        mailto_url = f"mailto:{self.patient.email}?{mailto_params}"

        return mailto_url


def task_schedule_item_sort_order() -> Tuple["Cast", "Cast"]:
    """
    Returns a tuple of sorting functions for use with SQLAlchemy ORM queries,
    to sort task schedule items.

    The durations are currently stored as seconds e.g. P0Y0MT2594592000.0S
    and the seconds aren't zero padded, so we need to do some processing
    to get them in the order we want.

    This will fail if durations ever get stored any other way.
    """
    due_from_order = cast(func.substr(TaskScheduleItem.due_from, 7),
                          Numeric())
    due_by_order = cast(func.substr(TaskScheduleItem.due_by, 7),
                        Numeric())

    return due_from_order, due_by_order


# =============================================================================
# Task schedule
# =============================================================================

class TaskSchedule(Base):
    """
    A named collection of task schedule items
    """
    __tablename__ = "_task_schedule"

    id = Column(
        "id", Integer,
        primary_key=True, autoincrement=True,
        comment="Arbitrary primary key"
    )

    group_id = Column(
        "group_id", Integer, ForeignKey(Group.id),
        nullable=False,
        comment="FK to {}.{}".format(Group.__tablename__,
                                     Group.id.name)
    )

    name = Column("name", UnicodeText, comment="name")

    email_subject = Column("email_subject", UnicodeText,
                           comment="email subject", nullable=False, default="")
    email_template = Column("email_template", UnicodeText,
                            comment="email template", nullable=False,
                            default="")

    items = relationship(
        "TaskScheduleItem",
        order_by=task_schedule_item_sort_order
    )  # type: Iterable[TaskScheduleItem]

    group = relationship(Group)

    patients = relationship("PatientTaskSchedule",
                            back_populates="task_schedule")

    def user_may_edit(self, req: "CamcopsRequest") -> bool:
        return req.user.may_administer_group(self.group_id)


class TaskScheduleItem(Base):
    """
    An individual item in a task schedule
    """
    __tablename__ = "_task_schedule_item"

    id = Column(
        "id", Integer,
        primary_key=True, autoincrement=True,
        comment="Arbitrary primary key"
    )

    schedule_id = Column(
        "schedule_id", Integer, ForeignKey(TaskSchedule.id),
        nullable=False,
        comment="FK to {}.{}".format(TaskSchedule.__tablename__,
                                     TaskSchedule.id.name)
    )

    task_table_name = Column(
        "task_table_name", TableNameColType,
        index=True,
        comment="Table name of the task's base table"
    )

    due_from = Column(
        "due_from", PendulumDurationAsIsoTextColType,
        comment=("Relative time from the start date by which the task may be "
                 "started")
    )  # type: Optional[Duration]

    due_by = Column(
        "due_by", PendulumDurationAsIsoTextColType,
        comment=("Relative time from the start date by which the task must be "
                 "completed")
    )  # type: Optional[Duration]

    @property
    def task_shortname(self) -> str:
        task_class_lookup = tablename_to_task_class_dict()

        return task_class_lookup[self.task_table_name].shortname

    @property
    def due_within(self) -> Optional[Duration]:
        if self.due_by is None:
            # Should not be possible if created through the form
            return None

        if self.due_from is None:
            return self.due_by

        return self.due_by - self.due_from

    def description(self, req: "CamcopsRequest") -> str:
        _ = req.gettext

        if self.due_from is None:
            # Should not be possible if created through the form
            due_days = "?"
        else:
            due_days = self.due_from.in_days()

        return _("{task_name} @ {due_days} days").format(
            task_name=self.task_shortname,
            due_days=due_days
        )


class TaskScheduleEmailTemplateFormatter(SafeFormatter):
    def __init__(self):
        super().__init__(["access_key", "server_url"])
