#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_group.py

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

**Group definitions.**

"""

import logging
from typing import List, Optional, Set

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import simple_repr
from cardinal_pythonlib.sqlalchemy.orm_inspect import gen_columns
from cardinal_pythonlib.sqlalchemy.orm_query import exists_orm
from sqlalchemy.ext.associationproxy import association_proxy
from sqlalchemy.orm import relationship, Session as SqlASession
from sqlalchemy.sql.schema import Column, ForeignKey, Table
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_ipuse import IpUse
from camcops_server.cc_modules.cc_policy import TokenizedPolicy
from camcops_server.cc_modules.cc_sqla_coltypes import (
    GroupDescriptionColType,
    GroupNameColType,
    IdPolicyColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Group-to-group association table
# =============================================================================
# A group can always see itself, but may also have permission to see others;
# see "Groups" in the CamCOPS documentation.

# http://docs.sqlalchemy.org/en/latest/orm/join_conditions.html#self-referential-many-to-many-relationship  # noqa
group_group_table = Table(
    "_security_group_group",
    Base.metadata,
    Column("group_id", Integer, ForeignKey("_security_groups.id"),
           primary_key=True),
    Column("can_see_group_id", Integer, ForeignKey("_security_groups.id"),
           primary_key=True)
)


# =============================================================================
# Group
# =============================================================================

class Group(Base):
    """
    Represents a CamCOPS group.

    See "Groups" in the CamCOPS documentation.
    """
    __tablename__ = "_security_groups"

    id = Column(
        "id", Integer,
        primary_key=True, autoincrement=True, index=True,
        comment="Group ID"
    )
    name = Column(
        "name", GroupNameColType,
        nullable=False, index=True, unique=True,
        comment="Group name"
    )
    description = Column(
        "description", GroupDescriptionColType,
        comment="Description of the group"
    )
    upload_policy = Column(
        "upload_policy", IdPolicyColType,
        comment="Upload policy for the group, as a string"
    )
    finalize_policy = Column(
        "finalize_policy", IdPolicyColType,
        comment="Finalize policy for the group, as a string"
    )

    ip_use_id = Column(
        "ip_use_id", Integer, ForeignKey(IpUse.id),
        nullable=True,
        comment=f"FK to {IpUse.__tablename__}.{IpUse.id.name}"
    )

    ip_use = relationship(IpUse, uselist=False, single_parent=True,
                          cascade="all, delete-orphan")

    # users = relationship(
    #     "User",  # defined with string to avoid circular import
    #     secondary=user_group_table,  # link via this mapping table
    #     back_populates="groups"  # see User.groups
    # )
    user_group_memberships = relationship(
        "UserGroupMembership", back_populates="group")
    users = association_proxy("user_group_memberships", "user")

    regular_user_group_memberships = relationship(
        "UserGroupMembership",
        primaryjoin="and_("
        "Group.id==UserGroupMembership.group_id, "
        "User.id==UserGroupMembership.user_id, "
        "User.auto_generated==False)"
    )
    regular_users = association_proxy(
        "regular_user_group_memberships",
        "user"
    )

    can_see_other_groups = relationship(
        "Group",  # link back to our own class
        secondary=group_group_table,  # via this mapping table
        primaryjoin=(id == group_group_table.c.group_id),  # "us"
        secondaryjoin=(id == group_group_table.c.can_see_group_id),  # "them"
        backref="groups_that_can_see_us",
        lazy="joined"  # not sure this does anything here
    )

    def __str__(self) -> str:
        return f"Group {self.id} ({self.name})"

    def __repr__(self) -> str:
        attrnames = sorted(attrname for attrname, _ in gen_columns(self))
        return simple_repr(self, attrnames)

    def ids_of_other_groups_group_may_see(self) -> Set[int]:
        """
        Returns a list of group IDs for groups that this group has permission
        to see. (Always includes our own group number.)
        """
        group_ids = set()  # type: Set[int]
        for other_group in self.can_see_other_groups:  # type: Group
            other_group_id = other_group.id  # type: Optional[int]
            if other_group_id is not None:
                group_ids.add(other_group_id)
        return group_ids

    def ids_of_groups_group_may_see(self) -> Set[int]:
        """
        Returns a list of group IDs for groups that this group has permission
        to see. (Always includes our own group number.)
        """
        ourself = {self.id}  # type: Set[int]
        return ourself.union(self.ids_of_other_groups_group_may_see())

    @classmethod
    def get_groups_from_id_list(cls, dbsession: SqlASession,
                                group_ids: List[int]) -> List["Group"]:
        """
        Fetches groups from a list of group IDs.
        """
        return dbsession.query(Group).filter(Group.id.in_(group_ids)).all()

    @classmethod
    def get_group_by_name(cls, dbsession: SqlASession,
                          name: str) -> Optional["Group"]:
        """
        Fetches a group from its name.
        """
        if not name:
            return None
        return dbsession.query(cls).filter(cls.name == name).first()

    @classmethod
    def get_group_by_id(cls, dbsession: SqlASession,
                        group_id: int) -> Optional["Group"]:
        """
        Fetches a group from its integer ID.
        """
        if group_id is None:
            return None
        return dbsession.query(cls).filter(cls.id == group_id).first()

    @classmethod
    def get_all_groups(cls, dbsession: SqlASession) -> List["Group"]:
        """
        Returns all groups.
        """
        return dbsession.query(Group).all()

    @classmethod
    def all_group_ids(cls, dbsession: SqlASession) -> List[int]:
        """
        Returns all group IDs.
        """
        query = dbsession.query(cls).order_by(cls.id)
        return [g.id for g in query]

    @classmethod
    def all_group_names(cls, dbsession: SqlASession) -> List[str]:
        """
        Returns all group names.
        """
        query = dbsession.query(cls).order_by(cls.id)
        return [g.name for g in query]

    @classmethod
    def group_exists(cls, dbsession: SqlASession, group_id: int) -> bool:
        """
        Does a particular group (specified by its integer ID) exist?
        """
        return exists_orm(dbsession, cls, cls.id == group_id)

    def tokenized_upload_policy(self) -> TokenizedPolicy:
        """
        Returns the upload policy for a group.
        """
        return TokenizedPolicy(self.upload_policy)

    def tokenized_finalize_policy(self) -> TokenizedPolicy:
        """
        Returns the finalize policy for a group.
        """
        return TokenizedPolicy(self.finalize_policy)
