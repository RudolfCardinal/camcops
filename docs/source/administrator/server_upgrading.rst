..  docs/source/administrator/server_upgrading.rst

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

Upgrading a CamCOPS server
==========================

This involves two steps: upgrading CamCOPS itself, then upgrading the CamCOPS
server database(s).


Upgrade the CamCOPS software
----------------------------

.. include:: include_this_is_manual_see_docker.rst

For Linux, this script will automate much of the work:

.. literalinclude:: upgrade_camcops_server.sh
    :language: bash


Upgrade the CamCOPS database
----------------------------

.. code-block:: bash

    # For example:
    sudo camcops_server upgrade_db --config /etc/camcops/camcops.conf

    # Notes:
    # - "sudo" only so that CamCOPS can read the config file
    # - Use your installation-specific config file!

For more detail, see :ref:`camcops_server <camcops_cli>`.
