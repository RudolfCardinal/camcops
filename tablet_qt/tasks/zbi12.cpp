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

#include "zbi12.h"
#include "common/appstrings.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 12;
const QString QPREFIX("q");

const QString Zbi12::ZBI12_TABLENAME("zbi12");


void initializeZbi12(TaskFactory& factory)
{
    static TaskRegistrar<Zbi12> registered(factory);
}


Zbi12::Zbi12(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, ZBI12_TABLENAME, false, false, true)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Zbi12::shortname() const
{
    return "ZBI-12";
}


QString Zbi12::longname() const
{
    return tr("Zarit Burden Interview, 12-item version");
}


QString Zbi12::description() const
{
    return tr("12-item caregiver-report scale.");
}


QString Zbi12::infoFilenameStem() const
{
    return "zbi";
}


// ============================================================================
// Instance info
// ============================================================================

bool Zbi12::isComplete() const
{
    return isRespondentComplete() &&
            noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Zbi12::summary() const
{
    return QStringList{valueString(RESPONDENT_NAME)};
}


QStringList Zbi12::detail() const
{
    return completenessInfo() + recordSummaryLines();
}


OpenableWidget* Zbi12::editor(const bool read_only)
{
    const NameValueOptions options{
        {appstring(appstrings::ZBI_A_PREFIX + "0"), 0},
        {appstring(appstrings::ZBI_A_PREFIX + "1"), 1},
        {appstring(appstrings::ZBI_A_PREFIX + "2"), 2},
        {appstring(appstrings::ZBI_A_PREFIX + "3"), 3},
        {appstring(appstrings::ZBI_A_PREFIX + "4"), 4},
    };
    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfields.append(QuestionWithOneField(
                           xstring(strnum("q", i), strnum("Q", i)),
                           fieldRef(strnum(QPREFIX, i))));
    }

    QuPagePtr page((new QuPage{
        getRespondentQuestionnaireBlockRawPointer(true),
        (new QuText(xstring("instruction")))->setBold(),
        new QuMcqGrid(qfields, options),
    })->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================
