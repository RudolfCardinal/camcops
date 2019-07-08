#!/usr/bin/env python

"""
camcops_server/tasks/suppsp.py

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

**Short UPPS-P Impulsive Behaviour Scale (SUPPS-P) task.**

"""
from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa, tr, answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ONE_TO_FOUR_CHECKER,
)

from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import TaskHasPatientMixin, \
    TaskHasClinicianMixin, Task, get_from_dict
from camcops_server.cc_modules.cc_text import SS
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy import Integer
from sqlalchemy.ext.declarative import DeclarativeMeta
from typing import List, Type, Tuple, Dict, Any


class SuppspMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Suppsp'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        comment_strings = [
            "see to end",
            "careful and purposeful",
            "problem situations",
            "unfinished bother",
            "stop and think",
            "do things regret",
            "hate to stop",
            "can't stop what I'm doing",
            "enjoy risks",
            "lose control",
            "finish",
            "rational sensible",
            "act without thinking upset",
            "new and exciting",
            "say things regret",
            "airplane",
            "others shocked",
            "skiing",
            "think carefully",
            "act without thinking excited",
        ]

        reverse_questions = {3, 6, 8, 9, 10, 13, 14, 15, 16, 17, 18, 20}

        for q_index in range(0, cls.N_QUESTIONS):
            q_num = q_index + 1
            q_field = "q{}".format(q_num)

            score_comment = "(1 strongly agree - 4 strongly disagree)"

            if q_num in reverse_questions:
                score_comment = "(1 strongly disagree - 4 strongly agree)"

            setattr(cls, q_field, CamcopsColumn(
                q_field, Integer,
                permitted_value_checker=ONE_TO_FOUR_CHECKER,
                comment="Q{} ({}) {}".format(
                    q_num, comment_strings[q_index], score_comment)
            ))

        super().__init__(name, bases, classdict)


class Suppsp(TaskHasPatientMixin,
             TaskHasClinicianMixin,
             Task,
             metaclass=SuppspMetaclass):
    __tablename__ = "suppsp"
    shortname = "SUPPS-P"

    N_QUESTIONS = 20
    MAX_SCORE = 4 * N_QUESTIONS
    ALL_QUESTIONS = strseq("q", 1, N_QUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Short UPPS-P Impulsive Behaviour Scale")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/{self.MAX_SCORE})"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.ALL_QUESTIONS):
            return False
        if not self.field_contents_valid():
            return False
        return True

    def total_score(self) -> int:
        return self.sum_fields(self.ALL_QUESTIONS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        normal_score_dict = {
            None: None,
            1: "1 — " + self.wxstring(req, "a0"),
            2: "2 — " + self.wxstring(req, "a1"),
            3: "3 — " + self.wxstring(req, "a2"),
            4: "4 — " + self.wxstring(req, "a3")
        }

        reverse_score_dict = {
            None: None,
            4: "4 — " + self.wxstring(req, "a0"),
            3: "3 — " + self.wxstring(req, "a1"),
            2: "2 — " + self.wxstring(req, "a2"),
            1: "1 — " + self.wxstring(req, "a3")
        }

        reverse_q_nums = {3, 6, 8, 9, 10, 13, 14, 15, 16, 17, 18, 20}

        rows = ""
        for q_num in range(1, self.N_QUESTIONS + 1):
            q_field = "q" + str(q_num)
            question_cell = "{}. {}".format(q_num, self.wxstring(req, q_field))

            score = getattr(self, q_field)
            score_dict = normal_score_dict

            if q_num in reverse_q_nums:
                score_dict = reverse_score_dict

            answer_cell = get_from_dict(score_dict, score)

            rows += tr_qa(question_cell, answer_cell)

        html = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {rows}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Sum for questions 1–20.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE) + " <sup>[1]</sup>",
                answer(self.total_score()) + f" / {self.MAX_SCORE}"
            ),
            rows=rows,
        )
        return html