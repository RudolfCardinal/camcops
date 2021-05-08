#!/usr/bin/env python

"""
camcops_server/tasks/bprse.py

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
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo, CTV_INCOMPLETE
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# BPRS-E
# =============================================================================

class BprseMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Bprse'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=7,
            comment_fmt="Q{n}, {s} (1-7, higher worse, or 0 for not assessed)",
            comment_strings=[
                "somatic concern", "anxiety", "depression", "suicidality",
                "guilt", "hostility", "elevated mood", "grandiosity",
                "suspiciousness", "hallucinations", "unusual thought content",
                "bizarre behaviour", "self-neglect", "disorientation",
                "conceptual disorganisation", "blunted affect",
                "emotional withdrawal", "motor retardation", "tension",
                "uncooperativeness", "excitement", "distractibility",
                "motor hyperactivity", "mannerisms and posturing"]
        )
        super().__init__(name, bases, classdict)


class Bprse(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
            metaclass=BprseMetaclass):
    """
    Server implementation of the BPRS-E task.
    """
    __tablename__ = "bprse"
    shortname = "BPRS-E"
    provides_trackers = True

    NQUESTIONS = 24
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 168

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Brief Psychiatric Rating Scale, Expanded")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="BPRS-E total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"BPRS-E total score {self.total_score()}/{self.MAX_SCORE}"
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
        return self.sum_fields(self.TASK_FIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        def bprs_string(x: str) -> str:
            return req.wxstring("bprs", x)

        main_dict = {
            None: None,
            0: "0 — " + bprs_string("old_option0"),
            1: "1 — " + bprs_string("old_option1"),
            2: "2 — " + bprs_string("old_option2"),
            3: "3 — " + bprs_string("old_option3"),
            4: "4 — " + bprs_string("old_option4"),
            5: "5 — " + bprs_string("old_option5"),
            6: "6 — " + bprs_string("old_option6"),
            7: "7 — " + bprs_string("old_option7")
        }

        q_a = ""
        for i in range(1, self.NQUESTIONS + 1):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(i) + "_s"),
                get_from_dict(main_dict, getattr(self, "q" + str(i)))
            )

        total_score = tr(
            req.sstring(SS.TOTAL_SCORE) +
            f" (0–{self.MAX_SCORE}; 24–{self.MAX_SCORE} if all rated)",
            answer(self.total_score())
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {total_score}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Each question has specific answer definitions (see e.g. tablet
                app).
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer <sup>[1]</sup></th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] All answers are in the range 1–7, or 0 (not assessed, for
                    some).
            </div>
        """
