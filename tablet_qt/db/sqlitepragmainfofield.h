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
#include <QString>
#include <QVariant>


// Represents the information from SQLite's "PRAGMA table_info" command.

class SqlitePragmaInfoField {
public:
    // http://www.stroustrup.com/C++11FAQ.html#member-init
    int cid = -1;  // column ID
    QString name;  // column name
    QString type;  // SQL type
    bool notnull = false;  // NOT NULL constraint?
    QVariant dflt_value;  // database default value
    bool pk = false;  // PRIMARY KEY?
public:
    // Debugging description
    friend QDebug operator<<(QDebug debug, const SqlitePragmaInfoField& info);
};
