#!/usr/bin/env python

"""
camcops_server/tasks/npiq.py

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
from sqlalchemy.sql.sqltypes import Boolean, Integer

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_UNLESS_UPGRADED_DIV,
    PV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, get_yes_no_unknown, tr
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
    TaskHasRespondentMixin,
)


# =============================================================================
# NPI-Q
# =============================================================================

ENDORSED = "endorsed"
SEVERITY = "severity"
DISTRESS = "distress"


class NpiQMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['NpiQ'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        question_snippets = [
            "delusions",  # 1
            "hallucinations",
            "agitation/aggression",
            "depression/dysphoria",
            "anxiety",  # 5
            "elation/euphoria",
            "apathy/indifference",
            "disinhibition",
            "irritability/lability",
            "motor disturbance",  # 10
            "night-time behaviour",
            "appetite/eating",
        ]
        add_multiple_columns(
            cls, ENDORSED, 1, cls.NQUESTIONS, Boolean,
            pv=PV.BIT,
            comment_fmt="Q{n}, {s}, endorsed?",
            comment_strings=question_snippets
        )
        add_multiple_columns(
            cls, SEVERITY, 1, cls.NQUESTIONS,
            pv=list(range(1, 3 + 1)),
            comment_fmt="Q{n}, {s}, severity (1-3), if endorsed",
            comment_strings=question_snippets
        )
        add_multiple_columns(
            cls, DISTRESS, 1, cls.NQUESTIONS,
            pv=list(range(0, 5 + 1)),
            comment_fmt="Q{n}, {s}, distress (0-5), if endorsed",
            comment_strings=question_snippets
        )
        super().__init__(name, bases, classdict)


class NpiQ(TaskHasPatientMixin, TaskHasRespondentMixin, Task,
           metaclass=NpiQMetaclass):
    """
    Server implementation of the NPI-Q task.
    """
    __tablename__ = "npiq"
    shortname = "NPI-Q"

    NQUESTIONS = 12
    ENDORSED_FIELDS = strseq(ENDORSED, 1, NQUESTIONS)
    MAX_SEVERITY = 3 * NQUESTIONS
    MAX_DISTRESS = 5 * NQUESTIONS

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Neuropsychiatric Inventory Questionnaire")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="n_endorsed", coltype=Integer(),
                value=self.n_endorsed(),
                comment=f"Number endorsed (/ {self.NQUESTIONS})"),
            SummaryElement(
                name="severity_score", coltype=Integer(),
                value=self.severity_score(),
                comment=f"Severity score (/ {self.MAX_SEVERITY})"),
            SummaryElement(
                name="distress_score", coltype=Integer(),
                value=self.distress_score(),
                comment=f"Distress score (/ {self.MAX_DISTRESS})"),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=(
                "Endorsed: {e}/{me}; severity {s}/{ms}; "
                "distress {d}/{md}".format(
                    e=self.n_endorsed(),
                    me=self.NQUESTIONS,
                    s=self.severity_score(),
                    ms=self.MAX_SEVERITY,
                    d=self.distress_score(),
                    md=self.MAX_DISTRESS,
                )
            )
        )]

    def q_endorsed(self, q: int) -> bool:
        return bool(getattr(self, ENDORSED + str(q)))

    def n_endorsed(self) -> int:
        return self.count_booleans(self.ENDORSED_FIELDS)

    def severity_score(self) -> int:
        total = 0
        for q in range(1, self.NQUESTIONS + 1):
            if self.q_endorsed(q):
                s = getattr(self, SEVERITY + str(q))
                if s is not None:
                    total += s
        return total

    def distress_score(self) -> int:
        total = 0
        for q in range(1, self.NQUESTIONS + 1):
            if self.q_endorsed(q):
                d = getattr(self, DISTRESS + str(q))
                if d is not None:
                    total += d
        return total

    def q_complete(self, q: int) -> bool:
        qstr = str(q)
        endorsed = getattr(self, ENDORSED + qstr)
        if endorsed is None:
            return False
        if not endorsed:
            return True
        if getattr(self, SEVERITY + qstr) is None:
            return False
        if getattr(self, DISTRESS + qstr) is None:
            return False
        return True

    def is_complete(self) -> bool:
        return (
            self.is_respondent_complete() and
            all(self.q_complete(q) for q in range(1, self.NQUESTIONS + 1)) and
            self.field_contents_valid()
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>Endorsed</td>
                        <td>{self.n_endorsed()} / 12</td>
                    </td>
                    <tr>
                        <td>Severity score</td>
                        <td>{self.severity_score()} / 36</td>
                    </td>
                    <tr>
                        <td>Distress score</td>
                        <td>{self.distress_score()} / 60</td>
                    </td>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="40%">Question</th>
                    <th width="20%">Endorsed</th>
                    <th width="20%">Severity (patient)</th>
                    <th width="20%">Distress (carer)</th>
                </tr>
        """
        for q in range(1, self.NQUESTIONS + 1):
            qstr = str(q)
            e = getattr(self, ENDORSED + qstr)
            s = getattr(self, SEVERITY + qstr)
            d = getattr(self, DISTRESS + qstr)
            qtext = "<b>{}:</b> {}".format(
                self.wxstring(req, "t" + qstr),
                self.wxstring(req, "q" + qstr),
            )
            etext = get_yes_no_unknown(req, e)
            if e:
                stext = self.wxstring(req, f"severity_{s}", s,
                                      provide_default_if_none=False)
                dtext = self.wxstring(req, f"distress_{d}", d,
                                      provide_default_if_none=False)
            else:
                stext = ""
                dtext = ""
            h += tr(qtext, answer(etext), answer(stext), answer(dtext))
        h += f"""
            </table>
            {DATA_COLLECTION_UNLESS_UPGRADED_DIV}
        """
        return h
