#!/usr/bin/env python

"""
camcops_server/alembic/versions/0054_gmcpq_sex_choices.py

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

gmcpq_sex

Revision ID: 0054
Revises: 0053
Creation date: 2020-10-24 00:23:14.292034

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0054'
down_revision = '0053'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    with op.batch_alter_table('gmcpq', schema=None) as batch_op:
        batch_op.alter_column(
            'q10',
            existing_type=sa.String(length=1),
            comment='Sex of rater (M, F, X)',
            existing_comment='Sex of rater (M, F)',
            existing_nullable=True
        )


# noinspection PyPep8,PyTypeChecker
def downgrade():
    with op.batch_alter_table('gmcpq', schema=None) as batch_op:
        batch_op.alter_column(
            'q10',
            existing_type=sa.String(length=1),
            comment='Sex of rater (M, F)',
            existing_comment='Sex of rater (M, F, X)',
            existing_nullable=True
        )
