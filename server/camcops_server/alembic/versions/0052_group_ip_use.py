#!/usr/bin/env python

"""
camcops_server/alembic/versions/0052_group_ip_use.py

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

group_ip_use

Revision ID: 0052
Revises: 0051
Creation date: 2020-09-10 10:41:39.432573

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0052'
down_revision = '0051'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    op.create_table(
        '_security_ip_use',
        sa.Column('id', sa.Integer(), autoincrement=True, nullable=False,
                  comment='IP Use ID'),
        sa.Column('commercial', sa.Boolean(), nullable=False,
                  comment='Applicable to a commercial context'),
        sa.Column('clinical', sa.Boolean(), nullable=False,
                  comment='Applicable to a clinical context'),
        sa.Column('educational', sa.Boolean(), nullable=False,
                  comment='Applicable to an educational context'),
        sa.Column('research', sa.Boolean(), nullable=False,
                  comment='Applicable to a research context'),
        sa.PrimaryKeyConstraint('id', name=op.f('pk__security_ip_use')),
        mysql_charset='utf8mb4 COLLATE utf8mb4_unicode_ci',
        mysql_engine='InnoDB',
        mysql_row_format='DYNAMIC'
    )
    with op.batch_alter_table('_security_ip_use', schema=None) as batch_op:
        batch_op.create_index(batch_op.f('ix__security_ip_use_id'), ['id'],
                              unique=False)

    with op.batch_alter_table('_security_groups', schema=None) as batch_op:
        batch_op.add_column(sa.Column('ip_use_id', sa.Integer(), nullable=True,
                                      comment='FK to _security_ip_use.id'))
        batch_op.create_foreign_key(batch_op.f('fk__security_groups_ip_use_id'),
                                    '_security_ip_use', ['ip_use_id'], ['id'])


# noinspection PyPep8,PyTypeChecker
def downgrade():
    with op.batch_alter_table('_security_groups', schema=None) as batch_op:
        batch_op.drop_constraint(batch_op.f('fk__security_groups_ip_use_id'),
                                 type_='foreignkey')
        batch_op.drop_column('ip_use_id')

    op.drop_table('_security_ip_use')
