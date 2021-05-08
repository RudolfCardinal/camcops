#!/usr/bin/env python

"""
camcops_server/tasks/bdi.py

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
import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, String

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_ONLY_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, bold, td, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# Constants
# =============================================================================

BDI_I_QUESTION_TOPICS = {
    # from Beck 1988, https://doi.org/10.1016/0272-7358(88)90050-5
    1: "mood",  # a
    2: "pessimism",  # b
    3: "sense of failure",  # c
    4: "lack of satisfaction",  # d
    5: "guilt feelings",  # e
    6: "sense of punishment",  # f
    7: "self-dislike",  # g
    8: "self-accusation",  # h
    9: "suicidal wishes",  # i
    10: "crying",  # j
    11: "irritability",  # k
    12: "social withdrawal",  # l
    13: "indecisiveness",  # m
    14: "distortion of body image",  # n
    15: "work inhibition",  # o
    16: "sleep disturbance",  # p
    17: "fatigability",  # q
    18: "loss of appetite",  # r
    19: "weight loss",  # s
    20: "somatic preoccupation",  # t
    21: "loss of libido",  # u
}
BDI_IA_QUESTION_TOPICS = {
    # from [Beck1996b]
    1: "sadness",
    2: "pessimism",
    3: "sense of failure",
    4: "self-dissatisfaction",
    5: "guilt",
    6: "punishment",
    7: "self-dislike",
    8: "self-accusations",
    9: "suicidal ideas",
    10: "crying",
    11: "irritability",
    12: "social withdrawal",
    13: "indecisiveness",
    14: "body image change",
    15: "work difficulty",
    16: "insomnia",
    17: "fatigability",
    18: "loss of appetite",
    19: "weight loss",
    20: "somatic preoccupation",
    21: "loss of libido",
}
BDI_II_QUESTION_TOPICS = {
    # from https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5889520/;
    # also https://www.ncbi.nlm.nih.gov/pubmed/10100838;
    # also [Beck1996b]
    # matches BDI-II paper version
    1: "sadness",
    2: "pessimism",
    3: "past failure",
    4: "loss of pleasure",
    5: "guilty feelings",
    6: "punishment feelings",
    7: "self-dislike",
    8: "self-criticalness",
    9: "suicidal thoughts or wishes",
    10: "crying",
    11: "agitation",
    12: "loss of interest",
    13: "indecisiveness",
    14: "worthlessness",
    15: "loss of energy",
    16: "changes in sleeping pattern",  # decrease or increase
    17: "irritability",
    18: "changes in appetite",  # decrease or increase
    19: "concentration difficulty",
    20: "tiredness or fatigue",
    21: "loss of interest in sex",
}
SCALE_BDI_I = "BDI-I"  # must match client
SCALE_BDI_IA = "BDI-IA"  # must match client
SCALE_BDI_II = "BDI-II"  # must match client
TOPICS_BY_SCALE = {
    SCALE_BDI_I: BDI_I_QUESTION_TOPICS,
    SCALE_BDI_IA: BDI_IA_QUESTION_TOPICS,
    SCALE_BDI_II: BDI_II_QUESTION_TOPICS,
}

NQUESTIONS = 21
TASK_SCORED_FIELDS = strseq("q", 1, NQUESTIONS)
MAX_SCORE = NQUESTIONS * 3
SUICIDALITY_QNUM = 9  # Q9 in all versions of the BDI (I, IA, II)
SUICIDALITY_FN = "q9"  # fieldname
CUSTOM_SOMATIC_KHANDAKER_BDI_II_QNUMS = [4, 15, 16, 18, 19, 20, 21]
CUSTOM_SOMATIC_KHANDAKER_BDI_II_FIELDS = Task.fieldnames_from_list(
    "q", CUSTOM_SOMATIC_KHANDAKER_BDI_II_QNUMS)


# =============================================================================
# BDI (crippled)
# =============================================================================

class BdiMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Bdi'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, NQUESTIONS,
            minimum=0, maximum=3,
            comment_fmt="Q{n} [{s}] (0-3, higher worse)",
            comment_strings=[
                (
                    f"BDI-I: {BDI_I_QUESTION_TOPICS[q]}; "
                    f"BDI-IA: {BDI_IA_QUESTION_TOPICS[q]}; "
                    f"BDI-II: {BDI_II_QUESTION_TOPICS[q]}"
                )
                for q in range(1, NQUESTIONS + 1)
            ]
        )
        super().__init__(name, bases, classdict)


class Bdi(TaskHasPatientMixin, Task,
          metaclass=BdiMetaclass):
    """
    Server implementation of the BDI task.
    """
    __tablename__ = "bdi"
    shortname = "BDI"
    provides_trackers = True

    bdi_scale = Column(
        "bdi_scale", String(length=10),  # was Text
        comment="Which BDI scale (BDI-I, BDI-IA, BDI-II)?"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Beck Depression Inventory (data collection only)")

    def is_complete(self) -> bool:
        return (
            self.field_contents_valid() and
            self.bdi_scale is not None and
            self.all_fields_not_none(TASK_SCORED_FIELDS)
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="BDI total score (rating depressive symptoms)",
            axis_label=f"Score for Q1-21 (out of {MAX_SCORE})",
            axis_min=-0.5,
            axis_max=MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"{ws.webify(self.bdi_scale)} "
            f"total score {self.total_score()}/{MAX_SCORE}"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{MAX_SCORE})"),
        ]

    def total_score(self) -> int:
        return self.sum_fields(TASK_SCORED_FIELDS)

    def is_bdi_ii(self) -> bool:
        return self.bdi_scale == SCALE_BDI_II

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()

        # Suicidal thoughts:
        suicidality_score = getattr(self, SUICIDALITY_FN)
        if suicidality_score is None:
            suicidality_text = bold("? (not completed)")
            suicidality_css_class = CssClass.INCOMPLETE
        elif suicidality_score == 0:
            suicidality_text = str(suicidality_score)
            suicidality_css_class = ""
        else:
            suicidality_text = bold(str(suicidality_score))
            suicidality_css_class = CssClass.WARNING

        # Custom somatic score for Khandaker Insight study:
        somatic_css_class = ""
        if self.is_bdi_ii():
            somatic_values = self.get_values(
                CUSTOM_SOMATIC_KHANDAKER_BDI_II_FIELDS)
            somatic_missing = False
            somatic_score = 0
            for v in somatic_values:
                if v is None:
                    somatic_missing = True
                    somatic_css_class = CssClass.INCOMPLETE
                    break
                else:
                    somatic_score += int(v)
            somatic_text = ("incomplete" if somatic_missing
                            else str(somatic_score))
        else:
            somatic_text = "N/A"  # not the BDI-II

        # Question rows:
        q_a = ""
        qdict = TOPICS_BY_SCALE.get(self.bdi_scale)
        topic = "?"
        for q in range(1, NQUESTIONS + 1):
            if qdict:
                topic = qdict.get(q, "??")
            q_a += tr_qa(
                f"{req.sstring(SS.QUESTION)} {q} ({topic})",
                getattr(self, "q" + str(q))
            )

        # HTML:
        tr_somatic_score = tr(
            td(
                "Custom somatic score for Insight study <sup>[2]</sup> "
                "(sum of scores for questions {}, for BDI-II only)".format(
                    ", ".join("Q" + str(qnum) for qnum in
                              CUSTOM_SOMATIC_KHANDAKER_BDI_II_QNUMS))
            ),
            td(somatic_text, td_class=somatic_css_class),
            literal=True
        )
        tr_which_scale = tr_qa(
            req.wappstring(AS.BDI_WHICH_SCALE) + " <sup>[3]</sup>",
            ws.webify(self.bdi_scale)
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr(req.sstring(SS.TOTAL_SCORE),
                        answer(score) + " / {}".format(MAX_SCORE))}
                    <tr>
                        <td>
                            Suicidal thoughts/wishes score
                            (Q{SUICIDALITY_QNUM}) <sup>[1]</sup>
                        </td>
                        {td(suicidality_text, td_class=suicidality_css_class)}
                    </tr>
                    {tr_somatic_score}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                All questions are scored from 0–3
                (0 free of symptoms, 3 most symptomatic).
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="70%">Question</th>
                    <th width="30%">Answer</th>
                </tr>
                {tr_which_scale}
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Suicidal thoughts are asked about in Q{SUICIDALITY_QNUM}
                    for all of: BDI-I (1961), BDI-IA (1978), and BDI-II (1996).

                [2] Insight study:
                    <a href="https://doi.org/10.1186/ISRCTN16942542">doi:10.1186/ISRCTN16942542</a>

                [3] See the
                    <a href="https://camcops.readthedocs.io/en/latest/tasks/bdi.html">CamCOPS
                    BDI help</a> for full references and bibliography for the
                    citations that follow.

                    <b>The BDI rates “right now” [Beck1988].
                    The BDI-IA rates the past week [Beck1988].
                    The BDI-II rates the past two weeks [Beck1996b].</b>

                    1961 BDI(-I) question topics from [Beck1988].
                    1978 BDI-IA question topics from [Beck1996b].
                    1996 BDI-II question topics from [Steer1999], [Gary2018].
                </ul>

            </div>
            {DATA_COLLECTION_ONLY_DIV}
        """  # noqa

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        scale_lookup = SnomedLookup.BDI_SCALE
        if self.bdi_scale in [SCALE_BDI_I, SCALE_BDI_IA]:
            score_lookup = SnomedLookup.BDI_SCORE
            proc_lookup = SnomedLookup.BDI_PROCEDURE_ASSESSMENT
        elif self.bdi_scale == SCALE_BDI_II:
            score_lookup = SnomedLookup.BDI_II_SCORE
            proc_lookup = SnomedLookup.BDI_II_PROCEDURE_ASSESSMENT
        else:
            return []
        codes = [SnomedExpression(req.snomed(proc_lookup))]
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(scale_lookup),
                {
                    req.snomed(score_lookup): self.total_score(),
                }
            ))
        return codes
