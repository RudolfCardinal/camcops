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

#include "cgii.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using stringfunc::strnum;

const QString CgiI::CGI_I_TABLENAME("cgi_i");

const QString Q("q");


void initializeCgiI(TaskFactory& factory)
{
    static TaskRegistrar<CgiI> registered(factory);
}


CgiI::CgiI(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CGI_I_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(Q, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CgiI::shortname() const
{
    return "CGI-I";
}


QString CgiI::longname() const
{
    return tr("Clinical Global Impressions – Improvement subscale "
              "(FROM-LP version)");
}


QString CgiI::description() const
{
    return tr("Clinician-administered; briefly rates global improvement.");
}


QString CgiI::xstringTaskname() const
{
    return "cgi";
}


QString CgiI::infoFilenameStem() const
{
    return "cgi";
}


// ============================================================================
// Instance info
// ============================================================================

bool CgiI::isComplete() const
{
    return !valueIsNull(Q);
}


QStringList CgiI::summary() const
{
    return QStringList{getRatingText()};
}


QStringList CgiI::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* CgiI::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;
    pages.append(getClinicianDetailsPage());

    NameValueOptions options;
    for (int i = 1; i <= 7; ++i) {  // we don't use 0 (not assessed)
        const QString name = xstring(QString("q2_option%1").arg(i));
        options.append(NameValuePair(name, i));
    }
    pages.append(QuPagePtr((new QuPage{
        new QuText(xstring("i_q")),
        new QuMcq(fieldRef(Q), options),
    })->setTitle(shortname())));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QString CgiI::getRatingText() const
{
    const QVariant v = value(Q);
    if (v.isNull() || v.toInt() < 1 || v.toInt() > 7) {
        return "";
    }
    return xstring(strnum("q2_option", v.toInt()));
}
