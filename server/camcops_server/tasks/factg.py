#!/usr/bin/env python
# camcops_server/tasks/factg.py

"""
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

- By Joe Kearney, Rudolf Cardinal.

"""

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Boolean, Float

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    tr_qa,
    subheading_spanning_two_columns,
    tr
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
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
)


# =============================================================================
# Fact-G
# =============================================================================

DISPLAY_DP = 2
MAX_QSCORE = 4
NON_REVERSE_SCORED_EMOTIONAL_QNUM = 2


class FactgMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Factg'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        answer_stem = (
            " (0 not at all, 1 a little bit, 2 somewhat, 3 quite a bit, "
            "4 very much)"
        )
        add_multiple_columns(
            cls, "p_q", 1, cls.N_QUESTIONS_PHYSICAL,
            minimum=0, maximum=4,
            comment_fmt="Physical well-being Q{n} ({s})" + answer_stem,
            comment_strings=[
                "lack of energy",
                "nausea",
                "trouble meeting family needs",
                "pain",
                "treatment side effects",
                "feel ill",
                "bedbound",
            ],
        )
        add_multiple_columns(
            cls, "s_q", 1, cls.N_QUESTIONS_SOCIAL,
            minimum=0, maximum=4,
            comment_fmt="Social well-being Q{n} ({s})" + answer_stem,
            comment_strings=[
                "close to friends",
                "emotional support from family",
                "support from friends",
                "family accepted illness",
                "good family comms re illness",
                "feel close to partner/main supporter",
                "satisfied with sex life",
            ],
        )
        add_multiple_columns(
            cls, "e_q", 1, cls.N_QUESTIONS_EMOTIONAL,
            minimum=0, maximum=4,
            comment_fmt="Emotional well-being Q{n} ({s})" + answer_stem,
            comment_strings=[
                "sad",
                "satisfied with coping re illness",
                "losing hope in fight against illness",
                "nervous"
                "worried about dying",
                "worried condition will worsen",
            ],
        )
        add_multiple_columns(
            cls, "f_q", 1, cls.N_QUESTIONS_FUNCTIONAL,
            minimum=0, maximum=4,
            comment_fmt="Functional well-being Q{n} ({s})" + answer_stem,
            comment_strings=[
                "able to work",
                "work fulfilling",
                "able to enjoy life",
                "accepted illness",
                "sleeping well",
                "enjoying usual fun things",
                "content with quality of life",
            ],
        )
        super().__init__(name, bases, classdict)


class FactgGroupInfo(object):
    """
    Internal information class for the FACT-G.
    """
    def __init__(self,
                 heading_xstring_name: str,
                 question_prefix: str,
                 fieldnames: List[str],
                 summary_fieldname: str,
                 summary_description: str,
                 max_score: int,
                 reverse_score_all: bool = False,
                 reverse_score_all_but_q2: bool = False) -> None:
        self.heading_xstring_name = heading_xstring_name
        self.question_prefix = question_prefix
        self.fieldnames = fieldnames
        self.summary_fieldname = summary_fieldname
        self.summary_description = summary_description
        self.max_score = max_score
        self.reverse_score_all = reverse_score_all
        self.reverse_score_all_but_q2 = reverse_score_all_but_q2
        self.n_questions = len(fieldnames)

    def subscore(self, task: "Factg") -> float:
        answered = 0
        scoresum = 0
        for qnum, fieldname in enumerate(self.fieldnames, start=1):
            answer_val = getattr(task, fieldname)
            try:
                answer_int = int(answer_val)
            except (TypeError, ValueError):
                continue
            answered += 1
            if (self.reverse_score_all or
                    (self.reverse_score_all_but_q2 and
                     qnum != NON_REVERSE_SCORED_EMOTIONAL_QNUM)):
                # reverse-scored
                scoresum += MAX_QSCORE - answer_int
            else:
                # normally scored
                scoresum += answer_int
        if answered == 0:
            return 0
        return scoresum * self.n_questions / answered


class Factg(TaskHasPatientMixin, Task,
            metaclass=FactgMetaclass):
    """
    Server implementation of the Fact-G task.
    """
    __tablename__ = "factg"
    shortname = "FACT-G"
    provides_trackers = True

    N_QUESTIONS_PHYSICAL = 7
    N_QUESTIONS_SOCIAL = 7
    N_QUESTIONS_EMOTIONAL = 6
    N_QUESTIONS_FUNCTIONAL = 7

    MAX_SCORE_PHYSICAL = 28
    MAX_SCORE_SOCIAL = 28
    MAX_SCORE_EMOTIONAL = 24
    MAX_SCORE_FUNCTIONAL = 28

    N_ALL = (
        N_QUESTIONS_PHYSICAL + N_QUESTIONS_SOCIAL +
        N_QUESTIONS_EMOTIONAL + N_QUESTIONS_FUNCTIONAL
    )

    MAX_SCORE_TOTAL = N_ALL * MAX_QSCORE

    PHYSICAL_PREFIX = "p_q"
    SOCIAL_PREFIX = "s_q"
    EMOTIONAL_PREFIX = "e_q"
    FUNCTIONAL_PREFIX = "f_q"

    QUESTIONS_PHYSICAL = strseq(PHYSICAL_PREFIX, 1, N_QUESTIONS_PHYSICAL)
    QUESTIONS_SOCIAL = strseq(SOCIAL_PREFIX, 1, N_QUESTIONS_SOCIAL)
    QUESTIONS_EMOTIONAL = strseq(EMOTIONAL_PREFIX, 1, N_QUESTIONS_EMOTIONAL)
    QUESTIONS_FUNCTIONAL = strseq(FUNCTIONAL_PREFIX, 1, N_QUESTIONS_FUNCTIONAL)

    GROUPS = [
        FactgGroupInfo("h1", PHYSICAL_PREFIX, QUESTIONS_PHYSICAL,
                       "physical_wellbeing", "Physical wellbeing subscore",
                       MAX_SCORE_PHYSICAL,
                       reverse_score_all=True),
        FactgGroupInfo("h2", SOCIAL_PREFIX, QUESTIONS_SOCIAL,
                       "social_family_wellbeing",
                       "Social/family wellbeing subscore",
                       MAX_SCORE_SOCIAL),
        FactgGroupInfo("h3", EMOTIONAL_PREFIX, QUESTIONS_EMOTIONAL,
                       "emotional_wellbeing", "Emotional wellbeing subscore",
                       MAX_SCORE_EMOTIONAL,
                       reverse_score_all_but_q2=True),
        FactgGroupInfo("h4", FUNCTIONAL_PREFIX, QUESTIONS_FUNCTIONAL,
                       "functional_wellbeing", "Functional wellbeing subscore",
                       MAX_SCORE_FUNCTIONAL),
    ]

    OPTIONAL_Q = "s_q7"

    ignore_s_q7 = CamcopsColumn("ignore_s_q7", Boolean,
                                permitted_value_checker=BIT_CHECKER)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Functional Assessment of Cancer Therapy — General")

    def is_complete(self) -> bool:
        questions_social = self.QUESTIONS_SOCIAL.copy()
        if self.ignore_s_q7:
            questions_social.remove(self.OPTIONAL_Q)

        all_qs = [self.QUESTIONS_PHYSICAL, questions_social,
                  self.QUESTIONS_EMOTIONAL, self.QUESTIONS_FUNCTIONAL]

        for qlist in all_qs:
            if self.any_fields_none(qlist):
                return False

        if not self.field_contents_valid():
            return False

        return True

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="FACT-G total score (rating well-being)",
            axis_label=f"Total score (out of {self.MAX_SCORE_TOTAL})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE_TOTAL + 0.5,
            axis_ticks=[
                TrackerAxisTick(108, "108"),
                TrackerAxisTick(100, "100"),
                TrackerAxisTick(80, "80"),
                TrackerAxisTick(60, "60"),
                TrackerAxisTick(40, "40"),
                TrackerAxisTick(20, "20"),
                TrackerAxisTick(0, "0"),
            ],
            horizontal_lines=[
                80,
                60,
                40,
                20
            ],
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        elements = self.standard_task_summary_fields()
        for info in self.GROUPS:
            subscore = info.subscore(self)
            elements.append(SummaryElement(
                name=info.summary_fieldname, coltype=Float(),
                value=subscore,
                comment=f"{info.summary_description} (out of {info.max_score})"
            ))
        elements.append(SummaryElement(
            name="total_score", coltype=Float(),
            value=self.total_score(),
            comment=f"Total score (out of {self.MAX_SCORE_TOTAL})"
        ))
        return elements

    def subscores(self) -> List[float]:
        sscores = []
        for info in self.GROUPS:
            sscores.append(info.subscore(self))
        return sscores

    def total_score(self) -> float:
        return sum(self.subscores())

    def get_task_html(self, req: CamcopsRequest) -> str:
        answers = {
            None: None,
            0: "0 — " + self.wxstring(req, "a0"),
            1: "1 — " + self.wxstring(req, "a1"),
            2: "2 — " + self.wxstring(req, "a2"),
            3: "3 — " + self.wxstring(req, "a3"),
            4: "4 — " + self.wxstring(req, "a4"),
        }
        subscore_html = ""
        answer_html = ""

        for info in self.GROUPS:
            heading = self.wxstring(req, info.heading_xstring_name)
            subscore = info.subscore(self)
            subscore_html += tr(
                heading,
                (
                    answer(round(subscore, DISPLAY_DP)) +
                    f" / {info.max_score}"
                )
            )
            answer_html += subheading_spanning_two_columns(heading)
            for q in info.fieldnames:
                if q == self.OPTIONAL_Q:
                    # insert additional row
                    answer_html += tr_qa(
                        self.xstring(req, "prefer_no_answer"),
                        self.ignore_s_q7)
                answer_val = getattr(self, q)
                answer_html += tr_qa(self.wxstring(req, q),
                                     get_from_dict(answers, answer_val))

        tscore = round(self.total_score(), DISPLAY_DP)

        tr_total_score = tr(
            req.sstring(SS.TOTAL_SCORE),
            answer(tscore) + f" / {self.MAX_SCORE_TOTAL}"
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                 <table class="{CssClass.SUMMARY}">
                     {self.get_is_complete_tr(req)}
                     {tr_total_score}
                     {subscore_html}
                 </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {answer_html}
            </table>
        """
