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

// By Joe Kearney, Rudolf Cardinal.

#include "cesd.h"
#include "core/camcopsapp.h"
#include "lib/version.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "maths/mathfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"

using mathfunc::countNull;
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 20;
const int MAX_SCORE = 60;
const int DEPRESSION_RISK_THRESHOLD = 16;

const QVector<int> REVERSE_SCORED_QUESTIONS{4, 8, 12, 16};

const QString QPREFIX("q");
const QString Cesd::CESD_TABLENAME("cesd");


void initializeCesd(TaskFactory& factory)
{
    static TaskRegistrar<Cesd> registered(factory);
}


Cesd::Cesd(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
            Task(app, db, CESD_TABLENAME, false, false, false),
            m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Cesd::shortname() const
{
    return "CESD";
}


QString Cesd::longname() const
{
    return tr("Center for Epidemiologic Studies Depression Scale");
}


QString Cesd::description() const
{
    return tr("20-item self-report depression scale.");
}


Version Cesd::minimumServerVersion() const
{
    return Version(2, 2, 8);
}


// ============================================================================
// Instance info
// ============================================================================

bool Cesd::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Cesd::summary() const
{
    return QStringList{
        totalScorePhrase(totalScore(), MAX_SCORE),
        standardResult(xstring("depression_or_risk_of"),
                       uifunc::yesNoUnknown(hasDepressionRisk()))
    };
}


QStringList Cesd::detail() const
{
    QStringList lines = completenessInfo();
    lines += summary();
    return lines;
}


OpenableWidget* Cesd::editor(const bool read_only)
{
    const NameValueOptions options{
        {xstring("a0"), 0},
        {xstring("a1"), 1},
        {xstring("a2"), 2},
        {xstring("a3"), 3}
    };

    const int question_width = 40;
    const QVector<int> option_widths{15, 15, 15, 15};

    QuPagePtr page((new QuPage{
        new QuText(xstring("instruction"))
    })->setTitle(xstring("title")));

    QVector<QuestionWithOneField> question_field_pairs;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        const QString field_and_q_name = stringfunc::strnum("q", q);
        question_field_pairs.append(
            QuestionWithOneField(xstring(field_and_q_name),
                                 fieldRef(field_and_q_name))
        );
    }
    page->addElement(
        (new QuMcqGrid(question_field_pairs, options))
                ->setTitle(xstring("stem"))
                ->setWidth(question_width, option_widths)
                ->setExpand(true)
                ->setQuestionsBold(false)
    );

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Cesd::totalScore() const
{
    // Need to store values as per original then flip here
    int total = 0;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        QVariant v = value(stringfunc::strnum("q", q));
        if (v.isNull()) {
            continue;
        }
        const int score = v.toInt();
        if (REVERSE_SCORED_QUESTIONS.contains(q)) {
            total += 3 - score;
        } else {
            total += score;
        }
    }
    return total;
}


QVariant Cesd::hasDepressionRisk() const
{
    return totalScore() >= DEPRESSION_RISK_THRESHOLD;
}


int Cesd::numNull(const int first, const int last) const
{
    return countNull(values(strseq(QPREFIX, first, last)));
}
