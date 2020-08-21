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

#include "pcl5.h"
#include "core/camcopsapp.h"
#include "common/textconst.h"
#include "common/varconst.h"
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
const int MAX_SCORE = 80;

const QString QPREFIX("q");
const QString Pcl5::PCL5_TABLENAME("pcl5");


void initializePcl5(TaskFactory& factory)
{
    static TaskRegistrar<Pcl5> registered(factory);
}


Pcl5::Pcl5(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
            Task(app, db, PCL5_TABLENAME, false, false, false),  // ... anon, clin, resp
            m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Pcl5::shortname() const
{
    return "PCL-5";
}


QString Pcl5::longname() const
{
    return tr("PTSD Checklist for DSM-5");
}


QString Pcl5::description() const
{
    return tr("20-item self-report scale, based on DSM-5 criteria.");
}


Version Pcl5::minimumServerVersion() const
{
    return Version(2, 2, 8);
}


// ============================================================================
// Instance info
// ============================================================================

bool Pcl5::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Pcl5::summary() const
{
    return QStringList{
        totalScorePhrase(totalScore(), MAX_SCORE),
        standardResult(xstring("dsm_criteria_met"),
                       uifunc::yesNoUnknown(hasPtsd()))
    };
}


QStringList Pcl5::detail() const
{
    QStringList lines = completenessInfo();
    lines += summary();
    return lines;
}


OpenableWidget* Pcl5::editor(const bool read_only)
{
    const NameValueOptions options{
        {xstring("a0"), 0},
        {xstring("a1"), 1},
        {xstring("a2"), 2},
        {xstring("a3"), 3},
        {xstring("a4"), 4},
    };

    QuPagePtr page((new QuPage{
        new QuText(xstring("instructions")),
        (new QuMcqGrid(
            {
                QuestionWithOneField(xstring("q1"), fieldRef("q1")),
                QuestionWithOneField(xstring("q2"), fieldRef("q2")),
                QuestionWithOneField(xstring("q3"), fieldRef("q3")),
                QuestionWithOneField(xstring("q4"), fieldRef("q4")),
                QuestionWithOneField(xstring("q5"), fieldRef("q5")),
                QuestionWithOneField(xstring("q6"), fieldRef("q6")),
                QuestionWithOneField(xstring("q7"), fieldRef("q7")),
                QuestionWithOneField(xstring("q8"), fieldRef("q8")),
                QuestionWithOneField(xstring("q9"), fieldRef("q9")),
                QuestionWithOneField(xstring("q10"), fieldRef("q10")),
                QuestionWithOneField(xstring("q11"), fieldRef("q11")),
                QuestionWithOneField(xstring("q12"), fieldRef("q12")),
                QuestionWithOneField(xstring("q13"), fieldRef("q13")),
                QuestionWithOneField(xstring("q14"), fieldRef("q14")),
                QuestionWithOneField(xstring("q15"), fieldRef("q15")),
                QuestionWithOneField(xstring("q16"), fieldRef("q16")),
                QuestionWithOneField(xstring("q17"), fieldRef("q17")),
                QuestionWithOneField(xstring("q18"), fieldRef("q18")),
                QuestionWithOneField(xstring("q19"), fieldRef("q19")),
                QuestionWithOneField(xstring("q20"), fieldRef("q20"))
            },
            options
        ))->setTitle(xstring("stem")),
    })->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Pcl5::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QVariant Pcl5::hasPtsd() const
{
    // PTSD = at least one "B" item (questions 1-5)
    //        and at least one "C" item (questions 6-7)
    //        and at least two "D" items (questions 8-14)
    //        and at least two "E" items (questions 15-20)
    // https://www.ptsd.va.gov/professional/assessment/adult-sr/ptsd-checklist.asp

    const int first_b = 1;
    const int last_b = 5;
    const int first_c = 6;
    const int last_c = 7;
    const int first_d = 8;
    const int last_d = 14;
    const int first_e = 15;
    const int last_e = 20;

    const int criterion_b = 1;
    const int criterion_c = 1;
    const int criterion_d = 2;
    const int criterion_e = 2;

    const int symptomatic_b = numSymptomatic(first_b, last_b);
    const int symptomatic_c = numSymptomatic(first_c, last_c);
    const int symptomatic_d = numSymptomatic(first_d, last_d);
    const int symptomatic_e = numSymptomatic(first_e, last_e);

    const int null_b = numNull(first_b, last_b);
    const int null_c = numNull(first_c, last_c);
    const int null_d = numNull(first_d, last_d);
    const int null_e = numNull(first_e, last_e);

    if (symptomatic_b >= criterion_b &&
            symptomatic_c >= criterion_c &&
            symptomatic_d >= criterion_d &&
            symptomatic_e >= criterion_e) {
        return true;  // has PTSD
    }
    if (symptomatic_b + null_b >= criterion_b &&
            symptomatic_c + null_c >= criterion_c &&
            symptomatic_d + null_d >= criterion_d &&
            symptomatic_e + null_e >= criterion_e) {
        return QVariant();  // might have PTSD, depending on more info
    }
    return false;  // not PTSD
}


int Pcl5::numSymptomatic(const int first, const int last) const
{
    int total = 0;
    for (int i = first; i <= last; ++i) {
        if (valueInt(strnum(QPREFIX, i)) >= 2) {
            // 2 and above scores as "symptomatic":
            // https://www.ptsd.va.gov/professional/assessment/adult-sr/ptsd-checklist.asp
            ++total;
        }
    }
    return total;
}


int Pcl5::numNull(const int first, const int last) const
{
    return countNull(values(strseq(QPREFIX, first, last)));
}
