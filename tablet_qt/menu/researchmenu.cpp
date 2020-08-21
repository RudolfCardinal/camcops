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

#include "researchmenu.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "dialogs/soundtestdialog.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/cardinalexpdetthreshold.h"
#include "tasks/cardinalexpectationdetection.h"
#include "tasks/chit.h"
#include "tasks/diagnosisicd9cm.h"
#include "tasks/ided3d.h"
#include "tasks/kirby.h"
#include "tasks/mfi20.h"
#include "tasks/qolbasic.h"
#include "tasks/qolsg.h"
#include "tasks/shaps.h"
#include "tasks/suppsp.h"
#include "taskxtra/cardinalexpdetcommon.h"


ResearchMenu::ResearchMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_RESEARCH))
{
}


QString ResearchMenu::title() const
{
    return tr("Research tasks");
}


void ResearchMenu::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MenuItem(tr("Well known or generic")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(Chit::CHIT_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(DiagnosisIcd9CM::DIAGNOSIS_ICD9CM_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(IDED3D::IDED3D_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Kirby::KIRBY_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(QolBasic::QOLBASIC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(QolSG::QOLSG_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Suppsp::SUPPSP_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Shaps::SHAPS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Mfi20::MFI20_TABLENAME, m_app),


        MenuItem(tr("Experimental")).setLabelOnly(),
        MenuItem(
            cardinalexpdetcommon::ExpDetTextConst::soundtestTitle(),
            std::bind(&ResearchMenu::soundTestCardinalExpDet, this),
            "",
            cardinalexpdetcommon::ExpDetTextConst::soundtestSubtitle()
        ),
        MAKE_TASK_MENU_ITEM(CardinalExpDetThreshold::CARDINALEXPDETTHRESHOLD_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(CardinalExpectationDetection::CARDINALEXPDET_TABLENAME, m_app),
    };
}


void ResearchMenu::soundTestCardinalExpDet()
{
    SoundTestDialog dlg(cardinalexpdetcommon::urlFromStem(
                            cardinalexpdetcommon::AUDITORY_BACKGROUND),
                        cardinalexpdetcommon::SOUNDTEST_VOLUME, this);
    dlg.exec();
}
