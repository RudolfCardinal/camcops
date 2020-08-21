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

#include "perinatalpoem.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"

// Table name
const QString PerinatalPoem::PERINATAL_POEM_TABLENAME("perinatal_poem");

// Field names
const QString FN_QA_RESPONDENT("qa");
const QString FN_QB_SERVICE_TYPE("qb");
const QString FN_Q1A_MH_FIRST_CONTACT("q1a");
const QString FN_Q1B_MH_DISCHARGE("q1b");
const QString FN_Q2A_STAFF_DID_NOT_COMMUNICATE("q2a");
const QString FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT("q2b");
const QString FN_Q2C_HELP_NOT_QUICK_ENOUGH("q2c");
const QString FN_Q2D_STAFF_LISTENED("q2d");
const QString FN_Q2E_STAFF_DID_NOT_INVOLVE_ME("q2e");
const QString FN_Q2F_SERVICE_PROVIDED_INFO("q2f");
const QString FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME("q2g");
const QString FN_Q2H_STAFF_HELPED_ME_UNDERSTAND("q2h");
const QString FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY("q2i");
const QString FN_Q2J_STAFF_HELPED_MY_CONFIDENCE("q2j");
const QString FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY("q2k");
const QString FN_Q2L_I_WOULD_RECOMMEND_SERVICE("q2l");
const QString FN_Q3A_UNIT_CLEAN("q3a");
const QString FN_Q3B_UNIT_NOT_GOOD_PLACE_TO_RECOVER("q3b");
const QString FN_Q3C_UNIT_DID_NOT_PROVIDE_ACTIVITIES("q3c");
const QString FN_Q3D_UNIT_GOOD_PLACE_FOR_BABY("q3d");
const QString FN_Q3E_UNIT_SUPPORTED_FAMILY_FRIENDS_CONTACT("q3e");
const QString FN_Q3F_FOOD_NOT_ACCEPTABLE("q3f");
const QString FN_GENERAL_COMMENTS("general_comments");
const QString FN_FUTURE_PARTICIPATION("future_participation");
const QString FN_CONTACT_DETAILS("contact_details");

// Response values
const int VAL_QA_PATIENT = 1;
const int VAL_QA_PARTNER_OTHER = 2;

const int VAL_QB_INPATIENT = 1;  // inpatient = MBU = mother and baby unit
const int VAL_QB_COMMUNITY = 2;

const int VAL_Q1_VERY_WELL = 1;
const int VAL_Q1_WELL = 2;
const int VAL_Q1_UNWELL = 3;
const int VAL_Q1_VERY_UNWELL = 4;
const int VAL_Q1_EXTREMELY_UNWELL = 5;

const int VAL_STRONGLY_AGREE = 1;
const int VAL_AGREE = 2;
const int VAL_DISAGREE = 3;
const int VAL_STRONGLY_DISAGREE = 4;

// Tags
const QString TAG_RESPONDENT("resp");
const QString TAG_MBU("mbu");
const QString TAG_CONTACT_DETAILS("contact");


// ============================================================================
// Register task
// ============================================================================

void initializePerinatalPoem(TaskFactory& factory)
{
    static TaskRegistrar<PerinatalPoem> registered(factory);
}


// ============================================================================
// Constructor
// ============================================================================

PerinatalPoem::PerinatalPoem(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, PERINATAL_POEM_TABLENAME, true, false, false)  // ... anon, clin, resp
    // no need, QPointer handles this: // m_questionnaire(nullptr)
{
    addField(FN_QA_RESPONDENT, QVariant::Int);
    addField(FN_QB_SERVICE_TYPE, QVariant::Int);
    addField(FN_Q1A_MH_FIRST_CONTACT, QVariant::Int);
    addField(FN_Q1B_MH_DISCHARGE, QVariant::Int);
    addField(FN_Q2A_STAFF_DID_NOT_COMMUNICATE, QVariant::Int);
    addField(FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT, QVariant::Int);
    addField(FN_Q2C_HELP_NOT_QUICK_ENOUGH, QVariant::Int);
    addField(FN_Q2D_STAFF_LISTENED, QVariant::Int);
    addField(FN_Q2E_STAFF_DID_NOT_INVOLVE_ME, QVariant::Int);
    addField(FN_Q2F_SERVICE_PROVIDED_INFO, QVariant::Int);
    addField(FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME, QVariant::Int);
    addField(FN_Q2H_STAFF_HELPED_ME_UNDERSTAND, QVariant::Int);
    addField(FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY, QVariant::Int);
    addField(FN_Q2J_STAFF_HELPED_MY_CONFIDENCE, QVariant::Int);
    addField(FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY, QVariant::Int);
    addField(FN_Q2L_I_WOULD_RECOMMEND_SERVICE, QVariant::Int);
    addField(FN_Q3A_UNIT_CLEAN, QVariant::Int);
    addField(FN_Q3B_UNIT_NOT_GOOD_PLACE_TO_RECOVER, QVariant::Int);
    addField(FN_Q3C_UNIT_DID_NOT_PROVIDE_ACTIVITIES, QVariant::Int);
    addField(FN_Q3D_UNIT_GOOD_PLACE_FOR_BABY, QVariant::Int);
    addField(FN_Q3E_UNIT_SUPPORTED_FAMILY_FRIENDS_CONTACT, QVariant::Int);
    addField(FN_Q3F_FOOD_NOT_ACCEPTABLE, QVariant::Int);
    addField(FN_GENERAL_COMMENTS, QVariant::String);
    addField(FN_FUTURE_PARTICIPATION, QVariant::String);
    addField(FN_CONTACT_DETAILS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString PerinatalPoem::shortname() const
{
    return "Perinatal-POEM";
}


QString PerinatalPoem::longname() const
{
    return tr("Perinatal Patient-rated Outcome and Experience Measure");
}


QString PerinatalPoem::description() const
{
    return tr("2 questions on mental health; 12 questions on patient "
              "experience; ±6 questions specific to mother/baby units.");
}


// ============================================================================
// Instance info
// ============================================================================

bool PerinatalPoem::isComplete() const
{
    const QStringList required_always{
        FN_QA_RESPONDENT,
        FN_QB_SERVICE_TYPE,
        FN_Q1A_MH_FIRST_CONTACT,
        FN_Q1B_MH_DISCHARGE,
        FN_Q2A_STAFF_DID_NOT_COMMUNICATE,
        FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT,
        FN_Q2C_HELP_NOT_QUICK_ENOUGH,
        FN_Q2D_STAFF_LISTENED,
        FN_Q2E_STAFF_DID_NOT_INVOLVE_ME,
        FN_Q2F_SERVICE_PROVIDED_INFO,
        FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME,
        FN_Q2H_STAFF_HELPED_ME_UNDERSTAND,
        FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY,
        FN_Q2J_STAFF_HELPED_MY_CONFIDENCE,
        FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY,
        FN_Q2L_I_WOULD_RECOMMEND_SERVICE,
        // not FN_GENERAL_COMMENTS,
        FN_FUTURE_PARTICIPATION,
        // not FN_CONTACT_DETAILS,
    };
    if (anyValuesNull(required_always)) {
        return false;
    }
    if (wasInpatient()) {
        const QStringList required_inpatient{
            FN_Q3A_UNIT_CLEAN,
            FN_Q3B_UNIT_NOT_GOOD_PLACE_TO_RECOVER,
            FN_Q3C_UNIT_DID_NOT_PROVIDE_ACTIVITIES,
            FN_Q3D_UNIT_GOOD_PLACE_FOR_BABY,
            FN_Q3E_UNIT_SUPPORTED_FAMILY_FRIENDS_CONTACT,
            FN_Q3F_FOOD_NOT_ACCEPTABLE,
        };
        if (anyValuesNull(required_inpatient)) {
            return false;
        }
    }
    return true;
}


QStringList PerinatalPoem::summary() const
{
    return QStringList{"No summary; see facsimile."};
}


OpenableWidget* PerinatalPoem::editor(const bool read_only)
{
    int pagenum = 1;
    const QString pagetitle = xstring("pagetitle");
    const QString note_to_respondent = xstring("note_to_respondent");
    const NameValueOptions options_agreement{
        {xstring("agreement_a1"), VAL_STRONGLY_AGREE},
        {xstring("agreement_a2"), VAL_AGREE},
        {xstring("agreement_a3"), VAL_DISAGREE},
        {xstring("agreement_a4"), VAL_STRONGLY_DISAGREE},
    };
    const NameValueOptions options_respondent{
        {xstring("qa_a1"), VAL_QA_PATIENT},
        {xstring("qa_a2"), VAL_QA_PARTNER_OTHER},
    };
    const NameValueOptions options_service{
        {xstring("qb_a1"), VAL_QB_INPATIENT},
        {xstring("qb_a2"), VAL_QB_COMMUNITY},
    };
    const NameValueOptions options_mh{
        {xstring("q1_a1"), VAL_Q1_VERY_WELL},
        {xstring("q1_a2"), VAL_Q1_WELL},
        {xstring("q1_a3"), VAL_Q1_UNWELL},
        {xstring("q1_a4"), VAL_Q1_VERY_UNWELL},
        {xstring("q1_a5"), VAL_Q1_EXTREMELY_UNWELL},
    };
    const NameValueOptions options_yn = CommonOptions::yesNoInteger();

    // ------------------------------------------------------------------------
    // Helper functions
    // ------------------------------------------------------------------------

    auto makeTitle = [&pagetitle, &pagenum]() -> QString {
        return pagetitle + QString(", page %1").arg(pagenum++);
    };
    auto makeNoteToRespondent = [&note_to_respondent]() -> QuText* {
        QuText* t = new QuText(note_to_respondent);
        t->setItalic();
        t->setTextAndWidgetAlignment(Qt::AlignTop | Qt::AlignCenter);
        t->addTag(TAG_RESPONDENT);
        return t;
    };
    auto makeRespondentSpacer = []() -> QuSpacer* {
        QuSpacer* s = new QuSpacer();
        s->addTag(TAG_RESPONDENT);
        return s;
    };
    auto makeQ = [this](const QString& xstringname) -> QuText* {
        return new QuText(xstring(xstringname));
    };
    auto makeGrid = [](const QVector<QuestionWithOneField>& question_field_pairs,
                       const NameValueOptions& options) -> QuMcqGrid* {
        const int n_options = options.size();
        const int width_per_option = 1;
        const QVector<int> option_widths(n_options, width_per_option);
        const int question_width = n_options;
        return (new QuMcqGrid(question_field_pairs, options))
            ->setQuestionsBold(false)
            ->setWidth(question_width, option_widths);
    };

    // ------------------------------------------------------------------------
    // Page 1
    // ------------------------------------------------------------------------

    QuPagePtr page_1((new QuPage{
        // note to respondent is already part of preamble text.
        new QuHeading(xstring("intro_title")),
        new QuText(xstring("intro_para_1")),
        new QuText(xstring("intro_para_2")),
        new QuText(xstring("intro_para_3")),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 2
    // ------------------------------------------------------------------------

    FieldRefPtr fr_respondent = fieldRef(FN_QA_RESPONDENT);
    connect(fr_respondent.data(), &FieldRef::valueChanged,
            this, &PerinatalPoem::respondentTypeChanged);

    FieldRefPtr fr_service = fieldRef(FN_QB_SERVICE_TYPE);
    connect(fr_service.data(), &FieldRef::valueChanged,
            this, &PerinatalPoem::serviceTypeChanged);

    QuPagePtr page_2((new QuPage{
        makeQ("qa_q"),
        new QuMcq(fieldRef(FN_QA_RESPONDENT), options_respondent),
        makeRespondentSpacer(),
        makeNoteToRespondent(),
        makeRespondentSpacer(),
        makeQ("qb_q"),
        new QuMcq(fr_service, options_service),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 3
    // ------------------------------------------------------------------------

    QuPagePtr page_3((new QuPage{
        makeNoteToRespondent(),
        makeRespondentSpacer(),
        makeQ("q1_stem"),
        makeGrid(
            {
                QuestionWithOneField(xstring("q1a_q"), fieldRef(FN_Q1A_MH_FIRST_CONTACT)),
                QuestionWithOneField(xstring("q1b_q"), fieldRef(FN_Q1B_MH_DISCHARGE)),
            },
            options_mh
        ),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 4
    // ------------------------------------------------------------------------

    QuPagePtr page_4((new QuPage{
        makeNoteToRespondent(),
        makeRespondentSpacer(),
        makeQ("q2_stem"),
        makeGrid(
            {
                QuestionWithOneField(xstring("q2a_q"), fieldRef(FN_Q2A_STAFF_DID_NOT_COMMUNICATE)),
                QuestionWithOneField(xstring("q2b_q"), fieldRef(FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT)),
                QuestionWithOneField(xstring("q2c_q"), fieldRef(FN_Q2C_HELP_NOT_QUICK_ENOUGH)),
                QuestionWithOneField(xstring("q2d_q"), fieldRef(FN_Q2D_STAFF_LISTENED)),
            },
            options_agreement
        ),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 5
    // ------------------------------------------------------------------------

    QuPagePtr page_5((new QuPage{
        makeNoteToRespondent(),
        makeRespondentSpacer(),
        makeQ("q2_stem"),
        makeGrid(
            {
                QuestionWithOneField(xstring("q2e_q"), fieldRef(FN_Q2E_STAFF_DID_NOT_INVOLVE_ME)),
                QuestionWithOneField(xstring("q2f_q"), fieldRef(FN_Q2F_SERVICE_PROVIDED_INFO)),
                QuestionWithOneField(xstring("q2g_q"), fieldRef(FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME)),
                QuestionWithOneField(xstring("q2h_q"), fieldRef(FN_Q2H_STAFF_HELPED_ME_UNDERSTAND)),
            },
            options_agreement
        ),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 6
    // ------------------------------------------------------------------------

    QuPagePtr page_6((new QuPage{
        makeNoteToRespondent(),
        makeRespondentSpacer(),
        makeQ("q2_stem"),
        makeGrid(
            {
                QuestionWithOneField(xstring("q2i_q"), fieldRef(FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY)),
                QuestionWithOneField(xstring("q2j_q"), fieldRef(FN_Q2J_STAFF_HELPED_MY_CONFIDENCE)),
                QuestionWithOneField(xstring("q2k_q"), fieldRef(FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY)),
                QuestionWithOneField(xstring("q2l_q"), fieldRef(FN_Q2L_I_WOULD_RECOMMEND_SERVICE)),
            },
            options_agreement
        ),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 7
    // ------------------------------------------------------------------------

    QuPagePtr page_7((new QuPage{
        makeNoteToRespondent(),
        makeRespondentSpacer(),
        makeQ("q3_stem")->addTag(TAG_MBU),
        makeGrid(
            {
                QuestionWithOneField(xstring("q3a_q"), fieldRef(FN_Q3A_UNIT_CLEAN)),
                QuestionWithOneField(xstring("q3b_q"), fieldRef(FN_Q3B_UNIT_NOT_GOOD_PLACE_TO_RECOVER)),
                QuestionWithOneField(xstring("q3c_q"), fieldRef(FN_Q3C_UNIT_DID_NOT_PROVIDE_ACTIVITIES)),
                QuestionWithOneField(xstring("q3d_q"), fieldRef(FN_Q3D_UNIT_GOOD_PLACE_FOR_BABY)),
                QuestionWithOneField(xstring("q3e_q"), fieldRef(FN_Q3E_UNIT_SUPPORTED_FAMILY_FRIENDS_CONTACT)),
                QuestionWithOneField(xstring("q3f_q"), fieldRef(FN_Q3F_FOOD_NOT_ACCEPTABLE)),
            },
            options_agreement
        )->addTag(TAG_MBU),
        makeQ("general_comments_q"),
        new QuTextEdit(fieldRef(FN_GENERAL_COMMENTS, false)),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 8
    // ------------------------------------------------------------------------

    m_fr_participation = fieldRef(FN_FUTURE_PARTICIPATION);
    connect(m_fr_participation.data(), &FieldRef::valueChanged,
            this, &PerinatalPoem::participationChanged);
    m_fr_contact_details = fieldRef(FN_CONTACT_DETAILS);

    QuPagePtr page_8((new QuPage{
        makeNoteToRespondent(),
        makeRespondentSpacer(),
        makeQ("participation_q"),
        new QuMcq(m_fr_participation, options_yn),
        makeQ("contact_details_q")->addTag(TAG_CONTACT_DETAILS),
        (new QuTextEdit(m_fr_contact_details))->addTag(TAG_CONTACT_DETAILS),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Page 9
    // ------------------------------------------------------------------------

    QuPagePtr page_9((new QuPage{
        new QuText(xstring("conclusion_thanks")),
        new QuText(xstring("contact_info_pqn_project_team")),
    })->setTitle(makeTitle()));

    // ------------------------------------------------------------------------
    // Questionnaire
    // ------------------------------------------------------------------------

    m_questionnaire = new Questionnaire(m_app, {
        page_1, page_2, page_3, page_4, page_5, page_6, page_7, page_8, page_9,
    });
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    // ------------------------------------------------------------------------
    // Per-page signals set above. Now set initial dynamic state:
    // ------------------------------------------------------------------------

    respondentTypeChanged();
    serviceTypeChanged();
    participationChanged();

    // ------------------------------------------------------------------------
    // Done
    // ------------------------------------------------------------------------

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

bool PerinatalPoem::wasInpatient() const
{
    return valueInt(FN_QB_SERVICE_TYPE) == VAL_QB_INPATIENT;
}


bool PerinatalPoem::respondentNotPatient() const
{
    return valueInt(FN_QA_RESPONDENT) == VAL_QA_PARTNER_OTHER;
}


bool PerinatalPoem::offeringParticipation() const
{
    return valueInt(FN_FUTURE_PARTICIPATION) == CommonOptions::YES_INT;
}


// ============================================================================
// Signal handlers
// ============================================================================

void PerinatalPoem::respondentTypeChanged()
{
    if (!m_questionnaire) {
        return;
    }
    const bool visible = respondentNotPatient();
    m_questionnaire->setVisibleByTag(TAG_RESPONDENT, visible, false);
}


void PerinatalPoem::serviceTypeChanged()
{
    if (!m_questionnaire) {
        return;
    }
    const bool visible = wasInpatient();
    m_questionnaire->setVisibleByTag(TAG_MBU, visible, false);
}


void PerinatalPoem::participationChanged()
{
    if (!m_fr_participation || !m_questionnaire) {
        return;
    }
    const bool mandatory = offeringParticipation();
    m_fr_contact_details->setMandatory(mandatory);
    m_questionnaire->setVisibleByTag(TAG_CONTACT_DETAILS, mandatory);
}
