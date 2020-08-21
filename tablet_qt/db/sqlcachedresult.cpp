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

#include "sqlcachedresult.h"
#include <QDateTime>
#include <QSqlDriver>

/*
   QSqlCachedResult is a convenience class for databases that only allow
   forward only fetching. It will cache all the results so we can iterate
   backwards over the results again.

   All you need to do is to inherit from QSqlCachedResult and reimplement
   gotoNext(). gotoNext() will have a reference to the internal cache and
   will give you an index where you can start filling in your data. Special
   case: If the user actually wants a forward-only query, idx will be -1
   to indicate that we are not interested in the actual values.
*/

static const uint initial_cache_size = 128;

// ============================================================================
// Helper functions
// ============================================================================

static bool qIsAlnum(const QChar ch)
{
    const auto u = uint(ch.unicode());
    // matches [a-zA-Z0-9_]
    return u - 'a' < 26 || u - 'A' < 26 || u - '0' < 10 || u == '_';
}


// ============================================================================
// From QSqlResultPrivate
// ============================================================================

void SqlCachedResult::resetBindCount()
{
    m_bind_count = 0;
}


void SqlCachedResult::clearIndex()
{
    m_indexes.clear();
    m_holders.clear();
    m_types.clear();
}


void SqlCachedResult::clear()
{
    clearValues();
    clearIndex();
}


QString SqlCachedResult::holderAt(const int index) const
{
    return m_holders.size() > index ? m_holders.at(index).holder_name
                                    : fieldSerial(index);
}


// return a unique id for bound names
QString SqlCachedResult::fieldSerial(int i) const
{
    ushort arr[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ushort* end = &arr[(sizeof(arr) / sizeof(*arr))];
    ushort* ptr = end;

    while (i > 0) {
        *(--ptr) = 'a' + i % 16;
        i >>= 4;
    }

    const int nb = end - ptr;
    *(--ptr) = 'a' + nb;
    *(--ptr) = ':';

    return QString::fromUtf16(ptr, int(end - ptr));
}


QString SqlCachedResult::positionalToNamedBinding(const QString& query) const
{
    const int n = query.size();

    QString result;
    result.reserve(n * 5 / 4);
    QChar closing_quote;
    int count = 0;
    const bool ignore_braces = (m_sqldriver->dbmsType() == QSqlDriver::PostgreSQL);

    for (int i = 0; i < n; ++i) {
        const QChar ch = query.at(i);
        if (!closing_quote.isNull()) {
            if (ch == closing_quote) {
                if (closing_quote == QLatin1Char(']') &&
                        i + 1 < n && query.at(i + 1) == closing_quote) {
                    // consume the extra character. don't close.
                    ++i;
                    result += ch;
                } else {
                    closing_quote = QChar();
                }
            }
            result += ch;
        } else {
            if (ch == QLatin1Char('?')) {
                result += fieldSerial(count++);
            } else {
                if (ch == QLatin1Char('\'') ||
                        ch == QLatin1Char('"') ||
                        ch == QLatin1Char('`')) {
                    closing_quote = ch;
                } else if (!ignore_braces && ch == QLatin1Char('[')) {
                    closing_quote = QLatin1Char(']');
                }
                result += ch;
            }
        }
    }
    result.squeeze();
    return result;
}


QString SqlCachedResult::namedToPositionalBinding(const QString& query)
{
    const int n = query.size();

    QString result;
    result.reserve(n);
    QChar closing_quote;
    int count = 0;
    int i = 0;
    const bool ignore_braces = (m_sqldriver->dbmsType() == QSqlDriver::PostgreSQL);

    while (i < n) {
        const QChar ch = query.at(i);
        if (!closing_quote.isNull()) {
            if (ch == closing_quote) {
                if (closing_quote == QLatin1Char(']')
                        && i + 1 < n && query.at(i + 1) == closing_quote) {
                    // consume the extra character. don't close.
                    ++i;
                    result += ch;
                } else {
                    closing_quote = QChar();
                }
            }
            result += ch;
            ++i;
        } else {
            if (ch == QLatin1Char(':') &&
                    (i == 0 || query.at(i - 1) != QLatin1Char(':')) &&
                    (i + 1 < n && qIsAlnum(query.at(i + 1)))) {
                int pos = i + 2;
                while (pos < n && qIsAlnum(query.at(pos))) {
                    ++pos;
                }
                QString holder(query.mid(i, pos - i));
                m_indexes[holder].append(count++);
                m_holders.append(QHolder(holder, i));
                result += QLatin1Char('?');
                i = pos;
            } else {
                if (ch == QLatin1Char('\'') ||
                        ch == QLatin1Char('"') ||
                        ch == QLatin1Char('`')) {
                    closing_quote = ch;
                } else if (!ignore_braces && ch == QLatin1Char('[')) {
                    closing_quote = QLatin1Char(']');
                }
                result += ch;
                ++i;
            }
        }
    }
    result.squeeze();
    m_values.resize(m_holders.size());
    return result;
}


// ============================================================================
// From combination of QSqlResultPrivate + SqlCachedResult
// ============================================================================

void SqlCachedResult::clearValues()
{
    setAt(QSql::BeforeFirstRow);
    m_row_cache_end = 0;
    m_at_end = false;

    m_values.clear();
    m_bind_count = 0;
}


// ============================================================================
// From combination of QSqlCachedResult + QSqlCachedResultPrivate
// ============================================================================

SqlCachedResult::SqlCachedResult(const QSqlDriver* drv) :
    QSqlResult(drv),
    m_row_cache_end(0),
    m_col_count(0),
    m_at_end(false)
{
}


void SqlCachedResult::cleanup()
{
    setAt(QSql::BeforeFirstRow);
    setActive(false);

    m_cache.clear();
    m_at_end = false;
    m_col_count = 0;
    m_row_cache_end = 0;
}


// ============================================================================
// From QSqlCachedResultPrivate
// ============================================================================

void SqlCachedResult::init(const int count, const bool fo)
{
    Q_ASSERT(count);
    cleanup();
    m_forward_only = fo;
    m_col_count = count;
    if (fo) {
        m_cache.resize(count);
        m_row_cache_end = count;
    } else {
        m_cache.resize(initial_cache_size * count);
    }
}


int SqlCachedResult::nextIndex()
{
    if (m_forward_only) {
        return 0;
    }
    int newIdx = m_row_cache_end;
    if (newIdx + m_col_count > m_cache.size()) {
        m_cache.resize(qMin(m_cache.size() * 2, m_cache.size() + 10000));
    }
    m_row_cache_end += m_col_count;

    return newIdx;
}


bool SqlCachedResult::canSeek(const int i) const
{
    if (m_forward_only || i < 0) {
        return false;
    }
    return m_row_cache_end >= (i + 1) * m_col_count;
}


void SqlCachedResult::revertLast()
{
    if (m_forward_only) {
        return;
    }
    m_row_cache_end -= m_col_count;
}


inline int SqlCachedResult::cacheCount() const
{
    Q_ASSERT(!m_forward_only);
    Q_ASSERT(m_col_count);
    return m_row_cache_end / m_col_count;
}



// ============================================================================
// From QSqlCachedResult
// ============================================================================

void SqlCachedResult::init(const int colCount)
{
    init(colCount, isForwardOnly());
}


bool SqlCachedResult::fetch(const int i)
{
    if ((!isActive()) || (i < 0)) {
        return false;
    }
    if (at() == i) {
        return true;
    }
    if (m_forward_only) {
        // speed hack - do not copy values if not needed
        if (at() > i || at() == QSql::AfterLastRow) {
            return false;
        }
        while (at() < i - 1) {
            if (!gotoNext(m_cache, -1)) {
                return false;
            }
            setAt(at() + 1);
        }
        if (!gotoNext(m_cache, 0)) {
            return false;
        }
        setAt(at() + 1);
        return true;
    }
    if (canSeek(i)) {
        setAt(i);
        return true;
    }
    if (m_row_cache_end > 0) {
        setAt(cacheCount());
    }
    while (at() < i + 1) {
        if (!cacheNext()) {
            if (canSeek(i)) {
                break;
            }
            return false;
        }
    }
    setAt(i);

    return true;
}


bool SqlCachedResult::fetchNext()
{
    if (canSeek(at() + 1)) {
        setAt(at() + 1);
        return true;
    }
    return cacheNext();
}


bool SqlCachedResult::fetchPrevious()
{
    return fetch(at() - 1);
}


bool SqlCachedResult::fetchFirst()
{
    if (m_forward_only && at() != QSql::BeforeFirstRow) {
        return false;
    }
    if (canSeek(0)) {
        setAt(0);
        return true;
    }
    return cacheNext();
}


bool SqlCachedResult::fetchLast()
{
    if (m_at_end) {
        if (m_forward_only) {
            return false;
        }
        return fetch(cacheCount() - 1);
    }

    int i = at();
    while (fetchNext()) {
        ++i; // brute force
    }
    if (m_forward_only && at() == QSql::AfterLastRow) {
        setAt(i);
        return true;
    }
    return fetch(i);
}


QVariant SqlCachedResult::data(const int i)
{
    const int idx = m_forward_only ? i : at() * m_col_count + i;
    if (i >= m_col_count || i < 0 || at() < 0 || idx >= m_row_cache_end) {
        return QVariant();
    }

    return m_cache.at(idx);
}


bool SqlCachedResult::isNull(const int i)
{
    const int idx = m_forward_only ? i : at() * m_col_count + i;
    if (i >= m_col_count || i < 0 || at() < 0 || idx >= m_row_cache_end) {
        return true;
    }

    return m_cache.at(idx).isNull();
}


bool SqlCachedResult::cacheNext()
{
    if (m_at_end) {
        return false;
    }

    if (isForwardOnly()) {
        m_cache.resize(m_col_count);
    }

    if (!gotoNext(m_cache, nextIndex())) {
        revertLast();
        m_at_end = true;
        return false;
    }
    setAt(at() + 1);
    return true;
}


int SqlCachedResult::colCount() const
{
    return m_col_count;
}


SqlCachedResult::ValueCache& SqlCachedResult::cache()
{
    return m_cache;
}


void SqlCachedResult::virtual_hook(const int id, void* data)
{
    QSqlResult::virtual_hook(id, data);
}


void SqlCachedResult::detachFromResultSet()
{
    cleanup();
}


void SqlCachedResult::setNumericalPrecisionPolicy(
        QSql::NumericalPrecisionPolicy policy)
{
    QSqlResult::setNumericalPrecisionPolicy(policy);
    cleanup();
}
