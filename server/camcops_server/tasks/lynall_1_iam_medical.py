#!/usr/bin/env python

"""
camcops_server/tasks/lynall_1_iam_medical.py

===============================================================================

    Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

"""

from typing import Any, Dict, Tuple, Type

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import (
    bold,
    get_yes_no_none,
    tr_span_col,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BoolColumn,
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin


# =============================================================================
# Lynall1MedicalHistory
# =============================================================================

class Lynall1IamMedicalHistory(TaskHasPatientMixin, Task):
    """
    Server implementation of the Lynall_1_MedicalHistory task.
    """
    __tablename__ = "lynall_1_iam_medical"
    shortname = "Lynall_1_IAM_Medical"

    Q2_N_OPTIONS = 6
    Q3_N_OPTIONS = 11
    Q4_N_OPTIONS = 5
    Q4_OPTION_PSYCH_BEFORE_PHYSICAL = 1
    Q4_OPTION_PSYCH_AFTER_PHYSICAL = 2

    q1_age_first_inflammatory_sx = Column(
        "q1_age_first_inflammatory_sx", Integer,
        comment="Age (y) at onset of first symptoms of inflammatory disease"
    )
    q2_when_psych_sx_started = CamcopsColumn(
        "q2_when_psych_sx_started", Integer,
        permitted_value_checker=PermittedValueChecker(
            minimum=1, maximum=Q2_N_OPTIONS),
        comment="Timing of onset of psych symptoms (1 = NA, 2 = before "
                "physical symptoms [Sx], 3 = same time as physical Sx but "
                "before diagnosis [Dx], 4 = around time of Dx, 5 = weeks or "
                "months after Dx, 6 = years after Dx)"
    )
    q3_worst_symptom_last_month = CamcopsColumn(
        "q3_worst_symptom_last_month", Integer,
        permitted_value_checker=PermittedValueChecker(
            minimum=1, maximum=Q3_N_OPTIONS),
        comment="Worst symptom in last month (1 = fatigue, 2 = low mood, 3 = "
                "irritable, 4 = anxiety, 5 = brain fog/confused, 6 = pain, "
                "7 = bowel Sx, 8 = mobility, 9 = skin, 10 = other, 11 = no Sx "
                "in past month)"
    )
    q4a_symptom_timing = CamcopsColumn(
        "q4a_symptom_timing", Integer,
        permitted_value_checker=PermittedValueChecker(
            minimum=1, maximum=Q4_N_OPTIONS),
        comment="Timing of brain/psych Sx relative to physical Sx (1 = brain "
                "before physical, 2 = brain after physical, 3 = same time, "
                "4 = no relationship, 5 = none of the above)"
    )
    q4b_days_psych_before_phys = Column(
        "q4b_days_psych_before_phys", Integer,
        comment="If Q4a == 1, number of days that brain Sx typically begin "
                "before physical Sx"
    )
    q4c_days_psych_after_phys = Column(
        "q4c_days_psych_after_phys", Integer,
        comment="If Q4a == 2, number of days that brain Sx typically begin "
                "after physical Sx"
    )
    q5_antibiotics = BoolColumn(
        "q5_antibiotics",
        comment="Medication for infection (e.g. antibiotics) in past 3 months?"
                " (0 = no, 1 = yes)"
    )
    q6a_inpatient_last_y = BoolColumn(
        "q6a_inpatient_last_y",
        comment="Inpatient in the last year? (0 = no, 1 = yes)"
    )
    q6b_inpatient_weeks = Column(
        "q6b_inpatient_weeks", Integer,
        comment="If Q6a is true, approximate number of weeks spent as an "
                "inpatient in the past year"
    )
    q7a_sx_last_2y = BoolColumn(
        "q7a_sx_last_2y",
        comment="Symptoms within the last 2 years? (0 = no, 1 = yes)"
    )
    q7b_variability = Column(
        "q7b_variability", Integer,
        comment="If Q7a is true, degree of variability of symptoms (1-10 "
                "where 1 = highly variable [from none to severe], 10 = "
                "there all the time)"
    )
    q8_smoking = Column(
        "q8_smoking", Integer,
        comment="Current smoking status (0 = no, 1 = yes but not every day, "
                "2 = every day)"
    )
    q9_pregnant = BoolColumn(
        "q9_pregnant",
        comment="Currently pregnant (0 = no or N/A, 1 = yes)"
    )
    q10a_effective_rx_physical = Column(
        "q10a_effective_rx_physical", UnicodeText,
        comment="Most effective treatments for physical Sx"
    )
    q10b_effective_rx_psych = Column(
        "q10b_effective_rx_psych", UnicodeText,
        comment="Most effective treatments for brain/psychiatric Sx"
    )
    q11a_ph_depression = BoolColumn(
        "q11a_ph_depression",
        comment="Personal history of depression?"
    )
    q11b_ph_bipolar = BoolColumn(
        "q11b_ph_bipolar",
        comment="Personal history of bipolar disorder?"
    )
    q11c_ph_schizophrenia = BoolColumn(
        "q11c_ph_schizophrenia",
        comment="Personal history of schizophrenia?"
    )
    q11d_ph_autistic_spectrum = BoolColumn(
        "q11d_ph_autistic_spectrum",
        comment="Personal history of autism/Asperger's?"
    )
    q11e_ph_ptsd = BoolColumn(
        "q11e_ph_ptsd",
        comment="Personal history of PTSD?"
    )
    q11f_ph_other_anxiety = BoolColumn(
        "q11f_ph_other_anxiety",
        comment="Personal history of other anxiety disorders?"
    )
    q11g_ph_personality_disorder = BoolColumn(
        "q11g_ph_personality_disorder",
        comment="Personal history of personality disorder?"
    )
    q11h_ph_other_psych = BoolColumn(
        "q11h_ph_other_psych",
        comment="Personal history of other psychiatric disorder(s)?"
    )
    q11h_ph_other_detail = Column(
        "q11h_ph_other_detail", UnicodeText,
        comment="If q11h_ph_other_psych is true, this is the free-text "
                "details field"
    )
    q12a_fh_depression = BoolColumn(
        "q12a_fh_depression",
        comment="Family history of depression?"
    )
    q12b_fh_bipolar = BoolColumn(
        "q12b_fh_bipolar",
        comment="Family history of bipolar disorder?"
    )
    q12c_fh_schizophrenia = BoolColumn(
        "q12c_fh_schizophrenia",
        comment="Family history of schizophrenia?"
    )
    q12d_fh_autistic_spectrum = BoolColumn(
        "q12d_fh_autistic_spectrum",
        comment="Family history of autism/Asperger's?"
    )
    q12e_fh_ptsd = BoolColumn(
        "q12e_fh_ptsd",
        comment="Family history of PTSD?"
    )
    q12f_fh_other_anxiety = BoolColumn(
        "q12f_fh_other_anxiety",
        comment="Family history of other anxiety disorders?"
    )
    q12g_fh_personality_disorder = BoolColumn(
        "q12g_fh_personality_disorder",
        comment="Family history of personality disorder?"
    )
    q12h_fh_other_psych = BoolColumn(
        "q12h_fh_other_psych",
        comment="Family history of other psychiatric disorder(s)?"
    )
    q12h_fh_other_detail = Column(
        "q12h_fh_other_detail", UnicodeText,
        comment="If q12h_fh_other_psych is true, this is the free-text "
                "details field"
    )
    q13a_behcet = BoolColumn(
        "q13a_behcet",
        comment="Behçet’s syndrome? (0 = no, 1 = yes)"
    )
    q13b_oral_ulcers = BoolColumn(
        "q13b_oral_ulcers",
        comment="(If Behçet’s) Oral ulcers? (0 = no, 1 = yes)"
    )
    q13c_oral_age_first = Column(
        "q13c_oral_age_first", Integer,
        comment="(If Behçet’s + oral) Age (y) at first oral ulcers"
    )
    q13d_oral_scarring = BoolColumn(
        "q13d_oral_scarring",
        comment="(If Behçet’s + oral) Oral scarring? (0 = no, 1 = yes)"
    )
    q13e_genital_ulcers = BoolColumn(
        "q13e_genital_ulcers",
        comment="(If Behçet’s) Genital ulcers? (0 = no, 1 = yes)"
    )
    q13f_genital_age_first = Column(
        "q13f_genital_age_first", Integer,
        comment="(If Behçet’s + genital) Age (y) at first genital ulcers"
    )
    q13g_genital_scarring = BoolColumn(
        "q13g_genital_scarring",
        comment="(If Behçet’s + genital) Genital scarring? (0 = no, 1 = yes)"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Lynall M-E — 1 — IAM — Medical history")

    def is_complete(self) -> bool:
        if self.any_fields_none(["q1_age_first_inflammatory_sx",
                                 "q2_when_psych_sx_started",
                                 "q3_worst_symptom_last_month",
                                 "q4a_symptom_timing",
                                 "q5_antibiotics",
                                 "q6a_inpatient_last_y",
                                 "q7a_sx_last_2y",
                                 "q8_smoking",
                                 "q9_pregnant",
                                 "q10a_effective_rx_physical",
                                 "q10b_effective_rx_psych",
                                 "q13a_behcet"]):
            return False
        if self.any_fields_null_or_empty_str(["q10a_effective_rx_physical",
                                              "q10b_effective_rx_psych"]):
            return False
        q4a = self.q4a_symptom_timing
        if (q4a == self.Q4_OPTION_PSYCH_BEFORE_PHYSICAL and
                self.q4b_days_psych_before_phys is None):
            return False
        if (q4a == self.Q4_OPTION_PSYCH_AFTER_PHYSICAL and
                self.q4c_days_psych_after_phys is None):
            return False
        if self.q6a_inpatient_last_y and self.q6b_inpatient_weeks is None:
            return False
        if self.q7a_sx_last_2y and self.q7b_variability is None:
            return False
        if self.q11h_ph_other_psych and not self.q11h_ph_other_detail:
            return False
        if self.q12h_fh_other_psych and not self.q12h_fh_other_detail:
            return False
        if self.q13a_behcet:
            if self.any_fields_none(["q13b_oral_ulcers",
                                     "q13e_genital_ulcers"]):
                return False
            if self.q13b_oral_ulcers:
                if self.any_fields_none(["q13c_oral_age_first",
                                         "q13d_oral_scarring"]):
                    return False
            if self.q13e_genital_ulcers:
                if self.any_fields_none(["q13f_genital_age_first",
                                         "q13g_genital_scarring"]):
                    return False
        return True

    def get_task_html(self, req: CamcopsRequest) -> str:
        return "" # ***
