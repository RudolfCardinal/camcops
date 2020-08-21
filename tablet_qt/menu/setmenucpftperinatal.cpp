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

#include "setmenucpftperinatal.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"

#include "tasks/apeqcpftperinatal.h"
// #include "tasks/apeqpt.h"
#include "tasks/core10.h"
#include "tasks/epds.h"
#include "tasks/gad7.h"
#include "tasks/gbogras.h"
#include "tasks/gbogres.h"
#include "tasks/gbogpc.h"
#include "tasks/honos.h"
#include "tasks/maas.h"
#include "tasks/pbq.h"
#include "tasks/perinatalpoem.h"
#include "tasks/phq9.h"
#include "tasks/ors.h"
#include "tasks/srs.h"


SetMenuCpftPerinatal::SetMenuCpftPerinatal(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_SETS_CLINICAL))
{
}


QString SetMenuCpftPerinatal::title() const
{
    return tr("CPFT Perinatal Service");
}


QString SetMenuCpftPerinatal::subtitle() const
{
    return tr("Cambridgeshire and Peterborough NHS Foundation Trust, UK — "
              "perinatal psychiatry service");
}


void SetMenuCpftPerinatal::makeItems()
{
    m_items = {
        MAKE_CHANGE_PATIENT(m_app),
        MenuItem(tr("Assessment/choice")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(APEQCPFTPerinatal::APEQCPFTPERINATAL_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Srs::SRS_TABLENAME, m_app),
        MenuItem(tr("Generic measures")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(Core10::CORE10_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Honos::HONOS_TABLENAME, m_app),
        MenuItem(tr("Mother–infant measures")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(Maas::MAAS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Pbq::PBQ_TABLENAME, m_app),
        MenuItem(tr("Specific conditions")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(Epds::EPDS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Phq9::PHQ9_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Gad7::GAD7_TABLENAME, m_app),
        MenuItem(tr("Treatment/care")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(GboGReS::GBOGRES_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(GboGPC::GBOGPC_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(GboGRaS::GBOGRAS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Ors::ORS_TABLENAME, m_app),
        MAKE_TASK_MENU_ITEM(Srs::SRS_TABLENAME, m_app),
        MenuItem(tr("End of treatment/care")).setLabelOnly(),
        MAKE_TASK_MENU_ITEM(PerinatalPoem::PERINATAL_POEM_TABLENAME, m_app),
    };
}
