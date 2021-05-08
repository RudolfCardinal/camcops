#!/usr/bin/env python

"""
camcops_server/tasks/gmcpq.py

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

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    POSSIBLE_SEX_VALUES,
)
from camcops_server.cc_modules.cc_html import (
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
    ONE_TO_FIVE_CHECKER,
    PermittedValueChecker,
    ZERO_TO_FIVE_CHECKER,
)
from camcops_server.cc_modules.cc_sqla_coltypes import SexColType
from camcops_server.cc_modules.cc_task import get_from_dict, Task
from camcops_server.cc_modules.cc_text import SS


# =============================================================================
# GMCPQ
# =============================================================================

class GMCPQ(Task):
    """
    Server implementation of the GMC-PQ task.
    """
    __tablename__ = "gmcpq"
    shortname = "GMC-PQ"

    RATING_TEXT = " (1 poor - 5 very good, 0 does not apply)"
    AGREE_TEXT = " (1 strongly disagree - 5 strongly agree, 0 does not apply)"

    doctor = Column(
        "doctor", UnicodeText,
        comment="Doctor's name"
    )
    q1 = CamcopsColumn(
        "q1", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=1, maximum=4),
        comment="Filling in questionnaire for... (1 yourself, "
                "2 child, 3 spouse/partner, 4 other relative/friend)"
    )
    q2a = CamcopsColumn(
        "q2a", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Reason: advice? (0 no, 1 yes)"
    )
    q2b = CamcopsColumn(
        "q2b", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Reason: one-off problem? (0 no, 1 yes)"
    )
    q2c = CamcopsColumn(
        "q2c", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Reason: ongoing problem? (0 no, 1 yes)")
    q2d = CamcopsColumn(
        "q2d", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Reason: routine check? (0 no, 1 yes)"
    )
    q2e = CamcopsColumn(
        "q2e", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Reason: treatment? (0 no, 1 yes)"
    )
    q2f = CamcopsColumn(
        "q2f", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Reason: other? (0 no, 1 yes)"
    )
    q2f_details = Column(
        "q2f_details", UnicodeText,
        comment="Reason, other, details"
    )
    q3 = CamcopsColumn(
        "q3", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="How important to health/wellbeing was the reason "
                "(1 not very - 5 very)"
    )
    q4a = CamcopsColumn(
        "q4a", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How good: being polite" + RATING_TEXT
    )
    q4b = CamcopsColumn(
        "q4b", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How good: making you feel at ease" + RATING_TEXT
    )
    q4c = CamcopsColumn(
        "q4c", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How good: listening" + RATING_TEXT
    )
    q4d = CamcopsColumn(
        "q4d", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How good: assessing medical condition" + RATING_TEXT
    )
    q4e = CamcopsColumn(
        "q4e", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How good: explaining" + RATING_TEXT
    )
    q4f = CamcopsColumn(
        "q4f", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How good: involving you in decisions" + RATING_TEXT
    )
    q4g = CamcopsColumn(
        "q4g", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How good: providing/arranging treatment" + RATING_TEXT
    )
    q5a = CamcopsColumn(
        "q5a", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="Agree/disagree: will keep info confidential" + AGREE_TEXT
    )
    q5b = CamcopsColumn(
        "q5b", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="Agree/disagree: honest/trustworthy" + AGREE_TEXT
    )
    q6 = CamcopsColumn(
        "q6", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Confident in doctor's ability to provide care (0 no, 1 yes)"
    )
    q7 = CamcopsColumn(
        "q7", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Would be completely happy to see this doctor again "
                "(0 no, 1 yes)"
    )
    q8 = CamcopsColumn(
        "q8", Integer,
        permitted_value_checker=BIT_CHECKER,
        comment="Was this visit with your usual doctor (0 no, 1 yes)"
    )
    q9 = Column(
        "q9", UnicodeText,
        comment="Other comments"
    )
    q10 = CamcopsColumn(
        "q10", SexColType,
        permitted_value_checker=PermittedValueChecker(
            permitted_values=POSSIBLE_SEX_VALUES),
        comment="Sex of rater (M, F, X)"
    )
    q11 = CamcopsColumn(
        "q11", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Age (1 = under 15, 2 = 15-20, 3 = 21-40, "
                "4 = 40-60, 5 = 60 or over"  # yes, I know it's daft
    )
    q12 = CamcopsColumn(
        "q12", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=1, maximum=16),
        comment="Ethnicity (1 = White British, 2 = White Irish, "
                "3 = White other, 4 = Mixed W/B Caribbean, "
                "5 = Mixed W/B African, 6 = Mixed W/Asian, 7 = Mixed other, "
                "8 = Asian/Asian British - Indian, 9 = A/AB - Pakistani, "
                "10 = A/AB - Bangladeshi, 11 = A/AB - other, "
                "12 = Black/Black British - Caribbean, 13 = B/BB - African, "
                "14 = B/BB - other, 15 = Chinese, 16 = other)"
    )
    q12_details = Column(
        "q12_details", UnicodeText,
        comment="Ethnic group, other, details"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("GMC Patient Questionnaire")

    def is_complete(self) -> bool:
        return (
                self.is_field_not_none("q1") and
                self.is_field_not_none("q3") and
                self.is_field_not_none("q4a") and
                self.is_field_not_none("q4b") and
                self.is_field_not_none("q4c") and
                self.is_field_not_none("q4d") and
                self.is_field_not_none("q4e") and
                self.is_field_not_none("q4f") and
                self.is_field_not_none("q4g") and
                self.is_field_not_none("q5a") and
                self.is_field_not_none("q5b") and
                self.is_field_not_none("q6") and
                self.is_field_not_none("q7") and
                self.is_field_not_none("q8") and
                self.field_contents_valid()
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        dict_q1 = {None: None}
        dict_q3 = {None: None}
        dict_q4 = {None: None}
        dict_q5 = {None: None}
        dict_q11 = {None: None}
        dict_q12 = {None: None}
        for option in range(1, 5):
            dict_q1[option] = self.wxstring(req, "q1_option" + str(option))
        for option in range(1, 6):
            dict_q3[option] = self.wxstring(req, "q3_option" + str(option))
            dict_q11[option] = self.wxstring(req, "q11_option" + str(option))
        for option in range(0, 6):
            prefix = str(option) + " – " if option > 0 else ""
            dict_q4[option] = prefix + self.wxstring(req,
                                                     "q4_option" + str(option))
            dict_q5[option] = prefix + self.wxstring(req,
                                                     "q5_option" + str(option))
        for option in range(1, 17):
            dict_q12[option] = self.wxstring(req,
                                             "ethnicity_option" + str(option))
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
        """
        ell = "&hellip; "  # horizontal ellipsis
        sep_row = subheading_spanning_two_columns("<br>")
        blank_cell = td("", td_class=CssClass.SUBHEADING)
        h += tr_qa(self.wxstring(req, "q_doctor"), ws.webify(self.doctor))
        h += sep_row
        h += tr_qa(self.wxstring(req, "q1"), get_from_dict(dict_q1, self.q1))
        h += tr(td(self.wxstring(req, "q2")), blank_cell, literal=True)
        h += tr_qa(ell + self.wxstring(req, "q2_a"),
                   get_yes_no_none(req, self.q2a),
                   default="")
        h += tr_qa(ell + self.wxstring(req, "q2_b"),
                   get_yes_no_none(req, self.q2b),
                   default="")
        h += tr_qa(ell + self.wxstring(req, "q2_c"),
                   get_yes_no_none(req, self.q2c),
                   default="")
        h += tr_qa(ell + self.wxstring(req, "q2_d"),
                   get_yes_no_none(req, self.q2d),
                   default="")
        h += tr_qa(ell + self.wxstring(req, "q2_e"),
                   get_yes_no_none(req, self.q2e),
                   default="")
        h += tr_qa(ell + self.wxstring(req, "q2_f"),
                   get_yes_no_none(req, self.q2f),
                   default="")
        h += tr_qa(ell + ell + self.wxstring(req, "q2f_s"),
                   ws.webify(self.q2f_details))
        h += tr_qa(self.wxstring(req, "q3"), get_from_dict(dict_q3, self.q3))
        h += tr(td(self.wxstring(req, "q4")), blank_cell, literal=True)
        h += tr_qa(ell + self.wxstring(req, "q4_a"),
                   get_from_dict(dict_q4, self.q4a))
        h += tr_qa(ell + self.wxstring(req, "q4_b"),
                   get_from_dict(dict_q4, self.q4b))
        h += tr_qa(ell + self.wxstring(req, "q4_c"),
                   get_from_dict(dict_q4, self.q4c))
        h += tr_qa(ell + self.wxstring(req, "q4_d"),
                   get_from_dict(dict_q4, self.q4d))
        h += tr_qa(ell + self.wxstring(req, "q4_e"),
                   get_from_dict(dict_q4, self.q4e))
        h += tr_qa(ell + self.wxstring(req, "q4_f"),
                   get_from_dict(dict_q4, self.q4f))
        h += tr_qa(ell + self.wxstring(req, "q4_g"),
                   get_from_dict(dict_q4, self.q4g))
        h += tr(td(self.wxstring(req, "q5")), blank_cell, literal=True)
        h += tr_qa(ell + self.wxstring(req, "q5_a"),
                   get_from_dict(dict_q5, self.q5a))
        h += tr_qa(ell + self.wxstring(req, "q5_b"),
                   get_from_dict(dict_q5, self.q5b))
        h += tr_qa(self.wxstring(req, "q6"), get_yes_no_none(req, self.q6))
        h += tr_qa(self.wxstring(req, "q7"), get_yes_no_none(req, self.q7))
        h += tr_qa(self.wxstring(req, "q8"), get_yes_no_none(req, self.q8))
        h += tr_qa(self.wxstring(req, "q9_s"), ws.webify(self.q9))
        h += sep_row
        h += tr_qa(req.sstring(SS.SEX), ws.webify(self.q10))
        h += tr_qa(self.wxstring(req, "q11"),
                   get_from_dict(dict_q11, self.q11))
        h += tr_qa(self.wxstring(req, "q12"),
                   get_from_dict(dict_q12, self.q12))
        h += tr_qa(ell + self.wxstring(req, "ethnicity_other_s"),
                   ws.webify(self.q12_details))
        h += """
            </table>
        """
        return h
