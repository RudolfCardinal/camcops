#!/usr/bin/env python

"""
camcops_server/tasks/epds.py

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

**EPDS task.**

"""

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import get_yes_no, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# EPDS
# =============================================================================

class EpdsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Epds'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(cls, "q", 1, cls.NQUESTIONS)
        super().__init__(name, bases, classdict)


class Epds(TaskHasPatientMixin, Task, metaclass=EpdsMetaclass):
    __tablename__ = "epds"
    shortname = "EPDS"
    provides_trackers = True

    NQUESTIONS = 10
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_TOTAL = 30
    CUTOFF_1_GREATER_OR_EQUAL = 10  # Cox et al. 1987, PubMed ID 3651732.
    CUTOFF_2_GREATER_OR_EQUAL = 13  # Cox et al. 1987, PubMed ID 3651732.

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Edinburgh Postnatal Depression Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="EPDS total score (rating depressive symptoms)",
            axis_label=f"Total score (out of {self.MAX_TOTAL})",
            axis_min=-0.5,
            axis_max=self.MAX_TOTAL + 0.5,
            horizontal_lines=[
                self.CUTOFF_2_GREATER_OR_EQUAL - 0.5,
                self.CUTOFF_1_GREATER_OR_EQUAL - 0.5,
            ],
            horizontal_labels=[
                TrackerLabel(self.CUTOFF_2_GREATER_OR_EQUAL,
                             "likely depression"),
                TrackerLabel(self.CUTOFF_1_GREATER_OR_EQUAL,
                             "possible depression"),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        text = f"EPDS total: {self.total_score()}/{self.MAX_TOTAL}"
        return [CtvInfo(content=text)]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (out of {self.MAX_TOTAL})"
            ),
        ]

    def is_complete(self) -> bool:
        return self.all_fields_not_none(self.TASK_FIELDS)

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        above_cutoff_1 = score >= 10
        above_cutoff_2 = score >= 13
        answer_dicts = []
        for q in range(1, self.NQUESTIONS + 1):
            d = {None: "?"}
            for option in range(0, 4):
                d[option] = (
                    str(option) + " — " +
                    self.wxstring(req, "q" + str(q) + "_option" + str(option)))
            answer_dicts.append(d)

        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_question"),
                get_from_dict(answer_dicts[q - 1], getattr(self, "q" + str(q)))
            )

        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>{req.sstring(SS.TOTAL_SCORE)}</td>
                        <td><b>{score}</b> / {self.MAX_TOTAL}</td>
                    </tr>
                    <tr>
                        <td>{self.wxstring(req, "above_cutoff_1")}
                            <sup>[1]</sup></td>
                        <td><b>{get_yes_no(req, above_cutoff_1)}</b></td>
                    </tr>
                    <tr>
                        <td>{self.wxstring(req, "above_cutoff_2")}
                            <sup>[2]</sup></td>
                        <td><b>{get_yes_no(req, above_cutoff_2)}</b></td>
                    </tr>
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings are over the last week.
                <b>{self.wxstring(req, "always_look_at_suicide")}</b>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] &ge;{self.CUTOFF_1_GREATER_OR_EQUAL}.
                [2] &ge;{self.CUTOFF_2_GREATER_OR_EQUAL}.
                (Cox et al. 1987, PubMed ID 3651732.)
            </div>
            <div class="{CssClass.COPYRIGHT}">
                Edinburgh Postnatal Depression Scale:
                © 1987 The Royal College of Psychiatrists. The Edinburgh
                Postnatal Depression Scale may be photocopied by individual
                researchers or clinicians for their own use without seeking
                permission from the publishers. The scale must be copied in
                full and all copies must acknowledge the following source: Cox,
                J.L., Holden, J.M., & Sagovsky, R. (1987). Detection of
                postnatal depression. Development of the 10-item Edinburgh
                Postnatal Depression Scale. British Journal of Psychiatry, 150,
                782-786. Written permission must be obtained from the Royal
                College of Psychiatrists for copying and distribution to others
                or for republication (in print, online or by any other medium).
            </div>
        """
