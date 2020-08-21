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

// #define DEBUG_SQL_QUERY
// #define DEBUG_QUERY_END
// #define DEBUG_SQL_RESULT
// #define DEBUG_VERBOSE_TABLE_CHANGE_PLANS
// #define DEBUG_QUERY_TIMING

#include "dbfunc.h"
#include <QDateTime>
#include <QObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include "db/databasemanager.h"
#include "db/sqlitepragmainfofield.h"
#include "db/whichdb.h"
#include "lib/convert.h"
#include "lib/debugfunc.h"
#include "lib/filefunc.h"
#include "lib/uifunc.h"


namespace dbfunc {

// ============================================================================
// Constants
// ============================================================================

const QString DATA_DATABASE_FILENAME("camcops_data.sqlite");
const QString SYSTEM_DATABASE_FILENAME("camcops_sys.sqlite");
const QString DATABASE_FILENAME_TEMP_SUFFIX("_temp");
const QString TABLE_TEMP_SUFFIX("_temp");

// Private to this file:
const QString CONNECTION_ENCRYPTION_TEMP_PLAIN("encryption_temp_plain");


// ============================================================================
// SQL fragments
// ============================================================================

QString delimit(const QString& identifier)
{
    // Delimits a table or fieldname, by ANSI SQL standards.

    // http://www.sqlite.org/lang_keywords.html
    // http://stackoverflow.com/questions/2901453/sql-standard-to-escape-column-names
    // You must delimit anything with funny characters or any keyword,
    // and the list of potential keywords is long, so just delimit everything.
    return "\"" + identifier + "\"";
}


QString selectColumns(const QStringList& columns, const QString& table)
{
    QStringList delimited_columns;
    for (const QString& column : columns) {
        delimited_columns.append(delimit(column));
    }
    return QString("SELECT %1 FROM %2").arg(delimited_columns.join(","),
                                            delimit(table));
}


SqlArgs updateColumns(const UpdateValues& updatevalues, const QString& table)
{
    QStringList columns;
    ArgList args;
    QMapIterator<QString, QVariant> it(updatevalues);
    while (it.hasNext()) {
        it.next();
        QString column = it.key();
        QVariant value = it.value();
        columns.append(QString("%1=?").arg(delimit(column)));
        args.append(value);
    }
    const QString sql = QString("UPDATE %1 SET %2").arg(delimit(table),
                                                        columns.join(", "));
    return SqlArgs(sql, args);
}


// ============================================================================
// Queries
// ============================================================================

void addOrderByClause(const OrderBy& order_by, SqlArgs& sqlargs_altered)
{
    if (order_by.isEmpty()) {
        return;
    }
    QStringList order_by_clauses;
    for (QPair<QString, bool> pair : order_by) {
        const QString fieldname = pair.first;
        const bool ascending = pair.second;
        order_by_clauses.append(QString("%1 %2").arg(
                                    delimit(fieldname),
                                    ascending ? "ASC" : "DESC"));
    }
    sqlargs_altered.sql += " ORDER BY " + order_by_clauses.join(", ");
}


void addArgs(QSqlQuery& query, const ArgList& args)
{
    // Adds arguments to a QSqlQuery from a list/vector.
    const int size = args.size();
    for (int i = 0; i < size; ++i) {
        query.addBindValue(args.at(i), QSql::In);
    }
}


bool execQuery(QSqlQuery& query, const SqlArgs& sqlargs,
               const bool suppress_errors)
{
    return execQuery(query, sqlargs.sql, sqlargs.args, suppress_errors);
}


bool execQuery(QSqlQuery& query, const QString& sql, const ArgList& args,
               const bool suppress_errors)
{
    // Executes an existing query (in place) with the supplied SQL/args.
    // THIS IS THE MAIN POINT THROUGH WHICH ALL QUERIES SHOULD BE EXECUTED.
    query.prepare(sql);
    addArgs(query, args);

#ifdef DEBUG_SQL_QUERY
    {
        qDebug() << "Executing:" << qUtf8Printable(sql);
        QDebug debug = qDebug().nospace();
        debug << "... args: ";
        debugfunc::debugConcisely(debug, args);
    }  // endl on destruction
#endif

#ifdef DEBUG_QUERY_TIMING
    const QDateTime start_time = QDateTime::currentDateTime();
#endif
    bool success = query.exec();
#ifdef DEBUG_QUERY_TIMING
    const QDateTime end_time = QDateTime::currentDateTime();
#endif
#ifdef DEBUG_QUERY_END
    qDebug() << "... query finished";
#endif
#ifdef DEBUG_QUERY_TIMING
    qDebug() << (query.isSelect() ? "SELECT" : "Non-SELECT")
             << "query took" << start_time.msecsTo(end_time) << "ms";
#endif
    if (!success && !suppress_errors) {
        qCritical() << "Query failed; error was:" << query.lastError();
        qCritical().noquote() << "SQL was:" << sql;
        qCritical() << "Args were:" << args;
    }
#ifdef DEBUG_SQL_RESULT
    if (success && query.isSelect() && !query.isForwardOnly()) {
        qDebug() << "Resultset preview:";
        int row = 0;
        while (query.next()) {
            QDebug debug = qDebug().nospace();
            const QSqlRecord rec = query.record();
            int ncols = rec.count();
            debug << "... row " << row << ": ";
            for (int col = 0; col < ncols; ++col) {
                if (col > 0) {
                    debug << "; ";
                }
                debug << rec.fieldName(col) << "=";
                debugfunc::debugConcisely(debug, query.value(col));
            }
            ++row;
        }  // endl on destruction
        if (row == 0) {
            qDebug() << "<no rows>";
        }
        query.seek(QSql::BeforeFirstRow);  // the original starting position
    }
#endif
    return success;
    // The return value is boolean (success?).
    // Use query.next() to iterate through a result set; see
    // http://doc.qt.io/qt-4.8/sql-sqlstatements.html
}


QString sqlParamHolders(const int n)
{
    // String like "?,?,?" for n parameter holders
    QString paramholders;
    for (int i = 0; i < n; ++i) {
        if (i != 0) {
            paramholders += ",";
        }
        paramholders += "?";
    }
    return paramholders;
}


ArgList argListFromIntList(const QVector<int>& intlist)
{
    ArgList args;
    for (auto value : intlist) {
        args.append(value);
    }
    return args;
}


// ============================================================================
// Database structure
// ============================================================================

QStringList fieldNamesFromPragmaInfo(
        const QVector<SqlitePragmaInfoField>& infolist,
        const bool delimited)
{
    QStringList fieldnames;
    const int size = infolist.size();
    for (int i = 0; i < size; ++i) {
        QString name = infolist.at(i).name;
        if (delimited) {
            name = delimit(name);
        }
        fieldnames.append(name);
    }
    return fieldnames;
}


QString makeCreationSqlFromPragmaInfo(
        const QString& tablename,
        const QVector<SqlitePragmaInfoField>& infolist)
{
    QStringList fieldspecs;
    const int size = infolist.size();
    for (int i = 0; i < size; ++i) {
        const SqlitePragmaInfoField& info = infolist.at(i);
        QStringList elements;
        elements.append(delimit(info.name));
        elements.append(info.type);
        if (info.notnull) {
            elements.append("NOT NULL");
        }
        if (!info.dflt_value.isNull()) {
            elements.append("DEFAULT " + info.dflt_value.toString());
            // default value already delimited by SQLite
        }
        if (info.pk) {
            elements.append("PRIMARY KEY");
        }
        fieldspecs.append(elements.join(" "));
    }
    return QString("CREATE TABLE IF NOT EXISTS %1 (%2)").arg(
        delimit(tablename), fieldspecs.join(", "));
}


// ============================================================================
// Altering structure
// ============================================================================

QString sqlCreateTable(const QString& tablename,
                       const QVector<Field>& fieldlist)
{
    QStringList coldefs;
    for (const Field& field : fieldlist) {
        const QString coltype = field.sqlColumnDef();
        coldefs << QString("%1 %2").arg(delimit(field.name()), coltype);
    }
    const QString sql = QString("CREATE TABLE IF NOT EXISTS %1 (%2)").arg(
        delimit(tablename), coldefs.join(", "));
    return sql;
}


// ============================================================================
// Encryption queries, via SQLCipher
// ============================================================================

bool encryptPlainDatabaseInPlace(const QString& filename,
                                 const QString& tempfilename,
                                 const QString& passphrase)
{
    // If the database was not empty, we have to use a temporary database
    // method:
    // https://discuss.zetetic.net/t/how-to-encrypt-a-plaintext-sqlite-database-to-use-sqlcipher-and-avoid-file-is-encrypted-or-is-not-a-database-errors/868
    qInfo().nospace()
            << "Converting plain database ("
            << filename << ") to encrypted database (using temporary file: "
            << tempfilename << ")";
    const QString title(QObject::tr("Error encrypting databases"));

    // 1. Check files exist/don't exist.
    if (!filefunc::fileExists(filename)) {
        uifunc::stopApp("Missing database: " + filename, title);
    }
    if (filefunc::fileExists(tempfilename)) {
        uifunc::stopApp("Temporary file exists but shouldn't: " + tempfilename,
                        title);
    }

    bool success = false;
    {  // scope to close db automatically
        // 2. Open the plain-text database
        DatabaseManager db(filename, CONNECTION_ENCRYPTION_TEMP_PLAIN,
                           whichdb::DBTYPE);

        // 3. Encrypt it to another database.
        success = db.encryptToAnother(tempfilename, passphrase);

        // 4. Close plain-text database properly... by ending this scope.
    }

    // 5. If we managed, rename the databases.
    if (!success) {
        qCritical() << "Failed to export plain -> encrypted";
        return false;
    }
    // If we get here, we're confident that we have a good encrypted database.
    // So, we take the plunge:
    if (!filefunc::deleteFile(filename)) {
        qCritical() << "Failed to delete: " + filename;
        return false;
    }
    if (!filefunc::renameFile(tempfilename, filename)) {
        qCritical() << "Failed to rename " + tempfilename + " -> " + filename;
        return false;
    }
    qInfo() << "... successfully converted";
    return true;
}


}  // namespace dbfunc
