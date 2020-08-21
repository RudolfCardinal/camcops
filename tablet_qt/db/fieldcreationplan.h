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
class Field;


// Describes the way in which we'd like to change a field/column when
// modifying a database.

class FieldCreationPlan {
public:
    // Field name
    QString name;

    // What we're aiming for.
    const Field* intended_field = nullptr;

    // Does the field already exist?
    bool exists_in_db = false;

    // Existing SQL type.
    QString existing_type;

    // Is the existing field NOT NULL?
    bool existing_not_null = false;

    // Are we adding this field?
    bool add = false;

    // Are we dropping this field?
    bool drop = false;

    // Are we modifying this field?
    bool change = false;
public:
    friend QDebug operator<<(QDebug debug, const FieldCreationPlan& plan);
};
