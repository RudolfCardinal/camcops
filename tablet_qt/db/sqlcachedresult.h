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
#include <QPointer>
#include <QSqlError>
#include <QSqlResult>
#include <QVariant>
#include <QVector>


// Non-private implementation of QSqlCachedResult / QSqlCachedResultPrivate

class SqlCachedResult : public QSqlResult
{
public:
    typedef QVector<QVariant> ValueCache;

protected:
    SqlCachedResult(const QSqlDriver* drv);

    void init(int col_count);
    void cleanup();
    void clearValues();

    virtual bool gotoNext(ValueCache& m_values, int index) = 0;

    QVariant data(int i) override;
    bool isNull(int i) override;
    bool fetch(int i) override;
    bool fetchNext() override;
    bool fetchPrevious() override;
    bool fetchFirst() override;
    bool fetchLast() override;

    int colCount() const;
    ValueCache& cache();

    void virtual_hook(int id, void* data) override;
    void detachFromResultSet() override;
    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy) override;
private:
    bool cacheNext();

    // ------------------------------------------------------------------------
    // These bits from QSqlCachedResultPrivate:
    // ------------------------------------------------------------------------
    bool canSeek(int i) const;
    inline int cacheCount() const;
    void init(int count, bool fo);
    int nextIndex();
    void revertLast();

    ValueCache m_cache;
    int m_row_cache_end;
    int m_col_count;
    bool m_at_end;

    // ------------------------------------------------------------------------
    // These bits from QSqlResultPrivate:
    // ------------------------------------------------------------------------
    struct QHolder {
        QHolder(const QString &hldr = QString(), int index = -1) :
            holder_name(hldr),
            holder_pos(index)
        {}
        bool operator==(const QHolder &h) const
        {
            return h.holder_pos == holder_pos && h.holder_name == holder_name;
        }
        bool operator!=(const QHolder &h) const
        {
            return h.holder_pos != holder_pos || h.holder_name != holder_name;
        }
        QString holder_name;
        int holder_pos;
    };

    void resetBindCount();
    void clearIndex();
    void clear();

    virtual QString fieldSerial(int) const;
    QString positionalToNamedBinding(const QString& query) const;
    QString namedToPositionalBinding(const QString& query);
    QString holderAt(int index) const;

    QPointer<QSqlDriver> m_sqldriver;
    // UNUSED, CLANG COMPLAINS // int m_idx;
    QString m_sql;
    // UNUSED, CLANG COMPLAINS // bool m_active;
    // UNUSED, CLANG COMPLAINS // bool m_is_sel;
    QSqlError m_error;
    bool m_forward_only;
    // UNUSED, CLANG COMPLAINS // QSql::NumericalPrecisionPolicy m_precision_policy;

    int m_bind_count;
    // UNUSED, CLANG COMPLAINS // QSqlResult::BindingSyntax m_binds;

    QString m_executed_query;
    QHash<int, QSql::ParamType> m_types;
    QVector<QVariant> m_values;
    typedef QHash<QString, QList<int>> IndexMap;
    IndexMap m_indexes;

    typedef QVector<QHolder> QHolderVector;
    QHolderVector m_holders;
};
