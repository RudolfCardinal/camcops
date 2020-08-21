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

#include "fast.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
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
const int N_QUESTIONS = 4;
const int MAX_SCORE = 16;
const QString QPREFIX("q");

const QString Fast::FAST_TABLENAME("fast");


void initializeFast(TaskFactory& factory)
{
    static TaskRegistrar<Fast> registered(factory);
}


Fast::Fast(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, FAST_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Fast::shortname() const
{
    return "FAST";
}


QString Fast::longname() const
{
    return tr("Fast Alcohol Screening Test");
}


QString Fast::description() const
{
    return tr("4-item self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Fast::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Fast::summary() const
{
    return QStringList{stringfunc::standardResult(xstring("positive"),
                                                  uifunc::yesNo(isPositive()),
                                                  " ")};
}


QStringList Fast::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ",
                            QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines.append(totalScorePhrase(totalScore(), MAX_SCORE));
    lines += summary();
    return lines;
}


OpenableWidget* Fast::editor(const bool read_only)
{
    const NameValueOptions main_options{
        {xstring("q1to3_option0"), 0},
        {xstring("q1to3_option1"), 1},
        {xstring("q1to3_option2"), 2},
        {xstring("q1to3_option3"), 3},
        {xstring("q1to3_option4"), 4},
    };
    const NameValueOptions q4_options{
        {xstring("q4_option0"), 0},
        {xstring("q4_option2"), 2},
        {xstring("q4_option4"), 4},
    };

    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold(true);
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options) -> QuElement* {
        return new QuMcq(fieldRef(fieldname), options);
    };

    QuPagePtr page((new QuPage{
        text("info"),
        boldtext("q1"),
        mcq(strnum(QPREFIX, 1), main_options),
        boldtext("q2"),
        mcq(strnum(QPREFIX, 2), main_options),
        boldtext("q3"),
        mcq(strnum(QPREFIX, 3), main_options),
        boldtext("q4"),
        mcq(strnum(QPREFIX, 4), q4_options),
    })->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Fast::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


bool Fast::isPositive() const
{
    const int q1 = valueInt(strnum(QPREFIX, 1));
    if (q1 == 0) {
        return false;  // "Never"
    }
    if (q1 == 3 || q1 == 4) {
        return true;  // "Weekly" or "Daily"
    }
    return totalScore() >= 3;
}
