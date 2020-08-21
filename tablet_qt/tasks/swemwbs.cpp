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

#include "swemwbs.h"
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
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 7;
const int MIN_Q_SCORE = 1;
const int MAX_Q_SCORE = 5;
const QString QPREFIX("q");

const QString Swemwbs::SWEMWBS_TABLENAME("swemwbs");


void initializeSwemwbs(TaskFactory& factory)
{
    static TaskRegistrar<Swemwbs> registered(factory);
}


Swemwbs::Swemwbs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, SWEMWBS_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Swemwbs::shortname() const
{
    return "SWEMWBS";
}


QString Swemwbs::longname() const
{
    return tr("Short Warwick–Edinburgh Mental Well-Being Scale");
}


QString Swemwbs::description() const
{
    return tr("7-item shortened version of the WEMWBS.");
}


QString Swemwbs::infoFilenameStem() const
{
    return "wemwbs";
}


QString Swemwbs::xstringTaskname() const
{
    return "wemwbs";
}


// ============================================================================
// Instance info
// ============================================================================

bool Swemwbs::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Swemwbs::summary() const
{
    return QStringList{
        QString("%1 <b>%2</b> (range %3–%4)")
                .arg(TextConst::totalScore())
                .arg(totalScore())
                .arg(N_QUESTIONS * MIN_Q_SCORE)
                .arg(N_QUESTIONS * MAX_Q_SCORE)
    };
}


QStringList Swemwbs::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("swemwbs_q", "", ": ",
                            QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Swemwbs::editor(const bool read_only)
{
    const NameValueOptions options{
        {xstring("wemwbs_a1"), 1},
        {xstring("wemwbs_a2"), 2},
        {xstring("wemwbs_a3"), 3},
        {xstring("wemwbs_a4"), 4},
        {xstring("wemwbs_a5"), 5},
    };

    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfields.append(QuestionWithOneField(xstring(strnum("swemwbs_q", i)),
                                            fieldRef(strnum(QPREFIX, i))));
    }

    QuPagePtr page((new QuPage{
        (new QuText(xstring("wemwbs_prompt")))->setBold(),
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

int Swemwbs::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
