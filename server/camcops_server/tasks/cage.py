#!/usr/bin/env python

"""
camcops_server/tasks/cage.py

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
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import CharColType
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# CAGE
# =============================================================================

class CageMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Cage'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS, CharColType,
            pv=['Y', 'N'],
            comment_fmt="Q{n}, {s} (Y, N)",
            comment_strings=["C", "A", "G", "E"]
        )
        super().__init__(name, bases, classdict)


class Cage(TaskHasPatientMixin, Task,
           metaclass=CageMetaclass):
    """
    Server implementation of the CAGE task.
    """
    __tablename__ = "cage"
    shortname = "CAGE"
    provides_trackers = True

    NQUESTIONS = 4
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("CAGE Questionnaire")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="CAGE total score",
            axis_label=f"Total score (out of {self.NQUESTIONS})",
            axis_min=-0.5,
            axis_max=self.NQUESTIONS + 0.5,
            horizontal_lines=[1.5]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"CAGE score {self.total_score()}/{self.NQUESTIONS}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/{self.NQUESTIONS})"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(Cage.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def get_value(self, q: int) -> int:
        return 1 if getattr(self, "q" + str(q)) == "Y" else 0

    def total_score(self) -> int:
        total = 0
        for i in range(1, Cage.NQUESTIONS + 1):
            total += self.get_value(i)
        return total

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        exceeds_cutoff = score >= 2
        q_a = ""
        for q in range(1, Cage.NQUESTIONS + 1):
            q_a += tr_qa(str(q) + " — " + self.wxstring(req, "q" + str(q)),
                         getattr(self, "q" + str(q)))  # answer is itself Y/N/NULL  # noqa
        total_score = tr(
            req.sstring(SS.TOTAL_SCORE),
            answer(score) + f" / {self.NQUESTIONS}"
        )
        over_threshold = tr_qa(
            self.wxstring(req, "over_threshold"),
            get_yes_no(req, exceeds_cutoff)
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {total_score}
                    {over_threshold}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="70%">Question</th>
                    <th width="30%">Answer</th>
                </tr>
                {q_a}
            </table>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.CAGE_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.CAGE_SCALE),
                {
                    req.snomed(SnomedLookup.CAGE_SCORE): self.total_score(),
                }
            ))
        return codes
