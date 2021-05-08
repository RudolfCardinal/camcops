#!/usr/bin/env python

"""
camcops_server/alembic/versions/0062.py

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

distinguish_api_sessions

Revision ID: 0062
Revises: 0061
Creation date: 2021-05-01 17:01:01.492442

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0062'
down_revision = '0061'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

def upgrade():
    with op.batch_alter_table('_security_webviewer_sessions', schema=None) as batch_op:  # noqa
        batch_op.add_column(sa.Column('is_api_session', sa.Boolean(), nullable=True, comment='This session is using the client API (not a human browsing).'))  # noqa


def downgrade():
    with op.batch_alter_table('_security_webviewer_sessions', schema=None) as batch_op:  # noqa
        batch_op.drop_column('is_api_session')
