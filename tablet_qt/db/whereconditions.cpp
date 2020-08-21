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

#include "whereconditions.h"
#include <QStringList>
#include "db/dbfunc.h"
#include "db/sqlargs.h"
#include "lib/convert.h"


void WhereConditions::add(const QString& column, const QVariant& value)
{
    m_columns.append(column);
    m_operators.append("=");
    m_values.append(value);
}


void WhereConditions::add(const QString& column, const QString& op,
                          const QVariant& value)
{
    m_columns.append(column);
    m_operators.append(op);
    m_values.append(value);
}


void WhereConditions::set(const SqlArgs &sql_args)
{
    m_raw_sqlargs = sql_args;
}


void WhereConditions::appendWhereClauseTo(SqlArgs& sqlargs_altered) const
{
    if (!m_raw_sqlargs.sql.isEmpty()) {
        sqlargs_altered.sql += " WHERE " + m_raw_sqlargs.sql;
        sqlargs_altered.args += m_raw_sqlargs.args;
    } else {
        if (m_columns.isEmpty()) {
            return;
        }
        QStringList whereclauses;
        const int n = m_columns.size();
        Q_ASSERT(n == m_operators.size());
        Q_ASSERT(n == m_values.size());
        for (int i = 0; i < n; ++i) {
            whereclauses.append(dbfunc::delimit(m_columns.at(i)) +
                                " " + m_operators.at(i) + " ?");
            sqlargs_altered.args.append(m_values.at(i));
        }
        sqlargs_altered.sql += " WHERE " + whereclauses.join(" AND ");
    }
}


QString WhereConditions::whereLiteralForDebuggingOnly() const
{
    if (!m_raw_sqlargs.sql.isEmpty()) {
        return "WHERE " + m_raw_sqlargs.literalForDebuggingOnly();
    } else {
        if (m_columns.isEmpty()) {
            return "";
        }
        QStringList whereclauses;
        const int n = m_columns.size();
        Q_ASSERT(n == m_operators.size());
        Q_ASSERT(n == m_values.size());
        for (int i = 0; i < n; ++i) {
            whereclauses.append(dbfunc::delimit(m_columns.at(i)) +
                                " " + m_operators.at(i) + " " +
                                convert::toSqlLiteral(m_values.at(i)));
        }
        return "WHERE " + whereclauses.join(" AND ");
    }
}


// ========================================================================
// For friends
// ========================================================================

QDebug operator<<(QDebug debug, const WhereConditions& w)
{
    debug << w.whereLiteralForDebuggingOnly();
    return debug;
}
