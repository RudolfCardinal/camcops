#!/usr/bin/env python

"""
camcops_server/tasks/sfmpq2.py

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

**Short-Form McGill Pain Questionnaire (SF-MPQ2) task.**

"""

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa, tr, answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ZERO_TO_10_CHECKER,
)

from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import TaskHasPatientMixin, Task
import cardinal_pythonlib.rnc_web as ws
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy import Float, Integer
from sqlalchemy.ext.declarative import DeclarativeMeta
from typing import List, Type, Tuple, Dict, Any


class Sfmpq2Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Sfmpq2'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        # Field descriptions are open access, as per:
        # https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5221718/
        # https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3225325/
        comment_strings = [
            "throbbing",
            "shooting",
            "stabbing",
            "sharp",
            "cramping",
            "gnawing",
            "hot-burning",
            "aching",
            "heavy",
            "tender",
            "splitting",
            "tiring–exhausting",
            "sickening",
            "fearful",
            "punishing–cruel",
            "electric-shock",
            "cold-freezing",
            "piercing",
            "light touch",
            "itching",
            "tingling",
            "numbness",
        ]
        score_comment = "(0 none - 10 worst)"

        for q_index in range(0, cls.N_QUESTIONS):
            q_num = q_index + 1
            q_field = "q{}".format(q_num)

            setattr(cls, q_field, CamcopsColumn(
                q_field, Integer,
                permitted_value_checker=ZERO_TO_10_CHECKER,
                comment="Q{} ({}) {}".format(
                    q_num,
                    comment_strings[q_index],
                    score_comment)
            ))

        super().__init__(name, bases, classdict)


class Sfmpq2(TaskHasPatientMixin,
             Task,
             metaclass=Sfmpq2Metaclass):
    __tablename__ = "sfmpq2"
    shortname = "SF-MPQ2"

    N_QUESTIONS = 22
    MAX_SCORE_PER_Q = 10
    ALL_QUESTIONS = strseq("q", 1, N_QUESTIONS)

    CONTINUOUS_PAIN_QUESTIONS = Task.fieldnames_from_list(
        "q", {1, 5, 6, 8, 9, 10})
    INTERMITTENT_PAIN_QUESTIONS = Task.fieldnames_from_list(
        "q", {2, 3, 4, 11, 16, 18})
    NEUROPATHIC_PAIN_QUESTIONS = Task.fieldnames_from_list(
        "q", {7, 17, 19, 20, 21, 22})
    AFFECTIVE_PAIN_QUESTIONS = Task.fieldnames_from_list(
        "q", {12, 13, 14, 15})

    @staticmethod
    def longname(req: CamcopsRequest) -> str:
        _ = req.gettext
        return _("Short-Form McGill Pain Questionnaire 2")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total_pain", coltype=Float(),
                value=self.total_pain(),
                comment=f"Total pain (/{self.MAX_SCORE_PER_Q})"),
            SummaryElement(
                name="continuous_pain", coltype=Float(),
                value=self.continuous_pain(),
                comment=f"Continuous pain (/{self.MAX_SCORE_PER_Q})"),
            SummaryElement(
                name="intermittent_pain", coltype=Float(),
                value=self.intermittent_pain(),
                comment=f"Intermittent pain (/{self.MAX_SCORE_PER_Q})"),
            SummaryElement(
                name="neuropathic_pain", coltype=Float(),
                value=self.neuropathic_pain(),
                comment=f"Neuropathic pain (/{self.MAX_SCORE_PER_Q})"),
            SummaryElement(
                name="affective_pain", coltype=Float(),
                value=self.affective_pain(),
                comment=f"Affective pain (/{self.MAX_SCORE_PER_Q})"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.ALL_QUESTIONS):
            return False
        if not self.field_contents_valid():
            return False
        return True

    def total_pain(self) -> float:
        return self.mean_fields(self.ALL_QUESTIONS)

    def continuous_pain(self) -> float:
        return self.mean_fields(self.CONTINUOUS_PAIN_QUESTIONS)

    def intermittent_pain(self) -> float:
        return self.mean_fields(self.INTERMITTENT_PAIN_QUESTIONS)

    def neuropathic_pain(self) -> float:
        return self.mean_fields(self.NEUROPATHIC_PAIN_QUESTIONS)

    def affective_pain(self) -> float:
        return self.mean_fields(self.AFFECTIVE_PAIN_QUESTIONS)

    def format_average(self, value) -> str:
        return "{} / {}".format(
            answer(ws.number_to_dp(value, 3, default="?")),
            self.MAX_SCORE_PER_Q
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        rows = ""
        for q_num in range(1, self.N_QUESTIONS + 1):
            q_field = "q" + str(q_num)
            question_cell = "{}. {}".format(q_num, self.wxstring(req, q_field))

            score = getattr(self, q_field)

            rows += tr_qa(question_cell, score)

        html = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_pain}
                    {continuous_pain}
                    {intermittent_pain}
                    {neuropathic_pain}
                    {affective_pain}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer <sup>[6]</sup></th>
                </tr>
                {rows}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Average of items 1–22.
                [2] Average of items 1, 5, 6, 8, 9, 10.
                [3] Average of items 2, 3, 4, 11, 16, 18.
                [4] Average of items 7, 17, 19, 20, 21, 22.
                [5] Average of items 12, 13, 14, 15.
                [6] All items are rated from “0 – none” to
                    “10 – worst possible”.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_pain=tr(
                self.wxstring(req, "total_pain") + " <sup>[1]</sup>",
                self.format_average(self.total_pain())
            ),
            continuous_pain=tr(
                self.wxstring(req, "continuous_pain") + " <sup>[2]</sup>",
                self.format_average(self.continuous_pain())
            ),
            intermittent_pain=tr(
                self.wxstring(req, "intermittent_pain") + " <sup>[3]</sup>",
                self.format_average(self.intermittent_pain())
            ),
            neuropathic_pain=tr(
                self.wxstring(req, "neuropathic_pain") + " <sup>[4]</sup>",
                self.format_average(self.neuropathic_pain())
            ),
            affective_pain=tr(
                self.wxstring(req, "affective_pain") + " <sup>[5]</sup>",
                self.format_average(self.affective_pain())
            ),
            rows=rows,
        )
        return html
