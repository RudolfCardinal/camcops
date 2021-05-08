#!/usr/bin/env python

"""
camcops_server/tasks/smast.py

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

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo, CTV_INCOMPLETE
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CharColType,
    SummaryCategoryColType,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerLabel,
    TrackerInfo,
)


# =============================================================================
# SMAST
# =============================================================================

class SmastMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Smast'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS, CharColType,
            pv=['Y', 'N'],
            comment_fmt="Q{n}: {s} (Y or N)",
            comment_strings=[
                "believe you are a normal drinker",
                "near relative worries/complains",
                "feel guilty",
                "friends/relative think you are a normal drinker",
                "stop when you want to",
                "ever attended Alcoholics Anonymous",
                "problems with close relative",
                "trouble at work",
                "neglected obligations for >=2 days",
                "sought help",
                "hospitalized",
                "arrested for drink-driving",
                "arrested for other drunken behaviour",
            ]
        )
        super().__init__(name, bases, classdict)


class Smast(TaskHasPatientMixin, Task,
            metaclass=SmastMetaclass):
    """
    Server implementation of the SMAST task.
    """
    __tablename__ = "smast"
    shortname = "SMAST"
    provides_trackers = True

    NQUESTIONS = 13
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Short Michigan Alcohol Screening Test")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="SMAST total score",
            axis_label=f"Total score (out of {self.NQUESTIONS})",
            axis_min=-0.5,
            axis_max=self.NQUESTIONS + 0.5,
            horizontal_lines=[
                2.5,
                1.5,
            ],
            horizontal_labels=[
                TrackerLabel(4, self.wxstring(req, "problem_probable")),
                TrackerLabel(2, self.wxstring(req, "problem_possible")),
                TrackerLabel(0.75, self.wxstring(req, "problem_unlikely")),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"SMAST total score {self.total_score()}/{self.NQUESTIONS} "
            f"({self.likelihood(req)})"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/{self.NQUESTIONS})"),
            SummaryElement(
                name="likelihood",
                coltype=SummaryCategoryColType,
                value=self.likelihood(req),
                comment="Likelihood of problem"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def get_score(self, q: int) -> int:
        yes = "Y"
        value = getattr(self, "q" + str(q))
        if value is None:
            return 0
        if q == 1 or q == 4 or q == 5:
            return 0 if value == yes else 1
        else:
            return 1 if value == yes else 0

    def total_score(self) -> int:
        total = 0
        for q in range(1, self.NQUESTIONS + 1):
            total += self.get_score(q)
        return total

    def likelihood(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        if score >= 3:
            return self.wxstring(req, "problem_probable")
        elif score >= 2:
            return self.wxstring(req, "problem_possible")
        else:
            return self.wxstring(req, "problem_unlikely")

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        likelihood = self.likelihood(req)
        main_dict = {
            None: None,
            "Y": req.sstring(SS.YES),
            "N": req.sstring(SS.NO)
        }
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += tr(
                self.wxstring(req, "q" + str(q)),
                answer(get_from_dict(main_dict, getattr(self, "q" + str(q)))) +
                " — " + str(self.get_score(q))
            )
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {problem_likelihood}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="80%">Question</th>
                    <th width="20%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Total score ≥3 probable, ≥2 possible, 0–1 unlikely.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(score) + f" / {self.NQUESTIONS}"
            ),
            problem_likelihood=tr_qa(
                self.wxstring(req, "problem_likelihood") + " <sup>[1]</sup>",
                likelihood
            ),
            q_a=q_a,
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [SnomedExpression(req.snomed(SnomedLookup.SMAST_SCALE))]
