#!/usr/bin/env python

"""
camcops_server/tasks/gaf.py

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

from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_ONLY_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import answer, tr
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
    TaskHasClinicianMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# GAF (crippled)
# =============================================================================

class Gaf(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the GAF task.
    """
    __tablename__ = "gaf"
    shortname = "GAF"
    provides_trackers = True

    score = CamcopsColumn(
        "score", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=100),
        comment="GAF score (1-100 or 0 for insufficient information)"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Global Assessment of Functioning (data collection only)")

    def is_complete(self) -> bool:
        return (
            self.score is not None and
            self.score != 0 and
            self.field_contents_valid()
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [TrackerInfo(
            value=self.total_score(),
            plot_label="GAF score (rating overall functioning)",
            axis_label="Score (1-100)",
            axis_min=0.5,
            axis_max=100.5
        )]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=f"GAF score {self.total_score()}")]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields()

    def total_score(self) -> Optional[int]:
        if self.score == 0:
            return None
        return self.score

    def get_task_html(self, req: CamcopsRequest) -> str:
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr(req.wappstring(AS.GAF_SCORE), answer(self.score))}
                </table>
            </div>
            {DATA_COLLECTION_ONLY_DIV}
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [SnomedExpression(req.snomed(SnomedLookup.GAF_SCALE))]
