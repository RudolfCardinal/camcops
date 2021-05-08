#!/usr/bin/env python

"""
camcops_server/tasks/chit.py

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

**Cambridge-Chicago Compulsivity Trait Scale task.**

"""

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    tr_qa,
    get_yes_no_unknown,
    tr,
    answer,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import BoolColumn
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    TaskHasPatientMixin,
    Task,
    get_from_dict,
)
from camcops_server.cc_modules.cc_text import SS
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy import Integer
from sqlalchemy.ext.declarative import DeclarativeMeta
from typing import List, Type, Tuple, Dict, Any


class ChitMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Chit'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_SCORED_QUESTIONS,
            minimum=0, maximum=3,
            comment_fmt="Q{n} ({s}) (0 strongly disagree - 3 strongly agree)",
            comment_strings=[
                "hate unfinished task",
                "just right",
                "keep doing task",
                "get stuck",
                "habit",
                "addictive",
                "stubborn rigid",
                "urges",
                "rewarding things",
                "hard moving",
                "higher standards",
                "improvement",
                "complete",
                "avoid situations",
                "hobby"]
        )

        setattr(
            cls, "q16",
            BoolColumn("q16", comment="Q16 (negative effect) (0 no, 1 yes)")
        )

        super().__init__(name, bases, classdict)


class Chit(TaskHasPatientMixin,
           Task,
           metaclass=ChitMetaclass):
    __tablename__ = "chit"
    shortname = "CHI-T"

    N_SCORED_QUESTIONS = 15
    N_QUESTIONS = 16
    MAX_SCORE_MAIN = 3 * N_SCORED_QUESTIONS
    SCORED_QUESTIONS = strseq("q", 1, N_SCORED_QUESTIONS)
    ALL_QUESTIONS = strseq("q", 1, N_QUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Cambridge–Chicago Compulsivity Trait Scale")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/{self.MAX_SCORE_MAIN})"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.ALL_QUESTIONS):
            return False
        if not self.field_contents_valid():
            return False
        return True

    def total_score(self) -> int:
        return self.sum_fields(self.SCORED_QUESTIONS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        score_dict = {
            None: None,
            0: "0 — " + self.wxstring(req, "a0"),
            1: "1 — " + self.wxstring(req, "a1"),
            2: "2 — " + self.wxstring(req, "a2"),
            3: "3 — " + self.wxstring(req, "a3")
        }

        rows = ""
        for i in range(1, self.N_SCORED_QUESTIONS + 1):
            q_field = "q" + str(i)
            question_cell = "{}. {}".format(i, self.wxstring(req, q_field))
            answer_cell = get_from_dict(score_dict, getattr(self, q_field))

            rows += tr_qa(question_cell, answer_cell)

        rows += tr_qa("16. " + self.wxstring(req, "q16"),
                      get_yes_no_unknown(req, "q16"))

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
                [1] Sum for questions 1–15.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE) + " <sup>[1]</sup>",
                answer(self.total_score()) + f" / {self.MAX_SCORE_MAIN}"
            ),
            rows=rows,
        )
        return html
