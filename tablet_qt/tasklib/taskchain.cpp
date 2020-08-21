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

#include <QDebug>
#include "taskchain.h"
#include "core/camcopsapp.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "menulib/menuwindow.h"
#include "questionnairelib/questionnaire.h"
#include "tasklib/taskfactory.h"


TaskChain::TaskChain(CamcopsApp& app,
                     const QStringList& task_tablenames,
                     CreationMethod creation_method,
                     const QString& title,
                     const QString& subtitle) :
    m_app(app),
    m_task_tablenames(task_tablenames),
    m_creation_method(creation_method),
    m_title(title),
    m_subtitle(subtitle),
    m_current_task_index(-1)
{
    QObject::connect(&m_app, &CamcopsApp::subWindowFinishedClosing,
                     this, &TaskChain::onAppSubWindowClosed);
}


QString TaskChain::title() const
{
    return m_title.isEmpty() ? tr("Task chain") : m_title;
}


QString TaskChain::subtitle() const
{
    return m_subtitle.isEmpty() ? description() : m_subtitle;
}


QString TaskChain::description(const bool longname) const
{
    QStringList tasknames;
    TaskFactory* factory = m_app.taskFactory();
    for (int i = 0; i < m_task_tablenames.length(); ++i) {
        const int pos = i + 1;
        const auto& tablename = m_task_tablenames[i];
        const QString taskname = longname ? factory->longname(tablename)
                                          : factory->shortname(tablename);
        tasknames.append(QString("%1. %2").arg(QString::number(pos),
                                               taskname));
    }
    return tasknames.join(" → ");
}


int TaskChain::nTasks() const
{
    return m_task_tablenames.length();
}


bool TaskChain::needsPatient() const
{
    TaskFactory* factory = m_app.taskFactory();
    for (const auto& tablename : m_task_tablenames) {
        TaskPtr specimen = factory->create(tablename);
        if (!specimen->isAnonymous()) {
            return true;
        }
    }
    return false;
}


bool TaskChain::permissible(QStringList& failure_reasons) const
{
    QString why_not_permissible;
    bool permissible = true;
    TaskFactory* factory = m_app.taskFactory();
    for (const auto& tablename : m_task_tablenames) {
        TaskPtr specimen = factory->create(tablename);
        if (!specimen->isTaskPermissible(why_not_permissible)) {
            const QString reason = QString("%1: %2")
                    .arg(specimen->shortname(),
                         stringfunc::bold(why_not_permissible));
            failure_reasons.append(reason);
            permissible = false;
        }
    }
    return permissible;
}


TaskChain::CreationMethod TaskChain::creationMethod() const
{
    return m_creation_method;
}


void TaskChain::ensureTaskCreated(const int index)
{
    if (index < 0 || index >= nTasks()) {
        return;
    }
    if (m_tasks.contains(index)) {
        // Already created
        return;
    }

    // Create the task
    TaskPtr task = m_app.taskFactory()->create(m_task_tablenames[index]);
    m_tasks[index] = task;

    // Set up the task
    // Compare SingleTaskMenu::addTask()
    const int patient_id = m_app.selectedPatientId();
    task->setupForEditingAndSave(patient_id);

    qDebug().nospace()
            << "Task chain created task " << index + 1
            << ": " << task->shortname();
}


void TaskChain::ensureAllTasksCreated()
{
    for (int i = 0; i < nTasks(); ++i) {
        ensureTaskCreated(i);
    }
}


TaskPtr TaskChain::getTask(const int index)
{
    if (index < 0 || index >= nTasks()) {
        return nullptr;
    }
    ensureTaskCreated(index);
    return m_tasks[index];
}


void TaskChain::start()
{
    // Pre-flight checks
    // Compare SingleTaskMenu::addTask()
    if (needsPatient() && !m_app.isPatientSelected()) {
        uifunc::alert(tr("No patient selected"));
        return;
    }
    QStringList failure_reasons;
    if (!permissible(failure_reasons)) {
        uifunc::alert(QString("%1<br><br>%2").arg(
                          tr("Task(s) not permissible:"),
                          failure_reasons.join("<br>")));
        return;
    }

    // Go
    m_current_task_index = -1;
    m_tasks.clear();
    if (m_creation_method == CreationMethod::AtStart) {
        ensureAllTasksCreated();
    }
    startNextTask();
}


void TaskChain::startNextTask()
{
    // Move to next task
    ++m_current_task_index;
    // All done?
    if (m_current_task_index >= nTasks()) {
        onAllTasksFinished();
        return;
    }

    // Create and configure the task
    TaskPtr task = getTask(m_current_task_index);
    OpenableWidget* widget = task->editor(false);
    if (!widget) {
        MenuWindow::complainTaskNotOfferingEditor();
        return;
    }
    Task* ptask = task.data();
    auto questionnaire = dynamic_cast<Questionnaire*>(widget);
    if (questionnaire) {
        questionnaire->setWithinChain(true);
    }
    MenuWindow::connectQuestionnaireToTask(widget, ptask);  // in case it's a questionnaire
    QObject::connect(ptask, &Task::editingFinished,
                     this, &TaskChain::onTaskFinished);
    QObject::connect(ptask, &Task::editingAborted,
                     this, &TaskChain::onTaskAborted);
    qDebug().nospace()
            << "Task chain launching task " << m_current_task_index + 1
            << ": " << ptask->shortname();

    // Launch the task
    m_proceed_when_app_has_closed_last_task = false;
    m_app.openSubWindow(widget, task, true);
}


void TaskChain::onAllTasksFinished()
{
    // Nothing needs doing.
}


void TaskChain::onTaskAborted()
{
    qDebug() << "Task chain: task was aborted";
    if (m_creation_method == CreationMethod::OnDemandOrAbort) {
        ensureAllTasksCreated();
    }
}


void TaskChain::onTaskFinished()
{
    qDebug() << "Task chain: task has finished successfully; waiting for app to close window";
    // Do not call startNextTask() yet.
    // The task's finishing signals will call the app's closeSubWindow(),
    // and we need that to finish first.
    m_proceed_when_app_has_closed_last_task = true;
}


void TaskChain::onAppSubWindowClosed()
{
    if (m_proceed_when_app_has_closed_last_task) {
        startNextTask();
    }
}
