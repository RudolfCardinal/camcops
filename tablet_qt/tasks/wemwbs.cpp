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

#include "wemwbs.h"
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
const int N_QUESTIONS = 14;
const int MIN_Q_SCORE = 1;
const int MAX_Q_SCORE = 5;
const QString QPREFIX("q");

const QString Wemwbs::WEMWBS_TABLENAME("wemwbs");


void initializeWemwbs(TaskFactory& factory)
{
    static TaskRegistrar<Wemwbs> registered(factory);
}


Wemwbs::Wemwbs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, WEMWBS_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Wemwbs::shortname() const
{
    return "WEMWBS";
}


QString Wemwbs::longname() const
{
    return tr("Warwick–Edinburgh Mental Well-Being Scale");
}


QString Wemwbs::description() const
{
    return tr("14 positively-phrased Likert-style items measuring mental "
              "well-being over the preceding 2 weeks.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Wemwbs::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Wemwbs::summary() const
{
    return QStringList{
        QString("%1 <b>%2</b> (range %3–%4)")
                .arg(TextConst::totalScore())
                .arg(totalScore())
                .arg(N_QUESTIONS * MIN_Q_SCORE)
                .arg(N_QUESTIONS * MAX_Q_SCORE)
    };
}


QStringList Wemwbs::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("wemwbs_q", "", ": ",
                            QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Wemwbs::editor(const bool read_only)
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
        qfields.append(QuestionWithOneField(xstring(strnum("wemwbs_q", i)),
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

int Wemwbs::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
