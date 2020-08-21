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


// Represents an ID number type, e.g. "type 3 means NHS number".

class IdNumDescription : public DatabaseObject
{
    Q_OBJECT

public:
    IdNumDescription(CamcopsApp& app, DatabaseManager& db,
                     int which_idnum = dbconst::NONEXISTENT_PK);

    // Returns the ID number type (e.g. 3)
    int whichIdNum() const;

    // Returns the description (e.g. "NHS number")
    QString description() const;

    // Returns the short description (e.g. "NHS")
    QString shortDescription() const;

    // Sets the descriptions and validation method
    bool setDescriptions(const QString& desc, const QString& shortdesc,
                         const QString& validation_method);

    // Returns the validation method, if specified; see e.g.
    // VALIDATION_METHOD_UK_NHS_NUMBER.
    QString validationMethod() const;

    // Delete all descriptions from the database.
    // (Resembles a Python classmethod; sort-of static function.)
    void deleteAllDescriptions();

    // Make table indexes.
    // (Resembles a Python classmethod; sort-of static function.)
    void makeIndexes();

    // Should it be validated as an NHS number?
    bool validateAsNhsNumber() const;

public:
    static const QString IDNUMDESC_TABLENAME;
    static const QString FN_IDNUM;
};
