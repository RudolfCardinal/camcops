#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_baseconstants.py

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

**Constants required during package creation, which therefore can't rely on
anything except the Python standard library.**

By simple extension, also directory/filename definitions within the server
tree.

Also, for visibility, environment variable names.
"""

import os
from os import pardir
from os.path import abspath, dirname, join
import sys

# =============================================================================
# Environment variable names
# =============================================================================

ENVVAR_CONFIG_FILE = "CAMCOPS_CONFIG_FILE"  # external or internal


# =============================================================================
# Directories and filenames
# =============================================================================

_this_directory = dirname(abspath(__file__))  # cc_modules
CAMCOPS_SERVER_DIRECTORY = abspath(join(_this_directory, pardir))  # camcops_server  # noqa

if "GENERATING_CAMCOPS_DOCS" in os.environ:
    CAMCOPS_SERVER_DIRECTORY = "/path/to/camcops/server"

ALEMBIC_BASE_DIR = CAMCOPS_SERVER_DIRECTORY

DEFAULT_EXTRA_STRINGS_DIR = join(CAMCOPS_SERVER_DIRECTORY, "extra_strings")

LINUX_DEFAULT_CAMCOPS_CONFIG_DIR = "/etc/camcops"
LINUX_DEFAULT_CAMCOPS_DIR = "/usr/share/camcops"
# Lintian dislikes files/subdirectories in: /usr/bin/X, /usr/local/X, /opt/X
# It dislikes images in /usr/lib
LINUX_DEFAULT_LOCK_DIR = "/var/lock/camcops"
LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR = "/var/cache/camcops/matplotlib"
# ... Lintian dislikes using /var/local
LINUX_DEFAULT_USER_DOWNLOAD_DIR = "/var/tmp/camcops"

STATIC_ROOT_DIR = join(CAMCOPS_SERVER_DIRECTORY, "static")
# ... mostly but not entirely superseded by STATIC_CAMCOPS_PACKAGE_PATH
TEMPLATE_DIR = join(CAMCOPS_SERVER_DIRECTORY, "templates")
TRANSLATIONS_DIR = join(CAMCOPS_SERVER_DIRECTORY, "translations")


# =============================================================================
# Filenames
# =============================================================================

if hasattr(sys, 'real_prefix'):
    # We're running in a virtual environment.
    # https://stackoverflow.com/questions/1871549/python-determine-if-running-inside-virtualenv
    _venv = sys.prefix
    _venv_bin = join(_venv, 'bin')
    CAMCOPS_EXECUTABLE = join(_venv_bin, "camcops")
else:
    CAMCOPS_EXECUTABLE = "camcops"  # fallback; may not work

ALEMBIC_CONFIG_FILENAME = join(ALEMBIC_BASE_DIR, 'alembic.ini')


# =============================================================================
# Significant table names
# =============================================================================

ALEMBIC_VERSION_TABLE = "_alembic_version"


# =============================================================================
# URLs
# =============================================================================

DOCUMENTATION_URL = "https://camcops.readthedocs.io/"


# =============================================================================
# Special environment detection
# =============================================================================

# Is this program running on readthedocs.org?
ON_READTHEDOCS = os.environ.get('READTHEDOCS') == 'True'


# =============================================================================
# Exit codes
# =============================================================================

EXIT_SUCCESS = 0
EXIT_FAILURE = 1
