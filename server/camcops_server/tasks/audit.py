#!/usr/bin/env python

"""
camcops_server/tasks/audit.py

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
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# AUDIT
# =============================================================================

class AuditMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Audit'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=4,
            comment_fmt="Q{n}, {s} (0-4, higher worse)",
            comment_strings=[
                "how often drink", "drinks per day", "how often six drinks",
                "unable to stop", "unable to do what was expected",
                "eye opener", "guilt", "unable to remember", "injuries",
                "others concerned"]
        )
        super().__init__(name, bases, classdict)


class Audit(TaskHasPatientMixin, Task,
            metaclass=AuditMetaclass):
    """
    Server implementation of the AUDIT task.
    """
    __tablename__ = "audit"
    shortname = "AUDIT"
    provides_trackers = True

    prohibits_commercial = True

    NQUESTIONS = 10
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("WHO Alcohol Use Disorders Identification Test")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="AUDIT total score",
            axis_label="Total score (out of 40)",
            axis_min=-0.5,
            axis_max=40.5,
            horizontal_lines=[7.5]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"AUDIT total score {self.total_score()}/40"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment="Total score (/40)"),
        ]

    # noinspection PyUnresolvedReferences
    def is_complete(self) -> bool:
        if not self.field_contents_valid():
            return False
        if self.q1 is None or self.q9 is None or self.q10 is None:
            return False
        if self.q1 == 0:
            # Special limited-information completeness
            return True
        if self.q2 is not None \
                and self.q3 is not None \
                and (self.q2 + self.q3 == 0):
            # Special limited-information completeness
            return True
        # Otherwise, any null values cause problems
        return self.all_fields_not_none(self.TASK_FIELDS)

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    # noinspection PyUnresolvedReferences
    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        exceeds_cutoff = score >= 8
        q1_dict = {None: None}
        q2_dict = {None: None}
        q3_to_8_dict = {None: None}
        q9_to_10_dict = {None: None}
        for option in range(0, 5):
            q1_dict[option] = str(option) + " – " + \
                self.wxstring(req, "q1_option" + str(option))
            q2_dict[option] = str(option) + " – " + \
                self.wxstring(req, "q2_option" + str(option))
            q3_to_8_dict[option] = str(option) + " – " + \
                self.wxstring(req, "q3to8_option" + str(option))
            if option != 1 and option != 3:
                q9_to_10_dict[option] = str(option) + " – " + \
                    self.wxstring(req, "q9to10_option" + str(option))

        q_a = tr_qa(self.wxstring(req, "q1_s"),
                    get_from_dict(q1_dict, self.q1))
        q_a += tr_qa(self.wxstring(req, "q2_s"),
                     get_from_dict(q2_dict, self.q2))
        for q in range(3, 8 + 1):
            q_a += tr_qa(
                self.wxstring(req, "q" + str(q) + "_s"),
                get_from_dict(q3_to_8_dict, getattr(self, "q" + str(q)))
            )
        q_a += tr_qa(self.wxstring(req, "q9_s"),
                     get_from_dict(q9_to_10_dict, self.q9))
        q_a += tr_qa(self.wxstring(req, "q10_s"),
                     get_from_dict(q9_to_10_dict, self.q10))

        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr(req.wsstring(SS.TOTAL_SCORE),
                        answer(score) + " / 40")}
                    {tr_qa(self.wxstring(req, "exceeds_standard_cutoff"),
                           get_yes_no(req, exceeds_cutoff))}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.COPYRIGHT}">
                AUDIT: Copyright © World Health Organization.
                Reproduced here under the permissions granted for
                NON-COMMERCIAL use only. You must obtain permission from the
                copyright holder for any other use.
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.AUDIT_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.AUDIT_SCALE),
                {
                    req.snomed(SnomedLookup.AUDIT_SCORE): self.total_score(),
                }
            ))
        return codes


# =============================================================================
# AUDIT-C
# =============================================================================

class AuditCMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['AuditC'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=4,
            comment_fmt="Q{n}, {s} (0-4, higher worse)",
            comment_strings=[
                "how often drink", "drinks per day", "how often six drinks"
            ]
        )
        super().__init__(name, bases, classdict)


class AuditC(TaskHasPatientMixin, Task,
             metaclass=AuditMetaclass):
    __tablename__ = "audit_c"
    shortname = "AUDIT-C"
    extrastring_taskname = "audit"  # shares strings with AUDIT

    prohibits_commercial = True

    NQUESTIONS = 3
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("AUDIT Alcohol Consumption Questions")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="AUDIT-C total score",
            axis_label="Total score (out of 12)",
            axis_min=-0.5,
            axis_max=12.5,
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"AUDIT-C total score {self.total_score()}/12"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment="Total score (/12)"),
        ]

    def is_complete(self) -> bool:
        return self.all_fields_not_none(self.TASK_FIELDS)

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        q1_dict = {None: None}
        q2_dict = {None: None}
        q3_dict = {None: None}
        for option in range(0, 5):
            q1_dict[option] = str(option) + " – " + \
                self.wxstring(req, "q1_option" + str(option))
            if option == 0:  # special!
                q2_dict[option] = str(option) + " – " + \
                    self.wxstring(req, "c_q2_option0")
            else:
                q2_dict[option] = str(option) + " – " + \
                    self.wxstring(req, "q2_option" + str(option))
            q3_dict[option] = str(option) + " – " + \
                self.wxstring(req, "q3to8_option" + str(option))

        # noinspection PyUnresolvedReferences
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr(req.sstring(SS.TOTAL_SCORE),
                        answer(score) + " / 12")}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {tr_qa(self.wxstring(req, "c_q1_question"),
                       get_from_dict(q1_dict, self.q1))}
                {tr_qa(self.wxstring(req, "c_q2_question"),
                       get_from_dict(q2_dict, self.q2))}
                {tr_qa(self.wxstring(req, "c_q3_question"),
                       get_from_dict(q3_dict, self.q3))}
            </table>
            <div class="{CssClass.COPYRIGHT}">
                AUDIT: Copyright © World Health Organization.
                Reproduced here under the permissions granted for
                NON-COMMERCIAL use only. You must obtain permission from the
                copyright holder for any other use.

                AUDIT-C: presumed to have the same restrictions.
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.AUDITC_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.AUDITC_SCALE),
                {
                    req.snomed(SnomedLookup.AUDITC_SCORE): self.total_score(),
                }
            ))
        return codes
