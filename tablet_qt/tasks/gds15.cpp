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

#include "gds15.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 15;
const int MAX_SCORE = N_QUESTIONS;
const QString QPREFIX("q");

const QString Gds15::GDS15_TABLENAME("gds15");

const QVector<int> SCORE_IF_YES{2, 3, 4, 6, 8, 9, 10, 12, 14, 15};
const QVector<int> SCORE_IF_NO{1, 5, 7, 11, 13};


void initializeGds15(TaskFactory& factory)
{
    static TaskRegistrar<Gds15> registered(factory);
}


Gds15::Gds15(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, GDS15_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::String);  // Y,N

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Gds15::shortname() const
{
    return "GDS-15";
}


QString Gds15::longname() const
{
    return tr("Geriatric Depression Scale, 15-item version");
}


QString Gds15::description() const
{
    return tr("15-item self-report scale.");
}


QString Gds15::infoFilenameStem() const
{
    return "gds";
}


// ============================================================================
// Instance info
// ============================================================================

bool Gds15::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Gds15::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Gds15::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Gds15::editor(const bool read_only)
{
    const NameValueOptions options = CommonOptions::yesNoChar();
    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfields.append(QuestionWithOneField(xstring(strnum("q", i)),
                                            fieldRef(strnum(QPREFIX, i))));
    }

    QuPagePtr page((new QuPage{
        new QuText(xstring("instruction")),
        new QuMcqGrid(qfields, options),
    })->setTitle(shortname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Gds15::totalScore() const
{
    int score = 0;
    for (auto q : SCORE_IF_YES) {
        if (valueString(strnum(QPREFIX, q)) == CommonOptions::YES_CHAR) {
            ++score;
        }
    }
    for (auto q : SCORE_IF_NO) {
        if (valueString(strnum(QPREFIX, q)) == CommonOptions::NO_CHAR) {
            ++score;
        }
    }
    return score;
}
