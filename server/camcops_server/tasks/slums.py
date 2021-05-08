#!/usr/bin/env python

"""
camcops_server/tasks/slums.py

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

from typing import List, Optional

from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_blob import (
    Blob,
    blob_relationship,
    get_blob_img_html,
)
from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no_none,
    subheading_spanning_two_columns,
    td,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    PermittedValueChecker,
    SummaryCategoryColType,
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
    TrackerInfo,
    TrackerLabel,
)


# =============================================================================
# SLUMS
# =============================================================================

ZERO_OR_TWO_CHECKER = PermittedValueChecker(permitted_values=[0, 2])


class Slums(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the SLUMS task.
    """
    __tablename__ = "slums"
    shortname = "SLUMS"
    provides_trackers = True

    alert = CamcopsColumn(
        "alert", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Is the patient alert? (0 no, 1 yes)")
    highschooleducation = CamcopsColumn(
        "highschooleducation", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Does that patient have at least a high-school level of "
                "education? (0 no, 1 yes)"
    )

    q1 = CamcopsColumn(
        "q1", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q1 (day) (0-1)"
    )
    q2 = CamcopsColumn(
        "q2", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q2 (year) (0-1)"
    )
    q3 = CamcopsColumn(
        "q3", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q3 (state) (0-1)"
    )
    q5a = CamcopsColumn(
        "q5a", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q5a (money spent) (0-1)"
    )
    q5b = CamcopsColumn(
        "q5b", Integer,
        permitted_value_checker=ZERO_OR_TWO_CHECKER,
        comment="Q5b (money left) (0 or 2)"
    )  # worth 2 points
    q6 = CamcopsColumn(
        "q6", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment="Q6 (animal naming) (0-3)"
    )  # from 0 to 3 points
    q7a = CamcopsColumn(
        "q7a", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q7a (recall apple) (0-1)"
    )
    q7b = CamcopsColumn(
        "q7b", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q7b (recall pen) (0-1)"
    )
    q7c = CamcopsColumn(
        "q7c", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q7c (recall tie) (0-1)"
    )
    q7d = CamcopsColumn(
        "q7d", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q7d (recall house) (0-1)"
    )
    q7e = CamcopsColumn(
        "q7e", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q7e (recall car) (0-1)"
    )
    q8b = CamcopsColumn(
        "q8b", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q8b (reverse 648) (0-1)"
    )
    q8c = CamcopsColumn(
        "q8c", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q8c (reverse 8537) (0-1)"
    )
    q9a = CamcopsColumn(
        "q9a", Integer,
        permitted_value_checker=ZERO_OR_TWO_CHECKER,
        comment="Q9a (clock - hour markers) (0 or 2)"
    )  # worth 2 points
    q9b = CamcopsColumn(
        "q9b", Integer,
        permitted_value_checker=ZERO_OR_TWO_CHECKER,
        comment="Q9b (clock - time) (0 or 2)"
    )  # worth 2 points
    q10a = CamcopsColumn(
        "q10a", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q10a (X in triangle) (0-1)"
    )
    q10b = CamcopsColumn(
        "q10b", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Q10b (biggest figure) (0-1)"
    )
    q11a = CamcopsColumn(
        "q11a", Integer,
        permitted_value_checker=ZERO_OR_TWO_CHECKER,
        comment="Q11a (story - name) (0 or 2)"
    )  # worth 2 points
    q11b = CamcopsColumn(
        "q11b", Integer,
        permitted_value_checker=ZERO_OR_TWO_CHECKER,
        comment="Q11b (story - occupation) (0 or 2)"
    )  # worth 2 points
    q11c = CamcopsColumn(
        "q11c", Integer,
        permitted_value_checker=ZERO_OR_TWO_CHECKER,
        comment="Q11c (story - back to work) (0 or 2)"
    )  # worth 2 points
    q11d = CamcopsColumn(
        "q11d", Integer,
        permitted_value_checker=ZERO_OR_TWO_CHECKER,
        comment="Q11d (story - state) (0 or 2)"
    )  # worth 2 points

    clockpicture_blobid = CamcopsColumn(
        "clockpicture_blobid", Integer,
        is_blob_id_field=True, blob_relationship_attr_name="clockpicture",
        comment="BLOB ID of clock picture"
    )
    shapespicture_blobid = CamcopsColumn(
        "shapespicture_blobid", Integer,
        is_blob_id_field=True, blob_relationship_attr_name="shapespicture",
        comment="BLOB ID of shapes picture"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )

    clockpicture = blob_relationship("Slums", "clockpicture_blobid")  # type: Optional[Blob]  # noqa
    shapespicture = blob_relationship("Slums", "shapespicture_blobid")  # type: Optional[Blob]  # noqa

    PREAMBLE_FIELDS = ["alert", "highschooleducation"]
    SCORED_FIELDS = [
        "q1", "q2", "q3",
        "q5a", "q5b",
        "q6",
        "q7a", "q7b", "q7c", "q7d", "q7e",
        "q8b", "q8c",
        "q9a", "q9b",
        "q10a", "q10b",
        "q11a", "q11b", "q11c", "q11d"
    ]
    MAX_SCORE = 30

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("St Louis University Mental Status")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        if self.highschooleducation == 1:
            hlines = [26.5, 20.5]
            y_upper = 28.25
            y_middle = 23.5
        else:
            hlines = [24.5, 19.5]
            y_upper = 27.25
            y_middle = 22
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="SLUMS total score",
            axis_label=f"Total score (out of {self.MAX_SCORE})",
            axis_min=-0.5,
            axis_max=self.MAX_SCORE + 0.5,
            horizontal_lines=hlines,
            horizontal_labels=[
                TrackerLabel(y_upper, req.sstring(SS.NORMAL)),
                TrackerLabel(y_middle, self.wxstring(req, "category_mci")),
                TrackerLabel(17, self.wxstring(req, "category_dementia")),
            ]
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"SLUMS total score {self.total_score()}/{self.MAX_SCORE} "
                    f"({self.category(req)})"
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score(),
                           comment=f"Total score (/{self.MAX_SCORE})"),
            SummaryElement(name="category",
                           coltype=SummaryCategoryColType,
                           value=self.category(req),
                           comment="Category"),
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.PREAMBLE_FIELDS +
                                     self.SCORED_FIELDS) and
            self.field_contents_valid()
        )

    def total_score(self) -> int:
        return self.sum_fields(self.SCORED_FIELDS)

    def category(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        if self.highschooleducation == 1:
            if score >= 27:
                return req.sstring(SS.NORMAL)
            elif score >= 21:
                return self.wxstring(req, "category_mci")
            else:
                return self.wxstring(req, "category_dementia")
        else:
            if score >= 25:
                return req.sstring(SS.NORMAL)
            elif score >= 20:
                return self.wxstring(req, "category_mci")
            else:
                return self.wxstring(req, "category_dementia")

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
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
                    <th width="80%">Question</th>
                    <th width="20%">Score</th>
                </tr>
        """.format(
            clinician_comments=self.get_standard_clinician_comments_block(
                req, self.comments),
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            total_score=tr(
                req.sstring(SS.TOTAL_SCORE),
                answer(score) + f" / {self.MAX_SCORE}"
            ),
            category=tr_qa(
                req.sstring(SS.CATEGORY) + " <sup>[1]</sup>",
                category
            ),
        )
        h += tr_qa(self.wxstring(req, "alert_s"),
                   get_yes_no_none(req, self.alert))
        h += tr_qa(self.wxstring(req, "highschool_s"),
                   get_yes_no_none(req, self.highschooleducation))
        h += tr_qa(self.wxstring(req, "q1_s"), self.q1)
        h += tr_qa(self.wxstring(req, "q2_s"), self.q2)
        h += tr_qa(self.wxstring(req, "q3_s"), self.q3)
        h += tr("Q5 <sup>[2]</sup> (money spent, money left "
                "[<i>scores 2</i>]",
                ", ".join([answer(x) for x in [self.q5a, self.q5b]]))
        h += tr_qa("Q6 (animal fluency) [<i>≥15 scores 3, 10–14 scores 2, "
                   "5–9 scores 1, 0–4 scores 0</i>]",
                   self.q6)
        h += tr("Q7 (recall: apple, pen, tie, house, car)",
                ", ".join([answer(x) for x in [self.q7a, self.q7b, self.q7c,
                                               self.q7d, self.q7e]]))
        h += tr("Q8 (backwards: 648, 8537)",
                ", ".join([answer(x) for x in [self.q8b, self.q8c]]))
        h += tr("Q9 (clock: hour markers, time [<i>score 2 each</i>]",
                ", ".join([answer(x) for x in [self.q9a, self.q9b]]))
        h += tr("Q10 (X in triangle; which is biggest?)",
                ", ".join([answer(x) for x in [self.q10a, self.q10b]]))
        h += tr("Q11 (story: Female’s name? Job? When back to work? "
                "State she lived in? [<i>score 2 each</i>])",
                ", ".join([answer(x) for x in [self.q11a, self.q11b,
                                               self.q11c, self.q11d]]))
        h += f"""
            </table>
            <table class="{CssClass.TASKDETAIL}">
        """
        h += subheading_spanning_two_columns("Images of tests: clock, shapes")
        # noinspection PyTypeChecker
        h += tr(
            td(get_blob_img_html(self.clockpicture),
               td_width="50%", td_class=CssClass.PHOTO),
            td(get_blob_img_html(self.shapespicture),
               td_width="50%", td_class=CssClass.PHOTO),
            literal=True
        )
        h += f"""
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] With high school education:
                ≥27 normal, ≥21 MCI, ≤20 dementia.
                Without high school education:
                ≥25 normal, ≥20 MCI, ≤19 dementia.
                (Tariq et al. 2006, PubMed ID 17068312.)
                [2] Q4 (learning the five words) isn’t scored.
            </div>
        """
        return h
