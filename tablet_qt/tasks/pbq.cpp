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

#include "pbq.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 25;
const int MAX_PER_QUESTION = 5;  // each question scored 0-5
const int MAX_SCORE = N_QUESTIONS * MAX_PER_QUESTION;
const QString QPREFIX("q");
const QVector<int> SCORED_A0N5_Q{1, 4, 8, 9, 11, 16, 22, 25};  // rest scored A5N0

const QString Pbq::PBQ_TABLENAME("pbq");


void initializePbq(TaskFactory& factory)
{
    static TaskRegistrar<Pbq> registered(factory);
}


Pbq::Pbq(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, PBQ_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Pbq::shortname() const
{
    return "PBQ";
}


QString Pbq::longname() const
{
    return tr("Postpartum Bonding Questionnaire");
}


QString Pbq::description() const
{
    return tr("25-item self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Pbq::isComplete() const
{
    return noValuesNull(strseq(QPREFIX, FIRST_Q, N_QUESTIONS));
}


QStringList Pbq::summary() const
{
    return QStringList{
        totalScorePhrase(totalScore(), MAX_SCORE),
    };
}


QStringList Pbq::detail() const
{
    return completenessInfo() + summary();

}


OpenableWidget* Pbq::editor(const bool read_only)
{
    const QString always = xstring("always");
    const QString very_often = xstring("very_often");
    const QString quite_often = xstring("quite_often");
    const QString sometimes = xstring("sometimes");
    const QString rarely = xstring("rarely");
    const QString never = xstring("never");
    const NameValueOptions a0n5 = NameValueOptions{
        {always, 0},
        {very_often, 1},
        {quite_often, 2},
        {sometimes, 3},
        {rarely, 4},
        {never, 5},
    };
    const NameValueOptions a5n0 = NameValueOptions{
        {always, 5},
        {very_often, 4},
        {quite_often, 3},
        {sometimes, 2},
        {rarely, 1},
        {never, 0},
    };
    QVector<int> a0n5_indexes;
    for (const int qnum : SCORED_A0N5_Q) {
        a0n5_indexes.append(qnum - 1);  // zero-based indexes
    }
    QVector<QuestionWithOneField> qfp;
    QVector<McqGridSubtitle> subtitles;
    for (int qnum = FIRST_Q; qnum <= N_QUESTIONS; ++qnum) {
        const QString qname = strnum(QPREFIX, qnum);
        const QString qtext = xstring(qname);
        qfp.append(QuestionWithOneField(qtext, fieldRef(qname)));
        const int qindex = qnum - 1;
        if (qindex > 0 && qindex % 5 == 0) {  // every 5
            subtitles.append(McqGridSubtitle(qindex));
        }
    }

    QuMcqGrid* grid = new QuMcqGrid(qfp, a5n0);
    grid->setAlternateNameValueOptions(a0n5_indexes, a0n5);
    grid->setSubtitles(subtitles);

    QVector<QuElement*> elements{
        new QuText(xstring("stem")),
        grid,
    };
    QuPagePtr page((new QuPage(elements))->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Pbq::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}
