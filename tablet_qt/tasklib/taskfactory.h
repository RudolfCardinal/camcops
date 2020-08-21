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
#include <type_traits>  // for std::is_base_of
#include <QList>
#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include "common/dbconst.h"
#include "task.h"
#include "taskproxy.h"
#include "taskregistrar.h"

class CamcopsApp;
class Version;

// Two of the best articles on this sort of factory method in C++:
// - http://accu.org/index.php/journals/597
// - http://www.drdobbs.com/cpp/self-registering-objects-in-c/184410633?pgno=1
// Note that:
// - To do more than one thing, and to deal with classes in the abstract
//   without having to instantiate one, we use a proxy class.
// See also:
// - https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern

// ===========================================================================
// The factory does the work.
// ===========================================================================

class TaskFactory
{
    // Stores registered copies of TaskProxy*

public:
    using ProxyType = const TaskProxy*;

    // Class to hold information about a task type.
    class TaskCache
    {
    public:
        QString tablename;  // the task's base table name
        QString shortname;  // the task's short name
        QString longname;  // the task's long name
        QStringList alltables;  // all the task's table names
        bool anonymous;  // is the task anonymous
        ProxyType proxy;  // a TaskProxy* (q.v.)
    };

    using MapType = QMap<QString, TaskCache>;
    using MapIteratorType = QMapIterator<QString, TaskCache>;

    // Ways to sort tasks
    enum class TaskClassSortMethod {
        Tablename,
        Shortname,
        Longname,
    };

public:

    // ------------------------------------------------------------------------
    // Factory creation and task registration
    // ------------------------------------------------------------------------

    // Create the task factory. There will be only one of these.
    TaskFactory(CamcopsApp& app);

    // Register an individual task type.
    void registerTask(ProxyType proxy);

    // Call this when all tasks have been registered. This builds the task
    // cache.
    void finishRegistration();

    // ------------------------------------------------------------------------
    // Operations relating to the whole registry
    // ------------------------------------------------------------------------

    // Return all task base table names.
    QStringList tablenames(TaskClassSortMethod sort_method =
            TaskClassSortMethod::Tablename) const;

    // Returns all task table names (base + ancillary).
    QStringList allTablenames() const;

    // Create all tables in the database.
    void makeAllTables() const;

    // Upgrade the database from one version of the CamCOPS client to another.
    void upgradeDatabase(const Version& old_version,
                         const Version& new_version);

    // ------------------------------------------------------------------------
    // Operations relating to specific tasks
    // ------------------------------------------------------------------------

    // Create or load a task, given its base table name (key) and PK.
    TaskPtr create(const QString& key,
                   int load_pk = dbconst::NONEXISTENT_PK) const;

    // Return the shortname of a task, given its base table name (key).
    QString shortname(const QString& key) const;

    // Return the longname of a task, given its base table name (key).
    QString longname(const QString& key) const;

    // Create all tables for a given task (key = base table name).
    void makeTables(const QString& key) const;

    // Fetch all tasks, either for a single base table, or across all tasks
    // (if tablename == ""). A KEY SECURITY FUNCTION; determines which tasks
    // users can see according to whether the app has a patient selected and
    // whether it is locked, etc.
    TaskPtrList fetchTasks(const QString& tablename = "", bool sort = true) const;

    // Fetch all tasks for a specified patient.
    TaskPtrList fetchAllTasksForPatient(int patient_id) const;

    // Return a list containing a specimen (blank instance) of each task.
    TaskPtrList allSpecimens() const;

    // Return a list containing a specimen (blank instance) of each task,
    // except anonymous tasks.
    TaskPtrList allSpecimensExceptAnonymous() const;

    // Given a base or ancillary table name for a task, find the task, and
    // return its Task::minimumServerVersion().
    Version minimumServerVersion(const QString& tablename) const;  // main or sub-table

protected:
    CamcopsApp& m_app;  // our app
    QStringList m_tablenames;  // all task base table names
    QStringList m_all_tablenames;  // all task table names (base + ancillary)
    QVector<ProxyType> m_initial_proxy_list;  // holds proxies during initial registration
    MapType m_map;  // maps base table name to TaskCache
};
