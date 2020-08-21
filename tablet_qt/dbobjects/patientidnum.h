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


// Represents a patient ID number.

class PatientIdNum : public DatabaseObject
{
    Q_OBJECT
public:
    // Normal constructor; loads if required.
    PatientIdNum(CamcopsApp& app, DatabaseManager& db,
                 int load_pk = dbconst::NONEXISTENT_PK);

    // Create-and-save constructor.
    PatientIdNum(int patient_fk, int which_idnum,
                 CamcopsApp& app, DatabaseManager& db);

    // Returns the ID number type (e.g. "3 meaning NHS number")
    int whichIdNum() const;

    // Returns the ID number value (e.g. 9876543210) as a QVariant.
    QVariant idnumAsVariant() const;

    // Returns the ID number value as a qint64 (qlonglong).
    // 64-bit signed integer; therefore up to +9,223,372,036,854,775,807
    // NOTE that SQLite3 can't handle unsigned 64-bit integers in plain types;
    // see https://www.sqlite.org/datatype3.html
    qint64 idnumAsInteger() const;

    // Returns the ID number value as a string.
    QString idnumAsString() const;

    // Is an ID number present?
    bool idnumIsPresent() const;

    // Sets the ID number
    bool setIdnumValue(qint64 idnum_value, bool save_to_db = true);

public:
    static const QString PATIENT_IDNUM_TABLENAME;
    static const QString FK_PATIENT;
    static const QString FN_WHICH_IDNUM;
    static const QString FN_IDNUM_VALUE;
};
