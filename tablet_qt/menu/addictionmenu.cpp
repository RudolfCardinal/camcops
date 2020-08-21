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

#include "addictionmenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/audit.h"
#include "tasks/auditc.h"
#include "tasks/cage.h"
#include "tasks/ciwa.h"
#include "tasks/dast.h"
#include "tasks/fast.h"
#include "tasks/mast.h"
#include "tasks/smast.h"


AddictionMenu::AddictionMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_ADDICTION))
{
}


QString AddictionMenu::title() const
{
    return tr("Drug/alcohol abuse and addiction");
}


void AddictionMenu::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MAKE_TASK_MENU_ITEM(Audit::AUDIT_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(AuditC::AUDITC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Cage::CAGE_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Ciwa::CIWA_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Dast::DAST_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Fast::FAST_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Mast::MAST_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Smast::SMAST_TABLENAME, m_app),
    };
}
