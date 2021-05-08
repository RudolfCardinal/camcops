#!/usr/bin/env python

"""
camcops_server/tasks/tests/maas_tests.py

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
from camcops_server.tasks.maas import Maas, MaasReport

from camcops_server.cc_modules.tests.cc_report_tests import (
    AverageScoreReportTestCase
)


class MaasReportTests(AverageScoreReportTestCase):
    PROGRESS_COL = 4

    def create_report(self) -> MaasReport:
        return MaasReport(via_index=False)

    def create_tasks(self) -> None:
        self.patient_1 = self.create_patient()

        self.create_task(patient=self.patient_1, q1=2, q2=2,
                         era="2019-03-01")  # total 17 + 2 + 2
        self.create_task(patient=self.patient_1, q1=5, q2=5,
                         era="2019-06-01")  # total 17 + 5 + 5
        self.dbsession.commit()

    def create_task(self, patient: Patient, era: str = None, **kwargs) -> None:
        task = Maas()
        self.apply_standard_task_fields(task)
        task.id = next(self.task_id_sequence)

        task.patient_id = patient.id
        for fieldname in Maas.TASK_FIELDS:
            value = kwargs.get(fieldname, 1)
            setattr(task, fieldname, value)

        if era is not None:
            task.when_created = pendulum.parse(era)

        self.dbsession.add(task)

    def test_average_progress_is_positive(self) -> None:
        tsv_pages = self.report.get_tsv_pages(req=self.req)

        expected_progress = 27 - 21
        actual_progress = tsv_pages[0].plainrows[0][self.PROGRESS_COL]

        self.assertEqual(actual_progress, expected_progress)
