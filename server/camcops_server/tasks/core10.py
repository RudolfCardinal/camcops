#!/usr/bin/env python

"""
camcops_server/tasks/core10.py

===============================================================================

    Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

"""

from typing import Dict, List, Optional

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.stringfunc import strseq
import pendulum
from semantic_version import Version
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo, CTV_INCOMPLETE
from camcops_server.cc_modules.cc_html import answer, tr, tr_qa
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_report import (
    AverageScoreReport,
    AverageScoreReportTestCase,
    ScoreDetails,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ZERO_TO_FOUR_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerAxisTick,
    TrackerInfo,
)


# =============================================================================
# CORE-10
# =============================================================================

class Core10(TaskHasPatientMixin, Task):
    """
    Server implementation of the CORE-10 task.
    """
    __tablename__ = "core10"
    shortname = "CORE-10"
    provides_trackers = True

    COMMENT_NORMAL = " (0 not at all - 4 most or all of the time)"
    COMMENT_REVERSED = " (0 most or all of the time - 4 not at all)"

    q1 = CamcopsColumn(
        "q1", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q1 (tension/anxiety)" + COMMENT_NORMAL
    )
    q2 = CamcopsColumn(
        "q2", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q2 (support)" + COMMENT_REVERSED
    )
    q3 = CamcopsColumn(
        "q3", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q3 (coping)" + COMMENT_REVERSED
    )
    q4 = CamcopsColumn(
        "q4", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q4 (talking is too much)" + COMMENT_NORMAL
    )
    q5 = CamcopsColumn(
        "q5", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q5 (panic)" + COMMENT_NORMAL
    )
    q6 = CamcopsColumn(
        "q6", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q6 (suicidality)" + COMMENT_NORMAL
    )
    q7 = CamcopsColumn(
        "q7", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q7 (sleep problems)" + COMMENT_NORMAL
    )
    q8 = CamcopsColumn(
        "q8", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q8 (despair/hopelessness)" + COMMENT_NORMAL
    )
    q9 = CamcopsColumn(
        "q9", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q9 (unhappy)" + COMMENT_NORMAL
    )
    q10 = CamcopsColumn(
        "q10", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Q10 (unwanted images)" + COMMENT_NORMAL
    )

    N_QUESTIONS = 10
    MAX_SCORE = 4 * N_QUESTIONS
    QUESTION_FIELDNAMES = strseq("q", 1, N_QUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Clinical Outcomes in Routine Evaluation, 10-item measure")

    # noinspection PyMethodParameters
    @classproperty
    def minimum_client_version(cls) -> Version:
        return Version("2.2.8")

    def is_complete(self) -> bool:
        return self.all_fields_not_none(self.QUESTION_FIELDNAMES)

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.clinical_score(),
            plot_label="CORE-10 clinical score (rating distress)",
            axis_label=f"Clinical score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
            axis_ticks=[
                TrackerAxisTick(40, "40"),
                TrackerAxisTick(35, "35"),
                TrackerAxisTick(30, "30"),
                TrackerAxisTick(25, "25"),
                TrackerAxisTick(20, "20"),
                TrackerAxisTick(15, "15"),
                TrackerAxisTick(10, "10"),
                TrackerAxisTick(5, "5"),
                TrackerAxisTick(0, "0"),
            ],
            horizontal_lines=[
                30,
                20,
                10,
            ],
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"CORE-10 clinical score {self.clinical_score()}/{self.MAX_SCORE}"
        ))]
        # todo: CORE10: add suicidality to clinical text?

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="clinical_score", coltype=Integer(),
                value=self.clinical_score(),
                comment=f"Clinical score (/{self.MAX_SCORE})"),
        ]

    def total_score(self) -> int:
        return self.sum_fields(self.QUESTION_FIELDNAMES)

    def n_questions_complete(self) -> int:
        return self.n_fields_not_none(self.QUESTION_FIELDNAMES)

    def clinical_score(self) -> float:
        n_q_completed = self.n_questions_complete()
        if n_q_completed == 0:
            # avoid division by zero
            return 0
        return self.N_QUESTIONS * self.total_score() / n_q_completed

    def get_task_html(self, req: CamcopsRequest) -> str:
        normal_dict = {
            None: None,
            0: "0 — " + self.wxstring(req, "a0"),
            1: "1 — " + self.wxstring(req, "a1"),
            2: "2 — " + self.wxstring(req, "a2"),
            3: "3 — " + self.wxstring(req, "a3"),
            4: "4 — " + self.wxstring(req, "a4"),
        }
        reversed_dict = {
            None: None,
            0: "0 — " + self.wxstring(req, "a4"),
            1: "1 — " + self.wxstring(req, "a3"),
            2: "2 — " + self.wxstring(req, "a2"),
            3: "3 — " + self.wxstring(req, "a1"),
            4: "4 — " + self.wxstring(req, "a0"),
        }

        def get_tr_qa(qnum_: int, mapping: Dict[Optional[int], str]) -> str:
            nstr = str(qnum_)
            return tr_qa(self.wxstring(req, "q" + nstr),
                         get_from_dict(mapping, getattr(self, "q" + nstr)))

        q_a = get_tr_qa(1, normal_dict)
        for qnum in [2, 3]:
            q_a += get_tr_qa(qnum, reversed_dict)
        for qnum in range(4, self.N_QUESTIONS + 1):
            q_a += get_tr_qa(qnum, normal_dict)

        tr_clinical_score = tr(
            "Clinical score <sup>[1]</sup>",
            answer(self.clinical_score()) + " / {}".format(self.MAX_SCORE)
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_clinical_score}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings are over the last week.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Clinical score is: number of questions × total score
                    ÷ number of questions completed. If all questions are
                    completed, it's just the total score.
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.CORE10_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.CORE10_SCALE),
                {
                    req.snomed(SnomedLookup.CORE10_SCORE): self.total_score(),
                }
            ))
        return codes


class Core10Report(AverageScoreReport):
    """
    An average score of the people seen at the start of treatment
    an average final measure and an average progress score.
    """
    @classproperty
    def report_id(cls) -> str:
        return "core10"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("CORE-10 — Average scores")

    @classproperty
    def task_class(cls) -> Task:
        return Core10

    @classmethod
    def scores(cls, req: "CamcopsRequest") -> List[ScoreDetails]:
        _ = req.gettext
        return [
            ScoreDetails(
                name=_("CORE-10 clinical score"),
                fieldnames=Core10.QUESTION_FIELDNAMES,
                min=0,
                max=Core10.MAX_SCORE
            )
        ]

    @classproperty
    def higher_score_is_better(cls) -> bool:
        return False


class Core10ReportTestCase(AverageScoreReportTestCase):
    def create_report(self) -> Core10Report:
        return Core10Report()

    def create_task(self, patient: Patient,
                    q1: int=0, q2: int=0, q3: int=0, q4: int=0,
                    q5: int=0, q6: int=0, q7: int=0, q8: int=0,
                    q9: int=0, q10: int=0, era: str=None) -> None:
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

        self.dbsession.add(task)


class Core10ReportTests(Core10ReportTestCase):
    def create_tasks(self):
        self.patient_1 = self.create_patient()
        self.patient_2 = self.create_patient()
        self.patient_3 = self.create_patient()

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
        plain_report = self.report.get_rows_colnames(self.req)

        expected_rows = [
            [
                3,  # Initial total
                3,  # Latest total
                6,  # Initial average
                3,  # Latest average
                3,  # Average progress
            ]
        ]

        self.assertEqual(plain_report.rows, expected_rows)


class Core10ReportEmptyTests(Core10ReportTestCase):
    def test_no_rows_when_no_data(self) -> None:
        plain_report = self.report.get_rows_colnames(req=self.req)

        self.assertEquals(
            plain_report.rows,
            [
                [
                    0, 0, "No data", "No data", "Unable to calculate",
                ]
            ]
        )


class Core10ReportDoubleCountingTests(Core10ReportTestCase):
    def create_tasks(self):
        self.patient_1 = self.create_patient()
        self.patient_2 = self.create_patient()
        self.patient_3 = self.create_patient()

        # Initial average score = (8 + 6 + 4) / 3 = 6
        # Latest average score = (3 + 3) / 3 = 2
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
        plain_report = self.report.get_rows_colnames(self.req)

        expected_rows = [
            [
                3,  # Initial total
                2,  # Latest total
                6,  # Initial average
                3,  # Latest average
                3,  # Average progress
            ]
        ]

        self.assertEqual(plain_report.rows, expected_rows)


class Core10ReportDateRangeTests(Core10ReportTestCase):
    def create_tasks(self) -> None:
        self.patient_1 = self.create_patient()
        self.patient_2 = self.create_patient()
        self.patient_3 = self.create_patient()

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

    def test_report_filtered_by_date_range(self):
        self.report.start_datetime = "2018-06-01T00:00:00.000000+00:00"
        self.report.end_datetime = "2018-09-01T00:00:00.000000+00:00"
        plain_report = self.report.get_rows_colnames(self.req)

        expected_rows = [
            [
                3,  # Initial total
                3,  # Latest total
                6,  # Initial average
                4,  # Latest average
                2,  # Average progress
            ]
        ]

        self.assertEqual(plain_report.rows, expected_rows)
