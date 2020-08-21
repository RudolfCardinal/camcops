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

#include "irac.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::standardResult;
using stringfunc::strnum;

const QString Irac::IRAC_TABLENAME("irac");

const QString AIM("aim");
const QString ACHIEVED("achieved");


void initializeIrac(TaskFactory& factory)
{
    static TaskRegistrar<Irac> registered(factory);
}


Irac::Irac(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, IRAC_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addField(AIM, QVariant::String);
    addField(ACHIEVED, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Irac::shortname() const
{
    return "IRAC";
}


QString Irac::longname() const
{
    return tr("Identify and Rate the Aim of the Contact");
}


QString Irac::description() const
{
    return tr("Clinician-specified aim of contact, and whether aim achieved.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Irac::isComplete() const
{
    return noneNull(values({AIM, ACHIEVED}));
}


QStringList Irac::summary() const
{
    return QStringList{
        fieldSummary(AIM, xstring("q_aim")) + ".",
        standardResult(xstring("q_achieved"), getAchievedText()),
    };
}


QStringList Irac::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Irac::editor(const bool read_only)
{
    NameValueOptions options_aim;
    for (int i = 1; i <= 10; ++i) {
        const QString s = xstring(strnum("aim_", i));
        options_aim.append(NameValuePair(s, s));
    }
    NameValueOptions options_achieved;
    for (int i = 0; i <= 2; ++i) {
        const QString s = xstring(strnum("achieved_", i));
        options_achieved.append(NameValuePair(s, i));
    }
    QuPagePtr page((new QuPage{
        (new QuText(xstring("q_aim")))->setBold(),
        new QuMcq(fieldRef(AIM), options_aim),
        (new QuText(xstring("q_achieved")))->setBold(),
        new QuMcq(fieldRef(ACHIEVED), options_achieved),
    })->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QString Irac::getAchievedText() const
{
    const QVariant v = value(ACHIEVED);
    const int i = v.toInt();
    if (v.isNull() || i < 0 || i > 2) {
        return "?";
    }
    return xstring(strnum("achieved_", i));
}
