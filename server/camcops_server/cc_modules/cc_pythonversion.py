#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_pythonversion.py

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

**Single place to determine the Python version required.**

Currently that is v3.6 (as of CamCOPS v2.3.1, 2019). That enables:

- f-strings (v3.6)

but not:

- dataclasses (v3.7)

"""

import sys

MINIMUM_PYTHON_VERSION = (3, 6)


def assert_minimum_python_version():
    """
    Asserts that this version of Python meets our minimum requirements.
    This function should be used except in installation environments where
    CamCOPS modules are unavailable.

    Raises:
        AssertionError

    Note that this module/function should use only Python 2 syntax!

    """
    if sys.version_info < MINIMUM_PYTHON_VERSION:
        required = ".".join(str(x) for x in MINIMUM_PYTHON_VERSION)
        actual = ".".join(str(x) for x in sys.version_info)
        raise AssertionError("Need Python %s or higher; this is %s" %
                             (required, actual))


if __name__ == "__main__":
    assert_minimum_python_version()
