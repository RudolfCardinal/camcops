#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_taskfactory.py

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

**Functions to fetch tasks from the database.**

"""

import logging
from typing import Optional, Type, TYPE_CHECKING, Union

from cardinal_pythonlib.logs import BraceStyleAdapter
import pyramid.httpexceptions as exc
from sqlalchemy.orm import Query, Session as SqlASession

from camcops_server.cc_modules.cc_task import (
    tablename_to_task_class_dict,
    Task,
)
from camcops_server.cc_modules.cc_taskindex import TaskIndexEntry

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Task query helpers
# =============================================================================

def task_query_restricted_to_permitted_users(
        req: "CamcopsRequest",
        q: Query,
        cls: Union[Type[Task], Type[TaskIndexEntry]],
        as_dump: bool) -> Optional[Query]:
    """
    Restricts an SQLAlchemy ORM query to permitted users, for a given
    task class. THIS IS A KEY SECURITY FUNCTION.

    Args:
        req:
            the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        q:
            the SQLAlchemy ORM query
        cls:
            the class of the task type, or the
            :class:`camcops_server.cc_modules.cc_taskindex.TaskIndexEntry`
            class
        as_dump:
            use the "dump" permissions rather than the "view" permissions?

    Returns:
        a filtered query (or the original query, if no filtering was required)

    """
    user = req.user

    if user.superuser:
        return q  # anything goes

    # Implement group security. Simple:
    if as_dump:
        group_ids = user.ids_of_groups_user_may_dump
    else:
        group_ids = user.ids_of_groups_user_may_see

    if not group_ids:
        return None

    if cls is TaskIndexEntry:
        # noinspection PyUnresolvedReferences
        q = q.filter(cls.group_id.in_(group_ids))
    else:  # a kind of Task
        q = q.filter(cls._group_id.in_(group_ids))

    return q


# =============================================================================
# Make a single task given its base table name and server PK
# =============================================================================

def task_factory(req: "CamcopsRequest", basetable: str,
                 serverpk: int) -> Optional[Task]:
    """
    Load a task from the database and return it.
    Filters to tasks permitted to the current user.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
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
    dbsession = req.dbsession
    # noinspection PyProtectedMember
    q = dbsession.query(cls).filter(cls._pk == serverpk)
    q = task_query_restricted_to_permitted_users(req, q, cls, as_dump=False)
    return q.first()


def task_factory_no_security_checks(dbsession: SqlASession, basetable: str,
                                    serverpk: int) -> Optional[Task]:
    """
    Load a task from the database and return it.
    Filters to tasks permitted to the current user.

    Args:
        dbsession: a :class:`sqlalchemy.orm.session.Session`
        basetable: name of the task's base table
        serverpk: server PK of the task

    Returns:
        the task, or ``None`` if the PK doesn't exist

    Raises:
        :exc:`KeyError` if the table doesn't exist
    """
    d = tablename_to_task_class_dict()
    cls = d[basetable]  # may raise KeyError
    # noinspection PyProtectedMember
    q = dbsession.query(cls).filter(cls._pk == serverpk)
    return q.first()


# =============================================================================
# Make a single task given its base table name and server PK
# =============================================================================

def task_factory_clientkeys_no_security_checks(dbsession: SqlASession,
                                               basetable: str,
                                               client_id: int,
                                               device_id: int,
                                               era: str) -> Optional[Task]:
    """
    Load a task from the database and return it.
    Filters to tasks permitted to the current user.

    Args:
        dbsession: a :class:`sqlalchemy.orm.session.Session`
        basetable: name of the task's base table
        client_id: task's ``_id`` value
        device_id: task's ``_device_id`` value
        era: task's ``_era`` value

    Returns:
        the task, or ``None`` if it doesn't exist

    Raises:
        :exc:`KeyError` if the table doesn't exist
    """
    d = tablename_to_task_class_dict()
    cls = d[basetable]  # may raise KeyError
    # noinspection PyProtectedMember
    q = (
        dbsession.query(cls)
        .filter(cls.id == client_id)
        .filter(cls._device_id == device_id)
        .filter(cls._era == era)
    )
    return q.first()
