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

#include "pswq.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 16;
const QString QPREFIX("q");
const QVector<int> REVERSE_SCORE{1, 3, 8, 10, 11};

const QString Pswq::PSWQ_TABLENAME("pswq");


void initializePswq(TaskFactory& factory)
{
    static TaskRegistrar<Pswq> registered(factory);
}


Pswq::Pswq(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, PSWQ_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Pswq::shortname() const
{
    return "PSWQ";
}


QString Pswq::longname() const
{
    return tr("Penn State Worry Questionnaire");
}


QString Pswq::description() const
{
    return tr("16-item self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Pswq::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Pswq::summary() const
{
    return QStringList{
        QString("%1: <b>%2</b> (range 16–80)")
                .arg(TextConst::totalScore())
                .arg(totalScore()),
    };
}


QStringList Pswq::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "", ": ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Pswq::editor(const bool read_only)
{
    const NameValueOptions options{
        {"1: " + xstring("anchor1"), 1},
        {"2", 2},
        {"3", 3},
        {"4", 4},
        {"5: " + xstring("anchor1"), 5},
    };
    QVector<QuestionWithOneField> qfields;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        qfields.append(QuestionWithOneField(xstring(strnum("q", q)),
                                            fieldRef(strnum(QPREFIX, q))));
    }

    QuPagePtr page((new QuPage{
        (new QuText(xstring("instruction")))->setBold(true),
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

int Pswq::totalScore() const
{
    int total = 0;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        const QVariant v = value(strnum(QPREFIX, q));
        if (v.isNull()) {
            continue;
        }
        int x = v.toInt();
        if (REVERSE_SCORE.contains(q)) {
            x = 6 - x;  // 5 becomes 1, 1 becomes 5
        }
        total += x;
    }
    return total;
}
