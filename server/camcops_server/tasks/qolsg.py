#!/usr/bin/env python

"""
camcops_server/tasks/qolsg.py

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

from typing import List

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Float, Integer, String

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import get_yes_no_none, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    PendulumDateTimeAsIsoTextColType,
    ZERO_TO_ONE_CHECKER,
)
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# QoL-SG
# =============================================================================

DP = 3


class QolSG(TaskHasPatientMixin, Task):
    """
    Server implementation of the QoL-SG task.
    """
    __tablename__ = "qolsg"
    shortname = "QoL-SG"
    provides_trackers = True

    category_start_time = Column(
        "category_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Time categories were offered (ISO-8601)"
    )
    category_responded = CamcopsColumn(
        "category_responded", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Responded to category choice? (0 no, 1 yes)"
    )
    category_response_time = Column(
        "category_response_time", PendulumDateTimeAsIsoTextColType,
        comment="Time category was chosen (ISO-8601)"
    )
    category_chosen = Column(
        "category_chosen", String(length=len("medium")),
        comment="Category chosen: high (QoL > 1) "
                "medium (0 <= QoL <= 1) low (QoL < 0)"
    )
    gamble_fixed_option = Column(
        "gamble_fixed_option", String(length=len("current")),
        comment="Fixed option in gamble (current, healthy, dead)"
    )
    gamble_lottery_option_p = Column(
        "gamble_lottery_option_p", String(length=len("current")),
        comment="Gamble: option corresponding to p  "
                "(current, healthy, dead)"
    )
    gamble_lottery_option_q = Column(
        "gamble_lottery_option_q", String(length=len("current")),
        comment="Gamble: option corresponding to q  "
                "(current, healthy, dead) (q = 1 - p)"
    )
    gamble_lottery_on_left = CamcopsColumn(
        "gamble_lottery_on_left", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Gamble: lottery shown on the left (0 no, 1 yes)"
    )
    gamble_starting_p = CamcopsColumn(
        "gamble_starting_p", Float,
        permitted_value_checker=ZERO_TO_ONE_CHECKER,
        comment="Gamble: starting value of p"
    )
    gamble_start_time = Column(
        "gamble_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Time gamble was offered (ISO-8601)"
    )
    gamble_responded = CamcopsColumn(
        "gamble_responded", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Gamble was responded to? (0 no, 1 yes)"
    )
    gamble_response_time = Column(
        "gamble_response_time", PendulumDateTimeAsIsoTextColType,
        comment="Time subject responded to gamble (ISO-8601)"
    )
    gamble_p = CamcopsColumn(
        "gamble_p", Float,
        permitted_value_checker=ZERO_TO_ONE_CHECKER,
        comment="Final value of p"
    )
    utility = Column(
        "utility", Float,
        comment="Calculated utility, h"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Quality of Life: Standard Gamble")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.utility,
            plot_label="Quality of life: standard gamble",
            axis_label="QoL (0-1)",
            axis_min=0,
            axis_max=1
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"Quality of life: {ws.number_to_dp(self.utility, DP)}"
        )]

    def is_complete(self) -> bool:
        return (
            self.utility is not None and
            self.field_contents_valid()
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_qa("Utility",
                           ws.number_to_dp(self.utility, DP, default=None))}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Quality of life (QoL) has anchor values of 0 (none) and 1
                (perfect health). The Standard Gamble offers a trade-off to
                determine utility (QoL).
                Values &lt;0 and &gt;1 are possible with some gambles.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr><th width="50%">Measure</th><th width="50%">Value</th></tr>
        """
        h += tr_qa("Category choice: start time", self.category_start_time)
        h += tr_qa("Category choice: responded?",
                   get_yes_no_none(req, self.category_responded))
        h += tr_qa("Category choice: response time",
                   self.category_response_time)
        h += tr_qa("Category choice: category chosen", self.category_chosen)
        h += tr_qa("Gamble: fixed option", self.gamble_fixed_option)
        h += tr_qa("Gamble: lottery option for <i>p</i>",
                   self.gamble_lottery_option_p)
        h += tr_qa("Gamble: lottery option for <i>q</i> = 1 – <i>p</i>",
                   self.gamble_lottery_option_q)
        h += tr_qa("Gamble: lottery on left?",
                   get_yes_no_none(req, self.gamble_lottery_on_left))
        h += tr_qa("Gamble: starting <i>p</i>", self.gamble_starting_p)
        h += tr_qa("Gamble: start time", self.gamble_start_time)
        h += tr_qa("Gamble: responded?",
                   get_yes_no_none(req, self.gamble_responded))
        h += tr_qa("Gamble: response time", self.gamble_response_time)
        h += tr_qa("Gamble: <i>p</i>",
                   ws.number_to_dp(self.gamble_p, DP, default=None))
        h += tr_qa("Calculated utility",
                   ws.number_to_dp(self.utility, DP, default=None))
        h += """
            </table>
        """
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [SnomedExpression(req.snomed(SnomedLookup.QOL_SCALE))]
