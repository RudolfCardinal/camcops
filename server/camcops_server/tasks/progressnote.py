#!/usr/bin/env python

"""
camcops_server/tasks/progressnote.py

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

from typing import Dict, List

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo
from camcops_server.cc_modules.cc_html import answer
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import (
    SnomedConcept,
    SnomedExpression,
    SnomedLookup,
)
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS


# =============================================================================
# ProgressNote
# =============================================================================

class ProgressNote(TaskHasPatientMixin, TaskHasClinicianMixin, Task):
    """
    Server implementation of the ProgressNote task.
    """
    __tablename__ = "progressnote"
    shortname = "ProgressNote"

    location = Column("location", UnicodeText, comment="Location")
    note = Column("note", UnicodeText, comment="Clinical note")

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Clinical progress note")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        return [CtvInfo(content=ws.webify(self.note))]

    def is_complete(self) -> bool:
        return bool(self.note)

    def get_task_html(self, req: CamcopsRequest) -> str:
        # Avoid tables - PDF generator crashes if text is too long.
        return f"""
            <div class="{CssClass.HEADING}">
                {req.sstring(SS.LOCATION)}
            </div>
            <div>
                {answer(ws.webify(self.location),
                        default_for_blank_strings=True)}
            </div>
            <div class="{CssClass.HEADING}">
                {req.sstring(SS.NOTE)}
            </div>
            <div>
                {answer(self.note, default_for_blank_strings=True)}
            </div>
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        refinement = {}  # type: Dict[SnomedConcept, str]
        if self.note:
            refinement[req.snomed(SnomedLookup.CLINICAL_NOTE)] = self.note
        codes = [SnomedExpression(
            req.snomed(SnomedLookup.PROGRESS_NOTE_PROCEDURE),
            refinement=refinement or None
        )]
        return codes
