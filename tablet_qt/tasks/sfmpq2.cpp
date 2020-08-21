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

#include "sfmpq2.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::meanOrNull;
using mathfunc::scorePhraseVariant;
using stringfunc::strnum;
using stringfunc::strnumlist;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 22;
const int MAX_SCORE_PER_Q = 10;
const QString QPREFIX("q");

const QVector<int> CONTINUOUS_PAIN_QUESTIONS{1, 5, 6, 8, 9, 10};
const QVector<int> INTERMITTENT_PAIN_QUESTIONS{2, 3, 4, 11, 16, 18};
const QVector<int> NEUROPATHIC_PAIN_QUESTIONS{7, 17, 19, 20, 21, 22};
const QVector<int> AFFECTIVE_PAIN_QUESTIONS{12, 13, 14, 15};

const bool IGNORE_NULL_FOR_MEAN = true;


const QString Sfmpq2::SFMPQ2_TABLENAME("sfmpq2");

void initializeSfmpq2(TaskFactory& factory)
{
    static TaskRegistrar<Sfmpq2> registered(factory);
}


Sfmpq2::Sfmpq2(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, SFMPQ2_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Sfmpq2::shortname() const
{
    return "SF-MPQ2";
}


QString Sfmpq2::longname() const
{
    return tr("Short-Form McGill Pain Questionnaire 2");
}


QString Sfmpq2::description() const
{
    return tr("22-item self-report measure of pain symptoms of both "
              "neuropathic and non-neuropathic pain conditions.");
}


QStringList Sfmpq2::fieldNames() const
{
    return strseq(QPREFIX, FIRST_Q, N_QUESTIONS);
}


// ============================================================================
// Instance info
// ============================================================================

bool Sfmpq2::isComplete() const
{
    if (anyNull(values(fieldNames()))) {
        return false;
    }

    return true;
}


QVariant Sfmpq2::totalPain() const
{
    return meanOrNull(values(fieldNames()), IGNORE_NULL_FOR_MEAN);
}


QVariant Sfmpq2::continuousPain() const
{
    return meanOrNull(values(strnumlist(QPREFIX, CONTINUOUS_PAIN_QUESTIONS)),
                      IGNORE_NULL_FOR_MEAN);
}


QVariant Sfmpq2::intermittentPain() const
{
    return meanOrNull(values(strnumlist(QPREFIX, INTERMITTENT_PAIN_QUESTIONS)),
                      IGNORE_NULL_FOR_MEAN);
}


QVariant Sfmpq2::neuropathicPain() const
{
    return meanOrNull(values(strnumlist(QPREFIX, NEUROPATHIC_PAIN_QUESTIONS)),
                      IGNORE_NULL_FOR_MEAN);
}


QVariant Sfmpq2::affectivePain() const
{
    return meanOrNull(values(strnumlist(QPREFIX, AFFECTIVE_PAIN_QUESTIONS)),
                      IGNORE_NULL_FOR_MEAN);
}


QStringList Sfmpq2::summary() const
{
    return QStringList{
        scorePhraseVariant(xstring("total_pain"), totalPain(),
                           MAX_SCORE_PER_Q),
        scorePhraseVariant(xstring("continuous_pain"), continuousPain(),
                           MAX_SCORE_PER_Q),
        scorePhraseVariant(xstring("intermittent_pain"), intermittentPain(),
                           MAX_SCORE_PER_Q),
        scorePhraseVariant(xstring("neuropathic_pain"), neuropathicPain(),
                           MAX_SCORE_PER_Q),
        scorePhraseVariant(xstring("affective_pain"), affectivePain(),
                           MAX_SCORE_PER_Q),
    };
}


QStringList Sfmpq2::detail() const
{
    QStringList lines = completenessInfo();
    const QString spacer = " ";
    const QString suffix = "";
    lines += fieldSummaries("q", suffix, spacer, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();

    return lines;
}


OpenableWidget* Sfmpq2::editor(const bool read_only)
{
    NameValueOptions intensity_options;
    QVector<int> option_widths;

    for (int i = 0; i <= MAX_SCORE_PER_Q; i++) {
        QString xstringname = QString("a%1").arg(i);
        auto option = NameValuePair(xstring(xstringname), i);
        intensity_options.append(option);
        option_widths.append(3);
    };

    QVector<QuestionWithOneField> q_field_pairs;

    for (const QString& fieldname : fieldNames()) {
        const QString& description = xstring(fieldname);
        q_field_pairs.append(QuestionWithOneField(description,
                                                  fieldRef(fieldname)));
    }
    auto grid = new QuMcqGrid(q_field_pairs, intensity_options);

    const int question_width = 4;
    grid->setWidth(question_width, option_widths);

    // Repeat options every five lines
    QVector<McqGridSubtitle> subtitles{
        {5, ""},
        {10, ""},
        {15, ""},
        {20, ""},
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
