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

#include "icd10schizotypal.h"
#include "common/appstrings.h"
#include "common/textconst.h"
#include "lib/convert.h"
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
using mathfunc::anyNull;
using mathfunc::countTrue;
using stringfunc::standardResult;
using stringfunc::strseq;
using uifunc::yesNoUnknown;

const QString Icd10Schizotypal::ICD10SZTYPAL_TABLENAME("icd10schizotypal");

const int N_A = 9;
const QString A_PREFIX("a");
const QString B("b");
const QString DATE_PERTAINS_TO("date_pertains_to");
const QString COMMENTS("comments");


void initializeIcd10Schizotypal(TaskFactory& factory)
{
    static TaskRegistrar<Icd10Schizotypal> registered(factory);
}


Icd10Schizotypal::Icd10Schizotypal(CamcopsApp& app, DatabaseManager& db,
                                   const int load_pk) :
    Task(app, db, ICD10SZTYPAL_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(A_PREFIX, 1, N_A), QVariant::Bool);
    addField(B, QVariant::Bool);
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

QString Icd10Schizotypal::shortname() const
{
    return "ICD10-schizotypal";
}


QString Icd10Schizotypal::longname() const
{
    return tr("ICD-10 criteria for schizotypal disorder (F21)");
}


QString Icd10Schizotypal::description() const
{
    return TextConst::icd10();
}


QString Icd10Schizotypal::infoFilenameStem() const
{
    return "icd";
}


// ============================================================================
// Instance info
// ============================================================================

bool Icd10Schizotypal::isComplete() const
{
    return !valueIsNull(DATE_PERTAINS_TO) &&
            !anyNull(values(strseq(A_PREFIX, 1, N_A))) &&
            !valueIsNull(B);
}


QStringList Icd10Schizotypal::summary() const
{
    return QStringList{
        standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                       shortDate(value(DATE_PERTAINS_TO))),
        standardResult(TextConst::meetsCriteria(),
                       yesNoUnknown(meetsCriteria())),
    };
}


QStringList Icd10Schizotypal::detail() const
{
    QStringList lines = completenessInfo() + summary();
    lines.append(fieldSummary(COMMENTS,
                              TextConst::examinerComments()));
    return lines;
}


OpenableWidget* Icd10Schizotypal::editor(const bool read_only)
{
    const NameValueOptions options = CommonOptions::falseTrueBoolean();

    auto grid = [this, &options]
            (const QStringList& fields_xstrings) -> QuElement* {
        // Assumes the xstring name matches the fieldname (as it does)
        QVector<QuestionWithOneField> qfields;
        for (const QString& fieldname : fields_xstrings) {
            qfields.append(QuestionWithOneField(xstring(fieldname),
                                                fieldRef(fieldname, true)));
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
        new QuHeading(xstring("a")),
        grid(strseq(A_PREFIX, 1, N_A)),
        new QuHeading(TextConst::txtAnd()),
        grid({B}),
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

QVariant Icd10Schizotypal::meetsCriteria() const
{
    if (!isCompleteCached()) {
        return QVariant();
    }
    return countTrue(values(strseq(A_PREFIX, 1, N_A))) >= 4 && valueBool(B);
}
