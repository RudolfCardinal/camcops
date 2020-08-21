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

#include "panss.h"
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
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int N_P = 7;
const int N_N = 7;
const int N_G = 16;
const int MAX_P = 49;
const int MAX_N = 49;
const int MAX_G = 112;
const int MAX_TOTAL = 210;
const QString P_PREFIX("p");
const QString N_PREFIX("n");
const QString G_PREFIX("g");

const QString Panss::PANSS_TABLENAME("panss");


void initializePanss(TaskFactory& factory)
{
    static TaskRegistrar<Panss> registered(factory);
}


Panss::Panss(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, PANSS_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(P_PREFIX, 1, N_P), QVariant::Int);
    addFields(strseq(N_PREFIX, 1, N_N), QVariant::Int);
    addFields(strseq(G_PREFIX, 1, N_G), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Panss::shortname() const
{
    return "PANSS";
}


QString Panss::longname() const
{
    return tr("Positive and Negative Syndrome Scale");
}


QString Panss::description() const
{
    return tr("Scale for positive (7 items) and negative symptoms of "
              "schizophrenia (7 items), and general psychopathology "
              "(16 items).");
}


// ============================================================================
// Instance info
// ============================================================================

bool Panss::isComplete() const
{
    return noneNull(values(strseq(P_PREFIX, 1, N_P))) &&
            noneNull(values(strseq(N_PREFIX, 1, N_N))) &&
            noneNull(values(strseq(G_PREFIX, 1, N_G)));
}


QStringList Panss::summary() const
{
    const int p = getP();
    const int n = getN();
    const int g = getG();
    const int composite = p - n;
    const int total = p + g + n;
    return QStringList{
        scorePhrase(xstring("p"), p, MAX_P),
        scorePhrase(xstring("n"), n, MAX_N),
        scorePhrase(xstring("g"), g, MAX_G),
        standardResult(xstring("composite"), QString::number(composite)),
        totalScorePhrase(total, MAX_TOTAL),
    };
}


QStringList Panss::detail() const
{
    QStringList lines = completenessInfo();
    lines += fieldSummaries("p", "_s", " ", P_PREFIX, 1, N_P);
    lines += fieldSummaries("n", "_s", " ", N_PREFIX, 1, N_N);
    lines += fieldSummaries("g", "_s", " ", G_PREFIX, 1, N_G);
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Panss::editor(const bool read_only)
{
    const NameValueOptions panss_options{
        {xstring("option1"), 1},
        {xstring("option2"), 2},
        {xstring("option3"), 3},
        {xstring("option4"), 4},
        {xstring("option5"), 5},
        {xstring("option6"), 6},
        {xstring("option7"), 7},
    };

    auto addqf = [this](QVector<QuestionWithOneField>& qf,
                        const QString& fieldprefix,
                        const QString& xstringprefix,
                        int q) -> void {
        qf.append(QuestionWithOneField(
                      xstring(xstringprefix + QString::number(q) + "_s"),
                      fieldRef(strnum(fieldprefix, q))));
    };
    auto boldtext = [](const QString& text) -> QuElement* {
        return (new QuText(text))->setBold(true);
    };

    QVector<QuestionWithOneField> p_qfields;
    QVector<QuestionWithOneField> n_qfields;
    QVector<QuestionWithOneField> g_qfields;
    for (int i = 1; i <= N_P; ++i) {
        addqf(p_qfields, P_PREFIX, "p", i);
    }
    for (int i = 1; i <= N_N; ++i) {
        addqf(n_qfields, N_PREFIX, "n", i);
    }
    for (int i = 1; i <= N_G; ++i) {
        addqf(g_qfields, G_PREFIX, "g", i);
    }

    QVector<QuPagePtr> pages;
    pages.append(getClinicianDetailsPage());

    pages.append(QuPagePtr((new QuPage{
        boldtext(TextConst::dataCollectionOnlyAnnouncement()),
        new QuMcqGrid(p_qfields, panss_options),
    })->setTitle(longname() + " (P)")));

    pages.append(QuPagePtr((new QuPage{
        boldtext(TextConst::dataCollectionOnlyAnnouncement()),
        new QuMcqGrid(n_qfields, panss_options),
    })->setTitle(longname() + " (N)")));

    pages.append(QuPagePtr((new QuPage{
        boldtext(TextConst::dataCollectionOnlyAnnouncement()),
        new QuMcqGrid(g_qfields, panss_options),
    })->setTitle(longname() + " (G)")));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Panss::getP() const
{
    return sumInt(values(strseq(P_PREFIX, 1, N_P)));
}


int Panss::getN() const
{
    return sumInt(values(strseq(N_PREFIX, 1, N_N)));
}


int Panss::getG() const
{
    return sumInt(values(strseq(G_PREFIX, 1, N_G)));
}
