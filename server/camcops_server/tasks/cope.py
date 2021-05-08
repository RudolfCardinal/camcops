#!/usr/bin/env python

"""
camcops_server/tasks/cope.py

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

from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    BIT_CHECKER,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)


# =============================================================================
# COPE_Brief
# =============================================================================

class CopeBriefMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['CopeBrief'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=3,
            comment_fmt="Q{n}, {s} (0 not at all - 3 a lot)",
            comment_strings=[
                "work/activities to take mind off",  # 1
                "concentrating efforts on doing something about it",
                "saying it's unreal",
                "alcohol/drugs to feel better",
                "emotional support from others",  # 5
                "given up trying to deal with it",
                "taking action to make situation better",
                "refusing to believe it's happened",
                "saying things to let unpleasant feelings escape",
                "getting help/advice from others",  # 10
                "alcohol/drugs to get through it",
                "trying to see it in a more positive light",
                "criticizing myself",
                "trying to come up with a strategy",
                "getting comfort/understanding from someone",  # 15
                "giving up the attempt to cope",
                "looking for something good in what's happening",
                "making jokes about it",
                "doing something to think about it less",
                "accepting reality of the fact it's happened",  # 20
                "expressing negative feelings",
                "seeking comfort in religion/spirituality",
                "trying to get help/advice from others about what to do",
                "learning to live with it",
                "thinking hard about what steps to take",  # 25
                "blaming myself",
                "praying/meditating",
                "making fun of the situation"  # 28
            ]
        )
        super().__init__(name, bases, classdict)


class CopeBrief(TaskHasPatientMixin, Task,
                metaclass=CopeBriefMetaclass):
    """
    Server implementation of the COPE-Brief task.
    """
    __tablename__ = "cope_brief"
    shortname = "COPE-Brief"
    extrastring_taskname = "cope"

    NQUESTIONS = 28
    RELATIONSHIP_OTHER_CODE = 0
    RELATIONSHIPS_FIRST = 0
    RELATIONSHIPS_FIRST_NON_OTHER = 1
    RELATIONSHIPS_LAST = 9

    completed_by_patient = CamcopsColumn(
        "completed_by_patient", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Task completed by patient? (0 no, 1 yes)"
    )
    completed_by = Column(
        "completed_by", UnicodeText,
        comment="Name of person task completed by (if not by patient)"
    )
    relationship_to_patient = CamcopsColumn(
        "relationship_to_patient", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=9),
        comment="Relationship of responder to patient (0 other, 1 wife, "
                "2 husband, 3 daughter, 4 son, 5 sister, 6 brother, "
                "7 mother, 8 father, 9 friend)"
    )
    relationship_to_patient_other = Column(
        "relationship_to_patient_other", UnicodeText,
        comment="Relationship of responder to patient (if OTHER chosen)"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Brief COPE Inventory")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="self_distraction",
                           coltype=Integer(),
                           value=self.self_distraction(),
                           comment="Self-distraction (2-8)"),
            SummaryElement(name="active_coping",
                           coltype=Integer(),
                           value=self.active_coping(),
                           comment="Active coping (2-8)"),
            SummaryElement(name="denial",
                           coltype=Integer(),
                           value=self.denial(),
                           comment="Denial (2-8)"),
            SummaryElement(name="substance_use",
                           coltype=Integer(),
                           value=self.substance_use(),
                           comment="Substance use (2-8)"),
            SummaryElement(name="emotional_support",
                           coltype=Integer(),
                           value=self.emotional_support(),
                           comment="Use of emotional support (2-8)"),
            SummaryElement(name="instrumental_support",
                           coltype=Integer(),
                           value=self.instrumental_support(),
                           comment="Use of instrumental support (2-8)"),
            SummaryElement(name="behavioural_disengagement",
                           coltype=Integer(),
                           value=self.behavioural_disengagement(),
                           comment="Behavioural disengagement (2-8)"),
            SummaryElement(name="venting",
                           coltype=Integer(),
                           value=self.venting(),
                           comment="Venting (2-8)"),
            SummaryElement(name="positive_reframing",
                           coltype=Integer(),
                           value=self.positive_reframing(),
                           comment="Positive reframing (2-8)"),
            SummaryElement(name="planning",
                           coltype=Integer(),
                           value=self.planning(),
                           comment="Planning (2-8)"),
            SummaryElement(name="humour",
                           coltype=Integer(),
                           value=self.humour(),
                           comment="Humour (2-8)"),
            SummaryElement(name="acceptance",
                           coltype=Integer(),
                           value=self.acceptance(),
                           comment="Acceptance (2-8)"),
            SummaryElement(name="religion",
                           coltype=Integer(),
                           value=self.religion(),
                           comment="Religion (2-8)"),
            SummaryElement(name="self_blame",
                           coltype=Integer(),
                           value=self.self_blame(),
                           comment="Self-blame (2-8)"),
        ]

    def is_complete_responder(self) -> bool:
        if self.completed_by_patient is None:
            return False
        if self.completed_by_patient:
            return True
        if not self.completed_by or self.relationship_to_patient is None:
            return False
        if (self.relationship_to_patient == self.RELATIONSHIP_OTHER_CODE and
                not self.relationship_to_patient_other):
            return False
        return True

    def is_complete(self) -> bool:
        return (
            self.is_complete_responder() and
            self.all_fields_not_none(
                [f"q{n}" for n in range(1, self.NQUESTIONS + 1)]) and
            self.field_contents_valid()
        )

    def self_distraction(self) -> int:
        return self.sum_fields(["q1", "q19"])

    def active_coping(self) -> int:
        return self.sum_fields(["q2", "q7"])

    def denial(self) -> int:
        return self.sum_fields(["q3", "q8"])

    def substance_use(self) -> int:
        return self.sum_fields(["q4", "q11"])

    def emotional_support(self) -> int:
        return self.sum_fields(["q5", "q15"])

    def instrumental_support(self) -> int:
        return self.sum_fields(["q10", "q23"])

    def behavioural_disengagement(self) -> int:
        return self.sum_fields(["q6", "q16"])

    def venting(self) -> int:
        return self.sum_fields(["q9", "q21"])

    def positive_reframing(self) -> int:
        return self.sum_fields(["q12", "q17"])

    def planning(self) -> int:
        return self.sum_fields(["q14", "q25"])

    def humour(self) -> int:
        return self.sum_fields(["q18", "q28"])

    def acceptance(self) -> int:
        return self.sum_fields(["q20", "q24"])

    def religion(self) -> int:
        return self.sum_fields(["q22", "q27"])

    def self_blame(self) -> int:
        return self.sum_fields(["q13", "q26"])

    def get_task_html(self, req: CamcopsRequest) -> str:
        answer_dict = {None: None}
        for option in range(0, 3 + 1):
            answer_dict[option] = (
                str(option) + " — " + self.wxstring(req, "a" + str(option))
            )
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += tr_qa(
                f"Q{q}. {self.wxstring(req, 'q' + str(q))}",
                get_from_dict(answer_dict, getattr(self, "q" + str(q)))
            )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_qa("Self-distraction (Q1, Q19)",
                           self.self_distraction())}
                    {tr_qa("Active coping (Q2, Q7)", self.active_coping())}
                    {tr_qa("Denial (Q3, Q8)", self.denial())}
                    {tr_qa("Substance use (Q4, Q11)", self.substance_use())}
                    {tr_qa("Use of emotional support (Q5, Q15)",
                           self.emotional_support())}
                    {tr_qa("Use of instrumental support (Q10, Q23)",
                           self.instrumental_support())}
                    {tr_qa("Behavioural disengagement (Q6, Q16)",
                           self.behavioural_disengagement())}
                    {tr_qa("Venting (Q9, Q21)", self.venting())}
                    {tr_qa("Positive reframing (Q12, Q17)",
                           self.positive_reframing())}
                    {tr_qa("Planning (Q14, Q25)", self.planning())}
                    {tr_qa("Humour (Q18, Q28)", self.humour())}
                    {tr_qa("Acceptance (Q20, Q24)", self.acceptance())}
                    {tr_qa("Religion (Q22, Q27)", self.religion())}
                    {tr_qa("Self-blame (Q13, Q26)", self.self_blame())}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Individual items are scored 0–3 (as in Carver 1997 PMID
                16250744), not 1–4 (as in
                http://www.psy.miami.edu/faculty/ccarver/sclBrCOPE.html).
                Summaries, which are all
                based on two items, are therefore scored 0–6.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {q_a}
            </table>
        """
