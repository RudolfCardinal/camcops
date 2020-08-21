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

#include "aims.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int LAST_SCORED_Q = 10;
const int N_QUESTIONS = 12;
const int MAX_SCORE = 40;
const QString QPREFIX("q");

const QString Aims::AIMS_TABLENAME("aims");


void initializeAims(TaskFactory& factory)
{
    static TaskRegistrar<Aims> registered(factory);
}


Aims::Aims(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, AIMS_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Aims::shortname() const
{
    return "AIMS";
}


QString Aims::longname() const
{
    return tr("Abnormal Involuntary Movement Scale");
}


QString Aims::description() const
{
    return tr("14-item clinician-rated scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Aims::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Aims::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Aims::detail() const
{
    const QString spacer = " ";
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", spacer,
                            QPREFIX, FIRST_Q, LAST_SCORED_Q);
    lines += fieldSummariesYesNo("q", "_s", spacer,
                                 QPREFIX, LAST_SCORED_Q + 1, N_QUESTIONS);
    lines += summary();
    return lines;
}


OpenableWidget* Aims::editor(const bool read_only)
{
    const NameValueOptions options_q1_8{
        {xstring("main_option0"), 0},
        {xstring("main_option1"), 1},
        {xstring("main_option2"), 2},
        {xstring("main_option3"), 3},
        {xstring("main_option4"), 4},
    };
    const NameValueOptions options_q9{
        {xstring("q9_option0"), 0},  // different
        {xstring("main_option1"), 1},
        {xstring("main_option2"), 2},
        {xstring("main_option3"), 3},
        {xstring("main_option4"), 4},
    };
    const NameValueOptions options_q10{
        {xstring("q10_option0"), 0},
        {xstring("q10_option1"), 1},
        {xstring("q10_option2"), 2},
        {xstring("q10_option3"), 3},
        {xstring("q10_option4"), 4},
    };

    QuPagePtr page1((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        new QuText(xstring("intro_info")),
    })
        ->setTitle(xstring("intro_title"))
        ->setType(QuPage::PageType::Clinician));

    QuPagePtr page2((new QuPage{
        new QuText(xstring("section1_stem")),
        (new QuMcqGrid(
            {
                QuestionWithOneField(xstring("q1_question"), fieldRef("q1")),
                QuestionWithOneField(xstring("q2_question"), fieldRef("q2")),
                QuestionWithOneField(xstring("q3_question"), fieldRef("q3")),
                QuestionWithOneField(xstring("q4_question"), fieldRef("q4")),
                QuestionWithOneField(xstring("q5_question"), fieldRef("q5")),
                QuestionWithOneField(xstring("q6_question"), fieldRef("q6")),
                QuestionWithOneField(xstring("q7_question"), fieldRef("q7")),
                QuestionWithOneField(xstring("q8_question"), fieldRef("q8")),
            },
            options_q1_8
        ))
            ->setTitle(xstring("q1_subtitle"))
            ->setSubtitles({
                McqGridSubtitle(5 - 1, xstring("q5_subtitle")),
                McqGridSubtitle(7 - 1, xstring("q7_subtitle")),
                McqGridSubtitle(8 - 1, xstring("q8_subtitle")),
            }),
    })->setTitle(xstring("section1_title")));

    QuPagePtr page3((new QuPage{
        (new QuText(xstring("q9_question")))->setBold(),
        new QuMcq(fieldRef("q9"), options_q9),
    })->setTitle(xstring("section2_title")));

    QuPagePtr page4((new QuPage{
        (new QuText(xstring("q10_question")))->setBold(),
        new QuMcq(fieldRef("q10"), options_q10),
    })->setTitle(xstring("section3_title")));

    QuPagePtr page5((new QuPage{
        new QuMcqGrid(
            {
                QuestionWithOneField(xstring("q11_question"), fieldRef("q11")),
                QuestionWithOneField(xstring("q12_question"), fieldRef("q12")),
            },
            CommonOptions::noYesInteger()
        ),
    })->setTitle(xstring("section4_title")));

    auto questionnaire = new Questionnaire(
            m_app, {page1, page2, page3, page4, page5});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Aims::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, LAST_SCORED_Q)));
}
