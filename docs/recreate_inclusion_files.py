#!/usr/bin/env python

"""
docs/recreate_inclusion_files.py

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

**Rebuild inclusion files for documentation.**

That is, e.g. "command --help > somefile.txt".

"""

import datetime
import logging
from os import DirEntry, environ, scandir
from os.path import abspath, dirname, exists, join, pardir, realpath
import subprocess
import sys
from typing import List, Optional

from cardinal_pythonlib.logs import main_only_quicksetup_rootlogger

log = logging.getLogger(__name__)

EXIT_FAILURE = 1

THIS_DIR = dirname(realpath(__file__))

DOCS_SOURCE_DIR = join(THIS_DIR, "source")
ADMIN_DIR = join(DOCS_SOURCE_DIR, "administrator")
USER_DIR = join(DOCS_SOURCE_DIR, "user")
DEV_DIR = join(DOCS_SOURCE_DIR, "developer")

CAMCOPS_ROOT_DIR = abspath(join(THIS_DIR, pardir))  # .../camcops
TABLET_ROOT_DIR = join(CAMCOPS_ROOT_DIR, "tablet_qt")
TABLET_TOOLS_DIR = join(TABLET_ROOT_DIR, "tools")
SERVER_ROOT_DIR = join(CAMCOPS_ROOT_DIR, "server")  # .../camcops/server
SERVER_TOOLS_DIR = join(SERVER_ROOT_DIR, "tools")


def build_directories() -> DirEntry:
    with scandir(CAMCOPS_ROOT_DIR) as it:
        for entry in it:
            if entry.name.startswith("build-") and entry.is_dir():
                yield entry


def find_camcops_client_executable() -> Optional[str]:
    for entry in build_directories():
        camcops_executable = join(entry.path, "camcops")

        if exists(camcops_executable):
            return camcops_executable

    return None


CAMCOPS_CLIENT_EXECUTABLE = find_camcops_client_executable()
if CAMCOPS_CLIENT_EXECUTABLE is None:
    log.error("Cannot find a camcops executable. Have you built it?")
    sys.exit(EXIT_FAILURE)


def run_cmd(cmdargs: List[str],
            output_filename: str,
            timestamp: bool = False,
            comment_prefix: str = "# ",
            encoding: str = sys.getdefaultencoding()) -> None:
    """
    Args:
        cmdargs:
            Command to run
        output_filename:
            File to write command's output to
        timestamp:
            Add timestamp? Perhaps helpful, but means that all files will
            appear to change whenever this script is run, because their
            timestamp will change.
        comment_prefix:
            Comment prefix for this type of output file
        encoding:
            Encoding to use
    """
    log.info(f"Running: {cmdargs}")

    modified_env = environ.copy()
    modified_env["GENERATING_CAMCOPS_DOCS"] = "True"
    modified_env.pop("CAMCOPS_QT_BASE_DIR", None)
    modified_env["CAMCOPS_CONFIG_FILE"] = "/path/to/camcops/config_file.ini"

    output = (
        subprocess.check_output(cmdargs, env=modified_env).decode(encoding)
        .replace(CAMCOPS_CLIENT_EXECUTABLE,
                 "/path/to/camcops/client/executable")
    )
    log.info(f"... writing to: {output_filename}")
    with open(output_filename, "wt") as f:
        f.write(output)
        if timestamp:
            now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            f.write(f"\n{comment_prefix}Generated at {now}\n")


def main():
    # administrator
    run_cmd(["camcops_backup_mysql_database", "--help"],
            join(ADMIN_DIR, "camcops_backup_mysql_database_help.txt"))
    run_cmd(["camcops_server", "--allhelp"],
            join(ADMIN_DIR, "camcops_server_allhelp.txt"))
    run_cmd(["camcops_server_meta", "--help"],
            join(ADMIN_DIR, "camcops_server_meta_help.txt"))
    log.warning("Skipping camcops_windows_service_help.txt (requires Windows)")
    run_cmd(["camcops_server", "demo_camcops_config"],
            join(ADMIN_DIR, "demo_camcops_config.ini"))
    run_cmd(["camcops_server", "demo_supervisor_config"],
            join(ADMIN_DIR, "demo_supervisor_config.txt"))
    run_cmd(["camcops_server", "demo_apache_config"],
            join(ADMIN_DIR, "demo_apache_config.txt"))
    run_cmd(["camcops_fetch_snomed_codes", "--allhelp"],
            join(ADMIN_DIR, "camcops_fetch_snomed_codes_help.txt"))
    # developer
    run_cmd(["python", join(TABLET_TOOLS_DIR, "build_qt.py"), "--help"],
            join(DEV_DIR, "build_qt_help.txt"))
    run_cmd(["python", join(SERVER_TOOLS_DIR, "build_translations.py"),
             "--help"],
            join(DEV_DIR, "build_translations_help.txt"))
    run_cmd(["python", join(SERVER_TOOLS_DIR, "create_database_migration.py"),
             "--help"],
            join(DEV_DIR, "create_database_migration_help.txt"))
    run_cmd(["python", join(TABLET_TOOLS_DIR, "decrypt_sqlcipher.py"),
             "--help"],
            join(DEV_DIR, "decrypt_sqlcipher_help.txt"))
    run_cmd(["python", join(TABLET_TOOLS_DIR, "encrypt_sqlcipher.py"),
             "--help"],
            join(DEV_DIR, "encrypt_sqlcipher_help.txt"))
    run_cmd(["python", join(SERVER_TOOLS_DIR, "make_xml_skeleton.py"),
             "--help"],
            join(DEV_DIR, "make_xml_skeleton_help.txt"))
    run_cmd(["python", join(TABLET_TOOLS_DIR, "open_sqlcipher.py"),
             "--help"],
            join(DEV_DIR, "open_sqlcipher_help.txt"))
    # user
    camcops_client_executable = find_camcops_client_executable()
    run_cmd([camcops_client_executable, "--help"],
            join(USER_DIR, "camcops_client_help.txt"))

    log.info("Done.")


if __name__ == "__main__":
    main_only_quicksetup_rootlogger()
    main()
