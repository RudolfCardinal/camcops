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

#include "bprs.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::scorePhrase;
using mathfunc::sumInt;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int LAST_SCORED_Q = 18;
const int N_QUESTIONS = 20;
const int MAX_SCORE = 126;
const QString QPREFIX("q");

const QString Bprs::BPRS_TABLENAME("bprs");

// Some scales use 9 for "not assessed"; we'll use 0 (as in the original BPRS).


void initializeBprs(TaskFactory& factory)
{
    static TaskRegistrar<Bprs> registered(factory);
}


Bprs::Bprs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, BPRS_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Bprs::shortname() const
{
    return "BPRS";
}


QString Bprs::longname() const
{
    return tr("Brief Psychiatric Rating Scale");
}


QString Bprs::description() const
{
    return tr("18-item clinician-administered rating of multiple aspects of "
              "psychopathology.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Bprs::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Bprs::summary() const
{
    return QStringList{scorePhrase(xstring("bprs18_total_score"),
                                   totalScore(), MAX_SCORE, " ", "")};
}


QStringList Bprs::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Bprs::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto addpage = [this, &pages](int n, bool include_na) -> void {
        NameValueOptions options;
        for (int i = 1; i <= 7; ++i) {
            const QString name = xstring(QString("q%1_option%2").arg(n).arg(i));
            options.append(NameValuePair(name, i));
        }
        if (include_na) {
            const QString name = xstring(QString("q%1_option0").arg(n));
            options.append(NameValuePair(name, 0));
        }
        const QString pagetitle = xstring(QString("q%1_title").arg(n));
        const QString question = xstring(QString("q%1_question").arg(n));
        const QString fieldname = strnum(QPREFIX, n);
        QuPagePtr page((new QuPage{
            new QuText(question),
            new QuMcq(fieldRef(fieldname), options),
        })->setTitle(pagetitle));
        pages.append(page);
    };

    pages.append(getClinicianDetailsPage());
    for (int n = FIRST_Q; n <= N_QUESTIONS; ++n) {
        const bool include_na = (
                    n == 1 || n == 2 || n == 5 || n == 8 || n == 9 ||
                    n == 10 || n == 11 || n == 12 || n == 15 ||
                    n == 18 || n == 20);
        addpage(n, include_na);
    }

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Bprs::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, LAST_SCORED_Q)));
}
