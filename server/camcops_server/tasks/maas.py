#!/usr/bin/env python

"""
camcops_server/tasks/maas.py

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

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.stringfunc import strnumlist, strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_report import (
    AverageScoreReport,
    ScoreDetails,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin


# =============================================================================
# MAAS
# =============================================================================

QUESTION_SNIPPETS = [
    # 1-5
    "thinking about baby",
    "strength of emotional feelings",
    "feelings about baby, negative to positive",
    "desire for info",
    "picturing baby",

    # 6-10
    "baby's personhood",
    "baby depends on me",
    "talking to baby",
    "thoughts, irritation to tender/loving",
    "clarity of mental picture",

    # 11-15
    "emotions about baby, sad to happy",
    "thoughts of punishing baby",
    "emotionally distant or close",
    "good diet",
    "expectation of feelings after birth",

    # 16-19
    "would like to hold baby when",
    "dreams about baby",
    "rubbing over baby",
    "feelings if pregnancy lost",
]


class MaasMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Maas'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, cls.FN_QPREFIX, 1, cls.N_QUESTIONS,
            minimum=cls.MIN_SCORE_PER_Q,
            maximum=cls.MAX_SCORE_PER_Q,
            comment_fmt="Q{n} ({s}; 1 least attachment - 5 most attachment)",
            comment_strings=QUESTION_SNIPPETS
        )
        super().__init__(name, bases, classdict)


class MaasScore(object):
    def __init__(self) -> None:
        self.quality_min = 0
        self.quality_score = 0
        self.quality_max = 0
        self.time_min = 0
        self.time_score = 0
        self.time_max = 0
        self.global_min = 0
        self.global_score = 0
        self.global_max = 0

    def add_question(self, qnum: int, score: Optional[int]) -> None:
        if score is None:
            return
        if qnum in Maas.QUALITY_OF_ATTACHMENT_Q:
            self.quality_min += Maas.MIN_SCORE_PER_Q
            self.quality_score += score
            self.quality_max += Maas.MAX_SCORE_PER_Q
        if qnum in Maas.TIME_IN_ATTACHMENT_MODE_Q:
            self.time_min += Maas.MIN_SCORE_PER_Q
            self.time_score += score
            self.time_max += Maas.MAX_SCORE_PER_Q
        self.global_min += Maas.MIN_SCORE_PER_Q
        self.global_score += score
        self.global_max += Maas.MAX_SCORE_PER_Q


class Maas(TaskHasPatientMixin, Task,
           metaclass=MaasMetaclass):
    """
    Server implementation of the MAAS task.
    """
    __tablename__ = "maas"
    shortname = "MAAS"

    FN_QPREFIX = "q"
    N_QUESTIONS = 19
    MIN_SCORE_PER_Q = 1
    MAX_SCORE_PER_Q = 5
    MIN_GLOBAL = N_QUESTIONS * MIN_SCORE_PER_Q
    MAX_GLOBAL = N_QUESTIONS * MAX_SCORE_PER_Q

    TASK_FIELDS = strseq(FN_QPREFIX, 1, N_QUESTIONS)

    # Questions whose options are presented from 5 to 1, not from 1 to 5:
    # REVERSED_Q = [1, 3, 5, 6, 7, 9, 10, 12, 15, 16, 18]

    # Questions that contribute to the "quality of attachment" score:
    QUALITY_OF_ATTACHMENT_Q = [3, 6, 9, 10, 11, 12, 13, 15, 16, 19]
    QUALITY_OF_ATTACHMENT_FIELDS = strnumlist(FN_QPREFIX,
                                              QUALITY_OF_ATTACHMENT_Q)
    N_QUALITY = len(QUALITY_OF_ATTACHMENT_Q)
    MIN_QUALITY = N_QUALITY * MIN_SCORE_PER_Q
    MAX_QUALITY = N_QUALITY * MAX_SCORE_PER_Q

    # Questions that contribute to the "time spent in attachment mode" score:
    TIME_IN_ATTACHMENT_MODE_Q = [1, 2, 4, 5, 8, 14, 17, 18]
    TIME_IN_ATTACHMENT_FIELDS = strnumlist(FN_QPREFIX,
                                           TIME_IN_ATTACHMENT_MODE_Q)
    N_TIME = len(TIME_IN_ATTACHMENT_MODE_Q)
    MIN_TIME = N_TIME * MIN_SCORE_PER_Q
    MAX_TIME = N_TIME * MAX_SCORE_PER_Q

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Maternal Antenatal Attachment Scale")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def get_score(self) -> MaasScore:
        scorer = MaasScore()
        for q in range(1, self.N_QUESTIONS + 1):
            scorer.add_question(q, getattr(self, self.FN_QPREFIX + str(q)))
        return scorer

    def get_quality_score(self) -> int:
        scorer = self.get_score()
        return scorer.quality_score

    def get_time_score(self) -> int:
        scorer = self.get_score()
        return scorer.time_score

    def get_global_score(self) -> int:
        scorer = self.get_score()
        return scorer.global_score

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        scorer = self.get_score()
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="quality_of_attachment_score", coltype=Integer(),
                value=scorer.quality_score,
                comment=f"Quality of attachment score (for complete tasks, "
                        f"range "
                        f"{self.MIN_QUALITY}-"
                        f"{self.MAX_QUALITY})"),
            SummaryElement(
                name="time_in_attachment_mode_score", coltype=Integer(),
                value=scorer.time_score,
                comment=f"Time spent in attachment mode (or intensity of "
                        f"preoccupation) score (for complete tasks, range "
                        f"{self.MIN_TIME}-"
                        f"{self.MAX_TIME})"),
            SummaryElement(
                name="global_attachment_score", coltype=Integer(),
                value=scorer.global_score,
                comment=f"Global attachment score (for complete tasks, range "
                        f"{self.MIN_GLOBAL}-"
                        f"{self.MAX_GLOBAL})"),
        ]

    def get_task_html(self, req: CamcopsRequest) -> str:
        scorer = self.get_score()
        quality = tr_qa(
            self.wxstring(req, "quality_of_attachment_score") +
            f" [{scorer.quality_min}–{scorer.quality_max}]",
            scorer.quality_score)
        time = tr_qa(
            self.wxstring(req, "time_in_attachment_mode_score") +
            f" [{scorer.time_min}–{scorer.time_max}]",
            scorer.time_score)
        globalscore = tr_qa(
            self.wxstring(req, "global_attachment_score") +
            f" [{scorer.global_min}–{scorer.global_max}]",
            scorer.global_score)
        lines = []  # type: List[str]
        for q in range(1, self.N_QUESTIONS + 1):
            question = f"{q}. " + self.wxstring(req, f"q{q}_q")
            value = getattr(self, self.FN_QPREFIX + str(q))
            answer = None
            if (value is not None and
                    self.MIN_SCORE_PER_Q <= value <= self.MAX_SCORE_PER_Q):
                answer = f"{value}: " + self.wxstring(req, f"q{q}_a{value}")
            lines.append(tr_qa(question, answer))
        q_a = "".join(lines)
        return f"""
          <div class="{CssClass.SUMMARY}">
            <table class="{CssClass.SUMMARY}">
              {self.get_is_complete_tr(req)}
              {quality}
              {time}
              {globalscore}
            </table>
          </div>
          <table class="{CssClass.TASKDETAIL}">
            <tr>
              <th width="60%">Question</th>
              <th width="40%">Answer</th>
            </tr>
            {q_a}
          </table>
          <div class="{CssClass.EXPLANATION}">
            Ratings for each question are from {self.MIN_SCORE_PER_Q} (lowest
            attachment) to {self.MAX_SCORE_PER_Q} (highest attachment). The
            quality of attachment score is the sum of questions
            {self.QUALITY_OF_ATTACHMENT_Q}. The “time spent in attachment mode”
            score is the sum of questions {self.TIME_IN_ATTACHMENT_MODE_Q}. The
            global score is the sum of all questions.
          </div>
          <div class="{CssClass.FOOTNOTES}">
            Condon, J. (2015). Maternal Antenatal Attachment Scale
            [Measurement instrument]. Retrieved from <a
            href="https://hdl.handle.net/2328/35292">https://hdl.handle.net/2328/35292</a>.

            Copyright © John T Condon 2015. This is an Open Access article
            distributed under the terms of the Creative Commons Attribution
            License 3.0 AU (<a
            href="https://creativecommons.org/licenses/by/3.0">https://creativecommons.org/licenses/by/3.0</a>),
            which permits unrestricted use, distribution, and reproduction in
            any medium, provided the original work is properly cited.
          </div>
        """


class MaasReport(AverageScoreReport):
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "MAAS"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("MAAS — Average scores")

    # noinspection PyMethodParameters
    @classproperty
    def task_class(cls) -> Type[Task]:
        return Maas

    @classmethod
    def scoretypes(cls, req: "CamcopsRequest") -> List[ScoreDetails]:
        _ = req.gettext
        return [
            ScoreDetails(
                name=_("Global attachment score"),
                scorefunc=Maas.get_global_score,
                minimum=Maas.MIN_GLOBAL,
                maximum=Maas.MAX_GLOBAL,
                higher_score_is_better=True
            ),
            ScoreDetails(
                name=_("Quality of attachment score"),
                scorefunc=Maas.get_quality_score,
                minimum=Maas.MIN_QUALITY,
                maximum=Maas.MAX_QUALITY,
                higher_score_is_better=True
            ),
            ScoreDetails(
                name=_("Time spent in attachment mode"),
                scorefunc=Maas.get_time_score,
                minimum=Maas.MIN_TIME,
                maximum=Maas.MAX_TIME,
                higher_score_is_better=True
            )
        ]
