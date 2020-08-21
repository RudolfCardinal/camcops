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

#include "demqol.h"
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
const int N_QUESTIONS = 29;
const int N_SCORED_QUESTIONS = 28;
const int MISSING_VALUE = -99;
const int MINIMUM_N_FOR_TOTAL_SCORE = 14;
const QVector<int>REVERSE_SCORE{1, 3, 5, 6, 10, 29};  // questions scored backwards

const QString QPREFIX("q");

const QString Demqol::DEMQOL_TABLENAME("demqol");


void initializeDemqol(TaskFactory& factory)
{
    static TaskRegistrar<Demqol> registered(factory);
}


Demqol::Demqol(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, DEMQOL_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Demqol::shortname() const
{
    return "DEMQOL";
}


QString Demqol::longname() const
{
    return tr("Dementia Quality of Life measure, self-report version");
}


QString Demqol::description() const
{
    return tr("28-item interviewer-administered questionnaire answered by the "
              "person with dementia.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Demqol::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Demqol::summary() const
{
    const int dp = 2;
    return QStringList{stringfunc::standardResult(
                    TextConst::totalScore(),
                    convert::prettyValue(totalScore(), dp)),
                    " (Q1–28, range 28–112)"};
}


QStringList Demqol::detail() const
{
    return completenessInfo() +
            fieldSummaries("q", "", " ", QPREFIX, 1, N_QUESTIONS) +
            QStringList{""} +
            summary();
}


OpenableWidget* Demqol::editor(const bool read_only)
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
        getClinicianDetailsPage(),
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
            qf.append(QuestionWithOneField(xstring(strnum("q", i)),
                                           fieldRef(strnum(QPREFIX, i))));
        }
        return qf;
    };

    pages.append(QuPagePtr((new QuPage{
        italic("instruction1"),
        bold("instruction2"),
        bold("instruction3"),
        italic("instruction4"),
        bold("instruction5"),
        bold("a1"),
        bold("a2"),
        bold("a3"),
        bold("a4"),
        italic("instruction6"),
        bold("instruction7"),
        italic("instruction8"),
        bold("instruction9"),
    })->setTitle(title(1))));

    pages.append(QuPagePtr((new QuPage{
        bold("instruction10"),
        bold("instruction11"),
        new QuMcqGrid(qfields(1, 13), main_options),
    })->setTitle(title(2))));

    pages.append(QuPagePtr((new QuPage{
        bold("instruction12"),
        new QuMcqGrid(qfields(14, 19), main_options),
    })->setTitle(title(3))));

    pages.append(QuPagePtr((new QuPage{
        bold("instruction13"),
        new QuMcqGrid(qfields(20, 28), main_options),
    })->setTitle(title(4))));

    pages.append(QuPagePtr((new QuPage{
        bold("instruction14"),
        bold("q29"),
        new QuMcq(fieldRef(strnum(QPREFIX, 29)), qol_options),
    })->setTitle(title(5))));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QVariant Demqol::totalScore() const
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
