#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_serversettings.py

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

**Represents server-wide configuration settings.**

Previously, we had a key/value pair system, both for device stored variables
(table "storedvars") and server ones ("_server_storedvars"). We used a "type"
column to indicate type, and then columns named "valueInteger", "valueText",
"valueReal" for the actual values.

Subsequently

- There's no need for devices to upload their settings here, so that table
  goes.

- The server stored vars stored

.. code-block:: none

    idDescription1 - idDescription8             } now have their own table
    idShortDescription1 - idShortDescription8   }

    idPolicyUpload                              } now part of Group definition
    idPolicyFinalize                            }

    lastAnalyticsSentAt                         now unused

    serverCamcopsVersion                        unnecessary (is in code)

    databaseTitle                               still needed somehow

So, two options:
https://stackoverflow.com/questions/2300356/using-a-single-row-configuration-table-in-sql-server-database-bad-idea

Let's use a single row, based on a fixed PK (of 1).

On some databases, you can constrain the PK value to enforce "one row only";
MySQL isn't one of those.

- https://docs.sqlalchemy.org/en/latest/core/constraints.html#check-constraint

- https://stackoverflow.com/questions/3967372/sql-server-how-to-constrain-a-table-to-contain-a-single-row

"""  # noqa

import logging
from typing import Optional, TYPE_CHECKING

from cardinal_pythonlib.logs import BraceStyleAdapter
import pendulum
from pendulum import DateTime as Pendulum
from sqlalchemy.sql.schema import Column, MetaData, Table
from sqlalchemy.sql.sqltypes import (
    DateTime, Float, Integer, String, UnicodeText,
)

from camcops_server.cc_modules.cc_sqla_coltypes import DatabaseTitleColType
from camcops_server.cc_modules.cc_sqlalchemy import Base

if TYPE_CHECKING:
    from datetime import datetime
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# ServerStoredVars - defunct, but maintained for database imports
# =============================================================================

class StoredVarTypesDefunct(object):
    """
    Variable types for the ServerStoredVars system.

    Defunct, but maintained for database imports.
    """
    # values for the "type" column
    TYPE_INTEGER = "integer"
    TYPE_TEXT = "text"
    TYPE_REAL = "real"


class ServerStoredVarNamesDefunct(object):
    """
    Variable names for the ServerStoredVars system.

    Defunct, but maintained for database imports.
    """
    # values for the "name" column
    ID_POLICY_UPLOAD = "idPolicyUpload"  # text
    ID_POLICY_FINALIZE = "idPolicyFinalize"  # text
    SERVER_CAMCOPS_VERSION = "serverCamcopsVersion"  # text
    DATABASE_TITLE = "databaseTitle"  # text
    LAST_ANALYTICS_SENT_AT = "lastAnalyticsSentAt"  # text
    ID_DESCRIPTION_PREFIX = "idDescription"  # text; apply suffixes 1-8
    ID_SHORT_DESCRIPTION_PREFIX = "idShortDescription"  # text; apply suffixes 1-8  # noqa


StoredVarNameColTypeDefunct = String(length=255)
StoredVarTypeColTypeDefunct = String(length=255)
_ssv_metadata = MetaData()


server_stored_var_table_defunct = Table(
    "_server_storedvars",  # table name
    _ssv_metadata,  # metadata separate from everything else
    Column(
        "name", StoredVarNameColTypeDefunct,
        primary_key=True, index=True,
        comment="Variable name"
    ),
    Column(
        "type", StoredVarTypeColTypeDefunct,
        nullable=False,
        comment="Variable type ('integer', 'real', 'text')"
    ),
    Column(
        "valueInteger", Integer,
        comment="Value of an integer variable"
    ),
    Column(
        "valueText", UnicodeText,
        comment="Value of a text variable"
    ),
    Column(
        "valueReal", Float,
        comment="Value of a real (floating-point) variable"
    )
)


# =============================================================================
# ServerSettings
# =============================================================================

SERVER_SETTINGS_SINGLETON_PK = 1
# CACHE_KEY_DATABASE_TITLE = "database_title"


class ServerSettings(Base):
    """
    Singleton SQLAlchemy object (i.e. there is just one row in the database
    table) representing server settings.
    """
    __tablename__ = "_server_settings"

    id = Column(
        "id", Integer,
        primary_key=True, autoincrement=True, index=True,
        comment=(
            f"PK (arbitrary integer but only a value of "
            f"{SERVER_SETTINGS_SINGLETON_PK} is ever used)"
        )
    )
    database_title = Column(
        "database_title", DatabaseTitleColType,
        comment="Database title"
    )
    last_dummy_login_failure_clearance_at_utc = Column(
        "last_dummy_login_failure_clearance_at_utc", DateTime,
        comment="Date/time (in UTC) when login failure records were cleared "
                "for nonexistent users (security feature)"
    )

    def get_last_dummy_login_failure_clearance_pendulum(self) \
            -> Optional[Pendulum]:
        """
        Returns the time at which login failure records were cleared for
        nonexistent users.

        This is part of a security failure to prevent attackers discovering
        usernames: since repeated attempts to hack a real account leads to an
        account lockout, we arrange things so that attempts to hack nonexistent
        accounts do likewise.

        Specifically, this function returns an offset-aware (timezone-aware)
        version of the raw UTC DATETIME from the database.
        """
        dt = self.last_dummy_login_failure_clearance_at_utc  # type: Optional[datetime]  # noqa
        if dt is None:
            return None
        return pendulum.instance(dt, tz=pendulum.UTC)


def get_server_settings(req: "CamcopsRequest") -> ServerSettings:
    """
    Gets the
    :class:`camcops_server.cc_modules.cc_serversettings.ServerSettings` object
    for the request.
    """
    dbsession = req.dbsession
    server_settings = dbsession.query(ServerSettings)\
        .filter(ServerSettings.id == SERVER_SETTINGS_SINGLETON_PK)\
        .first()
    if server_settings is None:
        server_settings = ServerSettings()
        server_settings.id = SERVER_SETTINGS_SINGLETON_PK
        server_settings.database_title = "DATABASE_TITLE_UNSET"
        dbsession.add(server_settings)
    return server_settings


# def get_database_title(req: "CamcopsRequest") -> str:
#     def creator() -> str:
#         server_settings = get_server_settings(req)
#         return server_settings.database_title or ""
#
#     return cache_region_static.get_or_create(CACHE_KEY_DATABASE_TITLE, creator)  # noqa


# def clear_database_title_cache() -> None:
#     cache_region_static.delete(CACHE_KEY_DATABASE_TITLE)


# def set_database_title(req: "CamcopsRequest", title: str) -> None:
#     server_settings = get_server_settings(req)
#     server_settings.database_title = title
#     clear_database_title_cache()
