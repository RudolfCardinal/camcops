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

#include "icd10depressive.h"
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
using mathfunc::anyNull;
using mathfunc::anyTrue;
using mathfunc::countNull;
using mathfunc::countTrue;
using stringfunc::bold;
using stringfunc::standardResult;

const QString Icd10Depressive::ICD10DEP_TABLENAME("icd10depressive");

const QString MOOD("mood");
const QString ANHEDONIA("anhedonia");
const QString ENERGY("energy");
const QStringList CORE_NAMES{
    MOOD,
    ANHEDONIA,
    ENERGY,
};
const QString SLEEP("sleep");
const QString WORTH("worth");
const QString APPETITE("appetite");
const QString GUILT("guilt");
const QString CONCENTRATION("concentration");
const QString ACTIVITY("activity");
const QString DEATH("death");
const QStringList ADDITIONAL_NAMES{
    SLEEP,
    WORTH,
    APPETITE,
    GUILT,
    CONCENTRATION,
    ACTIVITY,
    DEATH,
};
const QString SOMATIC_ANHEDONIA("somatic_anhedonia");
const QString SOMATIC_EMOTIONAL_UNREACTIVITY("somatic_emotional_unreactivity");
const QString SOMATIC_EARLY_MORNING_WAKING("somatic_early_morning_waking");
const QString SOMATIC_MOOD_WORSE_MORNING("somatic_mood_worse_morning");
const QString SOMATIC_PSYCHOMOTOR("somatic_psychomotor");
const QString SOMATIC_APPETITE("somatic_appetite");
const QString SOMATIC_WEIGHT("somatic_weight");
const QString SOMATIC_LIBIDO("somatic_libido");
const QStringList SOMATIC_NAMES{
    SOMATIC_ANHEDONIA,
    SOMATIC_EMOTIONAL_UNREACTIVITY,
    SOMATIC_EARLY_MORNING_WAKING,
    SOMATIC_MOOD_WORSE_MORNING,
    SOMATIC_PSYCHOMOTOR,
    SOMATIC_APPETITE,
    SOMATIC_WEIGHT,
    SOMATIC_LIBIDO,
};
const QString HALLUCINATIONS_SCHIZOPHRENIC("hallucinations_schizophrenic");
const QString HALLUCINATIONS_OTHER("hallucinations_other");
const QString DELUSIONS_SCHIZOPHRENIC("delusions_schizophrenic");
const QString DELUSIONS_OTHER("delusions_other");
const QString STUPOR("stupor");
const QStringList PSYCHOSIS_AND_SIMILAR_NAMES{
    HALLUCINATIONS_SCHIZOPHRENIC,
    HALLUCINATIONS_OTHER,
    DELUSIONS_SCHIZOPHRENIC,
    DELUSIONS_OTHER,
    STUPOR,
};
const QString DATE_PERTAINS_TO("date_pertains_to");
const QString COMMENTS("comments");
const QString DURATION_AT_LEAST_2_WEEKS("duration_at_least_2_weeks");
const QString SEVERE_CLINICALLY("severe_clinically");
const QStringList INFORMATIVE = CORE_NAMES + ADDITIONAL_NAMES +
        PSYCHOSIS_AND_SIMILAR_NAMES +
        QStringList{DURATION_AT_LEAST_2_WEEKS, SEVERE_CLINICALLY};


void initializeIcd10Depressive(TaskFactory& factory)
{
    static TaskRegistrar<Icd10Depressive> registered(factory);
}


Icd10Depressive::Icd10Depressive(CamcopsApp& app, DatabaseManager& db,
                                 const int load_pk) :
    Task(app, db, ICD10DEP_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(CORE_NAMES, QVariant::Bool);
    addFields(ADDITIONAL_NAMES, QVariant::Bool);
    addFields(SOMATIC_NAMES, QVariant::Bool);
    addFields(PSYCHOSIS_AND_SIMILAR_NAMES, QVariant::Bool);

    addField(DATE_PERTAINS_TO, QVariant::Date);
    addField(COMMENTS, QVariant::String);
    addField(DURATION_AT_LEAST_2_WEEKS, QVariant::Bool);
    addField(SEVERE_CLINICALLY, QVariant::Bool);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(DATE_PERTAINS_TO, datetime::nowDate(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString Icd10Depressive::shortname() const
{
    return "ICD10-depression";
}


QString Icd10Depressive::longname() const
{
    return tr("ICD-10 symptomatic criteria for a depressive episode "
              "(as in e.g. F06.3, F25, F31, F32, F33)");
}


QString Icd10Depressive::description() const
{
    return TextConst::icd10();
}


QString Icd10Depressive::infoFilenameStem() const
{
    return "icd";
}


// ============================================================================
// Instance info
// ============================================================================

bool Icd10Depressive::isComplete() const
{
    return !valueIsNull(DATE_PERTAINS_TO) && mainComplete();
}


QStringList Icd10Depressive::summary() const
{
    return QStringList{
        standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                       shortDate(value(DATE_PERTAINS_TO))),
        standardResult(TextConst::category(), getFullDescription()),
    };
}


QStringList Icd10Depressive::detail() const
{
    QStringList lines = completenessInfo();
    lines.append(standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                                shortDate(value(DATE_PERTAINS_TO))));
    lines.append(fieldSummary(COMMENTS,
                              TextConst::examinerComments()));
    lines.append(fieldSummaryTrueFalseUnknown(DURATION_AT_LEAST_2_WEEKS,
                                              DURATION_AT_LEAST_2_WEEKS));
    lines.append("");
    lines += detailGroup(CORE_NAMES);
    lines += detailGroup(ADDITIONAL_NAMES);
    lines.append(fieldSummaryTrueFalseUnknown(SEVERE_CLINICALLY,
                                              SEVERE_CLINICALLY));
    lines += detailGroup(SOMATIC_NAMES);
    lines += detailGroup(PSYCHOSIS_AND_SIMILAR_NAMES);
    lines.append("");
    lines.append(standardResult(TextConst::category(), getFullDescription()));
#ifdef SHOW_CLASSIFICATION_WORKING
    auto pv = [](const QVariant& v) -> QString {
        return bold(convert::prettyValue(v));
    };
    lines.append("");
    lines.append("nCore(): " + pv(nCore()));
    lines.append("nAdditional(): " + pv(nAdditional()));
    lines.append("nTotal(): " + pv(nTotal()));
    lines.append("nSomatic(): " + pv(nSomatic()));
    lines.append("mainComplete(): " + pv(mainComplete()));
    lines.append("meetsCriteriaSeverePsychoticSchizophrenic(): " +
                 pv(meetsCriteriaSeverePsychoticSchizophrenic()));
    lines.append("meetsCriteriaSeverePsychoticIcd(): " +
                 pv(meetsCriteriaSeverePsychoticIcd()));
    lines.append("meetsCriteriaSevereNonpsychotic(): " +
                 pv(meetsCriteriaSevereNonpsychotic()));
    lines.append("meetsCriteriaSevereIgnoringPsychosis(): " +
                 pv(meetsCriteriaSevereIgnoringPsychosis()));
    lines.append("meetsCriteriaModerate(): " + pv(meetsCriteriaModerate()));
    lines.append("meetsCriteriaMild(): " + pv(meetsCriteriaMild()));
    lines.append("meetsCriteriaNone(): " + pv(meetsCriteriaNone()));
    lines.append("meetsCriteriaSomatic(): " + pv(meetsCriteriaSomatic()));
#endif
    return lines;
}


OpenableWidget* Icd10Depressive::editor(const bool read_only)
{
    const NameValueOptions true_false_options = CommonOptions::falseTrueBoolean();
    const NameValueOptions present_absent_options = CommonOptions::absentPresentBoolean();

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
        heading("duration_text"),
        grid({DURATION_AT_LEAST_2_WEEKS}, true_false_options, true),
        heading("core"),
        grid(CORE_NAMES, present_absent_options, true),
        heading("additional"),
        grid(ADDITIONAL_NAMES, present_absent_options, true),
        grid({SEVERE_CLINICALLY}, true_false_options, true),
        heading("somatic"),
        grid(SOMATIC_NAMES, present_absent_options, false),
        heading("psychotic"),
        grid(PSYCHOSIS_AND_SIMILAR_NAMES, present_absent_options, false),
        new QuHeading(TextConst::comments()),
        new QuTextEdit(fieldRef(COMMENTS, false)),
    })->setTitle(longname()));

    for (const QString& fieldname : INFORMATIVE) {
        connect(fieldRef(fieldname).data(), &FieldRef::valueChanged,
                this, &Icd10Depressive::updateMandatory);
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

int Icd10Depressive::nCore() const
{
    return countTrue(values(CORE_NAMES));
}


int Icd10Depressive::nAdditional() const
{
    return countTrue(values(ADDITIONAL_NAMES));
}


int Icd10Depressive::nTotal() const
{
    return nCore() + nAdditional();
}


int Icd10Depressive::nSomatic() const
{
    return countTrue(values(SOMATIC_NAMES));
}


bool Icd10Depressive::mainComplete() const
{
    return (!valueIsNull(DURATION_AT_LEAST_2_WEEKS) &&
            noValuesNull(CORE_NAMES) &&
            noValuesNull(ADDITIONAL_NAMES)) || valueBool(SEVERE_CLINICALLY);
}


QVariant Icd10Depressive::meetsCriteriaSeverePsychoticSchizophrenic() const
{
    const QVariant severe = meetsCriteriaSevereIgnoringPsychosis();
    if (!severe.toBool()) {
        return severe;  // might be false or NULL
    }
    const QStringList icd10psychotic{STUPOR, HALLUCINATIONS_OTHER,
                                     DELUSIONS_OTHER};
    const QStringList schizophreniform{HALLUCINATIONS_SCHIZOPHRENIC,
                                       DELUSIONS_SCHIZOPHRENIC};
    if (anyValuesTrue(icd10psychotic)) {
        return false;  // that counts as F32.3
    }
    if (anyValuesNull(icd10psychotic)) {
        return QVariant();  // might be F32.3
    }
    if (anyValuesTrue(schizophreniform)) {
        return true;
    }
    if (anyValuesNull(schizophreniform)) {
        return QVariant();
    }
    return false;
}


QVariant Icd10Depressive::meetsCriteriaSeverePsychoticIcd() const
{
    const QVariant severe = meetsCriteriaSevereIgnoringPsychosis();
    if (!severe.toBool()) {
        return severe;  // might be false or NULL
    }
    // For psychotic depression (F32.3), the ICD-10 Green Book requires the
    // PRESENCE of non-schizophreniform psychotic symptoms, but not the ABSENCE
    // of schizophreniform psychotic symptoms.
    const QStringList icd10psychotic{STUPOR, HALLUCINATIONS_OTHER,
                                     DELUSIONS_OTHER};
    if (anyValuesTrue(icd10psychotic)) {
        return true;
    }
    if (anyValuesNull(icd10psychotic)) {
        return QVariant();
    }
    return false;
}


QVariant Icd10Depressive::meetsCriteriaSevereNonpsychotic() const
{
    const QVariant severe_ign_psy = meetsCriteriaSevereIgnoringPsychosis();
    if (!severe_ign_psy.toBool()) {
        return severe_ign_psy;  // might be false or NULL
    }
    if (anyValuesNull(PSYCHOSIS_AND_SIMILAR_NAMES)) {
        return QVariant();
    }
    return countTrue(values(PSYCHOSIS_AND_SIMILAR_NAMES)) == 0;
}


QVariant Icd10Depressive::meetsCriteriaSevereIgnoringPsychosis() const
{
    if (valueBool(SEVERE_CLINICALLY)) {
        return true;
    }
    if (valueIsFalseNotNull(DURATION_AT_LEAST_2_WEEKS)) {
        return false;  // too short
    }
    if (nCore() >= 3 && nTotal() >= 8) {
        return true;  // ICD-10 definition of severe deperssion
    }
    if (!mainComplete()) {
        return QVariant();  // addition of more information might increase severity
    }
    return false;
}


QVariant Icd10Depressive::meetsCriteriaModerate() const
{
    if (meetsCriteriaSevereIgnoringPsychosis().toBool()) {
        return false;  // too severe
    }
    if (valueIsFalseNotNull(DURATION_AT_LEAST_2_WEEKS)) {
        return false;  // too short
    }
    if (!mainComplete()) {
        return QVariant();  // addition of more information might increase severity
    }
    if (nCore() >= 2 && nTotal() >= 6) {
        return true;  // ICD-10 definition of moderate depression
    }
    return false;
}


QVariant Icd10Depressive::meetsCriteriaMild() const
{
    if (meetsCriteriaSevereIgnoringPsychosis().toBool() ||
            meetsCriteriaModerate().toBool()) {
        return false;  // too severe
    }
    if (valueIsFalseNotNull(DURATION_AT_LEAST_2_WEEKS)) {
        return false;  // too short
    }
    if (!mainComplete()) {
        return QVariant();  // addition of more information might increase severity
    }
    if (nCore() >= 2 && nTotal() >= 4) {
        return true;  // ICD-10 definition of mild depression
    }
    return false;
}


QVariant Icd10Depressive::meetsCriteriaNone() const
{
    if (meetsCriteriaSevereIgnoringPsychosis().toBool() ||
            meetsCriteriaModerate().toBool() ||
            meetsCriteriaMild().toBool()) {
        return false;  // depression is present
    }
    if (valueIsFalseNotNull(DURATION_AT_LEAST_2_WEEKS)) {
        return true;  // too short to have depression
    }
    if (!mainComplete()) {
        return QVariant();  // addition of more information might increase severity
    }
    return true;
}


QVariant Icd10Depressive::meetsCriteriaSomatic() const
{
    const int t = nSomatic();  // t for true
    const int u = countNull(values(SOMATIC_NAMES));  // u for unknown
    if (t >= 4) {
        return true;
    }
    if (t + u < 4) {
        return false;
    }
    return QVariant();
}


QString Icd10Depressive::getSomaticDescription() const
{
    const QVariant s = meetsCriteriaSomatic();
    if (s.isNull()) {
        return xstring("category_somatic_unknown");
    }
    if (s.toBool()) {
        return xstring("category_with_somatic");
    }
    return xstring("category_without_somatic");
}


QString Icd10Depressive::getMainDescription() const
{
    if (meetsCriteriaSeverePsychoticSchizophrenic().toBool()) {
        return xstring("category_severe_psychotic_schizophrenic");
    }
    if (meetsCriteriaSeverePsychoticIcd().toBool()) {
        return xstring("category_severe_psychotic");
    }
    if (meetsCriteriaSevereNonpsychotic().toBool()) {
        return xstring("category_severe_nonpsychotic");
    }
    if (meetsCriteriaModerate().toBool()) {
        return xstring("category_moderate");
    }
    if (meetsCriteriaMild().toBool()) {
        return xstring("category_mild");
    }
    if (meetsCriteriaNone().toBool()) {
        return xstring("category_none");
    }
    return TextConst::unknown();
}


QString Icd10Depressive::getFullDescription() const
{
    // I note in passing:
    //   QVariant v;
    //   bool b = bool(v);  // won't compile: invalid cast from type 'QVariant' to type 'bool'
    //   bool b = v;  // won't compile: cannot convert 'QVariant' to 'bool' in initialization
    //   bool b = true && v;  // won't compile: no match for 'operator&&' (operand types are 'bool' and 'QVariant')
    // ... which is good, as it means we can't forget the .toBool() part!
    const bool skip_somatic = mainComplete() && meetsCriteriaNone().toBool();
    return getMainDescription() + (skip_somatic
                                   ? ""
                                   : " " + getSomaticDescription());
}


QStringList Icd10Depressive::detailGroup(const QStringList& fieldnames) const
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

void Icd10Depressive::updateMandatory()
{
    const bool known = meetsCriteriaNone().toBool() ||
            meetsCriteriaMild().toBool() ||
            meetsCriteriaModerate().toBool() ||
            meetsCriteriaSevereNonpsychotic().toBool() ||
            meetsCriteriaSeverePsychoticIcd().toBool() ||
            meetsCriteriaSeverePsychoticSchizophrenic().toBool();
    const bool need = !known;
    for (const QString& fieldname : INFORMATIVE) {
        fieldRef(fieldname)->setMandatory(need, this);
    }
}
