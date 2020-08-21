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

#include "apeqpt.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/datetime.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quhorizontalcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "questionnairelib/quverticalcontainer.h"
#include "tasklib/taskfactory.h"

using mathfunc::anyNullOrEmpty;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strseq;

const QString Apeqpt::APEQPT_TABLENAME("apeqpt");

const QString FN_DATETIME("q_datetime");

const QString CHOICE_SUFFIX("_choice");
const QString SAT_SUFFIX("_satisfaction");

const int CHOICE_QUESTIONS_N = 3;

const QString FN_Q1_SATISFACTION("q1_satisfaction");
const QString FN_Q2_SATISFACTION("q2_satisfaction");


void initializeApeqpt(TaskFactory& factory)
{
    static TaskRegistrar<Apeqpt> registered(factory);
}


Apeqpt::Apeqpt(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, APEQPT_TABLENAME, true, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(FN_DATETIME, QVariant::DateTime);

    for (const QString& field : strseq("q", 1, CHOICE_QUESTIONS_N, CHOICE_SUFFIX)) {
        addField(field, QVariant::Int);
    }

    addField(FN_Q1_SATISFACTION, QVariant::Int);
    addField(FN_Q2_SATISFACTION, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(FN_DATETIME, datetime::now(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString Apeqpt::shortname() const
{
    return "APEQPT";
}


QString Apeqpt::longname() const
{
    return tr("Assessment Patient Experience Questionnaire "
              "for Psychological Therapies");
}


QString Apeqpt::description() const
{
    return tr("Patient feedback questionnaire on assessment for psychological "
              "therapy/choosing treatment.");
}

// ============================================================================
// Instance info
// ============================================================================

bool Apeqpt::isComplete() const
{
    QStringList required_always{FN_Q1_SATISFACTION};
    for (const QString& field : strseq("q", 1, CHOICE_QUESTIONS_N, CHOICE_SUFFIX)) {
        required_always.append(field);
    }
    required_always.append(FN_DATETIME);
    return !anyNullOrEmpty(values(required_always));
}


NameValueOptions Apeqpt::optionsSatisfaction() const
{
    return NameValueOptions{
        {xstring("a4_satisfaction"), 4},  // Completely satisfied
        {xstring("a3_satisfaction"), 3},
        {xstring("a2_satisfaction"), 2},
        {xstring("a1_satisfaction"), 1},
        {xstring("a0_satisfaction"), 0},  // Not at all satisfied
    };
}


NameValueOptions Apeqpt::optionsChoiceWithNA() const
{
    return NameValueOptions{
        {xstring("a1_choice"), 1},  // Yes
        {xstring("a0_choice"), 0},  // No
        {xstring("a2_choice"), 2},  // N/A
    };
}



QStringList Apeqpt::summary() const
{
    const NameValueOptions options_satisfaction = optionsSatisfaction();
    return QStringList{
        QString("Patient Satisfaction: %1").arg(
            options_satisfaction.nameFromValue(value(FN_Q1_SATISFACTION)))
    };
}


QStringList Apeqpt::detail() const
{
    QStringList lines = completenessInfo();
    const NameValueOptions ans = optionsChoiceWithNA();
    const QString spacer = " ";
    QString summaryLine, xstringname, fieldname, qnum;
    lines.append("<b>Choice</b>:");
    for (int i = 0; i < CHOICE_QUESTIONS_N; ++i) {
        qnum = QString::number(i+1);
        xstringname = QString("q%1_choice_s").arg(qnum);
        fieldname = QString("q%1_choice").arg(qnum);
        summaryLine = QString("Q%1 %2: %3").arg(
                    qnum, xstring(xstringname),
                    ans.nameFromValue(value(fieldname)));
        lines.append(summaryLine);
    }
    lines.append("");
    lines.append("<b>Satisfaction</b>:");
    lines.append(summary());
    lines.append("");
    lines.append("<b>Additional feedback</b>:");
    lines.append(value(FN_Q2_SATISFACTION).toString());
    return lines;
}


OpenableWidget* Apeqpt::editor(const bool read_only)
{
    const NameValueOptions options_choice{
        {xstring("a1_choice"), 1},  // Yes
        {xstring("a0_choice"), 0},  // No
    };
    const NameValueOptions options_choice_with_na = optionsChoiceWithNA();
    const NameValueOptions options_satisfaction = optionsSatisfaction();

    const int question_width = 25;
    const QVector<int>yes_no_opts_widths{ 38, 37 };
    const QVector<int>all_opts_widths{ 25, 25, 25 };

    QuPagePtr page(new QuPage{
        (new QuText(xstring("instructions_to_subject_1")))->setItalic()->setBig(),
        (new QuText(xstring("instructions_to_subject_2")))->setItalic()->setBig(),
        (new QuText(xstring("q_date")))->setBold(),
        (new QuDateTime(fieldRef(FN_DATETIME)))->setOfferNowButton(true),
        (new QuText(xstring("h1")))->setBig()->setBold(),
        (new QuMcqGrid(
            {
                QuestionWithOneField(xstring("q1_choice"), fieldRef("q1_choice")),
                QuestionWithOneField(xstring("q2_choice"), fieldRef("q2_choice")),
            }, options_choice
         ))->setWidth(question_width, yes_no_opts_widths)->setExpand(true),
        (new QuMcqGrid(
            {
                QuestionWithOneField(xstring("q3_choice"), fieldRef("q3_choice")),
            }, options_choice_with_na
        ))->setWidth(question_width, all_opts_widths)->setExpand(true),
        (new QuText(xstring("h2")))->setBig()->setBold(),
        (new QuMcq(fieldRef(FN_Q1_SATISFACTION), options_satisfaction))
                       ->setHorizontal(true)
                       ->setAsTextButton(true),
        (new QuText(xstring("q2_satisfaction")))->setBold(),
        new QuTextEdit(fieldRef(FN_Q2_SATISFACTION, false)),
        (new QuText(xstring("thanks")))->setItalic(),
    });

    page->setTitle(longname());

    m_questionnaire = new Questionnaire(m_app, {page});

    m_questionnaire->setReadOnly(read_only);

    return m_questionnaire;
}
