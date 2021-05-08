#!/usr/bin/env python

"""
camcops_server/tasks/gbo.py

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

Goal-Based Outcomes tasks.

- By Joe Kearney, Rudolf Cardinal.

"""

from typing import List

from cardinal_pythonlib.datetimefunc import format_datetime
from sqlalchemy import Column
from sqlalchemy.sql.sqltypes import Boolean, Integer, Date, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass, DateFormat
from camcops_server.cc_modules.cc_html import tr_qa, answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# Common GBO constants
# =============================================================================

AGENT_PATIENT = 1
AGENT_PARENT_CARER = 2
AGENT_CLINICIAN = 3
AGENT_OTHER = 4

AGENT_STRING_MAP = {
    AGENT_PATIENT: "Patient/service user",  # in original: "Child/young person"
    AGENT_PARENT_CARER: "Parent/carer",
    AGENT_CLINICIAN: "Practitioner/clinician",
    AGENT_OTHER: "Other: "
}
UNKNOWN_AGENT = "Unknown"

PROGRESS_COMMENT_SUFFIX = " (0 no progress - 10 reached fully)"


def agent_description(agent: int, other_detail: str) -> str:
    who = AGENT_STRING_MAP.get(agent, UNKNOWN_AGENT)
    if agent == AGENT_OTHER:
        who += other_detail or "?"
    return who


# =============================================================================
# GBO-GReS
# =============================================================================

class Gbogres(TaskHasPatientMixin, Task):
    """
    Server implementation of the GBO - Goal Record Sheet task.
    """
    __tablename__ = "gbogres"
    shortname = "GBO-GReS"
    extrastring_taskname = "gbo"

    FN_DATE = "date"  # NB SQL keyword too; doesn't matter
    FN_GOAL_1_DESC = "goal_1_description"
    FN_GOAL_2_DESC = "goal_2_description"
    FN_GOAL_3_DESC = "goal_3_description"
    FN_GOAL_OTHER = "other_goals"
    FN_COMPLETED_BY = "completed_by"
    FN_COMPLETED_BY_OTHER = "completed_by_other"

    REQUIRED_FIELDS = [FN_DATE, FN_GOAL_1_DESC, FN_COMPLETED_BY]

    date = Column(FN_DATE, Date, comment="Date of goal-setting")
    goal_1_description = Column(
        FN_GOAL_1_DESC, UnicodeText,
        comment="Goal 1 description")
    goal_2_description = Column(
        FN_GOAL_2_DESC, UnicodeText,
        comment="Goal 2 description")
    goal_3_description = Column(
        FN_GOAL_3_DESC, UnicodeText,
        comment="Goal 3 description")
    other_goals = Column(
        FN_GOAL_OTHER, UnicodeText,
        comment="Other/additional goal description(s)")
    completed_by = Column(
        FN_COMPLETED_BY, Integer,
        comment="Who completed the form ({})".format(
            "; ".join(f"{k} = {v}"
                      for k, v in AGENT_STRING_MAP.items())
        )
    )
    completed_by_other = Column(
        FN_COMPLETED_BY_OTHER, UnicodeText,
        comment="If completed by 'other', who?")

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Goal-Based Outcomes – 1 – Goal Record Sheet")

    def get_n_core_goals(self) -> int:
        """
        Returns the number of non-blank core (1-3) goals.
        """
        return len(list(filter(
            None,
            [self.goal_1_description, self.goal_2_description,
             self.goal_3_description])))

    def goals_set_tr(self) -> str:
        extra = " (additional goals specified)" if self.other_goals else ""
        return tr_qa("Number of goals set",
                     f"{self.get_n_core_goals()}{extra}")

    def completed_by_tr(self) -> str:
        who = agent_description(self.completed_by, self.completed_by_other)
        return tr_qa("Completed by", who)

    def get_date_tr(self) -> str:
        return tr_qa("Date", format_datetime(self.date,
                                             DateFormat.SHORT_DATE,
                                             default=None))

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields()

    def is_complete(self) -> bool:
        if self.any_fields_none(self.REQUIRED_FIELDS):
            return False
        if self.completed_by == AGENT_OTHER and not self.completed_by_other:
            return False
        return True

    def get_task_html(self, req: CamcopsRequest) -> str:
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {self.get_date_tr()}
                    {self.completed_by_tr()}
                    {self.goals_set_tr()}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="15%">Goal number</th>
                    <th width="85%">Goal description</th>
                </tr>
                <tr><td>1</td><td>{answer(self.goal_1_description,
                                          default="")}</td></tr>
                <tr><td>2</td><td>{answer(self.goal_2_description,
                                          default="")}</td></tr>
                <tr><td>3</td><td>{answer(self.goal_3_description,
                                          default="")}</td></tr>
                <tr><td>Other</td><td>{answer(self.other_goals,
                                              default="")}</td></tr>
            </table>
        """


# =============================================================================
# GBO-GPC
# =============================================================================

class Gbogpc(TaskHasPatientMixin, Task):
    """
    Server implementation of the GBO-GPC task.
    """
    __tablename__ = "gbogpc"
    shortname = "GBO-GPC"
    extrastring_taskname = "gbo"
    provides_trackers = True

    FN_DATE = "date"  # NB SQL keyword too; doesn't matter
    FN_SESSION = "session"
    FN_GOAL_NUMBER = "goal_number"
    FN_GOAL_DESCRIPTION = "goal_description"
    FN_PROGRESS = "progress"
    FN_WHOSE_GOAL = "whose_goal"
    FN_WHOSE_GOAL_OTHER = "whose_goal_other"

    date = Column(FN_DATE, Date, comment="Session date")
    session = Column(FN_SESSION, Integer, comment="Session number")
    goal_number = Column(FN_GOAL_NUMBER, Integer, comment="Goal number (1-3)")
    goal_text = Column(
        FN_GOAL_DESCRIPTION, UnicodeText,
        comment="Brief description of the goal")
    progress = Column(
        FN_PROGRESS, Integer,
        comment="Progress towards goal" + PROGRESS_COMMENT_SUFFIX
    )
    whose_goal = Column(
        FN_WHOSE_GOAL, Integer,
        comment="Whose goal is this ({})".format(
            "; ".join(f"{k} = {v}"
                      for k, v in AGENT_STRING_MAP.items())
        )
    )
    whose_goal_other = Column(
        FN_WHOSE_GOAL_OTHER, UnicodeText,
        comment="If 'whose goal' is 'other', who?")

    REQUIRED_FIELDS = [
        FN_DATE, FN_SESSION, FN_GOAL_NUMBER, FN_PROGRESS, FN_WHOSE_GOAL
    ]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Goal-Based Outcomes – 2 – Goal Progress Chart")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields()

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        axis_min = -0.5
        axis_max = 10.5
        hlines = [0, 5, 10]
        axis_label = "Progress towards goal (0-10)"
        title_start = "GBO Goal Progress Chart – Goal "
        return [
            TrackerInfo(
                value=self.progress if self.goal_number == 1 else None,
                plot_label=title_start + "1",
                axis_label=axis_label,
                axis_min=axis_min,
                axis_max=axis_max,
                horizontal_lines=hlines
            ),
            TrackerInfo(
                value=self.progress if self.goal_number == 2 else None,
                plot_label=title_start + "2",
                axis_label=axis_label,
                axis_min=axis_min,
                axis_max=axis_max,
                horizontal_lines=hlines
            ),
            TrackerInfo(
                value=self.progress if self.goal_number == 3 else None,
                plot_label=title_start + "3",
                axis_label=axis_label,
                axis_min=axis_min,
                axis_max=axis_max,
                horizontal_lines=hlines
            ),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.REQUIRED_FIELDS):
            return False
        if self.whose_goal == AGENT_OTHER and not self.whose_goal_other:
            return False
        return True

    def get_task_html(self, req: CamcopsRequest) -> str:
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="30%">Date</th>
                    <td width="70%">{
        answer(format_datetime(self.date, DateFormat.SHORT_DATE,
                               default=None))}</td>
                </tr>
                <tr>
                    <th>Session number</th>
                    <td>{answer(self.session)}</td>
                </tr>
                <tr>
                    <th>Goal number</th>
                    <td>{answer(self.goal_number)}</td>
                </tr>
                <tr>
                    <th>Goal description</th>
                    <td>{answer(self.goal_text)}</td>
                </tr>
                <tr>
                    <th>Progress <sup>[1]</sup></th>
                    <td>{answer(self.progress)}</td>
                </tr>
                <tr>
                    <th>Whose goal is this?</th>
                    <td>{answer(agent_description(self.whose_goal,
                                                  self.whose_goal_other))}</td>
                </tr>
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] {self.wxstring(req, "progress_explanation")}
            </div>
        """


# =============================================================================
# GBO-GRaS
# =============================================================================

class Gbogras(TaskHasPatientMixin, Task):
    """
    Server implementation of the GBO-GRaS task.
    """
    __tablename__ = "gbogras"
    shortname = "GBO-GRaS"
    extrastring_taskname = "gbo"
    provides_trackers = True

    FN_DATE = "date"  # NB SQL keyword too; doesn't matter
    FN_RATE_GOAL_1 = "rate_goal_1"
    FN_RATE_GOAL_2 = "rate_goal_2"
    FN_RATE_GOAL_3 = "rate_goal_3"
    FN_GOAL_1_DESC = "goal_1_description"
    FN_GOAL_2_DESC = "goal_2_description"
    FN_GOAL_3_DESC = "goal_3_description"
    FN_GOAL_1_PROGRESS = "goal_1_progress"
    FN_GOAL_2_PROGRESS = "goal_2_progress"
    FN_GOAL_3_PROGRESS = "goal_3_progress"
    FN_COMPLETED_BY = "completed_by"
    FN_COMPLETED_BY_OTHER = "completed_by_other"

    date = Column(FN_DATE, Date, comment="Date of ratings")
    # ... NB SQL keyword too; doesn't matter
    rate_goal_1 = Column(FN_RATE_GOAL_1, Boolean, comment="Rate goal 1?")
    rate_goal_2 = Column(FN_RATE_GOAL_2, Boolean, comment="Rate goal 2?")
    rate_goal_3 = Column(FN_RATE_GOAL_3, Boolean, comment="Rate goal 3?")
    goal_1_description = Column(
        FN_GOAL_1_DESC, UnicodeText,
        comment="Goal 1 description")
    goal_2_description = Column(
        FN_GOAL_2_DESC, UnicodeText,
        comment="Goal 2 description")
    goal_3_description = Column(
        FN_GOAL_3_DESC, UnicodeText,
        comment="Goal 3 description")
    goal_1_progress = Column(
        FN_GOAL_1_PROGRESS, Integer,
        comment="Goal 1 progress" + PROGRESS_COMMENT_SUFFIX)
    goal_2_progress = Column(
        FN_GOAL_2_PROGRESS, Integer,
        comment="Goal 2 progress" + PROGRESS_COMMENT_SUFFIX)
    goal_3_progress = Column(
        FN_GOAL_3_PROGRESS, Integer,
        comment="Goal 3 progress" + PROGRESS_COMMENT_SUFFIX)
    completed_by = Column(
        FN_COMPLETED_BY, Integer,
        comment="Who completed the form ({})".format(
            "; ".join(f"{k} = {v}"
                      for k, v in AGENT_STRING_MAP.items()
                      if k != AGENT_CLINICIAN)
        )
    )
    completed_by_other = Column(
        FN_COMPLETED_BY_OTHER, UnicodeText,
        comment="If completed by 'other', who?")

    REQUIRED_FIELDS = [FN_DATE, FN_COMPLETED_BY]
    GOAL_TUPLES = (
        # goalnum, rate it?, goal description, progress
        (1, FN_RATE_GOAL_1, FN_GOAL_1_DESC, FN_GOAL_1_PROGRESS),
        (2, FN_RATE_GOAL_2, FN_GOAL_2_DESC, FN_GOAL_2_PROGRESS),
        (3, FN_RATE_GOAL_3, FN_GOAL_3_DESC, FN_GOAL_3_PROGRESS),
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Goal-Based Outcomes – 3 – Goal Rating Sheet")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields()

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        axis_min = -0.5
        axis_max = 10.5
        hlines = [0, 5, 10]
        axis_label = "Progress towards goal (0-10)"
        title_start = "GBO Goal Rating Sheet – Goal "
        return [
            TrackerInfo(
                value=self.goal_1_progress if self.rate_goal_1 else None,
                plot_label=title_start + "1",
                axis_label=axis_label,
                axis_min=axis_min,
                axis_max=axis_max,
                horizontal_lines=hlines
            ),
            TrackerInfo(
                value=self.goal_2_progress if self.rate_goal_2 else None,
                plot_label=title_start + "2",
                axis_label=axis_label,
                axis_min=axis_min,
                axis_max=axis_max,
                horizontal_lines=hlines
            ),
            TrackerInfo(
                value=self.goal_3_progress if self.rate_goal_3 else None,
                plot_label=title_start + "3",
                axis_label=axis_label,
                axis_min=axis_min,
                axis_max=axis_max,
                horizontal_lines=hlines
            ),
        ]

    def is_complete(self) -> bool:
        if self.any_fields_none(self.REQUIRED_FIELDS):
            return False
        if self.completed_by == AGENT_OTHER and not self.completed_by_other:
            return False
        n_goals_completed = 0
        for _, rate_attr, desc_attr, prog_attr in self.GOAL_TUPLES:
            if getattr(self, rate_attr):
                n_goals_completed += 1
                if not getattr(self, desc_attr) or not getattr(self, prog_attr):  # noqa
                    return False
        return n_goals_completed > 0

    def completed_by_tr(self) -> str:
        who = agent_description(self.completed_by, self.completed_by_other)
        return tr_qa("Completed by", who)

    def get_date_tr(self) -> str:
        return tr_qa("Date", format_datetime(self.date,
                                             DateFormat.SHORT_DATE,
                                             default=None))

    def get_task_html(self, req: CamcopsRequest) -> str:
        rows = []  # type: List[str]
        for goalnum, rate_attr, desc_attr, prog_attr in self.GOAL_TUPLES:
            if getattr(self, rate_attr):
                rows.append(f"""
                    <tr>
                        <td>{answer(goalnum)}</td>
                        <td>{answer(getattr(self, desc_attr))}</td>
                        <td>{answer(getattr(self, prog_attr))}</td>
                    </tr>
                """)
        newline = "\n"
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {self.get_date_tr()}
                    {self.completed_by_tr()}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="15%">Goal number</th>
                    <th width="70%">Description</th>
                    <th width="15%">Progress <sup>[1]</sup></th>
                </tr>
                {newline.join(rows)}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] {self.wxstring(req, "progress_explanation")}
            </div>
        """
