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

#include "cpftlpsresetresponseclock.h"
#include "core/camcopsapp.h"
#include "common/uiconst.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNullOrEmpty;


const QString CPFTLPSResetResponseClock::CPFTLPSRESETCLOCK_TABLENAME("cpft_lps_resetresponseclock");

const QString RESET_START_TIME_TO("reset_start_time_to");
const QString REASON("reason");

const QString XSTRING_TO("to");
const QString XSTRING_REASON("reason");


void initializeCPFTLPSResetResponseClock(TaskFactory& factory)
{
    static TaskRegistrar<CPFTLPSResetResponseClock> registered(factory);
}


CPFTLPSResetResponseClock::CPFTLPSResetResponseClock(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CPFTLPSRESETCLOCK_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(RESET_START_TIME_TO, QVariant::DateTime);
    addField(REASON, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CPFTLPSResetResponseClock::shortname() const
{
    return "CPFT_LPS_ResetResponseClock";
}


QString CPFTLPSResetResponseClock::longname() const
{
    return tr("CPFT LPS – reset response clock");
}


QString CPFTLPSResetResponseClock::description() const
{
    return tr("Reset referral response clock "
              "(CPFT Liaison Psychiatry Service).");
}


QString CPFTLPSResetResponseClock::infoFilenameStem() const
{
    return "clinical";
}


QString CPFTLPSResetResponseClock::xstringTaskname() const
{
    return "cpft_lps_resetresponseclock";
}


// ============================================================================
// Instance info
// ============================================================================

bool CPFTLPSResetResponseClock::isComplete() const
{
    return noneNullOrEmpty(values(QStringList{RESET_START_TIME_TO,
                                              REASON}));
}


QStringList CPFTLPSResetResponseClock::summary() const
{
    return QStringList{
        QString("%1: <b>%2</b>.").arg(xstring(XSTRING_TO),
                                      datetime::textDateTime(value(RESET_START_TIME_TO))),
        QString("%1: <b>%2</b>.").arg(xstring(XSTRING_REASON),
                                      prettyValue(REASON)),
    };
}


QStringList CPFTLPSResetResponseClock::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* CPFTLPSResetResponseClock::editor(const bool read_only)
{
    QuPagePtr page((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        new QuText(xstring(XSTRING_TO)),
        (new QuDateTime(fieldRef(RESET_START_TIME_TO)))
                       ->setMode(QuDateTime::Mode::DefaultDateTime)
                       ->setOfferNowButton(true),
        new QuText(xstring(XSTRING_REASON)),
        new QuTextEdit(fieldRef(REASON)),
    })->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}
