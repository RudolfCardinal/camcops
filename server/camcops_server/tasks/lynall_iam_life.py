#!/usr/bin/env python

"""
camcops_server/tasks/lynall_iam_life.py

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

**Lynall M-E — IAM study — life events.**

"""

from typing import Any, Dict, List, Tuple, Type

from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import answer, get_yes_no_none
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BoolColumn,
    CamcopsColumn,
    MIN_ZERO_CHECKER,
    ONE_TO_THREE_CHECKER,
    ZERO_TO_100_CHECKER,
)
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin


# =============================================================================
# LynallIamLifeEvents
# =============================================================================

N_QUESTIONS = 14

SPECIAL_SEVERITY_QUESTIONS = [14]
SPECIAL_FREQUENCY_QUESTIONS = [1, 2, 3, 8]
FREQUENCY_AS_PERCENT_QUESTIONS = [1, 2, 8]

QPREFIX = "q"
QSUFFIX_MAIN = "_main"
QSUFFIX_SEVERITY = "_severity"
QSUFFIX_FREQUENCY = "_frequency"

SEVERITY_MIN = 1
SEVERITY_MAX = 3


def qfieldname_main(qnum: int) -> str:
    return f"{QPREFIX}{qnum}{QSUFFIX_MAIN}"


def qfieldname_severity(qnum: int) -> str:
    return f"{QPREFIX}{qnum}{QSUFFIX_SEVERITY}"


def qfieldname_frequency(qnum: int) -> str:
    return f"{QPREFIX}{qnum}{QSUFFIX_FREQUENCY}"


class LynallIamLifeEventsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['LynallIamLifeEvents'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        comment_strings = [
            "illness/injury/assault (self)",  # 1
            "illness/injury/assault (relative)",
            "parent/child/spouse/sibling died",
            "close family friend/other relative died",
            "marital separation or broke off relationship",  # 5
            "ended long-lasting friendship with close friend/relative",
            "problems with close friend/neighbour/relative",
            "unsuccessful job-seeking for >1 month",  # 8
            "sacked/made redundant",  # 9
            "major financial crisis",  # 10
            "problem with police involving court appearance",
            "something valued lost/stolen",
            "self/partner gave birth",
            "other significant negative events",  # 14
        ]
        for q in range(1, N_QUESTIONS + 1):
            i = q - 1

            fn_main = qfieldname_main(q)
            cmt_main = f"Q{q}: in last 6 months: {comment_strings[i]} (0 no, 1 yes)"  # noqa
            setattr(cls, fn_main, BoolColumn(fn_main, comment=cmt_main))

            fn_severity = qfieldname_severity(q)
            cmt_severity = (
                f"Q{q}: (if yes) how bad was that "
                f"(1 not too bad, 2 moderately bad, 3 very bad)"
            )
            setattr(cls, fn_severity, CamcopsColumn(
                fn_severity,
                Integer,
                comment=cmt_severity,
                permitted_value_checker=ONE_TO_THREE_CHECKER
            ))

            fn_frequency = qfieldname_frequency(q)
            if q in FREQUENCY_AS_PERCENT_QUESTIONS:
                cmt_frequency = (
                    f"Q{q}: For what percentage of your life since aged 18 "
                    f"has [this event: {comment_strings[i]}] been happening? "
                    f"(0-100)"
                )
                pv_frequency = ZERO_TO_100_CHECKER
            else:
                cmt_frequency = (
                    f"Q{q}: Since age 18, how many times has this happened to "
                    f"you in total?"
                )
                pv_frequency = MIN_ZERO_CHECKER
            setattr(cls, fn_frequency, CamcopsColumn(
                fn_frequency,
                Integer,
                comment=cmt_frequency,
                permitted_value_checker=pv_frequency
            ))

        super().__init__(name, bases, classdict)


class LynallIamLifeEvents(TaskHasPatientMixin, Task,
                          metaclass=LynallIamLifeEventsMetaclass):
    """
    Server implementation of the LynallIamLifeEvents task.
    """
    __tablename__ = "lynall_iam_life"
    shortname = "Lynall_IAM_Life"

    prohibits_commercial = True

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Lynall M-E — IAM — Life events")

    def is_complete(self) -> bool:
        for q in range(1, N_QUESTIONS + 1):
            value_main = getattr(self, qfieldname_main(q))
            if value_main is None:
                return False
            if not value_main:
                continue
            if (getattr(self, qfieldname_severity(q)) is None or
                    getattr(self, qfieldname_frequency(q)) is None):
                return False
        return True

    def n_endorsed(self) -> int:
        """
        The number of main items endorsed.
        """
        fieldnames = [qfieldname_main(q) for q in range(1, N_QUESTIONS + 1)]
        return self.count_booleans(fieldnames)

    def severity_score(self) -> int:
        """
        The sum of severity scores.

        These are intrinsically coded 1 = not too bad, 2 = moderately bad, 3 =
        very bad. In addition, we score 0 for "not experienced".
        """
        total = 0
        for q in range(1, N_QUESTIONS + 1):
            v_main = getattr(self, qfieldname_main(q))
            if v_main:  # if endorsed
                v_severity = getattr(self, qfieldname_severity(q))
                if v_severity is not None:
                    total += v_severity
        return total

    def get_task_html(self, req: CamcopsRequest) -> str:
        options_severity = {
            3: self.wxstring(req, "severity_a3"),
            2: self.wxstring(req, "severity_a2"),
            1: self.wxstring(req, "severity_a1"),
        }
        q_a = []  # type: List[str]
        for q in range(1, N_QUESTIONS + 1):
            fieldname_main = qfieldname_main(q)
            q_main = self.wxstring(req, fieldname_main)
            v_main = getattr(self, fieldname_main)
            a_main = answer(get_yes_no_none(req, v_main))
            if v_main:
                v_severity = getattr(self, qfieldname_severity(q))
                a_severity = answer(
                    f"{v_severity}: {options_severity.get(v_severity)}"
                    if v_severity is not None else None
                )
                v_frequency = getattr(self, qfieldname_frequency(q))
                text_frequency = v_frequency
                if q in FREQUENCY_AS_PERCENT_QUESTIONS:
                    note_frequency = "a"
                    if v_frequency is not None:
                        text_frequency = f"{v_frequency}%"
                else:
                    note_frequency = "b"
                a_frequency = (
                    f"{answer(text_frequency)} <sup>[{note_frequency}]</sup>"
                    if text_frequency is not None else answer(None)
                )
            else:
                a_severity = ""
                a_frequency = ""
            q_a.append(f"""
                <tr>
                    <td>{q_main}</td>
                    <td>{a_main}</td>
                    <td>{a_severity}</td>
                    <td>{a_frequency}</td>
                </tr>
            """)
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>Number of categories endorsed</td>
                        <td>{answer(self.n_endorsed())} / {N_QUESTIONS}</td>
                    </tr>
                    <tr>
                        <td>Severity score <sup>[c]</sup></td>
                        <td>{answer(self.severity_score())} /
                            {N_QUESTIONS * 3}</td>
                    </tr>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="40%">Question</th>
                    <th width="20%">Experienced</th>
                    <th width="20%">Severity</th>
                    <th width="20%">Frequency</th>
                </tr>
                {"".join(q_a)}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [a] Percentage of life, since age 18, spent experiencing this.
                [b] Number of times this has happened, since age 18.
                [c] The severity score is the sum of “severity” ratings
                    (0 = not experienced, 1 = not too bad, 1 = moderately bad,
                    3 = very bad).
            </div>
        """
