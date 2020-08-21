/*
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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cpftlpsdischarge.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "diagnosis/icd10.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/qudiagnosticcode.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;


const QString CPFTLPSDischarge::CPFTLPSDISCHARGE_TABLENAME("cpft_lps_discharge");

const QString DISCHARGE_DATE("discharge_date");
const QString DISCHARGE_REASON_CODE("discharge_reason_code");

const QString LEAFLET_OR_DISCHARGE_CARD_GIVEN("leaflet_or_discharge_card_given");
const QString FREQUENT_ATTENDER("frequent_attender");
const QString PATIENT_WANTED_COPY_OF_LETTER("patient_wanted_copy_of_letter");
const QString GAF_AT_FIRST_ASSESSMENT("gaf_at_first_assessment");
const QString GAF_AT_DISCHARGE("gaf_at_discharge");

const QString REFERRAL_REASON_SELF_HARM_OVERDOSE("referral_reason_self_harm_overdose");
const QString REFERRAL_REASON_SELF_HARM_OTHER("referral_reason_self_harm_other");
const QString REFERRAL_REASON_SUICIDAL_IDEAS("referral_reason_suicidal_ideas");
const QString REFERRAL_REASON_BEHAVIOURAL_DISTURBANCE("referral_reason_behavioural_disturbance");
const QString REFERRAL_REASON_LOW_MOOD("referral_reason_low_mood");
const QString REFERRAL_REASON_ELEVATED_MOOD("referral_reason_elevated_mood");
const QString REFERRAL_REASON_PSYCHOSIS("referral_reason_psychosis");
const QString REFERRAL_REASON_PRE_TRANSPLANT("referral_reason_pre_transplant");
const QString REFERRAL_REASON_POST_TRANSPLANT("referral_reason_post_transplant");
const QString REFERRAL_REASON_DELIRIUM("referral_reason_delirium");
const QString REFERRAL_REASON_ANXIETY("referral_reason_anxiety");
const QString REFERRAL_REASON_SOMATOFORM_MUS("referral_reason_somatoform_mus");
const QString REFERRAL_REASON_MOTIVATION_ADHERENCE("referral_reason_motivation_adherence");
const QString REFERRAL_REASON_CAPACITY("referral_reason_capacity");
const QString REFERRAL_REASON_EATING_DISORDER("referral_reason_eating_disorder");
const QString REFERRAL_REASON_SAFEGUARDING("referral_reason_safeguarding");
const QString REFERRAL_REASON_DISCHARGE_PLACEMENT("referral_reason_discharge_placement");
const QString REFERRAL_REASON_COGNITIVE_PROBLEM("referral_reason_cognitive_problem");
const QString REFERRAL_REASON_SUBSTANCE_ALCOHOL("referral_reason_substance_alcohol");
const QString REFERRAL_REASON_SUBSTANCE_OTHER("referral_reason_substance_other");
const QString REFERRAL_REASON_OTHER("referral_reason_other");

const QString REFERRAL_REASON_TRANSPLANT_ORGAN("referral_reason_transplant_organ");
const QString REFERRAL_REASON_OTHER_DETAIL("referral_reason_other_detail");

const QString DIAGNOSIS_NO_ACTIVE_MENTAL_HEALTH_PROBLEM("diagnosis_no_active_mental_health_problem");
const QString DIAGNOSIS_PSYCH_1_ICD10CODE("diagnosis_psych_1_icd10code");
const QString DIAGNOSIS_PSYCH_1_DESCRIPTION("diagnosis_psych_1_description");
const QString DIAGNOSIS_PSYCH_2_ICD10CODE("diagnosis_psych_2_icd10code");
const QString DIAGNOSIS_PSYCH_2_DESCRIPTION("diagnosis_psych_2_description");
const QString DIAGNOSIS_PSYCH_3_ICD10CODE("diagnosis_psych_3_icd10code");
const QString DIAGNOSIS_PSYCH_3_DESCRIPTION("diagnosis_psych_3_description");
const QString DIAGNOSIS_PSYCH_4_ICD10CODE("diagnosis_psych_4_icd10code");
const QString DIAGNOSIS_PSYCH_4_DESCRIPTION("diagnosis_psych_4_description");
const QString DIAGNOSIS_MEDICAL_1("diagnosis_medical_1");
const QString DIAGNOSIS_MEDICAL_2("diagnosis_medical_2");
const QString DIAGNOSIS_MEDICAL_3("diagnosis_medical_3");
const QString DIAGNOSIS_MEDICAL_4("diagnosis_medical_4");

const QString MANAGEMENT_ASSESSMENT_DIAGNOSTIC("management_assessment_diagnostic");
const QString MANAGEMENT_MEDICATION("management_medication");
const QString MANAGEMENT_SPECIALLING_BEHAVIOURAL_DISTURBANCE("management_specialling_behavioural_disturbance");
const QString MANAGEMENT_SUPPORTIVE_PATIENT("management_supportive_patient");
const QString MANAGEMENT_SUPPORTIVE_CARERS("management_supportive_carers");
const QString MANAGEMENT_SUPPORTIVE_STAFF("management_supportive_staff");
const QString MANAGEMENT_NURSING_MANAGEMENT("management_nursing_management");
const QString MANAGEMENT_THERAPY_CBT("management_therapy_cbt");
const QString MANAGEMENT_THERAPY_CAT("management_therapy_cat");
const QString MANAGEMENT_THERAPY_OTHER("management_therapy_other");
const QString MANAGEMENT_TREATMENT_ADHERENCE("management_treatment_adherence");
const QString MANAGEMENT_CAPACITY("management_capacity");
const QString MANAGEMENT_EDUCATION_PATIENT("management_education_patient");
const QString MANAGEMENT_EDUCATION_CARERS("management_education_carers");
const QString MANAGEMENT_EDUCATION_STAFF("management_education_staff");
const QString MANAGEMENT_ACCOMMODATION_PLACEMENT("management_accommodation_placement");
const QString MANAGEMENT_SIGNPOSTING_EXTERNAL_REFERRAL("management_signposting_external_referral");
const QString MANAGEMENT_MHA_S136("management_mha_s136");
const QString MANAGEMENT_MHA_S5_2("management_mha_s5_2");
const QString MANAGEMENT_MHA_S2("management_mha_s2");
const QString MANAGEMENT_MHA_S3("management_mha_s3");
const QString MANAGEMENT_COMPLEX_CASE_CONFERENCE("management_complex_case_conference");
const QString MANAGEMENT_OTHER("management_other");

const QString MANAGEMENT_OTHER_DETAIL("management_other_detail");

const QString OUTCOME("outcome");
const QString OUTCOME_HOSPITAL_TRANSFER_DETAIL("outcome_hospital_transfer_detail");
const QString OUTCOME_OTHER_DETAIL("outcome_other_detail");


void initializeCPFTLPSDischarge(TaskFactory& factory)
{
    static TaskRegistrar<CPFTLPSDischarge> registered(factory);
}


CPFTLPSDischarge::CPFTLPSDischarge(CamcopsApp& app, DatabaseManager& db,
                                   const int load_pk) :
    Task(app, db, CPFTLPSDISCHARGE_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(DISCHARGE_DATE, QVariant::Date);
    addField(DISCHARGE_REASON_CODE, QVariant::String);

    addField(LEAFLET_OR_DISCHARGE_CARD_GIVEN, QVariant::Bool);
    addField(FREQUENT_ATTENDER, QVariant::Bool);
    addField(PATIENT_WANTED_COPY_OF_LETTER, QVariant::Bool);  // was erroneously String prior to 2.0.3
    addField(GAF_AT_FIRST_ASSESSMENT, QVariant::Int);
    addField(GAF_AT_DISCHARGE, QVariant::Int);

    addField(REFERRAL_REASON_SELF_HARM_OVERDOSE, QVariant::Bool);
    addField(REFERRAL_REASON_SELF_HARM_OTHER, QVariant::Bool);
    addField(REFERRAL_REASON_SUICIDAL_IDEAS, QVariant::Bool);
    addField(REFERRAL_REASON_BEHAVIOURAL_DISTURBANCE, QVariant::Bool);
    addField(REFERRAL_REASON_LOW_MOOD, QVariant::Bool);
    addField(REFERRAL_REASON_ELEVATED_MOOD, QVariant::Bool);
    addField(REFERRAL_REASON_PSYCHOSIS, QVariant::Bool);
    addField(REFERRAL_REASON_PRE_TRANSPLANT, QVariant::Bool);
    addField(REFERRAL_REASON_POST_TRANSPLANT, QVariant::Bool);
    addField(REFERRAL_REASON_DELIRIUM, QVariant::Bool);
    addField(REFERRAL_REASON_ANXIETY, QVariant::Bool);
    addField(REFERRAL_REASON_SOMATOFORM_MUS, QVariant::Bool);
    addField(REFERRAL_REASON_MOTIVATION_ADHERENCE, QVariant::Bool);
    addField(REFERRAL_REASON_CAPACITY, QVariant::Bool);
    addField(REFERRAL_REASON_EATING_DISORDER, QVariant::Bool);
    addField(REFERRAL_REASON_SAFEGUARDING, QVariant::Bool);
    addField(REFERRAL_REASON_DISCHARGE_PLACEMENT, QVariant::Bool);
    addField(REFERRAL_REASON_COGNITIVE_PROBLEM, QVariant::Bool);
    addField(REFERRAL_REASON_SUBSTANCE_ALCOHOL, QVariant::Bool);
    addField(REFERRAL_REASON_SUBSTANCE_OTHER, QVariant::Bool);
    addField(REFERRAL_REASON_OTHER, QVariant::Bool);

    addField(REFERRAL_REASON_TRANSPLANT_ORGAN, QVariant::String);
    addField(REFERRAL_REASON_OTHER_DETAIL, QVariant::String);

    addField(DIAGNOSIS_NO_ACTIVE_MENTAL_HEALTH_PROBLEM, QVariant::Bool);
    addField(DIAGNOSIS_PSYCH_1_ICD10CODE, QVariant::String);
    addField(DIAGNOSIS_PSYCH_1_DESCRIPTION, QVariant::String);
    addField(DIAGNOSIS_PSYCH_2_ICD10CODE, QVariant::String);
    addField(DIAGNOSIS_PSYCH_2_DESCRIPTION, QVariant::String);
    addField(DIAGNOSIS_PSYCH_3_ICD10CODE, QVariant::String);
    addField(DIAGNOSIS_PSYCH_3_DESCRIPTION, QVariant::String);
    addField(DIAGNOSIS_PSYCH_4_ICD10CODE, QVariant::String);
    addField(DIAGNOSIS_PSYCH_4_DESCRIPTION, QVariant::String);
    addField(DIAGNOSIS_MEDICAL_1, QVariant::String);
    addField(DIAGNOSIS_MEDICAL_2, QVariant::String);
    addField(DIAGNOSIS_MEDICAL_3, QVariant::String);
    addField(DIAGNOSIS_MEDICAL_4, QVariant::String);

    addField(MANAGEMENT_ASSESSMENT_DIAGNOSTIC, QVariant::Bool);
    addField(MANAGEMENT_MEDICATION, QVariant::Bool);
    addField(MANAGEMENT_SPECIALLING_BEHAVIOURAL_DISTURBANCE, QVariant::Bool);
    addField(MANAGEMENT_SUPPORTIVE_PATIENT, QVariant::Bool);
    addField(MANAGEMENT_SUPPORTIVE_CARERS, QVariant::Bool);
    addField(MANAGEMENT_SUPPORTIVE_STAFF, QVariant::Bool);
    addField(MANAGEMENT_NURSING_MANAGEMENT, QVariant::Bool);
    addField(MANAGEMENT_THERAPY_CBT, QVariant::Bool);
    addField(MANAGEMENT_THERAPY_CAT, QVariant::Bool);
    addField(MANAGEMENT_THERAPY_OTHER, QVariant::Bool);
    addField(MANAGEMENT_TREATMENT_ADHERENCE, QVariant::Bool);
    addField(MANAGEMENT_CAPACITY, QVariant::Bool);
    addField(MANAGEMENT_EDUCATION_PATIENT, QVariant::Bool);
    addField(MANAGEMENT_EDUCATION_CARERS, QVariant::Bool);
    addField(MANAGEMENT_EDUCATION_STAFF, QVariant::Bool);
    addField(MANAGEMENT_ACCOMMODATION_PLACEMENT, QVariant::Bool);
    addField(MANAGEMENT_SIGNPOSTING_EXTERNAL_REFERRAL, QVariant::Bool);
    addField(MANAGEMENT_MHA_S136, QVariant::Bool);
    addField(MANAGEMENT_MHA_S5_2, QVariant::Bool);
    addField(MANAGEMENT_MHA_S2, QVariant::Bool);
    addField(MANAGEMENT_MHA_S3, QVariant::Bool);
    addField(MANAGEMENT_COMPLEX_CASE_CONFERENCE, QVariant::Bool);
    addField(MANAGEMENT_OTHER, QVariant::Bool);
    addField(MANAGEMENT_OTHER_DETAIL, QVariant::String);

    addField(OUTCOME, QVariant::String);
    addField(OUTCOME_HOSPITAL_TRANSFER_DETAIL, QVariant::String);
    addField(OUTCOME_OTHER_DETAIL, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CPFTLPSDischarge::shortname() const
{
    return "CPFT_LPS_Discharge";
}


QString CPFTLPSDischarge::longname() const
{
    return tr("CPFT LPS – discharge");
}


QString CPFTLPSDischarge::description() const
{
    return tr("Discharge from CPFT Liaison Psychiatry Service.");
}


QString CPFTLPSDischarge::infoFilenameStem() const
{
    return "clinical";
}


QString CPFTLPSDischarge::xstringTaskname() const
{
    return "cpft_lps_discharge";
}


// ============================================================================
// Instance info
// ============================================================================

bool CPFTLPSDischarge::isComplete() const
{
    // The bare minimum:
    return noneNull(values(QStringList{DISCHARGE_DATE, DISCHARGE_REASON_CODE}));
}


QStringList CPFTLPSDischarge::summary() const
{
    return QStringList{
        QString("%1: <b>%2</b>.").arg(xstring("discharge_date"),
                                      datetime::textDateTime(value(DISCHARGE_DATE))),
        QString("%1: <b>%2</b>.").arg(xstring("discharge_reason"),
                                      prettyValue(DISCHARGE_REASON_CODE)),
    };
}


QStringList CPFTLPSDischarge::detail() const
{
    return completenessInfo() + summary() + QStringList{
        "",
        TextConst::seeFacsimileForMoreDetail(),
    };
}


OpenableWidget* CPFTLPSDischarge::editor(const bool read_only)
{
    const NameValueOptions discharge_reason_code_options{
        {xstring("reason_code_F"), "F"},
        {xstring("reason_code_A"), "A"},
        {xstring("reason_code_O"), "O"},
        {xstring("reason_code_C"), "C"},
    };
    const NameValueOptions wanted_letter_options = CommonOptions::optionsCopyingDescriptions({
        "None done",
        "Yes",
        "No",
        "Not appropriate",
    });
    const NameValueOptions outcome_options = CommonOptions::optionsCopyingDescriptions({
        "Outcome achieved/no follow-up",
        "CMHT (new)",
        "CMHT (ongoing)",
        "CRHTT (new)",
        "CRHTT (ongoing)",
        "GP follow-up",
        "Liaison outpatient follow-up",
        "Transferred to general hospital",
        "Transferred to psychiatric hospital",
        "Nursing home",
        "Day hospital",
        "Treatment declined",
        "Patient died",
        "Other",
    });
    const NameValueOptions organ_options = CommonOptions::optionsCopyingDescriptions({
        "Liver",
        "Kidney",
        "Small bowel",
        "Other",
        "Multivisceral",
    });
    const NameValueOptions yesno_options = CommonOptions::noYesBoolean();
    DiagnosticCodeSetPtr icd10(new Icd10(m_app));

    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto yn = [this, &yesno_options](const QString& fieldname,
                                     bool mandatory = false) -> QuElement* {
        return (new QuMcq(fieldRef(fieldname, mandatory), yesno_options))
                ->setAsTextButton(true)
                ->setHorizontal(true);
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options,
                      bool mandatory = false) -> QuElement* {
        return (new QuMcq(fieldRef(fieldname, mandatory), options))
                ->setAsTextButton(true)
                ->setHorizontal(true);
    };
    auto boolbutton = [this](const QString& fieldname,
                             const QString& xstringname,
                             bool mandatory = false) -> QuElement* {
        return (new QuBoolean(xstring(xstringname),
                              fieldRef(fieldname, mandatory)))
                ->setAsTextButton(true);
    };

    const QString dis_dx_psych = xstring("diagnosis_psych");
    const QString dis_dx_med = xstring("diagnosis_medical");

    QuPagePtr page((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),

        boldtext("discharge_date"),
        (new QuDateTime(fieldRef(DISCHARGE_DATE)))
                       ->setMode(QuDateTime::Mode::DefaultDate)
                       ->setOfferNowButton(true),
        boldtext("discharge_reason"),
        mcq(DISCHARGE_REASON_CODE, discharge_reason_code_options, true),
        boldtext("leaflet_or_discharge_card_given"),
        yn(LEAFLET_OR_DISCHARGE_CARD_GIVEN),
                        boldtext("frequent_attender"),
                        yn(FREQUENT_ATTENDER),
        boldtext("patient_wanted_copy_of_letter"),
                        yn(PATIENT_WANTED_COPY_OF_LETTER),
        questionnairefunc::defaultGridRawPointer({
            {xstring("gaf_at_first_assessment"),
             new QuLineEditInteger(fieldRef(GAF_AT_FIRST_ASSESSMENT, false), 0, 100)},
            {xstring("gaf_at_discharge"),
             new QuLineEditInteger(fieldRef(GAF_AT_DISCHARGE, false), 0, 100)},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("referral_reason_t"),
        new QuFlowContainer{
            boolbutton(REFERRAL_REASON_SELF_HARM_OVERDOSE, "referral_reason_self_harm_overdose"),
            boolbutton(REFERRAL_REASON_SELF_HARM_OTHER, "referral_reason_self_harm_other"),
            boolbutton(REFERRAL_REASON_SUICIDAL_IDEAS, "referral_reason_suicidal_ideas"),
            boolbutton(REFERRAL_REASON_BEHAVIOURAL_DISTURBANCE, "referral_reason_behavioural_disturbance"),
            boolbutton(REFERRAL_REASON_LOW_MOOD, "referral_reason_low_mood"),
            boolbutton(REFERRAL_REASON_ELEVATED_MOOD, "referral_reason_elevated_mood"),
            boolbutton(REFERRAL_REASON_PSYCHOSIS, "referral_reason_psychosis"),
            boolbutton(REFERRAL_REASON_PRE_TRANSPLANT, "referral_reason_pre_transplant"),
            boolbutton(REFERRAL_REASON_POST_TRANSPLANT, "referral_reason_post_transplant"),
            boolbutton(REFERRAL_REASON_DELIRIUM, "referral_reason_delirium"),
            boolbutton(REFERRAL_REASON_ANXIETY, "referral_reason_anxiety"),
            boolbutton(REFERRAL_REASON_SOMATOFORM_MUS, "referral_reason_somatoform_mus"),
            boolbutton(REFERRAL_REASON_MOTIVATION_ADHERENCE, "referral_reason_motivation_adherence"),
            boolbutton(REFERRAL_REASON_CAPACITY, "referral_reason_capacity"),
            boolbutton(REFERRAL_REASON_EATING_DISORDER, "referral_reason_eating_disorder"),
            boolbutton(REFERRAL_REASON_SAFEGUARDING, "referral_reason_safeguarding"),
            boolbutton(REFERRAL_REASON_DISCHARGE_PLACEMENT, "referral_reason_discharge_placement"),
            boolbutton(REFERRAL_REASON_COGNITIVE_PROBLEM, "referral_reason_cognitive_problem"),
            boolbutton(REFERRAL_REASON_SUBSTANCE_ALCOHOL, "referral_reason_substance_alcohol"),
            boolbutton(REFERRAL_REASON_SUBSTANCE_OTHER, "referral_reason_substance_other"),
            boolbutton(REFERRAL_REASON_OTHER, "referral_reason_other"),
        },
        questionnairefunc::defaultGridRawPointer({
            {xstring("referral_reason_transplant_organ"),
             mcq(REFERRAL_REASON_TRANSPLANT_ORGAN, organ_options)},
            {xstring("referral_reason_other_detail"),
             new QuTextEdit(fieldRef(REFERRAL_REASON_OTHER_DETAIL, false))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("diagnoses_t"),
        boolbutton(DIAGNOSIS_NO_ACTIVE_MENTAL_HEALTH_PROBLEM, "diagnosis_no_active_mental_health_problem"),
        questionnairefunc::defaultGridRawPointer({
            {dis_dx_psych + " 1",
             new QuDiagnosticCode(icd10,
                                  fieldRef(DIAGNOSIS_PSYCH_1_ICD10CODE, false),
                                  fieldRef(DIAGNOSIS_PSYCH_1_DESCRIPTION, false))},
            {dis_dx_psych + " 2",
             new QuDiagnosticCode(icd10,
                                  fieldRef(DIAGNOSIS_PSYCH_2_ICD10CODE, false),
                                  fieldRef(DIAGNOSIS_PSYCH_2_DESCRIPTION, false))},
            {dis_dx_psych + " 3",
             new QuDiagnosticCode(icd10,
                                  fieldRef(DIAGNOSIS_PSYCH_3_ICD10CODE, false),
                                  fieldRef(DIAGNOSIS_PSYCH_3_DESCRIPTION, false))},
            {dis_dx_psych + " 4",
             new QuDiagnosticCode(icd10,
                                  fieldRef(DIAGNOSIS_PSYCH_4_ICD10CODE, false),
                                  fieldRef(DIAGNOSIS_PSYCH_4_DESCRIPTION, false))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
        questionnairefunc::defaultGridRawPointer({
            {dis_dx_med + " 1",
             new QuTextEdit(fieldRef(DIAGNOSIS_MEDICAL_1, false))},
            {dis_dx_med + " 2",
             new QuTextEdit(fieldRef(DIAGNOSIS_MEDICAL_2, false))},
            {dis_dx_med + " 3",
             new QuTextEdit(fieldRef(DIAGNOSIS_MEDICAL_3, false))},
            {dis_dx_med + " 4",
             new QuTextEdit(fieldRef(DIAGNOSIS_MEDICAL_4, false))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("referral_reason_t"),
        new QuFlowContainer{
            boolbutton(MANAGEMENT_ASSESSMENT_DIAGNOSTIC, "management_assessment_diagnostic"),
            boolbutton(MANAGEMENT_MEDICATION, "management_medication"),
            boolbutton(MANAGEMENT_SPECIALLING_BEHAVIOURAL_DISTURBANCE, "management_specialling_behavioural_disturbance"),
            boolbutton(MANAGEMENT_SUPPORTIVE_PATIENT, "management_supportive_patient"),
            boolbutton(MANAGEMENT_SUPPORTIVE_CARERS, "management_supportive_carers"),
            boolbutton(MANAGEMENT_SUPPORTIVE_STAFF, "management_supportive_staff"),
            boolbutton(MANAGEMENT_NURSING_MANAGEMENT, "management_nursing_management"),
            boolbutton(MANAGEMENT_THERAPY_CBT, "management_therapy_cbt"),
            boolbutton(MANAGEMENT_THERAPY_CAT, "management_therapy_cat"),
            boolbutton(MANAGEMENT_THERAPY_OTHER, "management_therapy_other"),
            boolbutton(MANAGEMENT_TREATMENT_ADHERENCE, "management_treatment_adherence"),
            boolbutton(MANAGEMENT_CAPACITY, "management_capacity"),
            boolbutton(MANAGEMENT_EDUCATION_PATIENT, "management_education_patient"),
            boolbutton(MANAGEMENT_EDUCATION_CARERS, "management_education_carers"),
            boolbutton(MANAGEMENT_EDUCATION_STAFF, "management_education_staff"),
            boolbutton(MANAGEMENT_ACCOMMODATION_PLACEMENT, "management_accommodation_placement"),
            boolbutton(MANAGEMENT_SIGNPOSTING_EXTERNAL_REFERRAL, "management_signposting_external_referral"),
            boolbutton(MANAGEMENT_MHA_S136, "management_mha_s136"),
            boolbutton(MANAGEMENT_MHA_S5_2, "management_mha_s5_2"),
            boolbutton(MANAGEMENT_MHA_S2, "management_mha_s2"),
            boolbutton(MANAGEMENT_COMPLEX_CASE_CONFERENCE, "management_complex_case_conference"),
            boolbutton(MANAGEMENT_OTHER, "management_other"),
        },
        questionnairefunc::defaultGridRawPointer({
            {xstring("management_other_detail"),
             new QuTextEdit(fieldRef(MANAGEMENT_OTHER_DETAIL, false))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("outcome_t"),
        mcq(OUTCOME, outcome_options),
        questionnairefunc::defaultGridRawPointer({
            {xstring("outcome_hospital_transfer_detail"),
             new QuTextEdit(fieldRef(OUTCOME_HOSPITAL_TRANSFER_DETAIL, false))},
            {xstring("outcome_other_detail"),
             new QuTextEdit(fieldRef(OUTCOME_OTHER_DETAIL, false))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

    })->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================
