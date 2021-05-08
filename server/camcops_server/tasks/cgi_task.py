#!/usr/bin/env python

"""
camcops_server/tasks/cgi_task.py

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

from typing import Dict, List

from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    answer,
    italic,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# CGI
# =============================================================================

class Cgi(TaskHasPatientMixin, TaskHasClinicianMixin, Task):
    """
    Server implementation of the CGI task.
    """
    __tablename__ = "cgi"
    shortname = "CGI"
    provides_trackers = True

    q1 = CamcopsColumn(
        "q1", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=7),
        comment="Q1. Severity (1-7, higher worse, 0 not assessed)"
    )
    q2 = CamcopsColumn(
        "q2", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=7),
        comment="Q2. Global improvement (1-7, higher worse, 0 not assessed)"
    )
    q3t = CamcopsColumn(
        "q3t", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=4),
        comment="Q3T. Therapeutic effects (1-4, higher worse, 0 not assessed)"
    )
    q3s = CamcopsColumn(
        "q3s", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=4),
        comment="Q3S. Side effects (1-4, higher worse, 0 not assessed)"
    )
    q3 = CamcopsColumn(
        "q3", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=16),
        comment="Q3 (calculated). Efficacy index [(Q3T - 1) * 4 + Q3S]."
    )

    TASK_FIELDS = ["q1", "q2", "q3t", "q3s", "q3"]
    MAX_SCORE = 30

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Clinical Global Impressions")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="CGI total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"CGI total score {self.total_score()}/{self.MAX_SCORE}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score()),
        ]

    def is_complete(self) -> bool:
        if not (self.all_fields_not_none(self.TASK_FIELDS) and
                self.field_contents_valid()):
            return False
        # Requirement for everything to be non-zero removed in v2.0.0
        # if self.q1 == 0 or self.q2 == 0 or self.q3t == 0 or self.q3s == 0:
        #     return False
        return True

    def total_score(self) -> int:
        return self.sum_fields(["q1", "q2", "q3"])

    def get_task_html(self, req: CamcopsRequest) -> str:
        q1_dict = {
            None: None,
            0: self.wxstring(req, "q1_option0"),
            1: self.wxstring(req, "q1_option1"),
            2: self.wxstring(req, "q1_option2"),
            3: self.wxstring(req, "q1_option3"),
            4: self.wxstring(req, "q1_option4"),
            5: self.wxstring(req, "q1_option5"),
            6: self.wxstring(req, "q1_option6"),
            7: self.wxstring(req, "q1_option7"),
        }
        q2_dict = {
            None: None,
            0: self.wxstring(req, "q2_option0"),
            1: self.wxstring(req, "q2_option1"),
            2: self.wxstring(req, "q2_option2"),
            3: self.wxstring(req, "q2_option3"),
            4: self.wxstring(req, "q2_option4"),
            5: self.wxstring(req, "q2_option5"),
            6: self.wxstring(req, "q2_option6"),
            7: self.wxstring(req, "q2_option7"),
        }
        q3t_dict = {
            None: None,
            0: self.wxstring(req, "q3t_option0"),
            1: self.wxstring(req, "q3t_option1"),
            2: self.wxstring(req, "q3t_option2"),
            3: self.wxstring(req, "q3t_option3"),
            4: self.wxstring(req, "q3t_option4"),
        }
        q3s_dict = {
            None: None,
            0: self.wxstring(req, "q3s_option0"),
            1: self.wxstring(req, "q3s_option1"),
            2: self.wxstring(req, "q3s_option2"),
            3: self.wxstring(req, "q3s_option3"),
            4: self.wxstring(req, "q3s_option4"),
        }

        tr_total_score = tr(
            "Total score <sup>[1]</sup>",
            answer(self.total_score())
        )
        tr_q1 = tr_qa(
            self.wxstring(req, "q1_s") + " <sup>[2]</sup>",
            get_from_dict(q1_dict, self.q1)
        )
        tr_q2 = tr_qa(
            self.wxstring(req, "q2_s") + " <sup>[2]</sup>",
            get_from_dict(q2_dict, self.q2)
        )
        tr_q3t = tr_qa(
            self.wxstring(req, "q3t_s") + " <sup>[3]</sup>",
            get_from_dict(q3t_dict, self.q3t)
        )
        tr_q3s = tr_qa(
            self.wxstring(req, "q3s_s") + " <sup>[3]</sup>",
            get_from_dict(q3s_dict, self.q3s)
        )
        tr_q3 = tr(
            f"""
                {self.wxstring(req, "q3_s")} <sup>[4]</sup>
                <div class="{CssClass.SMALLPRINT}">
                    [(Q3T – 1) × 4 + Q3S]
                </div>
            """,
            answer(self.q3, formatter_answer=italic)
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_total_score}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="30%">Question</th>
                    <th width="70%">Answer</th>
                </tr>
                {tr_q1}
                {tr_q2}
                {tr_q3t}
                {tr_q3s}
                {tr_q3}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Total score: Q1 + Q2 + Q3. Range 3–30 when complete.
                [2] Questions 1 and 2 are scored 1–7 (0 for not assessed).
                [3] Questions 3T and 3S are scored 1–4 (0 for not assessed).
                [4] Q3 is scored 1–16 if Q3T/Q3S complete.
            </div>
        """


# =============================================================================
# CGI-I
# =============================================================================

class CgiI(TaskHasPatientMixin, TaskHasClinicianMixin, Task):
    __tablename__ = "cgi_i"
    shortname = "CGI-I"
    extrastring_taskname = "cgi"  # shares with CGI

    q = CamcopsColumn(
        "q", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=7),
        comment="Global improvement (1-7, higher worse)"
    )

    TASK_FIELDS = ["q"]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Clinical Global Impressions – Improvement")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content="CGI-I rating: {}".format(self.get_rating_text(req))
        )]

    def is_complete(self) -> bool:
        return (self.all_fields_not_none(self.TASK_FIELDS) and
                self.field_contents_valid())

    def get_rating_text(self, req: CamcopsRequest) -> str:
        qdict = self.get_q_dict(req)
        return get_from_dict(qdict, self.q)

    def get_q_dict(self, req: CamcopsRequest) -> Dict:
        return {
            None: None,
            0: self.wxstring(req, "q2_option0"),
            1: self.wxstring(req, "q2_option1"),
            2: self.wxstring(req, "q2_option2"),
            3: self.wxstring(req, "q2_option3"),
            4: self.wxstring(req, "q2_option4"),
            5: self.wxstring(req, "q2_option5"),
            6: self.wxstring(req, "q2_option6"),
            7: self.wxstring(req, "q2_option7"),
        }

    def get_task_html(self, req: CamcopsRequest) -> str:
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {tr_qa(self.wxstring(req, "i_q"), self.get_rating_text(req))}
            </table>
        """
