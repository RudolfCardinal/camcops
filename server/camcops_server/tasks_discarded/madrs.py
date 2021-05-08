#!/usr/bin/env python

"""
camcops_server/tasks_discarded/madrs.py

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

**MADRS task.**

"""

_ = '''

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, Text

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
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
# MADRS
# =============================================================================

class MadrsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Madrs'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(cls, "q", 1, cls.NQUESTIONS)
        super().__init__(name, bases, classdict)


class Madrs(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
            metaclass=MadrsMetaclass):
    __tablename__ = "madrs"
    shortname = "MADRS"
    provides_trackers = True

    period_rated = Column("period_rated", Text)

    NQUESTIONS = 10
    QFIELDS = strseq("q", 1, NQUESTIONS)
    TASK_FIELDS = QFIELDS + ["period_rated"]
    MAX_TOTAL = 60

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Montgomery–Åsberg Depression Rating Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="MADRS total score",
            axis_label="Total score (out of {})".format(self.MAX_TOTAL),
            axis_min=-0.5,
            axis_max=self.MAX_TOTAL + 0.5,
            horizontal_lines=[
                33.5,
                19.5,
                6.5,
            ],
            horizontal_labels=[
                TrackerLabel(35, req.sstring(SS.SEVERE)),
                TrackerLabel(25, req.sstring(SS.MODERATE)),
                TrackerLabel(14, req.sstring(SS.MILD)),
                TrackerLabel(3, req.sstring(SS.NORMAL))
            ]
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="total",
                           coltype=Integer(),
                           value=self.total_score()),
        ]

    def is_complete(self) -> bool:
        return self.are_all_fields_complete(self.TASK_FIELDS)

    def total_score(self) -> int:
        return self.sum_fields(self.QFIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        if score > 34:
            category = req.sstring(SS.SEVERE)
        elif score >= 20:
            category = req.sstring(SS.MODERATE)
        elif score >= 7:
            category = req.sstring(SS.MILD)
        else:
            category = req.sstring(SS.NORMAL)
        answer_dicts = []
        for q in range(1, self.NQUESTIONS + 1):
            d = {None: "?"}
            for option in range(0, 7):
                if option == 1 or option == 3 or option == 5:
                    d[option] = option
                else:
                    d[option] = self.wxstring(req, "q" + str(q) + "_option" +
                                              str(option))
            answer_dicts.append(d)
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += """<tr><td>{}</td><td><b>{}</b></td></tr>""".format(
                self.wxstring(req, "madrs_q" + str(q) + "_s"),
                get_from_dict(answer_dicts[q - 1], getattr(self, "q" + str(q)))
            )
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {is_complete}
                    <tr>
                        <td>{total_score_str}</td>
                        <td><b>{score}</b> / {max_total}</td>
                    </tr>
                    <tr>
                        <td>{category_str} <sup>[1]</sup></td>
                        <td><b>{category}</b>
                    </tr>
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Ratings are from 0–6 (0 none, 6 extreme problem).
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="30%">Question</th>
                    <th width="70%">Answer</th>
                </tr>
                <tr>
                    <td>{q_period_rated}</td>
                    <td><b>{period_rated}</b></td>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Total score &gt;34 severe, &ge;20 moderate, &ge;7 mild,
                &lt;7 normal. (Hermann et al. 1998, PubMed ID 9506602.)
            </div>
        """.format(
            CssClass=CssClass,
            is_complete=self.get_is_complete_tr(req),
            total_score_str=req.sstring(SS.TOTAL_SCORE),
            score=score,
            max_total=self.MAX_TOTAL,
            category_str=req.sstring(SS.CATEGORY),
            category=category,
            q_period_rated=self.wxstring(req, "q_period_rated"),
            period_rated=self.period_rated,
            q_a=q_a,
        )
        return h

'''
