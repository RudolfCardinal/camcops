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

#pragma once
#include <QSqlRecord>
#include "db/sqlcachedresult.h"

struct sqlite3_stmt;
class SQLCipherDriver;


class SQLCipherResult : public SqlCachedResult
{
    friend class SQLCipherDriver;

public:
    explicit SQLCipherResult(const SQLCipherDriver* drv);
    ~SQLCipherResult() override;
    QVariant handle() const override;

protected:
    bool gotoNext(SqlCachedResult::ValueCache& row, int idx) override;
    bool reset(const QString& query) override;
    bool prepare(const QString& query) override;
    bool exec() override;
    int size() override;
    int numRowsAffected() override;
    QVariant lastInsertId() const override;
    QSqlRecord record() const override;
    void detachFromResultSet() override;
    void virtual_hook(int id, void* data) override;

    // ------------------------------------------------------------------------
    // Extra:
    // ------------------------------------------------------------------------
    const SQLCipherDriver* cipherDriver() const;

    // ------------------------------------------------------------------------
    // From SQLiteResultPrivate:
    // ------------------------------------------------------------------------
    void cleanup();
    using SqlCachedResult::fetchNext;  // prevent Clang warning "... hides overloaded virtual function..."
    bool fetchNext(SqlCachedResult::ValueCache& values, int idx,
                   bool initial_fetch);
    // initializes the recordInfo and the cache
    void initColumns(bool emptyResultset);
    void finalize();

    sqlite3_stmt* m_stmt;

    bool m_skipped_status; // the status of the fetchNext() that's skipped
    bool m_skip_row; // skip the next fetchNext()?
    QSqlRecord m_r_inf;
    QVector<QVariant> m_first_row;
};
