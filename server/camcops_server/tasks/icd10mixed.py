#!/usr/bin/env python

"""
camcops_server/tasks/icd10mixed.py

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

from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.typetests import is_false
import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Date, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DateFormat,
    ICD10_COPYRIGHT_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    get_true_false_none,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
)
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS


# =============================================================================
# Icd10Mixed
# =============================================================================

class Icd10Mixed(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the ICD10-MIXED task.
    """
    __tablename__ = "icd10mixed"
    shortname = "ICD10-MIXED"

    date_pertains_to = Column(
        "date_pertains_to", Date,
        comment="Date the assessment pertains to"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )
    mixture_or_rapid_alternation = CamcopsColumn(
        "mixture_or_rapid_alternation", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="The episode is characterized by either a mixture or "
                "a rapid alternation (i.e. within a few hours) of hypomanic, "
                "manic and depressive symptoms."
    )
    duration_at_least_2_weeks = CamcopsColumn(
        "duration_at_least_2_weeks", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Both manic and depressive symptoms must be prominent"
                " most of the time during a period of at least two weeks."
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "ICD-10 symptomatic criteria for a mixed affective episode "
            "(as in e.g. F06.3, F25, F38.00, F31.6)"
        )

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        category = (
            ("Meets" if self.meets_criteria() else "Does not meet") +
            " criteria for mixed affective episode"
        )
        infolist = [CtvInfo(
            content="Pertains to: {}. {}.".format(
                format_datetime(self.date_pertains_to, DateFormat.LONG_DATE),
                category
            )
        )]
        if self.comments:
            infolist.append(CtvInfo(content=ws.webify(self.comments)))
        return infolist

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="meets_criteria",
                coltype=Boolean(),
                value=self.meets_criteria(),
                comment="Meets criteria for a mixed affective episode?"),
        ]

    # Meets criteria? These also return null for unknown.
    def meets_criteria(self) -> Optional[bool]:
        if (self.mixture_or_rapid_alternation and
                self.duration_at_least_2_weeks):
            return True
        if is_false(self.mixture_or_rapid_alternation):
            return False
        if is_false(self.duration_at_least_2_weeks):
            return False
        return None

    def is_complete(self) -> bool:
        return (
            self.meets_criteria() is not None and
            self.field_contents_valid()
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        return """
            {clinician_comments}
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {date_pertains_to}
                    {meets_criteria}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                {icd10_symptomatic_disclaimer}
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="80%">Question</th>
                    <th width="20%">Answer</th>
                </tr>
                {mixture_or_rapid_alternation}
                {duration_at_least_2_weeks}
            </table>
            {ICD10_COPYRIGHT_DIV}
        """.format(
            clinician_comments=self.get_standard_clinician_comments_block(
                req, self.comments),
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            date_pertains_to=tr_qa(
                req.wappstring(AS.DATE_PERTAINS_TO),
                format_datetime(self.date_pertains_to, DateFormat.LONG_DATE,
                                default=None)
            ),
            meets_criteria=tr_qa(
                req.sstring(SS.MEETS_CRITERIA),
                get_true_false_none(req, self.meets_criteria())
            ),
            icd10_symptomatic_disclaimer=req.wappstring(
                AS.ICD10_SYMPTOMATIC_DISCLAIMER),
            mixture_or_rapid_alternation=self.get_twocol_bool_row_true_false(
                req, "mixture_or_rapid_alternation", self.wxstring(req, "a")),
            duration_at_least_2_weeks=self.get_twocol_bool_row_true_false(
                req, "duration_at_least_2_weeks", self.wxstring(req, "b")),
            ICD10_COPYRIGHT_DIV=ICD10_COPYRIGHT_DIV,
        )
