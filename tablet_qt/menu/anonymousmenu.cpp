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

#include "anonymousmenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/apeqpt.h"
#include "tasks/gmcpq.h"
#include "tasks/perinatalpoem.h"
#include "tasks/referrersatisfactiongen.h"


AnonymousMenu::AnonymousMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_ANONYMOUS))
{
}


QString AnonymousMenu::title() const
{
    return tr("Anonymous questionnaires");
}


void AnonymousMenu::makeItems()
{
    m_items = {
        // Seems inappropriate: MAKE_CHANGE_PATIENT(m_app),
        MAKE_TASK_MENU_ITEM(Apeqpt::APEQPT_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(GmcPq::GMCPQ_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(PerinatalPoem::PERINATAL_POEM_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(ReferrerSatisfactionGen::REF_SATIS_GEN_TABLENAME, m_app),
    };
}
