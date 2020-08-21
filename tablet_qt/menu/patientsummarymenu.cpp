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

#include "patientsummarymenu.h"
#include "common/uiconst.h"
#include "dbobjects/patient.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"
#include "tasklib/taskfactory.h"


PatientSummaryMenu::PatientSummaryMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_PATIENT_SUMMARY))
{
}


void PatientSummaryMenu::extraLayoutCreation()
{
    // Signals
    connect(&m_app, &CamcopsApp::selectedPatientChanged,
            this, &PatientSummaryMenu::refreshTaskList,
            Qt::UniqueConnection);
    connect(&m_app, &CamcopsApp::taskAlterationFinished,
            this, &PatientSummaryMenu::refreshTaskList,
            Qt::UniqueConnection);
}


QString PatientSummaryMenu::title() const
{
    return tr("Patient summary");
}


void PatientSummaryMenu::makeItems()
{
    TaskFactory* factory = m_app.taskFactory();

    // Common items
    m_items = {
        MenuItem(tr("Options")).setLabelOnly(),
        MAKE_CHANGE_PATIENT(m_app),
        MenuItem(tr("Task instances")).setLabelOnly(),
    };

    // Task items
    const TaskPtrList tasklist = factory->fetchTasks();
    qDebug() << Q_FUNC_INFO << "-" << tasklist.size() << "task(s)";
    for (const TaskPtr& task : tasklist) {
        m_items.append(MenuItem(task, true, false));
    }
}


void PatientSummaryMenu::refreshTaskList()
{
    rebuild(false);  // no need to rebuild header
}
