#!/usr/bin/env python

"""
camcops_server/tasks/esspri.py

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

**EULAR Sjögren’s Syndrome Patient Reported Index (ESSPRI) task.**

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


class EsspriMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Esspri'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        comment_strings = [
            "dryness",
            "fatigue",
            "pain",
        ]

        for q_index in range(0, cls.N_QUESTIONS):
            q_num = q_index + 1
            q_field = "q{}".format(q_num)

            score_comment = "(0 none - 10 maximum imaginable)"

            setattr(cls, q_field, CamcopsColumn(
                q_field, Integer,
                permitted_value_checker=ZERO_TO_10_CHECKER,
                comment="Q{} ({}) {}".format(
                    q_num, comment_strings[q_index], score_comment)
            ))

        super().__init__(name, bases, classdict)


class Esspri(TaskHasPatientMixin,
             Task,
             metaclass=EsspriMetaclass):
    __tablename__ = "esspri"
    shortname = "ESSPRI"

    N_QUESTIONS = 3
    MAX_SCORE = 10  # Mean of 3 scores of 10
    ALL_QUESTIONS = strseq("q", 1, N_QUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("EULAR Sjögren’s Syndrome Patient Reported Index")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="overall_score", coltype=Float(),
                value=self.overall_score(),
                comment=f"Overall score (/{self.MAX_SCORE})"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.ALL_QUESTIONS):
            return False
        if not self.field_contents_valid():
            return False
        return True

    def overall_score(self) -> float:
        return self.mean_fields(self.ALL_QUESTIONS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        rows = ""
        for q_num in range(1, self.N_QUESTIONS + 1):
            q_field = "q" + str(q_num)
            question_cell = "{}. {}".format(q_num, self.wxstring(req, q_field))

            score = getattr(self, q_field)

            rows += tr_qa(question_cell, score)

        formatted_score = ws.number_to_dp(self.overall_score(), 3, default="?")

        html = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {overall_score}
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
                [1] Mean of three numerical rating scales, each rated 0-10.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            overall_score=tr(
                self.wxstring(req, "overall_score") + " <sup>[1]</sup>",
                "{} / {}".format(
                    answer(formatted_score),
                    self.MAX_SCORE
                )
            ),
            rows=rows,
        )
        return html
