#!/usr/bin/env python

"""
camcops_server/tasks/distressthermometer.py

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
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass, PV
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    get_yes_no_none,
    subheading_spanning_two_columns,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin


# =============================================================================
# Distress Thermometer
# =============================================================================

class DistressThermometerMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['DistressThermometer'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            pv=PV.BIT,
            comment_fmt="{n}. {s} (0 no, 1 yes)",
            comment_strings=[
                "child care",
                "housing",
                "insurance/financial",
                "transportation",
                "work/school",
                "children",
                "partner",
                "close friend/relative",
                "depression",
                "fears",
                "nervousness",
                "sadness",
                "worry",
                "loss of interest",
                "spiritual/religious",
                "appearance",
                "bathing/dressing",
                "breathing",
                "urination",
                "constipation",
                "diarrhoea",
                "eating",
                "fatigue",
                "feeling swollen",
                "fevers",
                "getting around",
                "indigestion",
                "memory/concentration",
                "mouth sores",
                "nausea",
                "nose dry/congested",
                "pain",
                "sexual",
                "skin dry/itchy",
                "sleep",
                "tingling in hands/feet",
            ]
        )
        super().__init__(name, bases, classdict)


class DistressThermometer(TaskHasPatientMixin, Task,
                          metaclass=DistressThermometerMetaclass):
    """
    Server implementation of the DistressThermometer task.
    """
    __tablename__ = "distressthermometer"
    shortname = "Distress Thermometer"

    distress = CamcopsColumn(
        "distress", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=10),
        comment="Distress (0 none - 10 extreme)"
    )
    other = Column("other", UnicodeText, comment="Other problems")

    NQUESTIONS = 36
    COMPLETENESS_FIELDS = strseq("q", 1, NQUESTIONS) + ["distress"]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Distress Thermometer")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if self.distress is None:
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=f"Overall distress: {self.distress}/10"
        )]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(self.COMPLETENESS_FIELDS) and
            self.field_contents_valid()
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_qa("Overall distress (0–10)", self.distress)}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                All questions relate to distress/problems “in the past week,
                including today” (yes = problem, no = no problem).
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
        """
        h += tr_qa("Distress (0 no distress – 10 extreme distress)",
                   self.distress)
        h += subheading_spanning_two_columns("Practical problems")
        for i in range(1, 5 + 1):
            h += tr_qa(
                f"{i}. {self.wxstring(req, 'q' + str(i))}",
                get_yes_no_none(req, getattr(self, "q" + str(i)))
            )
        h += subheading_spanning_two_columns("Family problems")
        for i in range(6, 8 + 1):
            h += tr_qa(
                f"{i}. {self.wxstring(req, 'q' + str(i))}",
                get_yes_no_none(req, getattr(self, "q" + str(i)))
            )
        h += subheading_spanning_two_columns("Emotional problems")
        for i in range(9, 14 + 1):
            h += tr_qa(
                f"{i}. {self.wxstring(req, 'q' + str(i))}",
                get_yes_no_none(req, getattr(self, "q" + str(i)))
            )
        h += subheading_spanning_two_columns("Spiritual problems")
        for i in range(15, 15 + 1):
            h += tr_qa(
                f"{i}. {self.wxstring(req, 'q' + str(i))}",
                get_yes_no_none(req, getattr(self, "q" + str(i)))
            )
        h += subheading_spanning_two_columns("Physical problems")
        for i in range(16, self.NQUESTIONS + 1):
            h += tr_qa(
                f"{i}. {self.wxstring(req, 'q' + str(i))}",
                get_yes_no_none(req, getattr(self, "q" + str(i)))
            )
        h += subheading_spanning_two_columns("Other problems")
        h += tr_qa(self.wxstring(req, "other_s"), self.other)
        h += """
            </table>
        """
        return h
