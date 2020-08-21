#!/usr/bin/env python

"""
camcops_server/tasks/das28.py

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

**Disease Activity Score-28 (DAS28) task.**

"""

import math
from typing import Any, Dict, List, Optional, Type, Tuple

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import (
    answer,
    table_row,
    th,
    td,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BoolColumn,
    CamcopsColumn,
    PermittedValueChecker,
    SummaryCategoryColType,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
    TaskHasClinicianMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerAxisTick,
    TrackerInfo,
    TrackerLabel,
)

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy import Column, Float, Integer
from sqlalchemy.ext.declarative import DeclarativeMeta


class Das28Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Das28'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        for field_name in cls.get_joint_field_names():
            setattr(cls, field_name,
                    BoolColumn(field_name, comment="0 no, 1 yes"))

        setattr(
            cls, 'vas',
            CamcopsColumn(
                'vas',
                Integer,
                comment="Patient assessment of health (0-100mm)",
                permitted_value_checker=PermittedValueChecker(
                    minimum=0, maximum=100)
            )
        )

        setattr(
            cls, 'crp',
            Column('crp', Float, comment="CRP (0-300 mg/L)")
        )

        setattr(
            cls, 'esr',
            Column('esr', Float, comment="ESR (1-300 mm/h)")
        )

        super().__init__(name, bases, classdict)


class Das28(TaskHasPatientMixin,
            TaskHasClinicianMixin,
            Task,
            metaclass=Das28Metaclass):
    __tablename__ = "das28"
    shortname = "DAS28"
    provides_trackers = True

    JOINTS = (
        ['shoulder', 'elbow', 'wrist'] +
        [f"mcp_{n}" for n in range(1, 6)] +
        [f"pip_{n}" for n in range(1, 6)] +
        ['knee']
    )

    SIDES = ['left', 'right']
    STATES = ['swollen', 'tender']

    OTHER_FIELD_NAMES = ['vas', 'crp', 'esr']

    # as recommended by https://rmdopen.bmj.com/content/3/1/e000382
    CRP_REMISSION_LOW_CUTOFF = 2.4
    CRP_LOW_MODERATE_CUTOFF = 2.9
    CRP_MODERATE_HIGH_CUTOFF = 4.6

    # https://onlinelibrary.wiley.com/doi/full/10.1002/acr.21649
    # (has same cutoffs for CRP)
    ESR_REMISSION_LOW_CUTOFF = 2.6
    ESR_LOW_MODERATE_CUTOFF = 3.2
    ESR_MODERATE_HIGH_CUTOFF = 5.1

    @classmethod
    def field_name(cls, side, joint, state) -> str:
        return f"{side}_{joint}_{state}"

    @classmethod
    def get_joint_field_names(cls) -> List:
        field_names = []

        for joint in cls.JOINTS:
            for side in cls.SIDES:
                for state in cls.STATES:
                    field_names.append(cls.field_name(side, joint, state))

        return field_names

    @classmethod
    def get_all_field_names(cls) -> List:
        return cls.get_joint_field_names() + cls.OTHER_FIELD_NAMES

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Disease Activity Score-28")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="das28_crp", coltype=Float(),
                value=self.das28_crp(),
                comment="DAS28-CRP"),
            SummaryElement(
                name="activity_state_crp", coltype=SummaryCategoryColType,
                value=self.activity_state_crp(req, self.das28_crp()),
                comment="Activity state (CRP)"),
            SummaryElement(
                name="das28_esr", coltype=Float(),
                value=self.das28_esr(),
                comment="DAS28-ESR"),
            SummaryElement(
                name="activity_state_esr", coltype=SummaryCategoryColType,
                value=self.activity_state_esr(req, self.das28_esr()),
                comment="Activity state (ESR)"),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.get_joint_field_names() + ['vas']):
            return False

        # noinspection PyUnresolvedReferences
        if self.crp is None and self.esr is None:
            return False

        if not self.field_contents_valid():
            return False

        return True

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [
            self.get_crp_tracker(req),
            self.get_esr_tracker(req),
        ]

    def get_crp_tracker(self, req: CamcopsRequest) -> TrackerInfo:
        axis_min = -0.5
        axis_max = 9.0
        axis_ticks = [TrackerAxisTick(n, str(n))
                      for n in range(0, int(axis_max) + 1)]

        horizontal_lines = [
            self.CRP_MODERATE_HIGH_CUTOFF,
            self.CRP_LOW_MODERATE_CUTOFF,
            self.CRP_REMISSION_LOW_CUTOFF,
            0,
        ]

        horizontal_labels = [
            TrackerLabel(6.8, self.wxstring(req, "high")),
            TrackerLabel(3.75, self.wxstring(req, "moderate")),
            TrackerLabel(2.65, self.wxstring(req, "low")),
            TrackerLabel(1.2, self.wxstring(req, "remission")),
        ]

        return TrackerInfo(
            value=self.das28_crp(),
            plot_label="DAS28-CRP",
            axis_label="DAS28-CRP",
            axis_min=axis_min,
            axis_max=axis_max,
            axis_ticks=axis_ticks,
            horizontal_lines=horizontal_lines,
            horizontal_labels=horizontal_labels,
        )

    def get_esr_tracker(self, req: CamcopsRequest) -> TrackerInfo:
        axis_min = -0.5
        axis_max = 10.0
        axis_ticks = [TrackerAxisTick(n, str(n))
                      for n in range(0, int(axis_max) + 1)]

        horizontal_lines = [
            self.ESR_MODERATE_HIGH_CUTOFF,
            self.ESR_LOW_MODERATE_CUTOFF,
            self.ESR_REMISSION_LOW_CUTOFF,
            0,
        ]

        horizontal_labels = [
            TrackerLabel(7.55, self.wxstring(req, "high")),
            TrackerLabel(4.15, self.wxstring(req, "moderate")),
            TrackerLabel(2.9, self.wxstring(req, "low")),
            TrackerLabel(1.3, self.wxstring(req, "remission")),
        ]

        return TrackerInfo(
            value=self.das28_esr(),
            plot_label="DAS28-ESR",
            axis_label="DAS28-ESR",
            axis_min=axis_min,
            axis_max=axis_max,
            axis_ticks=axis_ticks,
            horizontal_lines=horizontal_lines,
            horizontal_labels=horizontal_labels,
        )

    def swollen_joint_count(self):
        return self.count_booleans(
            [n for n in self.get_joint_field_names() if n.endswith("swollen")]
        )

    def tender_joint_count(self):
        return self.count_booleans(
            [n for n in self.get_joint_field_names() if n.endswith("tender")]
        )

    def das28_crp(self) -> Optional[float]:
        # noinspection PyUnresolvedReferences
        if self.crp is None or self.vas is None:
            return None

        # noinspection PyUnresolvedReferences
        return (
            0.56 * math.sqrt(self.tender_joint_count()) +
            0.28 * math.sqrt(self.swollen_joint_count()) +
            0.36 * math.log(self.crp + 1) +
            0.014 * self.vas +
            0.96
        )

    def das28_esr(self) -> Optional[float]:
        # noinspection PyUnresolvedReferences
        if self.esr is None or self.vas is None:
            return None

        # noinspection PyUnresolvedReferences
        return (
            0.56 * math.sqrt(self.tender_joint_count()) +
            0.28 * math.sqrt(self.swollen_joint_count()) +
            0.70 * math.log(self.esr) +
            0.014 * self.vas
        )

    def activity_state_crp(self, req: CamcopsRequest, measurement: Any) -> str:
        if measurement is None:
            return self.wxstring(req, "n_a")

        if measurement < self.CRP_REMISSION_LOW_CUTOFF:
            return self.wxstring(req, "remission")

        if measurement < self.CRP_LOW_MODERATE_CUTOFF:
            return self.wxstring(req, "low")

        if measurement > self.CRP_MODERATE_HIGH_CUTOFF:
            return self.wxstring(req, "high")

        return self.wxstring(req, "moderate")

    def activity_state_esr(self, req: CamcopsRequest, measurement: Any) -> str:
        if measurement is None:
            return self.wxstring(req, "n_a")

        if measurement < self.ESR_REMISSION_LOW_CUTOFF:
            return self.wxstring(req, "remission")

        if measurement < self.ESR_LOW_MODERATE_CUTOFF:
            return self.wxstring(req, "low")

        if measurement > self.ESR_MODERATE_HIGH_CUTOFF:
            return self.wxstring(req, "high")

        return self.wxstring(req, "moderate")

    def get_task_html(self, req: CamcopsRequest) -> str:
        sides_strings = [self.wxstring(req, s) for s in self.SIDES]
        states_strings = [self.wxstring(req, s) for s in self.STATES]

        joint_rows = table_row([""] + sides_strings,
                               colspans=[1, 2, 2])

        joint_rows += table_row([""] + states_strings * 2)

        for joint in self.JOINTS:
            cells = [th(self.wxstring(req, joint))]
            for side in self.SIDES:
                for state in self.STATES:
                    value = "?"
                    fval = getattr(self, self.field_name(side, joint, state))
                    if fval is not None:
                        value = "✓" if fval else "×"

                    cells.append(td(value))

            joint_rows += tr(*cells, literal=True)

        das28_crp = self.das28_crp()
        das28_esr = self.das28_esr()

        other_rows = "".join([tr_qa(self.wxstring(req, f), getattr(self, f))
                              for f in self.OTHER_FIELD_NAMES])

        html = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {das28_crp}
                    {das28_esr}
                    {swollen_joint_count}
                    {tender_joint_count}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                {joint_rows}
            </table>
            <table class="{CssClass.TASKDETAIL}">
                {other_rows}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] 0.56 × √(tender joint count) +
                    0.28 × √(swollen joint count) +
                    0.36 × ln(CRP + 1) +
                    0.014 x VAS disease activity +
                    0.96.
                    CRP 0–300 mg/L. VAS: 0–100mm.<br>
                    Cutoffs:
                    &lt;2.4 remission,
                    &lt;2.9 low disease activity,
                    2.9–4.6 moderate disease activity,
                    &gt;4.6 high disease activity.<br>
                [2] 0.56 × √(tender joint count) +
                    0.28 × √(swollen joint count) +
                    0.70 × ln(ESR) +
                    0.014 x VAS disease activity.
                    ESR 1–300 mm/h. VAS: 0–100mm.<br>
                    Cutoffs:
                    &lt;2.6 remission,
                    &lt;3.2 low disease activity,
                    3.2–5.1 moderate disease activity,
                    &gt;5.1 high disease activity.<br>
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            das28_crp=tr(
                self.wxstring(req, "das28_crp") + " <sup>[1]</sup>",
                "{} ({})".format(
                    answer(ws.number_to_dp(das28_crp, 2, default="?")),
                    self.activity_state_crp(req, das28_crp)
                )
            ),
            das28_esr=tr(
                self.wxstring(req, "das28_esr") + " <sup>[2]</sup>",
                "{} ({})".format(
                    answer(ws.number_to_dp(das28_esr, 2, default="?")),
                    self.activity_state_esr(req, das28_esr)
                )
            ),
            swollen_joint_count=tr(
                self.wxstring(req, "swollen_count"),
                answer(self.swollen_joint_count())
            ),
            tender_joint_count=tr(
                self.wxstring(req, "tender_count"),
                answer(self.tender_joint_count())
            ),
            joint_rows=joint_rows,
            other_rows=other_rows
        )
        return html
