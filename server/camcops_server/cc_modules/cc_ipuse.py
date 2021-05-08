#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_ipuse.py

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

IP Use flags that determine the contexts in which CamCOPS is to be used (and
hence the tasks that will be permitted).

"""

from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Integer

from cardinal_pythonlib.reprfunc import auto_repr, auto_str

from camcops_server.cc_modules.cc_sqlalchemy import Base


_DEFAULT_APPLICABILITY = False


class IpContexts(object):
    """
    String constants, used as form parameter names etc.
    """
    CLINICAL = "clinical"
    COMMERCIAL = "commercial"
    EDUCATIONAL = "educational"
    RESEARCH = "research"


class IpUse(Base):
    __tablename__ = "_security_ip_use"

    CONTEXTS = (
        IpContexts.CLINICAL,
        IpContexts.COMMERCIAL,
        IpContexts.EDUCATIONAL,
        IpContexts.RESEARCH,
    )
    _DEFAULT = False

    id = Column(
        "id", Integer,
        primary_key=True,
        autoincrement=True,
        index=True,
        comment="IP Use ID"
    )

    clinical = Column(
        "clinical", Boolean,
        nullable=False,
        default=_DEFAULT,
        comment="Applicable to a clinical context"
    )
    commercial = Column(
        "commercial", Boolean,
        nullable=False,
        default=_DEFAULT,
        comment="Applicable to a commercial context"
    )
    educational = Column(
        "educational", Boolean,
        nullable=False,
        default=_DEFAULT,
        comment="Applicable to an educational context"
    )
    research = Column(
        "research", Boolean,
        nullable=False,
        default=_DEFAULT,
        comment="Applicable to a research context"
    )

    def __init__(self,
                 clinical: bool = _DEFAULT_APPLICABILITY,
                 commercial: bool = _DEFAULT_APPLICABILITY,
                 educational: bool = _DEFAULT_APPLICABILITY,
                 research: bool = _DEFAULT_APPLICABILITY) -> None:
        """
        We provide __init__() so we can create a default object without
        touching the database.
        """
        self.clinical = clinical
        self.commercial = commercial
        self.educational = educational
        self.research = research

    def __repr__(self) -> str:
        return auto_repr(self)

    def __str__(self) -> str:
        return auto_str(self)
