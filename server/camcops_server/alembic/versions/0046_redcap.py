#!/usr/bin/env python

"""
camcops_server/alembic/versions/0046_redcap.py

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

redcap

Revision ID: 0046
Revises: 0045
Creation date: 2019-12-10 17:25:58.604235

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0046'
down_revision = '0045'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    with op.batch_alter_table('_export_recipients', schema=None) as batch_op:
        batch_op.add_column(sa.Column('redcap_api_url', sa.Text(), nullable=True, comment='(REDCap) REDCap API URL, pointing to the REDCap server'))
        batch_op.add_column(sa.Column('redcap_fieldmap_filename', sa.Text(), nullable=True, comment='(REDCap) File defining CamCOPS-to-REDCap field mapping'))

    op.create_table(
        '_exported_task_redcap',
        sa.Column('id', sa.Integer(), autoincrement=True, nullable=False, comment='Arbitrary primary key'),
        sa.Column('exported_task_id', sa.BigInteger(), nullable=False, comment='FK to _exported_tasks.id'),
        sa.Column('redcap_record_id', sa.UnicodeText(), nullable=True, comment='ID of the (patient) record on the REDCap instance where this task has been exported'),
        sa.Column('redcap_instrument_name', sa.UnicodeText(), nullable=True, comment='The name of the REDCap instrument name (form) where this task has been exported'),
        sa.Column('redcap_instance_id', sa.Integer(), nullable=True, comment='1-based index of this particular task within the patient record. Increments on every repeat attempt.'),
        sa.ForeignKeyConstraint(['exported_task_id'], ['_exported_tasks.id'], name=op.f('fk__exported_task_redcap_exported_task_id')),
        sa.PrimaryKeyConstraint('id', name=op.f('pk__exported_task_redcap')),
        mysql_charset='utf8mb4 COLLATE utf8mb4_unicode_ci',
        mysql_engine='InnoDB',
        mysql_row_format='DYNAMIC'
    )


# noinspection PyPep8,PyTypeChecker
def downgrade():
    with op.batch_alter_table('_export_recipients', schema=None) as batch_op:
        batch_op.drop_column('redcap_fieldmap_filename')
        batch_op.drop_column('redcap_api_url')

    op.drop_table('_exported_task_redcap')
