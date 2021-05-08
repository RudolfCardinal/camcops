#!/usr/bin/env python

"""
camcops_server/tasks/cesdr.py

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

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.stringfunc import strseq
from semantic_version import Version
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Boolean

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo, CTV_INCOMPLETE
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    get_yes_no, tr, tr_qa
)
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
# CESD-R
# =============================================================================

class CesdrMetaclass(DeclarativeMeta):
    """
    There is a multilayer metaclass problem; see hads.py for discussion.
    """
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Cesdr'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_QUESTIONS,
            minimum=0, maximum=4,
            comment_fmt=("Q{n} ({s}) (0 not at all - "
                         "4 nearly every day for two weeks)"),
            comment_strings=[
                "poor appetite",
                "unshakable blues",
                "poor concentration",
                "depressed",
                "sleep restless",
                "sad",
                "could not get going",
                "nothing made me happy",
                "felt a bad person",
                "loss of interest",
                "oversleeping",
                "moving slowly",
                "fidgety",
                "wished were dead",
                "wanted to hurt self",
                "tiredness",
                "disliked self",
                "unintended weight loss",
                "difficulty getting to sleep",
                "lack of focus",
            ]
        )
        super().__init__(name, bases, classdict)


class Cesdr(TaskHasPatientMixin, Task,
            metaclass=CesdrMetaclass):
    """
    Server implementation of the CESD task.
    """
    __tablename__ = 'cesdr'
    shortname = 'CESD-R'
    provides_trackers = True
    extrastring_taskname = "cesdr"

    CAT_NONCLINICAL = 0
    CAT_SUB = 1
    CAT_POSS_MAJOR = 2
    CAT_PROB_MAJOR = 3
    CAT_MAJOR = 4

    DEPRESSION_RISK_THRESHOLD = 16

    FREQ_NOT_AT_ALL = 0
    FREQ_1_2_DAYS_LAST_WEEK = 1
    FREQ_3_4_DAYS_LAST_WEEK = 2
    FREQ_5_7_DAYS_LAST_WEEK = 3
    FREQ_DAILY_2_WEEKS = 4

    N_QUESTIONS = 20
    N_ANSWERS = 5

    POSS_MAJOR_THRESH = 2
    PROB_MAJOR_THRESH = 3
    MAJOR_THRESH = 4

    SCORED_FIELDS = strseq("q", 1, N_QUESTIONS)
    TASK_FIELDS = SCORED_FIELDS
    MIN_SCORE = 0
    MAX_SCORE = 3 * N_QUESTIONS

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _('Center for Epidemiologic Studies Depression Scale (Revised)')

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
        return (
            self.sum_fields(self.SCORED_FIELDS) -
            self.count_where(self.SCORED_FIELDS, [self.FREQ_DAILY_2_WEEKS])
        )

    def get_depression_category(self) -> int:

        if not self.has_depression_risk():
            return self.CAT_SUB

        q_group_anhedonia = [8, 10]
        q_group_dysphoria = [2, 4, 6]
        other_q_groups = {
            'appetite': [1, 18],
            'sleep': [5, 11, 19],
            'thinking': [3, 20],
            'guilt': [9, 17],
            'tired': [7, 16],
            'movement': [12, 13],
            'suicidal': [14, 15]
        }

        # Dysphoria or anhedonia must be present at frequency FREQ_DAILY_2_WEEKS
        anhedonia_criterion = (
            self.fulfils_group_criteria(q_group_anhedonia, True) or
            self.fulfils_group_criteria(q_group_dysphoria, True)
        )
        if anhedonia_criterion:
            category_count_high_freq = 0
            category_count_lower_freq = 0
            for qgroup in other_q_groups.values():
                if self.fulfils_group_criteria(qgroup, True):
                    # Category contains an answer == FREQ_DAILY_2_WEEKS
                    category_count_high_freq += 1
                if self.fulfils_group_criteria(qgroup, False):
                    # Category contains an answer == FREQ_DAILY_2_WEEKS or
                    # FREQ_5_7_DAYS_LAST_WEEK
                    category_count_lower_freq += 1

            if category_count_high_freq >= self.MAJOR_THRESH:
                # Anhedonia or dysphoria (at FREQ_DAILY_2_WEEKS)
                # plus 4 other symptom groups at FREQ_DAILY_2_WEEKS
                return self.CAT_MAJOR
            if category_count_lower_freq >= self.PROB_MAJOR_THRESH:
                # Anhedonia or dysphoria (at FREQ_DAILY_2_WEEKS)
                # plus 3 other symptom groups at FREQ_DAILY_2_WEEKS or
                # FREQ_5_7_DAYS_LAST_WEEK
                return self.CAT_PROB_MAJOR
            if category_count_lower_freq >= self.POSS_MAJOR_THRESH:
                # Anhedonia or dysphoria (at FREQ_DAILY_2_WEEKS)
                # plus 2 other symptom groups at FREQ_DAILY_2_WEEKS or
                # FREQ_5_7_DAYS_LAST_WEEK
                return self.CAT_POSS_MAJOR

        if self.has_depression_risk():
            # Total CESD-style score >= 16 but doesn't meet other criteria.
            return self.CAT_SUB

        return self.CAT_NONCLINICAL

    def fulfils_group_criteria(self, qnums: List[int],
                               nearly_every_day_2w: bool) -> bool:
        qstrings = ["q" + str(qnum) for qnum in qnums]
        if nearly_every_day_2w:
            possible_values = [self.FREQ_DAILY_2_WEEKS]
        else:
            possible_values = [self.FREQ_5_7_DAYS_LAST_WEEK,
                               self.FREQ_DAILY_2_WEEKS]
        count = self.count_where(qstrings, possible_values)
        return count > 0

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        line_step = 20
        threshold_line = self.DEPRESSION_RISK_THRESHOLD - 0.5
        # noinspection PyTypeChecker
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="CESD-R total score",
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
            content=f"CESD-R total score {self.total_score()}"
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
        )
        tr_depression_or_risk_of = tr_qa(
            self.wxstring(req, "depression_or_risk_of") +
            "? <sup>[1]</sup>",
            get_yes_no(req, self.has_depression_risk())
        )
        tr_provisional_diagnosis = tr(
            'Provisional diagnosis <sup>[2]</sup>',
            self.wxstring(req,
                          "category_" + str(self.get_depression_category()))
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_total_score}
                    {tr_depression_or_risk_of}
                    {tr_provisional_diagnosis}
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
            [2] Diagnostic criteria described at
                <a href="https://cesd-r.com/cesdr/">https://cesd-r.com/cesdr/</a>
            </div>
        """  # noqa
