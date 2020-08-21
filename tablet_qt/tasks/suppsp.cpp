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

#include "suppsp.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/quspacer.h"
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
const int MAX_SCORE_PER_Q = 4;
const int MIN_SCORE = MIN_SCORE_PER_Q * N_QUESTIONS;
const int MAX_SCORE = MAX_SCORE_PER_Q * N_QUESTIONS;
const int N_Q_PER_SUBSCALE = 4;  // always
const int MIN_SUBSCALE = MIN_SCORE_PER_Q * N_Q_PER_SUBSCALE;
const int MAX_SUBSCALE = MAX_SCORE_PER_Q * N_Q_PER_SUBSCALE;
const QString QPREFIX("q");
const QVector<int> NEGATIVE_URGENCY_QUESTIONS{6, 8, 13, 15};
const QVector<int> LACK_OF_PERSEVERANCE_QUESTIONS{1, 4, 7, 11};
const QVector<int> LACK_OF_PREMEDITATION_QUESTIONS{2, 5, 12, 19};
const QVector<int> SENSATION_SEEKING_QUESTIONS{9, 14, 16, 18};
const QVector<int> POSITIVE_URGENCY_QUESTIONS{3, 10, 17, 20};


const QString Suppsp::SUPPSP_TABLENAME("suppsp");

void initializeSuppsp(TaskFactory& factory)
{
    static TaskRegistrar<Suppsp> registered(factory);
}


Suppsp::Suppsp(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, SUPPSP_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Suppsp::shortname() const
{
    return "SUPPS-P";
}


QString Suppsp::longname() const
{
    return tr("Short UPPS-P Impulsive Behaviour Scale");
}


QString Suppsp::description() const
{
    return tr("A short English version of the UPPS-P Impulsive Behaviour Scale");
}


QStringList Suppsp::fieldNames() const
{
    return strseq(QPREFIX, FIRST_Q, N_QUESTIONS);
}

// ============================================================================
// Instance info
// ============================================================================


bool Suppsp::isComplete() const
{
    if (anyNull(values(fieldNames()))) {
        return false;
    }

    return true;
}


int Suppsp::totalScore() const
{
    return sumInt(values(fieldNames()));
}


int Suppsp::negativeUrgency() const
{
    return sumInt(values(strnumlist(QPREFIX, NEGATIVE_URGENCY_QUESTIONS)));
}


int Suppsp::lackOfPerseverance() const
{
    return sumInt(values(strnumlist(QPREFIX, LACK_OF_PERSEVERANCE_QUESTIONS)));
}


int Suppsp::lackOfPremeditation() const
{
    return sumInt(values(strnumlist(QPREFIX, LACK_OF_PREMEDITATION_QUESTIONS)));
}


int Suppsp::sensationSeeking() const
{
    return sumInt(values(strnumlist(QPREFIX, SENSATION_SEEKING_QUESTIONS)));
}


int Suppsp::positiveUrgency() const
{
    return sumInt(values(strnumlist(QPREFIX, POSITIVE_URGENCY_QUESTIONS)));
}


QStringList Suppsp::summary() const
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
        rangeScore(xstring("negative_urgency"), negativeUrgency(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("lack_of_perseverance"), lackOfPerseverance(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("lack_of_premeditation"), lackOfPremeditation(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("sensation_seeking"), sensationSeeking(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
        rangeScore(xstring("positive_urgency"), positiveUrgency(),
                   MIN_SUBSCALE, MAX_SUBSCALE),
    };
}


QStringList Suppsp::detail() const
{
    QStringList lines = completenessInfo();
    const QString spacer = " ";
    const QString suffix = "";
    lines += fieldSummaries("q", suffix, spacer, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();

    return lines;
}


OpenableWidget* Suppsp::editor(const bool read_only)
{
    const NameValueOptions agreement_options{
        {xstring("a0"), 1},
        {xstring("a1"), 2},
        {xstring("a2"), 3},
        {xstring("a3"), 4},
    };

    const NameValueOptions reverse_agreement_options{
        {xstring("a0"), 4},
        {xstring("a1"), 3},
        {xstring("a2"), 2},
        {xstring("a3"), 1},
    };

    // Items 3, 6, 8, 9, 10, 13, 14, 15, 16, 17, 18, 20 are reverse coded.
    const QVector<int> reverse_q_nums{3, 6, 8, 9, 10, 13, 14, 15, 16, 17, 18, 20};

    QVector<QuestionWithOneField> q_field_pairs;

    for (const QString& fieldname : fieldNames()) {
        const QString& description = xstring(fieldname);
        q_field_pairs.append(QuestionWithOneField(description,
                                                  fieldRef(fieldname)));
    }
    auto grid = new QuMcqGrid(q_field_pairs, agreement_options);

    QVector<int> reversed_indexes;
    for (const int qnum : reverse_q_nums) {
        reversed_indexes.append(qnum - 1);  // zero-based indexes
    }
    grid->setAlternateNameValueOptions(reversed_indexes, reverse_agreement_options);

    const int question_width = 4;
    const QVector<int> option_widths = {1, 1, 1, 1};
    grid->setWidth(question_width, option_widths);

    // Repeat options every five lines
    QVector<McqGridSubtitle> subtitles{
        {5, ""},
        {10, ""},
        {15, ""},
    };
    grid->setSubtitles(subtitles);

    QuPagePtr page((new QuPage{
                        grid
                    })->setTitle(xstring("title_main")));

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    return m_questionnaire;
}
