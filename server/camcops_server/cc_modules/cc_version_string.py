#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_version_string.py

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

**Contains CamCOPS version strings.**

"""

# This file MAY NOT import anything except the standard library, because it
# is read by setup.py, which must operate in a base Python environment.

# =============================================================================
# Version constants and configuration variables read by shell scripts
# =============================================================================

# -----------------------------------------------------------------------------
# CamCOPS server version/date
# -----------------------------------------------------------------------------
# When you alter the server, it is normal to change these two:

CAMCOPS_SERVER_VERSION_STRING = "2.3.7"
CAMCOPS_SERVER_CHANGEDATE = "2020-03-03"

# +++ NOW ALSO UPDATE: +++
#
#       documentation/source/changelog.rst


# -----------------------------------------------------------------------------
# Minimum tablet version permitted to upload
# -----------------------------------------------------------------------------
# BEWARE: it is not normal to have to change MINIMUM_TABLET_VERSION_STRING.
# If you increase it, you may prevent old clients from uploading.

MINIMUM_TABLET_VERSION_STRING = "1.14.0"
