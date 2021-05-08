#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_patientidnum.py

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

**Represent patient ID numbers.**

We were looking up ID descriptors from the device's stored variables.
However, that is a bit of a nuisance for a server-side researcher, and
it's a pain to copy the server's storedvar values (and -- all or some?)
when a patient gets individually moved off the tablet. Anyway, they're
important, so a little repetition is not the end of the world. So,
let's have the tablet store its current ID descriptors in the patient
record at the point of upload, and then it's available here directly.
Thus, always complete and contemporaneous.

... DECISION CHANGED 2017-07-08; see justification in tablet
    overall_design.txt

"""

import logging
from typing import List, Tuple, TYPE_CHECKING

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import simple_repr
from sqlalchemy.orm import relationship
from sqlalchemy.sql.schema import Column, ForeignKey
from sqlalchemy.sql.sqltypes import BigInteger, Integer

from camcops_server.cc_modules.cc_constants import (
    EXTRA_COMMENT_PREFIX,
    EXTRA_IDNUM_FIELD_PREFIX,
    NUMBER_OF_IDNUMS_DEFUNCT,
)
from camcops_server.cc_modules.cc_db import GenericTabletRecordMixin
from camcops_server.cc_modules.cc_idnumdef import IdNumDefinition
from camcops_server.cc_modules.cc_simpleobjects import IdNumReference
from camcops_server.cc_modules.cc_sqla_coltypes import CamcopsColumn
from camcops_server.cc_modules.cc_sqlalchemy import Base

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_patient import Patient
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# PatientIdNum class
# =============================================================================
# Stores ID numbers for a specific patient

class PatientIdNum(GenericTabletRecordMixin, Base):
    """
    SQLAlchemy ORM class representing an ID number (as a
    which_idnum/idnum_value pair) for a patient.
    """
    __tablename__ = "patient_idnum"

    id = Column(
        "id", Integer,
        nullable=False,
        comment="Primary key on the source tablet device"
    )
    patient_id = Column(
        "patient_id", Integer,
        nullable=False,
        comment="FK to patient.id (for this device/era)"
    )
    which_idnum = Column(
        "which_idnum", Integer, ForeignKey(IdNumDefinition.which_idnum),
        nullable=False,
        comment="Which of the server's ID numbers is this?"
    )
    idnum_value = CamcopsColumn(
        "idnum_value", BigInteger,
        identifies_patient=True,
        comment="The value of the ID number"
    )
    # Note: we don't use a relationship() to IdNumDefinition here; we do that
    # sort of work via the CamcopsRequest, which caches them for speed.

    patient = relationship(
        # http://docs.sqlalchemy.org/en/latest/orm/join_conditions.html#relationship-custom-foreign
        # http://docs.sqlalchemy.org/en/latest/orm/relationship_api.html#sqlalchemy.orm.relationship  # noqa
        # http://docs.sqlalchemy.org/en/latest/orm/join_conditions.html#relationship-primaryjoin  # noqa
        "Patient",
        primaryjoin=(
            "and_("
            " remote(Patient.id) == foreign(PatientIdNum.patient_id), "
            " remote(Patient._device_id) == foreign(PatientIdNum._device_id), "
            " remote(Patient._era) == foreign(PatientIdNum._era), "
            " remote(Patient._current) == True "
            ")"
        ),
        uselist=False,
        viewonly=True,
    )

    # -------------------------------------------------------------------------
    # String representations
    # -------------------------------------------------------------------------

    def __str__(self) -> str:
        return f"idnum{self.which_idnum}={self.idnum_value}"

    def prettystr(self, req: "CamcopsRequest") -> str:
        """
        A prettified version of __str__.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        """
        return f"{self.short_description(req)} {self.idnum_value}"

    def full_prettystr(self, req: "CamcopsRequest") -> str:
        """
        A long-version prettified version of __str__.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        """
        return f"{self.description(req)} {self.idnum_value}"

    def __repr__(self) -> str:
        return simple_repr(self, [
            "_pk", "_device_id", "_era",
            "id", "patient_id", "which_idnum", "idnum_value"
        ])

    # -------------------------------------------------------------------------
    # Equality
    # -------------------------------------------------------------------------

    def __members(self) -> Tuple:
        """
        For :meth:`__hash__` and :meth:`__eq__`, as per
        https://stackoverflow.com/questions/45164691/recommended-way-to-implement-eq-and-hash
        """  # noqa
        return self.which_idnum, self.idnum_value

    def __hash__(self) -> int:
        """
        Must be compatible with __eq__.
        
        See also 
        https://stackoverflow.com/questions/45164691/recommended-way-to-implement-eq-and-hash
        """  # noqa
        return hash(self.__members())

    def __eq__(self, other: "PatientIdNum") -> bool:
        """
        Do ``self`` and ``other`` represent the same ID number?

        Equivalent to:

        .. code-block:: python

            return (
                self.which_idnum == other.which_idnum and
                self.idnum_value == other.idnum_value and
                self.which_idnum is not None and
                self.idnum_value is not None
            )
        """
        sm = self.__members()
        return (
            type(self) is type(other) and
            (None not in sm) and
            sm == other.__members()
        )

    # -------------------------------------------------------------------------
    # Validity
    # -------------------------------------------------------------------------

    def is_superficially_valid(self) -> bool:
        """
        Is this a valid ID number?
        """
        return (
            self.which_idnum is not None and
            self.idnum_value is not None and
            self.which_idnum >= 0 and
            self.idnum_value >= 0
        )

    def is_fully_valid(self, req: "CamcopsRequest") -> bool:
        if not self.is_superficially_valid():
            return False
        return req.is_idnum_valid(self.which_idnum, self.idnum_value)

    def why_invalid(self, req: "CamcopsRequest") -> str:
        if not self.is_superficially_valid():
            _ = req.gettext
            return _("ID number fails basic checks")
        return req.why_idnum_invalid(self.which_idnum, self.idnum_value)

    # -------------------------------------------------------------------------
    # ID type description
    # -------------------------------------------------------------------------

    def description(self, req: "CamcopsRequest") -> str:
        """
        Returns the full description for this ID number.
        """
        which_idnum = self.which_idnum  # type: int
        return req.get_id_desc(which_idnum, default="?")

    def short_description(self, req: "CamcopsRequest") -> str:
        """
        Returns the short description for this ID number.
        """
        which_idnum = self.which_idnum  # type: int
        return req.get_id_shortdesc(which_idnum, default="?")

    # -------------------------------------------------------------------------
    # Other representations
    # -------------------------------------------------------------------------

    def get_idnum_reference(self) -> IdNumReference:
        """
        Returns an
        :class:`camcops_server.cc_modules.cc_simpleobjects.IdNumReference`
        object summarizing this ID number.
        """
        return IdNumReference(which_idnum=self.which_idnum,
                              idnum_value=self.idnum_value)

    def get_filename_component(self, req: "CamcopsRequest") -> str:
        """
        Returns a string including the short description of the ID number, and
        the number itself, for use in filenames.
        """
        if self.which_idnum is None or self.idnum_value is None:
            return ""
        return f"{self.short_description(req)}-{self.idnum_value}"

    # -------------------------------------------------------------------------
    # Set value
    # -------------------------------------------------------------------------

    def set_idnum(self, idnum_value: int) -> None:
        """
        Sets the ID number value.
        """
        self.idnum_value = idnum_value

    # -------------------------------------------------------------------------
    # Patient
    # -------------------------------------------------------------------------

    def get_patient_server_pk(self) -> int:
        patient = self.patient  # type: Patient
        if not patient:
            raise ValueError(
                "Corrupted database? PatientIdNum can't fetch its Patient")
        return patient.pk


# =============================================================================
# Fake ID values when upgrading from old ID number system
# =============================================================================

def fake_tablet_id_for_patientidnum(patient_id: int, which_idnum: int) -> int:
    """
    Returns a fake client-side PK (tablet ID) for a patient number. Only for
    use in upgrading old databases.
    """
    return patient_id * NUMBER_OF_IDNUMS_DEFUNCT + which_idnum


# =============================================================================
# Additional ID number column info for DB_PATIENT_ID_PER_ROW export option
# =============================================================================

def extra_id_colname(which_idnum: int) -> str:
    """
    The column name used for the extra ID number columns provided by the
    ``DB_PATIENT_ID_PER_ROW`` export option.

    Args:
        which_idnum: ID number type

    Returns:
        str: ``idnum<which_idnum>``

    """
    return f"{EXTRA_IDNUM_FIELD_PREFIX}{which_idnum}"


def extra_id_column(req: "CamcopsRequest", which_idnum: int) -> CamcopsColumn:
    """
    The column definition used for the extra ID number columns provided by the
    ``DB_PATIENT_ID_PER_ROW`` export option.

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        which_idnum: ID number type

    Returns:
        the column definition

    """
    desc = req.get_id_desc(which_idnum)
    return CamcopsColumn(
        extra_id_colname(which_idnum),
        BigInteger,
        identifies_patient=True,
        comment=EXTRA_COMMENT_PREFIX + f"ID number {which_idnum}: {desc}"
    )


def all_extra_id_columns(req: "CamcopsRequest") -> List[CamcopsColumn]:
    """
    Returns all column definitions used for the extra ID number columns
    provided by the ``DB_PATIENT_ID_PER_ROW`` export option.

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`

    Returns:
        list: the column definitions
    """
    return [
        extra_id_column(req, which_idnum)
        for which_idnum in req.valid_which_idnums
    ]
