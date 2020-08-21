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
#include "db/databaseobject.h"
class CamcopsApp;


// Represents a config variable stored in the system database for a CamcopsApp.

class StoredVar : public DatabaseObject
{
public:
    StoredVar(CamcopsApp& app,
              DatabaseManager& db,
              const QString& name = "",  // empty for a specimen
              QVariant::Type type = QVariant::Int,
              const QVariant& default_value = QVariant());
    virtual ~StoredVar() = default;

    // Sets the value.
    bool setValue(const QVariant& value, bool save_to_db = true);

    // Returns the value as a QVariant.
    QVariant value() const;

    // Returns the variable's name.
    QString name() const;

    // Makes indexes for the table.
    void makeIndexes();

protected:
    // http://stackoverflow.com/questions/411103/function-with-same-name-but-different-signature-in-derived-class
    // http://stackoverflow.com/questions/1628768/why-does-an-overridden-function-in-the-derived-class-hide-other-overloads-of-the
    using DatabaseObject::setValue;
    using DatabaseObject::value;

protected:
    // The name. Only for name(), really.
    QString m_name;

    // What QVariant type are we representing?
    QVariant::Type m_type;

    // Which field is in active use?
    QString m_value_fieldname;
};
