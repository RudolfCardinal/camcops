#!/usr/bin/env python

"""
camcops_server/tasks/rapid3.py

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

**Routine Assessment of Patient Index Data (RAPID 3) task.**

"""

from typing import Any, Dict, List, Optional, Type, Tuple

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy import Float, Integer
from sqlalchemy.ext.declarative import DeclarativeMeta

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import answer, tr_qa, tr, tr_span_col
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
    ZERO_TO_THREE_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import TaskHasPatientMixin, Task
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerAxisTick,
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# RAPID 3
# =============================================================================

class Rapid3Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type["Rapid3"],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        comment_strings = [
            "get dressed",
            "get in bed",
            "lift full cup",
            "walk outdoors",
            "wash body",
            "bend down",
            "turn taps",
            "get in car",
            "walk 2 miles",
            "sports",
            "sleep",
            "anxiety",
            "depression",
        ]
        score_comment = "(0 without any difficulty - 3 unable to do)"

        for q_index, q_fieldname in cls.q1_all_indexed_fieldnames():
            setattr(cls, q_fieldname, CamcopsColumn(
                q_fieldname, Integer,
                permitted_value_checker=ZERO_TO_THREE_CHECKER,
                comment="{} ({}) {}".format(
                    q_fieldname.capitalize(),
                    comment_strings[q_index],
                    score_comment
                )
            ))

        permitted_scale_values = [v / 2.0 for v in range(0, 20 + 1)]

        setattr(cls, "q2", CamcopsColumn(
            "q2", Float,
            permitted_value_checker=PermittedValueChecker(
                permitted_values=permitted_scale_values
            ),
            comment=("Q2 (pain tolerance) (0 no pain - 10 pain as bad as "
                     "it could be")
        ))

        setattr(cls, "q3", CamcopsColumn(
            "q3", Float,
            permitted_value_checker=PermittedValueChecker(
                permitted_values=permitted_scale_values
            ),
            comment="Q3 (patient global estimate) (0 very well - very poorly)"
        ))

        super().__init__(name, bases, classdict)


class Rapid3(TaskHasPatientMixin,
             Task,
             metaclass=Rapid3Metaclass):
    __tablename__ = "rapid3"
    shortname = "RAPID3"
    provides_trackers = True

    N_Q1_QUESTIONS = 13
    N_Q1_SCORING_QUESTIONS = 10

    # > 12 = HIGH
    # 6.1 - 12 = MODERATE
    # 3.1 - 6 = LOW
    # <= 3 = REMISSION

    MINIMUM = 0
    NEAR_REMISSION_MAX = 3
    LOW_SEVERITY_MAX = 6
    MODERATE_SEVERITY_MAX = 12
    MAXIMUM = 30

    @classmethod
    def q1_indexed_letters(cls, last: int) -> List[Tuple[int, str]]:
        return [(i, chr(i + ord("a"))) for i in range(0, last)]

    @classmethod
    def q1_indexed_fieldnames(cls, last: int) -> List[Tuple[int, str]]:
        return [(i, f"q1{c}") for (i, c) in cls.q1_indexed_letters(last)]

    @classmethod
    def q1_all_indexed_fieldnames(cls) -> List[Tuple[int, str]]:
        return [(i, f) for (i, f) in
                cls.q1_indexed_fieldnames(cls.N_Q1_QUESTIONS)]

    @classmethod
    def q1_all_fieldnames(cls) -> List[str]:
        return [f for (i, f) in
                cls.q1_indexed_fieldnames(cls.N_Q1_QUESTIONS)]

    @classmethod
    def q1_all_letters(cls) -> List[str]:
        return [c for (i, c) in
                cls.q1_indexed_letters(cls.N_Q1_QUESTIONS)]

    @classmethod
    def q1_scoring_fieldnames(cls) -> List[str]:
        return [f for (i, f) in
                cls.q1_indexed_fieldnames(cls.N_Q1_SCORING_QUESTIONS)]

    @classmethod
    def all_fieldnames(cls) -> List[str]:
        return cls.q1_all_fieldnames() + ["q2", "q3"]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Routine Assessment of Patient Index Data")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="rapid3", coltype=Float(),
                value=self.rapid3(),
                comment="RAPID3"),
        ]

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        axis_min = self.MINIMUM - 0.5
        axis_max = self.MAXIMUM + 0.5
        axis_ticks = [TrackerAxisTick(n, str(n))
                      for n in range(0, int(axis_max) + 1, 2)]

        horizontal_lines = [
            self.MAXIMUM,
            self.MODERATE_SEVERITY_MAX,
            self.LOW_SEVERITY_MAX,
            self.NEAR_REMISSION_MAX,
            self.MINIMUM,
        ]

        horizontal_labels = [
            TrackerLabel(self.MODERATE_SEVERITY_MAX + 8.0,
                         self.wxstring(req, "high_severity")),
            TrackerLabel(self.MODERATE_SEVERITY_MAX - 3.0,
                         self.wxstring(req, "moderate_severity")),
            TrackerLabel(self.LOW_SEVERITY_MAX - 1.5,
                         self.wxstring(req, "low_severity")),
            TrackerLabel(self.NEAR_REMISSION_MAX - 1.5,
                         self.wxstring(req, "near_remission")),
        ]

        return [
            TrackerInfo(
                value=self.rapid3(),
                plot_label="RAPID3",
                axis_label="RAPID3",
                axis_min=axis_min,
                axis_max=axis_max,
                axis_ticks=axis_ticks,
                horizontal_lines=horizontal_lines,
                horizontal_labels=horizontal_labels,
            ),
        ]

    def rapid3(self) -> Optional[float]:
        if not self.is_complete():
            return None

        return (self.functional_status() +
                self.pain_tolerance() +
                self.global_estimate())

    def functional_status(self) -> float:
        return round(self.sum_fields(self.q1_scoring_fieldnames()) / 3, 1)

    def pain_tolerance(self) -> float:
        # noinspection PyUnresolvedReferences
        return self.q2

    def global_estimate(self) -> float:
        # noinspection PyUnresolvedReferences
        return self.q3

    def is_complete(self) -> bool:
        if self.any_fields_none(self.all_fieldnames()):
            return False

        if not self.field_contents_valid():
            return False

        return True

    def get_task_html(self, req: CamcopsRequest) -> str:
        rows = tr_span_col(
            f'{self.wxstring(req, "q1")}<br>'
            f'{self.wxstring(req, "q1sub")}',
            cols=2
        )
        for letter in self.q1_all_letters():
            q_fieldname = f"q1{letter}"

            qtext = self.wxstring(req, q_fieldname)
            score = getattr(self, q_fieldname)

            description = "?"
            if score is not None:
                description = self.wxstring(req, f"q1_option{score}")

            rows += tr_qa(qtext, f"{score} — {description}")

        for q_num in (2, 3):
            q_fieldname = f"q{q_num}"
            qtext = self.wxstring(req, q_fieldname)
            min_text = self.wxstring(req, f"{q_fieldname}_min")
            max_text = self.wxstring(req, f"{q_fieldname}_max")
            qtext += f" <i>(0.0 = {min_text}, 10.0 = {max_text})</i>"
            score = getattr(self, q_fieldname)

            rows += tr_qa(qtext, score)

        rapid3 = ws.number_to_dp(self.rapid3(), 1, default="?")

        html = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {rapid3}
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
                [1] Add scores for questions 1a–1j (ten questions each scored
                    0–3), divide by 3, and round to 1 decimal place (giving a
                    score for Q1 in the range 0–10). Then add this to scores
                    for Q2 and Q3 (each scored 0–10) to get the RAPID3
                    cumulative score (0–30), as shown here.
                    Interpretation of the cumulative score:
                    ≤3: Near remission (NR).
                    3.1–6: Low severity (LS).
                    6.1–12: Moderate severity (MS).
                    >12: High severity (HS).

                    Note also: questions 1k–1m are each scored 0, 1.1, 2.2, or
                    3.3 in the PDF/paper version of the RAPID3, but do not
                    contribute to the formal score. They are shown here with
                    values 0, 1, 2, 3 (and, similarly, do not contribute to
                    the overall score).

            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            rapid3=tr(
                self.wxstring(req, "rapid3") + " (0–30) <sup>[1]</sup>",
                "{} ({})".format(
                    answer(rapid3),
                    self.disease_severity(req)
                )
            ),
            rows=rows,
        )
        return html

    def disease_severity(self, req: CamcopsRequest) -> str:
        rapid3 = self.rapid3()

        if rapid3 is None:
            return self.wxstring(req, "n_a")

        if rapid3 <= self.NEAR_REMISSION_MAX:
            return self.wxstring(req, "near_remission")

        if rapid3 <= self.LOW_SEVERITY_MAX:
            return self.wxstring(req, "low_severity")

        if rapid3 <= self.MODERATE_SEVERITY_MAX:
            return self.wxstring(req, "moderate_severity")

        return self.wxstring(req, "high_severity")
