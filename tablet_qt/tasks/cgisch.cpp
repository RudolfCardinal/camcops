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

#include "cgisch.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::strnum;
using stringfunc::strseq;

const QString CgiSch::CGISCH_TABLENAME("cgisch");

const QString FP_SEVERITY("severity");
const QString FP_CHANGE("change");

const int NQ_PER_SECTION = 5;


void initializeCgiSch(TaskFactory& factory)
{
    static TaskRegistrar<CgiSch> registered(factory);
}


CgiSch::CgiSch(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CGISCH_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(FP_SEVERITY, 1, NQ_PER_SECTION), QVariant::Int);
    addFields(strseq(FP_CHANGE, 1, NQ_PER_SECTION), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CgiSch::shortname() const
{
    return "CGI-SCH";
}


QString CgiSch::longname() const
{
    return tr("Clinical Global Impression – Schizophrenia");
}


QString CgiSch::description() const
{
    return tr("Clinician-administered; briefly rates illness severity and "
              "degree of change in four domains and overall.");
}


// ============================================================================
// Instance info
// ============================================================================

bool CgiSch::isComplete() const
{
    return noneNull(values(strseq(FP_SEVERITY, 1, NQ_PER_SECTION))) &&
            noneNull(values(strseq(FP_CHANGE, 1, NQ_PER_SECTION)));
}


QStringList CgiSch::summary() const
{
    const QString sep(": ");
    const QString suffix(".");
    return QStringList{
        fieldSummary(strnum(FP_SEVERITY, 5), xstring("summary_i_5"), sep, suffix),
        fieldSummary(strnum(FP_CHANGE, 5), xstring("summary_ii_5"), sep, suffix),
    };
}


QStringList CgiSch::detail() const
{
    const QString sep(": ");
    QStringList lines = completenessInfo();
    lines.append(xstring("i_title"));
    lines.append("");
    for (int i = 1; i <= NQ_PER_SECTION; ++i) {
        lines.append(fieldSummary(strnum(FP_SEVERITY, i),
                                  xstring(strnum("q", i), sep)));
    }
    lines.append("");
    lines.append(xstring("ii_title"));
    lines.append("");
    for (int i = 1; i <= NQ_PER_SECTION; ++i) {
        lines.append(fieldSummary(strnum(FP_CHANGE, i),
                                  xstring(strnum("q", i), sep)));
    }
    return lines;
}


OpenableWidget* CgiSch::editor(const bool read_only)
{
    NameValueOptions severity_options;
    NameValueOptions change_options;
    for (int i = 1; i <= 7; ++i) {
        severity_options.append(NameValuePair(xstring(strnum("i_option", i)),
                                               i));
        change_options.append(NameValuePair(xstring(strnum("ii_option", i)),
                                             i));
    }
    change_options.append(NameValuePair(xstring("ii_option9"), 9));
    QVector<QuestionWithOneField> severity_qfields;
    QVector<QuestionWithOneField> change_qfields;
    for (int i = 1; i <= NQ_PER_SECTION; ++i) {
        const QString question = xstring(strnum("q", i));
        severity_qfields.append(QuestionWithOneField(
                                    question,
                                    fieldRef(strnum(FP_SEVERITY, i))));
        change_qfields.append(QuestionWithOneField(
                                    question,
                                    fieldRef(strnum(FP_CHANGE, i))));
    }

    QuPagePtr page1(getClinicianDetailsPage());

    QuPagePtr page2((new QuPage{
        (new QuText(xstring("i_question")))->setBold(),
        new QuMcqGrid(severity_qfields, severity_options),
    })->setTitle(xstring("i_title")));

    QuPagePtr page3((new QuPage{
        (new QuText(xstring("ii_question")))->setBold(),
        new QuMcqGrid(change_qfields, change_options),
        new QuText(xstring("ii_postscript"))
    })->setTitle(xstring("ii_title")));

    auto questionnaire = new Questionnaire(
                m_app, {page1, page2, page3});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}
