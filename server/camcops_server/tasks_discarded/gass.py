#!/usr/bin/env python

"""
camcops_server/tasks_discarded/gass.py

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

**GASS task.**

"""

_ = '''

from typing import Any, Dict, List, Tuple, Type

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, Text

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import get_yes_no
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# GASS
# =============================================================================

class GassMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Gass'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(cls, "q", 1, cls.NQUESTIONS)
        add_multiple_columns(cls, "d", 1, cls.NQUESTIONS)
        super().__init__(name, bases, classdict)


class Gass(TaskHasPatientMixin, Task,
           metaclass=GassMetaclass):
    __tablename__ = "gass"
    shortname = "GASS"
    provides_trackers = True

    medication = Column("medication", Text)

    NQUESTIONS = 22
    list_sedation = [1, 2]
    list_cardiovascular = [3, 4]
    list_epse = [5, 6, 7, 8, 9, 10]
    list_anticholinergic = [11, 12, 13]
    list_gastrointestinal = [14]
    list_genitourinary = [15]
    list_prolactinaemic_female = [17, 18, 19, 21]
    list_prolactinaemic_male = [17, 18, 19, 20]
    list_weightgain = [22]
    MAX_TOTAL = 63

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Glasgow Antipsychotic Side-effect Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="GASS total score",
            axis_label="Total score (out of {})".format(self.MAX_TOTAL),
            axis_min=-0.5,
            axis_max=self.MAX_TOTAL + 0.5
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment="Total score (out of {})".format(self.MAX_TOTAL)
            ),
        ]

    @staticmethod
    def get_q_fieldlist(group) -> List[str]:
        return ["q" + str(q) for q in group]

    @staticmethod
    def get_d_fieldlist(group) -> List[str]:
        return ["d" + str(q) for q in group]

    def get_relevant_q_fieldlist(self) -> List[str]:
        qnums = list(range(1, self.NQUESTIONS + 1))
        if self.is_female():
            qnums.remove(20)
        else:
            qnums.remove(21)
        return ["q" + str(q) for q in qnums]

    def is_complete(self) -> bool:
        return self.are_all_fields_complete(self.get_relevant_q_fieldlist())

    def total_score(self) -> int:
        return self.sum_fields(self.get_relevant_q_fieldlist())

    def group_score(self, qnums: List[int]) -> int:
        return self.sum_fields(self.get_q_fieldlist(qnums))

    @staticmethod
    def get_subheading(subtitle: str, score: int, max_score: int) -> str:
        return """
            <tr class="{CssClass.SUBHEADING}">
                <td>{subtitle}</td>
                <td colspan="2"><i><b>{score}</b> / {max_score}</i></td>
            </tr>
        """.format(
            CssClass=CssClass,
            subtitle=subtitle,
            score=score,
            max_score=max_score,
        )

    def get_row(self, req: CamcopsRequest, q: int, answer_dict: Dict) -> str:
        return """<tr><td>{}</td><td><b>{}</b></td><td>{}</td></tr>""".format(
            self.wxstring(req, "q" + str(q)),
            get_from_dict(answer_dict, getattr(self, "q" + str(q))),
            get_yes_no(req, getattr(self, "d" + str(q)))
        )

    def get_group_html(self,
                       req: CamcopsRequest,
                       qnums: List[int],
                       subtitle: str,
                       answer_dict: Dict) -> str:
        h = self.get_subheading(
            subtitle,
            self.group_score(qnums),
            len(qnums) * 3
        )
        for q in qnums:
            h += self.get_row(req, q, answer_dict)
        return h

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        answer_dict = {None: "?"}
        for option in range(0, 4):
            answer_dict[option] = (
                str(option) + " — " +
                self.wxstring(req, "option" + str(option))
            )
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {is_complete}
                    <tr>
                        <td>{total_score_str}</td>
                        <td><b>{score}</b> / {max_total}</td>
                    </tr>
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings pertain to the past week.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="65%">Question</th>
                    <th width="20%">Answer</th>
                    <th width="15%">Distressing?</th>
                </tr>
        """.format(
            CssClass=CssClass,
            is_complete=self.get_is_complete_tr(req),
            total_score_str=req.sstring(SS.TOTAL_SCORE),
            score=score,
            max_total=self.MAX_TOTAL,
        )
        h += self.get_group_html(req,
                                 self.list_sedation,
                                 self.wxstring(req, "group_sedation"),
                                 answer_dict)
        h += self.get_group_html(req,
                                 self.list_cardiovascular,
                                 self.wxstring(req, "group_cardiovascular"),
                                 answer_dict)
        h += self.get_group_html(req,
                                 self.list_epse,
                                 self.wxstring(req, "group_epse"),
                                 answer_dict)
        h += self.get_group_html(req,
                                 self.list_anticholinergic,
                                 self.wxstring(req, "group_anticholinergic"),
                                 answer_dict)
        h += self.get_group_html(req,
                                 self.list_gastrointestinal,
                                 self.wxstring(req, "group_gastrointestinal"),
                                 answer_dict)
        h += self.get_group_html(req,
                                 self.list_genitourinary,
                                 self.wxstring(req, "group_genitourinary"),
                                 answer_dict)
        if self.is_female():
            h += self.get_group_html(
                req,
                self.list_prolactinaemic_female,
                self.wxstring(req, "group_prolactinaemic") +
                " (" + req.sstring(SS.FEMALE) + ")",
                answer_dict)
        else:
            h += self.get_group_html(
                req,
                self.list_prolactinaemic_male,
                self.wxstring(req, "group_prolactinaemic") +
                " (" + req.sstring(SS.MALE) + ")",
                answer_dict)
        h += self.get_group_html(req,
                                 self.list_weightgain,
                                 self.wxstring(req, "group_weightgain"),
                                 answer_dict)
        h += """
                <tr class="{CssClass.SUBHEADING}">
                    <td colspan="3">{medication_hint}</td>
                </tr>
                <tr><td colspan="3">{medication}</td></tr>
            </table>
        """.format(
            CssClass=CssClass,
            medication_hint=self.wxstring(req, "medication_hint"),
            medication=ws.webify(self.medication)
        )
        return h

'''
