#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_sqla_coltypes_tests.py

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
"""

import datetime
from typing import Union
import unittest

from cardinal_pythonlib.datetimefunc import coerce_to_pendulum
from cardinal_pythonlib.sqlalchemy.session import SQLITE_MEMORY_URL
from pendulum import DateTime as Pendulum, Duration
from semantic_version import Version
from sqlalchemy.sql.functions import func
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import (
    DateTime,
    Integer,
)

from camcops_server.cc_modules.cc_sqla_coltypes import (
    isotzdatetime_to_utcdatetime,
    PendulumDateTimeAsIsoTextColType,
    PendulumDurationAsIsoTextColType,
    SemanticVersionColType,
    unknown_field_to_utcdatetime,
)


# =============================================================================
# Unit testing
# =============================================================================

class SqlaColtypesTest(unittest.TestCase):
    """
    Unit tests.
    """
    # don't inherit from ExtendedTestCase; circular import

    @staticmethod
    def _assert_dt_equal(a: Union[datetime.datetime, Pendulum],
                         b: Union[datetime.datetime, Pendulum]) -> None:
        # Accept that one may have been truncated or rounded to milliseconds.
        a = coerce_to_pendulum(a)
        b = coerce_to_pendulum(b)
        diff = a - b
        assert diff.microseconds < 1000, f"{a!r} != {b!r}"

    def test_iso_datetime_field(self) -> None:
        import pendulum
        from sqlalchemy.engine import create_engine
        from sqlalchemy.sql.expression import select
        from sqlalchemy.sql.schema import MetaData, Table

        engine = create_engine(SQLITE_MEMORY_URL, echo=True)
        meta = MetaData()
        meta.bind = engine  # adds execute() method to select() etc.
        # ... http://docs.sqlalchemy.org/en/latest/core/connections.html

        id_colname = 'id'
        dt_local_colname = 'dt_local'
        dt_utc_colname = 'dt_utc'
        iso_colname = 'iso'
        id_col = Column(id_colname, Integer, primary_key=True)
        dt_local_col = Column(dt_local_colname, DateTime)
        dt_utc_col = Column(dt_utc_colname, DateTime)
        iso_col = Column(iso_colname, PendulumDateTimeAsIsoTextColType)

        table = Table('testtable', meta,
                      id_col, dt_local_col, dt_utc_col, iso_col)
        table.create()

        now = Pendulum.now()
        now_utc = now.in_tz(pendulum.UTC)
        yesterday = now.subtract(days=1)
        yesterday_utc = yesterday.in_tz(pendulum.UTC)

        table.insert().values([
            {
                id_colname: 1,
                dt_local_colname: now,
                dt_utc_colname: now_utc,
                iso_colname: now,
            },
            {
                id_colname: 2,
                dt_local_colname: yesterday,
                dt_utc_colname: yesterday_utc,
                iso_colname: yesterday
            },
        ]).execute()
        select_fields = [
            id_col,
            dt_local_col,
            dt_utc_col,
            iso_col,
            func.length(dt_local_col).label("len_dt_local_col"),
            func.length(dt_utc_col).label("len_dt_utc_col"),
            func.length(iso_col).label("len_iso_col"),
            isotzdatetime_to_utcdatetime(iso_col).label("iso_to_utcdt"),
            unknown_field_to_utcdatetime(dt_utc_col).label("uk_utcdt_to_utcdt"),
            unknown_field_to_utcdatetime(iso_col).label("uk_iso_to_utc_dt"),
        ]
        rows = list(
            select(select_fields)
            .select_from(table)
            .order_by(id_col)
            .execute()
        )
        self._assert_dt_equal(rows[0][dt_local_col], now)
        self._assert_dt_equal(rows[0][dt_utc_col], now_utc)
        self._assert_dt_equal(rows[0][iso_colname], now)
        self._assert_dt_equal(rows[0]["iso_to_utcdt"], now_utc)
        self._assert_dt_equal(rows[0]["uk_utcdt_to_utcdt"], now_utc)
        self._assert_dt_equal(rows[0]["uk_iso_to_utc_dt"], now_utc)
        self._assert_dt_equal(rows[1][dt_local_col], yesterday)
        self._assert_dt_equal(rows[1][dt_utc_col], yesterday_utc)
        self._assert_dt_equal(rows[1][iso_colname], yesterday)
        self._assert_dt_equal(rows[1]["iso_to_utcdt"], yesterday_utc)
        self._assert_dt_equal(rows[1]["uk_utcdt_to_utcdt"], yesterday_utc)
        self._assert_dt_equal(rows[1]["uk_iso_to_utc_dt"], yesterday_utc)

    @staticmethod
    def _assert_duration_equal(a: Duration, b: Duration) -> None:
        assert a == b, f"{a!r} != {b!r}"

    def test_iso_duration_field(self) -> None:
        from sqlalchemy.engine import create_engine
        from sqlalchemy.sql.expression import select
        from sqlalchemy.sql.schema import MetaData, Table

        # As above:
        engine = create_engine(SQLITE_MEMORY_URL, echo=True)
        meta = MetaData()
        meta.bind = engine

        id_colname = 'id'
        duration_colname = 'duration_iso'
        id_col = Column(id_colname, Integer, primary_key=True)
        duration_col = Column(duration_colname,
                              PendulumDurationAsIsoTextColType)

        table = Table('testtable', meta,
                      id_col, duration_col)
        table.create()

        d1 = Duration(years=1, months=3, seconds=3, microseconds=4)
        d2 = Duration(seconds=987.654321)
        d3 = Duration(days=-5)

        table.insert().values([
            {
                id_colname: 1,
                duration_colname: d1,
            },
            {
                id_colname: 2,
                duration_colname: d2,
            },
            {
                id_colname: 3,
                duration_colname: d3,
            },
        ]).execute()
        select_fields = [
            id_col,
            duration_col,
        ]
        rows = list(
            select(select_fields)
            .select_from(table)
            .order_by(id_col)
            .execute()
        )
        self._assert_duration_equal(rows[0][duration_col], d1)
        self._assert_duration_equal(rows[1][duration_col], d2)
        self._assert_duration_equal(rows[2][duration_col], d3)

    @staticmethod
    def _assert_version_equal(a: Version, b: Version) -> None:
        assert a == b, f"{a!r} != {b!r}"

    def test_semantic_version_field(self) -> None:
        from sqlalchemy.engine import create_engine
        from sqlalchemy.sql.expression import select
        from sqlalchemy.sql.schema import MetaData, Table

        # As above:
        engine = create_engine(SQLITE_MEMORY_URL, echo=True)
        meta = MetaData()
        meta.bind = engine

        id_colname = 'id'
        version_colname = 'version'
        id_col = Column(id_colname, Integer, primary_key=True)
        version_col = Column(version_colname, SemanticVersionColType)

        table = Table('testtable', meta,
                      id_col, version_col)
        table.create()

        v1 = Version("1.1.0")
        v2 = Version("2.0.1")
        v3 = Version("14.0.0")

        table.insert().values([
            {
                id_colname: 1,
                version_colname: v1,
            },
            {
                id_colname: 2,
                version_colname: v2,
            },
            {
                id_colname: 3,
                version_colname: v3,
            },
        ]).execute()
        select_fields = [
            id_col,
            version_col,
        ]
        rows = list(
            select(select_fields)
            .select_from(table)
            .order_by(id_col)
            .execute()
        )
        self._assert_version_equal(rows[0][version_col], v1)
        self._assert_version_equal(rows[1][version_col], v2)
        self._assert_version_equal(rows[2][version_col], v3)
