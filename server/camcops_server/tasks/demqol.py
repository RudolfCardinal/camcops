#!/usr/bin/env python

"""
camcops_server/tasks/demqol.py

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

from typing import Any, Dict, List, Optional, Tuple, Type, Union

from cardinal_pythonlib.stringfunc import strseq
import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Float, Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no,
    subheading_spanning_two_columns,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
    TaskHasRespondentMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo

# =============================================================================
# Constants
# =============================================================================

DP = 2
MISSING_VALUE = -99
PERMITTED_VALUES = list(range(1, 4 + 1)) + [MISSING_VALUE]
END_DIV = f"""
    </table>
    <div class="{CssClass.FOOTNOTES}">
        [1] Extrapolated total scores are: total_for_responded_questions ×
        n_questions / n_responses.
    </div>
"""
COPYRIGHT_DIV = f"""
    <div class="{CssClass.COPYRIGHT}">
        DEMQOL/DEMQOL-Proxy: Copyright © Institute of Psychiatry, King’s
        College London. Reproduced with permission.
    </div>
"""


# =============================================================================
# DEMQOL
# =============================================================================

class DemqolMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Demqol'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_SCORED_QUESTIONS,
            pv=PERMITTED_VALUES,
            comment_fmt="Q{n}. {s} (1 a lot - 4 not at all; -99 no response)",
            comment_strings=[
                # 1-13
                "cheerful", "worried/anxious", "enjoying life", "frustrated",
                "confident", "full of energy", "sad", "lonely", "distressed",
                "lively", "irritable", "fed up", "couldn't do things",
                # 14-19
                "worried: forget recent", "worried: forget people",
                "worried: forget day", "worried: muddled",
                "worried: difficulty making decisions",
                "worried: poor concentration",
                # 20-28
                "worried: not enough company",
                "worried: get on with people close",
                "worried: affection", "worried: people not listening",
                "worried: making self understood", "worried: getting help",
                "worried: toilet", "worried: feel in self",
                "worried: health overall",
            ]
        )
        super().__init__(name, bases, classdict)


class Demqol(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
             metaclass=DemqolMetaclass):
    """
    Server implementation of the DEMQOL task.
    """
    __tablename__ = "demqol"
    shortname = "DEMQOL"
    provides_trackers = True

    q29 = CamcopsColumn(
        "q29", Integer,
        permitted_value_checker=PermittedValueChecker(
            permitted_values=PERMITTED_VALUES),
        comment="Q29. Overall quality of life (1 very good - 4 poor; "
                "-99 no response)."
    )

    NQUESTIONS = 29
    N_SCORED_QUESTIONS = 28
    MINIMUM_N_FOR_TOTAL_SCORE = 14
    REVERSE_SCORE = [1, 3, 5, 6, 10, 29]  # questions scored backwards
    MIN_SCORE = N_SCORED_QUESTIONS
    MAX_SCORE = MIN_SCORE * 4

    COMPLETENESS_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Dementia Quality of Life measure, self-report version")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.COMPLETENESS_FIELDS) and
            self.field_contents_valid()
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="DEMQOL total score",
            axis_label=(
                f"Total score (range {self.MIN_SCORE}–{self.MAX_SCORE}, "
                f"higher better)"
            ),
            axis_min=self.MIN_SCORE - 0.5,
            axis_max=self.MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"Total score {ws.number_to_dp(self.total_score(), DP)} "
            f"(range {self.MIN_SCORE}–{self.MAX_SCORE}, higher better)"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Float(),
                value=self.total_score(),
                comment=f"Total score ({self.MIN_SCORE}-{self.MAX_SCORE})"),
        ]

    def totalscore_extrapolated(self) -> Tuple[float, bool]:
        return calc_total_score(
            obj=self,
            n_scored_questions=self.N_SCORED_QUESTIONS,
            reverse_score_qs=self.REVERSE_SCORE,
            minimum_n_for_total_score=self.MINIMUM_N_FOR_TOTAL_SCORE
        )

    def total_score(self) -> float:
        (total, extrapolated) = self.totalscore_extrapolated()
        return total

    def get_q(self, req: CamcopsRequest, n: int) -> str:
        nstr = str(n)
        return "Q" + nstr + ". " + self.wxstring(req, "proxy_q" + nstr)

    def get_task_html(self, req: CamcopsRequest) -> str:
        (total, extrapolated) = self.totalscore_extrapolated()
        main_dict = {
            None: None,
            1: "1 — " + self.wxstring(req, "a1"),
            2: "2 — " + self.wxstring(req, "a2"),
            3: "3 — " + self.wxstring(req, "a3"),
            4: "4 — " + self.wxstring(req, "a4"),
            MISSING_VALUE: self.wxstring(req, "no_response")
        }
        last_q_dict = {
            None: None,
            1: "1 — " + self.wxstring(req, "q29_a1"),
            2: "2 — " + self.wxstring(req, "q29_a2"),
            3: "3 — " + self.wxstring(req, "q29_a3"),
            4: "4 — " + self.wxstring(req, "q29_a4"),
            MISSING_VALUE: self.wxstring(req, "no_response")
        }
        instruction_dict = {
            1: self.wxstring(req, "instruction11"),
            14: self.wxstring(req, "instruction12"),
            20: self.wxstring(req, "instruction13"),
            29: self.wxstring(req, "instruction14"),
        }
        # https://docs.python.org/2/library/stdtypes.html#mapping-types-dict
        # http://paltman.com/try-except-performance-in-python-a-simple-test/
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>Total score ({self.MIN_SCORE}–{self.MAX_SCORE}),
                            higher better</td>
                        <td>{answer(ws.number_to_dp(total, DP))}</td>
                    </tr>
                    <tr>
                        <td>Total score extrapolated using incomplete
                        responses? <sup>[1]</sup></td>
                        <td>{answer(get_yes_no(req, extrapolated))}</td>
                    </tr>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
        """
        for n in range(1, self.NQUESTIONS + 1):
            if n in instruction_dict:
                h += subheading_spanning_two_columns(instruction_dict.get(n))
            d = main_dict if n <= self.N_SCORED_QUESTIONS else last_q_dict
            q = self.get_q(req, n)
            a = get_from_dict(d, getattr(self, "q" + str(n)))
            h += tr_qa(q, a)
        h += END_DIV + COPYRIGHT_DIV
        return h


# =============================================================================
# DEMQOL-Proxy
# =============================================================================

class DemqolProxyMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['DemqolProxy'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.N_SCORED_QUESTIONS,
            pv=PERMITTED_VALUES,
            comment_fmt="Q{n}. {s} (1 a lot - 4 not at all; -99 no response)",
            comment_strings=[
                # 1-11
                "cheerful", "worried/anxious", "frustrated", "full of energy",
                "sad", "content", "distressed", "lively", "irritable",
                "fed up", "things to look forward to",
                # 12-20
                "worried: memory in general", "worried: forget distant",
                "worried: forget recent", "worried: forget people",
                "worried: forget place", "worried: forget day",
                "worried: muddled",
                "worried: difficulty making decisions",
                "worried: making self understood",
                # 21-31
                "worried: keeping clean", "worried: keeping self looking nice",
                "worried: shopping", "worried: using money to pay",
                "worried: looking after finances", "worried: taking longer",
                "worried: getting in touch with people",
                "worried: not enough company",
                "worried: not being able to help others",
                "worried: not playing a useful part",
                "worried: physical health",
            ]
        )
        super().__init__(name, bases, classdict)


class DemqolProxy(TaskHasPatientMixin, TaskHasRespondentMixin,
                  TaskHasClinicianMixin, Task,
                  metaclass=DemqolProxyMetaclass):
    __tablename__ = "demqolproxy"
    shortname = "DEMQOL-Proxy"
    extrastring_taskname = "demqol"

    q32 = CamcopsColumn(
        "q32", Integer,
        permitted_value_checker=PermittedValueChecker(
            permitted_values=PERMITTED_VALUES),
        comment="Q32. Overall quality of life (1 very good - 4 poor; "
                "-99 no response)."
    )

    NQUESTIONS = 32
    N_SCORED_QUESTIONS = 31
    MINIMUM_N_FOR_TOTAL_SCORE = 16
    REVERSE_SCORE = [1, 4, 6, 8, 11, 32]  # questions scored backwards
    MIN_SCORE = N_SCORED_QUESTIONS
    MAX_SCORE = MIN_SCORE * 4

    COMPLETENESS_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Dementia Quality of Life measure, proxy version")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.COMPLETENESS_FIELDS) and
            self.field_contents_valid()
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="DEMQOL-Proxy total score",
            axis_label=(
                f"Total score (range {self.MIN_SCORE}–{self.MAX_SCORE},"
                f" higher better)"
            ),
            axis_min=self.MIN_SCORE - 0.5,
            axis_max=self.MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"Total score {ws.number_to_dp(self.total_score(), DP)} "
            f"(range {self.MIN_SCORE}–{self.MAX_SCORE}, higher better)"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Float(),
                value=self.total_score(),
                comment=f"Total score ({self.MIN_SCORE}-{self.MAX_SCORE})"),
        ]

    def totalscore_extrapolated(self) -> Tuple[float, bool]:
        return calc_total_score(
            obj=self,
            n_scored_questions=self.N_SCORED_QUESTIONS,
            reverse_score_qs=self.REVERSE_SCORE,
            minimum_n_for_total_score=self.MINIMUM_N_FOR_TOTAL_SCORE
        )

    def total_score(self) -> float:
        (total, extrapolated) = self.totalscore_extrapolated()
        return total

    def get_q(self, req: CamcopsRequest, n: int) -> str:
        nstr = str(n)
        return "Q" + nstr + ". " + self.wxstring(req, "proxy_q" + nstr)

    def get_task_html(self, req: CamcopsRequest) -> str:
        (total, extrapolated) = self.totalscore_extrapolated()
        main_dict = {
            None: None,
            1: "1 — " + self.wxstring(req, "a1"),
            2: "2 — " + self.wxstring(req, "a2"),
            3: "3 — " + self.wxstring(req, "a3"),
            4: "4 — " + self.wxstring(req, "a4"),
            MISSING_VALUE: self.wxstring(req, "no_response")
        }
        last_q_dict = {
            None: None,
            1: "1 — " + self.wxstring(req, "q29_a1"),
            2: "2 — " + self.wxstring(req, "q29_a2"),
            3: "3 — " + self.wxstring(req, "q29_a3"),
            4: "4 — " + self.wxstring(req, "q29_a4"),
            MISSING_VALUE: self.wxstring(req, "no_response")
        }
        instruction_dict = {
            1: self.wxstring(req, "proxy_instruction11"),
            12: self.wxstring(req, "proxy_instruction12"),
            21: self.wxstring(req, "proxy_instruction13"),
            32: self.wxstring(req, "proxy_instruction14"),
        }
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>Total score ({self.MIN_SCORE}–{self.MAX_SCORE}),
                            higher better</td>
                        <td>{answer(ws.number_to_dp(total, DP))}</td>
                    </tr>
                    <tr>
                        <td>Total score extrapolated using incomplete
                        responses? <sup>[1]</sup></td>
                        <td>{answer(get_yes_no(req, extrapolated))}</td>
                    </tr>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
        """
        for n in range(1, self.NQUESTIONS + 1):
            if n in instruction_dict:
                h += subheading_spanning_two_columns(instruction_dict.get(n))
            d = main_dict if n <= self.N_SCORED_QUESTIONS else last_q_dict
            q = self.get_q(req, n)
            a = get_from_dict(d, getattr(self, "q" + str(n)))
            h += tr_qa(q, a)
        h += END_DIV + COPYRIGHT_DIV
        return h


# =============================================================================
# Common scoring function
# =============================================================================

def calc_total_score(obj: Union[Demqol, DemqolProxy],
                     n_scored_questions: int,
                     reverse_score_qs: List[int],
                     minimum_n_for_total_score: int) \
        -> Tuple[Optional[float], bool]:
    """Returns (total, extrapolated?)."""
    n = 0
    total = 0
    for q in range(1, n_scored_questions + 1):
        x = getattr(obj, "q" + str(q))
        if x is None or x == MISSING_VALUE:
            continue
        if q in reverse_score_qs:
            x = 5 - x
        n += 1
        total += x
    if n < minimum_n_for_total_score:
        return None, False
    if n < n_scored_questions:
        return n_scored_questions * total / n, True
    return total, False
