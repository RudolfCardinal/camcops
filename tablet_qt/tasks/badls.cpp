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

#include "badls.h"
#include <QMap>
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
// using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 20;
const int MAX_SCORE = N_QUESTIONS * 3;
const QString QPREFIX("q");

const QString Badls::BADLS_TABLENAME("badls");
const QMap<QString, int> BADLS_SCORING{
    {"a", 0},
    {"b", 1},
    {"c", 2},
    {"d", 3},
    {"e", 0},
};


void initializeBadls(TaskFactory& factory)
{
    static TaskRegistrar<Badls> registered(factory);
}


Badls::Badls(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, BADLS_TABLENAME, false, false, true)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Badls::shortname() const
{
    return "BADLS";
}


QString Badls::longname() const
{
    return tr("Bristol Activities of Daily Living Scale");
}


QString Badls::description() const
{
    return tr("20-item carer-rated scale for use in dementia.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Badls::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Badls::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Badls::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "", ": ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Badls::editor(const bool read_only)
{
    QVector<QuElementPtr> elements;
    const bool second_person = true;

    elements.append(getRespondentQuestionnaireBlockElementPtr(second_person));
    elements.append(QuElementPtr(new QuText(xstring("instruction_1"))));
    elements.append(QuElementPtr(new QuText(xstring("instruction_2"))));
    elements.append(QuElementPtr(new QuText(xstring("instruction_3"))));
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        QString qnumstr = QString::number(i);
        NameValueOptions options{
            {xstring("q" + qnumstr + "_a"), "a"},
            {xstring("q" + qnumstr + "_b"), "b"},
            {xstring("q" + qnumstr + "_c"), "c"},
            {xstring("q" + qnumstr + "_d"), "d"},
            {xstring("q" + qnumstr + "_e"), "e"},
        };
        elements.append(QuElementPtr(
            (new QuText(xstring("q" + qnumstr)))->setBold()));
        elements.append(QuElementPtr(
            new QuMcq(fieldRef(QPREFIX + qnumstr), options)));
    }

    QuPagePtr page((new QuPage(elements))->setTitle(shortname()));

    auto* questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Badls::score(const int qnum) const
{
    const QVariant v = value(QPREFIX + QString::number(qnum));
    return BADLS_SCORING[v.toString()];
    // If the key is not present, we will get a default-initialized int [1],
    // which will be 0 [2, 3].
    // [1] http://doc.qt.io/qt-5/qmap.html#operator-5b-5d
    // [2] http://doc.qt.io/qt-5/containers.html#default-constructed-value
    // [3] http://stackoverflow.com/questions/2667355/mapint-int-default-values
}


int Badls::totalScore() const
{
    int total = 0;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        total += score(i);
    }
    return total;
}
