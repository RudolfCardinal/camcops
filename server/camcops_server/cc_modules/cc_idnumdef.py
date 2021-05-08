#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_idnumdef.py

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

**ID number definitions.**

"""

import logging
from typing import List, Optional, Tuple, TYPE_CHECKING

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.nhs import is_valid_nhs_number
from cardinal_pythonlib.reprfunc import simple_repr
from sqlalchemy.orm import Session as SqlASession
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, String

from camcops_server.cc_modules.cc_sqla_coltypes import (
    HL7AssigningAuthorityType,
    HL7IdTypeType,
    IdDescriptorColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# ID number validation
# =============================================================================

ID_NUM_VALIDATION_METHOD_MAX_LEN = 50


class IdNumValidationMethod(object):
    """
    Constants representing ways that CamCOPS knows to validate ID numbers.
    """
    NONE = ""  # special
    UK_NHS_NUMBER = "uk_nhs_number"


ID_NUM_VALIDATION_METHOD_CHOICES = (
    # for HTML forms: value, description
    (IdNumValidationMethod.NONE, "None"),
    (IdNumValidationMethod.UK_NHS_NUMBER, "UK NHS number"),
)


def validate_id_number(req: "CamcopsRequest",
                       idnum: Optional[int], method: str) -> Tuple[bool, str]:
    """
    Validates an ID number according to a method (as per
    :class:`IdNumValidationMethod`).

    If the number is ``None``, that's valid (that's an ID policy failure, not
    a number validation failure). If ``method`` is falsy, that's also valid
    (no constraints).

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        idnum: the ID number, or ``None``
        method:

    Returns:
        tuple: ``valid, why_invalid`` where ``valid`` is ``bool`` and
        ``why_invalid`` is ``str``.

    """
    _ = req.gettext
    if idnum is None or not method:
        return True, ""
    if not isinstance(idnum, int):
        return False, _("Not an integer")
    if method == IdNumValidationMethod.UK_NHS_NUMBER:
        if is_valid_nhs_number(idnum):
            return True, ""
        else:
            return False, _("Invalid UK NHS number")
    return False, _("Unknown validation method")


# =============================================================================
# IdNumDefinition
# =============================================================================

class IdNumDefinition(Base):
    """
    Represents an ID number definition.
    """
    __tablename__ = "_idnum_definitions"

    which_idnum = Column(
        "which_idnum", Integer, primary_key=True, index=True,
        comment="Which of the server's ID numbers is this?"
    )
    description = Column(
        "description", IdDescriptorColType,
        comment="Full description of the ID number"
    )
    short_description = Column(
        "short_description", IdDescriptorColType,
        comment="Short description of the ID number"
    )
    hl7_id_type = Column(
        "hl7_id_type", HL7IdTypeType,
        comment="HL7: Identifier Type code: 'a code corresponding to the type "
                "of identifier. In some cases, this code may be used as a "
                "qualifier to the \"Assigning Authority\" component.'"
    )
    hl7_assigning_authority = Column(
        "hl7_assigning_authority", HL7AssigningAuthorityType,
        comment="HL7: Assigning Authority for ID number (unique name of the "
                "system/organization/agency/department that creates the data)."
    )
    validation_method = Column(
        "validation_method", String(length=ID_NUM_VALIDATION_METHOD_MAX_LEN),
        comment="Optional validation method"
    )

    def __init__(self,
                 which_idnum: int = None,
                 description: str = "",
                 short_description: str = "",
                 hl7_id_type: str = "",
                 hl7_assigning_authority: str = "",
                 validation_method: str = ""):
        # We permit a "blank" constructor for automatic copying, e.g. merge_db.
        self.which_idnum = which_idnum
        self.description = description
        self.short_description = short_description
        self.hl7_id_type = hl7_id_type
        self.hl7_assigning_authority = hl7_assigning_authority
        self.validation_method = validation_method

    def __repr__(self) -> str:
        return simple_repr(self,
                           ["which_idnum", "description", "short_description"],
                           with_addr=False)


# =============================================================================
# Retrieving all IdNumDefinition objects
# =============================================================================

def get_idnum_definitions(dbsession: SqlASession) -> List[IdNumDefinition]:
    """
    Get all ID number definitions from the database, in order.
    """
    return list(
        dbsession.query(IdNumDefinition)
        .order_by(IdNumDefinition.which_idnum)
    )
