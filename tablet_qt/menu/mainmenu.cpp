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

#include "mainmenu.h"
#include <QDebug>
#include <QSharedPointer>
#include "common/uiconst.h"
#include "core/networkmanager.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"
#include "menulib/menuproxy.h"

#include "menu/addictionmenu.h"
#include "menu/affectivemenu.h"
#include "menu/alltasksmenu.h"
#include "menu/anonymousmenu.h"
#include "menu/catatoniaepsemenu.h"
#include "menu/clinicalmenu.h"
#include "menu/clinicalsetsmenu.h"
#include "menu/cognitivemenu.h"
#include "menu/executivemenu.h"
#include "menu/globalmenu.h"
#include "menu/helpmenu.h"
#include "menu/patientsummarymenu.h"
#include "menu/personalitymenu.h"
#include "menu/physicalillnessmenu.h"
#include "menu/psychosismenu.h"
#include "menu/researchmenu.h"
#include "menu/researchsetsmenu.h"
#include "menu/serviceevaluationmenu.h"
#include "menu/settingsmenu.h"


MainMenu::MainMenu(CamcopsApp& app)
    : MenuWindow(
          app,
          uifunc::iconFilename(uiconst::ICON_CAMCOPS),
          true)
{
}


QString MainMenu::title() const
{
    return tr("CamCOPS: Cambridge Cognitive and Psychiatric Assessment Kit");
}


void MainMenu::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MAKE_MENU_MENU_ITEM(PatientSummaryMenu, m_app),
        MenuItem(
            tr("Upload data to server"),
            std::bind(&MainMenu::upload, this),
            uifunc::iconFilename(uiconst::ICON_UPLOAD)
        ).setNotIfLocked(),
        MAKE_MENU_MENU_ITEM(HelpMenu, m_app),
        MAKE_MENU_MENU_ITEM(SettingsMenu, m_app),

        MenuItem(tr("Tasks by type")).setLabelOnly(),
        MAKE_MENU_MENU_ITEM(ClinicalMenu, m_app),
        MAKE_MENU_MENU_ITEM(GlobalMenu, m_app),
        MAKE_MENU_MENU_ITEM(CognitiveMenu, m_app),
        MAKE_MENU_MENU_ITEM(AffectiveMenu, m_app),
        MAKE_MENU_MENU_ITEM(AddictionMenu, m_app),
        MAKE_MENU_MENU_ITEM(PsychosisMenu, m_app),
        MAKE_MENU_MENU_ITEM(CatatoniaEpseMenu, m_app),
        MAKE_MENU_MENU_ITEM(PersonalityMenu, m_app),
        MAKE_MENU_MENU_ITEM(ExecutiveMenu, m_app),
        MAKE_MENU_MENU_ITEM(PhysicalIllnessMenu, m_app),
        MAKE_MENU_MENU_ITEM(ServiceEvaluationMenu, m_app),
        MAKE_MENU_MENU_ITEM(ResearchMenu, m_app),
        MAKE_MENU_MENU_ITEM(AnonymousMenu, m_app),

        MenuItem(tr("Task collections")).setLabelOnly(),
        MAKE_MENU_MENU_ITEM(ClinicalSetsMenu, m_app),
        MAKE_MENU_MENU_ITEM(ResearchSetsMenu, m_app),
        MAKE_MENU_MENU_ITEM(AllTasksMenu, m_app),
    };
    connect(&m_app, &CamcopsApp::fontSizeChanged,
            this, &MainMenu::reloadStyleSheet);
}


void MainMenu::upload()
{
    m_app.upload();
}
