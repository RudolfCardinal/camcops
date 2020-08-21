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

#include "sqlcipherresult.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QSqlDriver>
#include <QSqlField>
#include "common/preprocessor_aid.h"
#include "db/sqlcipherhelpers.h"
#include "db/sqlcipherdriver.h"

#ifdef MODIFIED_FROM_SQLITE
#include "db/whichdb.h"
#ifdef USE_SQLCIPHER
#include <sqlcipher/sqlite3.h>
#endif
#else
#include <sqlite3.h>
#endif

using sqlcipherhelpers::qGetColumnType;
using sqlcipherhelpers::qMakeError;


// ============================================================================
// From combination of SQLiteResultPrivate + SQLiteResult
// ============================================================================

SQLCipherResult::SQLCipherResult(const SQLCipherDriver* drv) :
    SqlCachedResult(drv),
    m_stmt(nullptr),
    m_skipped_status(false),
    m_skip_row(false)
{
    drv->m_results.append(this);
}


// ============================================================================
// From SQLiteResultPrivate
// ============================================================================

void SQLCipherResult::cleanup()
{
    finalize();
    m_r_inf.clear();
    m_skipped_status = false;
    m_skip_row = false;
    setAt(QSql::BeforeFirstRow);
    setActive(false);
    SqlCachedResult::cleanup();
}


void SQLCipherResult::finalize()
{
    if (!m_stmt) {
        return;
    }
    sqlite3_finalize(m_stmt);
    m_stmt = nullptr;
}


void SQLCipherResult::initColumns(const bool emptyResultset)
{
    const int n_cols = sqlite3_column_count(m_stmt);
    if (n_cols <= 0) {
        return;
    }

    init(n_cols);

    for (int i = 0; i < n_cols; ++i) {
        const QString col_name = QString(
                    reinterpret_cast<const QChar*>(sqlite3_column_name16(m_stmt, i)))
                .remove(QLatin1Char('"'));

        // must use type_name for resolving the type to match QSqliteDriver::record
        QString type_name = QString(reinterpret_cast<const QChar*>(
                    sqlite3_column_decltype16(m_stmt, i)));
        // sqlite3_column_type is documented to have undefined behavior if the result set is empty
        int stp = emptyResultset ? -1 : sqlite3_column_type(m_stmt, i);

        QVariant::Type field_type;

        if (!type_name.isEmpty()) {
            field_type = qGetColumnType(type_name);
        } else {
            // Get the proper type for the field based on stp value
            switch (stp) {
            case SQLITE_INTEGER:
                field_type = QVariant::Int;
                break;
            case SQLITE_FLOAT:
                field_type = QVariant::Double;
                break;
            case SQLITE_BLOB:
                field_type = QVariant::ByteArray;
                break;
            case SQLITE_TEXT:
                field_type = QVariant::String;
                break;
            case SQLITE_NULL:
            default:
                field_type = QVariant::Invalid;
                break;
            }
        }

        QSqlField fld(col_name, field_type);
        fld.setSqlType(stp);
        m_r_inf.append(fld);
    }
}


bool SQLCipherResult::fetchNext(SqlCachedResult::ValueCache& values,
                                const int idx, const bool initial_fetch)
{
    int res;
    int i;

    if (m_skip_row) {
        // already fetched
        Q_ASSERT(!initial_fetch);
        m_skip_row = false;
        for (int i = 0; i < m_first_row.count(); i++) {
            values[i] = m_first_row[i];
        }
        return m_skipped_status;
    }
    m_skip_row = initial_fetch;

    if (initial_fetch) {
        m_first_row.clear();
        m_first_row.resize(sqlite3_column_count(m_stmt));
    }

    if (!m_stmt) {
        setLastError(QSqlError(
                         QCoreApplication::translate("SQLCipherResult", "Unable to fetch row"),
                         QCoreApplication::translate("SQLCipherResult", "No query"),
                         QSqlError::ConnectionError));
        setAt(QSql::AfterLastRow);
        return false;
    }
    res = sqlite3_step(m_stmt);

    switch(res) {
    case SQLITE_ROW:
        // check to see if should fill out columns
        if (m_r_inf.isEmpty()) {
            // must be first call.
            initColumns(false);
        }
        if (idx < 0 && !initial_fetch) {
            return true;
        }
        for (i = 0; i < m_r_inf.count(); ++i) {
            switch (sqlite3_column_type(m_stmt, i)) {
            case SQLITE_BLOB:
                values[i + idx] = QByteArray(static_cast<const char*>(
                            sqlite3_column_blob(m_stmt, i)),
                            sqlite3_column_bytes(m_stmt, i));
                break;
            case SQLITE_INTEGER:
                values[i + idx] = sqlite3_column_int64(m_stmt, i);
                break;
            case SQLITE_FLOAT:
                switch (numericalPrecisionPolicy()) {
                    case QSql::LowPrecisionInt32:
                        values[i + idx] = sqlite3_column_int(m_stmt, i);
                        break;
                    case QSql::LowPrecisionInt64:
                        values[i + idx] = sqlite3_column_int64(m_stmt, i);
                        break;
                    case QSql::LowPrecisionDouble:
                    case QSql::HighPrecision:
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
                    default:
#endif
                        values[i + idx] = sqlite3_column_double(m_stmt, i);
                        break;
                };
                break;
            case SQLITE_NULL:
                values[i + idx] = QVariant(QVariant::String);
                break;
            default:
                values[i + idx] = QString(reinterpret_cast<const QChar*>(
                            sqlite3_column_text16(m_stmt, i)),
                            sqlite3_column_bytes16(m_stmt, i) / sizeof(QChar));
                break;
            }
        }
        return true;
    case SQLITE_DONE:
        if (m_r_inf.isEmpty()) {
            // must be first call.
            initColumns(true);
        }
        setAt(QSql::AfterLastRow);
        sqlite3_reset(m_stmt);
        return false;
    case SQLITE_CONSTRAINT:
    case SQLITE_ERROR:
        // SQLITE_ERROR is a generic error code and we must call sqlite3_reset()
        // to get the specific error message.
        res = sqlite3_reset(m_stmt);
        setLastError(qMakeError(
                            cipherDriver()->m_access,
                            QCoreApplication::translate("SQLCipherResult",
                                                        "Unable to fetch row"),
                            QSqlError::ConnectionError,
                            res));
        setAt(QSql::AfterLastRow);
        return false;
    case SQLITE_MISUSE:
    case SQLITE_BUSY:
    default:
        // something wrong, don't get col info, but still return false
        setLastError(qMakeError(
                            cipherDriver()->m_access,
                            QCoreApplication::translate("SQLCipherResult",
                                                        "Unable to fetch row"),
                            QSqlError::ConnectionError,
                            res));
        sqlite3_reset(m_stmt);
        setAt(QSql::AfterLastRow);
        return false;
    }
    // will never get here: return false;
}


// ============================================================================
// Extra
// ============================================================================

const SQLCipherDriver* SQLCipherResult::cipherDriver() const
{
    const QSqlDriver* drv = driver();
    return !drv ? nullptr : reinterpret_cast<const SQLCipherDriver*>(drv);
}


// ============================================================================
// From SQLiteResult
// ============================================================================

SQLCipherResult::~SQLCipherResult()
{
    const SQLCipherDriver* cipherdriver = cipherDriver();
    if (cipherdriver) {
        cipherdriver->m_results.removeOne(this);
    }
    cleanup();
}


void SQLCipherResult::virtual_hook(int id, void* data)
{
    SqlCachedResult::virtual_hook(id, data);
}


bool SQLCipherResult::reset(const QString& query)
{
    if (!prepare(query)) {
        return false;
    }
    return exec();
}


bool SQLCipherResult::prepare(const QString& query)
{
    if (!driver() || !driver()->isOpen() || driver()->isOpenError()) {
        return false;
    }

    cleanup();

    setSelect(false);

    const void* pzTail = nullptr;

#if (SQLITE_VERSION_NUMBER >= 3003011)
    int res = sqlite3_prepare16_v2(
                cipherDriver()->m_access,
                query.constData(),
                (query.size() + 1) * sizeof(QChar),
                &m_stmt,
                &pzTail);
#else
    int res = sqlite3_prepare16(
                cipherDriver()->m_access,
                query.constData(),
                (query.size() + 1) * sizeof(QChar),
                &stmt,
                &pzTail);
#endif

    if (res != SQLITE_OK) {
        setLastError(qMakeError(
                         cipherDriver()->m_access,
                         QCoreApplication::translate("SQLCipherResult",
                                                     "Unable to execute statement"),
                         QSqlError::StatementError,
                         res));
        finalize();
        return false;
    }
    if (pzTail && !QString(reinterpret_cast<const QChar*>(pzTail)).trimmed().isEmpty()) {
        setLastError(qMakeError(
                         cipherDriver()->m_access,
                         QCoreApplication::translate("SQLCipherResult",
                                                     "Unable to execute multiple statements at a time"),
                         QSqlError::StatementError,
                         SQLITE_MISUSE));
        finalize();
        return false;
    }
    return true;
}


bool SQLCipherResult::exec()
{
    const QVector<QVariant> values = boundValues();

    m_skipped_status = false;
    m_skip_row = false;
    m_r_inf.clear();
    clearValues();
    setLastError(QSqlError());

    int res = sqlite3_reset(m_stmt);
    if (res != SQLITE_OK) {
        setLastError(qMakeError(
                         cipherDriver()->m_access,
                         QCoreApplication::translate("SQLCipherResult",
                                                     "Unable to reset statement"),
                         QSqlError::StatementError, res));

        finalize();
        return false;
    }
    int param_count = sqlite3_bind_parameter_count(m_stmt);
    if (param_count == values.count()) {
        for (int i = 0; i < param_count; ++i) {
            res = SQLITE_OK;
            const QVariant& value = values.at(i);

            if (value.isNull()) {
                res = sqlite3_bind_null(m_stmt, i + 1);
            } else {
                switch (value.type()) {
                case QVariant::ByteArray:
                {
                    auto ba = static_cast<const QByteArray*>(value.constData());
                    res = sqlite3_bind_blob(m_stmt, i + 1, ba->constData(),
                                            ba->size(), SQLITE_STATIC);
                    break;
                }
                case QVariant::Int:
                case QVariant::Bool:
                    res = sqlite3_bind_int(m_stmt, i + 1, value.toInt());
                    break;
                case QVariant::Double:
                    res = sqlite3_bind_double(m_stmt, i + 1, value.toDouble());
                    break;
                case QVariant::UInt:
                case QVariant::LongLong:
                    res = sqlite3_bind_int64(m_stmt, i + 1, value.toLongLong());
                    break;
                case QVariant::DateTime:
                {
                    const QDateTime dateTime = value.toDateTime();
                    const QString str = dateTime.toString(QStringLiteral("yyyy-MM-ddThh:mm:ss.zzz"));
                    res = sqlite3_bind_text16(m_stmt, i + 1, str.utf16(),
                                              str.size() * sizeof(ushort), SQLITE_TRANSIENT);
                    break;
                }
                case QVariant::Time:
                {
                    const QTime time = value.toTime();
                    const QString str = time.toString(QStringLiteral("hh:mm:ss.zzz"));
                    res = sqlite3_bind_text16(m_stmt, i + 1, str.utf16(),
                                              str.size() * sizeof(ushort), SQLITE_TRANSIENT);
                    break;
                }
                case QVariant::String:
                {
                    // lifetime of string == lifetime of its qvariant
                    auto str = static_cast<const QString*>(value.constData());
                    res = sqlite3_bind_text16(m_stmt, i + 1, str->utf16(),
                                              (str->size()) * sizeof(QChar), SQLITE_STATIC);
                    break;
                }
                default:
                {
                    QString str = value.toString();
                    // SQLITE_TRANSIENT makes sure that sqlite buffers the data
                    res = sqlite3_bind_text16(m_stmt, i + 1, str.utf16(),
                                              (str.size()) * sizeof(QChar), SQLITE_TRANSIENT);
                    break;
                }
                }
            }
            if (res != SQLITE_OK) {
                setLastError(qMakeError(
                                 cipherDriver()->m_access,
                                 QCoreApplication::translate("SQLCipherResult",
                                                             "Unable to bind parameters"),
                                 QSqlError::StatementError,
                                 res));
                finalize();
                return false;
            }
        }
    } else {
        setLastError(QSqlError(
                         QCoreApplication::translate("SQLCipherResult",
                                                     "Parameter count mismatch"),
                         QString(),
                         QSqlError::StatementError));
        return false;
    }
    m_skipped_status = fetchNext(m_first_row, 0, true);
    if (lastError().isValid()) {
        setSelect(false);
        setActive(false);
        return false;
    }
    setSelect(!m_r_inf.isEmpty());
    setActive(true);
    return true;
}


bool SQLCipherResult::gotoNext(SqlCachedResult::ValueCache& row, const int idx)
{
    return fetchNext(row, idx, false);
}


int SQLCipherResult::size()
{
    return -1;
}


int SQLCipherResult::numRowsAffected()
{
    return sqlite3_changes(cipherDriver()->m_access);
}


QVariant SQLCipherResult::lastInsertId() const
{
    if (isActive()) {
        qint64 id = sqlite3_last_insert_rowid(cipherDriver()->m_access);
        if (id) {
            return id;
        }
    }
    return QVariant();
}


QSqlRecord SQLCipherResult::record() const
{
    if (!isActive() || !isSelect()) {
        return QSqlRecord();
    }
    return m_r_inf;
}


void SQLCipherResult::detachFromResultSet()
{
    if (m_stmt) {
        sqlite3_reset(m_stmt);
    }
    // RNC:
    SqlCachedResult::detachFromResultSet();
}


QVariant SQLCipherResult::handle() const
{
    return QVariant::fromValue(reinterpret_cast<void*>(m_stmt));
}
