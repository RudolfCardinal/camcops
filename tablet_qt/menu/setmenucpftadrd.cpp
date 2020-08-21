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

#include "setmenucpftadrd.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/diagnosisicd9cm.h"
#include "tasks/gad7.h"
#include "tasks/hamd.h"
#include "tasks/iesr.h"
#include "tasks/pdss.h"
#include "tasks/phq9.h"
#include "tasks/pswq.h"
#include "tasks/swemwbs.h"
#include "tasks/wsas.h"
#include "tasks/ybocs.h"
#include "tasks/ybocssc.h"


SetMenuCpftADRD::SetMenuCpftADRD(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_SETS_CLINICAL))
{
}


QString SetMenuCpftADRD::title() const
{
    return tr("CPFT Affective Disorders Research Database");
}


QString SetMenuCpftADRD::subtitle() const
{
    return tr("Cambridgeshire and Peterborough NHS Foundation Trust, UK — "
              "affective disorders");
}


void SetMenuCpftADRD::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MAKE_TASK_MENU_ITEM(DiagnosisIcd9CM::DIAGNOSIS_ICD9CM_TABLENAME, m_app),  // = DSM-IV
        MAKE_TASK_MENU_ITEM(Gad7::GAD7_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(HamD::HAMD_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Iesr::IESR_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Pdss::PDSS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Phq9::PHQ9_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Pswq::PSWQ_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Swemwbs::SWEMWBS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Wsas::WSAS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Ybocs::YBOCS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(YbocsSc::YBOCSSC_TABLENAME, m_app),
    };
}
