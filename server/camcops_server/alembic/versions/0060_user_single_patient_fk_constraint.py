#!/usr/bin/env python

"""
camcops_server/alembic/versions/0060_user_single_patient_fk_constraint.py

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

user_single_patient_fk_constraint

Revision ID: 0060
Revises: 0059
Creation date: 2021-03-26 11:01:30.418048

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0060'
down_revision = '0059'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    with op.batch_alter_table('_security_users', schema=None) as batch_op:
        batch_op.drop_constraint('fk__security_users_single_patient_pk',
                                 type_='foreignkey')
        batch_op.create_foreign_key(
            batch_op.f('fk__security_users_single_patient_pk'),
            'patient',
            ['single_patient_pk'],
            ['_pk'],
            ondelete='SET NULL'
        )


# noinspection PyPep8,PyTypeChecker
def downgrade():
    with op.batch_alter_table('_security_users', schema=None) as batch_op:
        batch_op.drop_constraint(
            batch_op.f('fk__security_users_single_patient_pk'),
            type_='foreignkey'
        )
        batch_op.create_foreign_key(
            'fk__security_users_single_patient_pk',
            'patient',
            ['single_patient_pk'],
            ['_pk']
        )
