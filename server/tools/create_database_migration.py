#!/usr/bin/env python

"""
tools/create_database_migration.py

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

**Creates an Alembic database migration for CamCOPS.**

For developer use only.

"""

from argparse import ArgumentParser, RawDescriptionHelpFormatter
import logging
from os import environ, pardir
from os.path import abspath, dirname, join
import textwrap

from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)
from cardinal_pythonlib.sqlalchemy.alembic_func import create_database_migration_numbered_style  # noqa

from camcops_server.cc_modules.cc_baseconstants import ENVVAR_CONFIG_FILE
from camcops_server.cc_modules.cc_config import get_default_config_from_os_env

log = BraceStyleAdapter(logging.getLogger(__name__))

N_SEQUENCE_CHARS = 4  # like Django
CURRENT_DIR = dirname(abspath(__file__))  # camcops/server/tools
SERVER_BASE_DIR = abspath(join(CURRENT_DIR, pardir))  # camcops/server
SERVER_PACKAGE_DIR = join(SERVER_BASE_DIR, "camcops_server")  # camcops/server/camcops_server  # noqa
ALEMBIC_INI_FILE = join(SERVER_PACKAGE_DIR, "alembic.ini")
ALEMBIC_VERSIONS_DIR = join(SERVER_PACKAGE_DIR, 'alembic', 'versions')


def main() -> None:
    """
    Creates an Alembic database migration for CamCOPS, by comparing the
    metadata (from Python) with the current database.

    Note special difficulty with "variant" types, such as
    ``Integer().with_variant(...)`` which are (2017-08-21, alembic==0.9.4)
    rendered as ``sa.Variant()`` only with a MySQL backend.

    - https://bitbucket.org/zzzeek/alembic/issues/433/variant-base-not-taken-into-account-when
    - https://bitbucket.org/zzzeek/alembic/issues/131/create-special-rendering-for-variant
    
    We deal with these via
    :func:`camcops_server.alembic.env.process_revision_directives` in
    ``env.py``.
    """  # noqa
    desc = (
        "Create database revision. Note:\n"
        "- The config used will be that from the environment variable {} "
        "(currently {!r}).\n"
        "- Alembic compares (a) the current state of the DATABASE to (b) "
        "the state of the SQLAlchemy metadata (i.e. the CODE). It creates a "
        "migration file to change the database to match the code.\n"
        "- Accordingly, in the rare event of wanting to do a fresh start, you "
        "need an *empty* database.\n"
        "- More commonly, you want a database that is synced to a specific "
        "Alembic version (with the correct structure, and the correct version "
        "in the alembic_version table). If you have made manual changes, such "
        "that the actual database structure doesn't match the structure that "
        "Alembic expects based on that version, there's likely to be "
        "trouble.\n".format(
            ENVVAR_CONFIG_FILE,
            environ.get(ENVVAR_CONFIG_FILE, None)
        )
    )
    wrapped = "\n\n".join(
        textwrap.fill(x, width=79, initial_indent='', subsequent_indent='  ')
        for x in desc.splitlines()
    )
    # noinspection PyTypeChecker
    parser = ArgumentParser(description=wrapped,
                            formatter_class=RawDescriptionHelpFormatter)
    parser.add_argument(
        "message",
        help="Revision message"
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Be verbose"
    )
    args = parser.parse_args()
    main_only_quicksetup_rootlogger(level=logging.DEBUG if args.verbose
                                    else logging.INFO)
    # ... hmpf; ignored (always debug); possible Alembic forces this.

    # Check the existing database version is OK.
    config = get_default_config_from_os_env()
    config.assert_database_ok()

    # Then, if OK, create an upgrade.
    create_database_migration_numbered_style(
        alembic_ini_file=ALEMBIC_INI_FILE,
        alembic_versions_dir=ALEMBIC_VERSIONS_DIR,
        message=args.message,
        n_sequence_chars=N_SEQUENCE_CHARS
    )
    print(r"""
Now:

- Check the new migration file.
- Check in particular for incorrectly dialect-specific stuff, e.g. with

      grep "mysql\." *.py

  ... should only show "sa.SOMETYPE().with_variant(mysql.MYSQLTYPE..."
  
- and

      grep "sa\.Variant" *.py
      
  ... suggests an error that should be "Sometype().with_variant(...)"; see 
  source here.

    """)


if __name__ == "__main__":
    main()
