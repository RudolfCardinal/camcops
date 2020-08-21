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
#include "common/aliases_qt.h"


// Represents SQL with an associated list of argument values.

struct SqlArgs {
public:
    SqlArgs(const QString& sql = "", const ArgList& args = ArgList()) :
        sql(sql), args(args) {}

    // Returns an SQL literal with realized parameters -- NOT for proper use
    // (risk of SQL injection).
    QString literalForDebuggingOnly() const;

public:
    // The SQL, with "?" parameter placeholders.
    QString sql;

    // The arguments.
    ArgList args;

public:
    friend QDebug operator<<(QDebug debug, const SqlArgs& s);
};
