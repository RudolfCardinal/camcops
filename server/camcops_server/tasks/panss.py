#!/usr/bin/env python

"""
camcops_server/tasks/panss.py

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

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_ONLY_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# PANSS
# =============================================================================

class PanssMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Panss'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "p", 1, cls.NUM_P,
            minimum=1, maximum=7,
            comment_fmt="P{n}: {s} (1 absent - 7 extreme)",
            comment_strings=[
                "delusions", "conceptual disorganisation",
                "hallucinatory behaviour", "excitement",
                "grandiosity", "suspiciousness/persecution",
                "hostility",
            ]
        )
        add_multiple_columns(
            cls, "n", 1, cls.NUM_N,
            minimum=1, maximum=7,
            comment_fmt="N{n}: {s} (1 absent - 7 extreme)",
            comment_strings=[
                "blunted affect", "emotional withdrawal",
                "poor rapport", "passive/apathetic social withdrawal",
                "difficulty in abstract thinking",
                "lack of spontaneity/conversation flow",
                "stereotyped thinking",
            ]
        )
        add_multiple_columns(
            cls, "g", 1, cls.NUM_G,
            minimum=1, maximum=7,
            comment_fmt="G{n}: {s} (1 absent - 7 extreme)",
            comment_strings=[
                "somatic concern",
                "anxiety",
                "guilt feelings",
                "tension",
                "mannerisms/posturing",
                "depression",
                "motor retardation",
                "uncooperativeness",
                "unusual thought content",
                "disorientation",
                "poor attention",
                "lack of judgement/insight",
                "disturbance of volition",
                "poor impulse control",
                "preoccupation",
                "active social avoidance",
            ]
        )
        super().__init__(name, bases, classdict)


class Panss(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
            metaclass=PanssMetaclass):
    """
    Server implementation of the PANSS task.
    """
    __tablename__ = "panss"
    shortname = "PANSS"
    provides_trackers = True

    NUM_P = 7
    NUM_N = 7
    NUM_G = 16

    P_FIELDS = strseq("p", 1, NUM_P)
    N_FIELDS = strseq("n", 1, NUM_N)
    G_FIELDS = strseq("g", 1, NUM_G)
    TASK_FIELDS = P_FIELDS + N_FIELDS + G_FIELDS

    MIN_P = 1 * NUM_P
    MAX_P = 7 * NUM_P
    MIN_N = 1 * NUM_N
    MAX_N = 7 * NUM_N
    MIN_G = 1 * NUM_G
    MAX_G = 7 * NUM_G
    MIN_TOTAL = MIN_P + MIN_N + MIN_G
    MAX_TOTAL = MAX_P + MAX_N + MAX_G
    MIN_P_MINUS_N = MIN_P - MAX_N
    MAX_P_MINUS_N = MAX_P - MIN_N

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Positive and Negative Syndrome Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [
            TrackerInfo(
                value=self.total_score(),
                plot_label="PANSS total score",
                axis_label=f"Total score ({self.MIN_TOTAL}-{self.MAX_TOTAL})",
                axis_min=self.MIN_TOTAL - 0.5,
                axis_max=self.MAX_TOTAL + 0.5
            ),
            TrackerInfo(
                value=self.score_p(),
                plot_label="PANSS P score",
                axis_label=f"P score ({self.MIN_P}-{self.MAX_P})",
                axis_min=self.MIN_P - 0.5,
                axis_max=self.MAX_P + 0.5
            ),
            TrackerInfo(
                value=self.score_n(),
                plot_label="PANSS N score",
                axis_label=f"N score ({self.MIN_N}-{self.MAX_N})",
                axis_min=self.MIN_N - 0.5,
                axis_max=self.MAX_N + 0.5
            ),
            TrackerInfo(
                value=self.score_g(),
                plot_label="PANSS G score",
                axis_label=f"G score ({self.MIN_G}-{self.MAX_G})",
                axis_min=self.MIN_G - 0.5,
                axis_max=self.MAX_G + 0.5
            ),
            TrackerInfo(
                value=self.composite(),
                plot_label=f"PANSS composite score "
                           f"({self.MIN_P_MINUS_N} to {self.MAX_P_MINUS_N})",
                axis_label="P - N"
            ),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=(
                f"PANSS total score {self.total_score()} "
                f"(P {self.score_p()}, N {self.score_n()}, G {self.score_g()}, "
                f"composite P–N {self.composite()})"
            )
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score ({self.MIN_TOTAL}-{self.MAX_TOTAL})"
            ),
            SummaryElement(
                name="p",
                coltype=Integer(),
                value=self.score_p(),
                comment=f"Positive symptom (P) score ({self.MIN_P}-{self.MAX_P})"  # noqa
            ),
            SummaryElement(
                name="n",
                coltype=Integer(),
                value=self.score_n(),
                comment=f"Negative symptom (N) score ({self.MIN_N}-{self.MAX_N})"  # noqa
            ),
            SummaryElement(
                name="g",
                coltype=Integer(),
                value=self.score_g(),
                comment=f"General symptom (G) score ({self.MIN_G}-{self.MAX_G})"  # noqa
            ),
            SummaryElement(
                name="composite",
                coltype=Integer(),
                value=self.composite(),
                comment=f"Composite score (P - N) ({self.MIN_P_MINUS_N} "
                        f"to {self.MAX_P_MINUS_N})"
            ),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def score_p(self) -> int:
        return self.sum_fields(self.P_FIELDS)

    def score_n(self) -> int:
        return self.sum_fields(self.N_FIELDS)

    def score_g(self) -> int:
        return self.sum_fields(self.G_FIELDS)

    def composite(self) -> int:
        return self.score_p() - self.score_n()

    def get_task_html(self, req: CamcopsRequest) -> str:
        p = self.score_p()
        n = self.score_n()
        g = self.score_g()
        composite = self.composite()
        total = p + n + g
        answers = {
            None: None,
            1: self.wxstring(req, "option1"),
            2: self.wxstring(req, "option2"),
            3: self.wxstring(req, "option3"),
            4: self.wxstring(req, "option4"),
            5: self.wxstring(req, "option5"),
            6: self.wxstring(req, "option6"),
            7: self.wxstring(req, "option7"),
        }
        q_a = ""
        for q in self.TASK_FIELDS:
            q_a += tr_qa(
                self.wxstring(req, "" + q + "_s"),
                get_from_dict(answers, getattr(self, q))
            )
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {p}
                    {n}
                    {g}
                    {composite}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="40%">Question</th>
                    <th width="60%">Answer</th>
                </tr>
                {q_a}
            </table>
            {DATA_COLLECTION_ONLY_DIV}
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr_qa(
                f"{req.sstring(SS.TOTAL_SCORE)} "
                f"({self.MIN_TOTAL}–{self.MAX_TOTAL})",
                total
            ),
            p=tr_qa(
                f"{self.wxstring(req, 'p')} ({self.MIN_P}–{self.MAX_P})",
                p
            ),
            n=tr_qa(
                f"{self.wxstring(req, 'n')} ({self.MIN_N}–{self.MAX_N})",
                n
            ),
            g=tr_qa(
                f"{self.wxstring(req, 'g')} ({self.MIN_G}–{self.MAX_G})",
                g
            ),
            composite=tr_qa(
                f"{self.wxstring(req, 'composite')} "
                f"({self.MIN_P_MINUS_N}–{self.MAX_P_MINUS_N})",
                composite
            ),
            q_a=q_a,
            DATA_COLLECTION_ONLY_DIV=DATA_COLLECTION_ONLY_DIV,
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [SnomedExpression(req.snomed(SnomedLookup.PANSS_SCALE))]
