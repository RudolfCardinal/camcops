#!/usr/bin/env python

"""
camcops_server/alembic/versions/0022_index_exported_tasks_start_at.py

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

Add an index missed somehow from revision 0014.

Revision ID: 0022
Revises: 0021
Creation date: 2019-04-23 19:38:06.769369

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0022'
down_revision = '0021'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    with op.batch_alter_table('_exported_tasks', schema=None) as batch_op:
        batch_op.create_index(batch_op.f('ix__exported_tasks_start_at_utc'), ['start_at_utc'], unique=False)


# noinspection PyPep8,PyTypeChecker
def downgrade():
    with op.batch_alter_table('_exported_tasks', schema=None) as batch_op:
        batch_op.drop_index(batch_op.f('ix__exported_tasks_start_at_utc'))
