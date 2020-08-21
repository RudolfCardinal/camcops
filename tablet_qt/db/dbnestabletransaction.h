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
#include <QString>
class DatabaseManager;


// Represents an SQL transaction that can be nested, using
//      SAVEPOINT name;
//      RELEASE name;  -- on success
//      ROLLBACK TO SAVEPOINT name;  -- on failure

class DbNestableTransaction
{
    // https://www.sqlite.org/lang_savepoint.html
public:
    // Create the transaction. It starts in a "successful" state.
    DbNestableTransaction(DatabaseManager& db);

    // When the transaction is destroyed, it releases or rolls back depending
    // on whether it's been told of failure or not.
    ~DbNestableTransaction();

    // Mark the transaction as a failure.
    void fail();

    // Mark the transaction as successful.
    void succeed();

protected:

    // Our database manager.
    DatabaseManager& m_db;

    // Have we failed?
    bool m_fail;

    // What's our SAVEPOINT name?
    QString m_name;

    // Used for the savepoint name; continuously increments
    static int s_count;

    // Current depth within savepoint stack
    static int s_level;
};
