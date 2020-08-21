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

#include "wsas.h"
#include "common/appstrings.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 5;
const int MAX_PER_Q = 8;
const QString QPREFIX("q");
const QString Wsas::WSAS_TABLENAME("wsas");
const QString RETIRED_ETC("retired_etc");
const QString Q1_TAG("q1");


void initializeWsas(TaskFactory& factory)
{
    static TaskRegistrar<Wsas> registered(factory);
}


Wsas::Wsas(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, WSAS_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addField(RETIRED_ETC, QVariant::Bool);
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Wsas::shortname() const
{
    return "WSAS";
}


QString Wsas::longname() const
{
    return tr("Work and Social Adjustment Scale");
}


QString Wsas::description() const
{
    return tr("5-item self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Wsas::isComplete() const
{
    return (valueBool(RETIRED_ETC) || !valueIsNull(strnum(QPREFIX, FIRST_Q))) &&
            noneNull(values(strseq(QPREFIX, FIRST_Q + 1, N_QUESTIONS)));
}


QStringList Wsas::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), maxScore())};
}


QStringList Wsas::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Wsas::editor(const bool read_only)
{
    const NameValueOptions options{
        {appstring(appstrings::WSAS_A_PREFIX + "0"), 0},
        {appstring(appstrings::WSAS_A_PREFIX + "1"), 1},
        {appstring(appstrings::WSAS_A_PREFIX + "2"), 2},
        {appstring(appstrings::WSAS_A_PREFIX + "3"), 3},
        {appstring(appstrings::WSAS_A_PREFIX + "4"), 4},
        {appstring(appstrings::WSAS_A_PREFIX + "5"), 5},
        {appstring(appstrings::WSAS_A_PREFIX + "6"), 6},
        {appstring(appstrings::WSAS_A_PREFIX + "7"), 7},
        {appstring(appstrings::WSAS_A_PREFIX + "8"), 8},
    };

    QVector<QuestionWithOneField> q1fields{QuestionWithOneField(
                    xstring(strnum("q", FIRST_Q), strnum("Q", FIRST_Q)),
                    fieldRef(strnum(QPREFIX, FIRST_Q)))};

    QVector<QuestionWithOneField> otherqfields;
    for (int i = FIRST_Q + 1; i <= N_QUESTIONS; ++i) {
        otherqfields.append(QuestionWithOneField(
                           xstring(strnum("q", i), strnum("Q", i)),
                           fieldRef(strnum(QPREFIX, i))));
    }

    FieldRefPtr fr_retired = fieldRef(RETIRED_ETC, false);

    QuPagePtr page((new QuPage{
        (new QuText(xstring("instruction")))->setBold(),
        new QuBoolean(xstring("q_retired_etc"), fr_retired),
        (new QuMcqGrid(q1fields, options))->addTag(Q1_TAG),
        new QuMcqGrid(otherqfields, options),
    })->setTitle(longname()));

    connect(fr_retired.data(), &FieldRef::valueChanged,
            this, &Wsas::workChanged);

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    workChanged();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Wsas::totalScore() const
{
    return (valueBool(RETIRED_ETC) ? 0 : valueInt(strnum(QPREFIX, FIRST_Q))) +
            sumInt(values(strseq(QPREFIX, FIRST_Q + 1, N_QUESTIONS)));
}


int Wsas::maxScore() const
{
    return MAX_PER_Q * (valueBool(RETIRED_ETC) ? (N_QUESTIONS - 1)
                                               : N_QUESTIONS);
}

// ============================================================================
// Task-specific calculations
// ============================================================================

void Wsas::workChanged()
{
    if (!m_questionnaire) {
        return;
    }
    m_questionnaire->setVisibleByTag(Q1_TAG, !valueBool(RETIRED_ETC));
}
