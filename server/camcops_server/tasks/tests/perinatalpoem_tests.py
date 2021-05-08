#!/usr/bin/env python

"""
camcops_server/tasks/tests/perinatalpoem_tests.py

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

from typing import Generator

import pendulum

from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase

from camcops_server.tasks.perinatalpoem import (
    PerinatalPoem,
    PerinatalPoemReport,
)


# =============================================================================
# Unit tests
# =============================================================================

class PerinatalPoemReportTestCase(DemoDatabaseTestCase):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    def setUp(self) -> None:
        super().setUp()

        self.report = PerinatalPoemReport()

        # Really only needed for tests
        self.report.start_datetime = None
        self.report.end_datetime = None

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1

    def create_task(self, **kwargs) -> None:
        task = PerinatalPoem()
        self.apply_standard_task_fields(task)
        task.id = next(self.id_sequence)

        era = kwargs.pop('era', None)
        if era is not None:
            task.when_created = pendulum.parse(era)

        for name, value in kwargs.items():
            setattr(task, name, value)

        self.dbsession.add(task)


class PerinatalPoemReportTests(PerinatalPoemReportTestCase):
    """
    Most of the base class tested in APEQCPFT Perinatal so just some basic
    sanity checking here
    """

    def create_tasks(self):
        self.create_task(general_comments="comment 1")
        self.create_task(general_comments="comment 2")
        self.create_task(general_comments="comment 3")

        self.dbsession.commit()

    def test_qa_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[0].rows

        self.assertEqual(len(rows), 1)
        self.assertEqual(len(rows[0]), 4)

    def test_qb_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[1].rows

        self.assertEqual(len(rows), 1)
        self.assertEqual(len(rows[0]), 4)

    def test_q1_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[2].rows

        self.assertEqual(len(rows), 2)
        self.assertEqual(len(rows[0]), 7)

    def test_q2_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[3].rows

        self.assertEqual(len(rows), 12)
        self.assertEqual(len(rows[0]), 6)

    def test_q3_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[4].rows

        self.assertEqual(len(rows), 6)
        self.assertEqual(len(rows[0]), 6)

    def test_participation_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[5].rows

        self.assertEqual(len(rows), 1)
        self.assertEqual(len(rows[0]), 4)

    def test_comments(self) -> None:
        expected_comments = [
            "comment 1", "comment 2", "comment 3",
        ]
        comments = self.report._get_comments(self.req)
        self.assertEqual(comments, expected_comments)


class PerinatalPoemReportDateRangeTests(PerinatalPoemReportTestCase):
    def create_tasks(self) -> None:
        self.create_task(general_comments="comments 1",
                         era="2018-10-01T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 2",
                         era="2018-10-02T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 3",
                         era="2018-10-03T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 4",
                         era="2018-10-04T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 5",
                         era="2018-10-05T00:00:00.000000+00:00")
        self.dbsession.commit()

    def test_comments_filtered_by_date(self) -> None:
        self.report.start_datetime = "2018-10-02T00:00:00.000000+00:00"
        self.report.end_datetime = "2018-10-05T00:00:00.000000+00:00"

        comments = self.report._get_comments(self.req)
        self.assertEqual(len(comments), 3)

        self.assertEqual(comments[0], "comments 2")
        self.assertEqual(comments[1], "comments 3")
        self.assertEqual(comments[2], "comments 4")
