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

#include "setmenufromlp.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/ace3.h"
#include "tasks/auditc.h"
#include "tasks/bmi.h"
#include "tasks/cgii.h"
#include "tasks/core10.h"
#include "tasks/epds.h"
#include "tasks/eq5d5l.h"
#include "tasks/fft.h"
#include "tasks/gad7.h"
#include "tasks/honos.h"
#include "tasks/irac.h"
#include "tasks/patientsatisfaction.h"
#include "tasks/phq9.h"
#include "tasks/referrersatisfactiongen.h"
#include "tasks/referrersatisfactionspec.h"


SetMenuFromLp::SetMenuFromLp(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_SETS_CLINICAL))
{
}


QString SetMenuFromLp::title() const
{
    return "FROM-LP";
}


QString SetMenuFromLp::subtitle() const
{
    return tr("RCPsych Framework for Routine Outcome Measurement in "
              "Liaison Psychiatry (FROM-LP)");
}


void SetMenuFromLp::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),

        MenuItem(tr("GENERIC SCALES")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(CgiI::CGI_I_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Core10::CORE10_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Fft::FFT_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Irac::IRAC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(PatientSatisfaction::PT_SATIS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(ReferrerSatisfactionGen::REF_SATIS_GEN_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(ReferrerSatisfactionSpec::REF_SATIS_SPEC_TABLENAME, m_app),

        MenuItem(tr("DISEASE-SPECIFIC SCALES")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(Ace3::ACE3_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(AuditC::AUDITC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Bmi::BMI_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Epds::EPDS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Gad7::GAD7_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Honos::HONOS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Phq9::PHQ9_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Eq5d5l::EQ5D5L_TABLENAME, m_app),
    };
}
