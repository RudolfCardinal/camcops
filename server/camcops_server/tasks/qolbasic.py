#!/usr/bin/env python

"""
camcops_server/tasks/qolbasic.py

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

from typing import List, Optional

from cardinal_pythonlib.maths_py import mean
import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.sqltypes import Float

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import answer, identity, tr
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# QoL-Basic
# =============================================================================

DP = 3


class QolBasic(TaskHasPatientMixin, Task):
    """
    Server implementation of the QoL-Basic task.
    """
    __tablename__ = "qolbasic"
    shortname = "QoL-Basic"
    provides_trackers = True

    tto = CamcopsColumn(
        "tto", Float,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=10),
        comment="Time trade-off (QoL * 10). Prompt: ... Indicate... the "
                "number of years in full health [0-10] that you think is "
                "of equal value to 10 years in your current health state."
    )
    rs = CamcopsColumn(
        "rs", Float,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=100),
        comment="Rating scale (QoL * 100). Prompt: Mark the point on the "
                "scale [0-100] that you feel best illustrates your current "
                "quality of life."
    )

    TASK_FIELDS = ["tto", "rs"]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Quality of Life: basic assessment")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [
            TrackerInfo(
                value=self.get_tto_qol(),
                plot_label="Quality of life: time trade-off",
                axis_label="TTO QoL (0-1)",
                axis_min=0,
                axis_max=1
            ),
            TrackerInfo(
                value=self.get_rs_qol(),
                plot_label="Quality of life: rating scale",
                axis_label="RS QoL (0-1)",
                axis_min=0,
                axis_max=1
            ),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        tto_qol = self.get_tto_qol()
        rs_qol = self.get_rs_qol()
        mean_qol = mean([tto_qol, rs_qol])
        return [CtvInfo(
            content=(
                f"Quality of life: time trade-off "
                f"{ws.number_to_dp(tto_qol, DP)}, "
                f"rating scale {ws.number_to_dp(rs_qol, DP)}, "
                f"mean {ws.number_to_dp(mean_qol, DP)}."
            )
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="tto_qol", coltype=Float(),
                value=self.get_tto_qol(),
                comment="Quality of life (0-1), from time trade-off method"),
            SummaryElement(
                name="rs_qol", coltype=Float(),
                value=self.get_rs_qol(),
                comment="Quality of life (0-1), from rating scale method"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(QolBasic.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def get_tto_qol(self) -> Optional[float]:
        return self.tto / 10 if self.tto is not None else None

    def get_rs_qol(self) -> Optional[float]:
        return self.rs / 100 if self.rs is not None else None

    def get_task_html(self, req: CamcopsRequest) -> str:
        tto_qol = self.get_tto_qol()
        rs_qol = self.get_rs_qol()
        mean_qol = mean([tto_qol, rs_qol])
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {mean_qol}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Quality of life (QoL) has anchor values of 0 (none) and 1
                (perfect health), and can be asked about in several ways.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="33%">Scale</th>
                    <th width="33%">Answer</th>
                    <td width="33%">Implied QoL</th>
                </tr>
                {tto}
                {rs}
            </table>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            mean_qol=tr(
                "Mean QoL",
                answer(ws.number_to_dp(mean_qol, DP, default=None),
                       formatter_answer=identity)
            ),
            tto=tr(
                self.wxstring(req, "tto_q_s"),
                answer(ws.number_to_dp(self.tto, DP, default=None)),
                answer(ws.number_to_dp(tto_qol, DP, default=None),
                       formatter_answer=identity)
            ),
            rs=tr(
                self.wxstring(req, "rs_q_s"),
                answer(ws.number_to_dp(self.rs, DP, default=None)),
                answer(ws.number_to_dp(rs_qol, DP, default=None),
                       formatter_answer=identity)
            ),
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [SnomedExpression(req.snomed(SnomedLookup.QOL_SCALE))]
