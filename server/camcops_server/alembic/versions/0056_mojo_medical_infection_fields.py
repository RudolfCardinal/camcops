#!/usr/bin/env python

"""
camcops_server/alembic/versions/0056_mojo_medical_infection_fields.py

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

mojo_medical_infection_fields

Revision ID: 0056
Revises: 0055
Creation date: 2020-11-16 11:28:45.597495

"""

# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0056'
down_revision = '0055'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

def upgrade():
    # We considered changing the wording of the questions about having had an
    # infection in the past month / two months preceding last month but decided
    # against it so that old data is comparable with new data. So this migration
    # now does nothing.
    pass


def downgrade():
    pass
