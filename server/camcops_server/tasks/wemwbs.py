#!/usr/bin/env python

"""
camcops_server/tasks/wemwbs.py

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
# WEMWBS
# =============================================================================

class WemwbsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Wemwbs'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_QUESTIONS,
            minimum=1, maximum=5,
            comment_fmt="Q{n} ({s}) (1 none of the time - 5 all of the time)",
            comment_strings=[
                "optimistic",
                "useful",
                "relaxed",
                "interested in other people",
                "energy",
                "dealing with problems well",
                "thinking clearly",
                "feeling good about myself",
                "feeling close to others",
                "confident",
                "able to make up my own mind",
                "feeling loved",
                "interested in new things",
                "cheerful",
            ]
        )
        super().__init__(name, bases, classdict)


class Wemwbs(TaskHasPatientMixin, Task,
             metaclass=WemwbsMetaclass):
    """
    Server implementation of the WEMWBS task.
    """
    __tablename__ = "wemwbs"
    shortname = "WEMWBS"
    provides_trackers = True

    MINQSCORE = 1
    MAXQSCORE = 5
    N_QUESTIONS = 14
    MINTOTALSCORE = N_QUESTIONS * MINQSCORE
    MAXTOTALSCORE = N_QUESTIONS * MAXQSCORE
    TASK_FIELDS = strseq("q", 1, N_QUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Warwick–Edinburgh Mental Well-Being Scale")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="WEMWBS total score (rating mental well-being)",
            axis_label=f"Total score ({self.MINTOTALSCORE}-{self.MAXTOTALSCORE})",  # noqa
            axis_min=self.MINTOTALSCORE - 0.5,
            axis_max=self.MAXTOTALSCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"WEMWBS total score {self.total_score()} "
                    f"(range {self.MINTOTALSCORE}–{self.MAXTOTALSCORE})"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (range "
                        f"{self.MINTOTALSCORE}-{self.MAXTOTALSCORE})"),
        ]

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        main_dict = {
            None: None,
            1: "1 — " + self.wxstring(req, "wemwbs_a1"),
            2: "2 — " + self.wxstring(req, "wemwbs_a2"),
            3: "3 — " + self.wxstring(req, "wemwbs_a3"),
            4: "4 — " + self.wxstring(req, "wemwbs_a4"),
            5: "5 — " + self.wxstring(req, "wemwbs_a5")
        }
        q_a = ""
        for i in range(1, self.N_QUESTIONS + 1):
            nstr = str(i)
            q_a += tr_qa(self.wxstring(req, "wemwbs_q" + nstr),
                         get_from_dict(main_dict, getattr(self, "q" + nstr)))
        h = """
            <div class="{css_summary}">
                <table class="{css_summary}">
                    {tr_is_complete}
                    {tr_total_score}
                </table>
            </div>
            <div class="{css_explanation}">
                Ratings are over the last 2 weeks.
            </div>
            <table class="{css_taskdetail}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{css_copyright}">
                WEMWBS: from Tennant et al. (2007), <i>Health and Quality of
                Life Outcomes</i> 5:63,
                <a href="http://www.hqlo.com/content/5/1/63">
                    http://www.hqlo.com/content/5/1/63</a>;
                © 2007 Tennant et al.; distributed under the terms of the
                Creative Commons Attribution License.
            </div>
        """.format(
            css_summary=CssClass.SUMMARY,
            tr_is_complete=self.get_is_complete_tr(req),
            tr_total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(self.total_score()) +
                f" (range {self.MINTOTALSCORE}–{self.MAXTOTALSCORE})"
            ),
            css_explanation=CssClass.EXPLANATION,
            css_taskdetail=CssClass.TASKDETAIL,
            q_a=q_a,
            css_copyright=CssClass.COPYRIGHT,
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.WEMWBS_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.WEMWBS_SCALE),
                {
                    req.snomed(SnomedLookup.WEMWBS_SCORE): self.total_score(),
                }
            ))
        return codes


# =============================================================================
# SWEMWBS
# =============================================================================

class SwemwbsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Swemwbs'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_QUESTIONS,
            minimum=1, maximum=5,
            comment_fmt="Q{n} ({s}) (1 none of the time - 5 all of the time)",
            comment_strings=[
                "optimistic",
                "useful",
                "relaxed",
                "interested in other people",
                "energy",
                "dealing with problems well",
                "thinking clearly",
                "feeling good about myself",
                "feeling close to others",
                "confident",
                "able to make up my own mind",
                "feeling loved",
                "interested in new things",
                "cheerful",
            ]
        )
        super().__init__(name, bases, classdict)


class Swemwbs(TaskHasPatientMixin, Task,
              metaclass=SwemwbsMetaclass):
    """
    Server implementation of the SWEMWBS task.
    """
    __tablename__ = "swemwbs"
    shortname = "SWEMWBS"
    extrastring_taskname = "wemwbs"  # shares

    MINQSCORE = 1
    MAXQSCORE = 5
    N_QUESTIONS = 7
    MINTOTALSCORE = N_QUESTIONS * MINQSCORE
    MAXTOTALSCORE = N_QUESTIONS * MAXQSCORE
    TASK_FIELDS = strseq("q", 1, N_QUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Short Warwick–Edinburgh Mental Well-Being Scale")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="SWEMWBS total score (rating mental well-being)",
            axis_label=f"Total score ({self.MINTOTALSCORE}-{self.MAXTOTALSCORE})",  # noqa
            axis_min=self.MINTOTALSCORE - 0.5,
            axis_max=self.MAXTOTALSCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"SWEMWBS total score {self.total_score()} "
                    f"(range {self.MINTOTALSCORE}–{self.MAXTOTALSCORE})"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (range "
                        f"{self.MINTOTALSCORE}-{self.MAXTOTALSCORE})"),
        ]

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        main_dict = {
            None: None,
            1: "1 — " + self.wxstring(req, "wemwbs_a1"),
            2: "2 — " + self.wxstring(req, "wemwbs_a2"),
            3: "3 — " + self.wxstring(req, "wemwbs_a3"),
            4: "4 — " + self.wxstring(req, "wemwbs_a4"),
            5: "5 — " + self.wxstring(req, "wemwbs_a5")
        }
        q_a = ""
        for i in range(1, self.N_QUESTIONS + 1):
            nstr = str(i)
            q_a += tr_qa(self.wxstring(req, "swemwbs_q" + nstr),
                         get_from_dict(main_dict, getattr(self, "q" + nstr)))

        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings are over the last 2 weeks.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.COPYRIGHT}">
                SWEMWBS: from Stewart-Brown et al. (2009), <i>Health and
                Quality of Life Outcomes</i> 7:15,
                http://www.hqlo.com/content/7/1/15;
                © 2009 Stewart-Brown et al.; distributed under the terms of the
                Creative Commons Attribution License.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(self.total_score()) +
                f" (range {self.MINTOTALSCORE}–{self.MAXTOTALSCORE})"
            ),
            q_a=q_a,
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.SWEMWBS_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.SWEMWBS_SCALE),
                {
                    req.snomed(SnomedLookup.SWEMWBS_SCORE): self.total_score(),
                }
            ))
        return codes
