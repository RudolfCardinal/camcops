#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_password.py

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

**Password-related functions.**

"""

from camcops_server.cc_modules.cc_baseconstants import PROHIBITED_PASSWORDS_FILE  # noqa


def password_prohibited(password: str) -> bool:
    """
    Checks a (cleartext) password and decides if it is prohibited by virtue
    of being in the UK National Cyber Security Centre (NCSC) list of common,
    hacked passwords
    (https://www.ncsc.gov.uk/blog-post/passwords-passwords-everywhere) --
    ultimately from https://haveibeenpwned.com/.

    Speed is not critical; we don't cache the file, for example.
    """
    with open(PROHIBITED_PASSWORDS_FILE) as f:
        for line in f:
            # It doesn't matter if we check against the comment lines.
            if password == line.rstrip():  # remove trailing newline etc.
                return True
        return False
