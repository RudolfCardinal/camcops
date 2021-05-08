#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_exception.py

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

**Exception-handling functions.**

"""

import logging
from typing import NoReturn

from cardinal_pythonlib.logs import BraceStyleAdapter

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Exception constants
# =============================================================================

STR_FORMAT_EXCEPTIONS = (
    # Exceptions that can be raised by str.format()
    IndexError,  # missing positional parameter: "{}, {}".format(1)
    KeyError,  # missing named parameter: "{x}".format(y=2)
    ValueError  # e.g. unmatched brace: "{x".format(x=1)
)


# =============================================================================
# Exception functions
# =============================================================================

def raise_runtime_error(msg: str) -> NoReturn:
    """
    Reports an error message to the Python log and raises a
    :exc:`RuntimeError`.
    """
    log.critical(msg)
    raise RuntimeError(msg)
