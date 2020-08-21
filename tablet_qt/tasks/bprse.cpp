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

#include "bprse.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 24;
const int MAX_SCORE = 168;
const QString QPREFIX("q");

const QString BprsE::BPRSE_TABLENAME("bprse");


void initializeBprsE(TaskFactory& factory)
{
    static TaskRegistrar<BprsE> registered(factory);
}


BprsE::BprsE(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, BPRSE_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString BprsE::shortname() const
{
    return "BPRS-E";
}


QString BprsE::longname() const
{
    return tr("Brief Psychiatric Rating Scale, Expanded");
}


QString BprsE::description() const
{
    return tr("24-item clinician-administered rating of multiple aspects of "
              "psychopathology.");
}


// ============================================================================
// Instance info
// ============================================================================

bool BprsE::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList BprsE::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList BprsE::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* BprsE::editor(const bool read_only)
{
    NameValuePair option0(xstring("option0"), 0);
    NameValuePair option1(xstring("option1"), 1);
    QVector<QuPagePtr> pages;

    auto addpage = [this, &pages, &option0, &option1](int n) -> void {
        NameValueOptions options{option0, option1};
        for (int i = 2; i <= 7; ++i) {
            const QString name = xstring(QString("q%1_option%2").arg(n).arg(i));
            options.append(NameValuePair(name, i));
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
        addpage(n);
    }

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int BprsE::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
