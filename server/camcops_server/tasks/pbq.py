#!/usr/bin/env python

"""
camcops_server/tasks/pbq.py

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
from cardinal_pythonlib.stringfunc import strnumlist, strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import answer, tr
from camcops_server.cc_modules.cc_report import (
    AverageScoreReport,
    ScoreDetails,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# PBQ
# =============================================================================

class PbqMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Pbq'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        comment_strings = [
            # This is the Brockington 2006 order; see XML for notes.
            # 1-5
            "I feel close to my baby",
            "I wish the old days when I had no baby would come back",
            "I feel distant from my baby",
            "I love to cuddle my baby",
            "I regret having this baby",
            # 6-10
            "The baby does not seem to be mine",
            "My baby winds me up",
            "I love my baby to bits",
            "I feel happy when my baby smiles or laughs",
            "My baby irritates me",
            # 11-15
            "I enjoy playing with my baby",
            "My baby cries too much",
            "I feel trapped as a mother",
            "I feel angry with my baby",
            "I resent my baby",
            # 16-20
            "My baby is the most beautiful baby in the world",
            "I wish my baby would somehow go away",
            "I have done harmful things to my baby",
            "My baby makes me anxious",
            "I am afraid of my baby",
            # 21-25
            "My baby annoys me",
            "I feel confident when changing my baby",
            "I feel the only solution is for someone else to look after my baby",  # noqa
            "I feel like hurting my baby",
            "My baby is easily comforted",
        ]
        pvc = PermittedValueChecker(minimum=cls.MIN_PER_Q,
                                    maximum=cls.MAX_PER_Q)
        for n in range(1, cls.NQUESTIONS + 1):
            i = n - 1
            colname = f"q{n}"
            if n in cls.SCORED_A0N5_Q:
                explan = "always 0 - never 5"
            else:
                explan = "always 5 - never 0"
            comment = f"Q{n}, {comment_strings[i]} ({explan}, higher worse)"
            setattr(
                cls,
                colname,
                CamcopsColumn(colname, Integer,
                              comment=comment, permitted_value_checker=pvc)
            )
        super().__init__(name, bases, classdict)


class Pbq(TaskHasPatientMixin, Task,
          metaclass=PbqMetaclass):
    """
    Server implementation of the PBQ task.
    """
    __tablename__ = "pbq"
    shortname = "PBQ"
    provides_trackers = True

    MIN_PER_Q = 0
    MAX_PER_Q = 5
    NQUESTIONS = 25
    QUESTION_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_TOTAL = MAX_PER_Q * NQUESTIONS
    SCORED_A0N5_Q = [1, 4, 8, 9, 11, 16, 22, 25]  # rest scored A5N0
    FACTOR_1_Q = [1, 2, 6, 7, 8, 9, 10, 12, 13, 15, 16, 17]  # 12 questions  # noqa
    FACTOR_2_Q = [3, 4, 5, 11, 14, 21, 23]  # 7 questions
    FACTOR_3_Q = [19, 20, 22, 25]  # 4 questions
    FACTOR_4_Q = [18, 24]  # 2 questions
    FACTOR_1_F = strnumlist("q", FACTOR_1_Q)
    FACTOR_2_F = strnumlist("q", FACTOR_2_Q)
    FACTOR_3_F = strnumlist("q", FACTOR_3_Q)
    FACTOR_4_F = strnumlist("q", FACTOR_4_Q)
    FACTOR_1_MAX = len(FACTOR_1_Q) * MAX_PER_Q
    FACTOR_2_MAX = len(FACTOR_2_Q) * MAX_PER_Q
    FACTOR_3_MAX = len(FACTOR_3_Q) * MAX_PER_Q
    FACTOR_4_MAX = len(FACTOR_4_Q) * MAX_PER_Q

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Postpartum Bonding Questionnaire")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="PBQ total score (lower is better)",
            axis_label=f"Total score (out of {self.MAX_TOTAL})",
            axis_min=-0.5,
            axis_max=self.MAX_TOTAL + 0.5
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total_score", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/ {self.MAX_TOTAL})"
            ),
            SummaryElement(
                name="factor_1_score", coltype=Integer(),
                value=self.factor_1_score(),
                comment=f"Factor 1 score (/ {self.FACTOR_1_MAX})"
            ),
            SummaryElement(
                name="factor_2_score", coltype=Integer(),
                value=self.factor_2_score(),
                comment=f"Factor 2 score (/ {self.FACTOR_2_MAX})"
            ),
            SummaryElement(
                name="factor_3_score", coltype=Integer(),
                value=self.factor_3_score(),
                comment=f"Factor 3 score (/ {self.FACTOR_3_MAX})"
            ),
            SummaryElement(
                name="factor_4_score", coltype=Integer(),
                value=self.factor_4_score(),
                comment=f"Factor 4 score (/ {self.FACTOR_4_MAX})"
            ),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"PBQ total score {self.total_score()}/{self.MAX_TOTAL}. "
            f"Factor 1 score {self.factor_1_score()}/{self.FACTOR_1_MAX}. "
            f"Factor 2 score {self.factor_2_score()}/{self.FACTOR_2_MAX}. "
            f"Factor 3 score {self.factor_3_score()}/{self.FACTOR_3_MAX}. "
            f"Factor 4 score {self.factor_4_score()}/{self.FACTOR_4_MAX}."
        ))]

    def total_score(self) -> int:
        return self.sum_fields(self.QUESTION_FIELDS)

    def factor_1_score(self) -> int:
        return self.sum_fields(self.FACTOR_1_F)

    def factor_2_score(self) -> int:
        return self.sum_fields(self.FACTOR_2_F)

    def factor_3_score(self) -> int:
        return self.sum_fields(self.FACTOR_3_F)

    def factor_4_score(self) -> int:
        return self.sum_fields(self.FACTOR_4_F)

    def is_complete(self) -> bool:
        return (
            self.field_contents_valid() and
            self.all_fields_not_none(self.QUESTION_FIELDS)
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        always = self.xstring(req, "always")
        very_often = self.xstring(req, "very_often")
        quite_often = self.xstring(req, "quite_often")
        sometimes = self.xstring(req, "sometimes")
        rarely = self.xstring(req, "rarely")
        never = self.xstring(req, "never")
        a0n5 = {
            0: always,
            1: very_often,
            2: quite_often,
            3: sometimes,
            4: rarely,
            5: never,
        }
        a5n0 = {
            5: always,
            4: very_often,
            3: quite_often,
            2: sometimes,
            1: rarely,
            0: never,
        }
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>Total score</td>
                        <td>{answer(self.total_score())} / {self.MAX_TOTAL}</td>
                    </td>
                    <tr>
                        <td>Factor 1 score <sup>[1]</sup></td>
                        <td>{answer(self.factor_1_score())} / {self.FACTOR_1_MAX}</td>
                    </td>
                    <tr>
                        <td>Factor 2 score <sup>[2]</sup></td>
                        <td>{answer(self.factor_2_score())} / {self.FACTOR_2_MAX}</td>
                    </td>
                    <tr>
                        <td>Factor 3 score <sup>[3]</sup></td>
                        <td>{answer(self.factor_3_score())} / {self.FACTOR_3_MAX}</td>
                    </td>
                    <tr>
                        <td>Factor 4 score <sup>[4]</sup></td>
                        <td>{answer(self.factor_4_score())} / {self.FACTOR_4_MAX}</td>
                    </td>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer ({self.MIN_PER_Q}–{self.MAX_PER_Q})</th>
                </tr>
        """  # noqa
        for q in range(1, self.NQUESTIONS + 1):
            qtext = f"{q}. " + self.wxstring(req, f"q{q}")
            a = getattr(self, f"q{q}")
            option = a0n5.get(a) if q in self.SCORED_A0N5_Q else a5n0.get(a)
            atext = f"{a}: {option}"
            h += tr(qtext, answer(atext))
        h += f"""
            </table>
            <div class="{CssClass.FOOTNOTES}">
                Factors and cut-off scores are from Brockington et al. (2006,
                PMID 16673041), as follows.
                [1] General factor; ≤11 normal, ≥12 high; based on questions
                    {", ".join(str(x) for x in self.FACTOR_1_Q)}.
                [2] Factor examining severe mother–infant relationship
                    disorders; ≤12 normal, ≥13 high (cf. original 2001 study
                    with ≤16 normal, ≥17 high); based on questions
                    {", ".join(str(x) for x in self.FACTOR_2_Q)}.
                [3] Factor relating to infant-focused anxiety; ≤9 normal, ≥10
                    high; based on questions
                    {", ".join(str(x) for x in self.FACTOR_3_Q)}.
                [4] Factor relating to thoughts of harm to infant; ≤1 normal,
                    ≥2 high (cf. original 2001 study with ≤2 normal, ≥3 high);
                    known low sensitivity; based on questions
                    {", ".join(str(x) for x in self.FACTOR_4_Q)}.
            </div>
        """
        return h

    # No SNOMED codes for the PBQ (checked 2019-04-01).


class PBQReport(AverageScoreReport):
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "PBQ"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("PBQ — Average scores")

    # noinspection PyMethodParameters
    @classproperty
    def task_class(cls) -> Type[Task]:
        return Pbq

    @classmethod
    def scoretypes(cls, req: "CamcopsRequest") -> List[ScoreDetails]:
        _ = req.gettext
        return [
            ScoreDetails(
                name=_("Total score"),
                scorefunc=Pbq.total_score,
                minimum=0,
                maximum=Pbq.MAX_TOTAL,
                higher_score_is_better=False
            ),
            ScoreDetails(
                name=_("Factor 1 score"),
                scorefunc=Pbq.factor_1_score,
                minimum=0,
                maximum=Pbq.FACTOR_1_MAX,
                higher_score_is_better=False
            ),
            ScoreDetails(
                name=_("Factor 2 score"),
                scorefunc=Pbq.factor_2_score,
                minimum=0,
                maximum=Pbq.FACTOR_2_MAX,
                higher_score_is_better=False
            ),
            ScoreDetails(
                name=_("Factor 3 score"),
                scorefunc=Pbq.factor_3_score,
                minimum=0,
                maximum=Pbq.FACTOR_3_MAX,
                higher_score_is_better=False
            ),
            ScoreDetails(
                name=_("Factor 4 score"),
                scorefunc=Pbq.factor_4_score,
                minimum=0,
                maximum=Pbq.FACTOR_4_MAX,
                higher_score_is_better=False
            ),
        ]
