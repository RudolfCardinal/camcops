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

#include "honosca.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qulineedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 15;
const int MAX_SCORE = 60;
const QString QPREFIX("q");

const QString Honosca::HONOSCA_TABLENAME("honosca");

const QString PERIOD_RATED("period_rated");
const QString Q8("q8");
const QString VALUE_OTHER("J");


void initializeHonosca(TaskFactory& factory)
{
    static TaskRegistrar<Honosca> registered(factory);
}


Honosca::Honosca(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, HONOSCA_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addField(PERIOD_RATED, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Honosca::shortname() const
{
    return "HoNOSCA";
}


QString Honosca::longname() const
{
    return tr("Health of the Nation Outcome Scales, Children and Adolescents");
}


QString Honosca::description() const
{
    return tr("13- to 15-item clinician-rated scale.");
}


QString Honosca::infoFilenameStem() const
{
    return "honos";
}


// ============================================================================
// Instance info
// ============================================================================

bool Honosca::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS))) &&
            !valueIsNullOrEmpty(PERIOD_RATED);
}


QStringList Honosca::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Honosca::detail() const
{
    const int section_a = scoreSum(1, 13);
    const int section_b = scoreSum(14, 15);
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ", QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += standardResult(xstring("section_a_total"),
                            QString::number(section_a));
    lines += standardResult(xstring("section_b_total"),
                            QString::number(section_b));
    lines += summary();
    return lines;
}


OpenableWidget* Honosca::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto getoptions = [this](int n) -> NameValueOptions {
        NameValueOptions options;
        for (int i = 0; i <= 4; ++i) {
            const QString name = xstring(QString("q%1_option%2").arg(n).arg(i));
            options.append(NameValuePair(name, i));
        }
        options.append(NameValuePair(xstring("option9"), 9));
        return options;
    };

    auto addpage = [this, &pages, &getoptions]
                   (int n, const QString& titleprefix) -> void {
        const NameValueOptions options = getoptions(n);
        const QString pagetitle = titleprefix + QString::number(n);
        const QString question = xstring(strnum("q", n));
        const QString fieldname = strnum(QPREFIX, n);
        QVector<QuElement*> elements{
            new QuText(question),
            new QuMcq(fieldRef(fieldname), options),
        };
        pages.append(QuPagePtr((new QuPage(elements))->setTitle(pagetitle)));
    };

    pages.append(getClinicianDetailsPage());

    pages.append(QuPagePtr((new QuPage{
        new QuText(xstring("period_rated")),
        new QuLineEdit(fieldRef(PERIOD_RATED)),
    })->setTitle(xstring("firstpage_title"))));

    pages.append(QuPagePtr((new QuPage{
        new QuText(xstring("section_a_instructions")),
    })->setTitle(xstring("section_a_title"))));

    for (int n = FIRST_Q; n <= 13; ++n) {
        addpage(n, xstring("section_a_title_prefix"));
    }

    pages.append(QuPagePtr((new QuPage{
        new QuText(xstring("section_b_instructions")),
    })->setTitle(xstring("section_b_title"))));

    for (int n = 14; n <= N_QUESTIONS; ++n) {
        addpage(n, xstring("section_b_title_prefix"));
    }

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Honosca::totalScore() const
{
    return scoreSum(FIRST_Q, N_QUESTIONS);
}


int Honosca::scoreSum(const int first, const int last) const
{
    int total = 0;
    for (int i = first; i <= last; ++i) {
        const int v = valueInt(strnum(QPREFIX, i));
        if (v != 9) {  // 9 is "not known"
            total += v;
        }
    }
    return total;
}
