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

#include "cgi.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::scorePhrase;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;

const QString Cgi::CGI_TABLENAME("cgi");

const QString Q1("q1");
const QString Q2("q2");
const QString Q3T("q3t");
const QString Q3S("q3s");
const QString Q3("q3");

const int MAX_SCORE_TOTAL = 30;
const int MAX_SCORE_Q1_Q2 = 7;
const int MAX_SCORE_Q3 = 16;


void initializeCgi(TaskFactory& factory)
{
    static TaskRegistrar<Cgi> registered(factory);
}


Cgi::Cgi(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CGI_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(Q1, QVariant::Int);
    addField(Q2, QVariant::Int);
    addField(Q3T, QVariant::Int);
    addField(Q3S, QVariant::Int);
    addField(Q3, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Cgi::shortname() const
{
    return "CGI";
}


QString Cgi::longname() const
{
    return tr("Clinical Global Impressions");
}


QString Cgi::description() const
{
    return tr("Clinician-administered; briefly rates illness severity, global "
              "improvement, and efficacy/side-effect balance of treatment.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Cgi::isComplete() const
{
    return noneNull(values(QStringList{Q1, Q2, Q3T, Q3S}));
}


QStringList Cgi::summary() const
{
    return QStringList{
        totalScorePhrase(totalScore(), MAX_SCORE_TOTAL),
        scorePhrase(xstring("severity"), valueInt(Q1), MAX_SCORE_Q1_Q2),
        scorePhrase(xstring("improvement"), valueInt(Q2), MAX_SCORE_Q1_Q2),
        scorePhrase(xstring("efficacy"), valueInt(Q3), MAX_SCORE_Q3),
    };
}


QStringList Cgi::detail() const
{
    QStringList lines = completenessInfo();
    const QString separator(" ");
    lines.append(fieldSummary(Q1, xstring("q1_s"), separator));
    lines.append(fieldSummary(Q2, xstring("q2_s"), separator));
    lines.append(fieldSummary(Q3T, xstring("q3t_s"), separator));
    lines.append(fieldSummary(Q3S, xstring("q3s_s"), separator));
    lines.append(fieldSummary(Q3, xstring("q3_s"), separator));
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Cgi::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto addpage = [this, &pages](const QString& fieldname,
                                  int lastoption, bool update_q3) -> void {
        NameValueOptions options;
        for (int i = 0; i <= lastoption; ++i) {
            QString name = xstring(QString("%1_option%2").arg(fieldname).arg(i));
            options.append(NameValuePair(name, i));
        }
        const QString pagetitle = xstring(QString("%1_title").arg(fieldname));
        const QString question = xstring(QString("%1_question").arg(fieldname));
        FieldRefPtr fr = fieldRef(fieldname);
        if (update_q3) {
            connect(fr.data(), &FieldRef::valueChanged,
                    this, &Cgi::setEfficacyIndex);
        }
        QuPagePtr page((new QuPage{
            new QuText(question),
            new QuMcq(fr, options),
        })->setTitle(pagetitle));
        pages.append(page);
    };

    pages.append(getClinicianDetailsPage());
    addpage(Q1, 7, false);
    addpage(Q2, 7, false);
    addpage(Q3T, 4, true);
    addpage(Q3S, 4, true);

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Cgi::totalScore() const
{
    return sumInt(values(QStringList{Q1, Q2, Q3}));
}


void Cgi::setEfficacyIndex()
{
    if (valueIsNull(Q3T) ||
            valueInt(Q3T) <= 0 ||
            valueInt(Q3T) > 4 ||
            valueIsNull(Q3S) ||
            valueInt(Q3S) <= 0 ||
            valueInt(Q3S) > 4) {
        setValue(Q3, 0);  // not assessed, or silly values
    } else {
        setValue(Q3, (valueInt(Q3T) - 1) * 4 + valueInt(Q3S));
        // that's the CGI algorithm for the efficacy index
    }
}
