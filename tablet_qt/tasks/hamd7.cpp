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

#include "hamd7.h"
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
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 7;
const int MAX_SCORE = 26;

const QString HamD7::HAMD7_TABLENAME("hamd7");
const QString QPREFIX("q");


void initializeHamD7(TaskFactory& factory)
{
    static TaskRegistrar<HamD7> registered(factory);
}


HamD7::HamD7(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, HAMD7_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString HamD7::shortname() const
{
    return "HAMD-7";
}


QString HamD7::longname() const
{
    return tr("Hamilton Depression Rating Scale, 7-item version");
}


QString HamD7::description() const
{
    return tr("7-item derivative of the HDRS.");
}


QString HamD7::infoFilenameStem() const
{
    return "hamd";
}


// ============================================================================
// Instance info
// ============================================================================

bool HamD7::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList HamD7::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList HamD7::detail() const
{
    const int score = totalScore();
    const QString severity = (
        score >= 20 ? xstring("severity_severe")
                    : (score >= 12 ? xstring("severity_moderate")
                                   : (score >= 4 ? xstring("severity_mild")
                                                 : xstring("severity_none"))));
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    lines.append(standardResult(xstring("severity"), severity));
    return lines;
}


OpenableWidget* HamD7::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto addpage = [this, &pages](int n) -> void {
        NameValueOptions options;
        int n_options = nOptions(n);
        for (int i = 0; i < n_options; ++i) {
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

int HamD7::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


int HamD7::nOptions(const int question) const
{
    return question == 6 ? 3 : 5;
}
