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

#include "lynalliammedical.h"
#include "common/preprocessor_aid.h"
#include "common/textconst.h"
#include "db/fieldref.h"
#include "lib/stringfunc.h"
#include "lib/version.h"
#include "tasklib/taskfactory.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumultipleresponse.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"


const QString LynallIamMedical::LYNALL_IAM_MEDICAL_TABLENAME(
        "lynall_1_iam_medical");  // historically fixed

// "Sx" symptoms; "PH" personal history; "FH" family history
const QString FN_Q1_AGE_FIRST_INFLAMMATORY_SX("q1_age_first_inflammatory_sx");
const QString FN_Q2_WHEN_PSYCH_SX_STARTED("q2_when_psych_sx_started");
const QString FN_Q3_WORST_SYMPTOM_LAST_MONTH("q3_worst_symptom_last_month");
const QString FN_Q4A_SYMPTOM_TIMING("q4a_symptom_timing");
const QString FN_Q4B_DAYS_PSYCH_BEFORE_PHYS("q4b_days_psych_before_phys");
const QString FN_Q4C_DAYS_PSYCH_AFTER_PHYS("q4c_days_psych_after_phys");
const QString FN_Q5_ANTIBIOTICS("q5_antibiotics");
const QString FN_Q6A_INPATIENT_LAST_Y("q6a_inpatient_last_y");
const QString FN_Q6B_INPATIENT_WEEKS("q6b_inpatient_weeks");
const QString FN_Q7A_SX_LAST_2Y("q7a_sx_last_2y");
const QString FN_Q7B_VARIABILITY("q7b_variability");
const QString FN_Q8_SMOKING("q8_smoking");
const QString FN_Q9_PREGNANT("q9_pregnant");
const QString FN_Q10A_EFFECTIVE_RX_PHYSICAL("q10a_effective_rx_physical");
const QString FN_Q10B_EFFECTIVE_RX_PSYCH("q10b_effective_rx_psych");
const QString FN_Q11A_PH_DEPRESSION("q11a_ph_depression");
const QString FN_Q11B_PH_BIPOLAR("q11b_ph_bipolar");
const QString FN_Q11C_PH_SCHIZOPHRENIA("q11c_ph_schizophrenia");
const QString FN_Q11D_PH_AUTISTIC_SPECTRUM("q11d_ph_autistic_spectrum");
const QString FN_Q11E_PH_PTSD("q11e_ph_ptsd");
const QString FN_Q11F_PH_OTHER_ANXIETY("q11f_ph_other_anxiety");
const QString FN_Q11G_PH_PERSONALITY_DISORDER("q11g_ph_personality_disorder");
const QString FN_Q11H_PH_OTHER_PSYCH("q11h_ph_other_psych");
const QString FN_Q11H_PH_OTHER_DETAIL("q11h_ph_other_detail");
const QString FN_Q12A_FH_DEPRESSION("q12a_fh_depression");
const QString FN_Q12B_FH_BIPOLAR("q12b_fh_bipolar");
const QString FN_Q12C_FH_SCHIZOPHRENIA("q12c_fh_schizophrenia");
const QString FN_Q12D_FH_AUTISTIC_SPECTRUM("q12d_fh_autistic_spectrum");
const QString FN_Q12E_FH_PTSD("q12e_fh_ptsd");
const QString FN_Q12F_FH_OTHER_ANXIETY("q12f_fh_other_anxiety");
const QString FN_Q12G_FH_PERSONALITY_DISORDER("q12g_fh_personality_disorder");
const QString FN_Q12H_FH_OTHER_PSYCH("q12h_fh_other_psych");
const QString FN_Q12H_FH_OTHER_DETAIL("q12h_fh_other_detail");
const QString FN_Q13A_BEHCET("q13a_behcet");
const QString FN_Q13B_ORAL_ULCERS("q13b_oral_ulcers");
const QString FN_Q13C_ORAL_AGE_FIRST("q13c_oral_age_first");
const QString FN_Q13D_ORAL_SCARRING("q13d_oral_scarring");
const QString FN_Q13E_GENITAL_ULCERS("q13e_genital_ulcers");
const QString FN_Q13F_GENITAL_AGE_FIRST("q13f_genital_age_first");
const QString FN_Q13G_GENITAL_SCARRING("q13g_genital_scarring");

const int Q2_N_OPTIONS = 6;
const int Q3_N_OPTIONS = 11;
const int Q4_N_OPTIONS = 5;
const int Q4_OPTION_PSYCH_BEFORE_PHYSICAL = 1;
const int Q4_OPTION_PSYCH_AFTER_PHYSICAL = 2;

const int MIN_AGE_Y = 0;
const int MAX_AGE_Y = 150;
const int MIN_TIMING_DIFFERENCE_DAYS = 1;
const int MAX_TIMING_DIFFERENCE_DAYS = 100;
const int MIN_WEEKS_INPATIENT = 0;
const int MAX_WEEKS_INPATIENT = 52;
const int Q7B_MIN = 1;
const int Q7B_MAX = 10;

const QString TAG_4B("4B");
const QString TAG_4C("4C");
const QString TAG_6B("6B");
const QString TAG_7B("7B");
const QString TAG_11OTHER("11other");
const QString TAG_12OTHER("12other");
const QString TAG_13B("13B");
const QString TAG_13C("13C");
const QString TAG_13D("13D");
const QString TAG_13E("13E");
const QString TAG_13F("13F");
const QString TAG_13G("13G");


void initializeLynallIamMedical(TaskFactory& factory)
{
    static TaskRegistrar<LynallIamMedical> registered(factory);
}


LynallIamMedical::LynallIamMedical(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, LYNALL_IAM_MEDICAL_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addField(FN_Q1_AGE_FIRST_INFLAMMATORY_SX, QVariant::Int);
    addField(FN_Q2_WHEN_PSYCH_SX_STARTED, QVariant::Int);
    addField(FN_Q3_WORST_SYMPTOM_LAST_MONTH, QVariant::Int);
    addField(FN_Q4A_SYMPTOM_TIMING, QVariant::Int);
    addField(FN_Q4B_DAYS_PSYCH_BEFORE_PHYS, QVariant::Int);
    addField(FN_Q4C_DAYS_PSYCH_AFTER_PHYS, QVariant::Int);
    addField(FN_Q5_ANTIBIOTICS, QVariant::Bool);
    addField(FN_Q6A_INPATIENT_LAST_Y, QVariant::Bool);
    addField(FN_Q6B_INPATIENT_WEEKS, QVariant::Int);
    addField(FN_Q7A_SX_LAST_2Y, QVariant::Bool);
    addField(FN_Q7B_VARIABILITY, QVariant::Int);
    addField(FN_Q8_SMOKING, QVariant::Int);
    addField(FN_Q9_PREGNANT, QVariant::Bool);
    addField(FN_Q10A_EFFECTIVE_RX_PHYSICAL, QVariant::String);
    addField(FN_Q10B_EFFECTIVE_RX_PSYCH, QVariant::String);
    addField(FN_Q11A_PH_DEPRESSION, QVariant::Bool);
    addField(FN_Q11B_PH_BIPOLAR, QVariant::Bool);
    addField(FN_Q11C_PH_SCHIZOPHRENIA, QVariant::Bool);
    addField(FN_Q11D_PH_AUTISTIC_SPECTRUM, QVariant::Bool);
    addField(FN_Q11E_PH_PTSD, QVariant::Bool);
    addField(FN_Q11F_PH_OTHER_ANXIETY, QVariant::Bool);
    addField(FN_Q11G_PH_PERSONALITY_DISORDER, QVariant::Bool);
    addField(FN_Q11H_PH_OTHER_PSYCH, QVariant::Bool);
    addField(FN_Q11H_PH_OTHER_DETAIL, QVariant::String);
    addField(FN_Q12A_FH_DEPRESSION, QVariant::Bool);
    addField(FN_Q12B_FH_BIPOLAR, QVariant::Bool);
    addField(FN_Q12C_FH_SCHIZOPHRENIA, QVariant::Bool);
    addField(FN_Q12D_FH_AUTISTIC_SPECTRUM, QVariant::Bool);
    addField(FN_Q12E_FH_PTSD, QVariant::Bool);
    addField(FN_Q12F_FH_OTHER_ANXIETY, QVariant::Bool);
    addField(FN_Q12G_FH_PERSONALITY_DISORDER, QVariant::Bool);
    addField(FN_Q12H_FH_OTHER_PSYCH, QVariant::Bool);
    addField(FN_Q12H_FH_OTHER_DETAIL, QVariant::String);
    addField(FN_Q13A_BEHCET, QVariant::Bool);
    addField(FN_Q13B_ORAL_ULCERS, QVariant::Bool);
    addField(FN_Q13C_ORAL_AGE_FIRST, QVariant::Int);
    addField(FN_Q13D_ORAL_SCARRING, QVariant::Bool);
    addField(FN_Q13E_GENITAL_ULCERS, QVariant::Bool);
    addField(FN_Q13F_GENITAL_AGE_FIRST, QVariant::Int);
    addField(FN_Q13G_GENITAL_SCARRING, QVariant::Bool);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString LynallIamMedical::shortname() const
{
    return "Lynall_IAM_Medical";
}


QString LynallIamMedical::longname() const
{
    return tr("Lynall M-E — IAM — Medical history");
}


QString LynallIamMedical::description() const
{
    return tr("Medical history details for IAM immunopsychiatry study.");
}


Version LynallIamMedical::minimumServerVersion() const
{
    return Version(2, 3, 3);
}


QString LynallIamMedical::xstringTaskname() const
{
    return "lynall_iam_medical";
}


QString LynallIamMedical::infoFilenameStem() const
{
    return xstringTaskname();
}


// ============================================================================
// Instance info
// ============================================================================

bool LynallIamMedical::isComplete() const
{
    if (anyValuesNull({
                      FN_Q1_AGE_FIRST_INFLAMMATORY_SX,
                      FN_Q2_WHEN_PSYCH_SX_STARTED,
                      FN_Q3_WORST_SYMPTOM_LAST_MONTH,
                      FN_Q4A_SYMPTOM_TIMING,
                      FN_Q5_ANTIBIOTICS,
                      FN_Q6A_INPATIENT_LAST_Y,
                      FN_Q7A_SX_LAST_2Y,
                      FN_Q8_SMOKING,
                      FN_Q9_PREGNANT,
                      FN_Q10A_EFFECTIVE_RX_PHYSICAL,
                      FN_Q10B_EFFECTIVE_RX_PSYCH,
                      FN_Q13A_BEHCET,
            })) {
        return false;
    }
    if (anyValuesNullOrEmpty({
                      FN_Q10A_EFFECTIVE_RX_PHYSICAL,
                      FN_Q10B_EFFECTIVE_RX_PSYCH,
            })) {
        return false;
    }
    const int q4a = valueInt(FN_Q4A_SYMPTOM_TIMING);
    if (q4a == Q4_OPTION_PSYCH_BEFORE_PHYSICAL &&
            valueIsNull(FN_Q4B_DAYS_PSYCH_BEFORE_PHYS)) {
        return false;
    }
    if (q4a == Q4_OPTION_PSYCH_AFTER_PHYSICAL &&
            valueIsNull(FN_Q4C_DAYS_PSYCH_AFTER_PHYS)) {
        return false;
    }
    if (valueBool(FN_Q6A_INPATIENT_LAST_Y) && valueIsNull(FN_Q6B_INPATIENT_WEEKS)) {
        return false;
    }
    if (valueBool(FN_Q7A_SX_LAST_2Y) && valueIsNull(FN_Q7B_VARIABILITY)) {
        return false;
    }
    if (valueBool(FN_Q11H_PH_OTHER_PSYCH) &&
            valueIsNullOrEmpty(FN_Q11H_PH_OTHER_DETAIL)) {
        return false;
    }
    if (valueBool(FN_Q12H_FH_OTHER_PSYCH) &&
            valueIsNullOrEmpty(FN_Q12H_FH_OTHER_DETAIL)) {
        return false;
    }
    if (valueBool(FN_Q13A_BEHCET)) {
        if (anyValuesNull({FN_Q13B_ORAL_ULCERS, FN_Q13E_GENITAL_ULCERS})) {
            return false;
        }
        if (valueBool(FN_Q13B_ORAL_ULCERS)) {
            if (anyValuesNull({FN_Q13C_ORAL_AGE_FIRST,
                               FN_Q13D_ORAL_SCARRING})) {
                return false;
            }
        }
        if (valueBool(FN_Q13E_GENITAL_ULCERS)) {
            if (anyValuesNull({FN_Q13F_GENITAL_AGE_FIRST,
                               FN_Q13E_GENITAL_ULCERS})) {
                return false;
            }
        }
    }
    return true;
}


QStringList LynallIamMedical::summary() const
{
    return QStringList{textconst.noSummarySeeFacsimile()};
}


QStringList LynallIamMedical::detail() const
{
    return QStringList{textconst.noDetailSeeFacsimile()};
}


OpenableWidget* LynallIamMedical::editor(const bool read_only)
{
    using stringfunc::strnum;
    QVector<QuElement*> elements;
    const NameValueOptions yn_options = CommonOptions::yesNoBoolean();

    int pagenum = 1;
    QVector<QuPage*> pages;
    auto addPage = [this, &pagenum, &pages]
            (std::initializer_list<QuElement*> elements) -> void {
        const QString title = xstring(QString("q%1_title").arg(pagenum++));
        auto page = new QuPage(elements);
        page->setTitle(title);
        pages.append(page);
    };
    auto qtext = [this](const QString& xstringname) -> QuText* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto ynQuestion = [this, &yn_options](const QString& fieldname) -> QuMcq* {
        auto mcq = new QuMcq(fieldRef(fieldname), yn_options);
        mcq->setHorizontal(true);
        return mcq;
    };

    // Q1
    addPage({
        qtext("q1_question"),
        new QuLineEditInteger(fieldRef(FN_Q1_AGE_FIRST_INFLAMMATORY_SX),
                              MIN_AGE_Y, MAX_AGE_Y)
    });

    // Q2
    addPage({
        qtext("q2_question"),
        new QuMcq(fieldRef(FN_Q2_WHEN_PSYCH_SX_STARTED),
                  makeOptionsFromXstrings("q2_option", 1, Q2_N_OPTIONS))
    });

    // Q3
    addPage({
        qtext("q3_question"),
        new QuMcq(fieldRef(FN_Q3_WORST_SYMPTOM_LAST_MONTH),
                  makeOptionsFromXstrings("q3_option", 1, Q3_N_OPTIONS))
    });

    // Q4
    addPage({
        qtext("q4a_question"),
        new QuMcq(fieldRef(FN_Q4A_SYMPTOM_TIMING),
                  makeOptionsFromXstrings("q4a_option", 1, Q4_N_OPTIONS)),
        qtext("q4b_question")->addTag(TAG_4B),
        (new QuLineEditInteger(fieldRef(FN_Q4B_DAYS_PSYCH_BEFORE_PHYS),
                               MIN_TIMING_DIFFERENCE_DAYS,
                               MAX_TIMING_DIFFERENCE_DAYS))
                ->addTag(TAG_4B),
        qtext("q4c_question")->addTag(TAG_4C),
        (new QuLineEditInteger(fieldRef(FN_Q4C_DAYS_PSYCH_AFTER_PHYS),
                               MIN_TIMING_DIFFERENCE_DAYS,
                               MAX_TIMING_DIFFERENCE_DAYS))
                ->addTag(TAG_4C),
    });

    // Q5
    addPage({
        qtext("q5_question"),
        ynQuestion(FN_Q5_ANTIBIOTICS)
    });

    // Q6
    addPage({
        qtext("q6a_question"),
        ynQuestion(FN_Q6A_INPATIENT_LAST_Y),
        qtext("q6b_question")->addTag(TAG_6B),
        (new QuLineEditInteger(fieldRef(FN_Q6B_INPATIENT_WEEKS),
                               MIN_WEEKS_INPATIENT,
                               MAX_WEEKS_INPATIENT))
                ->addTag(TAG_6B)
    });

    // Q7
    const NameValueOptions q7a_options({
        {xstring("q7a_option1"), 1},
        {xstring("q7a_option0"), 0},
    });
    NameValueOptions q7b_options = NameValueOptions::makeNumbers(Q7B_MIN, Q7B_MAX);
    q7b_options.replace(NameValuePair("1: " + xstring("q7b_anchor_1"), 1));
    q7b_options.replace(NameValuePair("10: " + xstring("q7b_anchor_10"), 10));
    addPage({
        qtext("q7a_question"),
        new QuMcq(fieldRef(FN_Q7A_SX_LAST_2Y), q7a_options),
        qtext("q7b_question")->addTag(TAG_7B),
        // The text is very long, so even a vertical slider looks silly.
        (new QuMcq(fieldRef(FN_Q7B_VARIABILITY), q7b_options))
                ->addTag(TAG_7B)
    });

    // Q8
    addPage({
        qtext("q8_question"),
        new QuMcq(fieldRef(FN_Q8_SMOKING),
                  makeOptionsFromXstrings("q8_option", 2, 0))
    });

    // Q9
    addPage({
        qtext("q9_question"),
        new QuMcq(fieldRef(FN_Q9_PREGNANT),
                  makeOptionsFromXstrings("q9_option", 1, 0))
    });

    // Q10
    addPage({
        qtext("q10_stem"),
        qtext("q10a_question"),
        new QuTextEdit(fieldRef(FN_Q10A_EFFECTIVE_RX_PHYSICAL)),
        qtext("q10b_question"),
        new QuTextEdit(fieldRef(FN_Q10B_EFFECTIVE_RX_PSYCH))
    });

    // Q11
    const QString depression = xstring("depression");
    const QString bipolar = xstring("bipolar");
    const QString schizophrenia = xstring("schizophrenia");
    const QString autistic_spectrum = xstring("autistic_spectrum");
    const QString ptsd = xstring("ptsd");
    const QString other_anxiety = xstring("other_anxiety");
    const QString personality_disorder = xstring("personality_disorder");
    const QString other_psych = xstring("other_psych");
    QVector<QuestionWithOneField> q11_parts{
        {depression, fieldRef(FN_Q11A_PH_DEPRESSION)},
        {bipolar, fieldRef(FN_Q11B_PH_BIPOLAR)},
        {schizophrenia, fieldRef(FN_Q11C_PH_SCHIZOPHRENIA)},
        {autistic_spectrum, fieldRef(FN_Q11D_PH_AUTISTIC_SPECTRUM)},
        {ptsd, fieldRef(FN_Q11E_PH_PTSD)},
        {other_anxiety, fieldRef(FN_Q11F_PH_OTHER_ANXIETY)},
        {personality_disorder, fieldRef(FN_Q11G_PH_PERSONALITY_DISORDER)},
        {other_psych, fieldRef(FN_Q11H_PH_OTHER_PSYCH)},
    };
    addPage({
        qtext("q11_question"),
        new QuMultipleResponse(q11_parts),
        (new QuTextEdit(fieldRef(FN_Q11H_PH_OTHER_DETAIL)))
                ->addTag(TAG_11OTHER)
    });

    // Q12
    QVector<QuestionWithOneField> q12_parts{
        {depression, fieldRef(FN_Q12A_FH_DEPRESSION)},
        {bipolar, fieldRef(FN_Q12B_FH_BIPOLAR)},
        {schizophrenia, fieldRef(FN_Q12C_FH_SCHIZOPHRENIA)},
        {autistic_spectrum, fieldRef(FN_Q12D_FH_AUTISTIC_SPECTRUM)},
        {ptsd, fieldRef(FN_Q12E_FH_PTSD)},
        {other_anxiety, fieldRef(FN_Q12F_FH_OTHER_ANXIETY)},
        {personality_disorder, fieldRef(FN_Q12G_FH_PERSONALITY_DISORDER)},
        {other_psych, fieldRef(FN_Q12H_FH_OTHER_PSYCH)},
    };
    addPage({
        qtext("q12_question"),
        new QuMultipleResponse(q12_parts),
        (new QuTextEdit(fieldRef(FN_Q12H_FH_OTHER_DETAIL)))
                ->addTag(TAG_12OTHER)
    });

    // Q13
    // We add indentation via a grid.
    // It looks better to use fixed indentation with:
    //  - setExpandHorizontally(false)
    //  - setFixedGrid(false)
    //  - QuSpacer(width, 0)
    // than to have a variable grid with
    //  - setColumnStretch(0, 5)
    //  - setColumnStretch(1, 5)
    //  - setColumnStretch(2, 90)
    auto grid = new QuGridContainer();
    grid->setExpandHorizontally(false);
    grid->setFixedGrid(false);
    int row = 0;
    const Qt::Alignment align = Qt::AlignTop | Qt::AlignLeft;
    const int indent_px = 25;

    // For the following function, lambda capture of indent_px is not required
    // by GCC, and the Qt UI (clang) says "lambda capture 'indent_px' is not
    // required to be captured for this use". However, without it, Visual C++
    // says "'indent_px' cannot be implicitly captured because no default
    // capture mode has been specified". The clang perspective is described at
    // https://stackoverflow.com/questions/43467095/why-is-a-const-variable-sometimes-not-required-to-be-captured-in-a-lambda
    auto addCell = [&grid, &row, &align
#ifdef COMPILER_WANTS_EXPLICIT_LAMBDA_CAPTURES
            , &indent_px
#endif
            ](int level, const QString& tag, QuElement* element) -> void {
        const int rowspan = 1;
        const int col = level;
        const int colspan = 3 - level;
        for (int i = 0; i < col; ++i) {
            auto spacer = new QuSpacer(QSize(indent_px, 0));
            spacer->addTag(tag);
            grid->addCell(QuGridCell(spacer, row, i, 1, 1));
        }
        element->addTag(tag);
        grid->addCell(QuGridCell(element, row++, col, rowspan, colspan, align));
    };
    addCell(1, TAG_13B, qtext("q13b_question"));
    addCell(1, TAG_13B, ynQuestion(FN_Q13B_ORAL_ULCERS));
    addCell(2, TAG_13C, qtext("q13c_question"));
    addCell(2, TAG_13C, new QuLineEditInteger(fieldRef(FN_Q13C_ORAL_AGE_FIRST),
                                              MIN_AGE_Y, MAX_AGE_Y));
    addCell(2, TAG_13D, qtext("q13d_question"));
    addCell(2, TAG_13D, ynQuestion(FN_Q13D_ORAL_SCARRING));
    addCell(1, TAG_13E, qtext("q13e_question"));
    addCell(1, TAG_13E, ynQuestion(FN_Q13E_GENITAL_ULCERS));
    addCell(2, TAG_13F, qtext("q13f_question"));
    addCell(2, TAG_13F, new QuLineEditInteger(fieldRef(FN_Q13F_GENITAL_AGE_FIRST),
                                              MIN_AGE_Y, MAX_AGE_Y));
    addCell(2, TAG_13G, qtext("q13g_question"));
    addCell(2, TAG_13G, ynQuestion(FN_Q13G_GENITAL_SCARRING));
    addPage({
        qtext("q13a_question"),
        ynQuestion(FN_Q13A_BEHCET),
        grid,
    });

    // Signals
    connect(fieldRef(FN_Q4A_SYMPTOM_TIMING).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);
    connect(fieldRef(FN_Q6A_INPATIENT_LAST_Y).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);
    connect(fieldRef(FN_Q7A_SX_LAST_2Y).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);
    connect(fieldRef(FN_Q11H_PH_OTHER_PSYCH).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);
    connect(fieldRef(FN_Q12H_FH_OTHER_PSYCH).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);
    connect(fieldRef(FN_Q13A_BEHCET).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);
    connect(fieldRef(FN_Q13B_ORAL_ULCERS).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);
    connect(fieldRef(FN_Q13E_GENITAL_ULCERS).data(), &FieldRef::valueChanged,
            this, &LynallIamMedical::updateMandatory);

    // Questionnaire
    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);
    updateMandatory();
    return m_questionnaire;
}


// ============================================================================
// Signal handlers
// ============================================================================

void LynallIamMedical::updateMandatory()
{
    const bool need_q4b_before =
            valueInt(FN_Q4A_SYMPTOM_TIMING) == Q4_OPTION_PSYCH_BEFORE_PHYSICAL;
    const bool need_q4c_after =
            valueInt(FN_Q4A_SYMPTOM_TIMING) == Q4_OPTION_PSYCH_AFTER_PHYSICAL;
    const bool need_inpatient_time = valueBool(FN_Q6A_INPATIENT_LAST_Y);
    const bool need_variability = valueBool(FN_Q7A_SX_LAST_2Y);
    const bool need_ph_other = valueBool(FN_Q11H_PH_OTHER_PSYCH);
    const bool need_fh_other = valueBool(FN_Q12H_FH_OTHER_PSYCH);

    const bool need_behcet = valueBool(FN_Q13A_BEHCET);
    const bool need_oral = need_behcet && valueBool(FN_Q13B_ORAL_ULCERS);
    const bool need_genital = need_behcet && valueBool(FN_Q13E_GENITAL_ULCERS);

    fieldRef(FN_Q4B_DAYS_PSYCH_BEFORE_PHYS)->setMandatory(need_q4b_before);
    fieldRef(FN_Q4C_DAYS_PSYCH_AFTER_PHYS)->setMandatory(need_q4c_after);
    fieldRef(FN_Q6B_INPATIENT_WEEKS)->setMandatory(need_inpatient_time);
    fieldRef(FN_Q7B_VARIABILITY)->setMandatory(need_variability);
    fieldRef(FN_Q11H_PH_OTHER_DETAIL)->setMandatory(need_ph_other);
    fieldRef(FN_Q12H_FH_OTHER_DETAIL)->setMandatory(need_fh_other);

    fieldRef(FN_Q13B_ORAL_ULCERS)->setMandatory(need_behcet);
    fieldRef(FN_Q13C_ORAL_AGE_FIRST)->setMandatory(need_oral);
    fieldRef(FN_Q13D_ORAL_SCARRING)->setMandatory(need_oral);
    fieldRef(FN_Q13E_GENITAL_ULCERS)->setMandatory(need_behcet);
    fieldRef(FN_Q13F_GENITAL_AGE_FIRST)->setMandatory(need_genital);
    fieldRef(FN_Q13G_GENITAL_SCARRING)->setMandatory(need_genital);

    if (!m_questionnaire) {
        return;
    }
    const bool current_page_only = false;
    m_questionnaire->setVisibleByTag(TAG_4B, need_q4b_before, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_4C, need_q4c_after, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_6B, need_inpatient_time, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_7B, need_variability, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_11OTHER, need_ph_other, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_12OTHER, need_fh_other, current_page_only);

    m_questionnaire->setVisibleByTag(TAG_13B, need_behcet, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_13C, need_oral, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_13D, need_oral, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_13E, need_behcet, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_13F, need_genital, current_page_only);
    m_questionnaire->setVisibleByTag(TAG_13G, need_genital, current_page_only);

}
