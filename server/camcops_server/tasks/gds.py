#!/usr/bin/env python

"""
camcops_server/tasks/gds.py

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
from sqlalchemy.sql.sqltypes import Integer, String

from camcops_server.cc_modules.cc_constants import CssClass, NO_CHAR, YES_CHAR
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# GDS-15
# =============================================================================

class Gds15Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Gds15'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS, String(length=1),
            pv=[NO_CHAR, YES_CHAR],
            comment_fmt="Q{n}, {s} ('Y' or 'N')",
            comment_strings=[
                "satisfied",
                "dropped activities",
                "life empty",
                "bored",
                "good spirits",  # 5
                "afraid",
                "happy",
                "helpless",
                "stay at home",
                "memory problems",  # 10
                "wonderful to be alive",
                "worthless",
                "full of energy",
                "hopeless",
                "others better off",  # 15
            ]
        )
        super().__init__(name, bases, classdict)


class Gds15(TaskHasPatientMixin, Task,
            metaclass=Gds15Metaclass):
    """
    Server implementation of the GDS-15 task.
    """
    __tablename__ = "gds15"
    shortname = "GDS-15"
    provides_trackers = True

    NQUESTIONS = 15
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    SCORE_IF_YES = [2, 3, 4, 6, 8, 9, 10, 12, 14, 15]
    SCORE_IF_NO = [1, 5, 7, 11, 13]
    MAX_SCORE = 15

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Geriatric Depression Scale, 15-item version")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="GDS-15 total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"GDS-15 total score {self.total_score()}/{self.MAX_SCORE}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{self.MAX_SCORE})"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        score = 0
        for q in self.SCORE_IF_YES:
            if getattr(self, "q" + str(q)) == YES_CHAR:
                score += 1
        for q in self.SCORE_IF_NO:
            if getattr(self, "q" + str(q)) == NO_CHAR:
                score += 1
        return score

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()

        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            suffix = " †" if q in self.SCORE_IF_YES else " *"
            q_a += tr_qa(
                str(q) + ". " + self.wxstring(req, "q" + str(q)) + suffix,
                getattr(self, "q" + str(q))
            )

        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr(req.sstring(SS.TOTAL_SCORE),
                        answer(score) + f" / {self.MAX_SCORE}")}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings are over the last 1 week.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="70%">Question</th>
                    <th width="30%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                (†) ‘Y’ scores 1; ‘N’ scores 0.
                (*) ‘Y’ scores 0; ‘N’ scores 1.
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.GDS15_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.GDS15_SCALE),
                {
                    req.snomed(SnomedLookup.GDS15_SCORE): self.total_score(),
                }
            ))
        return codes
