#!/usr/bin/env python

"""
camcops_server/alembic/versions/0061_fix_up_patient_idnum_ids.py

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

fix_up_patient_idnum_ids

Revision ID: 0061
Revises: 0060
Creation date: 2021-03-26 16:39:00

Fix up server-created patient ID numbers that were erroneously saved with id 0
when adding a new patient.

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
from sqlalchemy import orm

from sqlalchemy.exc import IntegrityError
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import Session as SqlASession
from sqlalchemy.sql.functions import func
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import ERA_NOW
from camcops_server.cc_modules.cc_sqla_coltypes import EraColType


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0061'
down_revision = '0060'
branch_labels = None
depends_on = None


Base = declarative_base()


class PatientIdNum(Base):
    __tablename__ = "patient_idnum"

    _pk = Column(
        "_pk", Integer,
        primary_key=True,
        autoincrement=True
    )

    id = Column(
        "id", Integer,
        nullable=False,
        comment="Primary key on the source tablet device"
    )

    _device_id = Column(
        "_device_id", Integer, nullable=False
    )

    _era = Column(
        "_era", EraColType, nullable=False, index=True
    )


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    bind = op.get_bind()
    session = orm.Session(bind=bind)

    for idnum in session.query(PatientIdNum):
        if idnum.id == 0:
            save_with_next_available_id(idnum, session)

    session.commit()


# noinspection PyPep8,PyTypeChecker
def downgrade():
    pass


def save_with_next_available_id(obj: Base,
                                dbsession: SqlASession) -> None:
    """
    Deliberately copied from cc_db.py and maintained separately

    Save a record with the next available client pk in sequence.
    """
    cls = obj.__class__

    saved_ok = False

    # MySql doesn't support "select for update" so we have to keep
    # trying the next available ID and checking for an integrity
    # error in case another user has grabbed it by the time we have
    # committed
    # noinspection PyProtectedMember
    last_id = (
        dbsession
        # func.max(cls.id) + 1 here will do the right thing for
        # backends that support select for update (maybe not for no rows)
        .query(func.max(cls.id))
        .filter(cls._device_id == obj._device_id)
        .filter(cls._era == ERA_NOW)
        .scalar()
    ) or 0

    next_id = last_id + 1

    while not saved_ok:
        obj.id = next_id

        dbsession.add(obj)

        try:
            dbsession.flush()
            saved_ok = True
        except IntegrityError:
            dbsession.rollback()
            next_id += 1
