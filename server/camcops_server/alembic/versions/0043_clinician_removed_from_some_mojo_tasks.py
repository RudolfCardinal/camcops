#!/usr/bin/env python

"""
camcops_server/alembic/versions/0043_clinician_removed_from_some_mojo_tasks.py

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

Clinician removed from ASDAS, CHIT, ESSPRI, MFI20, SFMPQ2, SUPPSP

Revision ID: 0043
Revises: 0042
Creation date: 2019-10-22 10:35:03.748385

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
from alembic.operations.base import BatchOperations
import sqlalchemy as sa


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0043'
down_revision = '0042'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade() -> None:
    def drop_clinician(batch_op_: BatchOperations) -> None:
        batch_op_.drop_column('clinician_specialty')
        batch_op_.drop_column('clinician_professional_registration')
        batch_op_.drop_column('clinician_service')
        batch_op_.drop_column('clinician_contact_details')
        batch_op_.drop_column('clinician_post')
        batch_op_.drop_column('clinician_name')

    with op.batch_alter_table('asdas', schema=None) as batch_op:
        drop_clinician(batch_op)

    with op.batch_alter_table('chit', schema=None) as batch_op:
        drop_clinician(batch_op)

    with op.batch_alter_table('esspri', schema=None) as batch_op:
        drop_clinician(batch_op)

    with op.batch_alter_table('mfi20', schema=None) as batch_op:
        drop_clinician(batch_op)

    with op.batch_alter_table('sfmpq2', schema=None) as batch_op:
        drop_clinician(batch_op)

    with op.batch_alter_table('suppsp', schema=None) as batch_op:
        drop_clinician(batch_op)


# noinspection PyPep8,PyTypeChecker
def downgrade() -> None:
    # noinspection PyPep8
    def add_clinician(batch_op_: BatchOperations) -> None:
        batch_op_.add_column(sa.Column('clinician_name', sa.Text(), nullable=True, comment="(CLINICIAN) Clinician's name (e.g. Dr X)"))
        batch_op_.add_column(sa.Column('clinician_post', sa.Text(), nullable=True, comment="(CLINICIAN) Clinician's post (e.g. Consultant)"))
        batch_op_.add_column(sa.Column('clinician_contact_details', sa.Text(), nullable=True, comment="(CLINICIAN) Clinician's contact details (e.g. bleep, extension)"))
        batch_op_.add_column(sa.Column('clinician_service', sa.Text(), nullable=True, comment="(CLINICIAN) Clinician's service (e.g. Liaison Psychiatry Service)"))
        batch_op_.add_column(sa.Column('clinician_professional_registration', sa.Text(), nullable=True, comment="(CLINICIAN) Clinician's professional registration (e.g. GMC# 12345)"))
        batch_op_.add_column(sa.Column('clinician_specialty', sa.Text(), nullable=True, comment="(CLINICIAN) Clinician's specialty (e.g. Liaison Psychiatry)"))

    with op.batch_alter_table('suppsp', schema=None) as batch_op:
        add_clinician(batch_op)

    with op.batch_alter_table('sfmpq2', schema=None) as batch_op:
        add_clinician(batch_op)

    with op.batch_alter_table('mfi20', schema=None) as batch_op:
        add_clinician(batch_op)

    with op.batch_alter_table('esspri', schema=None) as batch_op:
        add_clinician(batch_op)

    with op.batch_alter_table('chit', schema=None) as batch_op:
        add_clinician(batch_op)

    with op.batch_alter_table('asdas', schema=None) as batch_op:
        add_clinician(batch_op)
