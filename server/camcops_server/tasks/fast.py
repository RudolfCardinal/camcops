#!/usr/bin/env python

"""
camcops_server/tasks/fast.py

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
from sqlalchemy.sql.sqltypes import Boolean, Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, get_yes_no, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# FAST
# =============================================================================

class FastMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Fast'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=4,
            comment_fmt="Q{n}. {s} (0-4, higher worse)",
            comment_strings=[
                "M>8, F>6 drinks", "unable to remember",
                "failed to do what was expected", "others concerned"
            ]
        )
        super().__init__(name, bases, classdict)


class Fast(TaskHasPatientMixin, Task,
           metaclass=FastMetaclass):
    """
    Server implementation of the FAST task.
    """
    __tablename__ = "fast"
    shortname = "FAST"

    NQUESTIONS = 4
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 16

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Fast Alcohol Screening Test")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="FAST total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        classification = "positive" if self.is_positive() else "negative"
        return [CtvInfo(content=(
            f"FAST total score {self.total_score()}/{self.MAX_SCORE} "
            f"({classification})"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{self.MAX_SCORE})"),
            SummaryElement(name="positive",
                           coltype=Boolean(),
                           value=self.is_positive(),
                           comment="FAST positive?"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    # noinspection PyUnresolvedReferences
    def is_positive(self) -> bool:
        if self.q1 is not None:
            if self.q1 == 0:
                return False
            if self.q1 >= 3:
                return True
        return self.total_score() >= 3

    # noinspection PyUnresolvedReferences
    def get_task_html(self, req: CamcopsRequest) -> str:
        main_dict = {
            None: None,
            0: "0 — " + self.wxstring(req, "q1to3_option0"),
            1: "1 — " + self.wxstring(req, "q1to3_option1"),
            2: "2 — " + self.wxstring(req, "q1to3_option2"),
            3: "3 — " + self.wxstring(req, "q1to3_option3"),
            4: "4 — " + self.wxstring(req, "q1to3_option4"),
        }
        q4_dict = {
            None: None,
            0: "0 — " + self.wxstring(req, "q4_option0"),
            2: "2 — " + self.wxstring(req, "q4_option2"),
            4: "4 — " + self.wxstring(req, "q4_option4"),
        }
        q_a = tr_qa(self.wxstring(req, "q1"), get_from_dict(main_dict, self.q1))  # noqa
        q_a += tr_qa(self.wxstring(req, "q2"), get_from_dict(main_dict, self.q2))  # noqa
        q_a += tr_qa(self.wxstring(req, "q3"), get_from_dict(main_dict, self.q3))  # noqa
        q_a += tr_qa(self.wxstring(req, "q4"), get_from_dict(q4_dict, self.q4))

        tr_total_score = tr(
            req.sstring(SS.TOTAL_SCORE),
            answer(self.total_score()) + f" / {self.MAX_SCORE}"
        )
        tr_positive = tr_qa(
            self.wxstring(req, "positive") + " <sup>[1]</sup>",
            get_yes_no(req, self.is_positive())
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_total_score}
                    {tr_positive}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Negative if Q1 = 0. Positive if Q1 ≥ 3. Otherwise positive
                    if total score ≥ 3.
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.FAST_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.FAST_SCALE),
                {
                    req.snomed(SnomedLookup.FAST_SCORE): self.total_score(),
                }
            ))
        return codes
