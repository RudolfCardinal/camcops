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
#include <QDebug>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QVariant>
#include "common/aliases_qt.h"
#include "db/field.h"
#include "sqlargs.h"

class SqlitePragmaInfoField;

namespace dbfunc {

// ============================================================================
// Constants
// ============================================================================

extern const QString DATA_DATABASE_FILENAME;
extern const QString SYSTEM_DATABASE_FILENAME;
extern const QString DATABASE_FILENAME_TEMP_SUFFIX;
extern const QString TABLE_TEMP_SUFFIX;

// ============================================================================
// SQL fragments
// ============================================================================

// Returns an identifier (e.g. table name, field name), delimited according to
// ANSI SQL standards.
QString delimit(const QString& identifier);

// Returns "SELECT <columns> FROM <table>", with delimiting.
QString selectColumns(const QStringList& columns, const QString& table);

// Returns SQL like "UPDATE <table> SET <field1>=?, ...", with delimiting, in
// an SqlArgs object with the values.
// The updatevalues parameter maps fieldnames to values.
SqlArgs updateColumns(const UpdateValues& updatevalues, const QString& table);

// ============================================================================
// Queries
// ============================================================================

// If require, appends an " ORDER BY..." clause to the SQL given.
void addOrderByClause(const OrderBy& order_by, SqlArgs& sqlargs_altered);

// Binds arguments to a QSqlQuery, from a vector of argument values.
void addArgs(QSqlQuery& query, const ArgList& args);

// Executes a QSqlQuery. (Low-level function.)
bool execQuery(QSqlQuery& query, const SqlArgs& sqlargs,
               bool suppress_errors = false);

// Executes a QSqlQuery. (Low-level function.)
bool execQuery(QSqlQuery& query, const QString& sql, const ArgList& args,
               bool suppress_errors = false);

// Returns a string like "?,?,?" for n parameter holders
QString sqlParamHolders(int n);

// Converts a vector of ints to an ArgList; that is, a QVector<QVariant>.
ArgList argListFromIntList(const QVector<int>& intlist);

// ============================================================================
// Database structure
// ============================================================================

// Returns the field names from an SqlitePragmaInfoField object.
// If delimited is true, delimits the output.
QStringList fieldNamesFromPragmaInfo(
        const QVector<SqlitePragmaInfoField>& infolist,
        bool delimited = false);

// Returns "CREATE TABLE IF NOT EXISTS ..." SQL from information describing
// the table.
QString makeCreationSqlFromPragmaInfo(
        const QString& tablename,
        const QVector<SqlitePragmaInfoField>& infolist);

// ============================================================================
// Altering structure
// ============================================================================

// Returns "CREATE TABLE IF NOT EXISTS ..." SQL from a list of Field objects.
QString sqlCreateTable(const QString& tablename,
                       const QVector<Field>& fieldlist);

// ============================================================================
// Encryption queries, via SQLCipher
// ============================================================================

// Encrypts an SQLite database via SQLCipher, on disk, in place (in the end;
// in practice via a temporary file).
bool encryptPlainDatabaseInPlace(const QString& filename,
                                 const QString& tempfilename,
                                 const QString& passphrase);

}  // namespace dbfunc
