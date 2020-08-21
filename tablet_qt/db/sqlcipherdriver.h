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

/*
    Driver for Qt for SQLCiper, a cryptographic version of SQLite.

    Qt driver for SQLite: see qtbase/src/sql/drivers/sqlite
    [which is the SQLite 3 driver; ignore .../sqlite2]
        ... qsql_sqlite_p.h
        ... qsql_sqlite.cpp
        ... +/- qsql_sqlite.pri [to control builds]

    SQLCipher: https://www.zetetic.net/sqlcipher/

    Somebody else's Qt driver: https://github.com/sijk/qt5-sqlcipher
    ... uses qtbase5-private-dev
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


#pragma once
#include <QSqlDriver>

struct sqlite3;
class QSqlResult;
class SQLCipherResult;


// Qt driver for SQLCipher

class SQLCipherDriver : public QSqlDriver
{
    Q_OBJECT
    friend class SQLCipherResult;
public:
    explicit SQLCipherDriver(QObject* parent = nullptr);
    explicit SQLCipherDriver(sqlite3* connection, QObject* parent = nullptr);
    ~SQLCipherDriver() override;
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString& db,
              const QString& user,
              const QString& password,
              const QString& host,
              int port,
              const QString& conn_opts) override;
    void close() override;
    QSqlResult* createResult() const override;
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
    QStringList tables(QSql::TableType) const override;

    QSqlRecord record(const QString& tablename) const override;
    QSqlIndex primaryIndex(const QString& table) const override;
    QVariant handle() const override;
    QString escapeIdentifier(const QString& identifier, IdentifierType) const override;

    // ------------------------------------------------------------------------
    // From QSQLiteDriverPrivate:
    // ------------------------------------------------------------------------
protected:
    sqlite3* m_access;
    mutable QList<SQLCipherResult*> m_results;

};
