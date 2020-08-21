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
#include <QString>
#include <QVariant>
#include <QVector>
#include "db/sqlargs.h"

// Represents the WHERE clause of an SQL query/command.

class WhereConditions
{
public:
    WhereConditions() = default;

    // Adds a condition: "WHERE ... [AND] <column> = <value>"
    void add(const QString& column, const QVariant& value);  // op: "="

    // Adds a condition: "WHERE ... [AND] <column> <op> <value>"
    void add(const QString& column, const QString& op,
             const QVariant& value);

    // Sets the WHERE clause by hand. Overrides the "add" methods.
    void set(const SqlArgs& sql_args);

    // Modifies the SQL in the supplied SqlArgs object to add the WHERE.
    void appendWhereClauseTo(SqlArgs& sqlargs_altered) const;

    // Returns an SQL literal with realized parameters -- NOT for proper use
    // (risk of SQL injection).
    QString whereLiteralForDebuggingOnly() const;

protected:
    // Column names
    QVector<QString> m_columns;

    // Operators, e.g. "="
    QVector<QString> m_operators;

    // Values
    QVector<QVariant> m_values;

    // Raw SQL and arguments
    SqlArgs m_raw_sqlargs;

public:
    // Debugging description
    friend QDebug operator<<(QDebug debug, const WhereConditions& w);
};
