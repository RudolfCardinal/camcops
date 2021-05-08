#!/usr/bin/env python

"""
camcops_server/tasks/hamd7.py

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
from camcops_server.cc_modules.cc_sqla_coltypes import (
    SummaryCategoryColType,
    ZERO_TO_TWO_CHECKER,
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
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# HAMD-7
# =============================================================================

class Hamd7Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature,PyUnresolvedReferences
    def __init__(cls: Type['Hamd7'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=4,  # see below
            comment_fmt="Q{n}, {s} (0-4, except Q6 0-2; higher worse)",
            comment_strings=["depressed mood", "guilt",
                             "interest/pleasure/level of activities",
                             "psychological anxiety", "somatic anxiety",
                             "energy/somatic symptoms", "suicide"]
        )
        # Now fix the wrong bits. Hardly elegant!
        cls.q6.set_permitted_value_checker(ZERO_TO_TWO_CHECKER)

        super().__init__(name, bases, classdict)


class Hamd7(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
            metaclass=Hamd7Metaclass):
    """
    Server implementation of the HAMD-7 task.
    """
    __tablename__ = "hamd7"
    shortname = "HAMD-7"
    provides_trackers = True

    NQUESTIONS = 7
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 26

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Hamilton Rating Scale for Depression (7-item scale)")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="HAM-D-7 total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
            horizontal_lines=[19.5, 11.5, 3.5],
            horizontal_labels=[
                TrackerLabel(23, self.wxstring(req, "severity_severe")),
                TrackerLabel(15.5, self.wxstring(req, "severity_moderate")),
                TrackerLabel(7.5, self.wxstring(req, "severity_mild")),
                TrackerLabel(1.75, self.wxstring(req, "severity_none")),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"HAM-D-7 total score {self.total_score()}/{self.MAX_SCORE} "
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
        if score >= 20:
            return self.wxstring(req, "severity_severe")
        elif score >= 12:
            return self.wxstring(req, "severity_moderate")
        elif score >= 4:
            return self.wxstring(req, "severity_mild")
        else:
            return self.wxstring(req, "severity_none")

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        severity = self.severity(req)
        answer_dicts = []
        for q in range(1, self.NQUESTIONS + 1):
            d = {None: None}
            for option in range(0, 5):
                if q == 6 and option > 2:
                    continue
                d[option] = self.wxstring(req, "q" + str(q) + "_option" +
                                          str(option))
            answer_dicts.append(d)

        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_s"),
                get_from_dict(answer_dicts[q - 1], getattr(self, "q" + str(q)))
            )

        return """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {severity}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="30%">Question</th>
                    <th width="70%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] ≥20 severe, ≥12 moderate, ≥4 mild, &lt;4 none.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(score) + " / {}".format(self.MAX_SCORE)
            ),
            severity=tr_qa(
                self.wxstring(req, "severity") + " <sup>[1]</sup>",
                severity
            ),
            q_a=q_a,
        )
