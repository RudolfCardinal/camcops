#!/usr/bin/env python

"""
camcops_server/tasks/icd10schizotypal.py

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

from typing import Any, Dict, List, Optional, Tuple, Type

from cardinal_pythonlib.datetimefunc import format_datetime
import cardinal_pythonlib.rnc_web as ws
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Date, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DateFormat,
    ICD10_COPYRIGHT_DIV,
    PV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    get_yes_no_none,
    td,
    tr,
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
# Icd10Schizotypal
# =============================================================================

class Icd10SchizotypalMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Icd10Schizotypal'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "a", 1, cls.N_A, Boolean,
            pv=PV.BIT,
            comment_fmt="Criterion A({n}), {s}",
            comment_strings=[
                "inappropriate/constricted affect",
                "odd/eccentric/peculiar",
                "poor rapport/social withdrawal",
                "odd beliefs/magical thinking",
                "suspiciousness/paranoid ideas",
                "ruminations without inner resistance",
                "unusual perceptual experiences",
                "vague/circumstantial/metaphorical/over-elaborate/stereotyped "
                "thinking",
                "occasional transient quasi-psychotic episodes",
            ]
        )
        super().__init__(name, bases, classdict)


class Icd10Schizotypal(TaskHasClinicianMixin, TaskHasPatientMixin, Task,
                       metaclass=Icd10SchizotypalMetaclass):
    """
    Server implementation of the ICD10-SZTYP task.
    """
    __tablename__ = "icd10schizotypal"
    shortname = "ICD10-SZTYP"

    date_pertains_to = Column(
        "date_pertains_to", Date,
        comment="Date the assessment pertains to"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )
    b = CamcopsColumn(
        "b", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Criterion (B). True if: the subject has never met "
                "the criteria for any disorder in F20 (Schizophrenia)."
    )

    N_A = 9
    A_FIELDS = strseq("a", 1, N_A)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return "ICD-10 criteria for schizotypal disorder (F21)"

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        c = self.meets_criteria()
        if c is None:
            category = "Unknown if met or not met"
        elif c:
            category = "Met"
        else:
            category = "Not met"
        infolist = [CtvInfo(
            content=(
                "Pertains to: {}. Criteria for schizotypal "
                "disorder: {}.".format(
                    format_datetime(self.date_pertains_to,
                                    DateFormat.LONG_DATE),
                    category
                )
            )
        )]
        if self.comments:
            infolist.append(CtvInfo(content=ws.webify(self.comments)))
        return infolist

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="meets_criteria", coltype=Boolean(),
                           value=self.meets_criteria(),
                           comment="Meets criteria for schizotypal disorder?"),
        ]

    # Meets criteria? These also return null for unknown.
    def meets_criteria(self) -> Optional[bool]:
        if not self.is_complete():
            return None
        return self.count_booleans(self.A_FIELDS) >= 4 and self.b

    def is_complete(self) -> bool:
        return (
            self.date_pertains_to is not None and
            self.all_fields_not_none(self.A_FIELDS) and
            self.b is not None and
            self.field_contents_valid()
        )

    def text_row(self, req: CamcopsRequest, wstringname: str) -> str:
        return tr(td(self.wxstring(req, wstringname)),
                  td("", td_class=CssClass.SUBHEADING),
                  literal=True)

    def get_task_html(self, req: CamcopsRequest) -> str:
        q_a = self.text_row(req, "a")
        for i in range(1, self.N_A + 1):
            q_a += self.get_twocol_bool_row_true_false(
                req, "a" + str(i), self.wxstring(req, "a" + str(i)))
        q_a += self.get_twocol_bool_row_true_false(
            req, "b", self.wxstring(req, "b"))
        h = """
            {clinician_comments}
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {date_pertains_to}
                    {meets_criteria}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="80%">Question</th>
                    <th width="20%">Answer</th>
                </tr>
                {q_a}
            </table>
            {ICD10_COPYRIGHT_DIV}
        """.format(
            clinician_comments=self.get_standard_clinician_comments_block(
                req, self.comments),
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            date_pertains_to=tr_qa(
                req.wappstring(AS.DATE_PERTAINS_TO),
                format_datetime(self.date_pertains_to,
                                DateFormat.LONG_DATE, default=None)
            ),
            meets_criteria=tr_qa(
                req.sstring(SS.MEETS_CRITERIA),
                get_yes_no_none(req, self.meets_criteria())
            ),
            q_a=q_a,
            ICD10_COPYRIGHT_DIV=ICD10_COPYRIGHT_DIV,
        )
        return h
