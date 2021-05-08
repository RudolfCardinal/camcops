#!/usr/bin/env python

"""
camcops_server/tasks/mds_updrs.py

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

from typing import List

from sqlalchemy.sql.sqltypes import Boolean, Float, Integer

from camcops_server.cc_modules.cc_cache import cache_region_static, fkg
from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_ONLY_DIV,
)
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    gen_camcops_columns,
    get_camcops_column_attr_names,
    ZERO_TO_TWO_CHECKER,
    ZERO_TO_FOUR_CHECKER,
    ZERO_TO_FIVE_CHECKER,
)
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
    TaskHasClinicianMixin,
)


# =============================================================================
# MDS-UPDRS (crippled)
# =============================================================================

class MdsUpdrs(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the MDS-UPDRS task.
    """
    __tablename__ = "mds_updrs"
    shortname = "MDS-UPDRS"
    # Has clinician as of v2.0.0

    main_cmt = " (0 normal, 1 slight, 2 mild, 3 moderate, 4 severe)"
    main_pv = ZERO_TO_FOUR_CHECKER
    informant_cmt = " (0 patient, 1 caregiver, 2 both)"
    informant_pv = ZERO_TO_TWO_CHECKER
    yn_cmt = " (0 no, 1 yes)"
    on_off_cmt = " (0 off, 1 on)"
    hy_pv = ZERO_TO_FIVE_CHECKER

    # Part I
    q1a = CamcopsColumn(
        "q1a", Integer, permitted_value_checker=informant_pv,
        comment="Part I: informant for Q1.1-1.6" + informant_cmt
    )
    q1_1 = CamcopsColumn(
        "q1_1", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.1 " + main_cmt
    )
    q1_2 = CamcopsColumn(
        "q1_2", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.2 " + main_cmt
    )
    q1_3 = CamcopsColumn(
        "q1_3", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.3 " + main_cmt
    )
    q1_4 = CamcopsColumn(
        "q1_4", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.4 " + main_cmt
    )
    q1_5 = CamcopsColumn(
        "q1_5", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.5 " + main_cmt
    )
    q1_6 = CamcopsColumn(
        "q1_6", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.6 " + main_cmt
    )
    q1_6a = CamcopsColumn(
        "q1_6a", Integer, permitted_value_checker=informant_pv,
        comment="Part I, Q1.6a: informant for Q1.7-1.13" + informant_cmt
    )
    q1_7 = CamcopsColumn(
        "q1_7", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.7 " + main_cmt
    )
    q1_8 = CamcopsColumn(
        "q1_8", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.8 " + main_cmt
    )
    q1_9 = CamcopsColumn(
        "q1_9", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.9 " + main_cmt
    )
    q1_10 = CamcopsColumn(
        "q1_10", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.10 " + main_cmt
    )
    q1_11 = CamcopsColumn(
        "q1_11", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.11 " + main_cmt
    )
    q1_12 = CamcopsColumn(
        "q1_12", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.12 " + main_cmt
    )
    q1_13 = CamcopsColumn(
        "q1_13", Integer, permitted_value_checker=main_pv,
        comment="Part I, Q1.13 " + main_cmt
    )

    # Part II
    q2_1 = CamcopsColumn(
        "q2_1", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.1 " + main_cmt
    )
    q2_2 = CamcopsColumn(
        "q2_2", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.2 " + main_cmt
    )
    q2_3 = CamcopsColumn(
        "q2_3", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.3 " + main_cmt
    )
    q2_4 = CamcopsColumn(
        "q2_4", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.4 " + main_cmt
    )
    q2_5 = CamcopsColumn(
        "q2_5", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.5 " + main_cmt
    )
    q2_6 = CamcopsColumn(
        "q2_6", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.6 " + main_cmt
    )
    q2_7 = CamcopsColumn(
        "q2_7", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.7 " + main_cmt
    )
    q2_8 = CamcopsColumn(
        "q2_8", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.8 " + main_cmt
    )
    q2_9 = CamcopsColumn(
        "q2_9", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.9 " + main_cmt
    )
    q2_10 = CamcopsColumn(
        "q2_10", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.10 " + main_cmt
    )
    q2_11 = CamcopsColumn(
        "q2_11", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.11 " + main_cmt
    )
    q2_12 = CamcopsColumn(
        "q2_12", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.12 " + main_cmt
    )
    q2_13 = CamcopsColumn(
        "q2_13", Integer, permitted_value_checker=main_pv,
        comment="Part II, Q2.13 " + main_cmt
    )

    # Part III
    q3a = CamcopsColumn(
        "q3a", Boolean, permitted_value_checker=BIT_CHECKER,
        comment="Part III, Q3a (medication) " + yn_cmt
    )
    q3b = CamcopsColumn(
        "q3b", Boolean, permitted_value_checker=BIT_CHECKER,
        comment="Part III, Q3b (clinical state) " + on_off_cmt
    )
    q3c = CamcopsColumn(
        "q3c", Boolean, permitted_value_checker=BIT_CHECKER,
        comment="Part III, Q3c (levodopa) " + yn_cmt
    )
    q3c1 = CamcopsColumn(
        "q3c1", Float,
        comment="Part III, Q3c.1 (minutes since last dose)"
    )
    q3_1 = CamcopsColumn(
        "q3_1", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.1 " + main_cmt
    )
    q3_2 = CamcopsColumn(
        "q3_2", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.2 " + main_cmt
    )
    q3_3a = CamcopsColumn(
        "q3_3a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.3a " + main_cmt
    )
    q3_3b = CamcopsColumn(
        "q3_3b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.3b " + main_cmt
    )
    q3_3c = CamcopsColumn(
        "q3_3c", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.3c " + main_cmt
    )
    q3_3d = CamcopsColumn(
        "q3_3d", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.3d " + main_cmt
    )
    q3_3e = CamcopsColumn(
        "q3_3e", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.3e " + main_cmt
    )
    q3_4a = CamcopsColumn(
        "q3_4a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.4a " + main_cmt
    )
    q3_4b = CamcopsColumn(
        "q3_4b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.4b " + main_cmt
    )
    q3_5a = CamcopsColumn(
        "q3_5a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.5a " + main_cmt
    )
    q3_5b = CamcopsColumn(
        "q3_5b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.5b " + main_cmt
    )
    q3_6a = CamcopsColumn(
        "q3_6a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.6a " + main_cmt
    )
    q3_6b = CamcopsColumn(
        "q3_6b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.6b " + main_cmt
    )
    q3_7a = CamcopsColumn(
        "q3_7a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.7a " + main_cmt
    )
    q3_7b = CamcopsColumn(
        "q3_7b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.7b " + main_cmt
    )
    q3_8a = CamcopsColumn(
        "q3_8a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.8a " + main_cmt
    )
    q3_8b = CamcopsColumn(
        "q3_8b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.8b " + main_cmt
    )
    q3_9 = CamcopsColumn(
        "q3_9", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.9 " + main_cmt
    )
    q3_10 = CamcopsColumn(
        "q3_10", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.10 " + main_cmt
    )
    q3_11 = CamcopsColumn(
        "q3_11", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.11 " + main_cmt
    )
    q3_12 = CamcopsColumn(
        "q3_12", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.12 " + main_cmt
    )
    q3_13 = CamcopsColumn(
        "q3_13", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.13 " + main_cmt
    )
    q3_14 = CamcopsColumn(
        "q3_14", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.14 " + main_cmt
    )
    q3_15a = CamcopsColumn(
        "q3_15a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.15a " + main_cmt
    )
    q3_15b = CamcopsColumn(
        "q3_15b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.15b " + main_cmt
    )
    q3_16a = CamcopsColumn(
        "q3_16a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.16a " + main_cmt
    )
    q3_16b = CamcopsColumn(
        "q3_16b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.16b " + main_cmt
    )
    q3_17a = CamcopsColumn(
        "q3_17a", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.17a " + main_cmt
    )
    q3_17b = CamcopsColumn(
        "q3_17b", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.17b " + main_cmt
    )
    q3_17c = CamcopsColumn(
        "q3_17c", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.17c " + main_cmt
    )
    q3_17d = CamcopsColumn(
        "q3_17d", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.17d " + main_cmt
    )
    q3_17e = CamcopsColumn(
        "q3_17e", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.17e " + main_cmt
    )
    q3_18 = CamcopsColumn(
        "q3_18", Integer, permitted_value_checker=main_pv,
        comment="Part III, Q3.18 " + main_cmt
    )
    q3_dyskinesia_present = CamcopsColumn(
        "q3_dyskinesia_present", Boolean, permitted_value_checker=BIT_CHECKER,
        comment="Part III, q3_dyskinesia_present " + yn_cmt
    )
    q3_dyskinesia_interfered = CamcopsColumn(
        "q3_dyskinesia_interfered", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Part III, q3_dyskinesia_interfered " + yn_cmt
    )
    q3_hy_stage = CamcopsColumn(
        "q3_hy_stage", Integer, permitted_value_checker=hy_pv,
        comment="Part III, q3_hy_stage (0-5)"
    )

    # Part IV
    q4_1 = CamcopsColumn(
        "q4_1", Integer, permitted_value_checker=main_pv,
        comment="Part IV, Q4.1 " + main_cmt
    )
    q4_2 = CamcopsColumn(
        "q4_2", Integer, permitted_value_checker=main_pv,
        comment="Part IV, Q4.2 " + main_cmt
    )
    q4_3 = CamcopsColumn(
        "q4_3", Integer, permitted_value_checker=main_pv,
        comment="Part IV, Q4.3 " + main_cmt
    )
    q4_4 = CamcopsColumn(
        "q4_4", Integer, permitted_value_checker=main_pv,
        comment="Part IV, Q4.4 " + main_cmt
    )
    q4_5 = CamcopsColumn(
        "q4_5", Integer, permitted_value_checker=main_pv,
        comment="Part IV, Q4.5 " + main_cmt
    )
    q4_6 = CamcopsColumn(
        "q4_6", Integer, permitted_value_checker=main_pv,
        comment="Part IV, Q4.6 " + main_cmt
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "Movement Disorder Society-Sponsored Revision of the Unified "
            "Parkinson’s Disease Rating Scale (data collection only)"
        )

    @classmethod
    @cache_region_static.cache_on_arguments(function_key_generator=fkg)
    def task_fields_except_3c1(cls) -> List[str]:
        task_fields = get_camcops_column_attr_names(cls)
        return [x for x in task_fields if x != "q3c1"]

    def is_complete(self) -> bool:
        return (
            self.field_contents_valid() and
            self.all_fields_not_none(self.task_fields_except_3c1()) and
            (self.q3c1 is not None or not self.q3c)
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        q_a = ""
        for attrname, column in gen_camcops_columns(self):
            if attrname.startswith("clinician_"):  # not the most elegant!
                continue
            question = column.comment
            value = getattr(self, attrname)
            q_a += tr_qa(question, value)
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="70%">Question</th>
                    <th width="30%">Answer</th>
                </tr>
                {q_a}
            </table>
            {DATA_COLLECTION_ONLY_DIV}
        """

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [SnomedExpression(req.snomed(SnomedLookup.UPDRS_SCALE))]
