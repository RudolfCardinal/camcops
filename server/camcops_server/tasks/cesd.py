#!/usr/bin/env python

"""
camcops_server/tasks/cesd.py

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

- By Joe Kearney, Rudolf Cardinal.

"""

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.stringfunc import strseq
from semantic_version import Version
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Boolean

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo, CTV_INCOMPLETE
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import get_yes_no, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest

from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    equally_spaced_int,
    regular_tracker_axis_ticks_int,
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# CESD
# =============================================================================

class CesdMetaclass(DeclarativeMeta):
    """
    There is a multilayer metaclass problem; see hads.py for discussion.
    """
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Cesd'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_QUESTIONS,
            minimum=0, maximum=4,
            comment_fmt=(
                "Q{n} ({s}) (0 rarely/none of the time - 4 all of the time)"
            ),
            comment_strings=[
                "sensitivity/irritability",
                "poor appetite",
                "unshakeable blues",
                "low self-esteem",
                "poor concentration",
                "depressed",
                "everything effortful",
                "hopeful",
                "feelings of failure",
                "fearful",
                "sleep restless",
                "happy",
                "uncommunicative",
                "lonely",
                "perceived unfriendliness",
                "enjoyment",
                "crying spells",
                "sadness",
                "feeling disliked",
                "could not get going",
            ]
        )
        super().__init__(name, bases, classdict)


class Cesd(TaskHasPatientMixin, Task,
           metaclass=CesdMetaclass):
    """
    Server implementation of the CESD task.
    """
    __tablename__ = 'cesd'
    shortname = 'CESD'
    provides_trackers = True
    extrastring_taskname = "cesd"
    N_QUESTIONS = 20
    N_ANSWERS = 4
    DEPRESSION_RISK_THRESHOLD = 16
    SCORED_FIELDS = strseq("q", 1, N_QUESTIONS)
    TASK_FIELDS = SCORED_FIELDS
    MIN_SCORE = 0
    MAX_SCORE = 3 * N_QUESTIONS
    REVERSE_SCORED_QUESTIONS = [4, 8, 12, 16]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _('Center for Epidemiologic Studies Depression Scale')

    # noinspection PyMethodParameters
    @classproperty
    def minimum_client_version(cls) -> Version:
        return Version("2.2.8")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        # Need to store values as per original then flip here
        total = 0
        for qnum, fieldname in enumerate(self.SCORED_FIELDS, start=1):
            score = getattr(self, fieldname)
            if score is None:
                continue
            if qnum in self.REVERSE_SCORED_QUESTIONS:
                total += 3 - score
            else:
                total += score
        return total

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        line_step = 20
        threshold_line = self.DEPRESSION_RISK_THRESHOLD - 0.5
        # noinspection PyTypeChecker
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="CESD total score",
            axis_label=f"Total score ({self.MIN_SCORE}-{self.MAX_SCORE})",
            axis_min=self.MIN_SCORE - 0.5,
            axis_max=self.MAX_SCORE + 0.5,
            axis_ticks=regular_tracker_axis_ticks_int(
                self.MIN_SCORE,
                self.MAX_SCORE,
                step=line_step
            ),
            horizontal_lines=equally_spaced_int(
                self.MIN_SCORE + line_step,
                self.MAX_SCORE - line_step,
                step=line_step
            ) + [threshold_line],
            horizontal_labels=[
                TrackerLabel(threshold_line,
                             self.wxstring(req, "depression_or_risk_of")),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"CESD total score {self.total_score()}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="depression_risk",
                coltype=Boolean(),
                value=self.has_depression_risk(),
                comment="Has depression or at risk of depression"),
        ]

    def has_depression_risk(self) -> bool:
        return self.total_score() >= self.DEPRESSION_RISK_THRESHOLD

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        answer_dict = {None: None}
        for option in range(self.N_ANSWERS):
            answer_dict[option] = str(option) + " – " + \
                self.wxstring(req, "a" + str(option))
        q_a = ""
        for q in range(1, self.N_QUESTIONS):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_s"),
                get_from_dict(answer_dict, getattr(self, "q" + str(q)))
            )

        tr_total_score = tr_qa(
            f"{req.sstring(SS.TOTAL_SCORE)} (0–60)",
            score
        ),
        tr_depression_or_risk_of = tr_qa(
            self.wxstring(req, "depression_or_risk_of") +
            "? <sup>[1]</sup>",
            get_yes_no(req, self.has_depression_risk())
        ),
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_total_score}
                    {tr_depression_or_risk_of}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="70%">Question</th>
                    <th width="30%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
            [1] Presence of depression (or depression risk) is indicated by a
                score &ge; 16
            </div>
        """
