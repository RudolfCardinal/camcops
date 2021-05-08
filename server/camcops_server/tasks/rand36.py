#!/usr/bin/env python

"""
camcops_server/tasks/rand36.py

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

from typing import Any, Dict, List, Optional, Tuple, Type

from cardinal_pythonlib.maths_py import mean
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Float, Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, identity, tr, tr_span_col
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ONE_TO_FIVE_CHECKER,
    ONE_TO_SIX_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# RAND-36
# =============================================================================

class Rand36Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Rand36'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 3, 12,
            minimum=1, maximum=3,
            comment_fmt="Q{n} ({s}) (1 limited a lot - 3 not limited at all)",
            comment_strings=[
                "Vigorous activities",
                "Moderate activities",
                "Lifting or carrying groceries",
                "Climbing several flights of stairs",
                "Climbing one flight of stairs",
                "Bending, kneeling, or stooping",
                "Walking more than a mile",
                "Walking several blocks",
                "Walking one block",
                "Bathing or dressing yourself",
            ]
        )
        add_multiple_columns(
            cls, "q", 13, 16,
            minimum=1, maximum=2,
            comment_fmt="Q{n} (physical health: {s}) (1 yes, 2 no)",
            comment_strings=[
                "Cut down work/other activities",
                "Accomplished less than would like",
                "Were limited in the kind of work or other activities",
                "Had difficulty performing the work or other activities",
            ]
        )
        add_multiple_columns(
            cls, "q", 17, 19,
            minimum=1, maximum=2,
            comment_fmt="Q{n} (emotional problems: {s}) (1 yes, 2 no)",
            comment_strings=[
                "Cut down work/other activities",
                "Accomplished less than would like",
                "Didn't do work or other activities as carefully as usual",
                "Had difficulty performing the work or other activities",
            ]
        )
        add_multiple_columns(
            cls, "q", 23, 31,
            minimum=1, maximum=6,
            comment_fmt="Q{n} (past 4 weeks: {s}) (1 all of the time - "
                        "6 none of the time)",
            comment_strings=[
                "Did you feel full of pep?",
                "Have you been a very nervous person?",
                "Have you felt so down in the dumps that nothing could cheer "
                "you up?",
                "Have you felt calm and peaceful?",
                "Did you have a lot of energy?",
                "Have you felt downhearted and blue?",
                "Did you feel worn out?",
                "Have you been a happy person?",
                "Did you feel tired?",
            ]
        )
        add_multiple_columns(
            cls, "q", 33, 36,
            minimum=1, maximum=5,
            comment_fmt="Q{n} (how true/false: {s}) (1 definitely true - "
                        "5 definitely false)",
            comment_strings=[
                "I seem to get sick a little easier than other people",
                "I am as healthy as anybody I know",
                "I expect my health to get worse",
                "My health is excellent",
            ]
        )
        super().__init__(name, bases, classdict)


class Rand36(TaskHasPatientMixin, Task,
             metaclass=Rand36Metaclass):
    """
    Server implementation of the RAND-36 task.
    """
    __tablename__ = "rand36"
    shortname = "RAND-36"
    provides_trackers = True

    NQUESTIONS = 36

    q1 = CamcopsColumn(
        "q1", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Q1 (general health) (1 excellent - 5 poor)"
    )
    q2 = CamcopsColumn(
        "q2", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Q2 (health cf. 1y ago) (1 much better - 5 much worse)"
    )

    q20 = CamcopsColumn(
        "q20", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Q20 (past 4 weeks, to what extent physical health/"
                "emotional problems interfered with social activity) "
                "(1 not at all - 5 extremely)"
    )
    q21 = CamcopsColumn(
        "q21", Integer,
        permitted_value_checker=ONE_TO_SIX_CHECKER,
        comment="Q21 (past 4 weeks, how much pain (1 none - 6 very severe)"
    )
    q22 = CamcopsColumn(
        "q22", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Q22 (past 4 weeks, pain interfered with normal activity "
                "(1 not at all - 5 extremely)"
    )

    q32 = CamcopsColumn(
        "q32", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Q32 (past 4 weeks, how much of the time has physical "
                "health/emotional problems interfered with social activities "
                "(1 all of the time - 5 none of the time)"
    )
    # ... note Q32 extremely similar to Q20.

    TASK_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("RAND 36-Item Short Form Health Survey 1.0")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    @classmethod
    def tracker_element(cls, value: float, plot_label: str) -> TrackerInfo:
        return TrackerInfo(
            value=value,
            plot_label="RAND-36: " + plot_label,
            axis_label="Scale score (out of 100)",
            axis_min=-0.5,
            axis_max=100.5
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [
            self.tracker_element(
                self.score_overall(),
                self.wxstring(req, "score_overall")),
            self.tracker_element(
                self.score_physical_functioning(),
                self.wxstring(req, "score_physical_functioning")),
            self.tracker_element(
                self.score_role_limitations_physical(),
                self.wxstring(req, "score_role_limitations_physical")),
            self.tracker_element(
                self.score_role_limitations_emotional(),
                self.wxstring(req, "score_role_limitations_emotional")),
            self.tracker_element(
                self.score_energy(),
                self.wxstring(req, "score_energy")),
            self.tracker_element(
                self.score_emotional_wellbeing(),
                self.wxstring(req, "score_emotional_wellbeing")),
            self.tracker_element(
                self.score_social_functioning(),
                self.wxstring(req, "score_social_functioning")),
            self.tracker_element(
                self.score_pain(),
                self.wxstring(req, "score_pain")),
            self.tracker_element(
                self.score_general_health(),
                self.wxstring(req, "score_general_health")),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=(
                "RAND-36 (scores out of 100, 100 best): overall {ov}, "
                "physical functioning {pf}, physical role "
                "limitations {prl}, emotional role limitations {erl}, "
                "energy {e}, emotional wellbeing {ew}, social "
                "functioning {sf}, pain {p}, general health {gh}.".format(
                    ov=self.score_overall(),
                    pf=self.score_physical_functioning(),
                    prl=self.score_role_limitations_physical(),
                    erl=self.score_role_limitations_emotional(),
                    e=self.score_energy(),
                    ew=self.score_emotional_wellbeing(),
                    sf=self.score_social_functioning(),
                    p=self.score_pain(),
                    gh=self.score_general_health(),
                )
            )
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="overall", coltype=Float(),
                value=self.score_overall(),
                comment="Overall mean score (0-100, higher better)"),
            SummaryElement(
                name="physical_functioning", coltype=Float(),
                value=self.score_physical_functioning(),
                comment="Physical functioning score (0-100, higher better)"),
            SummaryElement(
                name="role_limitations_physical", coltype=Float(),
                value=self.score_role_limitations_physical(),
                comment="Role limitations due to physical health score "
                        "(0-100, higher better)"),
            SummaryElement(
                name="role_limitations_emotional", coltype=Float(),
                value=self.score_role_limitations_emotional(),
                comment="Role limitations due to emotional problems score "
                        "(0-100, higher better)"),
            SummaryElement(
                name="energy", coltype=Float(),
                value=self.score_energy(),
                comment="Energy/fatigue score (0-100, higher better)"),
            SummaryElement(
                name="emotional_wellbeing", coltype=Float(),
                value=self.score_emotional_wellbeing(),
                comment="Emotional well-being score (0-100, higher better)"),
            SummaryElement(
                name="social_functioning", coltype=Float(),
                value=self.score_social_functioning(),
                comment="Social functioning score (0-100, higher better)"),
            SummaryElement(
                name="pain", coltype=Float(),
                value=self.score_pain(),
                comment="Pain score (0-100, higher better)"),
            SummaryElement(
                name="general_health", coltype=Float(),
                value=self.score_general_health(),
                comment="General health score (0-100, higher better)"),
        ]

    # Scoring
    def recode(self, q: int) -> Optional[float]:
        x = getattr(self, "q" + str(q))  # response
        if x is None or x < 1:
            return None
        # http://m.rand.org/content/dam/rand/www/external/health/
        #        surveys_tools/mos/mos_core_36item_scoring.pdf
        if q == 1 or q == 2 or q == 20 or q == 22 or q == 34 or q == 36:
            # 1 becomes 100, 2 => 75, 3 => 50, 4 =>25, 5 => 0
            if x > 5:
                return None
            return 100 - 25 * (x - 1)
        elif 3 <= q <= 12:
            # 1 => 0, 2 => 50, 3 => 100
            if x > 3:
                return None
            return 50 * (x - 1)
        elif 13 <= q <= 19:
            # 1 => 0, 2 => 100
            if x > 2:
                return None
            return 100 * (x - 1)
        elif q == 21 or q == 23 or q == 26 or q == 27 or q == 30:
            # 1 => 100, 2 => 80, 3 => 60, 4 => 40, 5 => 20, 6 => 0
            if x > 6:
                return None
            return 100 - 20 * (x - 1)
        elif q == 24 or q == 25 or q == 28 or q == 29 or q == 31:
            # 1 => 0, 2 => 20, 3 => 40, 4 => 60, 5 => 80, 6 => 100
            if x > 6:
                return None
            return 20 * (x - 1)
        elif q == 32 or q == 33 or q == 35:
            # 1 => 0, 2 => 25, 3 => 50, 4 => 75, 5 => 100
            if x > 5:
                return None
            return 25 * (x - 1)
        return None

    def score_physical_functioning(self) -> Optional[float]:
        return mean([self.recode(3), self.recode(4), self.recode(5),
                     self.recode(6), self.recode(7), self.recode(8),
                     self.recode(9), self.recode(10), self.recode(11),
                     self.recode(12)])

    def score_role_limitations_physical(self) -> Optional[float]:
        return mean([self.recode(13), self.recode(14), self.recode(15),
                     self.recode(16)])

    def score_role_limitations_emotional(self) -> Optional[float]:
        return mean([self.recode(17), self.recode(18), self.recode(19)])

    def score_energy(self) -> Optional[float]:
        return mean([self.recode(23), self.recode(27), self.recode(29),
                     self.recode(31)])

    def score_emotional_wellbeing(self) -> Optional[float]:
        return mean([self.recode(24), self.recode(25), self.recode(26),
                     self.recode(28), self.recode(30)])

    def score_social_functioning(self) -> Optional[float]:
        return mean([self.recode(20), self.recode(32)])

    def score_pain(self) -> Optional[float]:
        return mean([self.recode(21), self.recode(22)])

    def score_general_health(self) -> Optional[float]:
        return mean([self.recode(1), self.recode(33), self.recode(34),
                     self.recode(35), self.recode(36)])

    @staticmethod
    def format_float_for_display(val: Optional[float]) -> Optional[str]:
        if val is None:
            return None
        return f"{val:.1f}"

    def score_overall(self) -> Optional[float]:
        values = []
        for q in range(1, self.NQUESTIONS + 1):
            values.append(self.recode(q))
        return mean(values)

    @staticmethod
    def section_row_html(text: str) -> str:
        return tr_span_col(text, cols=3, tr_class=CssClass.SUBHEADING)

    def answer_text(self, req: CamcopsRequest,
                    q: int, v: Any) -> Optional[str]:
        if v is None:
            return None
        # wxstring has its own validity checking, so we can do:
        if q == 1 or q == 2 or (20 <= q <= 22) or q == 32:
            return self.wxstring(req, "q" + str(q) + "_option" + str(v))
        elif 3 <= q <= 12:
            return self.wxstring(req, "activities_option" + str(v))
        elif 13 <= q <= 19:
            return self.wxstring(req, "yesno_option" + str(v))
        elif 23 <= q <= 31:
            return self.wxstring(req, "last4weeks_option" + str(v))
        elif 33 <= q <= 36:
            return self.wxstring(req, "q33to36_option" + str(v))
        else:
            return None

    def answer_row_html(self, req: CamcopsRequest, q: int) -> str:
        qtext = self.wxstring(req, "q" + str(q))
        v = getattr(self, "q" + str(q))
        atext = self.answer_text(req, q, v)
        s = self.recode(q)
        return tr(
            qtext,
            answer(v) + ": " + answer(atext),
            answer(s, formatter_answer=identity)
        )

    @staticmethod
    def scoreline(text: str, footnote_num: int, score: Optional[float]) -> str:
        return tr(
            text + f" <sup>[{footnote_num}]</sup>",
            answer(score) + " / 100"
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
        """
        h += self.scoreline(
            self.wxstring(req, "score_overall"), 1,
            self.format_float_for_display(self.score_overall()))
        h += self.scoreline(
            self.wxstring(req, "score_physical_functioning"), 2,
            self.format_float_for_display(self.score_physical_functioning()))
        h += self.scoreline(
            self.wxstring(req, "score_role_limitations_physical"), 3,
            self.format_float_for_display(
                self.score_role_limitations_physical()))
        h += self.scoreline(
            self.wxstring(req, "score_role_limitations_emotional"), 4,
            self.format_float_for_display(
                self.score_role_limitations_emotional()))
        h += self.scoreline(
            self.wxstring(req, "score_energy"), 5,
            self.format_float_for_display(self.score_energy()))
        h += self.scoreline(
            self.wxstring(req, "score_emotional_wellbeing"), 6,
            self.format_float_for_display(self.score_emotional_wellbeing()))
        h += self.scoreline(
            self.wxstring(req, "score_social_functioning"), 7,
            self.format_float_for_display(self.score_social_functioning()))
        h += self.scoreline(
            self.wxstring(req, "score_pain"), 8,
            self.format_float_for_display(self.score_pain()))
        h += self.scoreline(
            self.wxstring(req, "score_general_health"), 9,
            self.format_float_for_display(self.score_general_health()))
        h += f"""
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="30%">Answer</th>
                    <th width="10%">Score</th>
                </tr>
        """
        for q in range(1, 2 + 1):
            h += self.answer_row_html(req, q)
        h += self.section_row_html(self.wxstring(req, "activities_q"))
        for q in range(3, 12 + 1):
            h += self.answer_row_html(req, q)
        h += self.section_row_html(
            self.wxstring(req, "work_activities_physical_q"))
        for q in range(13, 16 + 1):
            h += self.answer_row_html(req, q)
        h += self.section_row_html(
            self.wxstring(req, "work_activities_emotional_q"))
        for q in range(17, 19 + 1):
            h += self.answer_row_html(req, q)
        h += self.section_row_html("<br>")
        h += self.answer_row_html(req, 20)
        h += self.section_row_html("<br>")
        for q in range(21, 22 + 1):
            h += self.answer_row_html(req, q)
        h += self.section_row_html(self.wxstring(req, "last4weeks_q_a") + " " +
                                   self.wxstring(req, "last4weeks_q_b"))
        for q in range(23, 31 + 1):
            h += self.answer_row_html(req, q)
        h += self.section_row_html("<br>")
        for q in [32]:
            h += self.answer_row_html(req, q)
        h += self.section_row_html(self.wxstring(req, "q33to36stem"))
        for q in range(33, 36 + 1):
            h += self.answer_row_html(req, q)
        h += f"""
            </table>
            <div class="{CssClass.COPYRIGHT}">
                The RAND 36-Item Short Form Health Survey was developed at RAND
                as part of the Medical Outcomes Study. See
                <a href="https://www.rand.org/health/surveys_tools/mos/mos_core_36item.html">
                https://www.rand.org/health/surveys_tools/mos/mos_core_36item.html</a>
            </div>
            <div class="{CssClass.FOOTNOTES}">
                All questions are first transformed to a score in the range
                0–100. Higher scores are always better. Then:
                [1] Mean of all 36 questions.
                [2] Mean of Q3–12 inclusive.
                [3] Q13–16.
                [4] Q17–19.
                [5] Q23, 27, 29, 31.
                [6] Q24, 25, 26, 28, 30.
                [7] Q20, 32.
                [8] Q21, 22.
                [9] Q1, 33–36.
            </div>
        """  # noqa
        return h
