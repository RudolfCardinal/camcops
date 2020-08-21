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

#include "gad7.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
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
const int MAX_SCORE = 21;
const QString QPREFIX("q");

const QString Gad7::GAD7_TABLENAME("gad7");


void initializeGad7(TaskFactory& factory)
{
    static TaskRegistrar<Gad7> registered(factory);
}


Gad7::Gad7(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, GAD7_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Gad7::shortname() const
{
    return "GAD-7";
}


QString Gad7::longname() const
{
    return tr("Generalized Anxiety Disorder Assessment");
}


QString Gad7::description() const
{
    return tr("7-item self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Gad7::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Gad7::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Gad7::detail() const
{
    const int total = totalScore();
    const QString severity =
            total >= 15 ? TextConst::severe()
                        : (total >= 10 ? TextConst::moderate()
                                       : (total >= 5 ? TextConst::mild()
                                                     : TextConst::none()));

    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    lines.append("");
    lines.append(standardResult(xstring("anxiety_severity"), severity));
    return lines;
}


OpenableWidget* Gad7::editor(const bool read_only)
{
    const NameValueOptions options{
        {xstring("a0"), 0},
        {xstring("a1"), 1},
        {xstring("a2"), 2},
        {xstring("a3"), 3},
    };
    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfields.append(QuestionWithOneField(fieldRef(strnum(QPREFIX, i)),
                                            xstring(strnum("q", i))));
    }

    QuPagePtr page((new QuPage{
        new QuText(xstring("stem")),
        new QuMcqGrid(qfields, options),
    })->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Gad7::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
