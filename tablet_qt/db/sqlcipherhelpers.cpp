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

#include "sqlcipherhelpers.h"
#include <QSqlField>
#include <QSqlQuery>

#ifdef MODIFIED_FROM_SQLITE
#include "db/whichdb.h"
#ifdef USE_SQLCIPHER
#include <sqlcipher/sqlite3.h>
#endif
#else
#include <sqlite3.h>
#endif


namespace sqlcipherhelpers
{


QString _q_escapeIdentifier(const QString& identifier)
{
    QString res = identifier;
    if (!identifier.isEmpty() &&
            identifier.left(1) != QString(QLatin1Char('"')) &&
            identifier.right(1) != QString(QLatin1Char('"')) ) {
        res.replace(QLatin1Char('"'), QLatin1String("\"\""));
        res.prepend(QLatin1Char('"')).append(QLatin1Char('"'));
        res.replace(QLatin1Char('.'), QLatin1String("\".\""));
    }
    return res;
}


QVariant::Type qGetColumnType(const QString& type_name)
{
    const QString tn = type_name.toLower();

    if (tn == QLatin1String("integer")
        || tn == QLatin1String("int")) {
        return QVariant::Int;
    }
    if (tn == QLatin1String("double")
        || tn == QLatin1String("float")
        || tn == QLatin1String("real")
        || tn.startsWith(QLatin1String("numeric"))) {
        return QVariant::Double;
    }
    if (tn == QLatin1String("blob")) {
        return QVariant::ByteArray;
    }
    if (tn == QLatin1String("boolean")
        || tn == QLatin1String("bool")) {
        return QVariant::Bool;
    }
    return QVariant::String;
}


QSqlError qMakeError(sqlite3* access, const QString& descr,
                     QSqlError::ErrorType type, int errorCode)
{
    return QSqlError(
                descr,
                QString(reinterpret_cast<const QChar*>(sqlite3_errmsg16(access))),
                type,
                QString::number(errorCode));
}


QSqlIndex qGetTableInfo(QSqlQuery& q, const QString& table_name,
                        const bool only_p_index)
{
    QString schema;
    QString table(table_name);
    int index_of_separator = table_name.indexOf(QLatin1Char('.'));
    if (index_of_separator > -1) {
        schema = table_name.left(index_of_separator).append(QLatin1Char('.'));
        table = table_name.mid(index_of_separator + 1);
    }
    q.exec(QLatin1String("PRAGMA ") + schema + QLatin1String("table_info (") +
           _q_escapeIdentifier(table) + QLatin1Char(')'));

    QSqlIndex ind;
    while (q.next()) {
        bool is_pk = q.value(5).toInt();
        if (only_p_index && !is_pk) {
            continue;
        }
        QString type_name = q.value(2).toString().toLower();
        QSqlField fld(q.value(1).toString(), qGetColumnType(type_name));
        if (is_pk && (type_name == QLatin1String("integer"))) {
            // INTEGER PRIMARY KEY fields are auto-generated in sqlite
            // INT PRIMARY KEY is not the same as INTEGER PRIMARY KEY!
            fld.setAutoValue(true);
        }
        fld.setRequired(q.value(3).toInt() != 0);
        fld.setDefaultValue(q.value(4));
        ind.append(fld);
    }
    return ind;
}


}  // namespace sqlcipherhelpers
