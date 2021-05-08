#!/usr/bin/env python

"""
camcops_server/tasks/hama.py

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
from camcops_server.cc_modules.cc_sqla_coltypes import SummaryCategoryColType
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# HAM-A
# =============================================================================

class HamaMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Hama'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            comment_fmt="Q{n}, {s} (0-4, higher worse)",
            minimum=0, maximum=4,
            comment_strings=[
                "anxious mood", "tension", "fears", "insomnia",
                "concentration/memory", "depressed mood", "somatic, muscular",
                "somatic, sensory", "cardiovascular", "respiratory",
                "gastrointestinal", "genitourinary", "other autonomic",
                "behaviour in interview"
            ]
        )
        super().__init__(name, bases, classdict)


class Hama(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
           metaclass=HamaMetaclass):
    """
    Server implementation of the HAM-A task.
    """
    __tablename__ = "hama"
    shortname = "HAM-A"
    provides_trackers = True

    NQUESTIONS = 14
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 56

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Hamilton Rating Scale for Anxiety")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="HAM-A total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
            horizontal_lines=[30.5, 24.5, 17.5],
            horizontal_labels=[
                TrackerLabel(33, req.sstring(SS.VERY_SEVERE)),
                TrackerLabel(27.5, req.sstring(SS.MODERATE_TO_SEVERE)),
                TrackerLabel(21, req.sstring(SS.MILD_TO_MODERATE)),
                TrackerLabel(8.75, req.sstring(SS.MILD)),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"HAM-A total score {self.total_score()}/{self.MAX_SCORE} "
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
        if score >= 31:
            return req.sstring(SS.VERY_SEVERE)
        elif score >= 25:
            return req.sstring(SS.MODERATE_TO_SEVERE)
        elif score >= 18:
            return req.sstring(SS.MILD_TO_MODERATE)
        else:
            return req.sstring(SS.MILD)

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        severity = self.severity(req)
        answer_dicts = []
        for q in range(1, self.NQUESTIONS + 1):
            d = {None: None}
            for option in range(0, 4 + 1):
                d[option] = self.wxstring(req, "q" + str(q) + "_option" +
                                          str(option))
            answer_dicts.append(d)
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_s") + " " +
                self.wxstring(req, "q" + str(q) + "_question"),
                get_from_dict(answer_dicts[q - 1], getattr(self, "q" + str(q)))
            )
        return """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {symptom_severity}
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
                [1] ≥31 very severe, ≥25 moderate to severe,
                    ≥18 mild to moderate, otherwise mild.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(score) + " / {}".format(self.MAX_SCORE)
            ),
            symptom_severity=tr_qa(
                self.wxstring(req, "symptom_severity") + " <sup>[1]</sup>",
                severity
            ),
            q_a=q_a,
        )
