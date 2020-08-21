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

#pragma once

#include <QObject>
#include <QStringList>
#include "common/aliases_camcops.h"

class CamcopsApp;


class TaskChain : public QObject
{
    Q_OBJECT

    // ------------------------------------------------------------------------
    // Enums
    // ------------------------------------------------------------------------

public:
    // How should we upload?
    enum class CreationMethod {
        OnDemand,
        // ... Create tasks when they are first edited.
        //
        // - Editing times will be correct.
        // - If a chain is aborted early, there will be no incomplete instances
        //   of tasks that weren't started.
        //
        //   - Good because: easy to see what's left to finish.
        //   - Bad because: can give lots of incomplete tasks.

        AtStart,
        // ... Create all tasks when the chain starts.
        //
        // - Editing times will be wrong for all except the first task.
        // - If a chain is aborted early, there will be incomplete instances
        //   of tasks that weren't started.
        // - Probably a poor choice.

        OnDemandOrAbort
        // ... Create all tasks when they are first edited, but if the chain
        // is aborted, create all tasks not yet created.
        //
        // - Editing times will be correct.
        // - If a chain is aborted early, there will be incomplete instances
        //   of tasks that weren't started.
        //
        //   - Good because: no superfluous incomplete tasks.
        //   - Bad because: harder to see what's left to finish.
    };

    // ------------------------------------------------------------------------
    // TaskChain
    // ------------------------------------------------------------------------

public:
    // Create the chain definition
    TaskChain(CamcopsApp& app,
              const QStringList& task_tablenames,
              CreationMethod creation_method = CreationMethod::OnDemandOrAbort,
              const QString& title = "",
              const QString& subtitle = "");

    // Title/subtitle, for menus
    QString title() const;
    QString subtitle() const;

    // Description
    QString description(bool longname = false) const;

    // Number of tasks in the chain
    int nTasks() const;

    // Does this chain contain at least one non-anonymous task?
    bool needsPatient() const;

    // Is the chain permissible?
    bool permissible(QStringList& failure_reasons) const;

    // How tasks should be created
    CreationMethod creationMethod() const;

    // Start a chain and manage it
    void start();

protected:
    // Create a specific task
    void ensureTaskCreated(int index);

    // Create all tasks not yet created
    void ensureAllTasksCreated();

    // Fetch a pointer to a specific task.
    TaskPtr getTask(int index);

    // Start the next task in the sequence
    void startNextTask();

    // Not sure this is necessary.
    void onAllTasksFinished();

protected slots:
    // A task has been aborted.
    void onTaskAborted();

    // A task has finished.
    void onTaskFinished();

    // The task has asked the app to close its window and the app has done
    // the necessary.
    void onAppSubWindowClosed();

protected:
    CamcopsApp& m_app;  // our app
    QStringList m_task_tablenames;  // tasks that are part of the chain
    CreationMethod m_creation_method;  // when to create each task
    QString m_title;  // non-default title
    QString m_subtitle;  // non-default subtitle
    int m_current_task_index;
    QMap<int, TaskPtr> m_tasks;
    bool m_proceed_when_app_has_closed_last_task;
};
