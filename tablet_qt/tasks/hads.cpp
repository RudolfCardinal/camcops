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

#include "hads.h"
#include "common/appstrings.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::scorePhrase;
using stringfunc::strnum;
using stringfunc::strnumlist;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 14;
const int MAX_SCORE_ANXIETY = 21;
const int MAX_SCORE_DEPRESSION = 21;
const QVector<int> INVERTED_QUESTIONS{1, 3, 5, 6, 8, 10, 11, 13};
// ... For these questions, option 3 appears at the top of the choices.
const QVector<int> ANXIETY_QUESTIONS{1, 3, 5, 7, 9, 11, 13};
const QVector<int> DEPRESSION_QUESTIONS{2, 4, 6, 8, 10, 12, 14};

const QString QPREFIX("q");

const QString Hads::HADS_TABLENAME("hads");


void initializeHads(TaskFactory& factory)
{
    static TaskRegistrar<Hads> registered(factory);
}


Hads::Hads(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Hads(app, db, HADS_TABLENAME, false, load_pk)
{
    // Main HADS constructor. No respondent.
}


Hads::Hads(CamcopsApp& app, DatabaseManager& db,
           const QString& tablename, const bool has_respondent,
           const int load_pk) :
    Task(app, db, tablename, false, false, has_respondent)  // ... anon, clin, resp
{
    // Constructor used directly by HadsRespondent and indirectly by the other
    // constructor.

    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}



// ============================================================================
// Class info
// ============================================================================

QString Hads::shortname() const
{
    return "HADS";
}


QString Hads::longname() const
{
    return tr("Hospital Anxiety and Depression Scale");
}


QString Hads::description() const
{
    return tr("14-item self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Hads::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Hads::summary() const
{
    return QStringList{
        scorePhrase(appstring(appstrings::HADS_ANXIETY_SCORE),
                    getScore(ANXIETY_QUESTIONS), MAX_SCORE_ANXIETY),
        scorePhrase(appstring(appstrings::HADS_DEPRESSION_SCORE),
                    getScore(DEPRESSION_QUESTIONS), MAX_SCORE_DEPRESSION),
    };
}


QStringList Hads::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Hads::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto fulloptions = [this](int question) -> NameValueOptions {
        NameValueOptions options;
        for (int i = 0; i <= 3; ++i) {
            QString xstringname = QString("q%1_a%2").arg(question).arg(i);
            options.append(NameValuePair(xstring(xstringname), i));
        }
        return options;
    };
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };

    if (isCrippled()) {
        // --------------------------------------------------------------------
        // Succinct version without any task text
        // --------------------------------------------------------------------
        const NameValueOptions options{
            {"0", 0},
            {"1", 1},
            {"2", 2},
            {"3", 3},
        };
        QVector<QuestionWithOneField> qfields;
        for (int n = 1; n <= N_QUESTIONS; ++n) {
            QString question = QString("%1 %2").arg(TextConst::question()).arg(n);
            if (ANXIETY_QUESTIONS.contains(n)) {
                question += " (A)";
            }
            if (DEPRESSION_QUESTIONS.contains(n)) {
                question += " (D)";
            }
            qfields.append(QuestionWithOneField(question,
                                                fieldRef(strnum(QPREFIX, n))));
        }
        QVector<QuElement*> elements;
        if (hasRespondent()) {  // for HadsRespondent
            elements.append(getRespondentQuestionnaireBlockRawPointer(true));
        }
        elements.append((new QuText(TextConst::dataCollectionOnlyAnnouncement()))
                        ->setBold());
        elements.append(new QuText(TextConst::enterTheAnswers()));
        elements.append(new QuMcqGrid(qfields, options));
        pages.append(QuPagePtr((new QuPage(elements))->setTitle(longname())));

    } else {
        // --------------------------------------------------------------------
        // Full version, if available
        // --------------------------------------------------------------------
        QVector<QuElement*> elements;
        if (hasRespondent()) {  // for HadsRespondent
            elements.append(getRespondentQuestionnaireBlockRawPointer(true));
        }
        elements.append(text("instruction_1"));
        elements.append(text("instruction_2"));
        elements.append(text("instruction_3"));
        elements.append((new QuText(TextConst::pressNextToContinue()))
                        ->setBold());
        pages.append(QuPagePtr((new QuPage(elements))->setTitle(longname())));
        for (int n = 1; n <= N_QUESTIONS; ++n) {
            NameValueOptions options = fulloptions(n);
            if (INVERTED_QUESTIONS.contains(n)) {
                options.reverse();
            }
            pages.append(QuPagePtr((new QuPage{
                boldtext(strnum("q", n, "_stem")),
                new QuMcq(fieldRef(strnum(QPREFIX, n)), options),
            })->setTitle(longname() + strnum(" Q", n))));
        }
    }

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Hads::getScore(const QVector<int>& questions) const
{
    return sumInt(values(strnumlist(QPREFIX, questions)));
}
