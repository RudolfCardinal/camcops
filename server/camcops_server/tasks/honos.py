#!/usr/bin/env python

"""
camcops_server/tasks/honos.py

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
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    subheading_spanning_two_columns,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    CharColType,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


PV_MAIN = [0, 1, 2, 3, 4, 9]
PV_PROBLEMTYPE = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J']

FOOTNOTE_SCORING = """
    [1] 0 = no problem;
    1 = minor problem requiring no action;
    2 = mild problem but definitely present;
    3 = moderately severe problem;
    4 = severe to very severe problem;
    9 = not known.
"""


# =============================================================================
# HoNOS abstract base class
# =============================================================================

# noinspection PyAbstractClass
class HonosBase(TaskHasPatientMixin, TaskHasClinicianMixin, Task):
    __abstract__ = True
    provides_trackers = True

    period_rated = Column(
        "period_rated", UnicodeText,
        comment="Period being rated"
    )

    COPYRIGHT_DIV = f"""
        <div class="{CssClass.COPYRIGHT}">
            Health of the Nation Outcome Scales:
            Copyright © Royal College of Psychiatrists.
            Used here with permission.
        </div>
    """

    QFIELDS = None  # type: List[str]  # must be overridden
    MAX_SCORE = None  # type: int  # must be overridden

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label=f"{self.shortname} total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"{self.shortname} total score "
            f"{self.total_score()}/{self.MAX_SCORE}"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{self.MAX_SCORE})"),
        ]

    def _total_score_for_fields(self, fieldnames: List[str]) -> int:
        total = 0
        for qname in fieldnames:
            value = getattr(self, qname)
            if value is not None and 0 <= value <= 4:
                # i.e. ignore null values and 9 (= not known)
                total += value
        return total

    def total_score(self) -> int:
        return self._total_score_for_fields(self.QFIELDS)

    def get_q(self, req: CamcopsRequest, q: int) -> str:
        return self.wxstring(req, "q" + str(q) + "_s")

    def get_answer(self, req: CamcopsRequest, q: int, a: int) -> Optional[str]:
        if a == 9:
            return self.wxstring(req, "option9")
        if a is None or a < 0 or a > 4:
            return None
        return self.wxstring(req, "q" + str(q) + "_option" + str(a))


# =============================================================================
# HoNOS
# =============================================================================

class HonosMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Honos'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            pv=PV_MAIN,
            comment_fmt="Q{n}, {s} (0-4, higher worse)",
            comment_strings=[
                "overactive/aggressive/disruptive/agitated",
                "deliberate self-harm",
                "problem-drinking/drug-taking",
                "cognitive problems",
                "physical illness/disability",
                "hallucinations/delusions",
                "depressed mood",
                "other mental/behavioural problem",
                "relationship problems",
                "activities of daily living",
                "problems with living conditions",
                "occupation/activities",
            ]
        )
        super().__init__(name, bases, classdict)


class Honos(HonosBase,
            metaclass=HonosMetaclass):
    """
    Server implementation of the HoNOS task.
    """
    __tablename__ = "honos"
    shortname = "HoNOS"

    q8problemtype = CamcopsColumn(
        "q8problemtype", CharColType,
        permitted_value_checker=PermittedValueChecker(
            permitted_values=PV_PROBLEMTYPE),
        comment="Q8: type of problem (A phobic; B anxiety; "
                "C obsessive-compulsive; D mental strain/tension; "
                "E dissociative; F somatoform; G eating; H sleep; "
                "I sexual; J other, specify)"
    )
    q8otherproblem = Column(
        "q8otherproblem", UnicodeText,
        comment="Q8: other problem: specify"
    )

    NQUESTIONS = 12
    QFIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 48

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Health of the Nation Outcome Scales, working age adults")

    # noinspection PyUnresolvedReferences
    def is_complete(self) -> bool:
        if self.any_fields_none(self.QFIELDS):
            return False
        if not self.field_contents_valid():
            return False
        if self.q8 != 0 and self.q8 != 9 and self.q8problemtype is None:
            return False
        if self.q8 != 0 and self.q8 != 9 and self.q8problemtype == "J" \
                and self.q8otherproblem is None:
            return False
        return self.period_rated is not None

    def get_task_html(self, req: CamcopsRequest) -> str:
        q8_problem_type_dict = {
            None: None,
            "A": self.wxstring(req, "q8problemtype_option_a"),
            "B": self.wxstring(req, "q8problemtype_option_b"),
            "C": self.wxstring(req, "q8problemtype_option_c"),
            "D": self.wxstring(req, "q8problemtype_option_d"),
            "E": self.wxstring(req, "q8problemtype_option_e"),
            "F": self.wxstring(req, "q8problemtype_option_f"),
            "G": self.wxstring(req, "q8problemtype_option_g"),
            "H": self.wxstring(req, "q8problemtype_option_h"),
            "I": self.wxstring(req, "q8problemtype_option_i"),
            "J": self.wxstring(req, "q8problemtype_option_j"),
        }
        one_to_eight = ""
        for i in range(1, 8 + 1):
            one_to_eight += tr_qa(
                self.get_q(req, i),
                self.get_answer(req, i, getattr(self, "q" + str(i)))
            )
        nine_onwards = ""
        for i in range(9, self.NQUESTIONS + 1):
            nine_onwards += tr_qa(
                self.get_q(req, i),
                self.get_answer(req, i, getattr(self, "q" + str(i)))
            )

        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer <sup>[1]</sup></th>
                </tr>
                {period_rated}
                {one_to_eight}
                {q8problemtype}
                {q8otherproblem}
                {nine_onwards}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                {FOOTNOTE_SCORING}
            </div>
            {copyright_div}
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(self.total_score()) + f" / {self.MAX_SCORE}"
            ),
            period_rated=tr_qa(self.wxstring(req, "period_rated"),
                               self.period_rated),
            one_to_eight=one_to_eight,
            q8problemtype=tr_qa(
                self.wxstring(req, "q8problemtype_s"),
                get_from_dict(q8_problem_type_dict, self.q8problemtype)
            ),
            q8otherproblem=tr_qa(
                self.wxstring(req, "q8otherproblem_s"),
                self.q8otherproblem
            ),
            nine_onwards=nine_onwards,
            FOOTNOTE_SCORING=FOOTNOTE_SCORING,
            copyright_div=self.COPYRIGHT_DIV,
        )
        return h

    # noinspection PyUnresolvedReferences
    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.HONOSWA_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.HONOSWA_SCALE),
                {
                    req.snomed(SnomedLookup.HONOSWA_SCORE): self.total_score(),
                    req.snomed(SnomedLookup.HONOSWA_1_OVERACTIVE_SCORE): self.q1,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_2_SELFINJURY_SCORE): self.q2,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_3_SUBSTANCE_SCORE): self.q3,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_4_COGNITIVE_SCORE): self.q4,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_5_PHYSICAL_SCORE): self.q5,
                    req.snomed(SnomedLookup.HONOSWA_6_PSYCHOSIS_SCORE): self.q6,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_7_DEPRESSION_SCORE): self.q7,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_8_OTHERMENTAL_SCORE): self.q8,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_9_RELATIONSHIPS_SCORE): self.q9,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_10_ADL_SCORE): self.q10,
                    req.snomed(SnomedLookup.HONOSWA_11_LIVINGCONDITIONS_SCORE): self.q11,  # noqa
                    req.snomed(SnomedLookup.HONOSWA_12_OCCUPATION_SCORE): self.q12,  # noqa
                }
            ))
        return codes


# =============================================================================
# HoNOS 65+
# =============================================================================

class Honos65Metaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Honos65'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            pv=PV_MAIN,
            comment_fmt="Q{n}, {s} (0-4, higher worse)",
            comment_strings=[  # not exactly identical to HoNOS
                "behavioural disturbance",
                "deliberate self-harm",
                "problem drinking/drug-taking",
                "cognitive problems",
                "physical illness/disability",
                "hallucinations/delusions",
                "depressive symptoms",
                "other mental/behavioural problem",
                "relationship problems",
                "activities of daily living",
                "living conditions",
                "occupation/activities",
            ]
        )
        super().__init__(name, bases, classdict)


class Honos65(HonosBase,
              metaclass=Honos65Metaclass):
    """
    Server implementation of the HoNOS 65+ task.
    """
    __tablename__ = "honos65"
    shortname = "HoNOS 65+"

    q8problemtype = CamcopsColumn(
        "q8problemtype", CharColType,
        permitted_value_checker=PermittedValueChecker(
            permitted_values=PV_PROBLEMTYPE),
        comment="Q8: type of problem (A phobic; B anxiety; "
                "C obsessive-compulsive; D stress; "  # NB slight difference: D
                "E dissociative; F somatoform; G eating; H sleep; "
                "I sexual; J other, specify)"
    )
    q8otherproblem = Column(
        "q8otherproblem", UnicodeText,
        comment="Q8: other problem: specify"
    )

    NQUESTIONS = 12
    QFIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 48

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Health of the Nation Outcome Scales, older adults")

    # noinspection PyUnresolvedReferences
    def is_complete(self) -> bool:
        if self.any_fields_none(self.QFIELDS):
            return False
        if not self.field_contents_valid():
            return False
        if self.q8 != 0 and self.q8 != 9 and self.q8problemtype is None:
            return False
        if self.q8 != 0 and self.q8 != 9 and self.q8problemtype == "J" \
                and self.q8otherproblem is None:
            return False
        return self.period_rated is not None

    def get_task_html(self, req: CamcopsRequest) -> str:
        q8_problem_type_dict = {
            None: None,
            "A": self.wxstring(req, "q8problemtype_option_a"),
            "B": self.wxstring(req, "q8problemtype_option_b"),
            "C": self.wxstring(req, "q8problemtype_option_c"),
            "D": self.wxstring(req, "q8problemtype_option_d"),
            "E": self.wxstring(req, "q8problemtype_option_e"),
            "F": self.wxstring(req, "q8problemtype_option_f"),
            "G": self.wxstring(req, "q8problemtype_option_g"),
            "H": self.wxstring(req, "q8problemtype_option_h"),
            "I": self.wxstring(req, "q8problemtype_option_i"),
            "J": self.wxstring(req, "q8problemtype_option_j"),
        }
        one_to_eight = ""
        for i in range(1, 8 + 1):
            one_to_eight += tr_qa(
                self.get_q(req, i),
                self.get_answer(req, i, getattr(self, "q" + str(i)))
            )
        nine_onwards = ""
        for i in range(9, Honos.NQUESTIONS + 1):
            nine_onwards += tr_qa(
                self.get_q(req, i),
                self.get_answer(req, i, getattr(self, "q" + str(i)))
            )

        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer <sup>[1]</sup></th>
                </tr>
                {period_rated}
                {one_to_eight}
                {q8problemtype}
                {q8otherproblem}
                {nine_onwards}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                {FOOTNOTE_SCORING}
            </div>
            {copyright_div}
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(self.total_score()) + f" / {self.MAX_SCORE}"
            ),
            period_rated=tr_qa(self.wxstring(req, "period_rated"),
                               self.period_rated),
            one_to_eight=one_to_eight,
            q8problemtype=tr_qa(
                self.wxstring(req, "q8problemtype_s"),
                get_from_dict(q8_problem_type_dict, self.q8problemtype)
            ),
            q8otherproblem=tr_qa(
                self.wxstring(req, "q8otherproblem_s"),
                self.q8otherproblem
            ),
            nine_onwards=nine_onwards,
            FOOTNOTE_SCORING=FOOTNOTE_SCORING,
            copyright_div=self.COPYRIGHT_DIV,
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.HONOS65_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.HONOS65_SCALE),
                {
                    req.snomed(SnomedLookup.HONOS65_SCORE): self.total_score(),
                }
            ))
        return codes


# =============================================================================
# HoNOSCA
# =============================================================================

class HonoscaMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Honosca'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            pv=PV_MAIN,
            comment_fmt="Q{n}, {s} (0-4, higher worse)",
            comment_strings=[
                "disruptive/antisocial/aggressive",
                "overactive/inattentive",
                "self-harm",
                "alcohol/drug misuse",
                "scholastic/language problems",
                "physical illness/disability",
                "delusions/hallucinations",
                "non-organic somatic symptoms",
                "emotional symptoms",
                "peer relationships",
                "self-care and independence",
                "family life/relationships",
                "school attendance",
                "problems with knowledge/understanding of child's problems",
                "lack of information about services",
            ]
        )
        super().__init__(name, bases, classdict)


class Honosca(HonosBase,
              metaclass=HonoscaMetaclass):
    """
    Server implementation of the HoNOSCA task.
    """
    __tablename__ = "honosca"
    shortname = "HoNOSCA"

    NQUESTIONS = 15
    QFIELDS = strseq("q", 1, NQUESTIONS)
    LAST_SECTION_A_Q = 13
    FIRST_SECTION_B_Q = 14
    SECTION_A_QFIELDS = strseq("q", 1, LAST_SECTION_A_Q)
    SECTION_B_QFIELDS = strseq("q", FIRST_SECTION_B_Q, NQUESTIONS)
    MAX_SCORE = 60
    MAX_SECTION_A = 4 * len(SECTION_A_QFIELDS)
    MAX_SECTION_B = 4 * len(SECTION_B_QFIELDS)
    TASK_FIELDS = QFIELDS + ["period_rated"]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "Health of the Nation Outcome Scales, Children and Adolescents")

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def section_a_score(self) -> int:
        return self._total_score_for_fields(self.SECTION_A_QFIELDS)

    def section_b_score(self) -> int:
        return self._total_score_for_fields(self.SECTION_B_QFIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        section_a = ""
        for i in range(1, 13 + 1):
            section_a += tr_qa(
                self.get_q(req, i),
                self.get_answer(req, i, getattr(self, "q" + str(i)))
            )
        section_b = ""
        for i in range(14, self.NQUESTIONS + 1):
            section_b += tr_qa(
                self.get_q(req, i),
                self.get_answer(req, i, getattr(self, "q" + str(i)))
            )

        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {section_a_total}
                    {section_b_total}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer <sup>[1]</sup></th>
                </tr>
                {period_rated}
                {section_a_subhead}
                {section_a}
                {section_b_subhead}
                {section_b}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                {FOOTNOTE_SCORING}
            </div>
            {copyright_div}
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(self.total_score()) + f" / {self.MAX_SCORE}"
            ),
            section_a_total=tr(
                self.wxstring(req, "section_a_total"),
                answer(self.section_a_score()) +
                f" / {self.MAX_SECTION_A}"
            ),
            section_b_total=tr(
                self.wxstring(req, "section_b_total"),
                answer(self.section_b_score()) +
                f" / {self.MAX_SECTION_B}"
            ),
            period_rated=tr_qa(self.wxstring(req, "period_rated"),
                               self.period_rated),
            section_a_subhead=subheading_spanning_two_columns(
                self.wxstring(req, "section_a_title")),
            section_a=section_a,
            section_b_subhead=subheading_spanning_two_columns(
                self.wxstring(req, "section_b_title")),
            section_b=section_b,
            FOOTNOTE_SCORING=FOOTNOTE_SCORING,
            copyright_div=self.COPYRIGHT_DIV,
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.HONOSCA_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            a = self.section_a_score()
            b = self.section_b_score()
            total = a + b
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.HONOSCA_SCALE),
                {
                    req.snomed(SnomedLookup.HONOSCA_SCORE): total,
                    req.snomed(SnomedLookup.HONOSCA_SECTION_A_SCORE): a,
                    req.snomed(SnomedLookup.HONOSCA_SECTION_B_SCORE): b,
                    req.snomed(SnomedLookup.HONOSCA_SECTION_A_PLUS_B_SCORE): total,  # noqa
                }
            ))
        return codes
