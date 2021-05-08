#!/usr/bin/env python

"""
camcops_server/tasks/cgisch.py

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

from typing import Any, Dict, List, Optional, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    subheading_spanning_two_columns,
    tr_qa,
    tr_span_col,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# CGI-SCH
# =============================================================================

QUESTION_FRAGMENTS = ["positive", "negative", "depressive", "cognitive",
                      "overall"]


class CgiSchMetaclass(DeclarativeMeta):
    """
    Metaclass for :class:`CgiSch`.
    """
    # noinspection PyInitNewSignature
    def __init__(cls: Type['CgiSch'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "severity", 1, 5,
            minimum=1, maximum=7,
            comment_fmt="Severity Q{n}, {s} (1-7, higher worse)",
            comment_strings=QUESTION_FRAGMENTS
        )
        add_multiple_columns(
            cls, "change", 1, 5,
            pv=list(range(1, 7 + 1)) + [9],
            comment_fmt="Change Q{n}, {s} (1-7, higher worse, or 9 N/A)",
            comment_strings=QUESTION_FRAGMENTS
        )
        super().__init__(name, bases, classdict)


class CgiSch(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
             metaclass=CgiSchMetaclass):
    """
    Server implementation of the CGI-SCH task.
    """
    __tablename__ = "cgisch"
    shortname = "CGI-SCH"
    provides_trackers = True

    TASK_FIELDS = strseq("severity", 1, 5) + strseq("change", 1, 5)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Clinical Global Impression – Schizophrenia")

    # noinspection PyUnresolvedReferences
    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        prefix = "CGI-SCH severity: "
        ylabel = "Score (1-7)"
        return [
            TrackerInfo(
                value=self.severity1,
                plot_label=prefix + "positive symptoms",
                axis_label=ylabel,
                axis_min=0.5,
                axis_max=7.5
            ),
            TrackerInfo(
                value=self.severity2,
                plot_label=prefix + "negative symptoms",
                axis_label=ylabel,
                axis_min=0.5,
                axis_max=7.5
            ),
            TrackerInfo(
                value=self.severity3,
                plot_label=prefix + "depressive symptoms",
                axis_label=ylabel,
                axis_min=0.5,
                axis_max=7.5
            ),
            TrackerInfo(
                value=self.severity4,
                plot_label=prefix + "cognitive symptoms",
                axis_label=ylabel,
                axis_min=0.5,
                axis_max=7.5
            ),
            TrackerInfo(
                value=self.severity5,
                plot_label=prefix + "overall severity",
                axis_label=ylabel,
                axis_min=0.5,
                axis_max=7.5
            ),
        ]

    # noinspection PyUnresolvedReferences
    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=(
                f"CGI-SCH. Severity: positive {self.severity1}, "
                f"negative {self.severity2}, depressive {self.severity3}, "
                f"cognitive {self.severity4}, overall {self.severity5}. "
                f"Change: positive {self.change1}, negative {self.change2}, "
                f"depressive {self.change3}, cognitive {self.change4}, "
                f"overall {self.change5}."
            )
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        # pylint: disable=unused-argument
        return self.standard_task_summary_fields()

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    # noinspection PyUnresolvedReferences
    def get_task_html(self, req: CamcopsRequest) -> str:
        severity_dict = {
            None: None,
            1: self.wxstring(req, "i_option1"),
            2: self.wxstring(req, "i_option2"),
            3: self.wxstring(req, "i_option3"),
            4: self.wxstring(req, "i_option4"),
            5: self.wxstring(req, "i_option5"),
            6: self.wxstring(req, "i_option6"),
            7: self.wxstring(req, "i_option7"),
        }
        change_dict = {
            None: None,
            1: self.wxstring(req, "ii_option1"),
            2: self.wxstring(req, "ii_option2"),
            3: self.wxstring(req, "ii_option3"),
            4: self.wxstring(req, "ii_option4"),
            5: self.wxstring(req, "ii_option5"),
            6: self.wxstring(req, "ii_option6"),
            7: self.wxstring(req, "ii_option7"),
            9: self.wxstring(req, "ii_option9"),
        }

        def tr_severity(xstring_name: str, value: Optional[int]) -> str:
            return tr_qa(self.wxstring(req, xstring_name),
                         get_from_dict(severity_dict, value))

        def tr_change(xstring_name: str, value: Optional[int]) -> str:
            return tr_qa(self.wxstring(req, xstring_name),
                         get_from_dict(change_dict, value))

        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="70%">Question</th>
                    <th width="30%">Answer <sup>[1]</sup></th>
                </tr>
                {subheading_spanning_two_columns(self.wxstring(req, "i_title"))}
                {tr_span_col(self.wxstring(req, "i_question"), cols=2)}
                {tr_severity("q1", self.severity1)}
                {tr_severity("q2", self.severity2)}
                {tr_severity("q3", self.severity3)}
                {tr_severity("q4", self.severity4)}
                {tr_severity("q5", self.severity5)}
                
                {subheading_spanning_two_columns(self.wxstring(req, "ii_title"))}
                {tr_span_col(self.wxstring(req, "ii_question"), cols=2)}
                {tr_change("q1", self.change1)}
                {tr_change("q2", self.change2)}
                {tr_change("q3", self.change3)}
                {tr_change("q4", self.change4)}
                {tr_change("q5", self.change5)}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] All questions are scored 1–7, or 9 (not applicable, for
                change questions).
                {self.wxstring(req, "ii_postscript")}
            </div>

        """  # noqa
