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

#include "mfi20.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::sumInt;
using mathfunc::scorePhrase;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strnumlist;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 20;
const int MIN_SCORE_PER_Q = 1;
const int MAX_SCORE_PER_Q = 5;
const int MIN_SCORE = MIN_SCORE_PER_Q * N_QUESTIONS;
const int MAX_SCORE = MAX_SCORE_PER_Q * N_QUESTIONS;
const int N_Q_PER_SUBSCALE = 4;  // always
const int MIN_SUBSCALE = MIN_SCORE_PER_Q * N_Q_PER_SUBSCALE;
const int MAX_SUBSCALE = MAX_SCORE_PER_Q * N_Q_PER_SUBSCALE;
const QString QPREFIX("q");

const QStringList REVERSE_QUESTIONS = strnumlist(
    QPREFIX, {2, 5, 9, 10, 13, 14, 16, 17, 18, 19});

const QVector<int> GENERAL_FATIGUE_QUESTIONS{1, 5, 12, 16};
const QVector<int> PHYSICAL_FATIGUE_QUESTIONS{2, 8, 14, 20};
const QVector<int> REDUCED_ACTIVITY_QUESTIONS{7, 11, 13, 19};
const QVector<int> REDUCED_MOTIVATION_QUESTIONS{3, 6, 10, 17};
const QVector<int> MENTAL_FATIGUE_QUESTIONS{4, 9, 15, 18};


const QString Mfi20::MFI20_TABLENAME("mfi20");

void initializeMfi20(TaskFactory& factory)
{
    static TaskRegistrar<Mfi20> registered(factory);
}


Mfi20::Mfi20(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, MFI20_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Mfi20::shortname() const
{
    return "MFI-20";
}


QString Mfi20::longname() const
{
    return tr("Multidimensional Fatigue Inventory");
}


QString Mfi20::description() const
{
    return tr("A 20-item self-report instrument designed to measure fatigue.");
}


QStringList Mfi20::fieldNames() const
{
    return strseq(QPREFIX, FIRST_Q, N_QUESTIONS);
}

// ============================================================================
// Instance info
// ============================================================================


bool Mfi20::isComplete() const
{
    if (anyNull(values(fieldNames()))) {
        return false;
    }

    return true;
}


QVector<QVariant> Mfi20::normalizeValues(const QStringList& fieldnames) const
{
    QVector<QVariant> values;
    for (const QString& fieldname : fieldnames) {
        QVariant normalized_value = value(fieldname);
        if (!normalized_value.isNull()) {
            if (REVERSE_QUESTIONS.contains(fieldname)) {
                normalized_value = MAX_SCORE_PER_Q + 1 - normalized_value.toInt();
            }
        }

        values.append(normalized_value);
    }

    return values;
}


int Mfi20::totalScore() const
{
    return sumInt(normalizeValues(fieldNames()));
}


int Mfi20::generalFatigue() const
{
    return sumInt(normalizeValues(strnumlist(QPREFIX, GENERAL_FATIGUE_QUESTIONS)));
}


int Mfi20::physicalFatigue() const
{
    return sumInt(normalizeValues(strnumlist(QPREFIX, PHYSICAL_FATIGUE_QUESTIONS)));
}


int Mfi20::reducedActivity() const
{
    return sumInt(normalizeValues(strnumlist(QPREFIX, REDUCED_ACTIVITY_QUESTIONS)));
}


int Mfi20::reducedMotivation() const
{
    return sumInt(normalizeValues(strnumlist(QPREFIX, REDUCED_MOTIVATION_QUESTIONS)));
}


int Mfi20::mentalFatigue() const
{
    return sumInt(normalizeValues(strnumlist(QPREFIX, MENTAL_FATIGUE_QUESTIONS)));
}


QStringList Mfi20::summary() const
{
    auto rangeScore = [](const QString& description, const int score,
                         const int min, const int max) {
        return QString("%1: <b>%2</b> [%3–%4].").arg(
                    description,
                    QString::number(score),
                    QString::number(min),
                    QString::number(max));
    };
    return QStringList{
        rangeScore(TextConst::totalScore(), totalScore(),
                   MIN_SCORE, MAX_SCORE),
        rangeScore(xstring("general_fatigue"), generalFatigue(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("physical_fatigue"), physicalFatigue(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("reduced_activity"), reducedActivity(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("reduced_motivation"), reducedMotivation(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("mental_fatigue"), mentalFatigue(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
    };
}


QStringList Mfi20::detail() const
{
    QStringList lines = completenessInfo();
    const QString spacer = " ";
    const QString suffix = "";
    lines += fieldSummaries("q", suffix, spacer, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();

    return lines;
}


OpenableWidget* Mfi20::editor(const bool read_only)
{
    const NameValueOptions agreement_options{
        {xstring("a0"), 1},
        {xstring("a1"), 2},
        {xstring("a2"), 3},
        {xstring("a3"), 4},
        {xstring("a4"), 5},
    };

    QVector<QuestionWithOneField> q_field_pairs;

    for (const QString& fieldname : fieldNames()) {
        const QString& description = xstring(fieldname);
        q_field_pairs.append(QuestionWithOneField(description,
                                                  fieldRef(fieldname)));
    }
    auto grid = new QuMcqGrid(q_field_pairs, agreement_options);

    const int question_width = 4;
    const QVector<int> option_widths = {1, 1, 1, 1, 1};
    grid->setWidth(question_width, option_widths);

    // Repeat options every five lines
    QVector<McqGridSubtitle> subtitles{
        {5, ""},
        {10, ""},
        {15, ""},
    };
    grid->setSubtitles(subtitles);

    QuPagePtr page((new QuPage{
                        new QuText(xstring("instructions")),
                        grid
                    })->setTitle(xstring("title_main")));

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    return m_questionnaire;
}
