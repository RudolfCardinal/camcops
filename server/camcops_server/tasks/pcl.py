#!/usr/bin/env python

"""
camcops_server/tasks/pcl.py

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

from abc import ABCMeta, ABC
from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no,
    subheading_spanning_two_columns,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# PCL
# =============================================================================

class PclMetaclass(DeclarativeMeta, ABCMeta):
    """
    There is a multilayer metaclass problem; see hads.py for discussion.
    """
    # noinspection PyInitNewSignature
    def __init__(cls: Type['PclCommon'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=1, maximum=5,
            comment_fmt="Q{n} ({s}) (1 not at all - 5 extremely)",
            comment_strings=[
                "disturbing memories/thoughts/images",
                "disturbing dreams",
                "reliving",
                "upset at reminders",
                "physical reactions to reminders",
                "avoid thinking/talking/feelings relating to experience",
                "avoid activities/situations because they remind",
                "trouble remembering important parts of stressful event",
                "loss of interest in previously enjoyed activities",
                "feeling distant/cut off from people",
                "feeling emotionally numb",
                "feeling future will be cut short",
                "hard to sleep",
                "irritable",
                "difficulty concentrating",
                "super alert/on guard",
                "jumpy/easily startled",
            ]
        )
        super().__init__(name, bases, classdict)


class PclCommon(TaskHasPatientMixin, Task, ABC,
                metaclass=PclMetaclass):
    __abstract__ = True
    provides_trackers = True
    extrastring_taskname = "pcl"

    NQUESTIONS = 17
    SCORED_FIELDS = strseq("q", 1, NQUESTIONS)
    TASK_FIELDS = SCORED_FIELDS  # may be overridden
    TASK_TYPE = "?"  # will be overridden
    # ... not really used; we display the generic question forms on the server
    MIN_SCORE = NQUESTIONS
    MAX_SCORE = 5 * NQUESTIONS

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        return self.sum_fields(self.SCORED_FIELDS)

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="PCL total score",
            axis_label=f"Total score ({self.MIN_SCORE}-{self.MAX_SCORE})",
            axis_min=self.MIN_SCORE - 0.5,
            axis_max=self.MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"PCL total score {self.total_score()}"
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
                        "(meaning scoring 3 or more)"),
            SummaryElement(
                name="num_symptomatic_B",
                coltype=Integer(),
                value=self.num_symptomatic_b(),
                comment="Number of group B symptoms considered symptomatic "
                        "(meaning scoring 3 or more)"),
            SummaryElement(
                name="num_symptomatic_C",
                coltype=Integer(),
                value=self.num_symptomatic_c(),
                comment="Number of group C symptoms considered symptomatic "
                        "(meaning scoring 3 or more)"),
            SummaryElement(
                name="num_symptomatic_D",
                coltype=Integer(),
                value=self.num_symptomatic_d(),
                comment="Number of group D symptoms considered symptomatic "
                        "(meaning scoring 3 or more)"),
            SummaryElement(
                name="ptsd",
                coltype=Boolean(),
                value=self.ptsd(),
                comment="Meets DSM-IV criteria for PTSD"),
        ]

    def get_num_symptomatic(self, first: int, last: int) -> int:
        n = 0
        for i in range(first, last + 1):
            value = getattr(self, "q" + str(i))
            if value is not None and value >= 3:
                n += 1
        return n

    def num_symptomatic(self) -> int:
        return self.get_num_symptomatic(1, self.NQUESTIONS)

    def num_symptomatic_b(self) -> int:
        return self.get_num_symptomatic(1, 5)

    def num_symptomatic_c(self) -> int:
        return self.get_num_symptomatic(6, 12)

    def num_symptomatic_d(self) -> int:
        return self.get_num_symptomatic(13, 17)

    def ptsd(self) -> bool:
        num_symptomatic_b = self.num_symptomatic_b()
        num_symptomatic_c = self.num_symptomatic_c()
        num_symptomatic_d = self.num_symptomatic_d()
        return num_symptomatic_b >= 1 and num_symptomatic_c >= 3 and \
            num_symptomatic_d >= 2

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        num_symptomatic = self.num_symptomatic()
        num_symptomatic_b = self.num_symptomatic_b()
        num_symptomatic_c = self.num_symptomatic_c()
        num_symptomatic_d = self.num_symptomatic_d()
        ptsd = self.ptsd()
        answer_dict = {None: None}
        for option in range(1, 6):
            answer_dict[option] = str(option) + " – " + \
                self.wxstring(req, "option" + str(option))
        q_a = ""
        if hasattr(self, "event") and hasattr(self, "eventdate"):
            # PCL-S
            q_a += tr_qa(self.wxstring(req, "s_event_s"), self.event)
            q_a += tr_qa(self.wxstring(req, "s_eventdate_s"), self.eventdate)
        for q in range(1, self.NQUESTIONS + 1):
            if q == 1 or q == 6 or q == 13:
                section = "B" if q == 1 else ("C" if q == 6 else "D")
                q_a += subheading_spanning_two_columns(
                    f"DSM-IV-TR section {section}"
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
                [1] Questions with scores ≥3 are considered symptomatic.
                [2] ≥1 ‘B’ symptoms and ≥3 ‘C’ symptoms and
                    ≥2 ‘D’ symptoms.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr_qa(
                f"{req.sstring(SS.TOTAL_SCORE)} (17–85)",
                score
            ),
            num_symptomatic=tr(
                "Number symptomatic <sup>[1]</sup>: B, C, D (total)",
                answer(num_symptomatic_b) + ", " +
                answer(num_symptomatic_c) + ", " +
                answer(num_symptomatic_d) + " (" + answer(num_symptomatic) + ")"  # noqa
            ),
            dsm_criteria_met=tr_qa(
                self.wxstring(req, "dsm_criteria_met") + " <sup>[2]</sup>",
                get_yes_no(req, ptsd)
            ),
            q_a=q_a,
        )
        return h


# =============================================================================
# PCL-C
# =============================================================================

class PclC(PclCommon,
           metaclass=PclMetaclass):
    """
    Server implementation of the PCL-C task.
    """
    __tablename__ = "pclc"
    shortname = "PCL-C"

    TASK_TYPE = "C"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("PTSD Checklist, Civilian version")


# =============================================================================
# PCL-M
# =============================================================================

class PclM(PclCommon,
           metaclass=PclMetaclass):
    """
    Server implementation of the PCL-M task.
    """
    __tablename__ = "pclm"
    shortname = "PCL-M"

    TASK_TYPE = "M"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("PTSD Checklist, Military version")


# =============================================================================
# PCL-S
# =============================================================================

class PclS(PclCommon,
           metaclass=PclMetaclass):
    """
    Server implementation of the PCL-S task.
    """
    __tablename__ = "pcls"
    shortname = "PCL-S"

    event = Column(
        "event", UnicodeText,
        comment="Traumatic event"
    )
    eventdate = Column(
        "eventdate", UnicodeText,
        comment="Date of traumatic event (free text)"
    )

    TASK_FIELDS = PclCommon.SCORED_FIELDS + ["event", "eventdate"]
    TASK_TYPE = "S"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("PTSD Checklist, Stressor-specific version")
