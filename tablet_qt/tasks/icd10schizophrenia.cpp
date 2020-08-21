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

#include "icd10schizophrenia.h"
#include "common/appstrings.h"
#include "common/textconst.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using datetime::shortDate;
using mathfunc::countNull;
using mathfunc::countTrue;
using mathfunc::falseNotNull;
using stringfunc::standardResult;
using uifunc::yesNoUnknown;

const QString Icd10Schizophrenia::ICD10SZ_TABLENAME("icd10schizophrenia");

const QString PASSIVITY_BODILY("passivity_bodily");
const QString PASSIVITY_MENTAL("passivity_mental");
const QString HV_COMMENTARY("hv_commentary");
const QString HV_DISCUSSING("hv_discussing");
const QString HV_FROM_BODY("hv_from_body");
const QString DELUSIONS("delusions");
const QString DELUSIONAL_PERCEPTION("delusional_perception");
const QString THOUGHT_ECHO("thought_echo");
const QString THOUGHT_WITHDRAWAL("thought_withdrawal");
const QString THOUGHT_INSERTION("thought_insertion");
const QString THOUGHT_BROADCASTING("thought_broadcasting");
const QStringList A_NAMES{
     PASSIVITY_BODILY,
     PASSIVITY_MENTAL,
     HV_COMMENTARY,
     HV_DISCUSSING,
     HV_FROM_BODY,
     DELUSIONS,
     DELUSIONAL_PERCEPTION,
     THOUGHT_ECHO,
     THOUGHT_WITHDRAWAL,
     THOUGHT_INSERTION,
     THOUGHT_BROADCASTING,
};
const QString HALLUCINATIONS_OTHER("hallucinations_other");
const QString THOUGHT_DISORDER("thought_disorder");
const QString CATATONIA("catatonia");
const QStringList B_NAMES{
     HALLUCINATIONS_OTHER,
     THOUGHT_DISORDER,
     CATATONIA,
};
const QString NEGATIVE("negative");
const QStringList C_NAMES{
     NEGATIVE,
};
const QString PRESENT_ONE_MONTH("present_one_month");
const QStringList D_NAMES{
     PRESENT_ONE_MONTH,
};
const QString ALSO_MANIC("also_manic");
const QString ALSO_DEPRESSIVE("also_depressive");
const QString IF_MOOD_PSYCHOSIS_FIRST("if_mood_psychosis_first");
const QStringList E_NAMES{
     ALSO_MANIC,
     ALSO_DEPRESSIVE,
     IF_MOOD_PSYCHOSIS_FIRST,
};
const QString NOT_ORGANIC_OR_SUBSTANCE("not_organic_or_substance");
const QStringList F_NAMES{
     NOT_ORGANIC_OR_SUBSTANCE,
};
const QString BEHAVIOUR_CHANGE("behaviour_change");
const QString PERFORMANCE_DECLINE("performance_decline");
const QStringList G_NAMES{
     BEHAVIOUR_CHANGE,
     PERFORMANCE_DECLINE,
};
const QString SUBTYPE_PARANOID("subtype_paranoid");
const QString SUBTYPE_HEBEPHRENIC("subtype_hebephrenic");
const QString SUBTYPE_CATATONIC("subtype_catatonic");
const QString SUBTYPE_UNDIFFERENTIATED("subtype_undifferentiated");
const QString SUBTYPE_POSTSCHIZOPHRENIC_DEPRESSION("subtype_postschizophrenic_depression");
const QString SUBTYPE_RESIDUAL("subtype_residual");
const QString SUBTYPE_SIMPLE("subtype_simple");
const QString SUBTYPE_CENESTHOPATHIC("subtype_cenesthopathic");
const QStringList H_NAMES{
     SUBTYPE_PARANOID,
     SUBTYPE_HEBEPHRENIC,
     SUBTYPE_CATATONIC,
     SUBTYPE_UNDIFFERENTIATED,
     SUBTYPE_POSTSCHIZOPHRENIC_DEPRESSION,
     SUBTYPE_RESIDUAL,
     SUBTYPE_SIMPLE,
     SUBTYPE_CENESTHOPATHIC,
};
const QString DATE_PERTAINS_TO("date_pertains_to");
const QString COMMENTS("comments");

const QStringList INFORMATIVE = (A_NAMES + B_NAMES + C_NAMES + D_NAMES +
                                 E_NAMES + F_NAMES + G_NAMES);  // but not H


void initializeIcd10Schizophrenia(TaskFactory& factory)
{
    static TaskRegistrar<Icd10Schizophrenia> registered(factory);
}


Icd10Schizophrenia::Icd10Schizophrenia(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, ICD10SZ_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(A_NAMES, QVariant::Bool);
    addFields(B_NAMES, QVariant::Bool);
    addFields(C_NAMES, QVariant::Bool);
    addFields(D_NAMES, QVariant::Bool);
    addFields(E_NAMES, QVariant::Bool);
    addFields(F_NAMES, QVariant::Bool);
    addFields(G_NAMES, QVariant::Bool);
    addFields(H_NAMES, QVariant::Bool);

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

QString Icd10Schizophrenia::shortname() const
{
    return "ICD10-schizophrenia";
}


QString Icd10Schizophrenia::longname() const
{
    return tr("ICD-10 criteria for schizophrenia (F20)");
}


QString Icd10Schizophrenia::description() const
{
    return TextConst::icd10();
}


QString Icd10Schizophrenia::infoFilenameStem() const
{
    return "icd";
}


// ============================================================================
// Instance info
// ============================================================================

bool Icd10Schizophrenia::isComplete() const
{
    return !valueIsNull(DATE_PERTAINS_TO) && !meetsGeneralCriteria().isNull();
}


QStringList Icd10Schizophrenia::summary() const
{
    return QStringList{
        standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                       shortDate(value(DATE_PERTAINS_TO))),
        standardResult(xstring("meets_general_criteria"),
                       yesNoUnknown(meetsGeneralCriteria())),
    };
}


QStringList Icd10Schizophrenia::detail() const
{
    QStringList lines = completenessInfo();
    lines.append(standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                                shortDate(value(DATE_PERTAINS_TO))));
    lines.append(fieldSummary(COMMENTS,
                              TextConst::examinerComments()));
    lines.append("");
    for (const QString& fieldname : (A_NAMES + B_NAMES + C_NAMES + D_NAMES +
                                     E_NAMES + F_NAMES + G_NAMES + H_NAMES)) {
        lines.append(fieldSummary(fieldname, xstring(fieldname)));
    }
    lines.append("");
    lines += standardResult(xstring("meets_general_criteria"),
                            yesNoUnknown(meetsGeneralCriteria()));
    return lines;
}


OpenableWidget* Icd10Schizophrenia::editor(const bool read_only)
{
    const NameValueOptions true_false_options = CommonOptions::falseTrueBoolean();
    const NameValueOptions present_absent_options = CommonOptions::absentPresentBoolean();

    auto heading = [this](const QString& xstringname) -> QuElement* {
        return new QuHeading(xstring(xstringname));
    };
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto grid = [this, &true_false_options, &present_absent_options]
            (const QStringList& fields_xstrings, bool present_absent)
            -> QuElement* {
        // Assumes the xstring name matches the fieldname (as it does)
        const NameValueOptions& options = present_absent
                ? present_absent_options
                : true_false_options;
        QVector<QuestionWithOneField> qfields;
        for (const QString& fieldname : fields_xstrings) {
            qfields.append(QuestionWithOneField(xstring(fieldname),
                                                fieldRef(fieldname, false)));
        }
        const int n = options.size();
        const QVector<int> v(n, 1);
        return (new QuMcqGrid(qfields, options))
                ->setExpand(true)
                ->setWidth(n, v);
    };

    QuPagePtr page((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        new QuText(appstring(appstrings::DATE_PERTAINS_TO)),
        (new QuDateTime(fieldRef(DATE_PERTAINS_TO)))
            ->setMode(QuDateTime::Mode::DefaultDate)
            ->setOfferNowButton(true),
        text("comments"),
        heading("core"),
        grid(A_NAMES, true),
        heading("other_positive"),
        grid(B_NAMES, true),
        heading("negative_title"),
        grid(C_NAMES, true),
        heading("other_criteria"),
        grid(D_NAMES, false),
        text("duration_comment"),
        grid(E_NAMES, false),
        text("affective_comment"),
        grid(F_NAMES, false),
        heading("simple_title"),
        grid(G_NAMES, true),
        heading("subtypes"),
        grid(H_NAMES, true),
        new QuHeading(TextConst::comments()),
        new QuTextEdit(fieldRef(COMMENTS, false)),
    })->setTitle(longname()));

    for (const QString& fieldname : INFORMATIVE) {
        connect(fieldRef(fieldname).data(), &FieldRef::valueChanged,
                this, &Icd10Schizophrenia::updateMandatory);
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

QVariant Icd10Schizophrenia::meetsGeneralCriteria() const
{
    const QVector<QVariant> va = values(A_NAMES);
    const QVector<QVariant> vb = values(B_NAMES);
    const QVector<QVariant> vc = values(C_NAMES);
    const int t1 = countTrue(va);  // t for true
    const int u1 = countNull(va);  // u for unknown
    const int t2 = countTrue(vb) + countTrue(vc);
    const int u2 = countNull(vb) + countNull(vc);

    if (t1 + u1 < 1 && t2 + u2 < 2) {
        // Not schizophrenia: insufficient symptoms
        return false;
    }
    if (falseNotNull(value(PRESENT_ONE_MONTH))) {
        // Not schizophrenia: not present for long enough
        return false;
    }
    if ((valueBool(ALSO_MANIC) || valueBool(ALSO_DEPRESSIVE)) &&
            falseNotNull(value(IF_MOOD_PSYCHOSIS_FIRST))) {
        // Not schizophrenia: affective disorder preceded psychosis
        return false;
    }
    if (falseNotNull(value(NOT_ORGANIC_OR_SUBSTANCE))) {
        // Not schizophrenia: organic or substance-induced, instead
        return false;
    }
    const bool symptoms = t1 >= 1 || t2 >= 2;
    const bool duration = valueBool(PRESENT_ONE_MONTH);
    const bool no_mood_exclusion = (falseNotNull(value(ALSO_MANIC)) &&
                              falseNotNull(value(ALSO_DEPRESSIVE))) ||
            valueBool(IF_MOOD_PSYCHOSIS_FIRST);
    // ... (not manic AND not depressed) OR (if mood, psychosis came first)
    const bool no_organic_substance_exclusion = valueBool(NOT_ORGANIC_OR_SUBSTANCE);
    if (symptoms && duration && no_mood_exclusion &&
            no_organic_substance_exclusion) {
        // Positive diagnosis of schizophrenia
        return true;
    }
    // Uncertain; return NULL
    return QVariant();
}


// ============================================================================
// Signal handlers
// ============================================================================

void Icd10Schizophrenia::updateMandatory()
{
    const bool known = !meetsGeneralCriteria().isNull();
    const bool need = !known;
    for (const QString& fieldname : INFORMATIVE) {
        fieldRef(fieldname)->setMandatory(need, this);
    }
}
