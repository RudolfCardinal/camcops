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

#include "psychiatricclerking.h"
#include "common/textconst.h"
#include "lib/stringfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"

const QString PsychiatricClerking::PSYCLERK_TABLENAME("psychiatricclerking");

const QString LOCATION("location");
const QString CONTACT_TYPE("contact_type");
const QString REASON_FOR_CONTACT("reason_for_contact");
const QString PRESENTING_ISSUE("presenting_issue");
const QString SYSTEMS_REVIEW("systems_review");
const QString COLLATERAL_HISTORY("collateral_history");

const QString DIAGNOSES_PSYCHIATRIC("diagnoses_psychiatric");
const QString DIAGNOSES_MEDICAL("diagnoses_medical");
const QString OPERATIONS_PROCEDURES("operations_procedures");
const QString ALLERGIES_ADVERSE_REACTIONS("allergies_adverse_reactions");
const QString MEDICATIONS("medications");
const QString RECREATIONAL_DRUG_USE("recreational_drug_use");
const QString FAMILY_HISTORY("family_history");
const QString DEVELOPMENTAL_HISTORY("developmental_history");
const QString PERSONAL_HISTORY("personal_history");
const QString PREMORBID_PERSONALITY("premorbid_personality");
const QString FORENSIC_HISTORY("forensic_history");
const QString CURRENT_SOCIAL_SITUATION("current_social_situation");

const QString MSE_APPEARANCE_BEHAVIOUR("mse_appearance_behaviour");
const QString MSE_SPEECH("mse_speech");
const QString MSE_MOOD_SUBJECTIVE("mse_mood_subjective");
const QString MSE_MOOD_OBJECTIVE("mse_mood_objective");
const QString MSE_THOUGHT_FORM("mse_thought_form");
const QString MSE_THOUGHT_CONTENT("mse_thought_content");
const QString MSE_PERCEPTION("mse_perception");
const QString MSE_COGNITION("mse_cognition");
const QString MSE_INSIGHT("mse_insight");

const QString PHYSICAL_EXAMINATION_GENERAL("physical_examination_general");
const QString PHYSICAL_EXAMINATION_CARDIOVASCULAR("physical_examination_cardiovascular");
const QString PHYSICAL_EXAMINATION_RESPIRATORY("physical_examination_respiratory");
const QString PHYSICAL_EXAMINATION_ABDOMINAL("physical_examination_abdominal");
const QString PHYSICAL_EXAMINATION_NEUROLOGICAL("physical_examination_neurological");

const QString ASSESSMENT_SCALES("assessment_scales");
const QString INVESTIGATIONS_RESULTS("investigations_results");

const QString SAFETY_ALERTS("safety_alerts");
const QString RISK_ASSESSMENT("risk_assessment");
const QString RELEVANT_LEGAL_INFORMATION("relevant_legal_information");

const QString CURRENT_PROBLEMS("current_problems");
const QString PATIENT_CARER_CONCERNS("patient_carer_concerns");
const QString IMPRESSION("impression");
const QString MANAGEMENT_PLAN("management_plan");
const QString INFORMATION_GIVEN("information_given");

const QStringList EXTRAFIELDS_B{
    LOCATION,
    CONTACT_TYPE,
    REASON_FOR_CONTACT,
    PRESENTING_ISSUE,
    SYSTEMS_REVIEW,
    COLLATERAL_HISTORY,
};
const QStringList EXTRAFIELDS_C{
    DIAGNOSES_PSYCHIATRIC,
    DIAGNOSES_MEDICAL,
    OPERATIONS_PROCEDURES,
    ALLERGIES_ADVERSE_REACTIONS,
    MEDICATIONS,
    RECREATIONAL_DRUG_USE,
    FAMILY_HISTORY,
    DEVELOPMENTAL_HISTORY,
    PERSONAL_HISTORY,
    PREMORBID_PERSONALITY,
    FORENSIC_HISTORY,
    CURRENT_SOCIAL_SITUATION,
};
const QStringList EXTRAFIELDS_MSE{
    MSE_APPEARANCE_BEHAVIOUR,
    MSE_SPEECH,
    MSE_MOOD_SUBJECTIVE,
    MSE_MOOD_OBJECTIVE,
    MSE_THOUGHT_FORM,
    MSE_THOUGHT_CONTENT,
    MSE_PERCEPTION,
    MSE_COGNITION,
    MSE_INSIGHT,
};
const QStringList EXTRAFIELDS_PE{
    PHYSICAL_EXAMINATION_GENERAL,
    PHYSICAL_EXAMINATION_CARDIOVASCULAR,
    PHYSICAL_EXAMINATION_RESPIRATORY,
    PHYSICAL_EXAMINATION_ABDOMINAL,
    PHYSICAL_EXAMINATION_NEUROLOGICAL,
};
const QStringList EXTRAFIELDS_D{
    ASSESSMENT_SCALES,
    INVESTIGATIONS_RESULTS,
};
const QStringList EXTRAFIELDS_E{
    SAFETY_ALERTS,
    RISK_ASSESSMENT,
    RELEVANT_LEGAL_INFORMATION,
};
const QStringList EXTRAFIELDS_F{
    CURRENT_PROBLEMS,
    PATIENT_CARER_CONCERNS,
    IMPRESSION,
    MANAGEMENT_PLAN,
    INFORMATION_GIVEN,
};


void initializePsychiatricClerking(TaskFactory& factory)
{
    static TaskRegistrar<PsychiatricClerking> registered(factory);
}


PsychiatricClerking::PsychiatricClerking(CamcopsApp& app, DatabaseManager& db,
                                         const int load_pk) :
    Task(app, db, PSYCLERK_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    // In the Javascript version, EXTRAFIELDS_A was the set of clinician fields.
    addFields(EXTRAFIELDS_B, QVariant::String);
    addFields(EXTRAFIELDS_C, QVariant::String);
    addFields(EXTRAFIELDS_MSE, QVariant::String);
    addFields(EXTRAFIELDS_PE, QVariant::String);
    addFields(EXTRAFIELDS_D, QVariant::String);
    addFields(EXTRAFIELDS_E, QVariant::String);
    addFields(EXTRAFIELDS_F, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString PsychiatricClerking::shortname() const
{
    return "Clerking";
}


QString PsychiatricClerking::longname() const
{
    return tr("Psychiatric clerking");
}


QString PsychiatricClerking::description() const
{
    return tr("Standard psychiatric clerking form.");
}


QString PsychiatricClerking::infoFilenameStem() const
{
    return "clinical";
}


// ============================================================================
// Instance info
// ============================================================================

bool PsychiatricClerking::isComplete() const
{
    return true;
}


QStringList PsychiatricClerking::summary() const
{
    return QStringList{
        fieldSummary(LOCATION, TextConst::location(), ": "),
    };
}


QStringList PsychiatricClerking::detail() const
{
    QStringList lines = completenessInfo() + clinicianDetails();

    auto add = [this, &lines](const QStringList& fields) -> void {
        for (const QString& field : fields) {
            lines.append(fieldSummary(field, xstring(field), ": "));
        }
    };

    add(EXTRAFIELDS_B);
    add(EXTRAFIELDS_C);
    add(EXTRAFIELDS_MSE);
    add(EXTRAFIELDS_PE);
    add(EXTRAFIELDS_D);
    add(EXTRAFIELDS_E);
    add(EXTRAFIELDS_F);
    return lines;
}


OpenableWidget* PsychiatricClerking::editor(const bool read_only)
{
    QVector<QuElement*> elements{getClinicianQuestionnaireBlockRawPointer()};

    auto addGroup = [this, &elements](const QStringList& fields) -> void {
        for (const QString& field : fields) {
            elements.append(new QuText(xstring(field)));
            QuTextEdit* edit = new QuTextEdit(fieldRef(field, false));
            edit->setHint("");
            elements.append(edit);
        }
    };
    auto addHeading = [this, &elements](const QString& xstringname) -> void {
        elements.append(new QuHeading(xstring(xstringname)));
    };
    auto addSubheading = [this, &elements](const QString& xstringname) -> void {
        elements.append((new QuText(xstring(xstringname)))->setBold(true));
    };

    addHeading("heading_current_contact");
    addGroup(EXTRAFIELDS_B);

    addHeading("heading_background");
    addGroup(EXTRAFIELDS_C);

    addHeading("heading_examination_investigations");
    addSubheading("mental_state_examination");
    addGroup(EXTRAFIELDS_MSE);
    addSubheading("physical_examination");
    addGroup(EXTRAFIELDS_PE);
    addSubheading("assessments_and_investigations");
    addGroup(EXTRAFIELDS_D);

    addHeading("heading_risk_legal");
    addGroup(EXTRAFIELDS_E);

    addHeading("heading_summary_plan");
    addGroup(EXTRAFIELDS_F);

    QuPagePtr page((new QuPage(elements))->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}
