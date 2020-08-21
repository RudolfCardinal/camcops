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

#include "icd10mixed.h"
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
using mathfunc::allTrue;
using mathfunc::anyFalse;
using stringfunc::standardResult;
using uifunc::trueFalseUnknown;

const QString Icd10Mixed::ICD10MIXED_TABLENAME("icd10mixed");

const QString DATE_PERTAINS_TO("date_pertains_to");
const QString COMMENTS("comments");
const QString MIXTURE_OR_RAPID_ALTERNATION("mixture_or_rapid_alternation");
const QString DURATION_AT_LEAST_2_WEEKS("duration_at_least_2_weeks");
const QStringList CRITERIA{
    MIXTURE_OR_RAPID_ALTERNATION,
    DURATION_AT_LEAST_2_WEEKS,
};

void initializeIcd10Mixed(TaskFactory& factory)
{
    static TaskRegistrar<Icd10Mixed> registered(factory);
}


Icd10Mixed::Icd10Mixed(CamcopsApp& app, DatabaseManager& db,
                       const int load_pk) :
    Task(app, db, ICD10MIXED_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(DATE_PERTAINS_TO, QVariant::Date);
    addField(COMMENTS, QVariant::String);
    addField(MIXTURE_OR_RAPID_ALTERNATION, QVariant::Bool);
    addField(DURATION_AT_LEAST_2_WEEKS, QVariant::Bool);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(DATE_PERTAINS_TO, datetime::nowDate(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString Icd10Mixed::shortname() const
{
    return "ICD10-mixed";
}


QString Icd10Mixed::longname() const
{
    return tr("ICD-10 symptomatic criteria for a mixed affective episode "
              "(as in e.g. F06.3, F25, F38.00, F31.6)");
}


QString Icd10Mixed::description() const
{
    return TextConst::icd10();
}


QString Icd10Mixed::infoFilenameStem() const
{
    return "icd";
}


// ============================================================================
// Instance info
// ============================================================================

bool Icd10Mixed::isComplete() const
{
    return !meetsCriteria().isNull();
}


QStringList Icd10Mixed::summary() const
{
    return QStringList{
        standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                       shortDate(value(DATE_PERTAINS_TO))),
        standardResult(TextConst::meetsCriteria(),
                       trueFalseUnknown(meetsCriteria())),
    };
}


QStringList Icd10Mixed::detail() const
{
    QStringList lines = completenessInfo() + summary() + QStringList{
        fieldSummary(COMMENTS, TextConst::examinerComments()),
        fieldSummary(MIXTURE_OR_RAPID_ALTERNATION, xstring("a")),
        fieldSummary(DURATION_AT_LEAST_2_WEEKS, xstring("b")),
    };
    return lines;
}


OpenableWidget* Icd10Mixed::editor(const bool read_only)
{
    const NameValueOptions true_false_options = CommonOptions::falseTrueBoolean();
    QVector<QuestionWithOneField> qfields{
        {fieldRef(MIXTURE_OR_RAPID_ALTERNATION), xstring("a")},
        {fieldRef(DURATION_AT_LEAST_2_WEEKS), xstring("b")},
    };

    QuPagePtr page((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        (new QuText(appstring(appstrings::ICD10_SYMPTOMATIC_DISCLAIMER)))->setBold(),
        new QuText(appstring(appstrings::DATE_PERTAINS_TO)),
        (new QuDateTime(fieldRef(DATE_PERTAINS_TO)))
            ->setMode(QuDateTime::Mode::DefaultDate)
            ->setOfferNowButton(true),
        new QuMcqGrid(qfields, true_false_options),
        new QuHeading(TextConst::comments()),
        new QuTextEdit(fieldRef(COMMENTS, false)),
    })->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QVariant Icd10Mixed::meetsCriteria() const
{
    const QVector<QVariant> criteria = values(CRITERIA);
    if (allTrue(criteria)) {
        return true;
    }
    if (anyFalse(criteria)) {
        return false;
    }
    return QVariant();  // don't know yet
}
