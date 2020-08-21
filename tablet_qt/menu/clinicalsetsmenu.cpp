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

#include "clinicalsetsmenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "menu/setmenucpftadrd.h"
#include "menu/setmenucpftperinatal.h"
#include "menu/setmenucpftpsychooncology.h"
#include "menu/setmenufromlp.h"
#include "menu/setmenufromperinatal.h"
#include "menu/setmenuobrien.h"


ClinicalSetsMenu::ClinicalSetsMenu(CamcopsApp& app) :
    MenuWindow(app,
               uifunc::iconFilename(uiconst::ICON_SETS_CLINICAL))
{
}


QString ClinicalSetsMenu::title() const
{
    return tr("Sets of tasks collected together for clinical purposes");
}


void ClinicalSetsMenu::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MAKE_MENU_MENU_ITEM(SetMenuCpftADRD, m_app),
        MAKE_MENU_MENU_ITEM(SetMenuCpftPerinatal, m_app),
        MAKE_MENU_MENU_ITEM(SetMenuCpftPsychooncology, m_app),
        MAKE_MENU_MENU_ITEM(SetMenuFromLp, m_app),
        MAKE_MENU_MENU_ITEM(SetMenuFromPerinatal, m_app),
        MAKE_MENU_MENU_ITEM(SetMenuOBrien, m_app),
    };
}
