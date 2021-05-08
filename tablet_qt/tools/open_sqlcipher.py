#!/usr/bin/env python

"""
tools/open_sqlcipher.py

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

Tool to open an encrypted database via the SQLCipher command line tool.
"""

import argparse
import getpass
import logging
import os
import shutil
import sys

from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)
import pexpect

log = BraceStyleAdapter(logging.getLogger(__name__))

EXIT_FAIL = 1
PASSWORD_ENV_VAR = "DECRYPT_SQLCIPHER_PASSWORD"
SQLCIPHER_ENV_VAR = "SQLCIPHER"
SQLCIPHER_DEFAULT = "sqlcipher"


def string_to_sql_literal(s: str) -> str:
    return "'{}'".format(s.replace("'", "''"))


def main() -> None:
    # -------------------------------------------------------------------------
    # Logging
    # -------------------------------------------------------------------------
    logging.basicConfig(level=logging.DEBUG)

    # -------------------------------------------------------------------------
    # Command-line arguments
    # -------------------------------------------------------------------------
    parser = argparse.ArgumentParser(
        description="Open an encrypted database at the SQLCipher command line",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "encrypted",
        help="Filename of the existing encrypted database")
    parser.add_argument(
        "--password", type=str, default=None,
        help="Password (if blank, environment variable {} will be used, or you"
             " will be prompted)".format(PASSWORD_ENV_VAR))
    parser.add_argument(
        "--sqlcipher", type=str, default=None,
        help=(
            "SQLCipher executable file (if blank, environment variable {} "
            "will be used, or the default of {})"
        ).format(SQLCIPHER_ENV_VAR, repr(SQLCIPHER_DEFAULT)))
    parser.add_argument(
        "--cipher_compatibility", type=int, default=None,
        help=(
            "Use compatibility settings for this major version of SQLCipher "
            "(e.g. 3)"
        )
    )
    parser.add_argument(
        "--cipher_migrate", action="store_true",
        help="Migrate the database to the latest version of SQLCipher"
    )
    parser.add_argument(
        "--encoding", type=str, default=sys.getdefaultencoding(),
        help="Encoding to use")
    progargs = parser.parse_args()
    # log.debug("Args: " + repr(progargs))

    assert (
       not (progargs.cipher_migrate and
            progargs.cipher_compatibility is not None)
    ), "Can't specify both --cipher_migrate and --cipher_compatibility"

    # -------------------------------------------------------------------------
    # SQLCipher executable
    # -------------------------------------------------------------------------
    sqlcipher = (progargs.sqlcipher or os.environ.get(SQLCIPHER_ENV_VAR) or
                 SQLCIPHER_DEFAULT)

    # -------------------------------------------------------------------------
    # Check file existence
    # -------------------------------------------------------------------------
    if not os.path.isfile(progargs.encrypted):
        log.critical("No such file: {!r}", progargs.encrypted)
        sys.exit(EXIT_FAIL)
    if not shutil.which(sqlcipher):
        log.critical("Can't find SQLCipher at {!r}", sqlcipher)
        sys.exit(EXIT_FAIL)

    # -------------------------------------------------------------------------
    # Password
    # -------------------------------------------------------------------------
    log.warning("Password will be visible on SQLCipher command line")
    password = progargs.password
    if password:
        log.debug("Using password from command-line arguments (NB danger: "
                  "visible to ps and similar tools)")
    elif PASSWORD_ENV_VAR in os.environ:
        password = os.environ[PASSWORD_ENV_VAR]
        log.debug("Using password from environment variable {}",
                  PASSWORD_ENV_VAR)
    else:
        log.info("Password not on command-line or in environment variable {};"
                 " please enter it manually.", PASSWORD_ENV_VAR)
        password = getpass.getpass()

    # -------------------------------------------------------------------------
    # Run SQLCipher to do the work
    # -------------------------------------------------------------------------
    sql_commands = [
        '-- Note that ".bail" does nothing in interactive mode.',
        "-- Show SQLCipher executable version",
        "PRAGMA cipher_version;",
        "-- Gain access to the encrypted database",
        "PRAGMA key = {key};".format(key=string_to_sql_literal(password)),
    ]
    if progargs.cipher_compatibility is not None:
        sql_commands += [
            "-- Set compatibility to a specific version of SQLCipher",
            "PRAGMA cipher_compatibility = {};".format(
                progargs.cipher_compatibility),
        ]
    if progargs.cipher_migrate:
        # This command takes several seconds (e.g. 3.4 s) if it has to do work,
        # but it's fairly quick, e.g. 260 ms, if it doesn't.
        timestring = "%Y-%m-%d %H:%M:%f"
        sql_commands += [
            "-- Migrate from a previous version of SQLCipher",
            "SELECT strftime('{}', 'now');".format(timestring),
            "PRAGMA cipher_migrate;",
            "SELECT strftime('{}', 'now');".format(timestring),
        ]
    sql_commands += [
        "-- Check we can read from old database "
        "(or quit without creating new one)",
        "SELECT COUNT(*) FROM sqlite_master;",
        "-- If there was an error, the password was wrong.",
        '-- If no error: access achieved! Try e.g. ".tables" to list tables.',
    ]
    log.info("Calling SQLCipher ({!r})...", sqlcipher)
    child = pexpect.spawn(sqlcipher, [progargs.encrypted])
    log.debug("Spawned")
    for line in sql_commands:
        child.sendline(line)
    child.interact()
    log.debug("Done")


if __name__ == '__main__':
    main_only_quicksetup_rootlogger(level=logging.DEBUG)
    main()
