/*
    Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).

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

#include "globalmenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/badls.h"
#include "tasks/bprs.h"
#include "tasks/bprse.h"
#include "tasks/cbir.h"
#include "tasks/cgi.h"
#include "tasks/cgii.h"
#include "tasks/copebrief.h"
#include "tasks/dad.h"
#include "tasks/demqol.h"
#include "tasks/demqolproxy.h"
#include "tasks/distressthermometer.h"
#include "tasks/eq5d5l.h"
#include "tasks/factg.h"
#include "tasks/frs.h"
#include "tasks/gaf.h"
#include "tasks/honos.h"
#include "tasks/honos65.h"
#include "tasks/honosca.h"
#include "tasks/npiq.h"
#include "tasks/ors.h"
#include "tasks/rand36.h"
#include "tasks/swemwbs.h"
#include "tasks/wemwbs.h"
#include "tasks/wsas.h"
#include "tasks/zbi12.h"


GlobalMenu::GlobalMenu(CamcopsApp& app) :
    MenuWindow(app,
               tr("Global function and multiple aspects of psychopathology"),
               uifunc::iconFilename(uiconst::ICON_GLOBAL))
{
    m_items = {
        MAKE_CHANGE_PATIENT(app),
        MAKE_TASK_MENU_ITEM(Badls::BADLS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Bprs::BPRS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(BprsE::BPRSE_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(CbiR::CBIR_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Cgi::CGI_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(CgiI::CGI_I_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(CopeBrief::COPEBRIEF_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Dad::DAD_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Demqol::DEMQOL_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(DemqolProxy::DEMQOLPROXY_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(DistressThermometer::DT_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Eq5d5l::EQ5D5L_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Factg::FACTG_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Frs::FRS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Gaf::GAF_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Honos::HONOS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Honos65::HONOS65_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Honosca::HONOSCA_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Ors::ORS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(NpiQ::NPIQ_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Rand36::RAND36_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Swemwbs::SWEMWBS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Wemwbs::WEMWBS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Wsas::WSAS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Zbi12::ZBI12_TABLENAME, app),
    };
}
