#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_membership.py

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

**Represents a user's membership of a group.**

"""

import logging
from typing import Optional

from cardinal_pythonlib.logs import BraceStyleAdapter
from sqlalchemy.orm import relationship, Session as SqlASession
from sqlalchemy.sql.schema import Column, ForeignKey
from sqlalchemy.sql.sqltypes import Boolean, Integer

from camcops_server.cc_modules.cc_sqlalchemy import Base

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# User-to-group association table
# =============================================================================
# This is many-to-many:
#   A user can [be in] many groups.
#   A group can [contain] many users.

# -----------------------------------------------------------------------------
# First version:
# -----------------------------------------------------------------------------
# http://docs.sqlalchemy.org/en/latest/orm/basic_relationships.html#many-to-many
# user_group_table = Table(
#     "_security_user_group",
#     Base.metadata,
#     Column("user_id", Integer, ForeignKey("_security_users.id"),
#            primary_key=True),
#     Column("group_id", Integer, ForeignKey("_security_groups.id"),
#            primary_key=True)
# )


# -----------------------------------------------------------------------------
# Second version, when we want more information in the relationship:
# -----------------------------------------------------------------------------
# https://stackoverflow.com/questions/7417906/sqlalchemy-manytomany-secondary-table-with-additional-fields  # noqa
# ... no, association_proxy isn't quite what we want
# ... http://docs.sqlalchemy.org/en/latest/orm/extensions/associationproxy.html
# http://docs.sqlalchemy.org/en/latest/orm/basic_relationships.html#association-object  # noqa
# ... yes
# ... ah, but that AND association_proxy:
# http://docs.sqlalchemy.org/en/latest/orm/extensions/associationproxy.html#simplifying-association-objects  # noqa
# ... no, not association_proxy!

class UserGroupMembership(Base):
    """
    Represents a user's membership of a group, and associated per-group
    permissions.
    """
    __tablename__ = "_security_user_group"

    # PK, so we can use this object easily on its own via the ORM.
    id = Column(
        "id", Integer, primary_key=True, autoincrement=True,
    )

    # Many-to-many mapping between User and Group
    user_id = Column(
        "user_id", Integer, ForeignKey("_security_users.id")
    )
    group_id = Column(
        "group_id", Integer, ForeignKey("_security_groups.id"),
    )

    # User attributes that are specific to their group membership
    groupadmin = Column(
        "groupadmin", Boolean,
        default=False,
        comment="Is the user a privileged administrator for this group?"
    )
    may_upload = Column(
        "may_upload", Boolean,
        default=False,
        comment="May the user upload data from a tablet device?"
    )
    may_register_devices = Column(
        "may_register_devices", Boolean,
        default=False,
        comment="May the user register tablet devices?"
    )
    may_use_webviewer = Column(
        "may_use_webviewer", Boolean,
        default=False,
        comment="May the user use the web front end to view "
                "CamCOPS data?"
    )
    view_all_patients_when_unfiltered = Column(
        "view_all_patients_when_unfiltered", Boolean,
        default=False,
        comment="When no record filters are applied, can the user see "
                "all records? (If not, then none are shown.)"
    )
    may_dump_data = Column(
        "may_dump_data", Boolean,
        default=False,
        comment="May the user run database data dumps via the web interface?"
    )
    may_run_reports = Column(
        "may_run_reports", Boolean,
        default=False,
        comment="May the user run reports via the web interface? "
                "(Overrides other view restrictions.)"
    )
    may_add_notes = Column(
        "may_add_notes", Boolean,
        default=False,
        comment="May the user add special notes to tasks?"
    )

    group = relationship("Group", back_populates="user_group_memberships")
    user = relationship("User", back_populates="user_group_memberships")

    def __init__(self, user_id: int, group_id: int):
        self.user_id = user_id
        self.group_id = group_id

    @classmethod
    def get_ugm_by_id(cls,
                      dbsession: SqlASession,
                      ugm_id: Optional[int]) -> Optional['UserGroupMembership']:
        """
        Fetches a :class:`UserGroupMembership` by its ID.
        """
        if ugm_id is None:
            return None
        return dbsession.query(cls).filter(cls.id == ugm_id).first()
