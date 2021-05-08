#!/usr/bin/env python

"""
camcops_server/tasks_discarded/bfcrs.py

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

**BFCRS task.**

"""

_ = '''

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

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
# BFCRS
# =============================================================================

class BfcrsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Bfcrs'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(cls, "q", 1, cls.NQUESTIONS)
        super().__init__(name, bases, classdict)


class Bfcrs(TaskHasPatientMixin, Task,
            metaclass=BfcrsMetaclass):
    __tablename__ = "bfcrs"
    shortname = "BFCRS"
    provides_trackers = True

    NQUESTIONS = 23
    N_CSI_QUESTIONS = 14  # the first 14
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_TOTAL = 69

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Bush–Francis Catatonia Rating Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="BFCRS total score",
            axis_label="Total score (out of {})".format(self.MAX_TOTAL),
            axis_min=-0.5,
            axis_max=self.MAX_TOTAL + 0.5
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment="Total score (out of {})".format(self.MAX_TOTAL)
            ),
        ]

    def is_complete(self) -> bool:
        return self.are_all_fields_complete(self.TASK_FIELDS)

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def get_num_csi_symptoms(self) -> int:
        n = 0
        for i in range(1, self.N_CSI_QUESTIONS + 1):
            value = getattr(self, "q" + str(i))
            if value is not None and value > 0:
                n += 1
        return n

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        n_csi_symptoms = self.get_num_csi_symptoms()
        csi_catatonia = n_csi_symptoms >= 2
        answer_dicts_dict = {}
        for q in self.TASK_FIELDS:
            d = {None: "?"}
            for option in range(0, 5):
                if (option != 0 and option != 3) and q in ["q17", "q18", "q19",
                                                           "q20", "q21"]:
                    continue
                d[option] = self.wxstring(req, q + "_option" + str(option))
            answer_dicts_dict[q] = d
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += """<tr><td>{}</td><td><b>{}</b></td></tr>""".format(
                "Q" + str(q) + " — " + self.wxstring(
                    req, "q" + str(q) + "_title"),
                get_from_dict(answer_dicts_dict["q" + str(q)],
                              getattr(self, "q" + str(q)))
            )
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {is_complete}
                    <tr>
                        <td>{total_score_str}</td>
                        <td><b>{score}</b> / {maxtotal}</td>
                    </tr>
                    <tr>
                        <td>{num_symptoms_present} <sup>[1]</sup></td>
                        <td><b>{n_csi_symptoms}</b></td>
                    </tr>
                    <tr>
                        <td>{catatonia_present} <sup>[2]</sup></td>
                        <td><b>{csi_catatonia}</b></td>
                    </tr>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="35%">Question</th>
                    <th width="65%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Symptoms 1–14, counted as present if score >0.
                [2] Number of CSI symptoms ≥2.
            </div>
        """.format(
            CssClass=CssClass,
            is_complete=self.get_is_complete_tr(req),
            total_score_str=req.sstring(SS.TOTAL_SCORE),
            score=score,
            maxtotal=self.MAX_TOTAL,
            num_symptoms_present=self.wxstring(req, "num_symptoms_present"),
            n_csi_symptoms=n_csi_symptoms,
            catatonia_present=self.wxstring(req, "catatonia_present"),
            csi_catatonia=get_yes_no(req, csi_catatonia),
            q_a=q_a,
        )
        return h

'''
