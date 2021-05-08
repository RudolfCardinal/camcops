#!/usr/bin/env python

"""
camcops_server/tasks/khandaker_mojo_sociodemographics.py

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

from typing import Any, Dict, Optional, Tuple, Type

from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ZERO_TO_10_CHECKER,
    ZERO_TO_FOUR_CHECKER,
    ZERO_TO_SEVEN_CHECKER,
    ZERO_TO_SIX_CHECKER,
    ZERO_TO_TWO_CHECKER,
)
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
)


class KhandakerMojoSociodemographicsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['KhandakerMojoSociodemographics'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        setattr(
            cls, cls.FN_GENDER,
            CamcopsColumn(
                cls.FN_GENDER, Integer,
                permitted_value_checker=ZERO_TO_TWO_CHECKER,
                comment="Gender at birth (0 Male, 1 Female, 2 Other (specify))"
            )
        )
        setattr(
            cls, cls.FN_OTHER_GENDER,
            CamcopsColumn(
                cls.FN_OTHER_GENDER, UnicodeText,
                comment="Other (specify)"
            )
        )
        setattr(
            cls, cls.FN_ETHNICITY,
            CamcopsColumn(
                cls.FN_ETHNICITY, Integer,
                permitted_value_checker=ZERO_TO_10_CHECKER,
                comment=("Ethnicity (0 White, 1 Mixed, 2 Indian, 3 Pakistani, "
                         "4 Bangladeshi, 5 Other Asian, 6 Black Caribbean, "
                         "7 Black African, 8 Black Other, 9 Chinese, "
                         "10 Other (specify))")
            )
        )
        setattr(
            cls, cls.FN_OTHER_ETHNICITY,
            CamcopsColumn(
                cls.FN_OTHER_ETHNICITY, UnicodeText,
                comment="Other (specify)"
            )
        )
        setattr(
            cls, cls.FN_WITH_WHOM_LIVE,
            CamcopsColumn(
                cls.FN_WITH_WHOM_LIVE, Integer,
                permitted_value_checker=ZERO_TO_SEVEN_CHECKER,
                comment=("0 Alone, 1 Alone with children, 2 Partner/Spouse, "
                         "3 Partner/Spouse and children, 4 Parents, "
                         "5 Other family, 6 Friends, 7 Other (specify)")
            )
        )
        setattr(
            cls, cls.FN_OTHER_WITH_WHOM_LIVE,
            CamcopsColumn(
                cls.FN_OTHER_WITH_WHOM_LIVE, UnicodeText,
                comment="Other (specify)"
            )
        )
        setattr(
            cls, cls.FN_RELATIONSHIP_STATUS,
            CamcopsColumn(
                cls.FN_RELATIONSHIP_STATUS, Integer,
                permitted_value_checker=ZERO_TO_FOUR_CHECKER,
                comment=("0 Single, 1 Married / Civil partnership, "
                         "2 In steady relationship, 3 Divorced / separated, "
                         "4 Widowed")
            )
        )
        setattr(
            cls, cls.FN_EDUCATION,
            CamcopsColumn(
                cls.FN_EDUCATION, Integer,
                permitted_value_checker=ZERO_TO_FOUR_CHECKER,
                comment=("0 No qualifications, 1 GCSE/O levels, 2 A levels, "
                         "3 Vocational/college (B. Tecs/NVQs etc), "
                         "4 University / Professional Qualifications")
            )
        )
        setattr(
            cls, cls.FN_EMPLOYMENT,
            CamcopsColumn(
                cls.FN_EMPLOYMENT, Integer,
                permitted_value_checker=ZERO_TO_SEVEN_CHECKER,
                comment=("0 No unemployed, 1 No student, 2 Yes full time, "
                         "3 Yes part time, 4 Full time homemaker, "
                         "5 Self employed, 6 Not working for medical reasons, "
                         "7 Other (specify)")
            )
        )
        setattr(
            cls, cls.FN_OTHER_EMPLOYMENT,
            CamcopsColumn(
                cls.FN_OTHER_EMPLOYMENT, UnicodeText,
                comment="Other (specify)"
            )
        )
        setattr(
            cls, cls.FN_ACCOMMODATION,
            CamcopsColumn(
                cls.FN_ACCOMMODATION, Integer,
                permitted_value_checker=ZERO_TO_SIX_CHECKER,
                comment=("0 Own outright, 1 Own with mortgage, "
                         "2 Rent from local authority etc, "
                         "3 Rent from landlord (private), "
                         "4 Shared ownership (part rent, part mortgage)"
                         "5 Live rent free, 6 Other (specify)")
            )
        )
        setattr(
            cls, cls.FN_OTHER_ACCOMMODATION,
            CamcopsColumn(
                cls.FN_OTHER_ACCOMMODATION, UnicodeText,
                comment="Other (specify)"
            )
        )

        super().__init__(name, bases, classdict)


class KhandakerMojoSociodemographics(
        TaskHasPatientMixin, Task,
        metaclass=KhandakerMojoSociodemographicsMetaclass):
    """
    Server implementation of the Khandaker_2_MOJOSociodemographics task
    """
    __tablename__ = "khandaker_mojo_sociodemographics"
    shortname = "Khandaker_MOJO_Sociodemographics"
    provides_trackers = False

    FN_GENDER = "gender"
    FN_ETHNICITY = "ethnicity"
    FN_WITH_WHOM_LIVE = "with_whom_live"
    FN_RELATIONSHIP_STATUS = "relationship_status"
    FN_EDUCATION = "education"
    FN_EMPLOYMENT = "employment"
    FN_ACCOMMODATION = "accommodation"

    FN_OTHER_GENDER = "other_gender"
    FN_OTHER_ETHNICITY = "other_ethnicity"
    FN_OTHER_WITH_WHOM_LIVE = "other_with_whom_live"
    FN_OTHER_EMPLOYMENT = "other_employment"
    FN_OTHER_ACCOMMODATION = "other_accommodation"

    MANDATORY_FIELD_NAMES = [
        FN_GENDER,
        FN_ETHNICITY,
        FN_WITH_WHOM_LIVE,
        FN_RELATIONSHIP_STATUS,
        FN_EDUCATION,
        FN_EMPLOYMENT,
        FN_ACCOMMODATION,
    ]

    OTHER_ANSWER_VALUES = {
        FN_GENDER: 2,
        FN_ETHNICITY: 10,
        FN_WITH_WHOM_LIVE: 7,
        FN_EMPLOYMENT: 7,
        FN_ACCOMMODATION: 6,
    }

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Khandaker GM — MOJO — Sociodemographics")

    def is_complete(self) -> bool:
        if self.any_fields_none(self.MANDATORY_FIELD_NAMES):
            return False

        if not self.field_contents_valid():
            return False

        for name, other_option in self.OTHER_ANSWER_VALUES.items():
            if getattr(self, name) == other_option:
                if getattr(self, f"other_{name}") is None:
                    return False

        return True

    def get_task_html(self, req: CamcopsRequest) -> str:
        rows = ""

        for field_name in self.MANDATORY_FIELD_NAMES:
            question_text = self.xstring(req, f"q_{field_name}")
            answer_text = self.get_answer_text(req, field_name)

            rows += tr_qa(question_text, answer_text)

        html = f"""
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
                {rows}
            </table>
        """

        return html

    def get_answer_text(self, req: CamcopsRequest,
                        field_name: str) -> Optional[str]:
        answer = getattr(self, field_name)

        if answer is None:
            return answer

        answer_text = self.xstring(req, f"{field_name}_option{answer}")

        if self.answered_other(field_name):
            other_answer = getattr(self, f"other_{field_name}")

            if not other_answer:
                other_answer = "?"

            answer_text = f"{answer_text} — {other_answer}"

        return f"{answer} — {answer_text}"

    def answered_other(self, field_name: str):
        if field_name not in self.OTHER_ANSWER_VALUES:
            return False

        other_option = self.OTHER_ANSWER_VALUES[field_name]

        return getattr(self, field_name) == other_option
