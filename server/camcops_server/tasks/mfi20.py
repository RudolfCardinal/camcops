#!/usr/bin/env python

"""
camcops_server/tasks/mfi20.py

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

**Multidimensional Fatigue Inventory (MFI-20) task.**

"""

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa, tr, answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ONE_TO_FIVE_CHECKER,
)

from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import TaskHasPatientMixin, Task
from camcops_server.cc_modules.cc_text import SS
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy import Integer
from sqlalchemy.ext.declarative import DeclarativeMeta
from typing import List, Type, Tuple, Dict, Any


class Mfi20Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Mfi20'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        comment_strings = [
            "feel fit",
            "physically little",
            "feel active",
            "nice things",
            "tired",
            "do a lot",
            "keep thought on",
            "take on a lot",
            "dread",
            "think little",
            "concentrate",
            "rested",
            "effort concentrate",
            "bad condition",
            "plans",
            "tire",
            "get little done",
            "don't feel like",
            "thoughts wander",
            "excellent condition",
        ]
        score_comment = "(1 yes - 5 no)"

        for q_index in range(0, cls.N_QUESTIONS):
            q_num = q_index + 1
            q_field = "q{}".format(q_num)

            setattr(cls, q_field, CamcopsColumn(
                q_field, Integer,
                permitted_value_checker=ONE_TO_FIVE_CHECKER,
                comment="Q{} ({}) {}".format(
                    q_num, comment_strings[q_index], score_comment)
            ))

        super().__init__(name, bases, classdict)


class Mfi20(TaskHasPatientMixin,
            Task,
            metaclass=Mfi20Metaclass):
    __tablename__ = "mfi20"
    shortname = "MFI-20"

    prohibits_clinical = True
    prohibits_commercial = True

    N_QUESTIONS = 20
    MIN_SCORE_PER_Q = 1
    MAX_SCORE_PER_Q = 5
    MIN_SCORE = MIN_SCORE_PER_Q * N_QUESTIONS
    MAX_SCORE = MAX_SCORE_PER_Q * N_QUESTIONS
    N_Q_PER_SUBSCALE = 4  # always
    MIN_SUBSCALE = MIN_SCORE_PER_Q * N_Q_PER_SUBSCALE
    MAX_SUBSCALE = MAX_SCORE_PER_Q * N_Q_PER_SUBSCALE
    ALL_QUESTIONS = strseq("q", 1, N_QUESTIONS)
    REVERSE_QUESTIONS = Task.fieldnames_from_list(
        "q", {2, 5, 9, 10, 13, 14, 16, 17, 18, 19})

    GENERAL_FATIGUE_QUESTIONS = Task.fieldnames_from_list(
        "q", {1, 5, 12, 16})
    PHYSICAL_FATIGUE_QUESTIONS = Task.fieldnames_from_list(
        "q", {2, 8, 14, 20})
    REDUCED_ACTIVITY_QUESTIONS = Task.fieldnames_from_list(
        "q", {7, 11, 13, 19})
    REDUCED_MOTIVATION_QUESTIONS = Task.fieldnames_from_list(
        "q", {3, 6, 10, 17})
    MENTAL_FATIGUE_QUESTIONS = Task.fieldnames_from_list(
        "q", {4, 9, 15, 18})

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Multidimensional Fatigue Inventory")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        subscale_range = f"[{self.MIN_SUBSCALE}–{self.MAX_SUBSCALE}]"
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score [{self.MIN_SCORE}–{self.MAX_SCORE}]"),
            SummaryElement(
                name="general_fatigue", coltype=Integer(),
                value=self.general_fatigue_score(),
                comment=f"General fatigue {subscale_range}"),
            SummaryElement(
                name="physical_fatigue", coltype=Integer(),
                value=self.physical_fatigue_score(),
                comment=f"Physical fatigue {subscale_range}"),
            SummaryElement(
                name="reduced_activity", coltype=Integer(),
                value=self.reduced_activity_score(),
                comment=f"Reduced activity {subscale_range}"),
            SummaryElement(
                name="reduced_motivation", coltype=Integer(),
                value=self.reduced_motivation_score(),
                comment=f"Reduced motivation {subscale_range}"),
            SummaryElement(
                name="mental_fatigue", coltype=Integer(),
                value=self.mental_fatigue_score(),
                comment=f"Mental fatigue {subscale_range}"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.ALL_QUESTIONS):
            return False
        if not self.field_contents_valid():
            return False
        return True

    def score_fields(self, fields: List[str]) -> int:
        total = 0
        for f in fields:
            value = getattr(self, f)
            if value is not None:
                if f in self.REVERSE_QUESTIONS:
                    value = self.MAX_SCORE_PER_Q + 1 - value

            total += value if value is not None else 0

        return total

    def total_score(self) -> int:
        return self.score_fields(self.ALL_QUESTIONS)

    def general_fatigue_score(self) -> int:
        return self.score_fields(self.GENERAL_FATIGUE_QUESTIONS)

    def physical_fatigue_score(self) -> int:
        return self.score_fields(self.PHYSICAL_FATIGUE_QUESTIONS)

    def reduced_activity_score(self) -> int:
        return self.score_fields(self.REDUCED_ACTIVITY_QUESTIONS)

    def reduced_motivation_score(self) -> int:
        return self.score_fields(self.REDUCED_MOTIVATION_QUESTIONS)

    def mental_fatigue_score(self) -> int:
        return self.score_fields(self.MENTAL_FATIGUE_QUESTIONS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        fullscale_range = f"[{self.MIN_SCORE}–{self.MAX_SCORE}]"
        subscale_range = f"[{self.MIN_SUBSCALE}–{self.MAX_SUBSCALE}]"

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
                    {total_score}
                    {general_fatigue_score}
                    {physical_fatigue_score}
                    {reduced_activity_score}
                    {reduced_motivation_score}
                    {mental_fatigue_score}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer <sup>[8]</sup></th>
                </tr>
                {rows}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Questions 2, 5, 9, 10, 13, 14, 16, 17, 18, 19
                    reverse-scored when summing.
                [2] Sum for questions 1–20.
                [3] Sum for questions 1, 5, 12, 16.
                [4] Sum for questions 2, 8, 14, 20.
                [5] Sum for questions 7, 11, 13, 19.
                [6] Sum for questions 3, 6, 10, 17.
                [7] Sum for questions 4, 9, 15, 18.
                [8] All questions are rated from “1 – yes, that is true” to
                    “5 – no, that is not true”.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE) + " <sup>[1][2]</sup>",
                f"{answer(self.total_score())} {fullscale_range}"
            ),
            general_fatigue_score=tr(
                self.wxstring(req, "general_fatigue") + " <sup>[1][3]</sup>",
                f"{answer(self.general_fatigue_score())} {subscale_range}"
            ),
            physical_fatigue_score=tr(
                self.wxstring(req, "physical_fatigue") + " <sup>[1][4]</sup>",
                f"{answer(self.physical_fatigue_score())} {subscale_range}"
            ),
            reduced_activity_score=tr(
                self.wxstring(req, "reduced_activity") + " <sup>[1][5]</sup>",
                f"{answer(self.reduced_activity_score())} {subscale_range}"
            ),
            reduced_motivation_score=tr(
                self.wxstring(req,
                              "reduced_motivation") + " <sup>[1][6]</sup>",
                f"{answer(self.reduced_motivation_score())} {subscale_range}"
            ),
            mental_fatigue_score=tr(
                self.wxstring(req, "mental_fatigue") + " <sup>[1][7]</sup>",
                f"{answer(self.mental_fatigue_score())} {subscale_range}"
            ),
            rows=rows,
        )
        return html
