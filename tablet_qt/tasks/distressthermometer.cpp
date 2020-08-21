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

#include "distressthermometer.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quthermometer.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::scorePhrase;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 36;
const int MAX_SCORE = N_QUESTIONS;
const QString QPREFIX("q");

const QString DistressThermometer::DT_TABLENAME("distressthermometer");

const QString DISTRESS("distress");
const QString OTHER("other");


void initializeDistressThermometer(TaskFactory& factory)
{
    static TaskRegistrar<DistressThermometer> registered(factory);
}


DistressThermometer::DistressThermometer(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, DT_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addField(DISTRESS, QVariant::Int);
    addField(OTHER, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString DistressThermometer::shortname() const
{
    return "Distress Thermometer";
}


QString DistressThermometer::longname() const
{
    return tr("Distress Thermometer");
}


QString DistressThermometer::description() const
{
    return tr("Self-rating of overall distress, plus Y/N rating of a range "
              "of potential problems.");
}


// ============================================================================
// Instance info
// ============================================================================

bool DistressThermometer::isComplete() const
{
    return !valueIsNull(DISTRESS) &&
            noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList DistressThermometer::summary() const
{
    return QStringList{
        scorePhrase(xstring("distress_s"), valueInt(DISTRESS), 10, " "),
        totalScorePhrase(totalScore(), MAX_SCORE),
    };
}


QStringList DistressThermometer::detail() const
{
    QStringList lines = completenessInfo();
    lines += summary();
    lines.append("");
    lines += fieldSummaries("q", "", ": ",
                            QPREFIX, FIRST_Q, N_QUESTIONS);
    lines += fieldSummary(OTHER, xstring("other_s"), " ");
    return lines;
}


OpenableWidget* DistressThermometer::editor(const bool read_only)
{
    QVector<QuThermometerItem> thermometer_items;
    for (int i = 0; i <= 10; ++i) {
        QString text = QString::number(i);
        if (i == 10) {
            text += " – " + xstring("distress_extreme");
        } else if (i == 0) {
            text += " – " + xstring("distress_none");
        }
        QuThermometerItem item(
            uifunc::resourceFilename(
                        QString("distressthermometer/dt_sel_%1.png").arg(i)),
            uifunc::resourceFilename(
                        QString("distressthermometer/dt_unsel_%1.png").arg(i)),
            text,
            i
        );
        thermometer_items.append(item);
    }

    QVector<QuPagePtr> pages;

    pages.append(QuPagePtr((new QuPage{
            new QuText(xstring("distress_question")),
            (new QuThermometer(fieldRef(DISTRESS), thermometer_items))
                                    ->setRescale(true, 0.4),
        })
        ->setTitle(xstring("section1_title"))
        ->allowScroll(false)  // 2018-10-29, for new thermometer widget
    ));

    QVector<QuestionWithOneField> qfpairs;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfpairs.append(QuestionWithOneField(xstring(strnum("q", i)),
                                            fieldRef(strnum(QPREFIX, i))));
    }
    QVector<McqGridSubtitle> subtitles{
        // {1 - 1, xstring("subtitle1")},  // use title instead
        {6 - 1, xstring("subtitle2")},
        {9 - 1, xstring("subtitle3")},
        {15 - 1, xstring("subtitle4")},
        {16 - 1, xstring("subtitle5")},
        {20, ""},
        {25, ""},
        {30, ""},
        {35, ""},
    };
    pages.append(QuPagePtr((new QuPage{
            new QuText(xstring("section2_stem")),
            (new QuMcqGrid(qfpairs, CommonOptions::yesNoInteger()))
                                    ->setTitle(xstring("subtitle1"))
                                    ->setSubtitles(subtitles),
        })
        ->setTitle(xstring("section2_title"))
    ));

    pages.append(QuPagePtr((new QuPage{
        new QuText(xstring("other_question")),
        new QuText(xstring("other_prompt")),
        new QuTextEdit(fieldRef(OTHER, false)),
    })->setTitle(xstring("section3_title"))));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int DistressThermometer::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
