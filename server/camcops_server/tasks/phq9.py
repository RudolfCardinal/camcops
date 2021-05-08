#!/usr/bin/env python

"""
camcops_server/tasks/phq9.py

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

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Boolean, Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo, CTV_INCOMPLETE
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, get_yes_no, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    SummaryCategoryColType,
    ZERO_TO_THREE_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerAxisTick,
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# PHQ-9
# =============================================================================

class Phq9Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Phq9'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_MAIN_QUESTIONS,
            minimum=0, maximum=3,
            comment_fmt="Q{n} ({s}) (0 not at all - 3 nearly every day)",
            comment_strings=[
                "anhedonia",
                "mood",
                "sleep",
                "energy",
                "appetite",
                "self-esteem/guilt",
                "concentration",
                "psychomotor",
                "death/self-harm",
            ]
        )
        super().__init__(name, bases, classdict)


class Phq9(TaskHasPatientMixin, Task,
           metaclass=Phq9Metaclass):
    """
    Server implementation of the PHQ9 task.
    """
    __tablename__ = "phq9"
    shortname = "PHQ-9"
    provides_trackers = True

    q10 = CamcopsColumn(
        "q10", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment="Q10 (difficulty in activities) (0 not difficult at "
                "all - 3 extremely difficult)"
    )

    N_MAIN_QUESTIONS = 9
    MAX_SCORE_MAIN = 3 * N_MAIN_QUESTIONS
    MAIN_QUESTIONS = strseq("q", 1, N_MAIN_QUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Patient Health Questionnaire-9")

    def is_complete(self) -> bool:
        if self.any_fields_none(self.MAIN_QUESTIONS):
            return False
        if self.total_score() > 0 and self.q10 is None:
            return False
        if not self.field_contents_valid():
            return False
        return True

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="PHQ-9 total score (rating depressive symptoms)",
            axis_label=f"Score for Q1-9 (out of {self.MAX_SCORE_MAIN})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE_MAIN + 0.5,
            axis_ticks=[
                TrackerAxisTick(27, "27"),
                TrackerAxisTick(25, "25"),
                TrackerAxisTick(20, "20"),
                TrackerAxisTick(15, "15"),
                TrackerAxisTick(10, "10"),
                TrackerAxisTick(5, "5"),
                TrackerAxisTick(0, "0"),
            ],
            horizontal_lines=[
                19.5,
                14.5,
                9.5,
                4.5
            ],
            horizontal_labels=[
                TrackerLabel(23, req.sstring(SS.SEVERE)),
                TrackerLabel(17, req.sstring(SS.MODERATELY_SEVERE)),
                TrackerLabel(12, req.sstring(SS.MODERATE)),
                TrackerLabel(7, req.sstring(SS.MILD)),
                TrackerLabel(2.25, req.sstring(SS.NONE)),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"PHQ-9 total score {self.total_score()}/{self.MAX_SCORE_MAIN} "
            f"({self.severity(req)})"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/{self.MAX_SCORE_MAIN})"),
            SummaryElement(
                name="n_core", coltype=Integer(),
                value=self.n_core(),
                comment="Number of core symptoms"),
            SummaryElement(
                name="n_other", coltype=Integer(),
                value=self.n_other(),
                comment="Number of other symptoms"),
            SummaryElement(
                name="n_total", coltype=Integer(),
                value=self.n_total(),
                comment="Total number of symptoms"),
            SummaryElement(
                name="is_mds", coltype=Boolean(),
                value=self.is_mds(),
                comment="PHQ9 major depressive syndrome?"),
            SummaryElement(
                name="is_ods", coltype=Boolean(),
                value=self.is_ods(),
                comment="PHQ9 other depressive syndrome?"),
            SummaryElement(
                name="severity", coltype=SummaryCategoryColType,
                value=self.severity(req),
                comment="PHQ9 depression severity"),
        ]

    def total_score(self) -> int:
        return self.sum_fields(self.MAIN_QUESTIONS)

    def one_if_q_ge(self, qnum: int, threshold: int) -> int:
        value = getattr(self, "q" + str(qnum))
        return 1 if value is not None and value >= threshold else 0

    def n_core(self) -> int:
        return (self.one_if_q_ge(1, 2) +
                self.one_if_q_ge(2, 2))

    def n_other(self) -> int:
        return (self.one_if_q_ge(3, 2) +
                self.one_if_q_ge(4, 2) +
                self.one_if_q_ge(5, 2) +
                self.one_if_q_ge(6, 2) +
                self.one_if_q_ge(7, 2) +
                self.one_if_q_ge(8, 2) +
                self.one_if_q_ge(9, 1))  # suicidality
        # suicidality counted whenever present

    def n_total(self) -> int:
        return self.n_core() + self.n_other()

    def is_mds(self) -> bool:
        return self.n_core() >= 1 and self.n_total() >= 5

    def is_ods(self) -> bool:
        return self.n_core() >= 1 and 2 <= self.n_total() <= 4

    def severity(self, req: CamcopsRequest) -> str:
        total = self.total_score()
        if total >= 20:
            return req.sstring(SS.SEVERE)
        elif total >= 15:
            return req.sstring(SS.MODERATELY_SEVERE)
        elif total >= 10:
            return req.sstring(SS.MODERATE)
        elif total >= 5:
            return req.sstring(SS.MILD)
        else:
            return req.sstring(SS.NONE)

    def get_task_html(self, req: CamcopsRequest) -> str:
        main_dict = {
            None: None,
            0: "0 — " + self.wxstring(req, "a0"),
            1: "1 — " + self.wxstring(req, "a1"),
            2: "2 — " + self.wxstring(req, "a2"),
            3: "3 — " + self.wxstring(req, "a3")
        }
        q10_dict = {
            None: None,
            0: "0 — " + self.wxstring(req, "fa0"),
            1: "1 — " + self.wxstring(req, "fa1"),
            2: "2 — " + self.wxstring(req, "fa2"),
            3: "3 — " + self.wxstring(req, "fa3")
        }
        q_a = ""
        for i in range(1, self.N_MAIN_QUESTIONS + 1):
            nstr = str(i)
            q_a += tr_qa(self.wxstring(req, "q" + nstr),
                         get_from_dict(main_dict, getattr(self, "q" + nstr)))
        q_a += tr_qa("10. " + self.wxstring(req, "finalq"),
                     get_from_dict(q10_dict, self.q10))

        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {depression_severity}
                    {n_symptoms}
                    {mds}
                    {ods}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings are over the last 2 weeks.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Sum for questions 1–9.
                [2] Total score ≥20 severe, ≥15 moderately severe,
                    ≥10 moderate, ≥5 mild, &lt;5 none.
                [3] Number of questions 1–2 rated ≥2.
                [4] Number of questions 3–8 rated ≥2, or question 9
                    rated ≥1.
                [5] ≥1 core symptom and ≥5 total symptoms (as per
                    DSM-IV-TR page 356).
                [6] ≥1 core symptom and 2–4 total symptoms (as per
                    DSM-IV-TR page 775).
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE) + " <sup>[1]</sup>",
                answer(self.total_score()) + f" / {self.MAX_SCORE_MAIN}"
            ),
            depression_severity=tr_qa(
                self.wxstring(req, "depression_severity") + " <sup>[2]</sup>",
                self.severity(req)
            ),
            n_symptoms=tr(
                "Number of symptoms: core <sup>[3]</sup>, other "
                "<sup>[4]</sup>, total",
                answer(self.n_core()) + "/2, " +
                answer(self.n_other()) + "/7, " +
                answer(self.n_total()) + "/9"
            ),
            mds=tr_qa(
                self.wxstring(req, "mds") + " <sup>[5]</sup>",
                get_yes_no(req, self.is_mds())
            ),
            ods=tr_qa(
                self.wxstring(req, "ods") + " <sup>[6]</sup>",
                get_yes_no(req, self.is_ods())
            ),
            q_a=q_a,
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        procedure = req.snomed(SnomedLookup.PHQ9_PROCEDURE_DEPRESSION_SCREENING)  # noqa
        codes = [SnomedExpression(procedure)]
        if self.is_complete():
            scale = req.snomed(SnomedLookup.PHQ9_SCALE)
            score = req.snomed(SnomedLookup.PHQ9_SCORE)
            screen_negative = req.snomed(SnomedLookup.PHQ9_FINDING_NEGATIVE_SCREENING_FOR_DEPRESSION)  # noqa
            screen_positive = req.snomed(SnomedLookup.PHQ9_FINDING_POSITIVE_SCREENING_FOR_DEPRESSION)  # noqa
            if self.is_mds() or self.is_ods():
                # Threshold debatable, but if you have "other depressive
                # syndrome", it seems wrong to say you've screened negative for
                # depression.
                procedure_result = screen_positive
            else:
                procedure_result = screen_negative
            codes.append(SnomedExpression(scale, {score: self.total_score()}))
            codes.append(SnomedExpression(procedure_result))
        return codes
