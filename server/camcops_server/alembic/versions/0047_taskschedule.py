#!/usr/bin/env python

"""
camcops_server/alembic/versions/0047_taskschedule.py

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

taskschedule

Revision ID: 0047
Revises: 0046
Creation date: 2020-03-23 15:10:13.993974

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa
import camcops_server.cc_modules.cc_sqla_coltypes


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0047'
down_revision = '0046'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    op.create_table(
        '_task_schedule',
        sa.Column('id', sa.Integer(), autoincrement=True, nullable=False, comment='Arbitrary primary key'),
        sa.Column('group_id', sa.Integer(), nullable=False, comment='FK to _security_groups.id'),
        sa.Column('name', sa.UnicodeText(), nullable=True, comment='name'),
        sa.ForeignKeyConstraint(['group_id'], ['_security_groups.id'], name=op.f('fk__task_schedule_group_id')),
        sa.PrimaryKeyConstraint('id', name=op.f('pk__task_schedule')),
        mysql_charset='utf8mb4 COLLATE utf8mb4_unicode_ci',
        mysql_engine='InnoDB',
        mysql_row_format='DYNAMIC'
    )
    op.create_table(
        '_task_schedule_item',
        sa.Column('id', sa.Integer(), autoincrement=True, nullable=False, comment='Arbitrary primary key'),
        sa.Column('schedule_id', sa.Integer(), nullable=False, comment='FK to _task_schedule.id'),
        sa.Column('task_table_name', sa.String(length=128), nullable=True, comment="Table name of the task's base table"),
        sa.Column('due_from', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDurationAsIsoTextColType(length=29), nullable=True, comment='Relative time from the start date by which the task may be started'),
        sa.Column('due_by', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDurationAsIsoTextColType(length=29), nullable=True, comment='Relative time from the start date by which the task must be completed'),
        sa.ForeignKeyConstraint(['schedule_id'], ['_task_schedule.id'], name=op.f('fk__task_schedule_item_schedule_id')),
        sa.PrimaryKeyConstraint('id', name=op.f('pk__task_schedule_item')),
        mysql_charset='utf8mb4 COLLATE utf8mb4_unicode_ci',
        mysql_engine='InnoDB',
        mysql_row_format='DYNAMIC'
    )
    with op.batch_alter_table('_task_schedule_item', schema=None) as batch_op:
        batch_op.create_index(batch_op.f('ix__task_schedule_item_task_table_name'), ['task_table_name'], unique=False)

    op.create_table(
        '_patient_task_schedule',
        sa.Column('id', sa.Integer(), autoincrement=True, nullable=False),
        sa.Column('patient_pk', sa.Integer(), nullable=True),
        sa.Column('schedule_id', sa.Integer(), nullable=True),
        sa.Column('start_datetime', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDateTimeAsIsoTextColType(length=32), nullable=True, comment='Schedule start date for the patient. Due from/within durations for a task schedule item are relative to this.'),
        sa.Column('settings',
                  camcops_server.cc_modules.cc_sqla_coltypes.JsonColType(),
                  nullable=True,
                  comment='Task-specific settings for this patient'),
        sa.ForeignKeyConstraint(['patient_pk'], ['patient._pk'], name=op.f('fk__patient_task_schedule_patient_pk'), ondelete='CASCADE'),
        sa.ForeignKeyConstraint(['schedule_id'], ['_task_schedule.id'], name=op.f('fk__patient_task_schedule_schedule_id'), ondelete='CASCADE'),
        sa.PrimaryKeyConstraint('id', name=op.f('pk__patient_task_schedule')),
        mysql_charset='utf8mb4 COLLATE utf8mb4_unicode_ci',
        mysql_engine='InnoDB',
        mysql_row_format='DYNAMIC'
    )


# noinspection PyPep8,PyTypeChecker
def downgrade():
    op.drop_table('_patient_task_schedule')
    op.drop_table('_task_schedule_item')
    op.drop_table('_task_schedule')
