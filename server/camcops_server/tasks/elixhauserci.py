#!/usr/bin/env python

"""
camcops_server/tasks/elixhauserci.py

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

**Elixhauser Comorbidity Index task.**

"""

from typing import Any, Dict, List, Tuple, Type

from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import get_yes_no_unknown, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import BoolColumn
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS


# =============================================================================
# ElixhauserCI
# =============================================================================

FIELDNAMES = [
    "congestive_heart_failure",
    "cardiac_arrhythmias",
    "valvular_disease",
    "pulmonary_circulation_disorders",
    "peripheral_vascular_disorders",

    "hypertension_uncomplicated",
    "hypertension_complicated",
    "paralysis",
    "other_neurological_disorders",
    "chronic_pulmonary_disease",

    "diabetes_uncomplicated",
    "diabetes_complicated",
    "hypothyroidism",
    "renal_failure",
    "liver_disease",

    "peptic_ulcer_disease_exc_bleeding",
    "aids_hiv",
    "lymphoma",
    "metastatic_cancer",
    "solid_tumor_without_metastasis",

    "rheumatoid_arthritis_collagen_vascular_diseases",
    "coagulopathy",
    "obesity",
    "weight_loss",
    "fluid_electrolyte_disorders",

    "blood_loss_anemia",
    "deficiency_anemia",
    "alcohol_abuse",
    "drug_abuse",
    "psychoses",

    "depression",
]
MAX_SCORE = len(FIELDNAMES)

CONSTRAINT_NAME_MAP = {
    "pulmonary_circulation_disorders": "ck_elixhauserci_pulm_circ",
    "peptic_ulcer_disease_exc_bleeding": "ck_elixhauserci_peptic",
    "solid_tumor_without_metastasis": "ck_elixhauserci_tumour_no_mets",
    "rheumatoid_arthritis_collagen_vascular_diseases": "ck_elixhauserci_ra_cvd",  # noqa
}


class ElixhauserCIMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['ElixhauserCI'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        for colname in FIELDNAMES:
            constraint_name = CONSTRAINT_NAME_MAP.get(colname)
            setattr(
                cls, colname,
                BoolColumn(colname, comment="Disease present (0 no, 1 yes)",
                           constraint_name=constraint_name))
        super().__init__(name, bases, classdict)


class ElixhauserCI(TaskHasPatientMixin,
                   TaskHasClinicianMixin,
                   Task,
                   metaclass=ElixhauserCIMetaclass):
    __tablename__ = "elixhauserci"
    shortname = "ElixhauserCI"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Elixhauser Comorbidity Index")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total", coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (out of {MAX_SCORE})"
            ),
        ]

    def is_complete(self) -> bool:
        return self.all_fields_not_none(FIELDNAMES)

    def total_score(self) -> int:
        return self.count_booleans(FIELDNAMES)

    def get_task_html(self, req: CamcopsRequest) -> str:
        score = self.total_score()
        q_a = ""
        for f in FIELDNAMES:
            v = getattr(self, f)
            q_a += tr_qa(
                self.wxstring(req, f),
                get_yes_no_unknown(req, v)
            )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>{req.sstring(SS.TOTAL_SCORE)}</td>
                        <td><b>{score}</b> / {MAX_SCORE}</td>
                    </tr>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {q_a}
            </table>
        """
