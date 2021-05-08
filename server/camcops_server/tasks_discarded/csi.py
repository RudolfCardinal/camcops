#!/usr/bin/env python

"""
camcops_server/tasks_discarded/csi.py

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

**CSI task.**

"""

_ = '''

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import get_yes_no, get_yes_no_unknown
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# CSI
# =============================================================================

class CsiMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Csi'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(cls, "q", 1, cls.NQUESTIONS)
        super().__init__(name, bases, classdict)


class Csi(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
          metaclass=CsiMetaclass):
    __tablename__ = "csi"
    shortname = "CSI"
    # !!! has_clinician was not implemented on tablet JS version; should be
    provides_trackers = True
    extrastring_taskname = "bfcrs"  # shares with BFCRS

    NQUESTIONS = 14
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Catatonia Screening Instrument")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="CSI total score",
            axis_label="Total score (out of {})".format(self.NQUESTIONS),
            axis_min=-0.5,
            axis_max=self.NQUESTIONS + 0.5,
            horizontal_lines=[1.5]
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Integer(),
                value=self.total_score(),
                comment="Total score (out of {})".format(self.NQUESTIONS)
            ),
        ]

    def is_complete(self) -> bool:
        return self.are_all_fields_complete(self.TASK_FIELDS)

    def total_score(self) -> int:
        return self.sum_fields(self.TASK_FIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        n_csi_symptoms = self.total_score()
        csi_catatonia = n_csi_symptoms >= 2
        q_a = ""
        for q in range(1, self.NQUESTIONS + 1):
            q_a += """<tr><td>{}</td><td><b>{}</b></td></tr>""".format(
                "Q" + str(q) + " — " +
                self.wxstring(req, "q" + str(q) + "_title"),
                get_yes_no_unknown(req, getattr(self, "q" + str(q)))
            )
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {is_complete}
                    <tr>
                        <td>{num_sx_str}</td>
                        <td><b>{n_csi_symptoms}</b> / {max_total}</td>
                    </tr>
                    <tr>
                        <td>{catatonia_str} <sup>[1]</sup></td>
                        <td><b>{csi_catatonia}</b></td>
                    </tr>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Present?</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Number of CSI symptoms ≥2.
            </div>
        """.format(
            CssClass=CssClass,
            is_complete=self.get_is_complete_tr(req),
            num_sx_str=self.wxstring(req, "num_symptoms_present"),
            n_csi_symptoms=n_csi_symptoms,
            max_total=self.NQUESTIONS,
            catatonia_str=self.wxstring(req, "catatonia_present"),
            csi_catatonia=get_yes_no(req, csi_catatonia),
            q_a=q_a,
        )
        return h

'''
