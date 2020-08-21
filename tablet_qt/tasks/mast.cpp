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

#include "mast.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 24;
const int MAX_SCORE = 53;
const QString QPREFIX("q");

const QString Mast::MAST_TABLENAME("mast");
const QVector<int> REVERSED_QUESTIONS{1, 4, 6, 7};
const QVector<int> QUESTIONS_SCORING_ONE{3, 5, 9, 16};
const QVector<int> QUESTIONS_SCORING_FIVE{8, 19, 20};
const int THRESHOLD_SCORE = 13;


void initializeMast(TaskFactory& factory)
{
    static TaskRegistrar<Mast> registered(factory);
}


Mast::Mast(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, MAST_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Mast::shortname() const
{
    return "MAST";
}


QString Mast::longname() const
{
    return tr("Michigan Alcohol Screening Test");
}


QString Mast::description() const
{
    return tr("24-item Y/N self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Mast::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Mast::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Mast::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    lines.append(standardResult(xstring("exceeds_threshold"),
                                uifunc::yesNo(totalScore() >= THRESHOLD_SCORE)));
    return lines;
}


OpenableWidget* Mast::editor(const bool read_only)
{
    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfields.append(QuestionWithOneField(xstring(strnum("q", i)),
                                            fieldRef(strnum(QPREFIX, i))));
    }
    const QVector<McqGridSubtitle> sub{
        {6, ""},
        {12, ""},
        {18, ""},
    };

    QuPagePtr page((new QuPage{
        new QuText(xstring("stem")),
        (new QuMcqGrid(qfields, CommonOptions::yesNoChar()))->setSubtitles(sub),
    })->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Mast::totalScore() const
{
    int total = 0;
    for (int q = 1; q <= N_QUESTIONS; ++q) {
        total += score(q);
    }
    return total;
}


int Mast::score(const int question) const
{
    const QVariant v = value(strnum(QPREFIX, question));
    if (v.isNull()) {
        return 0;
    }
    const bool yes = v.toString() == CommonOptions::YES_CHAR;
    const int presence = REVERSED_QUESTIONS.contains(question)
            ? (yes ? 0 : 1)  // reversed (negative responses are alcoholic)
            : (yes ? 1 : 0);  // normal
    int points;
    if (QUESTIONS_SCORING_ONE.contains(question)) {
        points = 1;
    } else if (QUESTIONS_SCORING_FIVE.contains(question)) {
        points = 5;
    } else {
        points = 2;  // most score 2
    }
    return points * presence;
}
