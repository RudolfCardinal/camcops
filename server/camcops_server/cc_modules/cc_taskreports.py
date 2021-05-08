#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_taskreports.py

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

**Server reports on CamCOPS tasks.**

"""

from collections import Counter
from typing import Any, List, Sequence, Type, TYPE_CHECKING

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.sqlalchemy.orm_query import get_rows_fieldnames_from_query  # noqa
from cardinal_pythonlib.sqlalchemy.sqlfunc import extract_month, extract_year
from sqlalchemy.sql.expression import and_, desc, func, literal, select

from camcops_server.cc_modules.cc_sqla_coltypes import (
    isotzdatetime_to_utcdatetime,
)
from camcops_server.cc_modules.cc_forms import (
    ReportParamSchema,
    ViaIndexSelector,
)
from camcops_server.cc_modules.cc_pyramid import ViewParam
from camcops_server.cc_modules.cc_report import Report, PlainReportType
from camcops_server.cc_modules.cc_task import Task
from camcops_server.cc_modules.cc_taskindex import TaskIndexEntry
from camcops_server.cc_modules.cc_user import User

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest


# =============================================================================
# Reports
# =============================================================================

class TaskCountReportSchema(ReportParamSchema):
    via_index = ViaIndexSelector()  # must match ViewParam.VIA_INDEX


class TaskCountReport(Report):
    """
    Report to count task instances.
    """

    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "taskcount"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "(Server) Count current task instances, by creation month and "
            "task type"
        )

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    @staticmethod
    def get_paramform_schema_class() -> Type["ReportParamSchema"]:
        return TaskCountReportSchema

    @classmethod
    def get_specific_http_query_keys(cls) -> List[str]:
        return [
            ViewParam.VIA_INDEX
        ]

    def get_rows_colnames(self, req: "CamcopsRequest") -> PlainReportType:
        final_rows = []  # type: List[Sequence[Sequence[Any]]]
        colnames = []  # type: List[str]
        dbsession = req.dbsession
        group_ids = req.user.ids_of_groups_user_may_report_on
        superuser = req.user.superuser
        via_index = req.get_bool_param(ViewParam.VIA_INDEX, True)
        if via_index:
            # noinspection PyUnresolvedReferences
            query = (
                select([
                    extract_year(TaskIndexEntry.when_created_utc).label("year"),  # noqa
                    extract_month(TaskIndexEntry.when_created_utc).label("month"),  # noqa
                    TaskIndexEntry.task_table_name.label("task"),
                    func.count().label("num_tasks_added"),
                ])
                .select_from(TaskIndexEntry.__table__)
                .group_by("year", "month", "task")
                .order_by(desc("year"), desc("month"), "task")
                # ... http://docs.sqlalchemy.org/en/latest/core/tutorial.html#ordering-or-grouping-by-a-label  # noqa
            )
            if not superuser:
                # Restrict to accessible groups
                # noinspection PyProtectedMember
                query = query.where(TaskIndexEntry.group_id.in_(group_ids))
            rows, colnames = get_rows_fieldnames_from_query(dbsession, query)
            # noinspection PyTypeChecker
            final_rows = rows
        else:
            classes = Task.all_subclasses_by_tablename()
            for cls in classes:
                # noinspection PyUnresolvedReferences
                query = (
                    select([
                        # func.year() is specific to some DBs, e.g. MySQL
                        # so is func.extract();
                        # http://modern-sql.com/feature/extract
                        extract_year(isotzdatetime_to_utcdatetime(
                            cls.when_created)).label("year"),
                        extract_month(isotzdatetime_to_utcdatetime(
                            cls.when_created)).label("month"),
                        literal(cls.__tablename__).label("task"),
                        func.count().label("num_tasks_added"),
                    ])
                    .select_from(cls.__table__)
                    .where(cls._current == True)  # noqa: E712
                    .group_by("year", "month")
                )
                if not superuser:
                    # Restrict to accessible groups
                    # noinspection PyProtectedMember
                    query = query.where(cls._group_id.in_(group_ids))
                rows, colnames = get_rows_fieldnames_from_query(
                    dbsession, query)
                final_rows.extend(rows)
            final_rows.sort(key=lambda r: (-r[0], -r[1], r[2]))
            # ... sort by: year (descending), month (descending), task
        return PlainReportType(rows=final_rows, column_names=colnames)


class TaskCountByUserReport(Report):
    """
    Report to count task instances.
    """

    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "taskcount_by_user"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "(Server) Count current task instances, by creation month and user"
        )

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    @staticmethod
    def get_paramform_schema_class() -> Type["ReportParamSchema"]:
        return TaskCountReportSchema

    @classmethod
    def get_specific_http_query_keys(cls) -> List[str]:
        return [
            ViewParam.VIA_INDEX
        ]

    def get_rows_colnames(self, req: "CamcopsRequest") -> PlainReportType:
        final_rows = []  # type: List[Sequence[Sequence[Any]]]
        colnames = []  # type: List[str]
        dbsession = req.dbsession
        group_ids = req.user.ids_of_groups_user_may_report_on
        superuser = req.user.superuser
        via_index = req.get_bool_param(ViewParam.VIA_INDEX, True)
        if via_index:
            # noinspection PyUnresolvedReferences
            query = (
                select([
                    extract_year(TaskIndexEntry.when_created_utc).label("year"),  # noqa
                    extract_month(TaskIndexEntry.when_created_utc).label("month"),  # noqa
                    User.username.label("adding_user_name"),
                    func.count().label("num_tasks_added"),
                ])
                .select_from(TaskIndexEntry.__table__)
                .select_from(User.__table__)
                .where(TaskIndexEntry.adding_user_id == User.id)
                .group_by("year", "month", "adding_user_name")
                .order_by(desc("year"), desc("month"), "adding_user_name")
            )
            if not superuser:
                # Restrict to accessible groups
                # noinspection PyProtectedMember
                query = query.where(TaskIndexEntry.group_id.in_(group_ids))
            rows, colnames = get_rows_fieldnames_from_query(dbsession, query)
            # noinspection PyTypeChecker
            final_rows = rows
        else:
            classes = Task.all_subclasses_by_tablename()
            counter = Counter()
            for cls in classes:
                # noinspection PyUnresolvedReferences
                query = (
                    select([
                        # func.year() is specific to some DBs, e.g. MySQL
                        # so is func.extract();
                        # http://modern-sql.com/feature/extract
                        extract_year(isotzdatetime_to_utcdatetime(
                            cls.when_created)).label("year"),
                        extract_month(isotzdatetime_to_utcdatetime(
                            cls.when_created)).label("month"),
                        User.username.label("adding_user_name"),
                        func.count().label("num_tasks_added"),
                    ])
                    .select_from(cls.__table__)
                    .select_from(User.__table__)
                    .where(and_(cls._current == True,  # noqa: E712
                                cls._adding_user_id == User.id))
                    .group_by("year", "month", "adding_user_name")
                )
                if not superuser:
                    # Restrict to accessible groups
                    # noinspection PyProtectedMember
                    query = query.where(cls._group_id.in_(group_ids))
                rows, colnames = get_rows_fieldnames_from_query(
                    dbsession, query)
                for row in rows:
                    year, month, username = row[0], row[1], row[2]
                    count = row[3]
                    counter.update({(year, month, username): count})
            for (year, month, username), total in counter.items():
                final_rows.append([year, month, username, total])
            final_rows.sort(key=lambda r: (-r[0], -r[1], r[2]))
            # ... sort by: year (descending), month (descending), username
        return PlainReportType(rows=final_rows, column_names=colnames)
