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
#include "db/databasemanager.h"
#include "task.h"
#include "taskproxy.h"


// For TaskFactory:
// ===========================================================================
// Wrapper that makes a TaskProxy out of any Task-derived class
// ===========================================================================

template<class Derived> class TaskRegistrar : public TaskProxy
{
    // For example, the PHQ9 task creates a single TaskRegistrar<Phq9> object.

    static_assert(std::is_base_of<Task, Derived>::value,
                  "You can only use Task-derived classes here.");

public:
    // Constructor. This registers the proxy with the task factory (see
    // TaskProxy::TaskProxy).
    TaskRegistrar(TaskFactory& factory) :
        TaskProxy(factory)
    {}

    // Create a single instance of a task (optionally, loading it).
    TaskPtr create(CamcopsApp& app,
                   DatabaseManager& db,
                   int load_pk = dbconst::NONEXISTENT_PK) const override
    {
        return TaskPtr(new Derived(app, db, load_pk));
    }

    // Fetch multiple tasks, either matching a patient_id, or all for
    // the task type.
    TaskPtrList fetch(CamcopsApp& app,
                      DatabaseManager& db,
                      int patient_id = dbconst::NONEXISTENT_PK) const override
    {
        Derived specimen(app, db, dbconst::NONEXISTENT_PK);
        bool anonymous = specimen.isAnonymous();
        if (patient_id != dbconst::NONEXISTENT_PK && anonymous) {
            // No anonymous tasks will match a specific patient.
            return TaskPtrList();
        }
        WhereConditions where;
        if (patient_id != dbconst::NONEXISTENT_PK) {
            where.add(Task::PATIENT_FK_FIELDNAME, QVariant(patient_id));
        }
        return fetchWhere(app, db, where);
    }

protected:

    // Fetch multiple tasks according to the field/value "where" criteria
    TaskPtrList fetchWhere(CamcopsApp& app,
                           DatabaseManager& db,
                           const WhereConditions& where) const override
    {
        TaskPtrList tasklist;
        Derived specimen(app, db, dbconst::NONEXISTENT_PK);
        SqlArgs sqlargs = specimen.fetchQuerySql(where);
        QueryResult result = db.query(sqlargs);
        int nrows = result.nRows();
        for (int row = 0; row < nrows; ++row) {
            TaskPtr p_new_task(new Derived(app, db,
                                           dbconst::NONEXISTENT_PK));
            p_new_task->setFromQuery(result, row, true);
            tasklist.append(p_new_task);
        }
        return tasklist;
    }
};
