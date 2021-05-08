#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_all_models.py

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

**The point of this is to import everything that's an SQLAlchemy model, so
they're registered (and also Task knows about all its subclasses).**

"""

import logging
# from pprint import pformat
from typing import Dict, List, Type

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.sqlalchemy.orm_inspect import gen_orm_classes_from_base
from sqlalchemy.orm import configure_mappers
from sqlalchemy.sql.schema import Table

from camcops_server.cc_modules.cc_baseconstants import ALEMBIC_VERSION_TABLE
from camcops_server.cc_modules.cc_db import GenericTabletRecordMixin
from camcops_server.cc_modules.cc_sqlalchemy import Base

# =============================================================================
# Non-task model imports representing client-side tables
# =============================================================================
# How to suppress "Unused import statement"?
# https://stackoverflow.com/questions/21139329/false-unused-import-statement-in-pycharm  # noqa
# http://codeoptimism.com/blog/pycharm-suppress-inspections-list/

# noinspection PyUnresolvedReferences
from camcops_server.cc_modules.cc_blob import Blob  # noqa: F401
# noinspection PyUnresolvedReferences
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum  # noqa: F401
# noinspection PyUnresolvedReferences
from camcops_server.cc_modules.cc_patient import Patient  # noqa: F401

# =============================================================================
# Other non-task model imports
# =============================================================================

from camcops_server.cc_modules.cc_audit import AuditEntry
from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_dirtytables import DirtyTable
from camcops_server.cc_modules.cc_email import Email
from camcops_server.cc_modules.cc_group import Group, group_group_table
from camcops_server.cc_modules.cc_exportmodels import (
    ExportedTaskEmail,
    ExportedTask,
    ExportedTaskFileGroup,
    ExportedTaskHL7Message,
)
from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient
from camcops_server.cc_modules.cc_idnumdef import IdNumDefinition
from camcops_server.cc_modules.cc_membership import UserGroupMembership
from camcops_server.cc_modules.cc_session import CamcopsSession
from camcops_server.cc_modules.cc_specialnote import SpecialNote
from camcops_server.cc_modules.cc_serversettings import ServerSettings
# noinspection PyUnresolvedReferences
from camcops_server.cc_modules.cc_task import Task
from camcops_server.cc_modules.cc_taskfilter import TaskFilter
from camcops_server.cc_modules.cc_taskschedule import (
    TaskSchedule,
    TaskScheduleItem,
)
# noinspection PyUnresolvedReferences
from camcops_server.cc_modules.cc_taskindex import (
    PatientIdNumIndexEntry,
    TaskIndexEntry,
)
from camcops_server.cc_modules.cc_user import (
    SecurityAccountLockout,
    SecurityLoginFailure,
    User,
)

# =============================================================================
# Task imports
# =============================================================================

# import_submodules("..tasks", __package__)
#
# ... NO LONGER SUFFICIENT as we are using SQLAlchemy relationship clauses that
# are EVALUATED and so require the class names to be in the relevant namespace
# at the time. So doing something equivalent to "import tasks.phq9" -- which is
# what we get from 'import_submodules("..tasks", __package__)' -- isn't enough.
# We need something equivalent to "from tasks.phq9 import Phq9".

# noinspection PyUnresolvedReferences
from camcops_server.tasks import *  # see tasks/__init__.py  # noqa: F401,F403

# =============================================================================
# Other report imports
# =============================================================================

# noinspection PyUnresolvedReferences
from camcops_server.cc_modules.cc_taskreports import TaskCountReport  # noqa: E501,F401

# =============================================================================
# Logging
# =============================================================================

log = BraceStyleAdapter(logging.getLogger(__name__))

# log.critical("Loading cc_all_models")

# =============================================================================
# Ensure that anything with an AbstractConcreteBase gets its mappers
# registered (i.e. Task).
# =============================================================================

configure_mappers()

# =============================================================================
# Tables (and fields) that clients can't touch
# =============================================================================

RESERVED_TABLE_NAMES = [
    ALEMBIC_VERSION_TABLE,
    AuditEntry.__tablename__,
    CamcopsSession.__tablename__,
    Device.__tablename__,
    DirtyTable.__tablename__,
    Email.__tablename__,
    ExportedTask.__tablename__,
    ExportedTaskEmail.__tablename__,
    ExportedTaskFileGroup.__tablename__,
    ExportedTaskHL7Message.__tablename__,
    ExportRecipient.__tablename__,
    Group.__tablename__,
    group_group_table.name,
    IdNumDefinition.__tablename__,
    PatientIdNumIndexEntry.__tablename__,
    SecurityAccountLockout.__tablename__,
    SecurityLoginFailure.__tablename__,
    ServerSettings.__tablename__,
    SpecialNote.__tablename__,
    TaskFilter.__tablename__,
    TaskIndexEntry.__tablename__,
    TaskSchedule.__tablename__,
    TaskScheduleItem.__tablename__,
    User.__tablename__,
    UserGroupMembership.__tablename__,
]
RESERVED_FIELDS = GenericTabletRecordMixin.RESERVED_FIELDS

# =============================================================================
# Catalogue tables that clients use
# =============================================================================

CLIENT_TABLE_MAP = {}  # type: Dict[str, Table]
NONTASK_CLIENT_TABLENAMES = []  # type: List[str]

# Add all tables that clients may upload to (including ancillary tables).
for __orm_class in gen_orm_classes_from_base(Base):  # type: Type[Base]
    # noinspection PyUnresolvedReferences
    if issubclass(__orm_class, GenericTabletRecordMixin):
        __tablename = __orm_class.__tablename__
        if __tablename not in RESERVED_TABLE_NAMES:
            # Additional safety check: no client tables start with "_" and all
            # server tables do:
            if __tablename.startswith("_"):
                pass
            # noinspection PyUnresolvedReferences
            __table = __orm_class.__table__  # type: Table
            CLIENT_TABLE_MAP[__tablename] = __table
            if not issubclass(__orm_class, Task):
                NONTASK_CLIENT_TABLENAMES.append(__tablename)
NONTASK_CLIENT_TABLENAMES.sort()
# log.debug("NONTASK_CLIENT_TABLENAMES: {}", NONTASK_CLIENT_TABLENAMES)
