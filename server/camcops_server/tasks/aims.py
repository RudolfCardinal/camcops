#!/usr/bin/env python

"""
camcops_server/tasks/aims.py

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

from camcops_server.cc_modules.cc_constants import CssClass, PV
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no_none,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
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
# AIMS
# =============================================================================

class AimsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Aims'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NSCOREDQUESTIONS,
            minimum=0, maximum=4,
            comment_fmt="Q{n}, {s} (0 none - 4 severe)",
            comment_strings=["facial_expression", "lips", "jaw", "tongue",
                             "upper_limbs", "lower_limbs", "trunk", "global",
                             "incapacitation", "awareness"]
        )
        add_multiple_columns(
            cls, "q", cls.NSCOREDQUESTIONS + 1, cls.NQUESTIONS, pv=PV.BIT,
            comment_fmt="Q{n}, {s} (not scored) (0 no, 1 yes)",
            comment_strings=["problems_teeth_dentures",
                             "usually_wears_dentures"]
        )

        super().__init__(name, bases, classdict)


class Aims(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
           metaclass=AimsMetaclass):
    """
    Server implementation of the AIMS task.
    """
    __tablename__ = "aims"
    shortname = "AIMS"
    provides_trackers = True

    NQUESTIONS = 12
    NSCOREDQUESTIONS = 10
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    SCORED_FIELDS = strseq("q", 1, NSCOREDQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Abnormal Involuntary Movement Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="AIMS total score",
            axis_label="Total score (out of 40)",
            axis_min=-0.5,
            axis_max=40.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"AIMS total score {self.total_score()}/40"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment="Total score (/40)"),
        ]

    def is_complete(self) -> bool:
        return (self.all_fields_not_none(Aims.TASK_FIELDS) and
                self.field_contents_valid())

    def total_score(self) -> int:
        return self.sum_fields(self.SCORED_FIELDS)

    # noinspection PyUnresolvedReferences
    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        main_dict = {None: None}
        q10_dict = {None: None}
        for option in range(0, 5):
            main_dict[option] = str(option) + " — " + \
                self.wxstring(req, "main_option" + str(option))
            q10_dict[option] = str(option) + " — " + \
                self.wxstring(req, "q10_option" + str(option))

        q_a = ""
        for q in range(1, 10):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_s"),
                get_from_dict(main_dict, getattr(self, "q" + str(q))))
        q_a += (
            tr_qa(self.wxstring(req, "q10_s"),
                  get_from_dict(q10_dict, self.q10)) +
            tr_qa(self.wxstring(req, "q11_s"),
                  get_yes_no_none(req, self.q11)) +
            tr_qa(self.wxstring(req, "q12_s"),
                  get_yes_no_none(req, self.q12))
        )

        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr(req.sstring(SS.TOTAL_SCORE) + " <sup>[1]</sup>",
                        answer(score) + " / 40")}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Only Q1–10 are scored.
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.AIMS_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.AIMS_SCALE),
                {
                    req.snomed(SnomedLookup.AIMS_TOTAL_SCORE): self.total_score(),  # noqa
                }
            ))
        return codes
