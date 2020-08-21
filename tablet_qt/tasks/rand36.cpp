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

#include "rand36.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::seq;
using mathfunc::meanOrNull;
using mathfunc::noneNull;
using mathfunc::scorePhraseVariant;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 36;
const int MAX_SCORE = 100;  // overall, or for subscales
const QString QPREFIX("q");

const QString Rand36::RAND36_TABLENAME("rand36");

const QVector<int> CODE_5STEP_DOWN{1, 2, 20, 22, 34, 36};
const QVector<int> CODE_3STEP_UP{3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
const QVector<int> CODE_2STEP_UP{13, 14, 15, 16, 17, 18, 19};
const QVector<int> CODE_6STEP_DOWN{21, 23, 26, 27, 30};
const QVector<int> CODE_6STEP_UP{24, 25, 28, 29, 31};
const QVector<int> CODE_5STEP_UP{32, 33, 35};

const QVector<int> PHYSICAL_FUNCTIONING_Q{3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
const QVector<int> ROLE_LIMITATIONS_PHYSICAL_Q{13, 14, 15, 16};
const QVector<int> ROLE_LIMITATIONS_EMOTIONAL_Q{17, 18, 19};
const QVector<int> ENERGY_Q{23, 27, 29, 31};
const QVector<int> EMOTIONAL_WELLBEING_Q{24, 25, 26, 28, 30};
const QVector<int> SOCIAL_FUNCTIONING_Q{20, 32};
const QVector<int> PAIN_Q{21, 22};
const QVector<int> GENERAL_HEALTH_Q{1, 33, 34, 35, 36};



void initializeRand36(TaskFactory& factory)
{
    static TaskRegistrar<Rand36> registered(factory);
}


Rand36::Rand36(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, RAND36_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Rand36::shortname() const
{
    return "RAND-36";
}


QString Rand36::longname() const
{
    return tr("RAND 36-Item Short Form Health Survey 1.0");
}


QString Rand36::description() const
{
    return tr("Patient-reported survey of general health.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Rand36::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Rand36::summary() const
{
    return QStringList{
        scorePhraseVariant(xstring("score_overall"), overallMean(), MAX_SCORE),
    };
}


QStringList Rand36::detail() const
{
    return completenessInfo() + summary() + QStringList{
        scorePhraseVariant(xstring("score_physical_functioning"),
                           subscaleMean(PHYSICAL_FUNCTIONING_Q), MAX_SCORE),
        scorePhraseVariant(xstring("score_role_limitations_physical"),
                           subscaleMean(ROLE_LIMITATIONS_PHYSICAL_Q), MAX_SCORE),
        scorePhraseVariant(xstring("score_role_limitations_emotional"),
                           subscaleMean(ROLE_LIMITATIONS_EMOTIONAL_Q), MAX_SCORE),
        scorePhraseVariant(xstring("score_energy"),
                           subscaleMean(ENERGY_Q), MAX_SCORE),
        scorePhraseVariant(xstring("score_emotional_wellbeing"),
                           subscaleMean(EMOTIONAL_WELLBEING_Q), MAX_SCORE),
        scorePhraseVariant(xstring("score_social_functioning"),
                           subscaleMean(SOCIAL_FUNCTIONING_Q), MAX_SCORE),
        scorePhraseVariant(xstring("score_pain"),
                           subscaleMean(PAIN_Q), MAX_SCORE),
        scorePhraseVariant(xstring("score_general_health"),
                           subscaleMean(GENERAL_HEALTH_Q), MAX_SCORE),
    };
}


OpenableWidget* Rand36::editor(const bool read_only)
{
    const NameValueOptions q1options{
        {xstring("q1_option1"), 1},
        {xstring("q1_option2"), 2},
        {xstring("q1_option3"), 3},
        {xstring("q1_option4"), 4},
        {xstring("q1_option5"), 5},
    };
    const NameValueOptions q2options{
        {xstring("q2_option1"), 1},
        {xstring("q2_option2"), 2},
        {xstring("q2_option3"), 3},
        {xstring("q2_option4"), 4},
        {xstring("q2_option5"), 5},
    };
    const NameValueOptions activities_options{
        {xstring("activities_option1"), 1},
        {xstring("activities_option2"), 2},
        {xstring("activities_option3"), 3},
    };
    const NameValueOptions yes_no_options{
        {xstring("yesno_option1"), 1},
        {xstring("yesno_option2"), 2},
    };
    const NameValueOptions q20options{
        {xstring("q20_option1"), 1},
        {xstring("q20_option2"), 2},
        {xstring("q20_option3"), 3},
        {xstring("q20_option4"), 4},
        {xstring("q20_option5"), 5},
    };
    const NameValueOptions q21options{
        {xstring("q21_option1"), 1},
        {xstring("q21_option2"), 2},
        {xstring("q21_option3"), 3},
        {xstring("q21_option4"), 4},
        {xstring("q21_option5"), 5},
        {xstring("q21_option6"), 6},
    };
    const NameValueOptions q22options{
        {xstring("q22_option1"), 1},
        {xstring("q22_option2"), 2},
        {xstring("q22_option3"), 3},
        {xstring("q22_option4"), 4},
        {xstring("q22_option5"), 5},
    };
    const NameValueOptions last4weeks_options{
        {xstring("last4weeks_option1"), 1},
        {xstring("last4weeks_option2"), 2},
        {xstring("last4weeks_option3"), 3},
        {xstring("last4weeks_option4"), 4},
        {xstring("last4weeks_option5"), 5},
        {xstring("last4weeks_option6"), 6},
    };
    const NameValueOptions q32options{
        {xstring("q32_option1"), 1},
        {xstring("q32_option2"), 2},
        {xstring("q32_option3"), 3},
        {xstring("q32_option4"), 4},
        {xstring("q32_option5"), 5},
    };
    const NameValueOptions q33to36_options{
        {xstring("q33to36_option1"), 1},
        {xstring("q33to36_option2"), 2},
        {xstring("q33to36_option3"), 3},
        {xstring("q33to36_option4"), 4},
        {xstring("q33to36_option5"), 5},
    };
    QVector<QuPagePtr> pages;

    auto title = [this](int pagenum) -> QString {
        return xstring("title") + " " + TextConst::page() + " " +
                QString::number(pagenum);
    };
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto q = [&boldtext](int question) -> QuElement* {
        return boldtext(strnum("q", question));
    };
    auto mcq = [this](int question,
                      const NameValueOptions& options,
                      bool mandatory = true) -> QuElement* {
        return new QuMcq(fieldRef(strnum(QPREFIX, question), mandatory),
                         options);
    };
    auto mcqgrid = [this](int firstq, int lastq,
                          const NameValueOptions& options,
                          bool mandatory = true) -> QuElement* {
        QVector<QuestionWithOneField> qfields;
        for (int q = firstq; q <= lastq; ++q) {
            qfields.append(QuestionWithOneField(
                               xstring(strnum("q", q)),
                               fieldRef(strnum(QPREFIX, q), mandatory)));
        }
        return new QuMcqGrid(qfields, options);
    };

    int pagenum = 1;

    pages.append(QuPagePtr((new QuPage{
        q(1),
        mcq(1, q1options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        q(2),
        mcq(2, q2options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        boldtext("activities_q"),
        mcqgrid(3, 12, activities_options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        boldtext("work_activities_physical_q"),
        mcqgrid(13, 16, yes_no_options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        boldtext("work_activities_emotional_q"),
        mcqgrid(17, 19, yes_no_options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        q(20),
        mcq(20, q20options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        q(21),
        mcq(21, q21options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        q(22),
        mcq(22, q22options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        text("last4weeks_q_a"),
        boldtext("last4weeks_q_b"),
        mcqgrid(23, 31, last4weeks_options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        q(32),
        mcq(32, q32options),
    })->setTitle(title(pagenum++))));

    pages.append(QuPagePtr((new QuPage{
        boldtext("q33to36stem"),
        mcqgrid(33, 36, q33to36_options),
    })->setTitle(title(pagenum++))));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QVariant Rand36::recoded(const int question) const
{
    const QVariant v = value(strnum(QPREFIX, question));
    if (v.isNull()) {
        return v;
    }
    const int x = v.toInt();
    if (x < 1) {
        return QVariant();
    }

    if (CODE_5STEP_DOWN.contains(question)) {
        // 1 becomes 100, 2 => 75, 3 => 50, 4 => 25, 5 => 0
        if (x > 5) {
            return QVariant();
        }
        return 100 - 25 * (x - 1);
    }
    if (CODE_3STEP_UP.contains(question)) {
        // 1 => 0, 2 => 50, 3 => 100
        if (x > 3) {
            return QVariant();
        }
        return 50 * (x - 1);
    }
    if (CODE_2STEP_UP.contains(question)) {
        // 1 => 0, 2 => 100
        if (x > 2) {
            return QVariant();
        }
        return 100 * (x - 1);
    }
    if (CODE_6STEP_DOWN.contains(question)) {
        // 1 => 100, 2 => 80, 3 => 60, 4 => 40, 5 => 20, 6 => 0
        if (x > 6) {
            return QVariant();
        }
        return 100 - 20 * (x - 1);
    }
    if (CODE_6STEP_UP.contains(question)) {
        // 1 => 0, 2 => 20, 3 => 40, 4 => 60, 5 => 80, 6 => 100
        if (x > 6) {
            return QVariant();
        }
        return 20 * (x - 1);
    }
    if (CODE_5STEP_UP.contains(question)) {
        // 1 => 0, 2 => 25, 3 => 50, 4 => 75, 5 => 100
        if (x > 5) {
            return QVariant();
        }
        return 25 * (x - 1);
    }
    qWarning() << Q_FUNC_INFO << "Invalid question" << question;
    return QVariant();
}


QVariant Rand36::subscaleMean(const QVector<int>& questions) const
{
    QVector<QVariant> values;
    for (int q : questions) {
        values.append(recoded(q));
    }
    return meanOrNull(values, true);
}


QVariant Rand36::overallMean() const
{
    return subscaleMean(seq(FIRST_Q, N_QUESTIONS));
}
