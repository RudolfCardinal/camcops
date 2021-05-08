#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_nlp.py

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

**Natural language processing functions (of sorts).**

"""

from typing import Dict

# =============================================================================
# Processing names
# =============================================================================

TITLES = [
    "DR",
    "PROF",
    "MR",
    "MISS",
    "MRS",
    "MS",
    "SR"
]


def guess_name_components(s: str, uppercase: bool = True) -> Dict[str, str]:
    """
    Takes a string such as 'Dr James T. Smith, M.D.' and returns parts.

    This will not be perfect! If it isn't reasonably sure, it returns
    everything in the surname field.

    Examples it will fail on:

    - Nurse Specialist Jones

    Returns:
        dict: dictionary with keys "surname", "forename", "prefix"

    """
    # Hard.
    # http://stackoverflow.com/questions/4276905/

    prefix = ""
    forename = ""

    # 1. Separate on spaces, chucking any blanks
    if s:
        parts = [p for p in s.split(" ") if p]
    else:
        parts = []

    # 2. Prefix?
    if len(parts) > 0:
        p = parts[0]
        if "." in p or p.replace(".", "").upper() in TITLES:
            prefix = p
            parts = parts[1:]

    # 3. Forename, surname
    if len(parts) == 2:
        if parts[0][-1] == ",":  # SURNAME, FORENAME
            forename = parts[1]
            surname = parts[0]
        else:  # FORENAME SURNAME
            forename = parts[0]
            surname = parts[1]
    else:  # No idea, really; shove it all in the surname component.
        surname = " ".join(parts)

    if uppercase:
        surname = surname.upper()
        forename = forename.upper()
        prefix = prefix.upper()
    return dict(
        surname=surname,
        forename=forename,
        prefix=prefix
    )
