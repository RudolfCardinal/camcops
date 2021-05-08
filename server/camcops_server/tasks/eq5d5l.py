#!/usr/bin/env python
# camcops_server/tasks/eq5d5l.py

"""
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

- By Joe Kearney, Rudolf Cardinal.

"""

from typing import List, Optional

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.sql.sqltypes import Integer, String

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ONE_TO_FIVE_CHECKER,
    ZERO_TO_100_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import (
    equally_spaced_int,
    regular_tracker_axis_ticks_int,
    TrackerInfo,
)


# =============================================================================
# EQ-5D-5L
# =============================================================================

class Eq5d5l(TaskHasPatientMixin, Task):
    """
    Server implementation of the EQ-5D-5L task.

    Note that the "index value" summary (e.g. SNOMED-CT code 736534008) is not
    implemented. This is a country-specific conversion of the raw values to a
    unitary health value; see

    - https://euroqol.org/publications/key-euroqol-references/value-sets/
    - https://euroqol.org/eq-5d-instruments/eq-5d-3l-about/valuation/choosing-a-value-set/
    """  # noqa
    __tablename__ = "eq5d5l"
    shortname = "EQ-5D-5L"
    provides_trackers = True

    q1 = CamcopsColumn(
        "q1", Integer,
        comment="Q1 (mobility) (1 no problems - 5 unable)",
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    q2 = CamcopsColumn(
        "q2", Integer,
        comment="Q2 (self-care) (1 no problems - 5 unable)",
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    q3 = CamcopsColumn(
        "q3", Integer,
        comment="Q3 (usual activities) (1 no problems - 5 unable)",
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    q4 = CamcopsColumn(
        "q4", Integer,
        comment="Q4 (pain/discomfort) (1 none - 5 extreme)",
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    q5 = CamcopsColumn(
        "q5", Integer,
        comment="Q5 (anxiety/depression) (1 not - 5 extremely)",
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    health_vas = CamcopsColumn(
        "health_vas", Integer,
        comment="Visual analogue scale for overall health (0 worst - 100 best)",  # noqa
        permitted_value_checker=ZERO_TO_100_CHECKER,
    )  # type: Optional[int]

    N_QUESTIONS = 5
    MISSING_ANSWER_VALUE = 9
    QUESTIONS = strseq("q", 1, N_QUESTIONS)
    QUESTIONS += ["health_vas"]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("EuroQol 5-Dimension, 5-Level Health Scale")

    def is_complete(self) -> bool:
        return self.all_fields_not_none(self.QUESTIONS)

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.health_vas,
            plot_label="EQ-5D-5L health visual analogue scale",
            axis_label="Self-rated health today (out of 100)",
            axis_min=-0.5,
            axis_max=100.5,
            axis_ticks=regular_tracker_axis_ticks_int(0, 100, 25),
            horizontal_lines=equally_spaced_int(0, 100, 25),
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="health_state", coltype=String(length=5),
                value=self.get_health_state_code(),
                comment="Health state as a 5-character string of numbers, "
                        "with 9 indicating a missing value"),
            SummaryElement(
                name="visual_task_score", coltype=Integer(),
                value=self.get_vis_score_or_999(),
                comment="Visual analogue health score "
                        "(0-100, with 999 replacing None)")
        ]

    def get_health_state_code(self) -> str:
        mcq = ""
        for i in range(1, self.N_QUESTIONS + 1):
            ans = getattr(self, "q" + str(i))
            if ans is None:
                mcq += str(self.MISSING_ANSWER_VALUE)
            else:
                mcq += str(ans)
        return mcq

    def get_vis_score_or_999(self) -> int:
        vis_score = self.health_vas
        if vis_score is None:
            return 999
        return vis_score

    def get_task_html(self, req: CamcopsRequest) -> str:
        q_a = ""

        for i in range(1, self.N_QUESTIONS + 1):
            nstr = str(i)
            answers = {
                None: None,
                1: "1 – " + self.wxstring(req, "q" + nstr + "_o1"),
                2: "2 – " + self.wxstring(req, "q" + nstr + "_o2"),
                3: "3 – " + self.wxstring(req, "q" + nstr + "_o3"),
                4: "4 – " + self.wxstring(req, "q" + nstr + "_o4"),
                5: "5 – " + self.wxstring(req, "q" + nstr + "_o5"),
            }

            q_a += tr_qa(nstr + ". " + self.wxstring(req, "q" + nstr + "_h"),
                         get_from_dict(answers, getattr(self, "q" + str(i))))

        q_a += tr_qa(
            ("Self-rated health on a visual analogue scale (0–100) "
             "<sup>[2]</sup>"),
            self.health_vas)

        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_qa("Health state code <sup>[1]</sup>",
                           self.get_health_state_code())}
                    {tr_qa("Visual analogue scale summary number <sup>[2]</sup>",
                           self.get_vis_score_or_999())}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] This is a string of digits, not a directly meaningful 
                    number. Each digit corresponds to a question.
                    A score of 1 indicates no problems in any given dimension.
                    5 indicates extreme problems. Missing values are
                    coded as 9.
                [2] This is the visual analogue health score, with missing 
                    values coded as 999.
            </div>
        """  # noqa

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.EQ5D5L_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.EQ5D5L_SCALE),
                {
                    # SnomedLookup.EQ5D5L_INDEX_VALUE: not used; see docstring above  # noqa
                    req.snomed(SnomedLookup.EQ5D5L_MOBILITY_SCORE): self.q1,
                    req.snomed(SnomedLookup.EQ5D5L_SELF_CARE_SCORE): self.q2,
                    req.snomed(SnomedLookup.EQ5D5L_USUAL_ACTIVITIES_SCORE): self.q3,  # noqa
                    req.snomed(SnomedLookup.EQ5D5L_PAIN_DISCOMFORT_SCORE): self.q4,  # noqa
                    req.snomed(SnomedLookup.EQ5D5L_ANXIETY_DEPRESSION_SCORE): self.q5,  # noqa
                }
            ))
        return codes
