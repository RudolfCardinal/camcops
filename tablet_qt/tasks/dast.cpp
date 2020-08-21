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

#include "dast.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 28;
const int MAX_SCORE = N_QUESTIONS;
const QString QPREFIX("q");

const QString Dast::DAST_TABLENAME("dast");


void initializeDast(TaskFactory& factory)
{
    static TaskRegistrar<Dast> registered(factory);
}


Dast::Dast(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, DAST_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Dast::shortname() const
{
    return "DAST";
}


QString Dast::longname() const
{
    return tr("Drug Abuse Screening Test");
}


QString Dast::description() const
{
    return tr("28-item Y/N self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Dast::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Dast::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Dast::detail() const
{
    const int total = totalScore();
    const bool exceeds_cutoff_1 = total >= 6;
    const bool exceeds_cutoff_2 = total >= 11;
    const QString scores = xstring("scores");

    QStringList lines = completenessInfo();
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        const QVariant v = value(strnum(QPREFIX, i));
        lines.append(QString("%1 <b>%2</b> (%3 <b>%4</b>)")
                     .arg(xstring(strnum("q", i, "_s")),  // contains colon
                          convert::prettyValue(v),
                          scores,
                          QString::number(score(v, i))));
    }
    lines.append("");
    lines += summary();
    lines.append("");
    lines.append(stringfunc::standardResult(
                     xstring("exceeds_standard_cutoff_1"),
                     uifunc::yesNo(exceeds_cutoff_1),
                     " "));
    lines.append(stringfunc::standardResult(
                     xstring("exceeds_standard_cutoff_2"),
                     uifunc::yesNo(exceeds_cutoff_2),
                     " "));
    return lines;
}


OpenableWidget* Dast::editor(const bool read_only)
{
    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        qfields.append(QuestionWithOneField(xstring(strnum("q", i)),
                                            fieldRef(strnum(QPREFIX, i))));
    }
    QVector<McqGridSubtitle> subtitles{
        {5, ""},
        {10, ""},
        {15, ""},
        {20, ""},
        {25, ""},
    };

    QuPagePtr page((new QuPage{
        (new QuMcqGrid(qfields, CommonOptions::yesNoChar()))
                        ->setSubtitles(subtitles),
    })->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Dast::totalScore() const
{
    int total = 0;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        total += score(value(strnum(QPREFIX, i)), i);
    }
    return total;
}


int Dast::score(const QVariant& value, const int question) const
{
    if (value.isNull()) {
        return 0;
    }
    const QString yes = CommonOptions::YES_CHAR;
    if (question == 4 || question == 5 || question == 7) {
        return value == yes ? 0 : 1;
    }
    return value == yes ? 1 : 0;
}
