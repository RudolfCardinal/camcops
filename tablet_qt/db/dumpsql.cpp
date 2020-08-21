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

#include "dumpsql.h"
#include <QDebug>
#include <QSqlRecord>
#include <QSqlQuery>
#include "db/databasemanager.h"
#include "db/dbfunc.h"
#include "db/queryresult.h"
#include "lib/stringfunc.h"
using stringfunc::replaceFirst;

const QString NL("\n"); // newline
const QString DUMP_T_START("PRAGMA foreign_keys=OFF;" + NL +
                           "BEGIN TRANSACTION;" + NL);
const QString DUMP_Q_1("SELECT name, type, sql FROM sqlite_master "
                       "WHERE sql NOT NULL AND type=='table' "
                       "AND name!='sqlite_sequence' ORDER BY name");
const QString DUMP_Q_2("SELECT name, type, sql FROM sqlite_master "
                       "WHERE name=='sqlite_sequence'");
const QString DUMP_Q_3("SELECT sql FROM sqlite_master WHERE sql NOT NULL "
                       "AND type IN ('index','trigger','view')");
const QString DUMP_E_START_1("SAVEPOINT dump;");
const QString DUMP_E_START_2("PRAGMA writable_schema=ON;");
const QString DUMP_E_WSOFF("PRAGMA writable_schema=OFF;");
const QString DUMP_E_RELEASE("RELEASE dump;");
const QString DUMP_T_WSOFF("PRAGMA writable_schema=OFF;" + NL);
const QString DUMP_T_WSON("PRAGMA writable_schema=ON;" + NL);
const QString DUMP_T_END_FAILURE("ROLLBACK; -- due to errors" + NL);
const QString DUMP_T_END_SUCCESS("COMMIT;" + NL);
const QString DUMP_T_SQL_TERMINATOR(";" + NL);
const QString TYPE_SEQUENCE("sqlite_sequence");
const QString DELETE_SEQUENCES("DELETE FROM sqlite_sequence;" + NL);
const QString STAT1("sqlite_stat1");
const QString ANALYSE_MASTER("ANALYZE sqlite_master;" + NL);
const QString PREFIX("sqlite_");
const QString CREATE_VT("CREATE VIRTUAL TABLE");
const QString INSERT_INTO_MASTER(
        "INSERT INTO sqlite_master(type,name,tbl_name,"
        "rootpage,sql) VALUES('table','%','%',0,'%');" + NL);
const QString PLACEHOLDER("%");
// ... we'll replace with a regex-based function, so don't use "?"
const QString TYPE_TABLE("table");
const QString PRAGMA_TABLEINFO("PRAGMA table_info(\"%\");");
const QString DATASELECT_1_SELECT_INSERT_INTO_VALUES(
        "SELECT 'INSERT INTO ' || '\"%\"' || ' VALUES(' || ");
const QString DATASELECT_2_QUOTE("quote(\"%\")");
const QString DATASELECT_3_FROM("|| ')' FROM \"%\"");
const QString COMMENT_STARTING(NL + "-- Starting" + NL + NL);
const QString COMMENT_TABLES(NL + "-- Tables" + NL + NL);
const QString COMMENT_SEQUENCES(NL + "-- Sequences" + NL + NL);
const QString COMMENT_OTHER(NL + "-- Indexes, triggers, views" + NL + NL);
const QString COMMENT_ENDING(NL + "-- Ending" + NL + NL);
// For comments "-- ", the space isn't standard SQL but some engines need it.
const QString GET_VERSION("SELECT sqlite_version() FROM sqlite_master");
const QString VALUE_SEP_COMMA = ", ";  // space less efficient but easier to read


void dumpsql::runTableDumpQuery(QTextStream& os,
                                DatabaseManager& db,
                                const QString& sql,
                                const QString& firstrow)
{
    const QueryResult result = db.query(sql);
    if (!result.succeeded()) {
        return;
    }
    os << firstrow;
    const int nrows = result.nRows();
    const int ncols = result.nCols();
    for (int row = 0; row < nrows; ++row) {
        for (int col = 0; col < ncols; ++col) {
            if (col > 0) {
                os << VALUE_SEP_COMMA;
            }
            os << result.at(row, col).toString();
        }
        if (ncols == 1 && result.at(row, 0).toString().contains("--")) {
            os << NL; // so comments don't subsume the final ";"
        }
        os << DUMP_T_SQL_TERMINATOR;
    }
}


bool dumpsql::runSchemaDumpQuery(QTextStream& os,
                                 DatabaseManager& db,
                                 const QString& schema_query_sql,
                                 bool writable_schema)
{
    const QueryResult result_a = db.query(schema_query_sql);
    if (!result_a.succeeded()) {
        return writable_schema;
    }
    bool firstline = true;
    const int nrows_a = result_a.nRows();
    for (int row_a = 0; row_a < nrows_a; ++row_a) {
        const QString table = result_a.at(row_a, 0).toString();
        const QString type = result_a.at(row_a, 1).toString();
        const QString maketable_sql = result_a.at(row_a, 2).toString();
        if (!firstline) {
            os << NL;
        } else {
            firstline = false;
        }
        QString prepstatement;
        if (table == TYPE_SEQUENCE) {
            prepstatement = DELETE_SEQUENCES;
        } else if (table == STAT1) {
            os << ANALYSE_MASTER;
        } else if (table.startsWith(PREFIX)) {
            continue;
        }
        if (maketable_sql.startsWith(CREATE_VT)) {
            if (!writable_schema) {
                os << DUMP_T_WSON;
                writable_schema = true;
            }
            QString ins = INSERT_INTO_MASTER;
            replaceFirst(ins, PLACEHOLDER, table);
            replaceFirst(ins, PLACEHOLDER, table);  // correct; table again
            replaceFirst(ins, PLACEHOLDER, maketable_sql);
            // ... seems to handle e.g. escaped quotes correctly
            os << ins << DUMP_T_SQL_TERMINATOR;
            continue;
        }
        os << maketable_sql << DUMP_T_SQL_TERMINATOR;
        if (type == TYPE_TABLE) {
            QString tableinfo_query = PRAGMA_TABLEINFO;
            replaceFirst(tableinfo_query, PLACEHOLDER, table);
            QueryResult result_b = db.query(tableinfo_query);
            if (!result_b.succeeded()) {
                continue;
            }
            QString select = DATASELECT_1_SELECT_INSERT_INTO_VALUES;
            replaceFirst(select, PLACEHOLDER, table);
            bool first = true;
            const int nrows_b = result_b.nRows();
            for (int row_b = 0; row_b < nrows_b; ++row_b) {
                if (!first) {
                    select += ",";
                } else {
                    first = false;
                }
                const QString text = result_b.at(row_b, 1).toString();
                QString databit = DATASELECT_2_QUOTE;
                replaceFirst(databit, PLACEHOLDER, text);
                select += databit;
            }
            QString endbit = DATASELECT_3_FROM;
            replaceFirst(endbit, PLACEHOLDER, table);
            select += endbit;
            runTableDumpQuery(os, db, select, prepstatement);
            // Doesn't deal with:
            // if( rc==SQLITE_CORRUPT ){
            //  zSelect = appendText(zSelect,
            //                       " ORDER BY rowid DESC", 0);
            //  run_table_dump_query(p, zSelect, 0);
            // }
        }
    }
    return writable_schema;
}


void dumpsql::dumpDatabase(QTextStream& os, DatabaseManager& db)
{
    bool success = true;  // not really used?
    bool writable_schema = false;

    os << COMMENT_STARTING;
    os << DUMP_T_START;

    db.execNoAnswer(DUMP_E_START_1);
    db.execNoAnswer(DUMP_E_START_2);

    // Tables
    os << COMMENT_TABLES;
    writable_schema = runSchemaDumpQuery(os, db, DUMP_Q_1, writable_schema) ||
            writable_schema;

    // Sequences
    os << COMMENT_SEQUENCES;
    writable_schema = runSchemaDumpQuery(os, db, DUMP_Q_2, writable_schema) ||
            writable_schema;

    // Indexes, triggers, views
    os << COMMENT_OTHER;
    runTableDumpQuery(os, db, DUMP_Q_3, "");

    // Finishing
    os << COMMENT_ENDING;
    if (writable_schema) {
        os << DUMP_T_WSOFF;
    }
    db.execNoAnswer(DUMP_E_WSOFF);
    db.execNoAnswer(DUMP_E_RELEASE);
    os << (success ? DUMP_T_END_SUCCESS : DUMP_T_END_FAILURE);
}
