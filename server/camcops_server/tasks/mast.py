#!/usr/bin/env python

"""
camcops_server/tasks/mast.py

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
from sqlalchemy.sql.sqltypes import Boolean, Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, get_yes_no, tr, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import CharColType
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    LabelAlignment,
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# MAST
# =============================================================================

class MastMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Mast'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS, CharColType,
            pv=['Y', 'N'],
            comment_fmt="Q{n}: {s} (Y or N)",
            comment_strings=[
                "feel you are a normal drinker",
                "couldn't remember evening before",
                "relative worries/complains",
                "stop drinking after 1-2 drinks",
                "feel guilty",
                "friends/relatives think you are a normal drinker",
                "can stop drinking when you want",
                "attended Alcoholics Anonymous",
                "physical fights",
                "drinking caused problems with relatives",
                "family have sought help",
                "lost friends",
                "trouble at work/school",
                "lost job",
                "neglected obligations for >=2 days",
                "drink before noon often",
                "liver trouble",
                "delirium tremens",
                "sought help",
                "hospitalized",
                "psychiatry admission",
                "clinic visit or professional help",
                "arrested for drink-driving",
                "arrested for other drunk behaviour",
            ]
        )
        super().__init__(name, bases, classdict)


class Mast(TaskHasPatientMixin, Task,
           metaclass=MastMetaclass):
    """
    Server implementation of the MAST task.
    """
    __tablename__ = "mast"
    shortname = "MAST"
    provides_trackers = True

    NQUESTIONS = 24
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_SCORE = 53
    ROSS_THRESHOLD = 13

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Michigan Alcohol Screening Test")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="MAST total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
            horizontal_lines=[self.ROSS_THRESHOLD - 0.5],
            horizontal_labels=[
                TrackerLabel(self.ROSS_THRESHOLD,
                             "Ross threshold", LabelAlignment.bottom)
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"MAST total score {self.total_score()}/{self.MAX_SCORE}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/{self.MAX_SCORE})"),
            SummaryElement(
                name="exceeds_threshold",
                coltype=Boolean(),
                value=self.exceeds_ross_threshold(),
                comment=f"Exceeds Ross threshold "
                        f"(total score >= {self.ROSS_THRESHOLD})"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.TASK_FIELDS) and
            self.field_contents_valid()
        )

    def get_score(self, q: int) -> int:
        yes = "Y"
        value = getattr(self, "q" + str(q))
        if value is None:
            return 0
        if q == 1 or q == 4 or q == 6 or q == 7:
            presence = 0 if value == yes else 1
        else:
            presence = 1 if value == yes else 0
        if q == 3 or q == 5 or q == 9 or q == 16:
            points = 1
        elif q == 8 or q == 19 or q == 20:
            points = 5
        else:
            points = 2
        return points * presence

    def total_score(self) -> int:
        total = 0
        for q in range(1, self.NQUESTIONS + 1):
            total += self.get_score(q)
        return total

    def exceeds_ross_threshold(self) -> bool:
        score = self.total_score()
        return score >= self.ROSS_THRESHOLD

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        exceeds_threshold = self.exceeds_ross_threshold()
        main_dict = {
            None: None,
            "Y": req.sstring(SS.YES),
            "N": req.sstring(SS.NO)
        }
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += tr(
                self.wxstring(req, "q" + str(q)),
                (
                    answer(get_from_dict(main_dict,
                                         getattr(self, "q" + str(q)))) +
                    answer(" — " + str(self.get_score(q)))
                )
            )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr(req.sstring(SS.TOTAL_SCORE),
                        answer(score) + " / {}".format(self.MAX_SCORE))}
                    {tr_qa(self.wxstring(req, "exceeds_threshold"),
                           get_yes_no(req, exceeds_threshold))}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="80%">Question</th>
                    <th width="20%">Answer</th>
                </tr>
                {q_a}
            </table>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.MAST_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.MAST_SCALE),
                {
                    req.snomed(SnomedLookup.MAST_SCORE): self.total_score(),
                }
            ))
        return codes
