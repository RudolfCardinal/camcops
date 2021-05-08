#!/usr/bin/env python

"""
camcops_server/tasks/basdai.py

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

**Bath Ankylosing Spondylitis Disease Activity Index (BASDAI) task.**

"""

import statistics
from typing import Any, Dict, List, Optional, Type, Tuple

import cardinal_pythonlib.rnc_web as ws
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Float

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import tr_qa, tr, answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import TaskHasPatientMixin, Task
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerAxisTick,
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# BASDAI
# =============================================================================

class BasdaiMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Basdai'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        add_multiple_columns(
            cls, "q", 1, cls.N_QUESTIONS, coltype=Float,
            minimum=0, maximum=10,
            comment_fmt="Q{n} - {s}",
            comment_strings=[
                "fatigue/tiredness 0-10 (none - very severe)",
                "AS neck, back, hip pain 0-10 (none - very severe)",
                "other joint pain/swelling 0-10 (none - very severe)",
                "discomfort from tender areas 0-10 (none - very severe)",
                "morning stiffness level 0-10 (none - very severe)",
                "morning stiffness duration 0-10 (none - 2 or more hours)",
            ]
        )

        super().__init__(name, bases, classdict)


class Basdai(TaskHasPatientMixin,
             Task,
             metaclass=BasdaiMetaclass):
    __tablename__ = "basdai"
    shortname = "BASDAI"
    provides_trackers = True

    N_QUESTIONS = 6
    FIELD_NAMES = strseq("q", 1, N_QUESTIONS)

    MINIMUM = 0.0
    ACTIVE_CUTOFF = 4.0
    MAXIMUM = 10.0

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Bath Ankylosing Spondylitis Disease Activity Index")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="basdai", coltype=Float(),
                value=self.basdai(),
                comment="BASDAI"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.FIELD_NAMES):
            return False

        if not self.field_contents_valid():
            return False

        return True

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        axis_min = self.MINIMUM - 0.5
        axis_max = self.MAXIMUM + 0.5
        axis_ticks = [TrackerAxisTick(n, str(n))
                      for n in range(0, int(axis_max) + 1)]

        horizontal_lines = [
            self.MAXIMUM,
            self.ACTIVE_CUTOFF,
            self.MINIMUM,
        ]

        horizontal_labels = [
            TrackerLabel(self.ACTIVE_CUTOFF + 0.5,
                         self.wxstring(req, "active")),
            TrackerLabel(self.ACTIVE_CUTOFF - 0.5,
                         self.wxstring(req, "inactive")),
        ]

        return [
            TrackerInfo(
                value=self.basdai(),
                plot_label="BASDAI",
                axis_label="BASDAI",
                axis_min=axis_min,
                axis_max=axis_max,
                axis_ticks=axis_ticks,
                horizontal_lines=horizontal_lines,
                horizontal_labels=horizontal_labels,
            ),
        ]

    def basdai(self) -> Optional[float]:
        """
        Calculating the BASDAI
        A. Add scores for questions 1 – 4
        B. Calculate the mean for questions 5 and 6
        C. Add A and B and divide by 5

        The higher the BASDAI score, the more severe the patient’s disability
        due to their AS.
        """
        if not self.is_complete():
            return None

        score_a_field_names = strseq("q", 1, 4)
        score_b_field_names = strseq("q", 5, 6)

        a = sum([getattr(self, q) for q in score_a_field_names])
        b = statistics.mean([getattr(self, q) for q in score_b_field_names])

        return (a + b) / 5

    def activity_state(self, req: CamcopsRequest) -> str:
        basdai = self.basdai()

        if basdai is None:
            return "?"

        if basdai < self.ACTIVE_CUTOFF:
            return self.wxstring(req, "inactive")

        return self.wxstring(req, "active")

    def get_task_html(self, req: CamcopsRequest) -> str:
        rows = ""
        for q_num in range(1, self.N_QUESTIONS + 1):
            q_field = "q" + str(q_num)
            qtext = self.xstring(req, q_field)  # includes HTML
            min_text = self.wxstring(req, q_field + "_min")
            max_text = self.wxstring(req, q_field + "_max")
            qtext += f" <i>(0 = {min_text}, 10 = {max_text})</i>"
            question_cell = f"{q_num}. {qtext}"
            score = getattr(self, q_field)

            rows += tr_qa(question_cell, score)

        basdai = ws.number_to_dp(self.basdai(), 1, default="?")

        html = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {basdai}
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
                [1] (A) Add scores for questions 1–4.
                    (B) Calculate the mean for questions 5 and 6.
                    (C) Add A and B and divide by 5, giving a total in the
                        range 0–10.
                    &lt;4.0 suggests inactive disease,
                    &ge;4.0 suggests active disease.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            basdai=tr(
                self.wxstring(req, "basdai") + " <sup>[1]</sup>",
                "{} ({})".format(
                    answer(basdai),
                    self.activity_state(req)
                )
            ),
            rows=rows,
        )
        return html
