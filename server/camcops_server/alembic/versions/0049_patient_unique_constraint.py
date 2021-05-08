#!/usr/bin/env python

"""
camcops_server/alembic/versions/0049_patient_unique_constraint.py

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

patient_unique_constraint

Revision ID: 0049
Revises: 0048
Creation date: 2020-04-28 11:48:52.826510

"""

# =============================================================================
# Imports
# =============================================================================


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0049'
down_revision = '0048'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    pass
    # One might think that we want records to be unique by id [within a
    # client's database] and _device_id/_era [defining a moment on a client's
    # database]. However, when we upload, a new record is inserted, and if the
    # upload succeeds, the old record is moved to a different era. But during
    # that moment of upload, the two appear to clash. The extra constraint is
    # that only one record with the combination of id/_device_id/era may also
    # be _current. Furthermore, non-current versions show a history trail of
    # uploads from the client, and so may be non-unique even within a given
    # era.
    #
    # In other words, non-current versions don't have to have that constraint.
    #
    # To implement such a constraint...
    # - https://stackoverflow.com/questions/18293543/can-i-conditionally-enforce-a-uniqueness-constraint  # noqa
    # - ... not possible?

    # with op.batch_alter_table('patient', schema=None) as batch_op:
    #     batch_op.create_unique_constraint(
    #         batch_op.f('uq_patient_id'),
    #         ['id', '_device_id', '_era', 'XXX']
    #     )


# noinspection PyPep8,PyTypeChecker
def downgrade():
    pass
    # with op.batch_alter_table('patient', schema=None) as batch_op:
    #     batch_op.drop_constraint(batch_op.f('uq_patient_id'), type_='unique')
