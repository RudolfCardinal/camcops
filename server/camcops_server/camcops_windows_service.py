#!/usr/bin/env python

"""
camcops_server/camcops_windows_service.py

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

**Run CamCOPS and its associated back-end tools as a Windows service.**

"""

import os
import logging
import sys

from cardinal_pythonlib.winservice import (
    ProcessDetails,
    generic_service_main,
    WindowsService,
)

from camcops_server.cc_modules.cc_baseconstants import ENVVAR_CONFIG_FILE

log = logging.getLogger(__name__)

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
WINSERVICE_LOGDIR_ENVVAR = "CAMCOPS_WINSERVICE_LOGDIR"


# =============================================================================
# Windows service framework
# =============================================================================

class CamcopsWinService(WindowsService):
    """
    Windows service class for CamCOPS.
    """
    # you can NET START/STOP the service by the following name
    _svc_name_ = "CamCOPS"
    # this text shows up as the service name in the Service
    # Control Manager (SCM)
    _svc_display_name_ = "CamCOPS service"
    # this text shows up as the description in the SCM
    _svc_description_ = (
        "Runs web server, scheduler, and worker processes for CamCOPS"
    )
    # how to launch?
    _exe_name_ = sys.executable  # python.exe in the virtualenv
    _exe_args_ = f'"{os.path.realpath(__file__)}"'  # this script

    # -------------------------------------------------------------------------
    # The service
    # -------------------------------------------------------------------------

    def service(self) -> None:
        """
        Run the Windows service.

        - Reads the log directory from the environment variable
          ``CAMCOPS_WINSERVICE_LOGDIR``.
        - Checks that the environment variable ``CAMCOPS_CONFIG_FILE`` is set.
        - Launches the CherryPy web server.
        - Launches the Celery scheduler.
        - Launches the Celery workers.
        """
        # Read from environment
        try:
            logdir = os.environ[WINSERVICE_LOGDIR_ENVVAR]
        except KeyError:
            raise ValueError(
                f"Must specify {WINSERVICE_LOGDIR_ENVVAR} "
                f"system environment variable")
        if ENVVAR_CONFIG_FILE not in os.environ:
            raise ValueError(
                f"Must specify {ENVVAR_CONFIG_FILE} "
                f"system environment variable")

        # Define processes
        camcops_server = os.path.join(CURRENT_DIR, "camcops_server.py")
        weblog = os.path.join(logdir, "camcops_webserver.log")
        schedulerlog = os.path.join(logdir, "camcops_scheduler.log")
        workerlog = os.path.join(logdir, "camcops_workers.log")
        procdetails = [
            ProcessDetails(
                name="CherryPy web server",
                procargs=[
                    sys.executable,
                    camcops_server,
                    "serve_cherrypy",
                ],
                logfile_out=weblog,
                logfile_err=weblog,
            ),
            ProcessDetails(
                name="Celery scheduler",
                procargs=[
                    sys.executable,
                    camcops_server,
                    "launch_scheduler",
                ],
                logfile_out=schedulerlog,
                logfile_err=schedulerlog,
            ),
            ProcessDetails(
                name="Celery workers",
                procargs=[
                    sys.executable,
                    camcops_server,
                    "launch_workers",
                ],
                logfile_out=workerlog,
                logfile_err=workerlog,
            ),
        ]

        # Run processes
        self.run_processes(procdetails)


# =============================================================================
# Main
# =============================================================================

def main():
    """
    Command-line entry point.
    """
    # Called as an entry point (see setup.py).
    logging.basicConfig(level=logging.DEBUG)
    generic_service_main(CamcopsWinService, 'CamcopsWinService')


if __name__ == '__main__':
    main()
