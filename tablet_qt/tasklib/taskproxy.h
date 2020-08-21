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
#include <QSqlDatabase>
#include "common/dbconst.h"
#include "task.h"

class TaskFactory;


// For TaskFactory:
// ===========================================================================
// Base "descriptor" class, so we can do more things with the class as an
// entity than just instantiate one.
// ===========================================================================

class TaskProxy
{
    // Base class for TaskRegistrar.
    // It doesn't do much (except register itself with a TaskFactory), but it
    // defines an interface.
    // For example, the PHQ9 task creates a single TaskRegistrar<Phq9> object.

public:

    // Construct the proxy, which registers itself with the factory.
    TaskProxy(TaskFactory& factory);

    // Destructor.
    virtual ~TaskProxy() = default;

    // Create an instance of the task and return it.
    virtual TaskPtr create(CamcopsApp& app,
                           DatabaseManager& db,
                           int load_pk = dbconst::NONEXISTENT_PK) const = 0;

    // Fetch tasks from the database.
    virtual TaskPtrList fetch(CamcopsApp& app,
                              DatabaseManager& db,
                              int patient_id = dbconst::NONEXISTENT_PK) const = 0;

protected:

    // Fetch tasks from the database that meet specified criteria.
    virtual TaskPtrList fetchWhere(CamcopsApp& app,
                                   DatabaseManager& db,
                                   const WhereConditions& where) const = 0;
};
