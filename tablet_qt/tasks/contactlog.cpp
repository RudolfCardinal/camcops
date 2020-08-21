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

#include "contactlog.h"
#include "common/textconst.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using uifunc::yesNoNull;

const QString ContactLog::CONTACTLOG_TABLENAME("contactlog");

const QString LOCATION("location");  // ... location
const QString START("start");
const QString END("end");
const QString PATIENT_CONTACT("patient_contact");
const QString STAFF_LIAISON("staff_liaison");
const QString OTHER_LIAISON("other_liaison");
const QString COMMENT("comment");


void initializeContactLog(TaskFactory& factory)
{
    static TaskRegistrar<ContactLog> registered(factory);
}


ContactLog::ContactLog(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CONTACTLOG_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(LOCATION, QVariant::String);
    addField(START, QVariant::DateTime);
    addField(END, QVariant::DateTime);
    addField(PATIENT_CONTACT, QVariant::Bool);
    addField(STAFF_LIAISON, QVariant::Bool);
    addField(OTHER_LIAISON, QVariant::Bool);
    addField(COMMENT, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString ContactLog::shortname() const
{
    return "Contact";
}


QString ContactLog::longname() const
{
    return tr("Contact log");
}


QString ContactLog::description() const
{
    return tr("Record of clinical contact with times.");
}


QString ContactLog::infoFilenameStem() const
{
    return "clinical";
}


// ============================================================================
// Instance info
// ============================================================================

bool ContactLog::isComplete() const
{
    return noneNull(values(QStringList{CLINICIAN_NAME,
                                       START,
                                       END,
                                       PATIENT_CONTACT,
                                       STAFF_LIAISON,
                                       OTHER_LIAISON}));
}


QStringList ContactLog::summary() const
{
    return QStringList{
        QString("%1: <b>%2</b>.").arg(xstring("start"),
                                      datetime::textDateTime(value(START))),
        QString("%1: <b>%2</b>.").arg(xstring("end"),
                                      datetime::textDateTime(value(END))),
        QString("%1: <b>%2</b> %3.")
                .arg(xstring("time_taken"))
                .arg(timeTakenMinutes())
                .arg(xstring("minutes")),
    };
}


QStringList ContactLog::detail() const
{
    QStringList lines = completenessInfo();
    auto add = [&lines](const QString& desc, const QString& value) {
        lines.append(QString("%1: <b>%2</b>.").arg(desc, value));
    };
    add(TextConst::clinicianName(), prettyValue(CLINICIAN_NAME));
    add(xstring("location"), prettyValue(LOCATION));
    add(xstring("patient_contact"), yesNoNull(value(PATIENT_CONTACT)));
    add(xstring("staff_liaison"), yesNoNull(value(STAFF_LIAISON)));
    add(xstring("other_liaison"), yesNoNull(value(OTHER_LIAISON)));
    add(xstring("comment"), prettyValue(COMMENT));
    lines += summary();
    return lines;
}


OpenableWidget* ContactLog::editor(const bool read_only)
{
    QuPagePtr page((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        new QuText(xstring("location")),
        new QuLineEdit(fieldRef(LOCATION, false)),
        new QuText(xstring("comment")),
        new QuTextEdit(fieldRef(COMMENT, false)),
        new QuText(xstring("start")),
        (new QuDateTime(fieldRef(START)))->setOfferNowButton(true),
        new QuText(xstring("end")),
        (new QuDateTime(fieldRef(END)))->setOfferNowButton(true),
        new QuBoolean(xstring("patient_contact"), fieldRef(PATIENT_CONTACT)),
        new QuBoolean(xstring("staff_liaison"), fieldRef(STAFF_LIAISON)),
        new QuBoolean(xstring("other_liaison"), fieldRef(OTHER_LIAISON)),
    })->setTitle(xstring("title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int ContactLog::timeTakenMinutes() const
{
    const QVariant start = value(START);
    const QVariant end = value(END);
    if (start.isNull() || end.isNull()) {
        return 0;
    }
    return static_cast<int>(start.toDateTime().secsTo(end.toDateTime()) / 60);
}
