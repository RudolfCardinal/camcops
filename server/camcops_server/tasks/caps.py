#!/usr/bin/env python

"""
camcops_server/tasks/caps.py

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

from camcops_server.cc_modules.cc_constants import CssClass, PV
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no_none,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# CAPS
# =============================================================================

QUESTION_SNIPPETS = [
    "sounds loud", "presence of another", "heard thoughts echoed",
    "see shapes/lights/colours", "burning or other bodily sensations",
    "hear noises/sounds", "thoughts spoken aloud", "unexplained smells",
    "body changing shape", "limbs not own", "voices commenting",
    "feeling a touch", "hearing words or sentences", "unexplained tastes",
    "sensations flooding", "sounds distorted",
    "hard to distinguish sensations", "odours strong",
    "shapes/people distorted", "hypersensitive to touch/temperature",
    "tastes stronger than normal", "face looks different",
    "lights/colours more intense", "feeling of being uplifted",
    "common smells seem different", "everyday things look abnormal",
    "altered perception of time", "hear voices conversing",
    "smells or odours that others are unaware of",
    "food/drink tastes unusual", "see things that others cannot",
    "hear sounds/music that others cannot"
]


class CapsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Caps'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "endorse", 1, cls.NQUESTIONS,
            pv=PV.BIT,
            comment_fmt="Q{n} ({s}): endorsed? (0 no, 1 yes)",
            comment_strings=QUESTION_SNIPPETS
        )
        add_multiple_columns(
            cls, "distress", 1, cls.NQUESTIONS,
            minimum=1, maximum=5,
            comment_fmt="Q{n} ({s}): distress (1 low - 5 high), if endorsed",
            comment_strings=QUESTION_SNIPPETS
        )
        add_multiple_columns(
            cls, "intrusiveness", 1, cls.NQUESTIONS,
            minimum=1, maximum=5,
            comment_fmt="Q{n} ({s}): intrusiveness (1 low - 5 high), "
                        "if endorsed",
            comment_strings=QUESTION_SNIPPETS
        )
        add_multiple_columns(
            cls, "frequency", 1, cls.NQUESTIONS,
            minimum=1, maximum=5,
            comment_fmt="Q{n} ({s}): frequency (1 low - 5 high), if endorsed",
            comment_strings=QUESTION_SNIPPETS
        )
        super().__init__(name, bases, classdict)


class Caps(TaskHasPatientMixin, Task,
           metaclass=CapsMetaclass):
    """
    Server implementation of the CAPS task.
    """
    __tablename__ = "caps"
    shortname = "CAPS"
    provides_trackers = True

    prohibits_commercial = True

    NQUESTIONS = 32
    ENDORSE_FIELDS = strseq("endorse", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Cardiff Anomalous Perceptions Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="CAPS total score",
            axis_label="Total score (out of 32)",
            axis_min=-0.5,
            axis_max=32.5
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment="Total score (/32)"),
            SummaryElement(
                name="distress", coltype=Integer(),
                value=self.distress_score(),
                comment="Distress score (/160)"),
            SummaryElement(
                name="intrusiveness", coltype=Integer(),
                value=self.intrusiveness_score(),
                comment="Intrusiveness score (/160)"),
            SummaryElement(
                name="frequency", coltype=Integer(),
                value=self.frequency_score(),
                comment="Frequency score (/160)"),
        ]

    def is_question_complete(self, q: int) -> bool:
        if getattr(self, "endorse" + str(q)) is None:
            return False
        if getattr(self, "endorse" + str(q)):
            if getattr(self, "distress" + str(q)) is None:
                return False
            if getattr(self, "intrusiveness" + str(q)) is None:
                return False
            if getattr(self, "frequency" + str(q)) is None:
                return False
        return True

    def is_complete(self) -> bool:
        if not self.field_contents_valid():
            return False
        for i in range(1, Caps.NQUESTIONS + 1):
            if not self.is_question_complete(i):
                return False
        return True

    def total_score(self) -> int:
        return self.count_booleans(self.ENDORSE_FIELDS)

    def distress_score(self) -> int:
        score = 0
        for q in range(1, Caps.NQUESTIONS + 1):
            if getattr(self, "endorse" + str(q)) \
                    and getattr(self, "distress" + str(q)) is not None:
                score += self.sum_fields(["distress" + str(q)])
        return score

    def intrusiveness_score(self) -> int:
        score = 0
        for q in range(1, Caps.NQUESTIONS + 1):
            if getattr(self, "endorse" + str(q)) \
                    and getattr(self, "intrusiveness" + str(q)) is not None:
                score += self.sum_fields(["intrusiveness" + str(q)])
        return score

    def frequency_score(self) -> int:
        score = 0
        for q in range(1, Caps.NQUESTIONS + 1):
            if getattr(self, "endorse" + str(q)) \
                    and getattr(self, "frequency" + str(q)) is not None:
                score += self.sum_fields(["frequency" + str(q)])
        return score

    def get_task_html(self, req: CamcopsRequest) -> str:
        total = self.total_score()
        distress = self.distress_score()
        intrusiveness = self.intrusiveness_score()
        frequency = self.frequency_score()

        q_a = ""
        for q in range(1, Caps.NQUESTIONS + 1):
            q_a += tr(
                self.wxstring(req, "q" + str(q)),
                answer(get_yes_no_none(req,
                                       getattr(self, "endorse" + str(q)))),
                answer(getattr(self, "distress" + str(q))
                       if getattr(self, "endorse" + str(q)) else ""),
                answer(getattr(self, "intrusiveness" + str(q))
                       if getattr(self, "endorse" + str(q)) else ""),
                answer(getattr(self, "frequency" + str(q))
                       if getattr(self, "endorse" + str(q)) else "")
            )

        tr_total_score = tr_qa(
            f"{req.sstring(SS.TOTAL_SCORE)} <sup>[1]</sup> (0–32)",
            total
        )
        tr_distress = tr_qa(
            "{} (0–160)".format(self.wxstring(req, "distress")),
            distress
        ),
        tr_intrusiveness = tr_qa(
            "{} (0–160)".format(self.wxstring(req, "intrusiveness")),
            intrusiveness
        ),
        tr_frequency = tr_qa(
            "{} (0–160)".format(self.wxstring(req, "frequency")),
            frequency
        )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_total_score}
                    {tr_distress}
                    {tr_intrusiveness}
                    {tr_frequency}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Anchor points:
                DISTRESS
                    {self.wxstring(req, "distress_option1")},
                    {self.wxstring(req, "distress_option5")}.
                INTRUSIVENESS
                    {self.wxstring(req, "intrusiveness_option1")},
                    {self.wxstring(req, "intrusiveness_option5")}.
                FREQUENCY
                    {self.wxstring(req, "frequency_option1")},
                    {self.wxstring(req, "frequency_option5")}.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="10%">Endorsed?</th>
                    <th width="10%">Distress (1–5)</th>
                    <th width="10%">Intrusiveness (1–5)</th>
                    <th width="10%">Frequency (1–5)</th>
                </tr>
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Total score: sum of endorsements (yes = 1, no = 0).
                Dimension scores: sum of ratings (0 if not endorsed).
                (Bell et al. 2006, PubMed ID 16237200)
            </div>
            <div class="{CssClass.COPYRIGHT}">
                CAPS: Copyright © 2005, Bell, Halligan & Ellis.
                Original article:
                    Bell V, Halligan PW, Ellis HD (2006).
                    The Cardiff Anomalous Perceptions Scale (CAPS): a new
                    validated measure of anomalous perceptual experience.
                    Schizophrenia Bulletin 32: 366–377.
                Published by Oxford University Press on behalf of the Maryland
                Psychiatric Research Center. All rights reserved. The online
                version of this article has been published under an open access
                model. Users are entitled to use, reproduce, disseminate, or
                display the open access version of this article for
                non-commercial purposes provided that: the original authorship
                is properly and fully attributed; the Journal and Oxford
                University Press are attributed as the original place of
                publication with the correct citation details given; if an
                article is subsequently reproduced or disseminated not in its
                entirety but only in part or as a derivative work this must be
                clearly indicated. For commercial re-use, please contact
                journals.permissions@oxfordjournals.org.<br>
                <b>This is a derivative work (partial reproduction, viz. the
                scale text).</b>
            </div>
        """
