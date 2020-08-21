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

#include "frs.h"
#include <limits>
#include "common/textconst.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 30;
const QString QPREFIX("q");

const QString Frs::FRS_TABLENAME("frs");

const QString COMMENTS("comments");

const int NEVER = 0;
const int SOMETIMES = 1;
const int ALWAYS = 2;
const int NA = -99;
const QVector<int> NA_QUESTIONS{9, 10, 11, 13, 14, 15, 17, 18, 19, 20, 21, 27};
const QVector<int> SPECIAL_NA_TEXT_QUESTIONS{27};
const QVector<int> NO_SOMETIMES_QUESTIONS{30};
const double DOUBLE_INFINITY = std::numeric_limits<double>::infinity();
const QVector<QPair<QPair<double, double>, double>> TABULAR_LOGIT_RANGES{
    // pairs are: {{a, b}, result}
    // tests a <= x < b; if true, returns result
    {{100, DOUBLE_INFINITY}, 5.39},
    {{97, 100}, 4.12},
    {{93, 97}, 3.35},
    {{90, 93}, 2.86},
    {{87, 90}, 2.49},
    {{83, 87}, 2.19},
    {{80, 83}, 1.92},
    {{77, 80}, 1.68},
    {{73, 77}, 1.47},
    {{70, 73}, 1.26},
    {{67, 70}, 1.07},
    {{63, 67}, 0.88},
    {{60, 63}, 0.7},
    {{57, 60}, 0.52},
    {{53, 57}, 0.34},
    {{50, 53}, 0.16},
    {{47, 50}, -0.02},
    {{43, 47}, -0.2},
    {{40, 43}, -0.4},
    {{37, 40}, -0.59},
    {{33, 37}, -0.8},
    {{30, 33}, -1.03},
    {{27, 30}, -1.27},
    {{23, 27}, -1.54},
    {{20, 23}, -1.84},
    {{17, 20}, -2.18},
    {{13, 17}, -2.58},
    {{10, 13}, -3.09},
    {{6, 10}, -3.8},
    {{3, 6}, -4.99},
    {{0, 3}, -6.66},
};
const QMap<int, int> SCORE{
    {NEVER, 1},
    {SOMETIMES, 0},
    {ALWAYS, 0},
};
// Confirmed by Eneida Mioshi 2015-01-20; "sometimes" and "always"
// score the same.


void initializeFrs(TaskFactory& factory)
{
    static TaskRegistrar<Frs> registered(factory);
}


Frs::Frs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, FRS_TABLENAME, false, true, true)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addField(COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Frs::shortname() const
{
    return "FRS";
}


QString Frs::longname() const
{
    return tr("Frontotemporal Dementia Rating Scale");
}


QString Frs::description() const
{
    return tr("30-item clinician-administered scale based on carer "
              "information.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Frs::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Frs::summary() const
{
    const ScoreInfo si = getScore();
    QStringList lines;
    const QString sep = " = ";
    lines.append(standardResult("Total", convert::prettyValue(si.total), sep,
                                " (0–n, higher better)."));
    lines.append(standardResult("n", convert::prettyValue(si.n), sep,
                                QString(" (out of %1).").arg(N_QUESTIONS)));
    lines.append(standardResult("Score", convert::prettyValue(si.score), sep,
                                " (0–1)."));
    lines.append(standardResult("Tabulated logit of score",
                                convert::prettyValue(si.logit), sep, "."));
    lines.append(standardResult("Severity", convert::prettyValue(si.severity),
                                sep, "."));
    return lines;
}


QStringList Frs::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Frs::editor(const bool read_only)
{
    auto makeoptions = [this](int q) -> NameValueOptions {
        const QString prefix = strnum("q", q, "_a_");
        NameValueOptions options{
            {xstring(prefix + "never"), NEVER},
        };
        if (!NO_SOMETIMES_QUESTIONS.contains(q)) {
            options.append(NameValuePair(xstring(prefix + "sometimes"),
                                          SOMETIMES));
        }
        options.append(NameValuePair(xstring(prefix + "always"),
                                      ALWAYS));
        if (NA_QUESTIONS.contains(q)) {
            if (SPECIAL_NA_TEXT_QUESTIONS.contains(q)) {
                options.append(NameValuePair(xstring(prefix + "na"), NA));
            } else {
                options.append(NameValuePair(TextConst::na(), NA));
            }
        }
        return options;
    };
    auto makeqelements = [this, &makeoptions](int q) -> QVector<QuElement*> {
        NameValueOptions options = makeoptions(q);
        return QVector<QuElement*>{
            (new QuText(xstring(strnum("q", q, "_q"))))->setBold(true),
            new QuText(xstring(strnum("q", q, "_detail"))),
            new QuMcq(fieldRef(strnum(QPREFIX, q)), options),
        };
    };
    auto makeqgroup = [&makeqelements](int start,
                                       int end) -> QVector<QuElement*> {
        QVector<QuElement*> elements;
        for (int q = start; q <= end; ++q) {
            elements += makeqelements(q);
        }
        return elements;
    };
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };

    QVector<QuPagePtr> pages{
        getClinicianAndRespondentDetailsPage(false),

        QuPagePtr((new QuPage(makeqgroup(1, 7)))
                  ->setTitle(xstring("h_behaviour"))),

        QuPagePtr((new QuPage(makeqgroup(8, 9)))
                  ->setTitle(xstring("h_outing"))),

        QuPagePtr((new QuPage(QVector<QuElement*>{
            text("houshold_instruction"),
        } + makeqgroup(10, 12)))
                  ->setTitle(xstring("h_household"))),

        QuPagePtr((new QuPage(QVector<QuElement*>{
            text("finances_instruction_1"),
            text("finances_instruction_2"),
        } + makeqgroup(13, 16)))
                  ->setTitle(xstring("h_finances"))),

        QuPagePtr((new QuPage(QVector<QuElement*>{
            text("medications_instruction"),
        } + makeqgroup(17, 18)))
                  ->setTitle(xstring("h_medications"))),

        QuPagePtr((new QuPage(QVector<QuElement*>{
            text("mealprep_instruction"),
        } + makeqgroup(19, 26)))
                  ->setTitle(xstring("h_mealprep"))),

        QuPagePtr((new QuPage(QVector<QuElement*>{
            text("selfcare_instruction"),
        } + makeqgroup(27, 30)))
                  ->setTitle(xstring("h_selfcare"))),

        QuPagePtr((new QuPage{
            new QuText(TextConst::cliniciansComments()),
            new QuTextEdit(fieldRef(COMMENTS, false)),
        })
                  ->setTitle(TextConst::comments())),
    };

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

Frs::ScoreInfo Frs::getScore() const
{
    ScoreInfo si;
    for (int q = FIRST_Q; q <= N_QUESTIONS; ++q) {
        const QVariant v = value(strnum(QPREFIX, q));
        if (!v.isNull() && v.toInt() != NA) {
            ++si.n;
            si.total += SCORE[v.toInt()];
        }
    }
    if (si.n > 0) {
        si.score = double(si.total) / double(si.n);
        si.logit = getTabularLogit(si.score.toDouble());
        si.severity = getSeverity(si.logit);
    }
    return si;
}


QVariant Frs::getTabularLogit(const double score) const
{
    const double pct_score = 100 * score;
    for (auto a_b_result : TABULAR_LOGIT_RANGES) {
        const QPair<double, double>& a_b = a_b_result.first;
        const double& result = a_b_result.second;
        const double& a = a_b.first;
        const double& b = a_b.second;
        if (a <= pct_score && pct_score < b) {
            return result;
        }
    }
    return QVariant();
}


QString Frs::getSeverity(const QVariant& logit) const
{
    // p1593 of Mioshi et al. (2010)
    // Copes with infinity comparisons
    if (logit.isNull()) {
        return "?";
    }
    const double l = logit.toDouble();
    if (l >= 4.12) {
        return "very mild";
    }
    if (l >= 1.92) {
        return "mild";
    }
    if (l >= -0.40) {
        return "moderate";
    }
    if (l >= -2.58) {
        return "severe";
    }
    if (l >= -4.99) {
        return "very severe";
    }
    return "profound";
}
