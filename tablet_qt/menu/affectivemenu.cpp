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

#include "affectivemenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/bdi.h"
#include "tasks/cisr.h"
#include "tasks/cesd.h"
#include "tasks/cesdr.h"
#include "tasks/core10.h"
#include "tasks/epds.h"
#include "tasks/gad7.h"
#include "tasks/gds15.h"
#include "tasks/hads.h"
#include "tasks/hama.h"
#include "tasks/hamd.h"
#include "tasks/hamd7.h"
#include "tasks/icd10depressive.h"
#include "tasks/icd10manic.h"
#include "tasks/icd10mixed.h"
#include "tasks/iesr.h"
#include "tasks/pcl5.h"
#include "tasks/pclc.h"
#include "tasks/pclm.h"
#include "tasks/pcls.h"
#include "tasks/pdss.h"
#include "tasks/phq9.h"
#include "tasks/phq15.h"
#include "tasks/pswq.h"
#include "tasks/ybocs.h"
#include "tasks/ybocssc.h"


AffectiveMenu::AffectiveMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_AFFECTIVE))
{
}


QString AffectiveMenu::title() const
{
    return tr("Affective (mood and anxiety) disorders");
}


void AffectiveMenu::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        // PERMISSION REFUSED: MAKE_TASK_MENU_ITEM("asrm", m_app),
        MAKE_TASK_MENU_ITEM(Bdi::BDI_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Cesd::CESD_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Cesdr::CESDR_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Cisr::CISR_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Core10::CORE10_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Epds::EPDS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Gad7::GAD7_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Gds15::GDS15_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Hads::HADS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(HamA::HAMA_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(HamD::HAMD_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(HamD7::HAMD7_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Icd10Depressive::ICD10DEP_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Icd10Manic::ICD10MANIC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Icd10Mixed::ICD10MIXED_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Iesr::IESR_TABLENAME, m_app),
        // PERMISSION REFUSED: MAKE_TASK_MENU_ITEM("madrs", m_app),
        MAKE_TASK_MENU_ITEM(Pcl5::PCL5_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(PclM::PCLM_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(PclC::PCLC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(PclS::PCLS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Pdss::PDSS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Phq9::PHQ9_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Phq15::PHQ15_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Pswq::PSWQ_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Ybocs::YBOCS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(YbocsSc::YBOCSSC_TABLENAME, m_app),
    };
}
