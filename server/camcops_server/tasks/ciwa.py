#!/usr/bin/env python

"""
camcops_server/tasks/ciwa.py

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
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Float, Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    subheading_spanning_two_columns,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    MIN_ZERO_CHECKER,
    PermittedValueChecker,
    SummaryCategoryColType,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerLabel,
    TrackerInfo,
)


# =============================================================================
# CIWA
# =============================================================================

class CiwaMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Ciwa'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NSCOREDQUESTIONS - 1,
            minimum=0, maximum=7,
            comment_fmt="Q{n}, {s} (0-7, higher worse)",
            comment_strings=[
                "nausea/vomiting", "tremor", "paroxysmal sweats", "anxiety",
                "agitation", "tactile disturbances", "auditory disturbances",
                "visual disturbances", "headache/fullness in head"
            ]
        )
        super().__init__(name, bases, classdict)


class Ciwa(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
           metaclass=CiwaMetaclass):
    """
    Server implementation of the CIWA-Ar task.
    """
    __tablename__ = "ciwa"
    shortname = "CIWA-Ar"
    provides_trackers = True

    NSCOREDQUESTIONS = 10
    SCORED_QUESTIONS = strseq("q", 1, NSCOREDQUESTIONS)

    q10 = CamcopsColumn(
        "q10", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=4),
        comment="Q10, orientation/clouding of sensorium (0-4, higher worse)"
    )
    t = Column(
        "t", Float,
        comment="Temperature (degrees C)"
    )
    hr = CamcopsColumn(
        "hr", Integer,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Heart rate (beats/minute)"
    )
    sbp = CamcopsColumn(
        "sbp", Integer,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Systolic blood pressure (mmHg)"
    )
    dbp = CamcopsColumn(
        "dbp", Integer,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Diastolic blood pressure (mmHg)"
    )
    rr = CamcopsColumn(
        "rr", Integer,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Respiratory rate (breaths/minute)"
    )

    MAX_SCORE = 67

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Clinical Institute Withdrawal Assessment for Alcohol "
                 "Scale, Revised")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="CIWA total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
            horizontal_lines=[14.5, 7.5],
            horizontal_labels=[
                TrackerLabel(17, req.sstring(SS.SEVERE)),
                TrackerLabel(11, req.sstring(SS.MODERATE)),
                TrackerLabel(3.75, req.sstring(SS.MILD)),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"CIWA total score: {self.total_score()}/{self.MAX_SCORE}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{self.MAX_SCORE})"),
            SummaryElement(name="severity",
                           coltype=SummaryCategoryColType,
                           value=self.severity(req),
                           comment="Likely severity"),
        ]

    def is_complete(self) -> bool:
        return (self.all_fields_not_none(self.SCORED_QUESTIONS) and
                self.field_contents_valid())

    def total_score(self) -> int:
        return self.sum_fields(self.SCORED_QUESTIONS)

    def severity(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        if score >= 15:
            severity = self.wxstring(req, "category_severe")
        elif score >= 8:
            severity = self.wxstring(req, "category_moderate")
        else:
            severity = self.wxstring(req, "category_mild")
        return severity

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        severity = self.severity(req)
        answer_dicts_dict = {}
        for q in self.SCORED_QUESTIONS:
            d = {None: None}
            for option in range(0, 8):
                if option > 4 and q == "q10":
                    continue
                d[option] = self.wxstring(req, q + "_option" + str(option))
            answer_dicts_dict[q] = d
        q_a = ""
        for q in range(1, Ciwa.NSCOREDQUESTIONS + 1):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_s"),
                get_from_dict(answer_dicts_dict["q" + str(q)],
                              getattr(self, "q" + str(q)))
            )
        tr_total_score = tr(
            req.sstring(SS.TOTAL_SCORE),
            answer(score) + f" / {self.MAX_SCORE}"
        )
        tr_severity = tr_qa(
            self.wxstring(req, "severity") + " <sup>[1]</sup>",
            severity
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_total_score}
                    {tr_severity}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="35%">Question</th>
                    <th width="65%">Answer</th>
                </tr>
                {q_a}
                {subheading_spanning_two_columns(
                    self.wxstring(req, "vitals_title"))}
                {tr_qa(self.wxstring(req, "t"), self.t)}
                {tr_qa(self.wxstring(req, "hr"), self.hr)}
                {tr(self.wxstring(req, "bp"),
                    answer(self.sbp) + " / " + answer(self.dbp))}
                {tr_qa(self.wxstring(req, "rr"), self.rr)}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Total score ≥15 severe, ≥8 moderate, otherwise
                    mild/minimal.
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.CIWA_AR_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.CIWA_AR_SCALE),
                {
                    req.snomed(SnomedLookup.CIWA_AR_SCORE): self.total_score(),
                }
            ))
        return codes
