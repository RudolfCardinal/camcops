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

#include "patientidnum.h"

const QString PatientIdNum::PATIENT_IDNUM_TABLENAME("patient_idnum");
const QString PatientIdNum::FK_PATIENT("patient_id");
const QString PatientIdNum::FN_WHICH_IDNUM("which_idnum");
const QString PatientIdNum::FN_IDNUM_VALUE("idnum_value");


PatientIdNum::PatientIdNum(CamcopsApp& app, DatabaseManager& db,
                           const int load_pk) :
    DatabaseObject(app, db, PATIENT_IDNUM_TABLENAME,
                   dbconst::PK_FIELDNAME,  // pk_fieldname
                   true,  // has_modification_timestamp
                   false,  // has_creation_timestamp
                   true,  // has_move_off_tablet_field
                   true)  // triggers_need_upload
{
    addField(FK_PATIENT, QVariant::Int, true);
    addField(FN_WHICH_IDNUM, QVariant::Int, true);
    addField(FN_IDNUM_VALUE, QVariant::LongLong);

    load(load_pk);
}


PatientIdNum::PatientIdNum(const int patient_fk, const int which_idnum,
                           CamcopsApp& app, DatabaseManager& db) :
    PatientIdNum(app, db)  // delegating constructor
{
    setValue(FK_PATIENT, patient_fk);
    setValue(FN_WHICH_IDNUM, which_idnum);
    save();
}


int PatientIdNum::whichIdNum() const
{
    return valueInt(FN_WHICH_IDNUM);
}


QVariant PatientIdNum::idnumAsVariant() const
{
    return value(FN_IDNUM_VALUE);
}


qint64 PatientIdNum::idnumAsInteger() const
{
    return valueInt64(FN_IDNUM_VALUE);
}


QString PatientIdNum::idnumAsString() const
{
    const QVariant var = idnumAsVariant();
    if (var.isNull()) {
        return "?";
    }
    return QString::number(var.toULongLong());
}


bool PatientIdNum::idnumIsPresent() const
{
    return !idnumAsVariant().isNull();
}


bool PatientIdNum::setIdnumValue(const qint64 idnum_value,
                                 const bool save_to_db)
{
    bool success = setValue(FN_IDNUM_VALUE, idnum_value);
    if (save_to_db) {
        success = save() && success;
    }
    return success;
}
