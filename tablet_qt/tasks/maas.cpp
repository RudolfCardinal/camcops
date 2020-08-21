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

#include "maas.h"
#include "common/textconst.h"
#include "lib/stringfunc.h"
#include "maths/mathfunc.h"
#include "tasklib/taskfactory.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
using mathfunc::scorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int N_QUESTIONS = 19;
const int MIN_SCORE_PER_Q = 1;
const int MAX_SCORE_PER_Q = 5;

const QString Maas::MAAS_TABLENAME("maas");

const QString FN_QPREFIX("q");
const QString XSTRING_Q_PREFIX("q");
const QString XSTRING_Q_SUFFIX("_q");
const QString XSTRING_Q_FMT("q%1_q");
const QString XSTRING_A_FMT("q%1_a%2");

// Questions whose options are presented from 5 to 1, not from 1 to 5:
const QVector<int> REVERSED_Q{1, 3, 5, 6, 7, 9, 10, 12, 15, 16, 18};

// Questions that contribute to the "quality of attachment" score:
const QVector<int> QUALITY_OF_ATTACHMENT_Q{3, 6, 9, 10, 11, 12, 13, 15, 16, 19};

// Questions that contribute to the "time spent in attachment mode" score:
const QVector<int> TIME_IN_ATTACHMENT_MODE_Q{1, 2, 4, 5, 8, 14, 17, 18};


void initializeMaas(TaskFactory& factory)
{
    static TaskRegistrar<Maas> registered(factory);
}


Maas::Maas(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, MAAS_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(FN_QPREFIX, 1, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Maas::shortname() const
{
    return "MAAS";
}


QString Maas::longname() const
{
    return tr("Maternal Antenatal Attachment Scale");
}


QString Maas::description() const
{
    return tr("19-item self-report scale relating to attachment to an unborn baby.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Maas::isComplete() const
{
    return noValuesNull(strseq(FN_QPREFIX, 1, N_QUESTIONS));
}


QStringList Maas::summary() const
{
    int quality_min = 0;
    int quality_score = 0;
    int quality_max = 0;
    int time_min = 0;
    int time_score = 0;
    int time_max = 0;
    int global_min = 0;
    int global_score = 0;
    int global_max = 0;
    for (int q = 1; q <= N_QUESTIONS; ++q) {
        const QVariant v = value(strnum(FN_QPREFIX, q));
        if (v.isNull()) {
            continue;
        }
        const int score = v.toInt();
        if (QUALITY_OF_ATTACHMENT_Q.contains(q)) {
            quality_min += MIN_SCORE_PER_Q;
            quality_score += score;
            quality_max += MAX_SCORE_PER_Q;
        }
        if (TIME_IN_ATTACHMENT_MODE_Q.contains(q)) {
            time_min += MIN_SCORE_PER_Q;
            time_score += score;
            time_max += MAX_SCORE_PER_Q;
        }
        global_min += MIN_SCORE_PER_Q;
        global_score += score;
        global_max += MAX_SCORE_PER_Q;
    }
    const QString fmt("%1 [%2–%3]: <b>%4</b>.");
    return QStringList{
        fmt.arg(xstring("quality_of_attachment_score"),
                QString::number(quality_min), QString::number(quality_max),
                QString::number(quality_score)),
        fmt.arg(xstring("time_in_attachment_mode_score"),
                QString::number(time_min), QString::number(time_max),
                QString::number(time_score)),
        fmt.arg(xstring("global_attachment_score"),
                QString::number(global_min), QString::number(global_max),
                QString::number(global_score)),
    };
}


QStringList Maas::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries(XSTRING_Q_PREFIX, XSTRING_Q_SUFFIX, " ",
                            FN_QPREFIX, 1, N_QUESTIONS);
    lines.append("");
    return lines + summary();
}


OpenableWidget* Maas::editor(const bool read_only)
{
    const QString copyright = xstring("copyright");
    const QVector<int> answers{1, 2, 3, 4, 5};
    QVector<QuPage*> pages;
    auto addQ = [this, &answers](QVector<QuElement*>& elements,
            const int q) -> void {
        // Establish options
        QVector<int> answer_sequence = answers;
        // ... If you mistakenly add "const" you get the error
        //     "no matching functino for call to 'swap(const in&, const int&)'"
        if (REVERSED_Q.contains(q)) {
            std::reverse(answer_sequence.begin(), answer_sequence.end());
        }
        NameValueOptions options;
        for (const int a : answer_sequence) {
            const QString atext = xstring(XSTRING_A_FMT.arg(
                                    QString::number(q), QString::number(a)));
            options.append(NameValuePair(atext, a));
        }
        // Create question
        const QString xstringname(XSTRING_Q_FMT.arg(q));
        QuText* question = new QuText(xstring(xstringname));
        question->setBold();
        elements.append(question);
        // Create MCQ
        const QString fieldname = strnum(FN_QPREFIX, q);
        QuMcq* mcq = new QuMcq(fieldRef(fieldname), options);
        elements.append(mcq);
    };
    auto addText = [this](QVector<QuElement*>& elements,
            const QString& xstringname) -> void {
        QuText* t = new QuText(xstring(xstringname));
        t->setBig();
        elements.append(t);
    };
    auto addCopyright = [&copyright](QVector<QuElement*>& elements) -> void {
        elements.append(new QuSpacer());
        elements.append(new QuSpacer());
        QuText* t = new QuText(copyright);
        t->setItalic();
        t->setFontSize(uiconst::FontSize::VerySmall);
        elements.append(t);
    };
    auto addPage = [&](const int q) {
        QVector<QuElement*> elements;
        if (q == 1) {
            addText(elements, "instructions");
            elements.append(new QuSpacer());
        }
        addQ(elements, q);
        if (q == N_QUESTIONS) {
            elements.append(new QuSpacer());
            addText(elements, "thanks");
            addCopyright(elements);
        }
        QuPage* page = new QuPage(elements);
        page->setTitle(QString("%1 %2").arg(textconst.question(),
                                            QString::number(q)));
        pages.append(page);
    };

    for (int qnum = 1; qnum <= N_QUESTIONS; ++qnum) {
        addPage(qnum);
    }

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}
