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

#include "ybocs.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::scorePhrase;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_MAIN_QUESTIONS = 19;
const int LAST_OBSESSION_Q = 5;
const int FIRST_COMPULSION_Q = 6;
const int LAST_SCORED_Q = 10;
const QString QPREFIX("q");

const int MAX_OBSESSION_SCORE = 20;
const int MAX_COMPULSION_SCORE = 20;
const int MAX_SCORE = 40;

const QString Ybocs::YBOCS_TABLENAME("ybocs");

const QString Q1B("q1b");
const QString Q6B("q6b");
const QString TARGET_OBSESSION_1("target_obsession_1");
const QString TARGET_OBSESSION_2("target_obsession_2");
const QString TARGET_OBSESSION_3("target_obsession_3");
const QString TARGET_COMPULSION_1("target_compulsion_1");
const QString TARGET_COMPULSION_2("target_compulsion_2");
const QString TARGET_COMPULSION_3("target_compulsion_3");
const QString TARGET_AVOIDANCE_1("target_avoidance_1");
const QString TARGET_AVOIDANCE_2("target_avoidance_2");
const QString TARGET_AVOIDANCE_3("target_avoidance_3");

const QVector<QPair<QString, int>> QSEQUENCE{
    // Pairs are: question name, max_score
    {"1", 4},
    {"1b", 4},
    {"2", 4},
    {"3", 4},
    {"4", 4},
    {"5", 4},
    {"6", 4},
    {"6b", 4},
    {"7", 4},
    {"8", 4},
    {"9", 4},
    {"10", 4},
    {"11", 4},
    {"12", 4},
    {"13", 4},
    {"14", 4},
    {"15", 4},
    {"16", 4},
    {"17", 6},
    {"18", 6},
    {"19", 3},
};


void initializeYbocs(TaskFactory& factory)
{
    static TaskRegistrar<Ybocs> registered(factory);
}


Ybocs::Ybocs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, YBOCS_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(Q1B, QVariant::Int);
    addField(Q6B, QVariant::Int);
    addField(TARGET_OBSESSION_1, QVariant::String);
    addField(TARGET_OBSESSION_2, QVariant::String);
    addField(TARGET_OBSESSION_3, QVariant::String);
    addField(TARGET_COMPULSION_1, QVariant::String);
    addField(TARGET_COMPULSION_2, QVariant::String);
    addField(TARGET_COMPULSION_3, QVariant::String);
    addField(TARGET_AVOIDANCE_1, QVariant::String);
    addField(TARGET_AVOIDANCE_2, QVariant::String);
    addField(TARGET_AVOIDANCE_3, QVariant::String);
    addFields(strseq(QPREFIX, FIRST_Q, N_MAIN_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Ybocs::shortname() const
{
    return "Y-BOCS";
}


QString Ybocs::longname() const
{
    return tr("Yale–Brown Obsessive Compulsive Scale, 9/89 revision");
}


QString Ybocs::description() const
{
    return tr("10-item clinician-rated scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Ybocs::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_MAIN_QUESTIONS)));
}


QStringList Ybocs::summary() const
{
    return QStringList{
        totalScorePhrase(totalScore(), MAX_SCORE),
        scorePhrase(tr("Obsession score"),
                    obsessionScore(), MAX_OBSESSION_SCORE),
        scorePhrase(tr("Compulsion score"),
                    obsessionScore(), MAX_COMPULSION_SCORE),
    };
}


QStringList Ybocs::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Ybocs::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold(true);
    };
    auto boldtextRaw = [](const QString& text) -> QuElement* {
        return (new QuText(text))->setBold(true);
    };

    auto addpage = [this, &pages, &text, &boldtext]
            (const QString& q, int max_score) -> void {
        NameValueOptions options;
        for (int i = 0; i <= max_score; ++i) {
            const QString name = xstring(QString("q%1_a%2").arg(q).arg(i));
            options.append(NameValuePair(name, i));
        }
        const QString pagetitle = xstring(QString("q%1_title").arg(q));
        const QString xquestion = QString("q%1_question").arg(q);
        const QString xexplanation = QString("q%1_explanation").arg(q);
        const QString fieldname = QPREFIX + q;
        QuPagePtr page((new QuPage{
            boldtext(xquestion),
            text(xexplanation),
            new QuMcq(fieldRef(fieldname), options),
        })->setTitle(pagetitle));
        pages.append(page);
    };

    pages.append(getClinicianDetailsPage());

    // Instruction page
    QVector<QuElement*> page1elements{boldtext("instruction_1")};
    for (int i = 2; i <= 18; ++i) {
        page1elements.append(text(strnum("instruction_", i)));
    }
    pages.append(QuPagePtr((new QuPage(page1elements))->setTitle(longname())));

    // Target symptom page
    const QString obs = xstring("target_obsession_stem");
    const QString com = xstring("target_compulsion_stem");
    const QString avo = xstring("target_avoidance_stem");
    pages.append(QuPagePtr((new QuPage{
        boldtextRaw(obs),
        questionnairefunc::defaultGridRawPointer({
            {obs + " 1", new QuLineEdit(fieldRef(TARGET_OBSESSION_1))},
            {obs + " 2", new QuLineEdit(fieldRef(TARGET_OBSESSION_2))},
            {obs + " 3", new QuLineEdit(fieldRef(TARGET_OBSESSION_3))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
        boldtextRaw(com),
        questionnairefunc::defaultGridRawPointer({
            {com + " 1", new QuLineEdit(fieldRef(TARGET_COMPULSION_1))},
            {com + " 2", new QuLineEdit(fieldRef(TARGET_COMPULSION_2))},
            {com + " 3", new QuLineEdit(fieldRef(TARGET_COMPULSION_3))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
        boldtextRaw(avo),
        questionnairefunc::defaultGridRawPointer({
            {avo + " 1", new QuLineEdit(fieldRef(TARGET_AVOIDANCE_1))},
            {avo + " 2", new QuLineEdit(fieldRef(TARGET_AVOIDANCE_2))},
            {avo + " 3", new QuLineEdit(fieldRef(TARGET_AVOIDANCE_3))},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
    })->setTitle(xstring("target_symptom_list_title"))));

    // Question pages
    for (QPair<QString, int> pair : QSEQUENCE) {
        addpage(pair.first, pair.second);
    }

    // Last page
    pages.append(QuPagePtr((new QuPage{
        text("closing_1"),
        text("closing_2"),
        text("closing_3"),
    })->setTitle(tr("End matter"))));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Ybocs::obsessionScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, LAST_OBSESSION_Q)));
}


int Ybocs::compulsionScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_COMPULSION_Q, LAST_SCORED_Q)));
}


int Ybocs::totalScore() const
{
    return obsessionScore() + compulsionScore();
}
