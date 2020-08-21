.. camcops/docs/source/administrator/windows_service.rst

..  Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).
    .
    This file is part of CamCOPS.
    .
    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    .
    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    .
    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

.. _Celery: http://www.celeryproject.org/
.. _CherryPy: https://cherrypy.org/


.. _windows_service:

(MANUAL) CamCOPS Windows service
================================

.. include:: include_this_is_manual_see_docker.rst

The most convenient way of running a CamCOPS server under Windows is via a
service.

The service starts the following:

- the CamCOPS web server, via CherryPy_ (equivalent to running ``camcops_server
  serve_cherrypy``);

- the CamCOPS scheduler, via Celery_ (equivalent to running ``camcops_server
  launch_scheduler``);

- the CamCOPS back-end worker processes, via Celery_ (equivalent to running
  ``camcops_server launch_workers``).

To create a Windows service for CamCOPS, use the ``camcops_windows_service``
command. You will need to run it from a command prompt with Administrator
authority.

Before starting the server, ensure you have set the following two system
environment variables:

- ``CAMCOPS_WINSERVICE_LOGDIR``, determining where disk logs are stored;
- ``CAMCOPS_CONFIG_FILE``, governing which config file will be used.

Logs from CamCOPS go to files in the directory specified by the environment
variable. However, output from the service itself goes to the Windows operating
system logs: see :menuselection:`Event Viewer --> Windows Logs -->
Application`.


camcops_windows_service
-----------------------

Options as of 2019-04-23:

..  literalinclude:: camcops_windows_service_help.txt
    :language: none
