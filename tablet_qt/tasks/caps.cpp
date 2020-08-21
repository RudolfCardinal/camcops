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

#include "caps.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::countTrue;
using mathfunc::scorePhrase;
using mathfunc::totalScorePhrase;
using stringfunc::bold;
using stringfunc::strnum;
using stringfunc::strseq;
using uifunc::yesNoNull;

const int FIRST_Q = 1;
const int N_QUESTIONS = 32;
const int MAX_TOTAL_SCORE = 32;
const int MAX_SUBSCALE_SCORE = 160;  // distress, intrusiveness, frequency

const QString Caps::CAPS_TABLENAME("caps");
const QString FN_ENDORSE_PREFIX("endorse");
const QString FN_DISTRESS_PREFIX("distress");
const QString FN_INTRUSIVE_PREFIX("intrusiveness");
const QString FN_FREQ_PREFIX("frequency");

const QString TAG_DETAIL("detail");


void initializeCaps(TaskFactory& factory)
{
    static TaskRegistrar<Caps> registered(factory);
}


Caps::Caps(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CAPS_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(FN_ENDORSE_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addFields(strseq(FN_DISTRESS_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addFields(strseq(FN_INTRUSIVE_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addFields(strseq(FN_FREQ_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Caps::shortname() const
{
    return "CAPS";
}


QString Caps::longname() const
{
    return tr("Cardiff Anomalous Perceptions Scale");
}


QString Caps::description() const
{
    return tr("32-item self-rated scale for perceptual anomalies.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Caps::isComplete() const
{
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        if (!questionComplete(q)) {
            return false;
        }
    }
    return true;
}


QStringList Caps::summary() const
{
    const QString sep(": ");
    const QString suffix(".");
    return QStringList{
        totalScorePhrase(totalScore(), MAX_TOTAL_SCORE, sep, suffix),
        scorePhrase("Distress", distressScore(), MAX_SUBSCALE_SCORE, sep, suffix),
        scorePhrase("Intrusiveness", intrusivenessScore(), MAX_SUBSCALE_SCORE, sep, suffix),
        scorePhrase("Frequency", frequencyScore(), MAX_SUBSCALE_SCORE, sep, suffix),
    };
}


QStringList Caps::detail() const
{
    QStringList lines = completenessInfo();
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        const QVariant e = endorse(q);
        QString msg = QString("%1 %2")
                .arg(xstring(strnum("q", q)),
                     bold(yesNoNull(e)));
        if (e.toInt() > 0) {
            msg += QString(" (D %1, I %2, F %3)")
                .arg(bold(convert::prettyValue(distress(q))),
                     bold(convert::prettyValue(intrusiveness(q))),
                     bold(convert::prettyValue(frequency(q))));
        }
        lines.append(msg);
    }
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Caps::editor(const bool read_only)
{
    const NameValueOptions options_endorse = CommonOptions::noYesInteger();
    const NameValueOptions options_distress{
        {xstring("distress_option1"), 1},
        {"2", 2},
        {"3", 3},
        {"4", 4},
        {xstring("distress_option5"), 5},
    };
    const NameValueOptions options_intrusiveness{
        {xstring("intrusiveness_option1"), 1},
        {"2", 2},
        {"3", 3},
        {"4", 4},
        {xstring("intrusiveness_option5"), 5},
    };
    const NameValueOptions options_frequency{
        {xstring("frequency_option1"), 1},
        {"2", 2},
        {"3", 3},
        {"4", 4},
        {xstring("frequency_option5"), 5},
    };
    const QString detail_prompt = xstring("if_yes_please_rate");
    QVector<QuPagePtr> pages;

    m_fr_distress.clear();
    m_fr_intrusiveness.clear();
    m_fr_frequency.clear();

    auto addpage = [this, &pages, &options_endorse, &options_distress,
                    &options_intrusiveness, &options_frequency,
                    &detail_prompt](int q) -> void {
        const bool need_detail = needsDetail(q);
        const QString pagetitle = QString("CAPS (%1 / %2)").arg(q).arg(N_QUESTIONS);
        const QString question = xstring(strnum("q", q));
        const QString pagetag = QString::number(q);
        const QString endorse_fieldname = strnum(FN_ENDORSE_PREFIX, q);
        const QString distress_fieldname = strnum(FN_DISTRESS_PREFIX, q);
        const QString intrusiveness_fieldname = strnum(FN_INTRUSIVE_PREFIX, q);
        const QString freq_fieldname = strnum(FN_FREQ_PREFIX, q);
        FieldRefPtr fr_endorse = fieldRef(endorse_fieldname);
        fr_endorse->setHint(q);
        FieldRefPtr fr_distress = fieldRef(distress_fieldname, need_detail);
        m_fr_distress[q] = fr_distress;
        FieldRefPtr fr_intrusive = fieldRef(intrusiveness_fieldname, need_detail);
        m_fr_intrusiveness[q] = fr_intrusive;
        FieldRefPtr fr_freq = fieldRef(freq_fieldname, need_detail);
        m_fr_frequency[q] = fr_freq;
        QuPagePtr page((new QuPage{
            (new QuText(question))
                ->setBold(),
            new QuMcq(fr_endorse, options_endorse),
            (new QuText(detail_prompt))
                ->setBold()
                ->addTag(TAG_DETAIL)
                ->setVisible(need_detail),
            (new QuMcq(fr_distress, options_distress))
                ->addTag(TAG_DETAIL)
                ->setVisible(need_detail),
            (new QuHorizontalLine())
                ->addTag(TAG_DETAIL)
                ->setVisible(need_detail),
            (new QuMcq(fr_intrusive, options_intrusiveness))
                ->addTag(TAG_DETAIL)
                ->setVisible(need_detail),
            (new QuHorizontalLine())
                ->addTag(TAG_DETAIL)
                ->setVisible(need_detail),
            (new QuMcq(fr_freq, options_frequency))
                ->addTag(TAG_DETAIL)
                ->setVisible(need_detail),
        })
            ->setTitle(pagetitle)
            ->addTag(pagetag));
        pages.append(page);
        connect(fr_endorse.data(), &FieldRef::valueChanged,
                this, &Caps::endorseChanged);
    };

    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
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

int Caps::totalScore() const
{
    return countTrue(values(strseq(FN_ENDORSE_PREFIX, FIRST_Q, N_QUESTIONS)));
}


int Caps::distressScore() const
{
    int score = 0;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        if (endorse(q).toInt()) {
            score += distress(q).toInt();  // 0 for null
        }
    }
    return score;
}


int Caps::intrusivenessScore() const
{
    int score = 0;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        if (endorse(q).toInt()) {
            score += intrusiveness(q).toInt();  // 0 for null
        }
    }
    return score;
}


int Caps::frequencyScore() const
{
    int score = 0;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        if (endorse(q).toInt()) {
            score += frequency(q).toInt();  // 0 for null
        }
    }
    return score;
}


bool Caps::questionComplete(const int q) const
{
    const QVariant e = endorse(q);
    if (e.isNull()) {
        return false;
    }
    if (e.toInt() == 0) {
        return true;
    }
    return !distress(q).isNull() && !intrusiveness(q).isNull() &&
            !frequency(q).isNull();
}


QVariant Caps::endorse(const int q) const
{
    return value(strnum(FN_ENDORSE_PREFIX, q));
}


QVariant Caps::distress(const int q) const
{
    return value(strnum(FN_DISTRESS_PREFIX, q));
}


QVariant Caps::intrusiveness(const int q) const
{
    return value(strnum(FN_INTRUSIVE_PREFIX, q));
}


QVariant Caps::frequency(const int q) const
{
    return value(strnum(FN_FREQ_PREFIX, q));
}


// ============================================================================
// Signal handlers
// ============================================================================

void Caps::endorseChanged(const FieldRef* fieldref)
{
    Q_ASSERT(fieldref);
    if (!m_questionnaire) {
        return;
    }
    const QVariant hint = fieldref->getHint();
    const int q = hint.toInt();
    Q_ASSERT(q >= FIRST_Q && q <= N_QUESTIONS);

    const QString pagetag = QString::number(q);
    const bool need_detail = needsDetail(q);

    m_questionnaire->setVisibleByTag(TAG_DETAIL, need_detail, false, pagetag);
    Q_ASSERT(m_fr_distress.contains(q));
    Q_ASSERT(m_fr_intrusiveness.contains(q));
    Q_ASSERT(m_fr_frequency.contains(q));
    FieldRefPtr distress_fieldref = m_fr_distress[q];
    FieldRefPtr intrusive_fieldref = m_fr_intrusiveness[q];
    FieldRefPtr freq_fieldref = m_fr_frequency[q];
    Q_ASSERT(distress_fieldref);
    Q_ASSERT(intrusive_fieldref);
    Q_ASSERT(freq_fieldref);
    distress_fieldref->setMandatory(need_detail);
    intrusive_fieldref->setMandatory(need_detail);
    freq_fieldref->setMandatory(need_detail);
}


bool Caps::needsDetail(const int q)
{
    Q_ASSERT(q >= FIRST_Q && q <= N_QUESTIONS);
    return endorse(q).toBool();
}
