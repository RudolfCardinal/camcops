#!/usr/bin/env python

"""
camcops_server/tasks/pcl5.py

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
from sqlalchemy.sql.sqltypes import Boolean, Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo, CTV_INCOMPLETE
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer, get_yes_no, subheading_spanning_two_columns, tr, tr_qa
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
# PCL-5
# =============================================================================

class Pcl5Metaclass(DeclarativeMeta):
    """
    There is a multilayer metaclass problem; see hads.py for discussion.
    """
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Pcl5'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_QUESTIONS,
            minimum=0, maximum=4,
            comment_fmt="Q{n} ({s}) (0 not at all - 4 extremely)",
            comment_strings=[
                "disturbing memories/thoughts/images",
                "disturbing dreams",
                "reliving",
                "upset at reminders",
                "physical reactions to reminders",
                "avoid thinking/talking/feelings relating to experience",
                "avoid activities/situations because they remind",
                "trouble remembering important parts of stressful event",
                "strong negative beliefs about self/others/world",
                "blaming",
                "strong negative emotions",
                "loss of interest in previously enjoyed activities",
                "feeling distant / cut off from people",
                "feeling emotionally numb",
                "irritable, angry and/or aggressive",
                "risk-taking and/or self-harming behaviour",
                "super alert/on guard",
                "jumpy/easily startled",
                "difficulty concentrating",
                "hard to sleep",
            ]
        )
        super().__init__(name, bases, classdict)


class Pcl5(TaskHasPatientMixin, Task,
           metaclass=Pcl5Metaclass):
    """
    Server implementation of the PCL-5 task.
    """
    __tablename__ = 'pcl5'
    shortname = 'PCL-5'
    provides_trackers = True
    extrastring_taskname = "pcl5"
    N_QUESTIONS = 20
    SCORED_FIELDS = strseq("q", 1, N_QUESTIONS)
    TASK_FIELDS = SCORED_FIELDS  # may be overridden
    MIN_SCORE = 0
    MAX_SCORE = 4 * N_QUESTIONS

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _('PTSD Checklist, DSM-5 version')

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
        return self.sum_fields(self.SCORED_FIELDS)

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        line_step = 20
        preliminary_cutoff = 33
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="PCL-5 total score",
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
            ) + [preliminary_cutoff],
            horizontal_labels=[
                TrackerLabel(preliminary_cutoff,
                             self.wxstring(req, "preliminary_cutoff")),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"PCL-5 total score {self.total_score()}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score ({self.MIN_SCORE}-{self.MAX_SCORE})"),
            SummaryElement(
                name="num_symptomatic",
                coltype=Integer(),
                value=self.num_symptomatic(),
                comment="Total number of symptoms considered symptomatic "
                        "(meaning scoring 2 or more)"),
            SummaryElement(
                name="num_symptomatic_B",
                coltype=Integer(),
                value=self.num_symptomatic_b(),
                comment="Number of group B symptoms considered symptomatic "
                        "(meaning scoring 2 or more)"),
            SummaryElement(
                name="num_symptomatic_C",
                coltype=Integer(),
                value=self.num_symptomatic_c(),
                comment="Number of group C symptoms considered symptomatic "
                        "(meaning scoring 2 or more)"),
            SummaryElement(
                name="num_symptomatic_D",
                coltype=Integer(),
                value=self.num_symptomatic_d(),
                comment="Number of group D symptoms considered symptomatic "
                        "(meaning scoring 2 or more)"),
            SummaryElement(
                name="num_symptomatic_E",
                coltype=Integer(),
                value=self.num_symptomatic_e(),
                comment="Number of group D symptoms considered symptomatic "
                        "(meaning scoring 2 or more)"),
            SummaryElement(
                name="ptsd",
                coltype=Boolean(),
                value=self.ptsd(),
                comment="Provisionally meets DSM-5 criteria for PTSD"),
        ]

    def get_num_symptomatic(self, first: int, last: int) -> int:
        n = 0
        for i in range(first, last + 1):
            value = getattr(self, "q" + str(i))
            if value is not None and value >= 2:
                n += 1
        return n

    def num_symptomatic(self) -> int:
        return self.get_num_symptomatic(1, self.N_QUESTIONS)

    def num_symptomatic_b(self) -> int:
        return self.get_num_symptomatic(1, 5)

    def num_symptomatic_c(self) -> int:
        return self.get_num_symptomatic(6, 7)

    def num_symptomatic_d(self) -> int:
        return self.get_num_symptomatic(8, 14)

    def num_symptomatic_e(self) -> int:
        return self.get_num_symptomatic(15, 20)

    def ptsd(self) -> bool:
        num_symptomatic_b = self.num_symptomatic_b()
        num_symptomatic_c = self.num_symptomatic_c()
        num_symptomatic_d = self.num_symptomatic_d()
        num_symptomatic_e = self.num_symptomatic_e()
        return (
            num_symptomatic_b >= 1 and
            num_symptomatic_c >= 1 and
            num_symptomatic_d >= 2 and
            num_symptomatic_e >= 2
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        num_symptomatic = self.num_symptomatic()
        num_symptomatic_b = self.num_symptomatic_b()
        num_symptomatic_c = self.num_symptomatic_c()
        num_symptomatic_d = self.num_symptomatic_d()
        num_symptomatic_e = self.num_symptomatic_e()
        ptsd = self.ptsd()
        answer_dict = {None: None}
        for option in range(5):
            answer_dict[option] = str(option) + " – " + \
                self.wxstring(req, "a" + str(option))
        q_a = ""

        section_start = {
            1: 'B (intrusion symptoms)',
            6: 'C (avoidance)',
            8: 'D (negative cognition/mood)',
            15: 'E (arousal/reactivity)'
        }

        for q in range(1, self.N_QUESTIONS + 1):
            if q in section_start:
                section = section_start[q]
                q_a += subheading_spanning_two_columns(
                    f"DSM-5 section {section}"
                )

            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_s"),
                get_from_dict(answer_dict, getattr(self, "q" + str(q)))
            )

        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {num_symptomatic}
                    {dsm_criteria_met}
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
                [1] Questions with scores ≥2 are considered symptomatic; see
                    https://www.ptsd.va.gov/professional/assessment/adult-sr/ptsd-checklist.asp
                [2] ≥1 ‘B’ symptoms and ≥1 ‘C’ symptoms and ≥2 ‘D’ symptoms
                    and ≥2 ‘E’ symptoms.
            </div>
        """.format(  # noqa
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr_qa(
                f"{req.sstring(SS.TOTAL_SCORE)} (0–80)",
                score
            ),
            num_symptomatic=tr(
                "Number symptomatic <sup>[1]</sup>: B, C, D, E (total)",
                answer(num_symptomatic_b) + ", " +
                answer(num_symptomatic_c) + ", " +
                answer(num_symptomatic_d) + ", " +
                answer(num_symptomatic_e) + " (" + answer(num_symptomatic) + ")"  # noqa
            ),
            dsm_criteria_met=tr_qa(
                self.wxstring(req, "dsm_criteria_met") + " <sup>[2]</sup>",
                get_yes_no(req, ptsd)
            ),
            q_a=q_a,
        )
        return h
