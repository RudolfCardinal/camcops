#!/usr/bin/env python

"""
camcops_server/tasks/moca.py

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
from sqlalchemy.sql.sqltypes import Integer, String, UnicodeText

from camcops_server.cc_modules.cc_blob import (
    Blob,
    blob_relationship,
    get_blob_img_html,
)
from camcops_server.cc_modules.cc_constants import CssClass, PV
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    italic,
    subheading_spanning_two_columns,
    td,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    ZERO_TO_THREE_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import (
    LabelAlignment,
    TrackerInfo,
    TrackerLabel,
)


WORDLIST = ["FACE", "VELVET", "CHURCH", "DAISY", "RED"]


# =============================================================================
# MoCA
# =============================================================================

class MocaMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Moca'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=1,  # see below
            comment_fmt="{s}",
            comment_strings=[
                "Q1 (VSE/path) (0-1)",
                "Q2 (VSE/cube) (0-1)",
                "Q3 (VSE/clock/contour) (0-1)",
                "Q4 (VSE/clock/numbers) (0-1)",
                "Q5 (VSE/clock/hands) (0-1)",
                "Q6 (naming/lion) (0-1)",
                "Q7 (naming/rhino) (0-1)",
                "Q8 (naming/camel) (0-1)",
                "Q9 (attention/5 digits) (0-1)",
                "Q10 (attention/3 digits) (0-1)",
                "Q11 (attention/tapping) (0-1)",
                "Q12 (attention/serial 7s) (0-3)",  # different max
                "Q13 (language/sentence 1) (0-1)",
                "Q14 (language/sentence 2) (0-1)",
                "Q15 (language/fluency) (0-1)",
                "Q16 (abstraction 1) (0-1)",
                "Q17 (abstraction 2) (0-1)",
                "Q18 (recall word/face) (0-1)",
                "Q19 (recall word/velvet) (0-1)",
                "Q20 (recall word/church) (0-1)",
                "Q21 (recall word/daisy) (0-1)",
                "Q22 (recall word/red) (0-1)",
                "Q23 (orientation/date) (0-1)",
                "Q24 (orientation/month) (0-1)",
                "Q25 (orientation/year) (0-1)",
                "Q26 (orientation/day) (0-1)",
                "Q27 (orientation/place) (0-1)",
                "Q28 (orientation/city) (0-1)",
            ]
        )
        # Fix maximum for Q12:
        # noinspection PyUnresolvedReferences
        cls.q12.set_permitted_value_checker(ZERO_TO_THREE_CHECKER)

        add_multiple_columns(
            cls, "register_trial1_", 1, 5,
            pv=PV.BIT,
            comment_fmt="Registration, trial 1 (not scored), {n}: {s} "
                        "(0 or 1)",
            comment_strings=WORDLIST
        )
        add_multiple_columns(
            cls, "register_trial2_", 1, 5,
            pv=PV.BIT,
            comment_fmt="Registration, trial 2 (not scored), {n}: {s} "
                        "(0 or 1)",
            comment_strings=WORDLIST
        )
        add_multiple_columns(
            cls, "recall_category_cue_", 1, 5,
            pv=PV.BIT,
            comment_fmt="Recall with category cue (not scored), {n}: {s} "
                        "(0 or 1)",
            comment_strings=WORDLIST
        )
        add_multiple_columns(
            cls, "recall_mc_cue_", 1, 5,
            pv=PV.BIT,
            comment_fmt="Recall with multiple-choice cue (not scored), "
                        "{n}: {s} (0 or 1)",
            comment_strings=WORDLIST
        )
        super().__init__(name, bases, classdict)


class Moca(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
           metaclass=MocaMetaclass):
    """
    Server implementation of the MoCA task.
    """
    __tablename__ = "moca"
    shortname = "MoCA"
    provides_trackers = True

    prohibits_commercial = True
    prohibits_research = True

    education12y_or_less = CamcopsColumn(
        "education12y_or_less", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="<=12 years of education (0 no, 1 yes)"
    )
    trailpicture_blobid = CamcopsColumn(
        "trailpicture_blobid", Integer,
        is_blob_id_field=True, blob_relationship_attr_name="trailpicture",
        comment="BLOB ID of trail picture"
    )
    cubepicture_blobid = CamcopsColumn(
        "cubepicture_blobid", Integer,
        is_blob_id_field=True, blob_relationship_attr_name="cubepicture",
        comment="BLOB ID of cube picture"
    )
    clockpicture_blobid = CamcopsColumn(
        "clockpicture_blobid", Integer,
        is_blob_id_field=True, blob_relationship_attr_name="clockpicture",
        comment="BLOB ID of clock picture"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )

    trailpicture = blob_relationship("Moca", "trailpicture_blobid")  # type: Optional[Blob]  # noqa
    cubepicture = blob_relationship("Moca", "cubepicture_blobid")  # type: Optional[Blob]  # noqa
    clockpicture = blob_relationship("Moca", "clockpicture_blobid")  # type: Optional[Blob]  # noqa

    NQUESTIONS = 28
    MAX_SCORE = 30

    QFIELDS = strseq("q", 1, NQUESTIONS)
    VSP_FIELDS = strseq("q", 1, 5)
    NAMING_FIELDS = strseq("q", 6, 8)
    ATTN_FIELDS = strseq("q", 9, 12)
    LANG_FIELDS = strseq("q", 13, 15)
    ABSTRACTION_FIELDS = strseq("q", 16, 17)
    MEM_FIELDS = strseq("q", 18, 22)
    ORIENTATION_FIELDS = strseq("q", 23, 28)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Montreal Cognitive Assessment")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="MOCA total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=(self.MAX_SCORE + 0.5),
            horizontal_lines=[25.5],
            horizontal_labels=[
                TrackerLabel(26, req.sstring(SS.NORMAL),
                             LabelAlignment.bottom),
                TrackerLabel(25, req.sstring(SS.ABNORMAL),
                             LabelAlignment.top),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"MOCA total score {self.total_score()}/{self.MAX_SCORE}"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{self.MAX_SCORE})"),
            SummaryElement(name="category",
                           coltype=String(50),
                           value=self.category(req),
                           comment="Categorization"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.QFIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        score = self.sum_fields(self.QFIELDS)
        # Interpretation of the educational extra point: see moca.cpp; we have
        # a choice of allowing 31/30 or capping at 30. I think the instructions
        # imply a cap of 30.
        if score < self.MAX_SCORE:
            score += self.sum_fields(["education12y_or_less"])
            # extra point for this
        return score

    def score_vsp(self) -> int:
        return self.sum_fields(self.VSP_FIELDS)

    def score_naming(self) -> int:
        return self.sum_fields(self.NAMING_FIELDS)

    def score_attention(self) -> int:
        return self.sum_fields(self.ATTN_FIELDS)

    def score_language(self) -> int:
        return self.sum_fields(self.LANG_FIELDS)

    def score_abstraction(self) -> int:
        return self.sum_fields(self.ABSTRACTION_FIELDS)

    def score_memory(self) -> int:
        return self.sum_fields(self.MEM_FIELDS)

    def score_orientation(self) -> int:
        return self.sum_fields(self.ORIENTATION_FIELDS)

    def category(self, req: CamcopsRequest) -> str:
        totalscore = self.total_score()
        return (req.sstring(SS.NORMAL) if totalscore >= 26
                else req.sstring(SS.ABNORMAL))

    # noinspection PyUnresolvedReferences
    def get_task_html(self, req: CamcopsRequest) -> str:
        vsp = self.score_vsp()
        naming = self.score_naming()
        attention = self.score_attention()
        language = self.score_language()
        abstraction = self.score_abstraction()
        memory = self.score_memory()
        orientation = self.score_orientation()
        totalscore = self.total_score()
        category = self.category(req)

        h = """
            {clinician_comments}
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {total_score}
                    {category}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="69%">Question</th>
                    <th width="31%">Score</th>
                </tr>
        """.format(
            clinician_comments=self.get_standard_clinician_comments_block(
                req, self.comments),
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(totalscore) + f" / {self.MAX_SCORE}"
            ),
            category=tr_qa(self.wxstring(req, "category") + " <sup>[1]</sup>",
                           category),
        )

        h += tr(self.wxstring(req, "subscore_visuospatial"),
                answer(vsp) + " / 5",
                tr_class=CssClass.SUBHEADING)
        h += tr("Path, cube, clock/contour, clock/numbers, clock/hands",
                ", ".join([answer(x) for x in [self.q1, self.q2, self.q3,
                                               self.q4, self.q5]]))

        h += tr(self.wxstring(req, "subscore_naming"),
                answer(naming) + " / 3",
                tr_class=CssClass.SUBHEADING)
        h += tr("Lion, rhino, camel",
                ", ".join([answer(x) for x in [self.q6, self.q7, self.q8]]))

        h += tr(self.wxstring(req, "subscore_attention"),
                answer(attention) + " / 6",
                tr_class=CssClass.SUBHEADING)
        h += tr("5 digits forwards, 3 digits backwards, tapping, serial 7s "
                "[<i>scores 3</i>]",
                ", ".join([answer(x) for x in [self.q9, self.q10, self.q11,
                                               self.q12]]))

        h += tr(self.wxstring(req, "subscore_language"),
                answer(language) + " / 3",
                tr_class=CssClass.SUBHEADING)
        h += tr("Repeat sentence 1, repeat sentence 2, fluency to letter ‘F’",
                ", ".join([answer(x) for x in [self.q13, self.q14, self.q15]]))

        h += tr(self.wxstring(req, "subscore_abstraction"),
                answer(abstraction) + " / 2",
                tr_class=CssClass.SUBHEADING)
        h += tr("Means of transportation, measuring instruments",
                ", ".join([answer(x) for x in [self.q16, self.q17]]))

        h += tr(self.wxstring(req, "subscore_memory"),
                answer(memory) + " / 5",
                tr_class=CssClass.SUBHEADING)
        h += tr(
            "Registered on first trial [<i>not scored</i>]",
            ", ".join([
                answer(x, formatter_answer=italic)
                for x in [
                    self.register_trial1_1,
                    self.register_trial1_2,
                    self.register_trial1_3,
                    self.register_trial1_4,
                    self.register_trial1_5
                ]
            ])
        )
        h += tr(
            "Registered on second trial [<i>not scored</i>]",
            ", ".join([
                answer(x, formatter_answer=italic)
                for x in [
                    self.register_trial2_1,
                    self.register_trial2_2,
                    self.register_trial2_3,
                    self.register_trial2_4,
                    self.register_trial2_5
                ]
            ])
        )
        h += tr(
            "Recall FACE, VELVET, CHURCH, DAISY, RED with no cue",
            ", ".join([
                answer(x) for x in [
                    self.q18, self.q19, self.q20, self.q21, self.q22
                ]
            ])
        )
        h += tr(
            "Recall with category cue [<i>not scored</i>]",
            ", ".join([
                answer(x, formatter_answer=italic)
                for x in [
                    self.recall_category_cue_1,
                    self.recall_category_cue_2,
                    self.recall_category_cue_3,
                    self.recall_category_cue_4,
                    self.recall_category_cue_5
                ]
            ])
        )
        h += tr(
            "Recall with multiple-choice cue [<i>not scored</i>]",
            ", ".join([
                answer(x, formatter_answer=italic)
                for x in [
                    self.recall_mc_cue_1,
                    self.recall_mc_cue_2,
                    self.recall_mc_cue_3,
                    self.recall_mc_cue_4,
                    self.recall_mc_cue_5
                ]
            ])
        )

        h += tr(self.wxstring(req, "subscore_orientation"),
                answer(orientation) + " / 6",
                tr_class=CssClass.SUBHEADING)
        h += tr(
            "Date, month, year, day, place, city",
            ", ".join([
                answer(x) for x in [
                    self.q23, self.q24, self.q25, self.q26, self.q27, self.q28
                ]
            ])
        )

        h += subheading_spanning_two_columns(self.wxstring(req, "education_s"))
        h += tr_qa("≤12 years’ education?", self.education12y_or_less)
        # noinspection PyTypeChecker
        h += """
            </table>
            <table class="{CssClass.TASKDETAIL}">
                {tr_subhead_images}
                {tr_images_1}
                {tr_images_2}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Normal is ≥26 (Nasreddine et al. 2005, PubMed ID 15817019).
            </div>
            <div class="{CssClass.COPYRIGHT}">
                MoCA: Copyright © Ziad Nasreddine. In 2012, could be reproduced
                without permission for CLINICAL and EDUCATIONAL use (with
                permission from the copyright holder required for any other
                use), with no special restrictions on electronic versions.
                However, as of 2021, electronic versions are prohibited; see <a
                href="https://camcops.readthedocs.io/en/latest/tasks/moca.html">
                https://camcops.readthedocs.io/en/latest/tasks/moca.html</a>.
            </div>
        """.format(
            CssClass=CssClass,
            tr_subhead_images=subheading_spanning_two_columns(
                "Images of tests: trail, cube, clock",
                th_not_td=True),
            tr_images_1=tr(
                td(get_blob_img_html(self.trailpicture),
                   td_class=CssClass.PHOTO, td_width="50%"),
                td(get_blob_img_html(self.cubepicture),
                   td_class=CssClass.PHOTO, td_width="50%"),
                literal=True,
            ),
            tr_images_2=tr(
                td(get_blob_img_html(self.clockpicture),
                   td_class=CssClass.PHOTO, td_width="50%"),
                td("", td_class=CssClass.SUBHEADING),
                literal=True,
            ),
        )
        return h

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.MOCA_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.MOCA_SCALE),
                {
                    req.snomed(SnomedLookup.MOCA_SCORE): self.total_score(),
                }
            ))
        return codes
