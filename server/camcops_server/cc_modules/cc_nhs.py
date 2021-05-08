#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_nhs.py

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

**NHS constants.**

"""

from typing import Dict, Optional, TYPE_CHECKING

from camcops_server.cc_modules.cc_string import AS

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest


# =============================================================================
# Permitted values in fields and corresponding dictionaries
# =============================================================================

# Do not use wappstring in the module-level code; the strings file is only
# initialized later. However, PV* fields are used at table creation.

PV_NHS_MARITAL_STATUS = ['S', 'M', 'D', 'W', 'P', 'N']
PV_NHS_ETHNIC_CATEGORY = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K',
                          'L', 'M', 'N', 'P', 'R', 'S', 'Z']


def get_nhs_dd_person_marital_status(
        req: "CamcopsRequest") -> Dict[Optional[str], Optional[str]]:
    """
    Returns a dictionary mapping NHS marital status codes to descriptive
    strings.
    """
    return {
        None: None,
        "S": req.wappstring(AS.NHS_PERSON_MARITAL_STATUS_CODE_S),
        "M": req.wappstring(AS.NHS_PERSON_MARITAL_STATUS_CODE_M),
        "D": req.wappstring(AS.NHS_PERSON_MARITAL_STATUS_CODE_D),
        "W": req.wappstring(AS.NHS_PERSON_MARITAL_STATUS_CODE_W),
        "P": req.wappstring(AS.NHS_PERSON_MARITAL_STATUS_CODE_P),
        "N": req.wappstring(AS.NHS_PERSON_MARITAL_STATUS_CODE_N),
    }


def get_nhs_dd_ethnic_category_code(
        req: "CamcopsRequest") -> Dict[Optional[str], Optional[str]]:
    """
    Returns a dictionary mapping NHS ethnicity codes to descriptive
    strings.
    """
    return {
        None: None,
        "A": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_A),
        "B": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_B),
        "C": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_C),
        "D": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_D),
        "E": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_E),
        "F": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_F),
        "G": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_G),
        "H": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_H),
        "J": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_J),
        "K": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_K),
        "L": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_L),
        "M": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_M),
        "N": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_N),
        "P": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_P),
        "R": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_R),
        "S": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_S),
        "Z": req.wappstring(AS.NHS_ETHNIC_CATEGORY_CODE_Z),
    }
