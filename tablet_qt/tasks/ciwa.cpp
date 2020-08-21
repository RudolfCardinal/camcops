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

#include "ciwa.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_SCORED_QUESTIONS = 10;
const int MAX_SCORE = 67;
const QString QPREFIX("q");

const QString Ciwa::CIWA_TABLENAME("ciwa");
const QString T("t");
const QString HR("hr");
const QString SBP("sbp");
const QString DBP("dbp");
const QString RR("rr");


void initializeCiwa(TaskFactory& factory)
{
    static TaskRegistrar<Ciwa> registered(factory);
}


Ciwa::Ciwa(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CIWA_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_SCORED_QUESTIONS), QVariant::Int);
    addField(T, QVariant::Double);  // previously int, which was wrong (is temp in degrees C)
    addField(HR, QVariant::Int);
    addField(SBP, QVariant::Int);
    addField(DBP, QVariant::Int);
    addField(RR, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Ciwa::shortname() const
{
    return "CIWA-Ar";
}


QString Ciwa::longname() const
{
    return tr("Clinical Institute Withdrawal Assessment for Alcohol Scale, "
              "Revised");
}


QString Ciwa::description() const
{
    return tr("10-item clinician-administered scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Ciwa::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_SCORED_QUESTIONS)));
}


QStringList Ciwa::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Ciwa::detail() const
{
    const int total_score = totalScore();
    const QString severity = severityDescription(total_score);
    const QString sep(": ");
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", " ",
                            QPREFIX, FIRST_Q, N_SCORED_QUESTIONS);
    lines.append("");
    lines.append(fieldSummary(T, xstring("t"), sep));
    lines.append(fieldSummary(HR, xstring("hr"), sep));
    lines.append(QString("%1: <b>%2/%3</b>").arg(xstring("bp"),
                                                 prettyValue(SBP),
                                                 prettyValue(DBP)));
    lines.append(fieldSummary(RR, xstring("rr"), sep));
    lines.append("");
    lines += summary();
    lines.append("");
    lines.append(xstring("severity") + " " + severity);
    return lines;
}


OpenableWidget* Ciwa::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto addpage = [this, &pages](int n, int lastoption) -> void {
        NameValueOptions options;
        for (int i = 0; i <= lastoption; ++i) {
            const QString name = xstring(QString("q%1_option%2").arg(n).arg(i));
            options.append(NameValuePair(name, i));
        }
        const QString pagetitle = xstring(QString("q%1_title").arg(n));
        const QString question = xstring(QString("q%1_question").arg(n));
        const QString fieldname = strnum(QPREFIX, n);
        QuPagePtr page((new QuPage{
            new QuText(question),
            new QuMcq(fieldRef(fieldname), options),
        })->setTitle(pagetitle));
        pages.append(page);
    };

    pages.append(getClinicianDetailsPage());

    for (int i = 1; i <= 9; ++i) {
        addpage(i, 7);
    }
    addpage(10, 4);

    pages.append(QuPagePtr((new QuPage{
        new QuText(xstring("vitals_question")),
        questionnairefunc::defaultGridRawPointer({
            {xstring("t"), new QuLineEditDouble(fieldRef(T, false), 0, 50, 2)},
            {xstring("hr"), new QuLineEditInteger(fieldRef(HR, false), 0, 400)},
            {xstring("sbp"), new QuLineEditInteger(fieldRef(SBP, false), 0, 400)},
            {xstring("dbp"), new QuLineEditInteger(fieldRef(DBP, false), 0, 400)},
            {xstring("rr"), new QuLineEditInteger(fieldRef(RR, false), 0, 400)},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
    })->setTitle(xstring("vitals_title"))));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Ciwa::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_SCORED_QUESTIONS)));
}


QString Ciwa::severityDescription(const int total_score) const
{
    if (total_score > 15) {
        return xstring("category_severe");
    }
    if (total_score >= 8) {
        return xstring("category_moderate");
    }
    return xstring("category_mild");
}
