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

#include "pdss.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::meanOrNull;
using mathfunc::noneNull;
using mathfunc::scorePhrase;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 7;
const int MAX_SCORE = 28;
const int MAX_COMPOSITE_SCORE = 4;
const QString QPREFIX("q");

const QString Pdss::PDSS_TABLENAME("pdss");


void initializePdss(TaskFactory& factory)
{
    static TaskRegistrar<Pdss> registered(factory);
}


Pdss::Pdss(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, PDSS_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Pdss::shortname() const
{
    return "PDSS";
}


QString Pdss::longname() const
{
    return tr("Panic Disorder Severity Scale");
}


QString Pdss::description() const
{
    return tr("7-item self-report scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Pdss::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Pdss::summary() const
{
    return QStringList{
        totalScorePhrase(totalScore(), MAX_SCORE),
        scorePhrase("Composite score", compositeScore(), MAX_COMPOSITE_SCORE),
    };
}


QStringList Pdss::detail() const
{
    return completenessInfo() + summary();

}


OpenableWidget* Pdss::editor(const bool read_only)
{
    QVector<QuElement*> elements;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        NameValueOptions options;
        for (int j = 0; j <= 4; ++j) {
            const QString xstringname = QString("q%1_option%2").arg(i).arg(j);
            options.append(NameValuePair(xstring(xstringname), j));
        }
        if (i > FIRST_Q) {
            elements.append(new QuHorizontalLine());
        }
        elements.append(new QuText(xstring(strnum("q", i))));
        elements.append(new QuMcq(fieldRef(strnum(QPREFIX, i)), options));
    }
    QuPagePtr page((new QuPage(elements))->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Pdss::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


double Pdss::compositeScore() const
{
    const QVariant m = meanOrNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)), true);
    return m.toDouble();
}
