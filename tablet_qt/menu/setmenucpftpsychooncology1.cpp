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

#include "setmenucpftpsychooncology1.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/cesd.h"
#include "tasks/cesdr.h"
#include "tasks/cgii.h"
#include "tasks/copebrief.h"
#include "tasks/eq5d5l.h"
#include "tasks/factg.h"
#include "tasks/gad7.h"
#include "tasks/hads.h"
#include "tasks/pcl5.h"
#include "tasks/phq9.h"
#include "tasks/wsas.h"


SetMenuCpftPsychooncology1::SetMenuCpftPsychooncology1(CamcopsApp& app) :
    MenuWindow(app,
               tr("CPFT Psycho-oncology Service"),
               uifunc::iconFilename(uiconst::ICON_SETS_CLINICAL))
{
    m_subtitle = "Cambridgeshire and Peterborough NHS Foundation Trust, UK — "
                 "psycho-oncology service";
    // See e-mail from Ruaidhri McCormack to Rudolf Cardinal, 2018-07-12.
    m_items = {
        MAKE_CHANGE_PATIENT(app),
        MAKE_TASK_MENU_ITEM(Cesd::CESD_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Cesdr::CESDR_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(CgiI::CGI_I_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(CopeBrief::COPEBRIEF_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Eq5d5l::EQ5D5L_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Factg::FACTG_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Gad7::GAD7_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Hads::HADS_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Phq9::PHQ9_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Pcl5::PCL5_TABLENAME, app),
        MAKE_TASK_MENU_ITEM(Wsas::WSAS_TABLENAME, app),
    };
}
