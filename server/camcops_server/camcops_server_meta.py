#!/usr/bin/env python

"""
camcops_server/camcops_server_meta.py

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

**The CamCOPS "meta" launcher: so you can run a CamCOPS command over multiple
config files (installations).**

This has been largely superseded by the "groups" concept, since a single
CamCOPS instance is now capable of much more.

"""

import argparse
import glob
import logging
import os
import sys

from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)

log = BraceStyleAdapter(logging.getLogger(__name__))

THIS_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
PROJECT_BASE_DIRECTORY = os.path.abspath(os.path.join(THIS_DIRECTORY,
                                                      os.pardir))
PYTHON_EXECUTABLE = sys.executable

if "GENERATING_CAMCOPS_DOCS" in os.environ:
    PROJECT_BASE_DIRECTORY = "/path/to/camcops/server"
    PYTHON_EXECUTABLE = "/path/to/python"

DEFAULT_CAMCOPS = os.path.join(PROJECT_BASE_DIRECTORY, 'camcops_server.py')


def meta_main() -> None:
    """
    Command-line process for ``camcops_server_meta`` tool.
    """
    parser = argparse.ArgumentParser(
        description="Run commands across multiple CamCOPS databases")
    parser.add_argument(
        'cc_command', type=str,
        help="Main command to pass to CamCOPS"
    )
    parser.add_argument(
        '--filespecs', nargs='+', required=True,
        help="List of CamCOPS config files (wildcards OK)")
    parser.add_argument(
        '--ccargs', nargs='*',
        help="List of CamCOPS arguments, to which '--' will be prefixed")
    parser.add_argument(
        '--python', default=PYTHON_EXECUTABLE,
        help=f"Python interpreter (default: {PYTHON_EXECUTABLE})")
    parser.add_argument(
        '--camcops', default=DEFAULT_CAMCOPS,
        help=f"CamCOPS server executable (default: {DEFAULT_CAMCOPS})")
    parser.add_argument(
        '-d', '--dummyrun', action="store_true",
        help="Dummy run (show filenames only)")
    parser.add_argument('-v', '--verbose', action="store_true", help="Verbose")
    args = parser.parse_args()
    main_only_quicksetup_rootlogger(level=logging.DEBUG if args.verbose
                                    else logging.INFO)
    log.debug("Arguments: {}", args)

    # Delayed import so --help doesn't take ages
    from camcops_server.camcops_server import main as camcops_main  # delayed import  # noqa

    did_something = False
    # old_sys_argv = sys.argv.copy()
    for filespec in args.filespecs:
        for filename in glob.glob(filespec):
            did_something = True
            log.info("Processing: {}", filename)
            sys.argv = (
                ['camcops_server',  # dummy argv[0]
                 args.cc_command,
                 "--config", filename] +
                [f'--{x}' for x in args.ccargs or []]
            )
            log.debug("Executing command: {}", sys.argv)
            if args.dummyrun:
                continue
            camcops_main()  # using the new sys.argv
    if not did_something:
        log.info("Nothing to do; no files found")


if __name__ == '__main__':
    meta_main()
