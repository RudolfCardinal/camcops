/*
    Copyright (C) 2012-2017 Rudolf Cardinal (rudolf@pobox.com).

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

#include "hama.h"
#include "common/textconst.h"
#include "lib/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::scoreString;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 14;
const int MAX_SCORE = 56;
const QString QPREFIX("q");

const QString HAMA_TABLENAME("hama");


void initializeHamA(TaskFactory& factory)
{
    static TaskRegistrar<HamA> registered(factory);
}


HamA::HamA(CamcopsApp& app, const QSqlDatabase& db, int load_pk) :
    Task(app, db, HAMA_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString HamA::shortname() const
{
    return "HAM-A";
}


QString HamA::longname() const
{
    return tr("Hamilton Rating Scale for Anxiety (HAM-A)");
}


QString HamA::menusubtitle() const
{
    return tr("14-item clinician-administered scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool HamA::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList HamA::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList HamA::detail() const
{
    int score = totalScore();
    QString severity = score >= 31
            ? textconst::VERY_SEVERE
            : (score >= 25 ? textconst::MODERATE_TO_SEVERE
                           : score >= 18 ? textconst::MILD_TO_MODERATE
                                         : textconst::MILD);
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    lines.append("");
    lines.append(standardResult(xstring("symptom_severity"),
                                severity));
    return lines;
}


OpenableWidget* HamA::editor(bool read_only)
{
    QList<QuPagePtr> pages;

    auto addpage = [this, &pages](int n) -> void {
        NameValueOptions options;
        for (int i = 0; i <= 4; ++i) {
            QString name = xstring(QString("q%1_option%2").arg(n).arg(i));
            options.addItem(NameValuePair(name, i));
        }
        QString pagetitle = xstring(QString("q%1_title").arg(n));
        QString question = xstring(QString("q%1_question").arg(n));
        QString fieldname = strnum(QPREFIX, n);
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

    Questionnaire* questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int HamA::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
