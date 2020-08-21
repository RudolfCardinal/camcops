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

#include "ifs.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/quimage.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::falseNotNull;
using mathfunc::scorePhrase;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;

const QString Ifs::IFS_TABLENAME("ifs");

const QString Q1("q1");
const QString Q2("q2");
const QString Q3("q3");
const QString Q4_LEN2_1("q4_len2_1");
const QString Q4_LEN2_2("q4_len2_2");
const QString Q4_LEN3_1("q4_len3_1");
const QString Q4_LEN3_2("q4_len3_2");
const QString Q4_LEN4_1("q4_len4_1");
const QString Q4_LEN4_2("q4_len4_2");
const QString Q4_LEN5_1("q4_len5_1");
const QString Q4_LEN5_2("q4_len5_2");
const QString Q4_LEN6_1("q4_len6_1");
const QString Q4_LEN6_2("q4_len6_2");
const QString Q4_LEN7_1("q4_len7_1");
const QString Q4_LEN7_2("q4_len7_2");
const QString Q5("q5");
const QString Q6_SEQ1("q6_seq1");
const QString Q6_SEQ2("q6_seq2");
const QString Q6_SEQ3("q6_seq3");
const QString Q6_SEQ4("q6_seq4");
const QString Q7_PROVERB1("q7_proverb1");
const QString Q7_PROVERB2("q7_proverb2");
const QString Q7_PROVERB3("q7_proverb3");
const QString Q8_SENTENCE1("q8_sentence1");
const QString Q8_SENTENCE2("q8_sentence2");
const QString Q8_SENTENCE3("q8_sentence3");

const QStringList SIMPLE_QUESTIONS{
    Q1, Q2, Q3, Q5,
    Q6_SEQ1, Q6_SEQ2, Q6_SEQ3, Q6_SEQ4,
    Q7_PROVERB1, Q7_PROVERB2, Q7_PROVERB3,
    Q8_SENTENCE1, Q8_SENTENCE2, Q8_SENTENCE3,
};
const int MAX_TOTAL = 30;
const int MAX_WM = 10;

const QString IMAGE_SWM("ifw/swm.png");

const QString Q4_TAG_PREFIX("q4_seqlen");


void initializeIfs(TaskFactory& factory)
{
    static TaskRegistrar<Ifs> registered(factory);
}


Ifs::Ifs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, IFS_TABLENAME, false, true, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(Q1, QVariant::Int);
    addField(Q2, QVariant::Int);
    addField(Q3, QVariant::Int);
    addField(Q4_LEN2_1, QVariant::Bool);
    addField(Q4_LEN2_2, QVariant::Bool);
    addField(Q4_LEN3_1, QVariant::Bool);
    addField(Q4_LEN3_2, QVariant::Bool);
    addField(Q4_LEN4_1, QVariant::Bool);
    addField(Q4_LEN4_2, QVariant::Bool);
    addField(Q4_LEN5_1, QVariant::Bool);
    addField(Q4_LEN5_2, QVariant::Bool);
    addField(Q4_LEN6_1, QVariant::Bool);
    addField(Q4_LEN6_2, QVariant::Bool);
    addField(Q4_LEN7_1, QVariant::Bool);
    addField(Q4_LEN7_2, QVariant::Bool);
    addField(Q5, QVariant::Int);
    addField(Q6_SEQ1, QVariant::Int);
    addField(Q6_SEQ2, QVariant::Int);
    addField(Q6_SEQ3, QVariant::Int);
    addField(Q6_SEQ4, QVariant::Int);
    addField(Q7_PROVERB1, QVariant::Double);  // can score 0.5
    addField(Q7_PROVERB2, QVariant::Double);
    addField(Q7_PROVERB3, QVariant::Double);
    addField(Q8_SENTENCE1, QVariant::Int);
    addField(Q8_SENTENCE2, QVariant::Int);
    addField(Q8_SENTENCE3, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Ifs::shortname() const
{
    return "IFS";
}


QString Ifs::longname() const
{
    return tr("INECO Frontal Screening");
}


QString Ifs::description() const
{
    return tr("30-point clinician-administered assessment.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Ifs::isComplete() const
{
    // Obligatory stuff
    if (anyNull(values(SIMPLE_QUESTIONS))) {
        return false;
    }
    // Q4 (digit span), where we can terminate early
    // The sequences come in pairs. The task terminates when the patient
    // gets both items wrong within the pair (or we run out).
    for (int seqlen = 2; seqlen <= 7; ++seqlen) {
        const QVariant v1 = q4FirstVal(seqlen);
        const QVariant v2 = q4SecondVal(seqlen);
        if (v1.isNull() || v2.isNull()) {
            return false;
        }
        if (!v1.toBool() && !v2.toBool()) {
            return true;  // all done
        }
    }
    return true;
}


QStringList Ifs::summary() const
{
    const IfsScore score = getScore();
    return QStringList{
        totalScorePhrase(score.total, MAX_TOTAL),
        scorePhrase("Working memory index", score.wm, MAX_WM),
    };
}


QStringList Ifs::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Ifs::editor(const bool read_only)
{
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto booltext = [this](const QString& fieldname,
                          const QString& xstringname,
                          bool mandatory = true) -> QuElement* {
        return new QuBoolean(xstring(xstringname),
                             fieldRef(fieldname, mandatory));
    };
    auto mcqoptions = [this](const QString& answer_prefix,
                             int last) -> NameValueOptions {
        NameValueOptions options;
        // Descending order:
        for (int i = last; i >= 0; --i) {
            options.append(NameValuePair(xstring(strnum(answer_prefix, i)), i));
        }
        return options;
    };
    auto mcq = [this, &mcqoptions](const QString& fieldname,
                                   const QString& answer_prefix,
                                   int last,
                                   bool mandatory = true) -> QuElement* {
        NameValueOptions options = mcqoptions(answer_prefix, last);
        return new QuMcq(fieldRef(fieldname, mandatory), options);
    };

    const NameValueOptions proverb_options{
        {xstring("q7_a_1"), 1},
        {xstring("q7_a_half"), 0.5},
        {xstring("q7_a_0"), 0},
    };
    const NameValueOptions inhibition_options{
        {xstring("q8_a2"), 2},
        {xstring("q8_a1"), 1},
        {xstring("q8_a0"), 0},
    };

    QVector<QuPagePtr> pages{getClinicianDetailsPage()};

    // Q1
    pages.append(QuPagePtr((new QuPage{
        boldtext("q1_instruction_1"),
        text("q1_instruction_2"),
        boldtext("q1_instruction_3"),
        text("q1_instruction_4"),
        boldtext("q1_instruction_5"),
        mcq(Q1, "q1_a", 3),
    })->setTitle(xstring("q1_title"))));

    // Q2
    pages.append(QuPagePtr((new QuPage{
        boldtext("q2_instruction_1"),
        text("q2_instruction_2"),
        boldtext("q2_instruction_3"),
        text("q2_instruction_4"),
        boldtext("q2_instruction_5"),
        mcq(Q2, "q2_a", 3),
    })->setTitle(xstring("q2_title"))));

    // Q3
    pages.append(QuPagePtr((new QuPage{
        boldtext("q3_instruction_1"),
        text("q3_instruction_2"),
        boldtext("q3_instruction_3"),
        text("q3_instruction_4"),
        boldtext("q3_instruction_5"),
        mcq(Q3, "q3_a", 3),
    })->setTitle(xstring("q3_title"))));

    // Q4
    QuPagePtr page4(new QuPage());
    page4->setTitle(xstring("q4_title"));
    page4->addElement(text("q4_instruction_1"));
    for (int seqlen = 2; seqlen <= 7; ++seqlen) {
        const QString tag = strnum(Q4_TAG_PREFIX, seqlen);
        for (int pair : {1, 2}) {
            const QString xname = QString("q4_seq_len%1_%2").arg(seqlen).arg(pair);
            const QString fname = QString("q4_len%1_%2").arg(seqlen).arg(pair);
            QuElement* el = booltext(fname, xname);
            el->addTag(tag);
            page4->addElement(el);
            connect(fieldRef(fname).data(), &FieldRef::valueChanged,
                    this, &Ifs::updateMandatory);
        }
    }
    pages.append(page4);

    // Q5
    pages.append(QuPagePtr((new QuPage{
        boldtext("q5_instruction_1"),
        text("q5_instruction_2"),
        text("q5_instruction_3"),
        mcq(Q5, "q5_a", 2),
    })->setTitle(xstring("q5_title"))));

    // Q6
    pages.append(QuPagePtr((new QuPage{
        boldtext("q6_instruction_1"),
        text("q6_instruction_2"),
        booltext(Q6_SEQ1, "q6_seq1"),
        booltext(Q6_SEQ2, "q6_seq2"),
        booltext(Q6_SEQ3, "q6_seq3"),
        booltext(Q6_SEQ4, "q6_seq4"),
        new QuImage(uifunc::resourceFilename(IMAGE_SWM)),
    })->setTitle(xstring("q6_title"))));

    // Q7
    pages.append(QuPagePtr((new QuPage{
        boldtext("q7_proverb1"),
        new QuMcq(fieldRef(Q7_PROVERB1), proverb_options),
        boldtext("q7_proverb2"),
        new QuMcq(fieldRef(Q7_PROVERB2), proverb_options),
        boldtext("q7_proverb3"),
        new QuMcq(fieldRef(Q7_PROVERB3), proverb_options),
    })->setTitle(xstring("q7_title"))));

    // Q8
    pages.append(QuPagePtr((new QuPage{
        boldtext("q8_instruction_1"),
        boldtext("q8_instruction_2"),
        boldtext("q8_instruction_3"),
        text("q8_instruction_4"),
        boldtext("q8_instruction_5"),
        text("q8_instruction_6"),
        boldtext("q8_instruction_7"),
        boldtext("q8_instruction_8"),
        boldtext("q8_instruction_9"),
        new QuHorizontalLine(),
        boldtext("q8_sentence_1"),
        new QuMcq(fieldRef(Q8_SENTENCE1), inhibition_options),
        boldtext("q8_sentence_2"),
        new QuMcq(fieldRef(Q8_SENTENCE2), inhibition_options),
        boldtext("q8_sentence_3"),
        new QuMcq(fieldRef(Q8_SENTENCE3), inhibition_options),
    })->setTitle(xstring("q8_title"))));

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);

    updateMandatory();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

Ifs::IfsScore Ifs::getScore() const
{
    const int q1 = valueInt(Q1);
    const int q2 = valueInt(Q2);
    const int q3 = valueInt(Q3);
    int q4 = 0;
    for (int seqlen = 2; seqlen <= 7; ++seqlen) {
        QVariant v1 = q4FirstVal(seqlen);
        QVariant v2 = q4SecondVal(seqlen);
        q4 += v1.toBool() || v2.toBool() ? 1 : 0;
        if (!v1.toBool() && !v2.toBool()) {
            break;
        }
    }
    const int q5 = valueInt(Q5);
    const int q6 = valueInt(Q6_SEQ1) +
            valueInt(Q6_SEQ2) +
            valueInt(Q6_SEQ3) +
            valueInt(Q6_SEQ4);
    const double q7 = valueDouble(Q7_PROVERB1) +
            valueDouble(Q7_PROVERB2) +
            valueDouble(Q7_PROVERB3);
    const int q8 = valueInt(Q8_SENTENCE1) +
            valueInt(Q8_SENTENCE2) +
            valueInt(Q8_SENTENCE3);

    IfsScore score;
    score.total = q1 + q2 + q3 + q4 + q5 + q6 + q7 + q8;
    score.wm = q4 + q6;  // working memory index (though not verbal)
    return score;
}


QVariant Ifs::q4FirstVal(const int seqlen) const
{
    return value(QString("q4_len%1_1").arg(seqlen));
}


QVariant Ifs::q4SecondVal(const int seqlen) const
{
    return value(QString("q4_len%1_2").arg(seqlen));
}


// ============================================================================
// Signal handlers
// ============================================================================

void Ifs::updateMandatory()
{
    if (!m_questionnaire) {
        return;
    }
    // Q4
    bool required = true;
    for (int seqlen = 2; seqlen <= 7; ++seqlen) {
        const QString tag = strnum(Q4_TAG_PREFIX, seqlen);
        m_questionnaire->setVisibleByTag(tag, required, false);
        if (required) {
            const QVariant v1 = q4FirstVal(seqlen);
            const QVariant v2 = q4SecondVal(seqlen);
            if (falseNotNull(v1) && falseNotNull(v2)) {
                required = false;  // for subsequent pairs
            }
        }
    }
}
