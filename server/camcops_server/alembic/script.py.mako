#!/usr/bin/env python

"""
camcops_server/alembic/versions/${ up_revision }.py

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

DATABASE REVISION SCRIPT

${ message }

Revision ID: ${ up_revision }
Revises: ${ down_revision | comma,n }
Creation date: ${ create_date }

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa
import cardinal_pythonlib.sqlalchemy.list_types
import camcops_server.cc_modules.cc_sqla_coltypes


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = ${ repr(up_revision) }
down_revision = ${ repr(down_revision) }
branch_labels = ${ repr(branch_labels) }
depends_on = ${ repr(depends_on) }


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    ${ upgrades if upgrades else "pass" }


# noinspection PyPep8,PyTypeChecker
def downgrade():
    ${ downgrades if downgrades else "pass" }
