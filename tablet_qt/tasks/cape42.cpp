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

#include "cape42.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using stringfunc::bold;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 42;

const QString Cape42::CAPE42_TABLENAME("cape42");

const QString FN_FREQ_PREFIX("frequency");
const QString FN_DISTRESS_PREFIX("distress");

const QString TAG_DISTRESS("distress");
const QVector<int> POSITIVE{2, 5, 6, 7,
                            10, 11, 13, 15, 17,
                            20, 22, 24, 26, 28,
                            30, 31, 33, 34,
                            41, 42};
const QVector<int> DEPRESSIVE{1, 9,
                              12, 14, 19,
                              38, 39,
                              40};
const QVector<int> NEGATIVE{3, 4, 8,
                            16, 18,
                            21, 23, 25, 27, 29,
                            32, 35, 36, 37};
const QVector<int> ALL{      1,  2,  3,  4,  5,  6,  7,  8,  9,
                        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                        30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                        40, 41, 42};  // not the most elegant ;)
const int MIN_SCORE_PER_Q = 1;
const int MAX_SCORE_PER_Q = 4;


void initializeCape42(TaskFactory& factory)
{
    static TaskRegistrar<Cape42> registered(factory);
}


Cape42::Cape42(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CAPE42_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(FN_FREQ_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addFields(strseq(FN_DISTRESS_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Cape42::shortname() const
{
    return "CAPE-42";
}


QString Cape42::longname() const
{
    return tr("Community Assessment of Psychic Experiences");
}


QString Cape42::description() const
{
    return tr("42-item self-rated scale for psychosis with positive, "
              "negative, and depressive dimensions.");
}


QString Cape42::infoFilenameStem() const
{
    return "cape";
}


// ============================================================================
// Instance info
// ============================================================================

bool Cape42::isComplete() const
{
    for (auto q : ALL) {
        if (!questionComplete(q)) {
            return false;
        }
    }
    return true;
}


QStringList Cape42::summary() const
{
    QStringList lines;
    auto addbit = [this, &lines](const QVector<int>& questions,
                                 const QString& name) -> void {
        const int n = questions.length();
        const int min_score = MIN_SCORE_PER_Q * n;
        const int max_score = MAX_SCORE_PER_Q * n;
        lines.append(
            QString("%1: frequency %2 (%3–%4), distress %5 (%6–%7).")
                    .arg(name,
                         bold(QString::number(frequencyScore(questions))),
                         QString::number(min_score),
                         QString::number(max_score),
                         bold(QString::number(distressScore(questions))),
                         QString::number(min_score),
                         QString::number(max_score)));
    };
    addbit(ALL, "ALL");
    addbit(POSITIVE, "POSITIVE");
    addbit(NEGATIVE, "NEGATIVE");
    addbit(DEPRESSIVE, "DEPRESSIVE");
    return lines;
}


QStringList Cape42::detail() const
{
    QStringList lines = completenessInfo();
    for (auto q : ALL) {
        const QVariant freq = value(strnum(FN_FREQ_PREFIX, q));
        QString msg = QString("%1 F:%2")
                .arg(xstring(strnum("q", q)),
                     bold(convert::prettyValue(freq)));
        if (freq.toInt() > MIN_SCORE_PER_Q) {
            msg += QString(" (D:%1)")
                    .arg(bold(prettyValue(strnum(FN_DISTRESS_PREFIX, q))));
        }
        lines.append(msg);
    };
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Cape42::editor(const bool read_only)
{
    const NameValueOptions options_distress{
        {xstring("distress_option1"), 1},
        {xstring("distress_option2"), 2},
        {xstring("distress_option3"), 3},
        {xstring("distress_option4"), 4},
    };
    const NameValueOptions options_frequency{
        {xstring("frequency_option1"), 1},
        {xstring("frequency_option2"), 2},
        {xstring("frequency_option3"), 3},
        {xstring("frequency_option4"), 4},
    };
    QVector<QuPagePtr> pages;
    const QString distress_stem = xstring("distress_stem");
    m_distress_fieldrefs.clear();

    auto addpage = [this, &pages, &options_distress, &options_frequency,
                    &distress_stem](const int q) -> void {
        const QString pagetag = QString::number(q);
        const QString pagetitle = QString("CAPE-42 (%1 / %2)").arg(q).arg(N_QUESTIONS);
        const QString question = xstring(strnum("q", q));
        const bool need_distress = needDistress(q);
        const QString freq_fieldname = strnum(FN_FREQ_PREFIX, q);
        const QString distress_fieldname = strnum(FN_DISTRESS_PREFIX, q);
        FieldRefPtr fr_freq = fieldRef(freq_fieldname);
        fr_freq->setHint(q);
        FieldRefPtr fr_distress = fieldRef(distress_fieldname, need_distress);
        m_distress_fieldrefs[q] = fr_distress;
        QuPagePtr page((new QuPage{
            (new QuText(question))
                ->setBold(),
            new QuMcq(fr_freq, options_frequency),
            (new QuText(distress_stem))
                ->setBold()
                ->addTag(TAG_DISTRESS)
                ->setVisible(need_distress),
            (new QuMcq(fr_distress, options_distress))
                ->addTag(TAG_DISTRESS)
                ->setVisible(need_distress),
        })
            ->setTitle(pagetitle)
            ->addTag(pagetag));
        pages.append(page);
        connect(fr_freq.data(), &FieldRef::valueChanged,
                this, &Cape42::frequencyChanged);
    };

    for (auto q : ALL) {
        addpage(q);
    }

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);
    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Cape42::distressScore(const QVector<int>& questions) const
{
    int score = 0;
    for (auto q : questions) {
        const int freq = valueInt(strnum(FN_FREQ_PREFIX, q));  // 0 for null
        if (freq > MIN_SCORE_PER_Q) {
            score += valueInt(strnum(FN_DISTRESS_PREFIX, q));  // 0 for null
        } else {
            score += MIN_SCORE_PER_Q;
            // ... if frequency is 1, score 1 for distress?
        }
    }
    return score;
}


int Cape42::frequencyScore(const QVector<int>& questions) const
{
    int score = 0;
    for (auto q : questions) {
        score += qMax(MIN_SCORE_PER_Q,
                      valueInt(strnum(FN_FREQ_PREFIX, q)));  // will be 0 if null
    }
    return score;
}


bool Cape42::questionComplete(const int q) const
{
    const QVariant freq = value(strnum(FN_FREQ_PREFIX, q));
    if (freq.isNull()) {
        return false;
    }
    if (freq.toInt() <= MIN_SCORE_PER_Q) {
        return true;
    }
    QVariant distress = value(strnum(FN_DISTRESS_PREFIX, q));
    return !distress.isNull();
}


// ============================================================================
// Signal handlers
// ============================================================================

void Cape42::frequencyChanged(const FieldRef* fieldref)
{
    Q_ASSERT(fieldref);
    const QVariant hint = fieldref->getHint();
    const int q = hint.toInt();
    Q_ASSERT(q >= FIRST_Q && q <= N_QUESTIONS);
    setDistressItems(q);
}


bool Cape42::needDistress(const int q)
{
    Q_ASSERT(q >= FIRST_Q && q <= N_QUESTIONS);
    return valueInt(strnum(FN_FREQ_PREFIX, q)) > MIN_SCORE_PER_Q;
    // ... we need a distress rating if the frequency rating is above minimum
}


void Cape42::setDistressItems(const int q)
{
    if (!m_questionnaire) {
        return;
    }
    const QString pagetag = QString::number(q);
    const bool need_distress = needDistress(q);
    m_questionnaire->setVisibleByTag(TAG_DISTRESS, need_distress, false, pagetag);
    Q_ASSERT(m_distress_fieldrefs.contains(q));
    FieldRefPtr distress_fieldref = m_distress_fieldrefs[q];
    Q_ASSERT(distress_fieldref);
    distress_fieldref->setMandatory(need_distress);
}
