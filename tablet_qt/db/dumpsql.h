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
#include <QTextStream>
class DatabaseManager;

namespace dumpsql {

// Complex helper function for dumpDatabase().
void runTableDumpQuery(QTextStream& os,
                       DatabaseManager& db,
                       const QString& sql,
                       const QString& firstrow);

// Complex helper function for dumpDatabase().
bool runSchemaDumpQuery(QTextStream& os,
                        DatabaseManager& db,
                        const QString& schema_query_sql,
                        bool writable_schema);

// Dumps an entire SQLite database to an output stream.
void dumpDatabase(QTextStream& os, DatabaseManager& db);

}  // namespace dumpsql
