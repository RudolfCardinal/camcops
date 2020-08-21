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

// #define DEBUG_TASK_MENU_CREATION

#include "singletaskmenu.h"
#include <QPushButton>
#include "common/uiconst.h"
#include "common/urlconst.h"
#include "dbobjects/patient.h"
#include "dialogs/scrollmessagebox.h"
#include "lib/uifunc.h"
#include "lib/stringfunc.h"
#include "menulib/menuheader.h"
#include "menulib/menuitem.h"
#include "tasklib/task.h"
#include "tasklib/taskfactory.h"


SingleTaskMenu::SingleTaskMenu(const QString& tablename, CamcopsApp& app) :
    MenuWindow(app, ""),  // start with a blank title
    m_tablename(tablename)
{
}


void SingleTaskMenu::extraLayoutCreation()
{
    // Title
    TaskFactory* factory = m_app.taskFactory();
    TaskPtr specimen = factory->create(m_tablename);
    m_anonymous = specimen->isAnonymous();
    if (m_anonymous) {
        setIcon(uifunc::iconFilename(uiconst::ICON_ANONYMOUS));
    }
    const bool crippled = specimen->isCrippled();
    m_p_header->setCrippled(crippled);

    // m_items is EXPENSIVE (and depends on security), so leave it to build()

    // Signals
    connect(&m_app, &CamcopsApp::selectedPatientChanged,
            this, &SingleTaskMenu::selectedPatientChanged,
            Qt::UniqueConnection);
    connect(&m_app, &CamcopsApp::taskAlterationFinished,
            this, &SingleTaskMenu::refreshTaskList,
            Qt::UniqueConnection);
    connect(&m_app, &CamcopsApp::lockStateChanged,
            this, &SingleTaskMenu::refreshTaskList,
            Qt::UniqueConnection);

    connect(m_p_header, &MenuHeader::addClicked,
            this, &SingleTaskMenu::addTask,
            Qt::UniqueConnection);
}


QString SingleTaskMenu::title() const
{
    TaskFactory* factory = m_app.taskFactory();
    TaskPtr specimen = factory->create(m_tablename);
    return specimen->menutitle();
}


void SingleTaskMenu::makeItems()
{
    TaskFactory* factory = m_app.taskFactory();
    TaskPtr specimen = factory->create(m_tablename);

    // Common items
    const QString info_icon_filename = uifunc::iconFilename(uiconst::ICON_INFO);
    m_items = {
        MenuItem(tr("Options")).setLabelOnly(),
    };
    if (!m_anonymous) {
        m_items.append(MAKE_CHANGE_PATIENT(m_app));
    }
    m_items.append(
        MenuItem(
            tr("Task information"),
            UrlMenuItem(
                urlconst::taskDocUrl(specimen->infoFilenameStem())
            ),
            info_icon_filename
        )
    );
    m_items.append(MenuItem(tr("Task status"),
                            std::bind(&SingleTaskMenu::showTaskStatus, this),
                            info_icon_filename));
    m_items.append(
        MenuItem(
            tr("Task instances") + ": " + specimen->menutitle()
        ).setLabelOnly()
    );

    // Task items
    TaskPtrList tasklist = factory->fetchTasks(m_tablename);
#ifdef DEBUG_TASK_MENU_CREATION
    qDebug() << Q_FUNC_INFO << "-" << tasklist.size() << "tasks";
#endif
    const bool show_patient_name = specimen->isAnonymous() ||
            !m_app.isPatientSelected();
    for (const TaskPtr& task : tasklist) {
        m_items.append(MenuItem(task, false, show_patient_name));
    }
}


void SingleTaskMenu::afterBuild()
{
    emit offerAdd(m_anonymous || m_app.isPatientSelected());
}


void SingleTaskMenu::addTask()
{
    // The task we create here needs to stay in scope for the duration of the
    // editing! The simplest way is to use a member object to hold the pointer.
    TaskFactory* factory = m_app.taskFactory();
    TaskPtr task = factory->create(m_tablename);
    QString why_not_permissible;
    QString why_not_uploadable;

    // Reasons we may say no
    if (!task->isTaskPermissible(why_not_permissible)) {
        const QString reason = QString("%1<br><br>%2: %3")
                .arg(tr("You cannot add this task with your current settings."),
                     tr("Current reason"),
                     stringfunc::bold(why_not_permissible));
        uifunc::alert(reason, tr("Not permitted to add task"));
        return;
    }
    const int patient_id = m_app.selectedPatientId();
    if (!task->isAnonymous()) {
        if (patient_id == dbconst::NONEXISTENT_PK) {
            qCritical() << Q_FUNC_INFO << "- no patient selected";
            return;
        }
    }

    // Reasons the user may want to pause
    if (!task->isTaskUploadable(why_not_uploadable)) {
        ScrollMessageBox msgbox(
                    QMessageBox::Warning,
                    tr("Really create?"),
                    tr("This task is not currently uploadable.") + "\n\n" +
                        why_not_uploadable + "\n\n" + tr("Create anyway?"),
                    this);
        QAbstractButton* yes = msgbox.addButton(tr("Yes, create"),
                                                QMessageBox::YesRole);
        msgbox.addButton(tr("No, cancel"), QMessageBox::NoRole);
        msgbox.exec();
        if (msgbox.clickedButton() != yes) {
            return;
        }
    }

    // OK; off we go!
    task->setupForEditingAndSave(patient_id);
    editTaskConfirmed(task);
}


void SingleTaskMenu::selectedPatientChanged(const Patient* patient)
{
    refreshTaskList();
    emit offerAdd(m_anonymous || patient);
}


void SingleTaskMenu::showTaskStatus() const
{
    TaskFactory* factory = m_app.taskFactory();
    TaskPtr specimen = factory->create(m_tablename);
    QStringList info;
    QString why_not_permissible;
    QString why_not_uploadable;
    auto add = [&info](const QString& desc, const QString& value) -> void {
        info.append(QString("%1: %2")
                    .arg(desc, stringfunc::bold(value)));
    };
    add(tr("Long name"), specimen->longname());
    add(tr("Short name"), specimen->shortname());
    add(tr("Main database table name"), specimen->tablename());
    add(tr("Implementation type"), specimen->implementationTypeDescription());
    add(tr("Anonymous"), uifunc::yesNo(specimen->isAnonymous()));
    add(tr("Has a clinician"), uifunc::yesNo(specimen->hasClinician()));
    add(tr("Has a respondent"), uifunc::yesNo(specimen->hasRespondent()));
    add(tr("Prohibits clinical use"), uifunc::yesNo(specimen->prohibitsClinical()));
    add(tr("Prohibits commercial use"), uifunc::yesNo(specimen->prohibitsCommercial()));
    add(tr("Prohibits educational use"), uifunc::yesNo(specimen->prohibitsEducational()));
    add(tr("Prohibits research use"), uifunc::yesNo(specimen->prohibitsResearch()));
    add(tr("Permissible (creatable) with current settings"), uifunc::yesNo(
            specimen->isTaskPermissible(why_not_permissible)));
    add(tr("If not, why not permissible"), why_not_permissible);
    add(tr("Uploadable to current server"), uifunc::yesNo(
            specimen->isTaskUploadable(why_not_uploadable)));
    add(tr("If not, why not uploadable"), why_not_uploadable);
    add(tr("Fully functional"), uifunc::yesNo(!specimen->isCrippled()));
    add(tr("Extra strings present from server"), uifunc::yesNo(
            specimen->hasExtraStrings()));
    add(tr("Editable once created"), uifunc::yesNo(specimen->isEditable()));
    uifunc::alert(info.join("<br>"), tr("Task status"));
}


void SingleTaskMenu::refreshTaskList()
{
    rebuild(false);  // no need to recreate hader
}
