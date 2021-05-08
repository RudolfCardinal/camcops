#!/usr/bin/env python

"""
camcops_server/alembic/versions/0059_basdai_comments.py

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

basdai_comments

Revision ID: 0059
Revises: 0058
Creation date: 2021-03-26 10:46:08.370069

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa

# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0059'
down_revision = '0058'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    with op.batch_alter_table('basdai', schema=None) as batch_op:
        batch_op.alter_column(
            'q1',
            existing_type=sa.Float(),
            comment='Q1 - fatigue/tiredness 0-10 (none - very severe)',
            existing_comment='Q1 - fatigue/tiredness 0-10 (None - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q2',
            existing_type=sa.Float(),
            comment='Q2 - AS neck, back, hip pain 0-10 (none - very severe)',
            existing_comment='Q2 - AS neck, back, hip pain 0-10 (None - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q3',
            existing_type=sa.Float(),
            comment='Q3 - other joint pain/swelling 0-10 (none - very severe)',
            existing_comment='Q3 - other joint pain/swelling 0-10 (None - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q4',
            existing_type=sa.Float(),
            comment='Q4 - discomfort from tender areas 0-10 (none - very severe)',
            existing_comment='Q4 - discomfort from tender areas 0-10 (None - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q5',
            existing_type=sa.Float(),
            comment='Q5 - morning stiffness level 0-10 (none - very severe)',
            existing_comment='Q5 - morning stiffness level 0-10 (None - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q6',
            existing_type=sa.Float(),
            comment='Q6 - morning stiffness duration 0-10 (none - 2 or more hours)',
            existing_comment='Q6 - morning stiffness duration 0-10 (None - 2 or more hours)',
            existing_nullable=True
        )


# noinspection PyPep8,PyTypeChecker
def downgrade():
    with op.batch_alter_table('basdai', schema=None) as batch_op:
        batch_op.alter_column(
            'q6',
            existing_type=sa.Float(),
            comment='Q6 - morning stiffness duration 0-10 (None - 2 or more hours)',
            existing_comment='Q6 - morning stiffness duration 0-10 (none - 2 or more hours)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q5',
            existing_type=sa.Float(),
            comment='Q5 - morning stiffness level 0-10 (None - very severe)',
            existing_comment='Q5 - morning stiffness level 0-10 (none - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q4',
            existing_type=sa.Float(),
            comment='Q4 - discomfort from tender areas 0-10 (None - very severe)',
            existing_comment='Q4 - discomfort from tender areas 0-10 (none - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q3',
            existing_type=sa.Float(),
            comment='Q3 - other joint pain/swelling 0-10 (None - very severe)',
            existing_comment='Q3 - other joint pain/swelling 0-10 (none - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q2',
            existing_type=sa.Float(),
            comment='Q2 - AS neck, back, hip pain 0-10 (None - very severe)',
            existing_comment='Q2 - AS neck, back, hip pain 0-10 (none - very severe)',
            existing_nullable=True
        )
        batch_op.alter_column(
            'q1',
            existing_type=sa.Float(),
            comment='Q1 - fatigue/tiredness 0-10 (None - very severe)',
            existing_comment='Q1 - fatigue/tiredness 0-10 (none - very severe)',
            existing_nullable=True
        )
