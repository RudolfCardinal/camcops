#!/usr/bin/env python

"""
camcops_server/tasks/shaps.py

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

**Snaith–Hamilton Pleasure Scale (SHAPS) task.**

"""

from typing import Any, Dict, List, Type, Tuple

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy import Integer
from sqlalchemy.ext.declarative import DeclarativeMeta

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import tr_qa, tr, answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    TaskHasPatientMixin,
    Task,
)
from camcops_server.cc_modules.cc_text import SS


class ShapsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Shaps'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        add_multiple_columns(
            cls, "q", 1, cls.N_QUESTIONS,
            minimum=0, maximum=3,
            comment_fmt="Q{n} - {s}",
            comment_strings=[
                "television",
                "family",
                "hobbies",
                "meal",
                "bath",
                "flowers",
                "smiling",
                "smart",
                "book",
                "tea",
                "sunny",
                "landscape",
                "helping",
                "praise",
            ]
        )

        super().__init__(name, bases, classdict)


class Shaps(TaskHasPatientMixin,
            Task,
            metaclass=ShapsMetaclass):
    __tablename__ = "shaps"
    shortname = "SHAPS"

    N_QUESTIONS = 14
    MAX_SCORE = 14
    ALL_QUESTIONS = strseq("q", 1, N_QUESTIONS)

    STRONGLY_DISAGREE = 0
    DISAGREE = 1
    AGREE = 2
    STRONGLY_OR_DEFINITELY_AGREE = 3

    # Q11 in British Journal of Psychiatry (1995), 167, 99-103
    # actually has two "Strongly disagree" options. Assuming this
    # is not intentional!
    REVERSE_QUESTIONS = {2, 4, 5, 7, 9, 12, 14}

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Snaith–Hamilton Pleasure Scale")

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
        # Consistent with client implementation
        return self.count_where(self.ALL_QUESTIONS,
                                [self.STRONGLY_DISAGREE, self.DISAGREE])

    def get_task_html(self, req: CamcopsRequest) -> str:
        strongly_disagree = self.wxstring(req, "strongly_disagree")
        disagree = self.wxstring(req, "disagree")
        agree = self.wxstring(req, "agree")

        # We store the actual answers given but these are scored 1 or 0
        forward_answer_dict = {
            None: None,
            self.STRONGLY_DISAGREE: "1 — " + strongly_disagree,
            self.DISAGREE: "1 — " + disagree,
            self.AGREE: "0 — " + agree,
            self.STRONGLY_OR_DEFINITELY_AGREE: "0 — " + self.wxstring(
                req, "strongly_agree")
        }

        # Subtle difference in wording when options presented in reverse
        reverse_answer_dict = {
            None: None,
            self.STRONGLY_OR_DEFINITELY_AGREE: "0 — " + self.wxstring(
                req, "definitely_agree"),
            self.AGREE: "0 — " + agree,
            self.DISAGREE: "1 — " + disagree,
            self.STRONGLY_DISAGREE: "1 — " + strongly_disagree
        }

        rows = ""
        for q_num in range(1, self.N_QUESTIONS + 1):
            q_field = "q" + str(q_num)
            question_cell = "{}. {}".format(q_num, self.wxstring(req, q_field))

            answer_dict = forward_answer_dict

            if q_num in self.REVERSE_QUESTIONS:
                answer_dict = reverse_answer_dict

            answer_cell = get_from_dict(answer_dict, getattr(self, q_field))
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
                [1] Score 1 point for either ‘disagree’ option,
                    0 points for either ‘agree’ option.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE) + " <sup>[1]</sup>",
                "{} / {}".format(
                    answer(self.total_score()),
                    self.MAX_SCORE
                )
            ),
            rows=rows,
        )
        return html
