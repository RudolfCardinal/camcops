#!/usr/bin/env python

"""
camcops_server/tasks/cpft_lps.py

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

import logging
from typing import Any, List, Optional, Type

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.logs import BraceStyleAdapter
import cardinal_pythonlib.rnc_web as ws
import pyramid.httpexceptions as exc
from sqlalchemy.sql.expression import and_, exists, select
from sqlalchemy.sql.selectable import SelectBase
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Date, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DateFormat,
    INVALID_VALUE,
)
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo
from camcops_server.cc_modules.cc_forms import (
    LinkingIdNumSelector,
    ReportParamSchema,
)
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no_none,
    subheading_spanning_four_columns,
    subheading_spanning_two_columns,
    tr_qa,
    tr_span_col,
)
from camcops_server.cc_modules.cc_nhs import (
    get_nhs_dd_ethnic_category_code,
    get_nhs_dd_person_marital_status,
    PV_NHS_ETHNIC_CATEGORY,
    PV_NHS_MARITAL_STATUS
)
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_pyramid import ViewParam
from camcops_server.cc_modules.cc_report import Report
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BoolColumn,
    CamcopsColumn,
    CharColType,
    PendulumDateTimeAsIsoTextColType,
    DiagnosticCodeColType,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.tasks.psychiatricclerking import PsychiatricClerking

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# CPFT_LPS_Referral
# =============================================================================

class CPFTLPSReferral(TaskHasPatientMixin, Task):
    """
    Server implementation of the CPFT_LPS_Referral task.
    """
    __tablename__ = "cpft_lps_referral"
    shortname = "CPFT_LPS_Referral"

    referral_date_time = Column("referral_date_time",
                                PendulumDateTimeAsIsoTextColType)
    lps_division = CamcopsColumn(
        "lps_division", UnicodeText,
        exempt_from_anonymisation=True)
    referral_priority = CamcopsColumn(
        "referral_priority", UnicodeText,
        exempt_from_anonymisation=True)
    referral_method = CamcopsColumn(
        "referral_method", UnicodeText,
        exempt_from_anonymisation=True)
    referrer_name = Column("referrer_name", UnicodeText)
    referrer_contact_details = Column("referrer_contact_details", UnicodeText)
    referring_consultant = Column("referring_consultant", UnicodeText)
    referring_specialty = CamcopsColumn(
        "referring_specialty", UnicodeText,
        exempt_from_anonymisation=True)
    referring_specialty_other = Column("referring_specialty_other", UnicodeText)
    patient_location = Column("patient_location", UnicodeText)
    admission_date = Column("admission_date", Date)
    estimated_discharge_date = Column("estimated_discharge_date", Date)
    patient_aware_of_referral = BoolColumn("patient_aware_of_referral")
    interpreter_required = BoolColumn("interpreter_required")
    sensory_impairment = BoolColumn("sensory_impairment")
    marital_status_code = CamcopsColumn(
        "marital_status_code", CharColType,
        permitted_value_checker=PermittedValueChecker(
            permitted_values=PV_NHS_MARITAL_STATUS)
    )
    ethnic_category_code = CamcopsColumn(
        "ethnic_category_code", CharColType,
        permitted_value_checker=PermittedValueChecker(
            permitted_values=PV_NHS_ETHNIC_CATEGORY)
    )
    admission_reason_overdose = BoolColumn("admission_reason_overdose")
    admission_reason_self_harm_not_overdose = BoolColumn(
        "admission_reason_self_harm_not_overdose",
        constraint_name="ck_cpft_lps_referral_arshno"
    )
    admission_reason_confusion = BoolColumn("admission_reason_confusion")
    admission_reason_trauma = BoolColumn("admission_reason_trauma")
    admission_reason_falls = BoolColumn("admission_reason_falls")
    admission_reason_infection = BoolColumn("admission_reason_infection")
    admission_reason_poor_adherence = BoolColumn(
        "admission_reason_poor_adherence",
        constraint_name="ck_cpft_lps_referral_adpa"
    )
    admission_reason_other = BoolColumn("admission_reason_other")
    existing_psychiatric_teams = Column("existing_psychiatric_teams",
                                        UnicodeText)
    care_coordinator = Column("care_coordinator", UnicodeText)
    other_contact_details = Column("other_contact_details", UnicodeText)
    referral_reason = Column("referral_reason", UnicodeText)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("CPFT LPS – referral")

    def is_complete(self) -> bool:
        return bool(
            self.patient_location and
            self.referral_reason and
            self.field_contents_valid()
        )

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        return [CtvInfo(
            heading=ws.webify(self.wxstring(req, "f_referral_reason_t")),
            content=self.referral_reason
        )]

    @staticmethod
    def four_column_row(q1: str, a1: Any,
                        q2: str, a2: Any,
                        default: str = "") -> str:
        return f"""
            <tr>
                <td>{q1}</td><td>{answer(a1, default=default)}</td>
                <td>{q2}</td><td>{answer(a2, default=default)}</td>
            </tr>
        """

    @staticmethod
    def tr_qa(q: str, a: Any, default: str = "") -> str:
        return f"""
            <tr>
                <td colspan="2">{q}</td>
                <td colspan="2"><b>{default if a is None else a}</b></td>
            </tr>
        """

    def get_task_html(self, req: CamcopsRequest) -> str:
        person_marital_status = get_nhs_dd_person_marital_status(req)
        ethnic_category_code = get_nhs_dd_ethnic_category_code(req)
        if self.lps_division == "G":
            banner_class = CssClass.BANNER_REFERRAL_GENERAL_ADULT
            division_name = self.wxstring(req, "service_G")
        elif self.lps_division == "O":
            banner_class = CssClass.BANNER_REFERRAL_OLD_AGE
            division_name = self.wxstring(req, "service_O")
        elif self.lps_division == "S":
            banner_class = CssClass.BANNER_REFERRAL_SUBSTANCE_MISUSE
            division_name = self.wxstring(req, "service_S")
        else:
            banner_class = ""
            division_name = None

        if self.referral_priority == "R":
            priority_name = self.wxstring(req, "priority_R")
        elif self.referral_priority == "U":
            priority_name = self.wxstring(req, "priority_U")
        elif self.referral_priority == "E":
            priority_name = self.wxstring(req, "priority_E")
        else:
            priority_name = None

        potential_admission_reasons = [
            "admission_reason_overdose",
            "admission_reason_self_harm_not_overdose",
            "admission_reason_confusion",
            "admission_reason_trauma",
            "admission_reason_falls",
            "admission_reason_infection",
            "admission_reason_poor_adherence",
            "admission_reason_other",
        ]
        admission_reasons = []
        for r in potential_admission_reasons:
            if getattr(self, r):
                admission_reasons.append(self.wxstring(req, "f_" + r))

        h = f"""
            <div class="{CssClass.BANNER} {banner_class}">
                {answer(division_name, default_for_blank_strings=True)}
                referral at {
                    answer(format_datetime(
                        self.referral_date_time,
                        DateFormat.SHORT_DATETIME_WITH_DAY_NO_TZ,
                        default=None))}
            </div>
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <col width="25%">
                <col width="25%">
                <col width="25%">
                <col width="25%">
        """
        h += subheading_spanning_four_columns(
            self.wxstring(req, "t_about_referral"))
        h += """
            <tr>
                <td>{q_method}</td>
                <td>{a_method}</td>
                <td>{q_priority}</td>
                <td class="{CssClass.HIGHLIGHT}">{a_priority}</td>
            </tr>
        """.format(
            CssClass=CssClass,
            q_method=self.wxstring(req, "f_referral_method"),
            a_method=answer(self.referral_method),
            q_priority=self.wxstring(req, "f_referral_priority"),
            a_priority=(
                answer(self.referral_priority, default_for_blank_strings=True) +  # noqa
                ": " + answer(priority_name)
            )
        )
        h += self.four_column_row(
            self.wxstring(req, "f_referrer_name"),
            self.referrer_name,
            self.wxstring(req, "f_referring_specialty"),
            self.referring_specialty
        )
        h += self.four_column_row(
            self.wxstring(req, "f_referrer_contact_details"),
            self.referrer_contact_details,
            self.wxstring(req, "f_referring_specialty_other"),
            self.referring_specialty_other
        )
        h += self.four_column_row(
            self.wxstring(req, "f_referring_consultant"),
            self.referring_consultant,
            "",
            ""
        )
        h += subheading_spanning_four_columns(
            self.wxstring(req, "t_patient"))
        h += """
            <tr>
                <td>{q_when}</td>
                <td>{a_when}</td>
                <td>{q_where}</td>
                <td class="{CssClass.HIGHLIGHT}">{a_where}</td>
            </tr>
        """.format(
            CssClass=CssClass,
            q_when=self.wxstring(req, "f_admission_date"),
            a_when=answer(
                format_datetime(self.admission_date, DateFormat.LONG_DATE,
                                default=None), ""),
            q_where=self.wxstring(req, "f_patient_location"),
            a_where=answer(self.patient_location),
        )
        h += self.four_column_row(
            self.wxstring(req, "f_estimated_discharge_date"),
            format_datetime(self.estimated_discharge_date,
                            DateFormat.LONG_DATE, ""),
            self.wxstring(req, "f_patient_aware_of_referral"),
            get_yes_no_none(req, self.patient_aware_of_referral)
        )
        h += self.four_column_row(
            self.wxstring(req, "f_marital_status"),
            person_marital_status.get(self.marital_status_code, INVALID_VALUE),
            self.wxstring(req, "f_interpreter_required"),
            get_yes_no_none(req, self.interpreter_required)
        )
        h += self.four_column_row(
            self.wxstring(req, "f_ethnic_category"),
            ethnic_category_code.get(self.ethnic_category_code, INVALID_VALUE),
            self.wxstring(req, "f_sensory_impairment"),
            get_yes_no_none(req, self.sensory_impairment)
        )
        h += subheading_spanning_four_columns(
            self.wxstring(req, "t_admission_reason"))
        h += tr_span_col(answer(", ".join(admission_reasons), ""), cols=4)
        h += subheading_spanning_four_columns(
            self.wxstring(req, "t_other_people"))
        h += self.tr_qa(
            self.wxstring(req, "f_existing_psychiatric_teams"),
            self.existing_psychiatric_teams, "")
        h += self.tr_qa(
            self.wxstring(req, "f_care_coordinator"),
            self.care_coordinator, "")
        h += self.tr_qa(
            self.wxstring(req, "f_other_contact_details"),
            self.other_contact_details, "")
        h += subheading_spanning_four_columns(
            self.wxstring(req, "t_referral_reason"))
        h += tr_span_col(answer(self.referral_reason, ""), cols=4)
        h += """
            </table>
        """
        return h


# =============================================================================
# CPFT_LPS_ResetResponseClock
# =============================================================================

class CPFTLPSResetResponseClock(TaskHasPatientMixin, TaskHasClinicianMixin,
                                Task):
    """
    Server implementation of the CPFT_LPS_ResetResponseClock task.
    """
    __tablename__ = "cpft_lps_resetresponseclock"
    shortname = "CPFT_LPS_ResetResponseClock"

    reset_start_time_to = Column(
        "reset_start_time_to", PendulumDateTimeAsIsoTextColType
    )
    reason = Column("reason", UnicodeText)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("CPFT LPS – reset response clock")

    def is_complete(self) -> bool:
        return bool(
            self.reset_start_time_to and
            self.reason and
            self.field_contents_valid()
        )

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        return [CtvInfo(content=self.reason)]

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <col width="25%">
                <col width="75%">
        """
        h += tr_qa(
            self.wxstring(req, "to"),
            format_datetime(self.reset_start_time_to,
                            DateFormat.LONG_DATETIME_WITH_DAY,
                            default=None))
        h += tr_qa(self.wxstring(req, "reason"), self.reason)
        h += """
            </table>
        """
        return h


# =============================================================================
# CPFT_LPS_Discharge
# =============================================================================

class CPFTLPSDischarge(TaskHasPatientMixin, TaskHasClinicianMixin, Task):
    """
    Server implementation of the CPFT_LPS_Discharge task.
    """
    __tablename__ = "cpft_lps_discharge"
    shortname = "CPFT_LPS_Discharge"

    discharge_date = Column("discharge_date", Date)
    discharge_reason_code = CamcopsColumn(
        "discharge_reason_code", UnicodeText,
        exempt_from_anonymisation=True)

    leaflet_or_discharge_card_given = BoolColumn(
        "leaflet_or_discharge_card_given",
        constraint_name="ck_cpft_lps_discharge_lodcg"
    )
    frequent_attender = BoolColumn("frequent_attender")
    patient_wanted_copy_of_letter = BoolColumn(
        # Was previously text! That wasn't right.
        "patient_wanted_copy_of_letter",
    )
    gaf_at_first_assessment = CamcopsColumn(
        "gaf_at_first_assessment", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=100)
    )
    gaf_at_discharge = CamcopsColumn(
        "gaf_at_discharge", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=100)
    )

    referral_reason_self_harm_overdose = BoolColumn(
        "referral_reason_self_harm_overdose",
        constraint_name="ck_cpft_lps_discharge_rrshoverdose"
    )
    referral_reason_self_harm_other = BoolColumn(
        "referral_reason_self_harm_other",
        constraint_name="ck_cpft_lps_discharge_rrshother"
    )
    referral_reason_suicidal_ideas = BoolColumn(
        "referral_reason_suicidal_ideas",
        constraint_name="ck_cpft_lps_discharge_rrsuicidal"
    )
    referral_reason_behavioural_disturbance = BoolColumn(
        "referral_reason_behavioural_disturbance",
        constraint_name="ck_cpft_lps_discharge_behavdisturb"
    )
    referral_reason_low_mood = BoolColumn("referral_reason_low_mood")
    referral_reason_elevated_mood = BoolColumn("referral_reason_elevated_mood")
    referral_reason_psychosis = BoolColumn("referral_reason_psychosis")
    referral_reason_pre_transplant = BoolColumn(
        "referral_reason_pre_transplant",
        constraint_name="ck_cpft_lps_discharge_pretransplant"
    )
    referral_reason_post_transplant = BoolColumn(
        "referral_reason_post_transplant",
        constraint_name="ck_cpft_lps_discharge_posttransplant"
    )
    referral_reason_delirium = BoolColumn("referral_reason_delirium")
    referral_reason_anxiety = BoolColumn("referral_reason_anxiety")
    referral_reason_somatoform_mus = BoolColumn(
        "referral_reason_somatoform_mus",
        constraint_name="ck_cpft_lps_discharge_mus"
    )
    referral_reason_motivation_adherence = BoolColumn(
        "referral_reason_motivation_adherence",
        constraint_name="ck_cpft_lps_discharge_motivadherence"
    )
    referral_reason_capacity = BoolColumn("referral_reason_capacity")
    referral_reason_eating_disorder = BoolColumn(
        "referral_reason_eating_disorder",
        constraint_name="ck_cpft_lps_discharge_eatingdis"
    )
    referral_reason_safeguarding = BoolColumn("referral_reason_safeguarding")
    referral_reason_discharge_placement = BoolColumn(
        "referral_reason_discharge_placement",
        constraint_name="ck_cpft_lps_discharge_dcplacement"
    )
    referral_reason_cognitive_problem = BoolColumn(
        "referral_reason_cognitive_problem",
        constraint_name="ck_cpft_lps_discharge_cognitiveprob"
    )
    referral_reason_substance_alcohol = BoolColumn(
        "referral_reason_substance_alcohol",
        constraint_name="ck_cpft_lps_discharge_alcohol"
    )
    referral_reason_substance_other = BoolColumn(
        "referral_reason_substance_other",
        constraint_name="ck_cpft_lps_discharge_substanceother"
    )
    referral_reason_other = BoolColumn("referral_reason_other")
    referral_reason_transplant_organ = CamcopsColumn(
        "referral_reason_transplant_organ", UnicodeText,
        exempt_from_anonymisation=True
    )
    referral_reason_other_detail = Column(
        "referral_reason_other_detail", UnicodeText
    )

    diagnosis_no_active_mental_health_problem = BoolColumn(
        "diagnosis_no_active_mental_health_problem",
        constraint_name="ck_cpft_lps_discharge_nomhprob"
    )
    diagnosis_psych_1_icd10code = Column(
        "diagnosis_psych_1_icd10code", DiagnosticCodeColType
    )
    diagnosis_psych_1_description = CamcopsColumn(
        "diagnosis_psych_1_description", UnicodeText,
        exempt_from_anonymisation=True
    )
    diagnosis_psych_2_icd10code = Column(
        "diagnosis_psych_2_icd10code", DiagnosticCodeColType
    )
    diagnosis_psych_2_description = CamcopsColumn(
        "diagnosis_psych_2_description", UnicodeText,
        exempt_from_anonymisation=True
    )
    diagnosis_psych_3_icd10code = Column(
        "diagnosis_psych_3_icd10code", DiagnosticCodeColType
    )
    diagnosis_psych_3_description = CamcopsColumn(
        "diagnosis_psych_3_description", UnicodeText,
        exempt_from_anonymisation=True
    )
    diagnosis_psych_4_icd10code = Column(
        "diagnosis_psych_4_icd10code", DiagnosticCodeColType
    )
    diagnosis_psych_4_description = CamcopsColumn(
        "diagnosis_psych_4_description", UnicodeText,
        exempt_from_anonymisation=True
    )
    diagnosis_medical_1 = Column("diagnosis_medical_1", UnicodeText)
    diagnosis_medical_2 = Column("diagnosis_medical_2", UnicodeText)
    diagnosis_medical_3 = Column("diagnosis_medical_3", UnicodeText)
    diagnosis_medical_4 = Column("diagnosis_medical_4", UnicodeText)

    management_assessment_diagnostic = BoolColumn(
        "management_assessment_diagnostic",
        constraint_name="ck_cpft_lps_discharge_mx_ass_diag"
    )
    management_medication = BoolColumn("management_medication")
    management_specialling_behavioural_disturbance = BoolColumn(
        "management_specialling_behavioural_disturbance",
        # Constraint name too long for MySQL unless we do this:
        constraint_name="ck_cpft_lps_discharge_msbd"
    )
    management_supportive_patient = BoolColumn("management_supportive_patient")
    management_supportive_carers = BoolColumn("management_supportive_carers")
    management_supportive_staff = BoolColumn("management_supportive_staff")
    management_nursing_management = BoolColumn("management_nursing_management")
    management_therapy_cbt = BoolColumn("management_therapy_cbt")
    management_therapy_cat = BoolColumn("management_therapy_cat")
    management_therapy_other = BoolColumn("management_therapy_other")
    management_treatment_adherence = BoolColumn(
        "management_treatment_adherence",
        constraint_name="ck_cpft_lps_discharge_mx_rx_adhere"
    )
    management_capacity = BoolColumn("management_capacity")
    management_education_patient = BoolColumn("management_education_patient")
    management_education_carers = BoolColumn("management_education_carers")
    management_education_staff = BoolColumn("management_education_staff")
    management_accommodation_placement = BoolColumn(
        "management_accommodation_placement",
        constraint_name="ck_cpft_lps_discharge_accom"
    )
    management_signposting_external_referral = BoolColumn(
        "management_signposting_external_referral",
        constraint_name="ck_cpft_lps_discharge_mx_signpostrefer"
    )
    management_mha_s136 = BoolColumn("management_mha_s136")
    management_mha_s5_2 = BoolColumn("management_mha_s5_2")
    management_mha_s2 = BoolColumn("management_mha_s2")
    management_mha_s3 = BoolColumn("management_mha_s3")
    management_complex_case_conference = BoolColumn(
        "management_complex_case_conference",
        constraint_name="ck_cpft_lps_discharge_caseconf"
    )
    management_other = BoolColumn("management_other")
    management_other_detail = Column("management_other_detail", UnicodeText)

    outcome = CamcopsColumn(
        "outcome", UnicodeText,
        exempt_from_anonymisation=True
    )
    outcome_hospital_transfer_detail = Column(
        "outcome_hospital_transfer_detail", UnicodeText
    )
    outcome_other_detail = Column("outcome_other_detail", UnicodeText)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("CPFT LPS – discharge")

    def is_complete(self) -> bool:
        return bool(
            self.discharge_date and
            self.discharge_reason_code and
            # self.outcome and  # v2.0.0
            self.field_contents_valid()
        )

    def get_discharge_reason(self, req: CamcopsRequest) -> Optional[str]:
        if self.discharge_reason_code == "F":
            return self.wxstring(req, "reason_code_F")
        elif self.discharge_reason_code == "A":
            return self.wxstring(req, "reason_code_A")
        elif self.discharge_reason_code == "O":
            return self.wxstring(req, "reason_code_O")
        elif self.discharge_reason_code == "C":
            return self.wxstring(req, "reason_code_C")
        else:
            return None

    def get_referral_reasons(self, req: CamcopsRequest) -> List[str]:
        potential_referral_reasons = [
            "referral_reason_self_harm_overdose",
            "referral_reason_self_harm_other",
            "referral_reason_suicidal_ideas",
            "referral_reason_behavioural_disturbance",
            "referral_reason_low_mood",
            "referral_reason_elevated_mood",
            "referral_reason_psychosis",
            "referral_reason_pre_transplant",
            "referral_reason_post_transplant",
            "referral_reason_delirium",
            "referral_reason_anxiety",
            "referral_reason_somatoform_mus",
            "referral_reason_motivation_adherence",
            "referral_reason_capacity",
            "referral_reason_eating_disorder",
            "referral_reason_safeguarding",
            "referral_reason_discharge_placement",
            "referral_reason_cognitive_problem",
            "referral_reason_substance_alcohol",
            "referral_reason_substance_other",
            "referral_reason_other",
        ]
        referral_reasons = []
        for r in potential_referral_reasons:
            if getattr(self, r):
                referral_reasons.append(self.wxstring(req, "" + r))
        return referral_reasons

    def get_managements(self, req: CamcopsRequest) -> List[str]:
        potential_managements = [
            "management_assessment_diagnostic",
            "management_medication",
            "management_specialling_behavioural_disturbance",
            "management_supportive_patient",
            "management_supportive_carers",
            "management_supportive_staff",
            "management_nursing_management",
            "management_therapy_cbt",
            "management_therapy_cat",
            "management_therapy_other",
            "management_treatment_adherence",
            "management_capacity",
            "management_education_patient",
            "management_education_carers",
            "management_education_staff",
            "management_accommodation_placement",
            "management_signposting_external_referral",
            "management_mha_s136",
            "management_mha_s5_2",
            "management_mha_s2",
            "management_mha_s3",
            "management_complex_case_conference",
            "management_other",
        ]
        managements = []
        for r in potential_managements:
            if getattr(self, r):
                managements.append(self.wxstring(req, "" + r))
        return managements

    def get_psychiatric_diagnoses(self, req: CamcopsRequest) -> List[str]:
        psychiatric_diagnoses = [
            self.wxstring(req, "diagnosis_no_active_mental_health_problem")
        ] if self.diagnosis_no_active_mental_health_problem else []
        for i in range(1, 4 + 1):  # magic number
            if getattr(self, "diagnosis_psych_" + str(i) + "_icd10code"):
                psychiatric_diagnoses.append(
                    ws.webify(getattr(self, "diagnosis_psych_" +
                                      str(i) + "_icd10code")) +
                    " – " +
                    ws.webify(getattr(self, "diagnosis_psych_" +
                                      str(i) + "_description"))
                )
        return psychiatric_diagnoses

    def get_medical_diagnoses(self) -> List[str]:
        medical_diagnoses = []
        for i in range(1, 4 + 1):  # magic number
            if getattr(self, "diagnosis_medical_" + str(i)):
                medical_diagnoses.append(
                    ws.webify(getattr(self, "diagnosis_medical_" + str(i))))
        return medical_diagnoses

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        diagnoses = self.get_psychiatric_diagnoses(req) + \
            self.get_medical_diagnoses()
        return [
            CtvInfo(
                heading=ws.webify(self.wxstring(req, "discharge_reason")),
                content=self.get_discharge_reason(req)
            ),
            CtvInfo(
                heading=ws.webify(
                    self.wxstring(req, "referral_reason_t")),
                content=", ".join(self.get_referral_reasons(req))
            ),
            CtvInfo(
                heading=ws.webify(self.wxstring(req, "diagnoses_t")),
                content=", ".join(diagnoses)
            ),
            CtvInfo(
                heading=ws.webify(self.wxstring(req, "management_t")),
                content=", ".join(self.get_managements(req))
            ),
            CtvInfo(
                heading=ws.webify(self.wxstring(req, "outcome_t")),
                content=self.outcome
            ),
        ]

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <col width="40%">
                <col width="60%">
        """
        h += tr_qa(self.wxstring(req, "discharge_date"),
                   format_datetime(self.discharge_date,
                                   DateFormat.LONG_DATE_WITH_DAY,
                                   default=None), "")
        h += tr_qa(self.wxstring(req, "discharge_reason"),
                   self.get_discharge_reason(req), "")
        h += tr_qa(self.wxstring(req, "leaflet_or_discharge_card_given"),
                   get_yes_no_none(req, self.leaflet_or_discharge_card_given),
                   "")
        h += tr_qa(self.wxstring(req, "frequent_attender"),
                   get_yes_no_none(req, self.frequent_attender), "")
        h += tr_qa(self.wxstring(req, "patient_wanted_copy_of_letter"),
                   self.patient_wanted_copy_of_letter, "")
        h += tr_qa(self.wxstring(req, "gaf_at_first_assessment"),
                   self.gaf_at_first_assessment, "")
        h += tr_qa(self.wxstring(req, "gaf_at_discharge"),
                   self.gaf_at_discharge, "")

        h += subheading_spanning_two_columns(
            self.wxstring(req, "referral_reason_t"))
        h += tr_span_col(answer(", ".join(self.get_referral_reasons(req))),
                         cols=2)
        h += tr_qa(self.wxstring(req, "referral_reason_transplant_organ"),
                   self.referral_reason_transplant_organ, "")
        h += tr_qa(self.wxstring(req, "referral_reason_other_detail"),
                   self.referral_reason_other_detail, "")

        h += subheading_spanning_two_columns(
            self.wxstring(req, "diagnoses_t"))
        h += tr_qa(self.wxstring(req, "psychiatric_t"),
                   "\n".join(self.get_psychiatric_diagnoses(req)), "")
        h += tr_qa(self.wxstring(req, "medical_t"),
                   "\n".join(self.get_medical_diagnoses()), "")

        h += subheading_spanning_two_columns(self.wxstring(req, "management_t"))
        h += tr_span_col(answer(", ".join(self.get_managements(req))), cols=2)
        h += tr_qa(self.wxstring(req, "management_other_detail"),
                   self.management_other_detail, "")

        h += subheading_spanning_two_columns(self.wxstring(req, "outcome_t"))
        h += tr_qa(self.wxstring(req, "outcome_t"),
                   self.outcome, "")
        h += tr_qa(self.wxstring(req, "outcome_hospital_transfer_detail"),
                   self.outcome_hospital_transfer_detail, "")
        h += tr_qa(self.wxstring(req, "outcome_other_detail"),
                   self.outcome_other_detail, "")

        h += """
            </table>
        """
        return h


# =============================================================================
# Reports
# =============================================================================

class LPSReportSchema(ReportParamSchema):
    which_idnum = LinkingIdNumSelector()  # must match ViewParam.WHICH_IDNUM


class LPSReportReferredNotDischarged(Report):
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "cpft_lps_referred_not_subsequently_discharged"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("CPFT LPS – referred but not yet discharged")

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    @staticmethod
    def get_paramform_schema_class() -> Type[ReportParamSchema]:
        return LPSReportSchema

    # noinspection PyProtectedMember,PyUnresolvedReferences
    def get_query(self, req: CamcopsRequest) -> SelectBase:
        which_idnum = req.get_int_param(ViewParam.WHICH_IDNUM, 1)
        if which_idnum is None:
            raise exc.HTTPBadRequest(
                f"{ViewParam.WHICH_IDNUM} not specified")

        group_ids = req.user.ids_of_groups_user_may_report_on

        # Step 1: link referral and patient
        p1 = Patient.__table__.alias("p1")
        i1 = PatientIdNum.__table__.alias("i1")
        desc = req.get_id_shortdesc(which_idnum)
        select_fields = [
            CPFTLPSReferral.lps_division,
            CPFTLPSReferral.referral_date_time,
            CPFTLPSReferral.referral_priority,
            p1.c.surname,
            p1.c.forename,
            p1.c.dob,
            i1.c.idnum_value.label(desc),
            CPFTLPSReferral.patient_location,
        ]
        select_from = p1.join(CPFTLPSReferral.__table__, and_(
            p1.c._current == True,  # noqa: E712
            CPFTLPSReferral.patient_id == p1.c.id,
            CPFTLPSReferral._device_id == p1.c._device_id,
            CPFTLPSReferral._era == p1.c._era,
            CPFTLPSReferral._current == True,
        ))
        select_from = select_from.join(i1, and_(
            i1.c.patient_id == p1.c.id,
            i1.c._device_id == p1.c._device_id,
            i1.c._era == p1.c._era,
            i1.c._current == True,  # noqa: E712
        ))
        wheres = [
            i1.c.which_idnum == which_idnum,
        ]
        if not req.user.superuser:
            # Restrict to accessible groups
            wheres.append(CPFTLPSReferral._group_id.in_(group_ids))

        # Step 2: not yet discharged
        p2 = Patient.__table__.alias("p2")
        i2 = PatientIdNum.__table__.alias("i2")
        discharge = (
            select(['*'])
            .select_from(
                p2.join(CPFTLPSDischarge.__table__, and_(
                    p2.c._current == True,  # noqa: E712
                    CPFTLPSDischarge.patient_id == p2.c.id,
                    CPFTLPSDischarge._device_id == p2.c._device_id,
                    CPFTLPSDischarge._era == p2.c._era,
                    CPFTLPSDischarge._current == True,
                )).join(i2, and_(
                    i2.c.patient_id == p2.c.id,
                    i2.c._device_id == p2.c._device_id,
                    i2.c._era == p2.c._era,
                    i2.c._current == True,
                ))
            )
            .where(and_(
                # Link on ID to main query: same patient
                i2.c.which_idnum == which_idnum,
                i2.c.idnum_value == i1.c.idnum_value,
                # Discharge later than referral
                (CPFTLPSDischarge.discharge_date >=
                 CPFTLPSReferral.referral_date_time),
            ))
        )  # nopep8
        if not req.user.superuser:
            # Restrict to accessible groups
            discharge = discharge.where(
                CPFTLPSDischarge._group_id.in_(group_ids))

        wheres.append(~exists(discharge))

        # Finish up
        order_by = [
            CPFTLPSReferral.lps_division,
            CPFTLPSReferral.referral_date_time,
            CPFTLPSReferral.referral_priority,
        ]
        query = select(select_fields) \
            .select_from(select_from) \
            .where(and_(*wheres)) \
            .order_by(*order_by)
        return query


class LPSReportReferredNotClerkedOrDischarged(Report):
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "cpft_lps_referred_not_subsequently_clerked_or_discharged"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("CPFT LPS – referred but not yet fully assessed or discharged")  # noqa

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    @staticmethod
    def get_paramform_schema_class() -> Type[ReportParamSchema]:
        return LPSReportSchema

    # noinspection PyProtectedMember
    def get_query(self, req: CamcopsRequest) -> SelectBase:
        which_idnum = req.get_int_param(ViewParam.WHICH_IDNUM, 1)
        if which_idnum is None:
            raise exc.HTTPBadRequest(
                f"{ViewParam.WHICH_IDNUM} not specified")

        group_ids = req.user.ids_of_groups_user_may_report_on

        # Step 1: link referral and patient
        # noinspection PyUnresolvedReferences
        p1 = Patient.__table__.alias("p1")
        # noinspection PyUnresolvedReferences
        i1 = PatientIdNum.__table__.alias("i1")
        desc = req.get_id_shortdesc(which_idnum)
        select_fields = [
            CPFTLPSReferral.lps_division,
            CPFTLPSReferral.referral_date_time,
            CPFTLPSReferral.referral_priority,
            p1.c.surname,
            p1.c.forename,
            p1.c.dob,
            i1.c.idnum_value.label(desc),
            CPFTLPSReferral.patient_location,
        ]
        # noinspection PyUnresolvedReferences
        select_from = p1.join(CPFTLPSReferral.__table__, and_(
            p1.c._current == True,  # noqa: E712
            CPFTLPSReferral.patient_id == p1.c.id,
            CPFTLPSReferral._device_id == p1.c._device_id,
            CPFTLPSReferral._era == p1.c._era,
            CPFTLPSReferral._current == True,
        ))
        select_from = select_from.join(i1, and_(
            i1.c.patient_id == p1.c.id,
            i1.c._device_id == p1.c._device_id,
            i1.c._era == p1.c._era,
            i1.c._current == True,  # noqa: E712
        ))  # nopep8
        wheres = [
            i1.c.which_idnum == which_idnum,
        ]
        if not req.user.superuser:
            # Restrict to accessible groups
            wheres.append(CPFTLPSReferral._group_id.in_(group_ids))

        # Step 2: not yet discharged
        # noinspection PyUnresolvedReferences
        p2 = Patient.__table__.alias("p2")
        # noinspection PyUnresolvedReferences
        i2 = PatientIdNum.__table__.alias("i2")
        # noinspection PyUnresolvedReferences
        discharge = (
            select(['*'])
            .select_from(
                p2.join(CPFTLPSDischarge.__table__, and_(
                    p2.c._current == True,  # noqa: E712
                    CPFTLPSDischarge.patient_id == p2.c.id,
                    CPFTLPSDischarge._device_id == p2.c._device_id,
                    CPFTLPSDischarge._era == p2.c._era,
                    CPFTLPSDischarge._current == True,
                )).join(i2, and_(
                    i2.c.patient_id == p2.c.id,
                    i2.c._device_id == p2.c._device_id,
                    i2.c._era == p2.c._era,
                    i2.c._current == True,
                ))
            )
            .where(and_(
                # Link on ID to main query: same patient
                i2.c.which_idnum == which_idnum,
                i2.c.idnum_value == i1.c.idnum_value,
                # Discharge later than referral
                (CPFTLPSDischarge.discharge_date >=
                 CPFTLPSReferral.referral_date_time),
            ))
        )  # nopep8
        if not req.user.superuser:
            # Restrict to accessible groups
            discharge = discharge.where(
                CPFTLPSDischarge._group_id.in_(group_ids))
        wheres.append(~exists(discharge))

        # Step 3: not yet clerked
        # noinspection PyUnresolvedReferences
        p3 = Patient.__table__.alias("p3")
        # noinspection PyUnresolvedReferences
        i3 = PatientIdNum.__table__.alias("i3")
        # noinspection PyUnresolvedReferences
        clerking = (
            select(['*'])
            .select_from(
                p3.join(PsychiatricClerking.__table__, and_(
                    p3.c._current == True,  # noqa: E712
                    PsychiatricClerking.patient_id == p3.c.id,
                    PsychiatricClerking._device_id == p3.c._device_id,
                    PsychiatricClerking._era == p3.c._era,
                    PsychiatricClerking._current == True,
                )).join(i3, and_(
                    i3.c.patient_id == p3.c.id,
                    i3.c._device_id == p3.c._device_id,
                    i3.c._era == p3.c._era,
                    i3.c._current == True,
                ))
            )
            .where(and_(
                # Link on ID to main query: same patient
                i3.c.which_idnum == which_idnum,
                i3.c.idnum_value == i1.c.idnum_value,
                # Discharge later than referral
                (PsychiatricClerking.when_created >=
                 CPFTLPSReferral.referral_date_time),
            ))
        )  # nopep8
        if not req.user.superuser:
            # Restrict to accessible groups
            clerking = clerking.where(
                PsychiatricClerking._group_id.in_(group_ids))
        wheres.append(~exists(clerking))

        # Finish up
        order_by = [
            CPFTLPSReferral.lps_division,
            CPFTLPSReferral.referral_date_time,
            CPFTLPSReferral.referral_priority,
        ]
        query = (
            select(select_fields)
            .select_from(select_from)
            .where(and_(*wheres))
            .order_by(*order_by)
        )
        return query
