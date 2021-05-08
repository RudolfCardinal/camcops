#!/usr/bin/env python

"""
camcops_server/tasks/wsas.py

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
from sqlalchemy.sql.sqltypes import Boolean, Integer

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_UNLESS_UPGRADED_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, get_true_false, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# WSAS
# =============================================================================

class WsasMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Wsas'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=cls.MIN_PER_Q, maximum=cls.MAX_PER_Q,
            comment_fmt="Q{n}, {s} (0-4, higher worse)",
            comment_strings=[
                "work",
                "home management",
                "social leisure",
                "private leisure",
                "relationships",
            ]
        )
        super().__init__(name, bases, classdict)


class Wsas(TaskHasPatientMixin, Task,
           metaclass=WsasMetaclass):
    """
    Server implementation of the WSAS task.
    """
    __tablename__ = "wsas"
    shortname = "WSAS"
    provides_trackers = True

    retired_etc = Column(
        "retired_etc", Boolean,
        comment="Retired or choose not to have job for reason unrelated "
                "to problem"
    )

    MIN_PER_Q = 0
    MAX_PER_Q = 8
    NQUESTIONS = 5
    QUESTION_FIELDS = strseq("q", 1, NQUESTIONS)
    Q2_TO_END = strseq("q", 2, NQUESTIONS)
    TASK_FIELDS = QUESTION_FIELDS + ["retired_etc"]
    MAX_IF_WORKING = MAX_PER_Q * NQUESTIONS
    MAX_IF_RETIRED = MAX_PER_Q * (NQUESTIONS - 1)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Work and Social Adjustment Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="WSAS total score (lower is better)",
            axis_label=f"Total score (out of "
                       f"{self.MAX_IF_RETIRED}–{self.MAX_IF_WORKING})",
            axis_min=-0.5,
            axis_max=self.MAX_IF_WORKING + 0.5
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total_score",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/ {self.max_score()})"),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"WSAS total score {self.total_score()}/{self.max_score()}"
        )]

    def total_score(self) -> int:
        return self.sum_fields(self.Q2_TO_END if self.retired_etc
                               else self.QUESTION_FIELDS)

    def max_score(self) -> int:
        return self.MAX_IF_RETIRED if self.retired_etc else self.MAX_IF_WORKING

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.Q2_TO_END if self.retired_etc
                                     else self.QUESTION_FIELDS) and
            self.field_contents_valid()
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        option_dict = {None: None}
        for a in range(self.MIN_PER_Q, self.MAX_PER_Q + 1):
            option_dict[a] = req.wappstring(AS.WSAS_A_PREFIX + str(a))
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            a = getattr(self, "q" + str(q))
            fa = get_from_dict(option_dict, a) if a is not None else None
            q_a += tr(self.wxstring(req, "q" + str(q)), answer(fa))
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>Total score</td>
                        <td>{answer(self.total_score())} / 40</td>
                    </td>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="75%">Question</th>
                    <th width="25%">Answer</th>
                </tr>
                {tr_qa(self.wxstring(req, "q_retired_etc"),
                       get_true_false(req, self.retired_etc))}
            </table>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="75%">Question</th>
                    <th width="25%">Answer (0–8)</th>
                </tr>
                {q_a}
            </table>
            {DATA_COLLECTION_UNLESS_UPGRADED_DIV}
        """

    # noinspection PyUnresolvedReferences
    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.WSAS_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            d = {
                req.snomed(SnomedLookup.WSAS_SCORE): self.total_score(),
                req.snomed(SnomedLookup.WSAS_HOME_MANAGEMENT_SCORE): self.q2,
                req.snomed(SnomedLookup.WSAS_SOCIAL_LEISURE_SCORE): self.q3,
                req.snomed(SnomedLookup.WSAS_PRIVATE_LEISURE_SCORE): self.q4,
                req.snomed(SnomedLookup.WSAS_RELATIONSHIPS_SCORE): self.q5,
            }
            if not self.retired_etc:
                d[req.snomed(SnomedLookup.WSAS_WORK_SCORE)] = self.q1
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.WSAS_SCALE),
                d
            ))
        return codes
