#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_simpleobjects.py

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

**Simple struct-like classes.**

"""

import copy
# import logging
from typing import List, TYPE_CHECKING

from pendulum import Date

from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.reprfunc import auto_repr

from camcops_server.cc_modules.cc_constants import DateFormat

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

# log = logging.getLogger(__name__)

# Prefer classes to collections.namedtuple; both support type checking but
# classes support better parameter checking (and refactoring) via PyCharm.


# =============================================================================
# IdNumReference
# =============================================================================

class IdNumReference(object):
    """
    A simple way of referring to an ID number.

    It's not stored in the database -- it's just an object to be passed around
    that encapsulates ``which_idnum`` and ``idnum_value``.

    As an example, suppose our administrator has defined ID type
    (``which_idnum``) 7 to be "NHS number". Then if a patient has NHS number
    9999999999, we might represent this ID of theirs as
    ``IdNumReference(which_idnum=7, idnum_value=9999999999)``.
    """
    def __init__(self, which_idnum: int, idnum_value: int) -> None:
        self.which_idnum = which_idnum
        self.idnum_value = idnum_value

    def __str__(self) -> str:
        return f"idnum{self.which_idnum}={self.idnum_value}"

    def __repr__(self) -> str:
        return auto_repr(self)

    def is_valid(self) -> bool:
        return (
            self.which_idnum is not None and self.which_idnum > 0 and
            self.idnum_value is not None and self.idnum_value > 0
        )

    def __eq__(self, other: "IdNumReference") -> bool:
        if not isinstance(other, IdNumReference):
            return False
        return (
            self.which_idnum == other.which_idnum and
            self.idnum_value == other.idnum_value
        )

    def description(self, req: "CamcopsRequest") -> str:
        if not self.is_valid():
            return "[invalid_IdNumReference]"
        return f"{req.get_id_shortdesc(self.which_idnum)} = {self.idnum_value}"


# =============================================================================
# HL7PatientIdentifier
# =============================================================================

# noinspection PyShadowingBuiltins
class HL7PatientIdentifier(object):
    """
    Represents a patient identifier for the HL7 protocol.
    """
    def __init__(self, pid: str, id_type: str,
                 assigning_authority: str) -> None:
        self.pid = pid
        # ... called "pid" not "id" as type checker sometimes thinks "id" must
        # be integer, as in Python's id(object).
        self.id_type = id_type
        self.assigning_authority = assigning_authority


# =============================================================================
# BarePatientInfo
# =============================================================================

class BarePatientInfo(object):
    """
    Represents information about a patient using a simple object with no
    connection to a database.

    In some situations we avoid using
    :class:`camcops_server.cc_modules.cc_patient.Patient`: specifically, when
    we would otherwise have to deal with mutual dependency problems and the use
    of the database (prior to full database initialization).
    """
    def __init__(self,
                 forename: str = None,
                 surname: str = None,
                 sex: str = None,
                 dob: Date = None,
                 address: str = None,
                 email: str = None,
                 gp: str = None,
                 otherdetails: str = None,
                 idnum_definitions: List[IdNumReference] = None) -> None:
        self.forename = forename
        self.surname = surname
        self.sex = sex
        self.dob = dob
        self.address = address
        self.email = email
        self.gp = gp
        self.otherdetails = otherdetails
        self.idnum_definitions = idnum_definitions or []  # type: List[IdNumReference]  # noqa

    def __str__(self) -> str:
        return (
            "Patient(forename={f!r}, surname={sur!r}, sex={sex!r}, DOB={dob}, "
            "address={a!r}, email={email!r}, gp={gp!r}, otherdetails={o!r}, "
            "idnums={i})".format(
                f=self.forename,
                sur=self.surname,
                sex=self.sex,
                dob=format_datetime(self.dob, DateFormat.ISO8601_DATE_ONLY),
                a=self.address,
                email=self.email,
                gp=self.gp,
                o=self.otherdetails,
                i="[{}]".format(", ".join(
                    str(idnum) for idnum in self.idnum_definitions)),
            )
        )

    def __repr__(self) -> str:
        return auto_repr(self)

    def add_idnum(self, idref: IdNumReference) -> None:
        """
        Adds an ID number. No checks in relation to what's already present.

        Args:
            idref: a :class:`IdNumReference`
        """
        self.idnum_definitions.append(idref)

    def __eq__(self, other: "BarePatientInfo") -> bool:
        """
        Do all data elements match those of ``other``?
        """
        if not isinstance(other, BarePatientInfo):
            return False
        return (
            self.forename == other.forename and
            self.surname == other.surname and
            self.sex == other.sex and
            self.dob == other.dob and
            self.address == other.address and
            self.email == other.email and
            self.gp == other.gp and
            self.otherdetails == other.otherdetails and
            self.idnum_definitions == other.idnum_definitions
        )


# =============================================================================
# Raw XML value
# =============================================================================

class XmlSimpleValue(object):
    """
    Represents XML lowest-level items. See functions in ``cc_xml.py``.
    """
    def __init__(self, value) -> None:
        self.value = value


# =============================================================================
# TaskExportOptions
# =============================================================================

class TaskExportOptions(object):
    """
    Information-holding object for options controlling XML and other
    representations of tasks.
    """
    def __init__(self,
                 db_patient_id_per_row: bool = False,
                 db_make_all_tables_even_empty: bool = False,
                 db_include_summaries: bool = False,
                 include_blobs: bool = False,
                 xml_include_ancillary: bool = False,
                 xml_include_calculated: bool = False,
                 xml_include_comments: bool = True,
                 xml_include_patient: bool = False,
                 xml_include_plain_columns: bool = False,
                 xml_include_snomed: bool = False,
                 xml_skip_fields: List[str] = None,
                 xml_sort_by_name: bool = True,
                 xml_with_header_comments: bool = False) -> None:
        """
        Args:
            db_patient_id_per_row:
                generates an anonymisation staging database -- that is, a
                database with patient IDs in every row of every table, suitable
                for feeding into an anonymisation system like CRATE
                (https://doi.org/10.1186%2Fs12911-017-0437-1).
            db_make_all_tables_even_empty:
                create all tables, even empty ones

            include_blobs:
                include binary large objects (BLOBs) (applies to several export
                formats)

            xml_include_ancillary:
                include ancillary tables as well as the main?
            xml_include_calculated:
                include fields calculated by the task
            xml_include_comments:
                include comments in XML?
            xml_include_patient:
                include patient details?
            xml_include_plain_columns:
                include the base columns
            xml_include_snomed:
                include SNOMED-CT codes, if available?
            xml_skip_fields:
                fieldnames to skip
            xml_sort_by_name:
                sort by field/attribute names?
            xml_with_header_comments:
                include header-style comments?
        """
        self.db_patient_id_in_each_row = db_patient_id_per_row
        self.db_make_all_tables_even_empty = db_make_all_tables_even_empty
        self.db_include_summaries = db_include_summaries

        self.include_blobs = include_blobs

        self.xml_include_ancillary = xml_include_ancillary
        self.xml_include_calculated = xml_include_calculated
        self.xml_include_comments = xml_include_comments
        self.xml_include_patient = xml_include_patient
        self.xml_include_plain_columns = xml_include_plain_columns
        self.xml_include_snomed = xml_include_snomed
        self.xml_skip_fields = xml_skip_fields or []  # type: List[str]
        self.xml_sort_by_name = xml_sort_by_name
        self.xml_with_header_comments = xml_with_header_comments

    def clone(self) -> "TaskExportOptions":
        """
        Returns a copy of this object.
        """
        return copy.copy(self)
