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

#include "demqolproxy.h"
#include "common/textconst.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 32;
const int N_SCORED_QUESTIONS = 31;
const int MISSING_VALUE = -99;
const int MINIMUM_N_FOR_TOTAL_SCORE = 16;
const QVector<int>REVERSE_SCORE{1, 4, 6, 8, 11, 32};  // questions scored backwards

const QString QPREFIX("q");

const QString DemqolProxy::DEMQOLPROXY_TABLENAME("demqolproxy");


void initializeDemqolProxy(TaskFactory& factory)
{
    static TaskRegistrar<DemqolProxy> registered(factory);
}


DemqolProxy::DemqolProxy(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, DEMQOLPROXY_TABLENAME, false, true, true)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString DemqolProxy::shortname() const
{
    return "DEMQOL-Proxy";
}


QString DemqolProxy::longname() const
{
    return tr("Dementia Quality of Life measure, proxy version");
}


QString DemqolProxy::description() const
{
    return tr("31-item interviewer-administered questionnaire answered by a "
              "caregiver.");
}


QString DemqolProxy::infoFilenameStem() const
{
    return "demqol";  // shares its HTML
}


QString DemqolProxy::xstringTaskname() const
{
    return "demqol";
}


// ============================================================================
// Instance info
// ============================================================================

bool DemqolProxy::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList DemqolProxy::summary() const
{
    const int dp = 2;
    return QStringList{stringfunc::standardResult(
                    TextConst::totalScore(),
                    convert::prettyValue(totalScore(), dp)),
                    " (Q1–31, range 31–124)"};
}


QStringList DemqolProxy::detail() const
{
    return completenessInfo() +
            fieldSummaries("q", "", " ", QPREFIX, 1, N_QUESTIONS) +
            QStringList{""} +
            summary();
}


OpenableWidget* DemqolProxy::editor(const bool read_only)
{
    const NameValueOptions main_options{
        {xstring("a1"), 1},
        {xstring("a2"), 2},
        {xstring("a3"), 3},
        {xstring("a4"), 4},
        {xstring("no_response"), MISSING_VALUE},
    };
    const NameValueOptions qol_options{
        {xstring("q29_a1"), 1},
        {xstring("q29_a2"), 2},
        {xstring("q29_a3"), 3},
        {xstring("q29_a4"), 4},
        {xstring("no_response"), MISSING_VALUE},
    };
    QVector<QuPagePtr> pages{
        getClinicianAndRespondentDetailsPage(false),
    };

    auto title = [this](int pagenum) -> QString {
        return shortname() + " " + TextConst::page() + " " +
                QString::number(pagenum) + "/5";
    };
    auto bold = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto italic = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setItalic();
    };
    auto qfields = [this](int first,
                          int last) -> QVector<QuestionWithOneField> {
        QVector<QuestionWithOneField> qf;
        for (int i = first; i <= last; ++i) {
            qf.append(QuestionWithOneField(xstring(strnum("proxy_q", i)),
                                           fieldRef(strnum(QPREFIX, i))));
        }
        return qf;
    };

    pages.append(QuPagePtr((new QuPage{
        italic("proxy_instruction1"),
        bold("proxy_instruction2"),
        bold("proxy_instruction3"),
        italic("proxy_instruction4"),
        bold("proxy_instruction5"),
        bold("a1"),
        bold("a2"),
        bold("a3"),
        bold("a4"),
        italic("proxy_instruction6"),
        bold("proxy_instruction7"),
        italic("proxy_instruction8"),
        bold("proxy_instruction9"),
    })->setTitle(title(1))));

    pages.append(QuPagePtr((new QuPage{
        bold("proxy_instruction10"),
        bold("proxy_instruction11"),
        new QuMcqGrid(qfields(1, 11), main_options),
    })->setTitle(title(2))));

    pages.append(QuPagePtr((new QuPage{
        bold("proxy_instruction12"),
        new QuMcqGrid(qfields(12, 20), main_options),
    })->setTitle(title(3))));

    pages.append(QuPagePtr((new QuPage{
        bold("proxy_instruction13"),
        new QuMcqGrid(qfields(21, 31), main_options),
    })->setTitle(title(4))));

    pages.append(QuPagePtr((new QuPage{
        bold("proxy_instruction14"),
        bold("proxy_q32"),
        new QuMcq(fieldRef(strnum(QPREFIX, 32)), qol_options),
    })->setTitle(title(5))));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QVariant DemqolProxy::totalScore() const
{
    // Higher score, better HRQL (health-related quality of life)
    int n = 0;
    int total = 0;
    for (int q = 1; q <= N_SCORED_QUESTIONS; ++q) {
        const QVariant v = value(strnum(QPREFIX, q));
        if (v.isNull()) {
            continue;
        }
        int x = v.toInt();
        if (x == MISSING_VALUE) {
            continue;
        }
        if (REVERSE_SCORE.contains(q)) {
            x = 5 - x;
        }
        n += 1;
        total += x;
    }
    if (n < MINIMUM_N_FOR_TOTAL_SCORE) {
        return QVariant();
    }
    if (n < N_SCORED_QUESTIONS) {
        // As per the authors' sample SPSS script (spss-syntax-demqol.pdf),
        // but in a more obvious mathematical way:
        return double(N_SCORED_QUESTIONS * total) / double(n);
    }
    return total;
}
