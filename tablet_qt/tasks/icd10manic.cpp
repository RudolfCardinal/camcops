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

#define SHOW_CLASSIFICATION_WORKING

#include "icd10manic.h"
#include "common/appstrings.h"
#include "common/textconst.h"
#include "lib/convert.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using datetime::shortDate;
using mathfunc::allFalse;
using mathfunc::anyNull;
using mathfunc::anyTrue;
using mathfunc::countNull;
using mathfunc::countTrue;
using mathfunc::falseNotNull;
using stringfunc::bold;
using stringfunc::standardResult;

const QString Icd10Manic::ICD10MANIC_TABLENAME("icd10manic");

const QString MOOD_ELEVATED("mood_elevated");
const QString MOOD_IRRITABLE("mood_irritable");
const QStringList CORE_NAMES{
    MOOD_ELEVATED,
    MOOD_IRRITABLE,
};
const QString DISTRACTIBLE("distractible");
const QString ACTIVITY("activity");
const QString SLEEP("sleep");
const QString TALKATIVENESS("talkativeness");
const QString RECKLESSNESS("recklessness");
const QString SOCIAL_DISINHIBITION("social_disinhibition");
const QString SEXUAL("sexual");
const QStringList HYPOMANIA_MANIA_NAMES{
    DISTRACTIBLE,
    ACTIVITY,
    SLEEP,
    TALKATIVENESS,
    RECKLESSNESS,
    SOCIAL_DISINHIBITION,
    SEXUAL,
};
const QString GRANDIOSITY("grandiosity");
const QString FLIGHT_OF_IDEAS("flight_of_ideas");
const QStringList MANIA_NAMES{
    GRANDIOSITY,
    FLIGHT_OF_IDEAS,
};
const QString SUSTAINED4DAYS("sustained4days");
const QString SUSTAINED7DAYS("sustained7days");
const QString ADMISSION_REQUIRED("admission_required");
const QString SOME_INTERFERENCE_FUNCTIONING("some_interference_functioning");
const QString SEVERE_INTERFERENCE_FUNCTIONING("severe_interference_functioning");
const QStringList OTHER_CRITERIA_NAMES{
    SUSTAINED4DAYS,
    SUSTAINED7DAYS,
    ADMISSION_REQUIRED,
    SOME_INTERFERENCE_FUNCTIONING,
    SEVERE_INTERFERENCE_FUNCTIONING,
};
const QString PERCEPTUAL_ALTERATIONS("perceptual_alterations");
const QString HALLUCINATIONS_SCHIZOPHRENIC("hallucinations_schizophrenic");
const QString HALLUCINATIONS_OTHER("hallucinations_other");
const QString DELUSIONS_SCHIZOPHRENIC("delusions_schizophrenic");
const QString DELUSIONS_OTHER("delusions_other");
const QStringList PSYCHOSIS_AND_SIMILAR_NAMES{
    PERCEPTUAL_ALTERATIONS,  // not psychotic
    HALLUCINATIONS_SCHIZOPHRENIC,
    HALLUCINATIONS_OTHER,
    DELUSIONS_SCHIZOPHRENIC,
    DELUSIONS_OTHER,
};
const QStringList PSYCHOSIS_NAMES{
    HALLUCINATIONS_SCHIZOPHRENIC,
    HALLUCINATIONS_OTHER,
    DELUSIONS_SCHIZOPHRENIC,
    DELUSIONS_OTHER,
};
const QStringList INFORMATIVE = CORE_NAMES + HYPOMANIA_MANIA_NAMES +
        MANIA_NAMES + OTHER_CRITERIA_NAMES + PSYCHOSIS_AND_SIMILAR_NAMES;
const QString DATE_PERTAINS_TO("date_pertains_to");
const QString COMMENTS("comments");


void initializeIcd10Manic(TaskFactory& factory)
{
    static TaskRegistrar<Icd10Manic> registered(factory);
}


Icd10Manic::Icd10Manic(CamcopsApp& app, DatabaseManager& db,
                       const int load_pk) :
    Task(app, db, ICD10MANIC_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(CORE_NAMES, QVariant::Bool);
    addFields(HYPOMANIA_MANIA_NAMES, QVariant::Bool);
    addFields(MANIA_NAMES, QVariant::Bool);
    addFields(OTHER_CRITERIA_NAMES, QVariant::Bool);
    addFields(PSYCHOSIS_AND_SIMILAR_NAMES, QVariant::Bool);

    addField(DATE_PERTAINS_TO, QVariant::Date);
    addField(COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(DATE_PERTAINS_TO, datetime::nowDate(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString Icd10Manic::shortname() const
{
    return "ICD10-mania";
}


QString Icd10Manic::longname() const
{
    return tr("ICD-10 symptomatic criteria for a manic/hypomanic episode "
              "(as in e.g. F06.3, F25, F30, F31)");
}


QString Icd10Manic::description() const
{
    return TextConst::icd10();
}


QString Icd10Manic::infoFilenameStem() const
{
    return "icd";
}


// ============================================================================
// Instance info
// ============================================================================

bool Icd10Manic::isComplete() const
{
    return !valueIsNull(DATE_PERTAINS_TO) && !meetsCriteriaNone().isNull();
}


QStringList Icd10Manic::summary() const
{
    return QStringList{
        standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                       shortDate(value(DATE_PERTAINS_TO))),
        standardResult(TextConst::category(), getDescription()),
    };
}


QStringList Icd10Manic::detail() const
{
    QStringList lines = completenessInfo();
    lines.append(standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                                shortDate(value(DATE_PERTAINS_TO))));
    lines.append(fieldSummary(COMMENTS,
                              TextConst::examinerComments()));
    lines.append("");
    lines += detailGroup(CORE_NAMES);
    lines += detailGroup(HYPOMANIA_MANIA_NAMES);
    lines += detailGroup(MANIA_NAMES);
    lines += detailGroup(OTHER_CRITERIA_NAMES);
    lines += detailGroup(PSYCHOSIS_AND_SIMILAR_NAMES);
    lines.append("");
    lines.append(standardResult(TextConst::category(), getDescription()));
#ifdef SHOW_CLASSIFICATION_WORKING
    auto pv = [](const QVariant& v) -> QString {
        return bold(convert::prettyValue(v));
    };
    lines.append("");
    lines.append("meetsCriteriaManiaPsychoticSchizophrenic(): " +
                 pv(meetsCriteriaManiaPsychoticSchizophrenic()));
    lines.append("meetsCriteriaManiaPsychoticIcd(): " +
                 pv(meetsCriteriaManiaPsychoticIcd()));
    lines.append("meetsCriteriaManiaNonpsychotic(): " +
                 pv(meetsCriteriaManiaNonpsychotic()));
    lines.append("meetsCriteriaManiaIgnoringPsychosis(): " +
                 pv(meetsCriteriaManiaIgnoringPsychosis()));
    lines.append("meetsCriteriaHypomania(): " + pv(meetsCriteriaHypomania()));
    lines.append("meetsCriteriaNone(): " + pv(meetsCriteriaNone()));
#endif
    return lines;
}


OpenableWidget* Icd10Manic::editor(const bool read_only)
{
    const NameValueOptions true_false_options = CommonOptions::falseTrueBoolean();

    auto heading = [this](const QString& xstringname) -> QuElement* {
        return new QuHeading(xstring(xstringname));
    };
    auto grid = [this](const QStringList& fields_xstrings,
                       const NameValueOptions& options,
                       bool mandatory) -> QuElement* {
        // Assumes the xstring name matches the fieldname (as it does)
        QVector<QuestionWithOneField> qfields;
        for (const QString& fieldname : fields_xstrings) {
            qfields.append(
                        QuestionWithOneField(xstring(fieldname),
                                             fieldRef(fieldname, mandatory)));
        }
        const int n = options.size();
        const QVector<int> v(n, 1);
        return (new QuMcqGrid(qfields, options))
                ->setExpand(true)
                ->setWidth(n, v);
    };

    QuPagePtr page((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        (new QuText(appstring(appstrings::ICD10_SYMPTOMATIC_DISCLAIMER)))->setBold(),
        new QuText(appstring(appstrings::DATE_PERTAINS_TO)),
        (new QuDateTime(fieldRef(DATE_PERTAINS_TO)))
            ->setMode(QuDateTime::Mode::DefaultDate)
            ->setOfferNowButton(true),
        heading("core"),
        grid(CORE_NAMES, true_false_options, true),
        heading("hypomania_mania"),
        grid(HYPOMANIA_MANIA_NAMES, true_false_options, true),
        heading("other_mania"),
        grid(MANIA_NAMES, true_false_options, false),
        heading("other_criteria"),
        grid(OTHER_CRITERIA_NAMES, true_false_options, false),
        heading("psychosis"),
        grid(PSYCHOSIS_AND_SIMILAR_NAMES, true_false_options, false),
        new QuHeading(TextConst::comments()),
        new QuTextEdit(fieldRef(COMMENTS, false)),
    })->setTitle(longname()));

    for (const QString& fieldname : INFORMATIVE) {
        connect(fieldRef(fieldname).data(), &FieldRef::valueChanged,
                this, &Icd10Manic::updateMandatory);
    }

    updateMandatory();

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QVariant Icd10Manic::meetsCriteriaManiaPsychoticSchizophrenic() const
{
    const QVariant mania = meetsCriteriaManiaIgnoringPsychosis();
    if (!mania.toBool()) {
        return mania;  // might be false or NULL
    }
    const QStringList icd10psychotic{HALLUCINATIONS_OTHER, DELUSIONS_OTHER};
    const QStringList schizophreniform{HALLUCINATIONS_SCHIZOPHRENIC,
                                       DELUSIONS_SCHIZOPHRENIC};
    if (anyTrue(values(icd10psychotic))) {
        return false;  // that counts as manic psychosis
    }
    if (anyNull(values(icd10psychotic))) {
        return QVariant();  // might be manic psychosis
    }
    if (anyTrue(values(schizophreniform))) {
        return true;
    }
    if (anyNull(values(schizophreniform))) {
        return QVariant();
    }
    return false;
}


QVariant Icd10Manic::meetsCriteriaManiaPsychoticIcd() const
{
    const QVariant mania = meetsCriteriaManiaIgnoringPsychosis();
    if (!mania.toBool()) {
        return mania;  // might be false or NULL
    }
    const QStringList icd10psychotic{HALLUCINATIONS_OTHER, DELUSIONS_OTHER};
    if (anyTrue(values(icd10psychotic))) {
        return true;
    }
    if (anyNull(values(icd10psychotic))) {
        return QVariant();
    }
    return false;
}


QVariant Icd10Manic::meetsCriteriaManiaNonpsychotic() const
{
    const QVariant mania_ign_psy = meetsCriteriaManiaIgnoringPsychosis();
    if (!mania_ign_psy.toBool()) {
        return mania_ign_psy;  // might be false or NULL
    }
    if (anyTrue(values(PSYCHOSIS_NAMES))) {
        return false;
    }
    if (anyNull(values(PSYCHOSIS_NAMES))) {
        return QVariant();
    }
    return true;
}


QVariant Icd10Manic::meetsCriteriaManiaIgnoringPsychosis() const
{
    // When can we say "definitely not"?
    if (allFalse(values({MOOD_ELEVATED, MOOD_IRRITABLE}))) {
        return false;
    }
    if (allFalse(values({SUSTAINED7DAYS, ADMISSION_REQUIRED}))) {
        return false;
    }
    const QVector<QVariant> symptoms = values(HYPOMANIA_MANIA_NAMES + MANIA_NAMES);
    const int t = countTrue(symptoms);  // t for true
    const int u = countNull(symptoms);  // u for unknown
    if (valueBool(MOOD_ELEVATED) && (t + u < 3)) {
        // With elevated mood, need at least 3 symptoms
        return false;
    }
    if (valueIsFalseNotNull(MOOD_ELEVATED) && (t + u < 4)) {
        // With only irritable mood, need at least 4 symptoms
        return false;
    }
    if (valueIsFalseNotNull(SEVERE_INTERFERENCE_FUNCTIONING)) {
        return false;
    }
    // OK. When can we say "yes"?
    if ((valueBool(MOOD_ELEVATED) || valueBool(MOOD_IRRITABLE)) &&
            (valueBool(SUSTAINED7DAYS) || valueBool(ADMISSION_REQUIRED)) &&
            ((valueBool(MOOD_ELEVATED) && t >= 3) ||
             (valueBool(MOOD_IRRITABLE) && t >= 4)) &&
            valueBool(SEVERE_INTERFERENCE_FUNCTIONING)) {
        return true;
    }
    return QVariant();
}


QVariant Icd10Manic::meetsCriteriaHypomania() const
{
    // When can we say "definitely not"?
    if (meetsCriteriaManiaIgnoringPsychosis().toBool()) {
        return false;  // silly to call it hypomania if it's mania
    }
    if (valueIsFalseNotNull(MOOD_ELEVATED) &&
            valueIsFalseNotNull(MOOD_IRRITABLE)) {
        return false;
    }
    if (valueIsFalseNotNull(SUSTAINED4DAYS)) {
        return false;
    }
    const QVector<QVariant> symptoms = values(HYPOMANIA_MANIA_NAMES);
    const int t = countTrue(symptoms);  // t for true
    const int u = countNull(symptoms);  // u for unknown
    if (t + u < 3) {
        // Need at least 3 symptoms
        return false;
    }
    if (valueIsFalseNotNull(SOME_INTERFERENCE_FUNCTIONING)) {
        return false;
    }
    // OK. When can we say "yes"?
    if ((valueBool(MOOD_ELEVATED) || valueBool(MOOD_IRRITABLE)) &&
            valueBool(SUSTAINED4DAYS) &&
            t >= 3 &&
            valueBool(SOME_INTERFERENCE_FUNCTIONING)) {
        return true;
    }
    return QVariant();
}


QVariant Icd10Manic::meetsCriteriaNone() const
{
    const QVariant h = meetsCriteriaHypomania();
    const QVariant m = meetsCriteriaManiaIgnoringPsychosis();
    if (h.toBool() || m.toBool()) {
        return false;
    }
    if (falseNotNull(h) || falseNotNull(m)) {
        return true;
    }
    return QVariant();
}


QString Icd10Manic::getDescription() const
{
    if (meetsCriteriaManiaPsychoticSchizophrenic().toBool()) {
        return xstring("category_manic_psychotic_schizophrenic");
    }
    if (meetsCriteriaManiaPsychoticIcd().toBool()) {
        return xstring("category_manic_psychotic");
    }
    if (meetsCriteriaManiaNonpsychotic().toBool()) {
        return xstring("category_manic_nonpsychotic");
    }
    if (meetsCriteriaHypomania().toBool()) {
        return xstring("category_hypomanic");
    }
    if (meetsCriteriaNone().toBool()) {
        return xstring("category_none");
    }
    return TextConst::unknown();
}


QStringList Icd10Manic::detailGroup(const QStringList& fieldnames) const
{
    QStringList lines;
    for (const QString& f : fieldnames) {
        lines.append(fieldSummary(f, f));
    }
    return lines;
}


// ============================================================================
// Signal handlers
// ============================================================================

void Icd10Manic::updateMandatory()
{
    // Information is mandatory until we have an answer.
    const bool known = meetsCriteriaNone().toBool() ||
            meetsCriteriaHypomania().toBool() ||
            meetsCriteriaManiaNonpsychotic().toBool() ||
            meetsCriteriaManiaPsychoticIcd().toBool() ||
            meetsCriteriaManiaPsychoticSchizophrenic().toBool();
    const bool need = !known;
    for (const QString& fieldname : INFORMATIVE) {
        fieldRef(fieldname)->setMandatory(need, this);
    }
}
