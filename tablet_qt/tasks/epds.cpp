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

#include "epds.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 10;  // each scored 0-3
const int MAX_SCORE = 30;
const QString QPREFIX("q");
const int CUTOFF_1_GREATER_OR_EQUAL = 10;  // Cox et al. 1987, PubMed ID 3651732.
const int CUTOFF_2_GREATER_OR_EQUAL = 13;  // Cox et al. 1987, PubMed ID 3651732.

const QVector<int> REVERSE_QUESTIONS{3, 5, 6, 7, 8, 9, 10};  // only 1, 2, 4 the other way

const QString Epds::EPDS_TABLENAME("epds");


void initializeEpds(TaskFactory& factory)
{
    static TaskRegistrar<Epds> registered(factory);
}


Epds::Epds(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, EPDS_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Epds::shortname() const
{
    return "EPDS";
}


QString Epds::longname() const
{
    return tr("Edinburgh Postnatal Depression Scale");
}


QString Epds::description() const
{
    return tr("10-item self-rating scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Epds::isComplete() const
{
    return noValuesNull(strseq(QPREFIX, FIRST_Q, N_QUESTIONS));
}


QStringList Epds::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Epds::detail() const
{
    using stringfunc::bold;
    using uifunc::yesNo;

    const int totalscore = totalScore();
    const bool above_cutoff_1 = totalscore >= CUTOFF_1_GREATER_OR_EQUAL;
    const bool above_cutoff_2 = totalscore >= CUTOFF_2_GREATER_OR_EQUAL;
    const QString spacer1 = ": ";
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", spacer1, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    lines.append("");
    const QString spacer2 = " ";
    lines.append(xstring("above_cutoff_1") + spacer2 + bold(yesNo(above_cutoff_1)));
    lines.append(xstring("above_cutoff_2") + spacer2 + bold(yesNo(above_cutoff_2)));
    lines.append(xstring("always_look_at_suicide"));
    return lines;
}


OpenableWidget* Epds::editor(const bool read_only)
{
    QuPagePtr page((new QuPage{
        (new QuText(xstring("question_common")))->setBold(true),
        new QuSpacer(),
    }));
    page->setTitle(xstring("title_main"));

    auto addQ = [this, &page](const int qnum) -> void {
        const QString fieldname = strnum("q", qnum);
        NameValueOptions options{
            {xstring(QString("q%1_option0").arg(qnum)), 0},
            {xstring(QString("q%1_option1").arg(qnum)), 1},
            {xstring(QString("q%1_option2").arg(qnum)), 2},
            {xstring(QString("q%1_option3").arg(qnum)), 3},
        };
        if (REVERSE_QUESTIONS.contains(qnum)) {
            options.reverse();
        }
        page->addElements({
            (new QuText(xstring(QString("q%1_question").arg(qnum))))->setBold(),
            new QuMcq(fieldRef(fieldname), options),
            new QuSpacer(),
        });
    };

    for (int qnum = FIRST_Q; qnum <= N_QUESTIONS; ++qnum) {
        addQ(qnum);
    }

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);
    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Epds::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
