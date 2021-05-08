#!/usr/bin/env python

"""
camcops_server/alembic/versions/0057_mojo_medication_to_brand_name.py

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

mojo_medication_to_brand_name

Revision ID: 0057
Revises: 0056
Creation date: 2020-11-17 17:16:16.530685

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa

# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0057'
down_revision = '0056'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    with op.batch_alter_table('khandaker_mojo_medication_item', schema=None) as batch_op:
        batch_op.alter_column(
            'medication_name',
            existing_type=sa.UnicodeText(),
            new_column_name='brand_name',
            existing_comment='Medication name',
            comment='Brand name'
        )


# noinspection PyPep8,PyTypeChecker
def downgrade():
    with op.batch_alter_table('khandaker_mojo_medication_item', schema=None) as batch_op:
        batch_op.alter_column(
            'brand_name',
            existing_type=sa.UnicodeText(),
            new_column_name='medication_name',
            existing_comment='Brand name',
            comment='Medication name'
        )
