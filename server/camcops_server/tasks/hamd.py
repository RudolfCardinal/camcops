#!/usr/bin/env python

"""
camcops_server/tasks/hamd.py

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
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    SummaryCategoryColType,
    ZERO_TO_ONE_CHECKER,
    ZERO_TO_TWO_CHECKER,
    ZERO_TO_THREE_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# HAM-D
# =============================================================================

MAX_SCORE = (
    4 * 15 -  # Q1-15 scored 0-5
    (2 * 6) +  # except Q4-6, 12-14 scored 0-2
    2 * 2  # Q16-17
)  # ... and not scored beyond Q17... total 52


class HamdMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Hamd'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, 15,
            comment_fmt="Q{n}, {s} (scored 0-4, except 0-2 for "
                        "Q4-6/12-14, higher worse)",
            minimum=0, maximum=4,  # amended below
            comment_strings=[
                "depressed mood", "guilt", "suicide", "early insomnia",
                "middle insomnia", "late insomnia", "work/activities",
                "psychomotor retardation", "agitation",
                "anxiety, psychological", "anxiety, somatic",
                "somatic symptoms, gastointestinal",
                "somatic symptoms, general", "genital symptoms",
                "hypochondriasis"
            ]
        )
        add_multiple_columns(
            cls, "q", 19, 21,
            comment_fmt="Q{n} (not scored), {s} (0-4 for Q19, "
                        "0-3 for Q20, 0-2 for Q21, higher worse)",
            minimum=0, maximum=4,  # below
            comment_strings=["depersonalization/derealization",
                             "paranoid symptoms",
                             "obsessional/compulsive symptoms"]
        )
        # Now fix the wrong bits. Hardly elegant!
        for qnum in [4, 5, 6, 12, 13, 14, 21]:
            qname = "q" + str(qnum)
            col = getattr(cls, qname)
            col.set_permitted_value_checker(ZERO_TO_TWO_CHECKER)
        # noinspection PyUnresolvedReferences
        cls.q20.set_permitted_value_checker(ZERO_TO_THREE_CHECKER)

        super().__init__(name, bases, classdict)


class Hamd(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
           metaclass=HamdMetaclass):
    """
    Server implementation of the HAM-D task.
    """
    __tablename__ = "hamd"
    shortname = "HAM-D"
    provides_trackers = True

    NSCOREDQUESTIONS = 17
    NQUESTIONS = 21
    TASK_FIELDS = strseq("q", 1, NQUESTIONS) + [
        "whichq16", "q16a", "q16b", "q17", "q18a", "q18b"
    ]

    whichq16 = CamcopsColumn(
        "whichq16", Integer,
        permitted_value_checker=ZERO_TO_ONE_CHECKER,
        comment="Method of assessing weight loss (0 = A, by history; "
                "1 = B, by measured change)"
    )
    q16a = CamcopsColumn(
        "q16a", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment="Q16A, weight loss, by history (0 none - 2 definite,"
                " or 3 not assessed [not scored])"
    )
    q16b = CamcopsColumn(
        "q16b", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment="Q16B, weight loss, by measurement (0 none - "
                "2 more than 2lb, or 3 not assessed [not scored])"
    )
    q17 = CamcopsColumn(
        "q17", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q17, lack of insight (0-2, higher worse)"
    )
    q18a = CamcopsColumn(
        "q18a", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q18A (not scored), diurnal variation, presence "
                "(0 none, 1 worse AM, 2 worse PM)"
    )
    q18b = CamcopsColumn(
        "q18b", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q18B (not scored), diurnal variation, severity "
                "(0-2, higher more severe)"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Hamilton Rating Scale for Depression")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="HAM-D total score",
            axis_label=f"Total score (out of {MAX_SCORE})",
            axis_min=-0.5,
            axis_max=MAX_SCORE + 0.5,
            horizontal_lines=[22.5, 19.5, 14.5, 7.5],
            horizontal_labels=[
                TrackerLabel(25, self.wxstring(req, "severity_verysevere")),
                TrackerLabel(21, self.wxstring(req, "severity_severe")),
                TrackerLabel(17, self.wxstring(req, "severity_moderate")),
                TrackerLabel(11, self.wxstring(req, "severity_mild")),
                TrackerLabel(3.75, self.wxstring(req, "severity_none")),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"HAM-D total score {self.total_score()}/{MAX_SCORE} "
            f"({self.severity(req)})"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{MAX_SCORE})"),
            SummaryElement(name="severity",
                           coltype=SummaryCategoryColType,
                           value=self.severity(req),
                           comment="Severity"),
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
        if self.q2 is not None and self.q3 is not None \
                and (self.q2 + self.q3 == 0):
            # Special limited-information completeness
            return True
        # Otherwise, any null values cause problems
        if self.whichq16 is None:
            return False
        for i in range(1, self.NSCOREDQUESTIONS + 1):
            if i == 16:
                if (self.whichq16 == 0 and self.q16a is None) \
                        or (self.whichq16 == 1 and self.q16b is None):
                    return False
            else:
                if getattr(self, "q" + str(i)) is None:
                    return False
        return True

    def total_score(self) -> int:
        total = 0
        for i in range(1, self.NSCOREDQUESTIONS + 1):
            if i == 16:
                relevant_field = "q16a" if self.whichq16 == 0 else "q16b"
                score = self.sum_fields([relevant_field])
                if score != 3:  # ... a value that's ignored
                    total += score
            else:
                total += self.sum_fields(["q" + str(i)])
        return total

    def severity(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        if score >= 23:
            return self.wxstring(req, "severity_verysevere")
        elif score >= 19:
            return self.wxstring(req, "severity_severe")
        elif score >= 14:
            return self.wxstring(req, "severity_moderate")
        elif score >= 8:
            return self.wxstring(req, "severity_mild")
        else:
            return self.wxstring(req, "severity_none")

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        severity = self.severity(req)
        task_field_list_for_display = (
            strseq("q", 1, 15) +
            [
                "whichq16",
                "q16a" if self.whichq16 == 0 else "q16b",  # funny one
                "q17",
                "q18a",
                "q18b"
            ] +
            strseq("q", 19, 21)
        )
        answer_dicts_dict = {}
        for q in task_field_list_for_display:
            d = {None: None}
            for option in range(0, 5):
                if (q == "q4" or q == "q5" or q == "q6" or q == "q12" or
                        q == "q13" or q == "q14" or q == "q17" or
                        q == "q18" or q == "q21") and option > 2:
                    continue
                d[option] = self.wxstring(req, "" + q + "_option" + str(option))
            answer_dicts_dict[q] = d
        q_a = ""
        for q in task_field_list_for_display:
            if q == "whichq16":
                qstr = self.wxstring(req, "whichq16_title")
            else:
                if q == "q16a" or q == "q16b":
                    rangestr = " <sup>range 0–2; ‘3’ not scored</sup>"
                else:
                    col = getattr(self.__class__, q)  # type: CamcopsColumn
                    rangestr = " <sup>range {}–{}</sup>".format(
                        col.permitted_value_checker.minimum,
                        col.permitted_value_checker.maximum
                    )
                qstr = self.wxstring(req, "" + q + "_s") + rangestr
            q_a += tr_qa(qstr, get_from_dict(answer_dicts_dict[q],
                                             getattr(self, q)))
        return """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {severity}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="40%">Question</th>
                    <th width="60%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Only Q1–Q17 scored towards the total.
                    Re Q16: values of ‘3’ (‘not assessed’) are not actively
                    scored, after e.g. Guy W (1976) <i>ECDEU Assessment Manual
                    for Psychopharmacology, revised</i>, pp. 180–192, esp.
                    pp. 187, 189
                    (https://archive.org/stream/ecdeuassessmentm1933guyw).
                [2] ≥23 very severe, ≥19 severe, ≥14 moderate,
                    ≥8 mild, &lt;8 none.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE) + " <sup>[1]</sup>",
                answer(score) + " / {}".format(MAX_SCORE)
            ),
            severity=tr_qa(
                self.wxstring(req, "severity") + " <sup>[2]</sup>",
                severity
            ),
            q_a=q_a,
        )

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.HAMD_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.HAMD_SCALE),
                {
                    req.snomed(SnomedLookup.HAMD_SCORE): self.total_score(),
                }
            ))
        return codes
