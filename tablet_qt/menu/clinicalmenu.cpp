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

#include "clinicalmenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/bmi.h"
#include "tasks/contactlog.h"
#include "tasks/cpftlpsdischarge.h"
#include "tasks/cpftlpsreferral.h"
#include "tasks/cpftlpsresetresponseclock.h"
#include "tasks/diagnosisicd10.h"
#include "tasks/gbogras.h"
#include "tasks/gbogres.h"
#include "tasks/gbogpc.h"
#include "tasks/irac.h"
#include "tasks/photo.h"
#include "tasks/photosequence.h"
#include "tasks/progressnote.h"
#include "tasks/psychiatricclerking.h"


ClinicalMenu::ClinicalMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_CLINICAL))
{
}


QString ClinicalMenu::title() const
{
    return tr("Clinical notes and logs");
}


void ClinicalMenu::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MAKE_TASK_MENU_ITEM(Bmi::BMI_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(ContactLog::CONTACTLOG_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(CPFTLPSReferral::CPFTLPSREFERRAL_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(CPFTLPSResetResponseClock::CPFTLPSRESETCLOCK_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(CPFTLPSDischarge::CPFTLPSDISCHARGE_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(DiagnosisIcd10::DIAGNOSIS_ICD10_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(GboGReS::GBOGRES_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(GboGPC::GBOGPC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(GboGRaS::GBOGRAS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Irac::IRAC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Photo::PHOTO_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(PhotoSequence::PHOTOSEQUENCE_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(ProgressNote::PROGNOTE_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(PsychiatricClerking::PSYCLERK_TABLENAME, m_app),
    };
}
