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

// Modified from Qt's qtbase/src/plugins/sqldrivers/sqlite/qsql_sqlite.cpp

/* ============================================================================
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
============================================================================ */

#define MODIFIED_FROM_SQLITE  // shows what we've done

#include "sqlcipherdriver.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QStringList>
#include <QtGlobal>  // for qAsConst
#include <QVariant>
#include <QVector>
#include "db/sqlcipherhelpers.h"
#include "db/sqlcipherresult.h"

#if defined Q_OS_WIN
# include <qt_windows.h>
#else
# include <unistd.h>
#endif

#ifdef MODIFIED_FROM_SQLITE
#include "db/whichdb.h"
    #ifdef USE_SQLCIPHER
#include <sqlcipher/sqlite3.h>  // does the 'extern "C" { ... }' part for us
    #else
#include <sqlite3.h>
    #endif
#else
#include <sqlite3.h>
#endif

Q_DECLARE_OPAQUE_POINTER(sqlite3*)
Q_DECLARE_METATYPE(sqlite3*)

Q_DECLARE_OPAQUE_POINTER(sqlite3_stmt*)
Q_DECLARE_METATYPE(sqlite3_stmt*)

using sqlcipherhelpers::_q_escapeIdentifier;
using sqlcipherhelpers::qMakeError;
using sqlcipherhelpers::qGetTableInfo;


// ============================================================================
// Ensure SQLCipher is installed (compiled with and linked in)
// ============================================================================

void ensureSqlCipherLinkedIfRequired()
{
#ifdef USE_SQLCIPHER
    // The <sqlcipher/sqlite3.h> and <sqlite3.h> headers are very similar,
    // and it's possible to compile with the SQLCipher header but then
    // accidentally link to the original sqlite3.o, so let's make sure...
    // This will only compile/link if we genuinely are using SQLCipher.
    sqlite3_key(nullptr, nullptr, 0);
    // https://www.zetetic.net/sqlcipher/sqlcipher-api/#sqlite3_key
#endif
}


// ============================================================================
// SQLCipherDriver + private
// ============================================================================

SQLCipherDriver::SQLCipherDriver(QObject* parent) :
    QSqlDriver(parent),
    m_access(nullptr)
{
    // dbmsType = QSqlDriver::SQLite;
}


// ============================================================================
// SQLCipherDriver
// ============================================================================

SQLCipherDriver::SQLCipherDriver(sqlite3* connection, QObject* parent) :
    QSqlDriver(parent)
{
    m_access = connection;
    setOpen(true);
    setOpenError(false);
}


SQLCipherDriver::~SQLCipherDriver()
{
}


bool SQLCipherDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case BLOB:
    case Transactions:
    case Unicode:
    case LastInsertId:
    case PreparedQueries:
    case PositionalPlaceholders:
    case SimpleLocking:
    case FinishQuery:
    case LowPrecisionNumbers:
        return true;
    case QuerySize:
    case NamedPlaceholders:
    case BatchOperations:
    case EventNotifications:
    case MultipleResultSets:
    case CancelQuery:
        return false;
    }
    return false;
}


// SQLite dbs have no user name, passwords, hosts or ports.
// just file names.
bool SQLCipherDriver::open(const QString& db, const QString& user,
                           const QString& password, const QString &host,
                           const int port, const QString& conn_opts)
{
    Q_UNUSED(user)
    Q_UNUSED(password)
    Q_UNUSED(host)
    Q_UNUSED(port)

    if (isOpen()) {
        close();
    }

    int time_out = 5000;
    bool shared_cache = false;
    bool open_read_only_option = false;
    bool open_uri_option = false;

    const auto opts = conn_opts.splitRef(QLatin1Char(';'));
    for (auto option : opts) {
        option = option.trimmed();
        if (option.startsWith(QLatin1String("QSQLITE_BUSY_TIMEOUT"))) {
            option = option.mid(20).trimmed();
            if (option.startsWith(QLatin1Char('='))) {
                bool ok;
                const int nt = option.mid(1).trimmed().toInt(&ok);
                if (ok) {
                    time_out = nt;
                }
            }
        } else if (option == QLatin1String("QSQLITE_OPEN_READONLY")) {
            open_read_only_option = true;
        } else if (option == QLatin1String("QSQLITE_OPEN_URI")) {
            open_uri_option = true;
        } else if (option == QLatin1String("QSQLITE_ENABLE_SHARED_CACHE")) {
            shared_cache = true;
        }
    }

    int open_mode = (open_read_only_option
                     ? SQLITE_OPEN_READONLY
                     : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
    if (open_uri_option) {
        open_mode |= SQLITE_OPEN_URI;
    }

    sqlite3_enable_shared_cache(shared_cache);

    if (sqlite3_open_v2(db.toUtf8().constData(), &m_access, open_mode,
                        nullptr) == SQLITE_OK) {
        sqlite3_busy_timeout(m_access, time_out);
        setOpen(true);
        setOpenError(false);
        return true;
    }
    if (m_access) {
        sqlite3_close(m_access);
        m_access = nullptr;
    }

    setLastError(qMakeError(m_access, tr("Error opening database"),
                 QSqlError::ConnectionError));
    setOpenError(true);
    return false;
}


void SQLCipherDriver::close()
{
    if (isOpen()) {
        for (SQLCipherResult* result : qAsConst(m_results)) {
            result->finalize();
        }

        if (sqlite3_close(m_access) != SQLITE_OK) {
            setLastError(qMakeError(m_access, tr("Error closing database"),
                                    QSqlError::ConnectionError));
        }
        m_access = nullptr;
        setOpen(false);
        setOpenError(false);
    }
}


QSqlResult* SQLCipherDriver::createResult() const
{
    return new SQLCipherResult(this);
}


bool SQLCipherDriver::beginTransaction()
{
    if (!isOpen() || isOpenError()) {
        return false;
    }

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("BEGIN"))) {
        setLastError(QSqlError(tr("Unable to begin transaction"),
                               q.lastError().databaseText(),
                               QSqlError::TransactionError));
        return false;
    }

    return true;
}


bool SQLCipherDriver::commitTransaction()
{
    if (!isOpen() || isOpenError()) {
        return false;
    }

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("COMMIT"))) {
        setLastError(QSqlError(tr("Unable to commit transaction"),
                               q.lastError().databaseText(),
                               QSqlError::TransactionError));
        return false;
    }

    return true;
}


bool SQLCipherDriver::rollbackTransaction()
{
    if (!isOpen() || isOpenError()) {
        return false;
    }

    QSqlQuery q(createResult());
    if (!q.exec(QLatin1String("ROLLBACK"))) {
        setLastError(QSqlError(tr("Unable to rollback transaction"),
                               q.lastError().databaseText(),
                               QSqlError::TransactionError));
        return false;
    }

    return true;
}


QStringList SQLCipherDriver::tables(const QSql::TableType type) const
{
    QStringList res;
    if (!isOpen()) {
        return res;
    }

    QSqlQuery q(createResult());
    q.setForwardOnly(true);

    QString sql = QLatin1String("SELECT name FROM sqlite_master WHERE %1 "
                                "UNION ALL SELECT name FROM sqlite_temp_master WHERE %1");
    if ((type & QSql::Tables) && (type & QSql::Views)) {
        sql = sql.arg(QLatin1String("type='table' OR type='view'"));
    } else if (type & QSql::Tables) {
        sql = sql.arg(QLatin1String("type='table'"));
    } else if (type & QSql::Views) {
        sql = sql.arg(QLatin1String("type='view'"));
    } else {
        sql.clear();
    }

    if (!sql.isEmpty() && q.exec(sql)) {
        while (q.next()) {
            res.append(q.value(0).toString());
        }
    }

    if (type & QSql::SystemTables) {
        // there are no internal tables beside this one:
        res.append(QLatin1String("sqlite_master"));
    }

    return res;
}


QSqlIndex SQLCipherDriver::primaryIndex(const QString& tblname) const
{
    if (!isOpen()) {
        return QSqlIndex();
    }

    QString table = tblname;
    if (isIdentifierEscaped(table, QSqlDriver::TableName)) {
        table = stripDelimiters(table, QSqlDriver::TableName);
    }

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    return qGetTableInfo(q, table, true);
}


QSqlRecord SQLCipherDriver::record(const QString& tbl) const
{
    if (!isOpen()) {
        return QSqlRecord();
    }

    QString table = tbl;
    if (isIdentifierEscaped(table, QSqlDriver::TableName)) {
        table = stripDelimiters(table, QSqlDriver::TableName);
    }

    QSqlQuery q(createResult());
    q.setForwardOnly(true);
    return qGetTableInfo(q, table);
}


QVariant SQLCipherDriver::handle() const
{
    return QVariant::fromValue(m_access);
}


QString SQLCipherDriver::escapeIdentifier(const QString& identifier,
                                          const IdentifierType type) const
{
    Q_UNUSED(type)
    return _q_escapeIdentifier(identifier);
}
