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

#include "physicalillnessmenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/asdas.h"
#include "tasks/das28.h"
#include "tasks/elixhauserci.h"
#include "tasks/esspri.h"
#include "tasks/sfmpq2.h"


PhysicalIllnessMenu::PhysicalIllnessMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_PHYSICAL))
{
}


QString PhysicalIllnessMenu::title() const
{
    return tr("Physical illness measurement");
}


void PhysicalIllnessMenu::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MAKE_TASK_MENU_ITEM(ElixhauserCI::ELIXHAUSERCI_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Esspri::ESSPRI_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Asdas::ASDAS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Sfmpq2::SFMPQ2_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Das28::DAS28_TABLENAME, m_app),
    };
}
