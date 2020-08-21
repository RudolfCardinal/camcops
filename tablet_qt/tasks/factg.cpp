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

#include "factg.h"

#include "lib/stringfunc.h"
#include "lib/version.h"
#include "maths/mathfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"

const QString Factg::FACTG_TABLENAME("factg");

const QString SUBTITLE_PHYSICAL("Physical Well-Being");
const QString SUBTITLE_SOCIAL("Social/Family Well-Being");
const QString SUBTITLE_EMOTIONAL("Emotional Well-Being");
const QString SUBTITLE_FUNCTIONAL("Functional Well-Being");

const QString PREFIX_PHYSICAL("p_q");
const QString PREFIX_SOCIAL("s_q");
const QString PREFIX_EMOTIONAL("e_q");
const QString PREFIX_FUNCTIONAL("f_q");

const QString IGNORE_SOCIAL_Q7("ignore_s_q7");
const QString OPTIONAL_Q(PREFIX_SOCIAL + "7");

const int FIRST_Q = 1;
const int LAST_Q_PHYSICAL = 7;
const int LAST_Q_SOCIAL = 7;
const int LAST_Q_EMOTIONAL = 6;
const int LAST_Q_FUNCTIONAL = 7;

const int N_PHYSICAL = 7;
const int N_SOCIAL = 7;
const int N_EMOTIONAL = 6;
const int N_FUNCTIONAL= 7;

const int MAX_QSCORE = 4;
const int MAX_SCORE_PHYSICAL = 28;
const int MAX_SCORE_SOCIAL = 28;
const int MAX_SCORE_EMOTIONAL = 24;
const int MAX_SCORE_FUNCTIONAL = 28;

const int MAX_SCORE =    MAX_SCORE_PHYSICAL
                       + MAX_SCORE_SOCIAL
                       + MAX_SCORE_EMOTIONAL
                       + MAX_SCORE_FUNCTIONAL;

const int NON_REVERSE_SCORED_EMOTIONAL_QNUM = 2;

const QString XSTRING_PREFER_NO_ANSWER("prefer_no_answer");

using stringfunc::strnum;
using stringfunc::strseq;
using mathfunc::anyNull;
using mathfunc::countNotNull;
using mathfunc::scorePhrase;
using mathfunc::sumDouble;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;


void initializeFactg(TaskFactory& factory)
{
    static TaskRegistrar<Factg> registered(factory);
}


Factg::Factg(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, FACTG_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_in_tickbox_change(false)
{
    for (auto field : strseq(PREFIX_PHYSICAL, FIRST_Q, LAST_Q_PHYSICAL)) {
        addField(field, QVariant::Int);
    }

    for (auto field : strseq(PREFIX_SOCIAL, FIRST_Q, LAST_Q_SOCIAL)) {
        addField(field, QVariant::Int);
    }

    for (auto field : strseq(PREFIX_EMOTIONAL, FIRST_Q, LAST_Q_EMOTIONAL)) {
        addField(field, QVariant::Int);
    }

    for (auto field : strseq(PREFIX_FUNCTIONAL, FIRST_Q, LAST_Q_FUNCTIONAL)) {
        addField(field, QVariant::Int);
    }

    addField(IGNORE_SOCIAL_Q7, QVariant::Bool);

    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(IGNORE_SOCIAL_Q7, false, false);
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Factg::shortname() const
{
    return "FACT-G";
}


QString Factg::longname() const
{
    return tr("Functional Assessment of Cancer Therapy—General");
}


QString Factg::description() const
{
    return tr("A 27-item general cancer quality-of-life (QL) measure; "
              "version 4.");
}


Version Factg::minimumServerVersion() const
{
    return Version(2, 2, 8);
}


// ============================================================================
// Instance info
// ============================================================================

Factg::FactgScore Factg::getScores() const
{
    FactgScore s;

    int answered, sum;

    auto reset = [&answered, &sum]() -> void {
        answered = 0;
        sum = 0;
    };

    auto processQuestion = [this, &answered, &sum](const QString& fieldname,
                                                   bool reverse) -> void {
        const QVariant& val = value(fieldname);
        if (!val.isNull()) {
            answered += 1;
            const int answer = val.toInt();
            if (reverse) {
                sum += MAX_QSCORE - answer;
            } else {
                sum += answer;
            }
        }
    };

    // Physical
    reset();
    for (const QString& fieldname : strseq(PREFIX_PHYSICAL,
                                           FIRST_Q, LAST_Q_PHYSICAL)) {
        // All negatively scored (reverse)
        processQuestion(fieldname, true);
    }
    if (answered > 0) {
        s.score_phys = sum * N_PHYSICAL / static_cast<double>(answered);
    }

    // Social
    reset();
    for (const QString& fieldname : strseq(PREFIX_SOCIAL,
                                           FIRST_Q, LAST_Q_SOCIAL)) {
        // All positively scored (do not reverse)
        processQuestion(fieldname, false);
    }
    if (answered > 0) {
        s.score_soc = sum * N_SOCIAL / static_cast<double>(answered);
    }

    // Emotional
    reset();
    for (int qnum = FIRST_Q; qnum <= LAST_Q_EMOTIONAL; ++qnum) {
        // Mixture of negative and positive scoring.
        const QString& fieldname = strnum(PREFIX_EMOTIONAL, qnum);
        const bool reverse = qnum != NON_REVERSE_SCORED_EMOTIONAL_QNUM;
        processQuestion(fieldname, reverse);
    }
    if (answered > 0) {
        s.score_emo = sum * N_EMOTIONAL / static_cast<double>(answered);
    }

    // Functional
    reset();
    for (const QString& fieldname : strseq(PREFIX_FUNCTIONAL,
                                           FIRST_Q, LAST_Q_FUNCTIONAL)) {
        // All positively scored (do not reverse)
        processQuestion(fieldname, false);
    }
    if (answered > 0) {
        s.score_func = sum * N_FUNCTIONAL / static_cast<double>(answered);
    }

    return s;
}


QStringList Factg::summary() const
{
    FactgScore s = getScores();
    return QStringList{totalScorePhrase(s.total(), MAX_SCORE)};
}


QStringList Factg::detail() const
{
    FactgScore s = getScores();

    QStringList lines{
        totalScorePhrase(s.total(), MAX_SCORE),
        scorePhrase(SUBTITLE_PHYSICAL, s.score_phys,
                    MAX_SCORE_PHYSICAL),
        scorePhrase(SUBTITLE_SOCIAL, s.score_soc,
                    MAX_SCORE_SOCIAL),
        scorePhrase(SUBTITLE_EMOTIONAL, s.score_emo,
                    MAX_SCORE_EMOTIONAL),
        scorePhrase(SUBTITLE_FUNCTIONAL, s.score_func,
                    MAX_SCORE_FUNCTIONAL)
    };
    lines.append("");
    lines.append("Answers (not scores):");

    // Physical
    lines.append("");
    lines.append(xstring("h1"));
    for (auto fieldname : strseq(PREFIX_PHYSICAL, FIRST_Q, LAST_Q_PHYSICAL)) {
        lines.append(fieldSummary(fieldname, xstring(fieldname)));
    }
    // Social
    lines.append("");
    lines.append(xstring("h2"));
    for (auto fieldname : strseq(PREFIX_SOCIAL, FIRST_Q, LAST_Q_SOCIAL - 1)) {
        lines.append(fieldSummary(fieldname, xstring(fieldname)));
    }
    lines.append(fieldSummary(IGNORE_SOCIAL_Q7, xstring(XSTRING_PREFER_NO_ANSWER)));
    const QString last_social_q = strnum(PREFIX_SOCIAL, LAST_Q_SOCIAL);
    lines.append(fieldSummary(last_social_q, xstring(last_social_q)));

    // Emotional
    lines.append("");
    lines.append(xstring("h3"));
    for (auto fieldname : strseq(PREFIX_EMOTIONAL, FIRST_Q, LAST_Q_EMOTIONAL)) {
        lines.append(fieldSummary(fieldname, xstring(fieldname)));
    }

    // Functional
    lines.append("");
    lines.append(xstring("h4"));
    for (auto fieldname : strseq(PREFIX_FUNCTIONAL, FIRST_Q, LAST_Q_FUNCTIONAL)) {
        lines.append(fieldSummary(fieldname, xstring(fieldname)));
    }

    return completenessInfo() + lines;
}


bool Factg::isComplete() const
{
    int last_q_social = LAST_Q_SOCIAL;

    if (valueBool(IGNORE_SOCIAL_Q7)) {
        --last_q_social;
    }

    return
        !(anyNull(values(strseq(PREFIX_PHYSICAL, FIRST_Q, LAST_Q_PHYSICAL))) ||
        anyNull(values(strseq(PREFIX_SOCIAL, FIRST_Q, last_q_social)))       ||
        anyNull(values(strseq(PREFIX_EMOTIONAL, FIRST_Q, LAST_Q_EMOTIONAL))) ||
        anyNull(values(strseq(PREFIX_FUNCTIONAL, FIRST_Q, LAST_Q_FUNCTIONAL))));
}


void Factg::updateQ7(const FieldRef* fieldref)
{
    // Called when the user ticks/unticks the tickbox.
    // Signal comes from IGNORE_SOCIAL_Q7 which is "prefer not to answer social
    // Q7 about sex life".

    // qDebug() << Q_FUNC_INFO << *fieldref;

    if (m_in_tickbox_change) {
        // avoid circular signal
        return;
    }
    if (!fieldref) {
        // in case of bugs
        return;
    }
    m_in_tickbox_change = true;

    const bool prefer_no_answer = fieldref->valueBool();
    // qDebug() << Q_FUNC_INFO << "fieldref->value()" << fieldref->value();

    FieldRefPtr fr_q7 = fieldRef(OPTIONAL_Q);
    fr_q7->setMandatory(!prefer_no_answer);

    if (prefer_no_answer) {
        fr_q7->setValue(QVariant());
    }

    m_in_tickbox_change = false;
}


void Factg::untickBox()
{
    // Called if the user does in fact answer the sex life question;
    // automatically unticks "don't wish to answer".
    qDebug() << Q_FUNC_INFO;
    if (m_in_tickbox_change) {
        // avoid circular signal
        return;
    }
    m_in_tickbox_change = true;
    fieldRef(IGNORE_SOCIAL_Q7)->setValue(false);
    m_in_tickbox_change = false;
}


OpenableWidget* Factg::editor(const bool read_only)
{
    const NameValueOptions options{
        {xstring("a0"), 0},
        {xstring("a1"), 1},
        {xstring("a2"), 2},
        {xstring("a3"), 3},
        {xstring("a4"), 4},
    };

    const int question_width = 50;
    const QVector<int> option_widths{10, 10, 10, 10, 10};
    const QString title_main = xstring("title_main");
    const QString instruction = xstring("instruction");
    const QString heading1 = xstring("h1");
    const QString heading2 = xstring("h2");
    const QString heading3 = xstring("h3");
    const QString heading4 = xstring("h4");

    // ========================================================================
    // Physical
    // ========================================================================
    QVector<QuestionWithOneField> fields;
    for (auto field : strseq(PREFIX_PHYSICAL, FIRST_Q, LAST_Q_PHYSICAL)) {
        fields.append(QuestionWithOneField(xstring(field), fieldRef(field)));
    }
    QuPagePtr p1(
        (new QuPage{
            (new QuHeading(heading1)),
            (new QuText(instruction))->setBold(true),
            (new QuMcqGrid({fields}, options))
                ->setExpand(true)
                ->setWidth(question_width, option_widths)
        })
            ->setTitle(title_main)
            ->setIndexTitle(heading1)
    );

    // ========================================================================
    // Social
    // ========================================================================
    fields.clear();
    for (auto field : strseq(PREFIX_SOCIAL, FIRST_Q, LAST_Q_SOCIAL - 1)) {
        fields.append(QuestionWithOneField(xstring(field), fieldRef(field)));
    }

    QuMcqGrid* g1;
    QuMcqGrid* g2;

    g1 = (new QuMcqGrid({fields}, options))
            ->setExpand(true)
            ->setWidth(question_width, option_widths);

    FieldRefPtr ignore_s_q7 = fieldRef(IGNORE_SOCIAL_Q7, false);
    connect(ignore_s_q7.data(), &FieldRef::valueChanged, this,
            &Factg::updateQ7);

    QuBoolean* no_answer = (new QuBoolean(
                            xstring(XSTRING_PREFER_NO_ANSWER),
                            ignore_s_q7))
            ->setFalseAppearsBlank();

    FieldRefPtr fr_q7 = fieldRef(PREFIX_SOCIAL + "7");
    fr_q7->setMandatory(!ignore_s_q7->valueBool());

    connect(fr_q7.data(), &FieldRef::valueChanged, this,
            &Factg::untickBox);

    g2 = (new QuMcqGrid({
            QuestionWithOneField(xstring(OPTIONAL_Q),
                                 fr_q7)}, options))
            ->showTitle(false)
            ->setExpand(true)
            ->setWidth(question_width, option_widths);

    QuPagePtr p2(
        (new QuPage{
            (new QuHeading(heading2)),
            (new QuText(instruction))->setBold(true),
            g1,
            no_answer,
            g2
        })
            ->setTitle(title_main)
            ->setIndexTitle(heading2)
    );

    // ========================================================================
    // Emotional
    // ========================================================================
    g1 = (new QuMcqGrid({
            QuestionWithOneField(xstring(PREFIX_EMOTIONAL + "1"),
                                 fieldRef(PREFIX_EMOTIONAL + "1"))
                                  }, options))
            ->setExpand(true)
            ->setWidth(question_width, option_widths);


    g2 = (new QuMcqGrid({
            QuestionWithOneField(xstring(PREFIX_EMOTIONAL + "2"),
                                 fieldRef(PREFIX_EMOTIONAL + "2"))
                                  }, options))
            ->showTitle(false)
            ->setExpand(true)
            ->setWidth(question_width, option_widths);

    fields.clear();
    for (auto field : strseq(PREFIX_EMOTIONAL, 3, LAST_Q_EMOTIONAL)) {
        fields.append(QuestionWithOneField(xstring(field), fieldRef(field)));
    }
    QuPagePtr p3(
        (new QuPage{
            (new QuHeading(heading3)),
            (new QuText(instruction))->setBold(true),
            g1,
            g2,
            (new QuMcqGrid({fields}, options))
                ->showTitle(false)
                ->setExpand(true)
                ->setWidth(question_width, option_widths)
        })
            ->setTitle(title_main)
            ->setIndexTitle(heading3)
    );

    // ========================================================================
    // Functional
    // ========================================================================
    fields.clear();
    for (auto field : strseq(PREFIX_FUNCTIONAL, FIRST_Q, LAST_Q_FUNCTIONAL)) {
        fields.append(QuestionWithOneField(xstring(field), fieldRef(field)));
    }
    QuPagePtr p4(
        (new QuPage{
            (new QuHeading(heading4)),
            (new QuText(instruction))->setBold(true),
            (new QuMcqGrid({fields}, options))
                ->setExpand(true),
            new QuSpacer(),
            (new QuText(xstring("thanks")))->setBold(true),
        })
            ->setTitle(title_main)
            ->setIndexTitle(heading4)
    );

    auto q = new Questionnaire(m_app, {p1, p2, p3, p4});
    q->setReadOnly(read_only);
    return q;
}
