#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_client_api_helpers.py

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

**Additional helper functions used by the client (tablet device) API and the
webview for special features.**

"""

from typing import Tuple

from sqlalchemy.sql.schema import Table

from camcops_server.cc_modules.cc_all_models import NONTASK_CLIENT_TABLENAMES
from camcops_server.cc_modules.cc_patient import Patient, PatientIdNum


# =============================================================================
# Table sort order
# =============================================================================

def upload_commit_order_sorter(x: Table) -> Tuple[bool, bool, bool, str]:
    """
    Function to sort tables for the commit phase of the upload.

    - "patient" must come before "patient_idnum", since ID number indexing
      looks at the associated Patient.

    - All of "patient", "blobs", and all ancillary tables must come before task
      tables, because task indexes depend on tasks correctly retrieving their
      subsidiary information to determine their
      :meth:`camcops_server.cc_modules.cc_task.Task.is_complete` status.
      (However, even though ancillary tables can refer to BLOBs, as long as
      both are complete before the task tables are examined, their relative
      order doesn't matter.)

    - Task tables come last.

    Args:
        x: an SQLAlchemy :class:`Table`

    Returns:
        a tuple suitable for use as the key to a ``sort()`` or ``sorted()``
        operation

    Note that ``False`` sorts before ``True``, and see
    https://stackoverflow.com/questions/23090664/sorting-a-list-of-string-in-python-such-that-a-specific-string-if-present-appea.
    """  # noqa
    return (x.name != Patient.__tablename__,
            x.name != PatientIdNum.__tablename__,
            x.name not in NONTASK_CLIENT_TABLENAMES,
            x.name)
