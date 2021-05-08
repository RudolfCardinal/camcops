#!/usr/bin/env python

"""
camcops_server/tasks/tests/core10_tests.py

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

import pendulum
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.tasks.core10 import Core10, Core10Report

from camcops_server.cc_modules.tests.cc_report_tests import (
    AverageScoreReportTestCase
)


class Core10ReportTestCase(AverageScoreReportTestCase):
    def create_report(self) -> Core10Report:
        return Core10Report(via_index=False)

    def create_task(self, patient: Patient,
                    q1: int = 0, q2: int = 0, q3: int = 0, q4: int = 0,
                    q5: int = 0, q6: int = 0, q7: int = 0, q8: int = 0,
                    q9: int = 0, q10: int = 0, era: str = None) -> None:
        task = Core10()
        self.apply_standard_task_fields(task)
        task.id = next(self.task_id_sequence)

        task.patient_id = patient.id

        task.q1 = q1
        task.q2 = q2
        task.q3 = q3
        task.q4 = q4
        task.q5 = q5
        task.q6 = q6
        task.q7 = q7
        task.q8 = q8
        task.q9 = q9
        task.q10 = q10

        if era is not None:
            task.when_created = pendulum.parse(era)
            # log.info(f"Creating task, when_created = {task.when_created}")

        self.dbsession.add(task)


class Core10ReportTests(Core10ReportTestCase):
    def create_tasks(self) -> None:
        self.patient_1 = self.create_patient(idnum_value=333)
        self.patient_2 = self.create_patient(idnum_value=444)
        self.patient_3 = self.create_patient(idnum_value=555)

        # Initial average score = (8 + 6 + 4) / 3 = 6
        # Latest average score = (2 + 3 + 4) / 3 = 3

        self.create_task(patient=self.patient_1, q1=4, q2=4,
                         era="2018-06-01")  # Score 8
        self.create_task(patient=self.patient_1, q7=1, q8=1,
                         era="2018-10-04")  # Score 2

        self.create_task(patient=self.patient_2, q3=3, q4=3,
                         era="2018-05-02")  # Score 6
        self.create_task(patient=self.patient_2, q3=2, q4=1,
                         era="2018-10-03")  # Score 3

        self.create_task(patient=self.patient_3, q5=2, q6=2,
                         era="2018-01-10")  # Score 4
        self.create_task(patient=self.patient_3, q9=1, q10=3,
                         era="2018-10-01")  # Score 4
        self.dbsession.commit()

    def test_row_has_totals_and_averages(self) -> None:
        tsv_pages = self.report.get_tsv_pages(req=self.req)
        # log.info(f"\n{tsv_pages[0]}")
        # log.info(f"\n{tsv_pages[1]}")
        expected_rows = [
            [
                3,  # n initial
                3,  # n latest
                6.0,  # Initial average
                3.0,  # Latest average
                3.0,  # Average progress
            ]
        ]
        self.assertEqual(tsv_pages[0].plainrows, expected_rows)


class Core10ReportEmptyTests(Core10ReportTestCase):
    def test_no_rows_when_no_data(self) -> None:
        tsv_pages = self.report.get_tsv_pages(req=self.req)
        no_data = self.report.no_data_value()
        expected_rows = [
            [
                0, 0, no_data, no_data, no_data,
            ]
        ]
        self.assertEqual(tsv_pages[0].plainrows, expected_rows)


class Core10ReportDoubleCountingTests(Core10ReportTestCase):
    def create_tasks(self) -> None:
        self.patient_1 = self.create_patient(idnum_value=333)
        self.patient_2 = self.create_patient(idnum_value=444)
        self.patient_3 = self.create_patient(idnum_value=555)

        # Initial average score = (8 + 6 + 4) / 3 = 6
        # Latest average score  = (    3 + 3) / 2 = 3
        # Progress avg score    = (    3 + 1) / 2 = 2  ... NOT 3.
        self.create_task(patient=self.patient_1, q1=4, q2=4,
                         era="2018-06-01")  # Score 8

        self.create_task(patient=self.patient_2, q3=3, q4=3,
                         era="2018-05-02")  # Score 6
        self.create_task(patient=self.patient_2, q3=2, q4=1,
                         era="2018-10-03")  # Score 3

        self.create_task(patient=self.patient_3, q5=2, q6=2,
                         era="2018-01-10")  # Score 4
        self.create_task(patient=self.patient_3, q9=1, q10=2,
                         era="2018-10-01")  # Score 3
        self.dbsession.commit()

    def test_record_does_not_appear_in_first_and_latest(self) -> None:
        tsv_pages = self.report.get_tsv_pages(req=self.req)
        expected_rows = [
            [
                3,  # n initial
                2,  # n latest
                6.0,  # Initial average
                3.0,  # Latest average
                2.0,  # Average progress
            ]
        ]
        self.assertEqual(tsv_pages[0].plainrows, expected_rows)


class Core10ReportDateRangeTests(Core10ReportTestCase):
    """
    Test code:

    .. code-block:: sql

        -- 2019-10-21
        -- For SQLite:

        CREATE TABLE core10
            (_pk INT, patient_id INT, when_created DATETIME, _current INT);

        .schema core10

        INSERT INTO core10
            (_pk,patient_id,when_created,_current)
        VALUES
            (1,1,'2018-06-01T00:00:00.000000+00:00',1),
            (2,1,'2018-08-01T00:00:00.000000+00:00',1),
            (3,1,'2018-10-01T00:00:00.000000+00:00',1),
            (4,2,'2018-06-01T00:00:00.000000+00:00',1),
            (5,2,'2018-08-01T00:00:00.000000+00:00',1),
            (6,2,'2018-10-01T00:00:00.000000+00:00',1),
            (7,3,'2018-06-01T00:00:00.000000+00:00',1),
            (8,3,'2018-08-01T00:00:00.000000+00:00',1),
            (9,3,'2018-10-01T00:00:00.000000+00:00',1);

        SELECT * from core10;

        SELECT STRFTIME('%Y-%m-%d %H:%M:%f', core10.when_created) from core10;
        -- ... gives e.g.
        -- 2018-06-01 00:00:00.000

        SELECT *
            FROM core10
            WHERE core10._current = 1
            AND STRFTIME('%Y-%m-%d %H:%M:%f', core10.when_created) >= '2018-06-01 00:00:00.000000'
            AND STRFTIME('%Y-%m-%d %H:%M:%f', core10.when_created) < '2018-09-01 00:00:00.000000';

        -- That fails. Either our date/time comparison code is wrong for SQLite, or
        -- we are inserting text in the wrong format.
        -- Ah. It's the number of decimal places:

        SELECT '2018-06-01 00:00:00.000' >= '2018-06-01 00:00:00.000000';  -- 0, false
        SELECT '2018-06-01 00:00:00.000' >= '2018-06-01 00:00:00.000';  -- 1, true

    See
    :func:`camcops_server.cc_modules.cc_sqla_coltypes.isotzdatetime_to_utcdatetime_sqlite`.

    """  # noqa

    def create_tasks(self) -> None:
        self.patient_1 = self.create_patient(idnum_value=333)
        self.patient_2 = self.create_patient(idnum_value=444)
        self.patient_3 = self.create_patient(idnum_value=555)

        # 2018-06 average score = (8 + 6 + 4) / 3 = 6
        # 2018-08 average score = (4 + 4 + 4) / 3 = 4
        # 2018-10 average score = (2 + 3 + 4) / 3 = 3

        self.create_task(patient=self.patient_1, q1=4, q2=4,
                         era="2018-06-01")  # Score 8
        self.create_task(patient=self.patient_1, q7=3, q8=1,
                         era="2018-08-01")  # Score 4
        self.create_task(patient=self.patient_1, q7=1, q8=1,
                         era="2018-10-01")  # Score 2

        self.create_task(patient=self.patient_2, q3=3, q4=3,
                         era="2018-06-01")  # Score 6
        self.create_task(patient=self.patient_2, q3=2, q4=2,
                         era="2018-08-01")  # Score 4
        self.create_task(patient=self.patient_2, q3=1, q4=2,
                         era="2018-10-01")  # Score 3

        self.create_task(patient=self.patient_3, q5=2, q6=2,
                         era="2018-06-01")  # Score 4
        self.create_task(patient=self.patient_3, q9=1, q10=3,
                         era="2018-08-01")  # Score 4
        self.create_task(patient=self.patient_3, q9=1, q10=3,
                         era="2018-10-01")  # Score 4
        self.dbsession.commit()

        self.dump_table(Core10.__tablename__, [
            "_pk", "patient_id", "when_created", "_current",
        ])

    def test_report_filtered_by_date_range(self) -> None:
        # self.report.start_datetime = pendulum.parse("2018-05-01T00:00:00.000000+00:00")  # noqa
        self.report.start_datetime = pendulum.parse("2018-06-01T00:00:00.000000+00:00")  # noqa
        self.report.end_datetime = pendulum.parse("2018-09-01T00:00:00.000000+00:00")  # noqa

        self.set_echo(True)
        tsv_pages = self.report.get_tsv_pages(req=self.req)
        self.set_echo(False)
        expected_rows = [
            [
                3,  # n initial
                3,  # n latest
                6.0,  # Initial average
                4.0,  # Latest average
                2.0,  # Average progress
            ]
        ]
        self.assertEqual(tsv_pages[0].plainrows, expected_rows)
