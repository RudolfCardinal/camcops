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

// #define DEBUG_TASK_CREATION

#include "taskfactory.h"
#include <algorithm>
#include "core/camcopsapp.h"
#include "lib/comparers.h"
#include "lib/version.h"
#include "tasklib/task.h"
#include "tasklib/tasksorter.h"
#include "version/camcopsversion.h"


// ===========================================================================
// TaskFactory
// ===========================================================================

TaskFactory::TaskFactory(CamcopsApp& app) :
    m_app(app)
{
}


void TaskFactory::registerTask(ProxyType proxy)
{
    m_initial_proxy_list.append(proxy);
    // We are here from WITHIN A CONSTRUCTOR (TaskProxy::TaskProxy), so don't
    // call back to the proxy.
}


void TaskFactory::finishRegistration()
{
    for (const ProxyType& proxy : m_initial_proxy_list) {
        TaskPtr p_task = proxy->create(m_app, m_app.db());
        TaskCache cache;
        cache.tablename = p_task->tablename();
        cache.shortname = p_task->shortname();
        cache.longname = p_task->longname();
        cache.anonymous = p_task->isAnonymous();
        cache.alltables = p_task->allTables();
        cache.proxy = proxy;
        if (m_map.contains(cache.tablename)) {
            QString msg = QString(
                "BAD TASK REGISTRATION: table %1 being registered for a second"
                " time by task with longname %2").arg(
                    cache.tablename, cache.longname);
            qFatal("%s", qPrintable(msg));
        }
        m_map.insert(cache.tablename, cache);  // tablenames are the keys
        m_tablenames.append(cache.tablename);
        m_all_tablenames += cache.alltables;
    }
    m_tablenames.sort();
    m_all_tablenames.sort();
}


QStringList TaskFactory::tablenames(const TaskClassSortMethod sort_method) const
{
    if (sort_method == TaskClassSortMethod::Tablename) {
        // Already sorted by this
        return m_tablenames;
    }
    using StringPair = QPair<QString, QString>;
    QVector<StringPair> pairs;
    const bool use_shortname = sort_method == TaskClassSortMethod::Shortname;
    for (const QString& tablename : m_tablenames) {
        const TaskCache& cache = m_map[tablename];
        pairs.append(StringPair(tablename, use_shortname ? cache.shortname
                                                         : cache.longname));
    }
    std::sort(pairs.begin(), pairs.end(), QPairSecondComparer());
    QStringList sorted_tablenames;
    for (const StringPair& pair : pairs) {
        sorted_tablenames.append(pair.first);
    }
    return sorted_tablenames;
}


QStringList TaskFactory::allTablenames() const
{
    return m_all_tablenames;
}


TaskPtr TaskFactory::create(const QString& key, const int load_pk) const
{
    if (!m_map.contains(key)) {
        qWarning().nospace() << "TaskFactory::create(" << key << ", "
                             << load_pk << ")" << "... no such task class";
        return TaskPtr(nullptr);
    }
#ifdef DEBUG_TASK_CREATION
    qDebug().nospace() << "TaskFactory::create(" << key << ", "
                       << load_pk << ")";
#endif
    ProxyType proxy = m_map[key].proxy;
    return proxy->create(m_app, m_app.db(), load_pk);
}


void TaskFactory::makeAllTables() const
{
    MapIteratorType it(m_map);
    while (it.hasNext()) {
        it.next();
        ProxyType proxy = it.value().proxy;
        TaskPtr p_task = proxy->create(m_app, m_app.db());
        p_task->makeTables();
    }
}


QString TaskFactory::shortname(const QString& key) const
{
    if (!m_map.contains(key)) {
        qWarning() << "Bad task: " << key;
        return nullptr;
    }
    return m_map[key].shortname;
}


QString TaskFactory::longname(const QString& key) const
{
    if (!m_map.contains(key)) {
        qWarning() << "Bad task: " << key;
        return nullptr;
    }
    return m_map[key].longname;
}


void TaskFactory::makeTables(const QString& key) const
{
    TaskPtr p_task = create(key);
    if (!p_task) {
        return;
    }
    p_task->makeTables();
}


TaskPtrList TaskFactory::fetchTasks(const QString& tablename, const bool sort) const
{
    // KEY SECURITY DECISIONS IMPLEMENTED HERE: which tasks users can see.
    const int patient_id = m_app.selectedPatientId();
    const bool patient_selected = patient_id != dbconst::NONEXISTENT_PK;
    TaskPtrList tasklist;
    if (tablename.isEmpty()) {
        // Patient summary view; "all tasks" request.
        // - Patient selected -> all tasks for current patient (whether locked
        //   or not).
        // - No patient selected -> return nothing.
        if (patient_selected) {
            tasklist = fetchAllTasksForPatient(patient_id);
        }
    } else if (!m_map.contains(tablename)) {
        // Duff task
        qWarning() << "Bad task: " << tablename;
    } else {
        // Specific task
        // - Patient-based task / patient selected -> tasks for that patient
        //   (whether locked or not).
        // - Patient-based task / no patient selected / unlocked -> all such
        //   tasks, for all patients.
        // - Patient-based task / no patient selected / locked -> nothing.
        // - Anonymous task / patient selected -> all such tasks
        //   ... see also TaskRegistrar::fetch().
        //   ... if you choose "none", users will probably wonder where
        //       tasks are vanishing to
        // - Anonymous task / no patient selected -> all such tasks
        const TaskCache& cache = m_map[tablename];
        ProxyType proxy = cache.proxy;
        const bool anonymous = cache.anonymous;
        const bool locked = m_app.locked();
        if (anonymous) {
            tasklist = proxy->fetch(m_app, m_app.db(), dbconst::NONEXISTENT_PK);
        } else {
            if (patient_selected || !locked) {
                tasklist = proxy->fetch(m_app, m_app.db(), patient_id);
            }
        }
    }

    if (sort) {
        // qDebug() << "Starting sort...";
        std::sort(tasklist.begin(), tasklist.end(), TaskSorter());
        // qDebug() << "... finished sort";
    }

    return tasklist;
}


TaskPtrList TaskFactory::fetchAllTasksForPatient(const int patient_id) const
{
    TaskPtrList tasklist;
    MapIteratorType it(m_map);
    while (it.hasNext()) {
        it.next();
        const TaskCache& cache = it.value();
        const bool anonymous = cache.anonymous;
        if (anonymous) {
            continue;
        }
        ProxyType proxy = cache.proxy;
        tasklist += proxy->fetch(m_app, m_app.db(), patient_id);
    }
    return tasklist;
}


TaskPtrList TaskFactory::allSpecimens() const
{
    TaskPtrList specimens;
    MapIteratorType it(m_map);
    while (it.hasNext()) {
        it.next();
        const TaskCache& cache = it.value();
        ProxyType proxy = cache.proxy;
        TaskPtr specimen = proxy->create(m_app, m_app.db(),
                                         dbconst::NONEXISTENT_PK);
        specimens.append(specimen);
    }
    return specimens;
}


TaskPtrList TaskFactory::allSpecimensExceptAnonymous() const
{
    TaskPtrList specimens;
    MapIteratorType it(m_map);
    while (it.hasNext()) {
        it.next();
        const TaskCache& cache = it.value();
        const bool anonymous = cache.anonymous;
        if (anonymous) {
            continue;
        }
        ProxyType proxy = cache.proxy;
        TaskPtr specimen = proxy->create(m_app, m_app.db(),
                                         dbconst::NONEXISTENT_PK);
        specimens.append(specimen);
    }
    return specimens;
}


void TaskFactory::upgradeDatabase(const Version& old_version,
                                  const Version& new_version)
{
    TaskPtrList specimens = allSpecimens();
    for (const TaskPtr& t : specimens) {
        t->upgradeDatabase(old_version, new_version);
    }
}


Version TaskFactory::minimumServerVersion(const QString& tablename) const
{
    // For speed order:
    if (m_map.contains(tablename)) {
        // It's a main tablename.
        TaskPtr specimen = create(tablename);
        return specimen->minimumServerVersion();
    }
    if (!m_all_tablenames.contains(tablename)) {
        // It's duff.
        qWarning() << "TaskFactory::minimumServerVersion: don't know table"
                   << tablename;
        return camcopsversion::MINIMUM_SERVER_VERSION;
    }
    // Otherwise, it's an ancillary.
    TaskPtrList specimens = allSpecimens();
    for (const TaskPtr& specimen : specimens) {
        const QStringList task_tables = specimen->allTables();
        if (task_tables.contains(tablename)) {
            return specimen->minimumServerVersion();
        }
    }
    qCritical() << "Bug in TaskFactory::minimumServerVersion! Tablename was"
                << tablename;
    return camcopsversion::MINIMUM_SERVER_VERSION;
}
