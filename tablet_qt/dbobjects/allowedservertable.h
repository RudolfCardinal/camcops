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
#include "db/databaseobject.h"
#include "lib/version.h"
class CamcopsApp;


// Represents tables that the server will allow us to upload.

class AllowedServerTable : public DatabaseObject
{
    Q_OBJECT
public:
    // Specimen constructor:
    AllowedServerTable(CamcopsApp& app, DatabaseManager& db);

    // Loading constructor:
    AllowedServerTable(CamcopsApp& app, DatabaseManager& db,
                       const QString& tablename);

    // Saving constructor:
    AllowedServerTable(CamcopsApp& app, DatabaseManager& db,
                       const QString& tablename,
                       const Version& min_client_version);

    // Destructor
    virtual ~AllowedServerTable();

    // Returns the table name
    QString tablename() const;

    // What's the minimum client version that the server will accept for this
    // table?
    Version minClientVersion() const;

    // Delete all AllowedServerTable records from the database
    // (Resembles a Python classmethod; sort-of static function.)
    void deleteAllAllowedServerTables();

    // Make table indexes
    // (Resembles a Python classmethod; sort-of static function.)
    void makeIndexes();

public:
    static const QString TABLENAME_FIELD;
    static const QString VERSION_FIELD;
};
