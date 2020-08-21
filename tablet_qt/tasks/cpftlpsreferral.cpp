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

#include "cpftlpsreferral.h"
#include "core/camcopsapp.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNullOrEmpty;


const QString CPFTLPSReferral::CPFTLPSREFERRAL_TABLENAME("cpft_lps_referral");

const QString REFERRAL_DATE_TIME("referral_date_time");
const QString LPS_DIVISION("lps_division");
const QString REFERRAL_PRIORITY("referral_priority");
const QString REFERRAL_METHOD("referral_method");
const QString REFERRER_NAME("referrer_name");
const QString REFERRER_CONTACT_DETAILS("referrer_contact_details");
const QString REFERRING_CONSULTANT("referring_consultant");
const QString REFERRING_SPECIALTY("referring_specialty");
const QString REFERRING_SPECIALTY_OTHER("referring_specialty_other");

const QString PATIENT_LOCATION("patient_location");
const QString ADMISSION_DATE("admission_date");
const QString ESTIMATED_DISCHARGE_DATE("estimated_discharge_date");
const QString PATIENT_AWARE_OF_REFERRAL("patient_aware_of_referral");
const QString INTERPRETER_REQUIRED("interpreter_required");
const QString SENSORY_IMPAIRMENT("sensory_impairment");
const QString MARITAL_STATUS_CODE("marital_status_code");
const QString ETHNIC_CATEGORY_CODE("ethnic_category_code");

const QString ADMISSION_REASON_OVERDOSE("admission_reason_overdose");
const QString ADMISSION_REASON_SELF_HARM_NOT_OVERDOSE("admission_reason_self_harm_not_overdose");
const QString ADMISSION_REASON_CONFUSION("admission_reason_confusion");
const QString ADMISSION_REASON_TRAUMA("admission_reason_trauma");
const QString ADMISSION_REASON_FALLS("admission_reason_falls");
const QString ADMISSION_REASON_INFECTION("admission_reason_infection");
const QString ADMISSION_REASON_POOR_ADHERENCE("admission_reason_poor_adherence");
const QString ADMISSION_REASON_OTHER("admission_reason_other");

const QString EXISTING_PSYCHIATRIC_TEAMS("existing_psychiatric_teams");
const QString CARE_COORDINATOR("care_coordinator");
const QString OTHER_CONTACT_DETAILS("other_contact_details");

const QString REFERRAL_REASON("referral_reason");



void initializeCPFTLPSReferral(TaskFactory& factory)
{
    static TaskRegistrar<CPFTLPSReferral> registered(factory);
}


CPFTLPSReferral::CPFTLPSReferral(CamcopsApp& app, DatabaseManager& db,
                                 const int load_pk) :
    Task(app, db, CPFTLPSREFERRAL_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addField(REFERRAL_DATE_TIME, QVariant::DateTime);
    addField(LPS_DIVISION, QVariant::String);
    addField(REFERRAL_PRIORITY, QVariant::String);
    addField(REFERRAL_METHOD, QVariant::String);
    addField(REFERRER_NAME, QVariant::String);
    addField(REFERRER_CONTACT_DETAILS, QVariant::String);
    addField(REFERRING_CONSULTANT, QVariant::String);
    addField(REFERRING_SPECIALTY, QVariant::String);
    addField(REFERRING_SPECIALTY_OTHER, QVariant::String);

    addField(PATIENT_LOCATION, QVariant::String);
    addField(ADMISSION_DATE, QVariant::Date);
    addField(ESTIMATED_DISCHARGE_DATE, QVariant::Date);
    addField(PATIENT_AWARE_OF_REFERRAL, QVariant::Bool);
    addField(INTERPRETER_REQUIRED, QVariant::Bool);
    addField(SENSORY_IMPAIRMENT, QVariant::Bool);
    addField(MARITAL_STATUS_CODE, QVariant::String);
    addField(ETHNIC_CATEGORY_CODE, QVariant::String);

    addField(ADMISSION_REASON_OVERDOSE, QVariant::Bool);
    addField(ADMISSION_REASON_SELF_HARM_NOT_OVERDOSE, QVariant::Bool);
    addField(ADMISSION_REASON_CONFUSION, QVariant::Bool);
    addField(ADMISSION_REASON_TRAUMA, QVariant::Bool);
    addField(ADMISSION_REASON_FALLS, QVariant::Bool);
    addField(ADMISSION_REASON_INFECTION, QVariant::Bool);
    addField(ADMISSION_REASON_POOR_ADHERENCE, QVariant::Bool);
    addField(ADMISSION_REASON_OTHER, QVariant::Bool);

    addField(EXISTING_PSYCHIATRIC_TEAMS, QVariant::String);
    addField(CARE_COORDINATOR, QVariant::String);
    addField(OTHER_CONTACT_DETAILS, QVariant::String);

    addField(REFERRAL_REASON, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CPFTLPSReferral::shortname() const
{
    return "CPFT_LPS_Referral";
}


QString CPFTLPSReferral::longname() const
{
    return tr("CPFT LPS – referral");
}


QString CPFTLPSReferral::description() const
{
    return tr("Referral to CPFT’s Liaison Psychiatry Service.");
}


QString CPFTLPSReferral::infoFilenameStem() const
{
    return "clinical";
}


QString CPFTLPSReferral::xstringTaskname() const
{
    return "cpft_lps_referral";
}


// ============================================================================
// Instance info
// ============================================================================

bool CPFTLPSReferral::isComplete() const
{
    // The bare minimum:
    return noneNullOrEmpty(values(QStringList{REFERRAL_DATE_TIME,
                                              PATIENT_LOCATION,
                                              REFERRAL_REASON}));
}


QStringList CPFTLPSReferral::summary() const
{
    return QStringList{
        QString("%1: <b>%2</b>.").arg(xstring("f_referral_date_time"),
                                      datetime::textDateTime(value(REFERRAL_DATE_TIME))),
        QString("%1: <b>%2</b>.").arg(xstring("f_patient_location"),
                                      prettyValue(PATIENT_LOCATION)),
        QString("%1: <b>%2</b>.").arg(xstring("f_referral_reason"),
                                      prettyValue(REFERRAL_REASON)),
    };
}


QStringList CPFTLPSReferral::detail() const
{
    return completenessInfo() + summary() + QStringList{
        "",
        TextConst::seeFacsimileForMoreDetail(),
    };
}


OpenableWidget* CPFTLPSReferral::editor(const bool read_only)
{
    const NameValueOptions referral_pickup_options = CommonOptions::optionsCopyingDescriptions({
        "Direct",
        "Morning Report",
        "Ops centre",
        "Other",
    });
    const NameValueOptions specialty_options = CommonOptions::optionsCopyingDescriptions({
        "Acute medicine",
        "Cardiology",
        "DME",
        "ED",
        "Endocrinology",
        "Gastroenterology",
        "Hepatology",
        "Neurology",
        "Oncology",
        "Perinatal/obstetric",
        "Renal",
        "Respiratory",
        "Surgery",
        "Transplant",
        "Trauma",
        "Other",  // last (others are alphabetical)
    });
    const NameValueOptions priority_options{
        {xstring("priority_R"), "R"},
        {xstring("priority_U"), "U"},
        {xstring("priority_E"), "E"},
    };
    const NameValueOptions lps_division_options{
        {xstring("service_G"), "G"},
        {xstring("service_O"), "O"},
        {xstring("service_S"), "S"},
    };
    const NameValueOptions yesno_options = CommonOptions::noYesBoolean();
    const NameValueOptions marital_options = m_app.nhsPersonMaritalStatusCodeOptions();
    const NameValueOptions ethnic_options = m_app.nhsEthnicCategoryCodeOptions();

    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
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

    QuPagePtr page((new QuPage{
        boldtext("t_about_referral"),
        text("f_referral_date_time"),
        (new QuDateTime(fieldRef(REFERRAL_DATE_TIME)))
                       ->setMode(QuDateTime::Mode::DefaultDateTime)
                       ->setOfferNowButton(true),
        text("f_lps_division"),
        mcq(LPS_DIVISION, lps_division_options, true),
        text("f_referral_priority"),
        mcq(REFERRAL_PRIORITY, priority_options, true),
        text("f_referral_method"),
        mcq(REFERRAL_METHOD, referral_pickup_options, true),
        questionnairefunc::defaultGridRawPointer({
            {xstring("f_referrer_name"),
             new QuLineEdit(fieldRef(REFERRER_NAME, true))},
            {xstring("f_referrer_contact_details"),
             new QuLineEdit(fieldRef(REFERRER_CONTACT_DETAILS, true))},
            {xstring("f_referring_consultant"),
             new QuLineEdit(fieldRef(REFERRING_CONSULTANT, true))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
        text("f_referring_specialty"),
        mcq(REFERRING_SPECIALTY, specialty_options, true),
        questionnairefunc::defaultGridRawPointer({
            {xstring("f_referring_specialty_other"),
             new QuTextEdit(fieldRef(REFERRING_SPECIALTY_OTHER, false))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("t_patient"),
        questionnairefunc::defaultGridRawPointer({
            {xstring("f_patient_location"),
             new QuTextEdit(fieldRef(PATIENT_LOCATION, true))},
            {xstring("f_admission_date"),
             (new QuDateTime(fieldRef(ADMISSION_DATE, false)))
                ->setMode(QuDateTime::Mode::DefaultDate)
                ->setOfferNowButton(true)},
            {xstring("f_estimated_discharge_date"),
             (new QuDateTime(fieldRef(ESTIMATED_DISCHARGE_DATE, false)))
                ->setMode(QuDateTime::Mode::DefaultDate)
                ->setOfferNowButton(true)},
            {xstring("f_patient_aware_of_referral"),
             yn(PATIENT_AWARE_OF_REFERRAL)},
            {xstring("f_interpreter_required"),
             yn(INTERPRETER_REQUIRED)},
            {xstring("f_sensory_impairment"),
             yn(SENSORY_IMPAIRMENT)},
            {xstring("f_marital_status"),
             mcq(MARITAL_STATUS_CODE, marital_options)},
            {xstring("f_ethnic_category"),
             mcq(ETHNIC_CATEGORY_CODE, ethnic_options)},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("t_referral_reason"),
        new QuFlowContainer{
            boolbutton(ADMISSION_REASON_OVERDOSE, "f_admission_reason_overdose"),
            boolbutton(ADMISSION_REASON_SELF_HARM_NOT_OVERDOSE, "f_admission_reason_self_harm_not_overdose"),
            boolbutton(ADMISSION_REASON_CONFUSION, "f_admission_reason_confusion"),
            boolbutton(ADMISSION_REASON_TRAUMA, "f_admission_reason_trauma"),
            boolbutton(ADMISSION_REASON_FALLS, "f_admission_reason_falls"),
            boolbutton(ADMISSION_REASON_INFECTION, "f_admission_reason_infection"),
            boolbutton(ADMISSION_REASON_POOR_ADHERENCE, "f_admission_reason_poor_adherence"),
            boolbutton(ADMISSION_REASON_OTHER, "f_admission_reason_other"),
        },

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("t_other_people"),
        questionnairefunc::defaultGridRawPointer({
            {xstring("f_existing_psychiatric_teams"),
             new QuTextEdit(fieldRef(EXISTING_PSYCHIATRIC_TEAMS, false))},
            {xstring("f_care_coordinator"),
             new QuTextEdit(fieldRef(CARE_COORDINATOR, false))},
            {xstring("f_other_contact_details"),
             new QuTextEdit(fieldRef(OTHER_CONTACT_DETAILS, false))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),

        // --------------------------------------------------------------------
        new QuHorizontalLine(),
        boldtext("t_referral_reason"),
        questionnairefunc::defaultGridRawPointer({
            {xstring("f_referral_reason"),
             new QuTextEdit(fieldRef(REFERRAL_REASON, true))},
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
