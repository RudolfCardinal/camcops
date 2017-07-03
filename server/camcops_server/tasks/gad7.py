#!/usr/bin/env python
# gad7.py

"""
===============================================================================
    Copyright (C) 2012-2017 Rudolf Cardinal (rudolf@pobox.com).

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
===============================================================================
"""

from typing import List

from ..cc_modules.cc_db import repeat_fieldspec
from ..cc_modules.cc_html import answer, tr, tr_qa
from ..cc_modules.cc_string import wappstring
from ..cc_modules.cc_task import (
    CtvInfo,
    CTV_INCOMPLETE,
    get_from_dict,
    Task,
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# GAD-7
# =============================================================================

class Gad7(Task):
    NQUESTIONS = 7

    tablename = "gad7"
    shortname = "GAD-7"
    longname = "Generalized Anxiety Disorder Assessment"
    fieldspecs = repeat_fieldspec(
        "q", 1, NQUESTIONS, min=0, max=3,
        comment_fmt="Q{n}, {s} (0 not at all - 3 nearly every day)",
        comment_strings=[
            "nervous/anxious/on edge",
            "can't stop/control worrying",
            "worrying too much about different things",
            "trouble relaxing",
            "restless",
            "irritable",
            "afraid"
        ])

    TASK_FIELDS = [x["name"] for x in fieldspecs]

    def get_trackers(self) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="GAD-7 total score",
            axis_label="Total score (out of 21)",
            axis_min=-0.5,
            axis_max=21.5,
            horizontal_lines=[14.5, 9.5, 4.5],
            horizontal_labels=[
                TrackerLabel(17, wappstring("severe")),
                TrackerLabel(12, wappstring("moderate")),
                TrackerLabel(7, wappstring("mild")),
                TrackerLabel(2.25, wappstring("none")),
            ]
        )]

    def get_clinical_text(self) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content="GAD-7 total score {}/21 ({})".format(
                self.total_score(), self.severity())
        )]

    def get_summaries(self):
        return [
            self.is_complete_summary_field(),
            dict(name="total", cctype="INT", value=self.total_score(),
                 comment="Total score (/21)"),
            dict(name="severity", cctype="TEXT", value=self.severity(),
                 comment="Severity"),
        ]

    def is_complete(self) -> bool:
        return (
            self.are_all_fields_complete(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def severity(self) -> str:
        score = self.total_score()
        if score >= 15:
            severity = wappstring("severe")
        elif score >= 10:
            severity = wappstring("moderate")
        elif score >= 5:
            severity = wappstring("mild")
        else:
            severity = wappstring("none")
        return severity

    def get_task_html(self) -> str:
        score = self.total_score()
        severity = self.severity()
        answer_dict = {None: None}
        for option in range(0, 4):
            answer_dict[option] = (
                str(option) + " — " + self.wxstring("a" + str(option))
            )
        h = """
            <div class="summary">
                <table class="summary">
        """ + self.get_is_complete_tr()
        h += tr(wappstring("total_score"), answer(score) + " / 21")
        h += tr(self.wxstring("anxiety_severity") + " <sup>[1]</sup>",
                severity)
        h += """
                </table>
            </div>
            <div class="explanation">
                Ratings are over the last 2 weeks.
            </div>
            <table class="taskdetail">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
        """
        for q in range(1, self.NQUESTIONS + 1):
            h += tr_qa(
                self.wxstring("q" + str(q)),
                get_from_dict(answer_dict, getattr(self, "q" + str(q)))
            )
        h += """
            </table>
            <div class="footnotes">
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
        """
        return h
