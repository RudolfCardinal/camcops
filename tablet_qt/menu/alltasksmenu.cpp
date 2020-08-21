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

#include "alltasksmenu.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"
#include "tasklib/taskfactory.h"


AllTasksMenu::AllTasksMenu(CamcopsApp& app) :
    MenuWindow(app,
               uifunc::iconFilename(uiconst::ICON_ALLTASKS),
               false,  // top
               true)  // offer_search
{
}


QString AllTasksMenu::title() const
{
    return tr("Search all tasks");
}


void AllTasksMenu::makeItems()
{
    TaskFactory* factory = m_app.taskFactory();
    // Sort by what you see:
    QStringList tablenames = factory->tablenames(
                TaskFactory::TaskClassSortMethod::Longname);
    for (const QString& tablename : tablenames) {
        m_items.append(MAKE_TASK_MENU_ITEM(tablename, m_app));
    }
}
