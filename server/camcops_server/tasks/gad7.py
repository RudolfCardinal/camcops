#!/usr/bin/env python

"""
camcops_server/tasks/gad7.py

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
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import SummaryCategoryColType
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
# GAD-7
# =============================================================================

class Gad7Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Gad7'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=3,
            comment_fmt="Q{n}, {s} (0 not at all - 3 nearly every day)",
            comment_strings=[
                "nervous/anxious/on edge",
                "can't stop/control worrying",
                "worrying too much about different things",
                "trouble relaxing",
                "restless",
                "irritable",
                "afraid"
            ]
        )
        super().__init__(name, bases, classdict)


class Gad7(TaskHasPatientMixin, Task,
           metaclass=Gad7Metaclass):
    """
    Server implementation of the GAD-7 task.
    """
    __tablename__ = "gad7"
    shortname = "GAD-7"
    provides_trackers = True

    NQUESTIONS = 7
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 21

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Generalized Anxiety Disorder Assessment")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="GAD-7 total score",
            axis_label="Total score (out of 21)",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
            horizontal_lines=[14.5, 9.5, 4.5],
            horizontal_labels=[
                TrackerLabel(17, req.sstring(SS.SEVERE)),
                TrackerLabel(12, req.sstring(SS.MODERATE)),
                TrackerLabel(7, req.sstring(SS.MILD)),
                TrackerLabel(2.25, req.sstring(SS.NONE)),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"GAD-7 total score {self.total_score()}/{self.MAX_SCORE} "
            f"({self.severity(req)})"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{self.MAX_SCORE})"),
            SummaryElement(name="severity",
                           coltype=SummaryCategoryColType,
                           value=self.severity(req),
                           comment="Severity"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def severity(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        if score >= 15:
            severity = req.sstring(SS.SEVERE)
        elif score >= 10:
            severity = req.sstring(SS.MODERATE)
        elif score >= 5:
            severity = req.sstring(SS.MILD)
        else:
            severity = req.sstring(SS.NONE)
        return severity

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        severity = self.severity(req)
        answer_dict = {None: None}
        for option in range(0, 4):
            answer_dict[option] = (
                str(option) + " — " + self.wxstring(req, "a" + str(option))
            )

        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q)),
                get_from_dict(answer_dict, getattr(self, "q" + str(q)))
            )

        return """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {anxiety_severity}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings are over the last 2 weeks.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] ≥15 severe, ≥10 moderate, ≥5 mild.
                Score ≥10 identifies: generalized anxiety disorder with
                sensitivity 89%, specificity 82% (Spitzer et al. 2006, PubMed
                ID 16717171);
                panic disorder with sensitivity 74%, specificity 81% (Kroenke
                et al. 2010, PMID 20633738);
                social anxiety with sensitivity 72%, specificity 80% (Kroenke
                et al. 2010);
                post-traumatic stress disorder with sensitivity 66%,
                specificity 81% (Kroenke et al. 2010).
                The majority of evidence contributing to these figures comes
                from primary care screening studies.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(score) + " / {}".format(self.MAX_SCORE)
            ),
            anxiety_severity=tr(
                self.wxstring(req, "anxiety_severity") + " <sup>[1]</sup>",
                severity
            ),
            q_a=q_a,
        )

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.GAD7_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.GAD7_SCALE),
                {
                    req.snomed(SnomedLookup.GAD7_SCORE): self.total_score(),
                }
            ))
        return codes
