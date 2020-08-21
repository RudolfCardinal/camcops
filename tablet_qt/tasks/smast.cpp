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

#include "smast.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::totalScorePhrase;
using stringfunc::bold;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 13;
const int MAX_SCORE = N_QUESTIONS;
const QString QPREFIX("q");

const QVector<int> REVERSE_SCORED_Q{1, 4, 5};

const QString Smast::SMAST_TABLENAME("smast");


void initializeSmast(TaskFactory& factory)
{
    static TaskRegistrar<Smast> registered(factory);
}


Smast::Smast(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, SMAST_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Smast::shortname() const
{
    return "SMAST";
}


QString Smast::longname() const
{
    return tr("Short Michigan Alcohol Screening Test");
}


QString Smast::description() const
{
    return tr("13-item Y/N self-report scale.");
}


QString Smast::infoFilenameStem() const
{
    return "mast";
}


// ============================================================================
// Instance info
// ============================================================================

bool Smast::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Smast::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Smast::detail() const
{
    const int total = totalScore();
    const QString likelihood =
            total >= 3 ? xstring("problem_probable")
                       : (total >= 2 ? xstring("problem_possible")
                                     : xstring("problem_unlikely"));
    const QString scores = ", " + xstring("scores") + " ";

    QStringList lines = completenessInfo();
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        lines.append(fieldSummary(strnum(QPREFIX, q),
                                  xstring(strnum("q", q, "_s")),
                                  " ") +
                     scores + bold(QString::number(score(q))));
    }
    lines.append("");
    lines += summary();
    lines.append("");
    lines.append(xstring("problem_likelihood") + " " + bold(likelihood));
    return lines;
}


OpenableWidget* Smast::editor(const bool read_only)
{
    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfields.append(QuestionWithOneField(xstring(strnum("q", i)),
                                            fieldRef(strnum(QPREFIX, i))));
    }
    const QVector<McqGridSubtitle> sub{
        {5, ""},
        {10, ""},
        {15, ""},
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

int Smast::score(const int question) const
{
    const QVariant v = value(strnum(QPREFIX, question));
    if (v.isNull()) {
        return 0;  // to avoid silly scoring of incomplete tasks
    }
    const bool yes = v.toString() == CommonOptions::YES_CHAR;
    if (REVERSE_SCORED_Q.contains(question)) {
        return yes ? 0 : 1;
    }
    return yes ? 1 : 0;
}


int Smast::totalScore() const
{
    int total = 0;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        total += score(q);
    }
    return total;
}
