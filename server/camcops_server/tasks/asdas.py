#!/usr/bin/env python

"""
camcops_server/tasks/asdas.py

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

**Ankylosing Spondylitis Disease Activity Score (ASDAS) task.**

"""

import math
from typing import Any, Dict, List, Optional, Type, Tuple

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import tr_qa, tr, answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    SummaryCategoryColType,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import TaskHasPatientMixin, Task
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerAxisTick,
    TrackerInfo,
    TrackerLabel,
)

import cardinal_pythonlib.rnc_web as ws
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy import Column, Float
from sqlalchemy.ext.declarative import DeclarativeMeta


class AsdasMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Asdas'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:

        add_multiple_columns(
            cls, "q", 1, cls.N_SCALE_QUESTIONS,
            minimum=0, maximum=10,
            comment_fmt="Q{n} - {s}",
            comment_strings=[
                "back pain 0-10 (None - very severe)",
                "morning stiffness 0-10 (None - 2+ hours)",
                "patient global 0-10 (Not active - very active)",
                "peripheral pain 0-10 (None - very severe)",
            ]
        )

        setattr(
            cls, cls.CRP_FIELD_NAME,
            Column(cls.CRP_FIELD_NAME, Float, comment="CRP (mg/L)")
        )

        setattr(
            cls, cls.ESR_FIELD_NAME,
            Column(cls.ESR_FIELD_NAME, Float, comment="ESR (mm/h)")
        )

        super().__init__(name, bases, classdict)


class Asdas(TaskHasPatientMixin,
            Task,
            metaclass=AsdasMetaclass):
    __tablename__ = "asdas"
    shortname = "ASDAS"
    provides_trackers = True

    N_SCALE_QUESTIONS = 4
    MAX_SCORE_SCALE = 10
    N_QUESTIONS = 6
    SCALE_FIELD_NAMES = strseq("q", 1, N_SCALE_QUESTIONS)
    CRP_FIELD_NAME = "q5"
    ESR_FIELD_NAME = "q6"

    INACTIVE_MODERATE_CUTOFF = 1.3
    MODERATE_HIGH_CUTOFF = 2.1
    HIGH_VERY_HIGH_CUTOFF = 3.5

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Ankylosing Spondylitis Disease Activity Score")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="asdas_crp", coltype=Float(),
                value=self.asdas_crp(),
                comment="ASDAS-CRP"),
            SummaryElement(
                name="activity_state_crp", coltype=SummaryCategoryColType,
                value=self.activity_state(req, self.asdas_crp()),
                comment="Activity state (CRP)"),
            SummaryElement(
                name="asdas_esr", coltype=Float(),
                value=self.asdas_esr(),
                comment="ASDAS-ESR"),
            SummaryElement(
                name="activity_state_esr", coltype=SummaryCategoryColType,
                value=self.activity_state(req, self.asdas_esr()),
                comment="Activity state (ESR)"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.SCALE_FIELD_NAMES):
            return False

        crp = getattr(self, self.CRP_FIELD_NAME)
        esr = getattr(self, self.ESR_FIELD_NAME)

        if crp is None and esr is None:
            return False

        if not self.field_contents_valid():
            return False

        return True

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        axis_min = -0.5
        axis_max = 7.5
        axis_ticks = [TrackerAxisTick(n, str(n))
                      for n in range(0, int(axis_max) + 1)]

        horizontal_lines = [
            self.HIGH_VERY_HIGH_CUTOFF,
            self.MODERATE_HIGH_CUTOFF,
            self.INACTIVE_MODERATE_CUTOFF,
            0,
        ]

        horizontal_labels = [
            TrackerLabel(5.25, self.wxstring(req, "very_high")),
            TrackerLabel(2.8, self.wxstring(req, "high")),
            TrackerLabel(1.7, self.wxstring(req, "moderate")),
            TrackerLabel(0.65, self.wxstring(req, "inactive")),
        ]

        return [
            TrackerInfo(
                value=self.asdas_crp(),
                plot_label="ASDAS-CRP",
                axis_label="ASDAS-CRP",
                axis_min=axis_min,
                axis_max=axis_max,
                axis_ticks=axis_ticks,
                horizontal_lines=horizontal_lines,
                horizontal_labels=horizontal_labels,
            ),
            TrackerInfo(
                value=self.asdas_esr(),
                plot_label="ASDAS-ESR",
                axis_label="ASDAS-ESR",
                axis_min=axis_min,
                axis_max=axis_max,
                axis_ticks=axis_ticks,
                horizontal_lines=horizontal_lines,
                horizontal_labels=horizontal_labels,
            ),
        ]

    def back_pain(self) -> float:
        return getattr(self, "q1")

    def morning_stiffness(self) -> float:
        return getattr(self, "q2")

    def patient_global(self) -> float:
        return getattr(self, "q3")

    def peripheral_pain(self) -> float:
        return getattr(self, "q4")

    def asdas_crp(self) -> Optional[float]:
        crp = getattr(self, self.CRP_FIELD_NAME)

        if crp is None:
            return None

        crp = max(crp, 2.0)

        return (
            0.12 * self.back_pain() +
            0.06 * self.morning_stiffness() +
            0.11 * self.patient_global() +
            0.07 * self.peripheral_pain() +
            0.58 * math.log(crp + 1)
        )

    def asdas_esr(self) -> Optional[float]:
        esr = getattr(self, self.ESR_FIELD_NAME)
        if esr is None:
            return None

        return (
            0.08 * self.back_pain() +
            0.07 * self.morning_stiffness() +
            0.11 * self.patient_global() +
            0.09 * self.peripheral_pain() +
            0.29 * math.sqrt(esr)
        )

    def activity_state(self, req: CamcopsRequest, measurement: Any) -> str:
        if measurement is None:
            return self.wxstring(req, "n_a")

        if measurement < self.INACTIVE_MODERATE_CUTOFF:
            return self.wxstring(req, "inactive")

        if measurement < self.MODERATE_HIGH_CUTOFF:
            return self.wxstring(req, "moderate")

        if measurement > self.HIGH_VERY_HIGH_CUTOFF:
            return self.wxstring(req, "very_high")

        return self.wxstring(req, "high")

    def get_task_html(self, req: CamcopsRequest) -> str:
        rows = ""
        for q_num in range(1, self.N_QUESTIONS + 1):
            q_field = "q" + str(q_num)
            qtext = self.wxstring(req, q_field)
            if q_num <= 4:  # not for ESR, CRP
                min_text = self.wxstring(req, q_field + "_min")
                max_text = self.wxstring(req, q_field + "_max")
                qtext += f" <i>(0 = {min_text}, 10 = {max_text})</i>"
            question_cell = f"{q_num}. {qtext}"
            score = getattr(self, q_field)

            rows += tr_qa(question_cell, score)

        asdas_crp = ws.number_to_dp(self.asdas_crp(), 2, default="?")
        asdas_esr = ws.number_to_dp(self.asdas_esr(), 2, default="?")

        html = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {asdas_crp}
                    {asdas_esr}
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
                [1] &lt;1.3 Inactive disease,
                    &lt;2.1 Moderate disease activity,
                    2.1-3.5 High disease activity,
                    &gt;3.5 Very high disease activity.<br>
                [2] 0.12 × back pain +
                    0.06 × duration of morning stiffness +
                    0.11 × patient global +
                    0.07 × peripheral pain +
                    0.58 × ln(CRP + 1).
                    CRP units: mg/L. When CRP&lt;2mg/L, use 2mg/L to calculate
                    ASDAS-CRP.<br>
                [3] 0.08 x back pain +
                    0.07 x duration of morning stiffness +
                    0.11 x patient global +
                    0.09 x peripheral pain +
                    0.29 x √(ESR).
                    ESR units: mm/h.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            asdas_crp=tr(
                self.wxstring(req, "asdas_crp") + " <sup>[1][2]</sup>",
                "{} ({})".format(
                    answer(asdas_crp),
                    self.activity_state(req, self.asdas_crp())
                )
            ),
            asdas_esr=tr(
                self.wxstring(req, "asdas_esr") + " <sup>[1][3]</sup>",
                "{} ({})".format(
                    answer(asdas_esr),
                    self.activity_state(req, self.asdas_esr())
                )
            ),
            rows=rows,
        )
        return html
