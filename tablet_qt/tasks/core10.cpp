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

#include "core10.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "lib/version.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::countNotNull;
using mathfunc::sumInt;
using mathfunc::scorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 10;
const int MAX_SCORE = 40;
const QString QPREFIX("q");
const QVector<int> REVERSE_SCORED_Q{2, 3};  // Q2 and Q3 are reverse-scored

const QString Core10::CORE10_TABLENAME("core10");


void initializeCore10(TaskFactory& factory)
{
    static TaskRegistrar<Core10> registered(factory);
}


Core10::Core10(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CORE10_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Core10::shortname() const
{
    return "CORE-10";
}


QString Core10::longname() const
{
    return tr("Clinical Outcomes in Routine Evaluation, 10-item measure");
}


QString Core10::description() const
{
    return tr("Self-rating of distress (wellbeing, symptoms, functioning, "
              "risk).");
}


Version Core10::minimumServerVersion() const
{
    return Version(2, 2, 8);
}


// ============================================================================
// Instance info
// ============================================================================

bool Core10::isComplete() const
{
    return !anyValuesNull(strseq(QPREFIX, FIRST_Q, N_QUESTIONS));
}


QStringList Core10::summary() const
{
    return QStringList{
        scorePhrase(tr("Clinical score"), clinicalScore(), MAX_SCORE)
    };
}


QStringList Core10::detail() const
{
    const QString spacer = " ";
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", spacer, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Core10::editor(const bool read_only)
{
    const NameValueOptions options_normal{
        {xstring("a0"), 0},
        {xstring("a1"), 1},
        {xstring("a2"), 2},
        {xstring("a3"), 3},
        {xstring("a4"), 4},
    };
    const NameValueOptions options_reversed{
        {xstring("a0"), 4},
        {xstring("a1"), 3},
        {xstring("a2"), 2},
        {xstring("a3"), 1},
        {xstring("a4"), 0},
    };

    // The problem here: two questions (Q2, Q3) are reverse-scored, but
    // we want that to be invisible to the user, while keeping an aligned
    // grid and not repeating titles.
    // 2019-01-04: fixed alignment properly via
    // QuMcqGrid::setAlternateNameValueOptions.

    const int question_width = 50;
    const QVector<int> option_widths{10, 10, 10, 10, 10};

    QVector<int> reversed_indexes;
    for (const int qnum : REVERSE_SCORED_Q) {
        reversed_indexes.append(qnum - 1);  // zero-based indexes
    }
    QVector<QuestionWithOneField> qfp;
    for (int qnum = FIRST_Q; qnum <= N_QUESTIONS; ++qnum) {
        const QString qname = strnum(QPREFIX, qnum);
        const QString qtext = xstring(qname);
        qfp.append(QuestionWithOneField(qtext, fieldRef(qname)));
    }

    QuMcqGrid* grid = new QuMcqGrid(qfp, options_normal);
    grid->setAlternateNameValueOptions(reversed_indexes, options_reversed);
    grid->setTitle(xstring("stem"));
    grid->setWidth(question_width, option_widths);
    grid->setExpand(true);
    grid->setQuestionsBold(false);

    QuPagePtr page((new QuPage{
        (new QuText(xstring("instruction_1")))->setBold(true),
        new QuText(xstring("instruction_2")),
        grid,
        (new QuText(xstring("thanks")))->setBold(true),
    })->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Core10::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


int Core10::nQuestionsCompleted() const
{
    return countNotNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


double Core10::clinicalScore() const
{
    const int n_q_completed = nQuestionsCompleted();
    if (n_q_completed == 0) {
        // Avoid division by zero. Doesn't crash, but does give "nan".
        return 0;
    }
    return static_cast<double>(N_QUESTIONS * totalScore()) /
            static_cast<double>(n_q_completed);
}
