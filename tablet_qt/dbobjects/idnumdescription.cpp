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

#include "idnumdescription.h"
#include "common/dbconst.h"
#include "db/databasemanager.h"

const QString IdNumDescription::IDNUMDESC_TABLENAME("idnum_descriptions");
const QString IdNumDescription::FN_IDNUM("idnum");
const QString FN_IDDESC("iddesc");
const QString FN_IDSHORTDESC("idshortdesc");
const QString FN_VALIDATION_METHOD("validation_method");  // new in v2.2.8

// Must match camcops_server.cc_modules.cc_idnumdef.IdNumValidationMethod:
const QString VALIDATION_METHOD_UK_NHS_NUMBER("uk_nhs_number");


IdNumDescription::IdNumDescription(CamcopsApp& app, DatabaseManager& db,
                                   const int which_idnum) :
    DatabaseObject(app, db, IDNUMDESC_TABLENAME,
                   FN_IDNUM,  // pk
                   false, false, false, false)  // internal tables only
{
    addField(FN_IDDESC, QVariant::String);
    addField(FN_IDSHORTDESC, QVariant::String);
    addField(FN_VALIDATION_METHOD, QVariant::String);

    load(which_idnum);
    if (!m_exists_in_db) {
        // PK will have been nullified, but we want it
        setValue(FN_IDNUM, which_idnum);
    }
}


int IdNumDescription::whichIdNum() const
{
    return valueInt(FN_IDNUM);
}


QString IdNumDescription::description() const
{
    if (!m_exists_in_db) {
        return dbconst::UNKNOWN_IDNUM_DESC.arg(valueInt(FN_IDNUM));
    }
    return valueString(FN_IDDESC);
}


QString IdNumDescription::shortDescription() const
{
    if (!m_exists_in_db) {
        return dbconst::UNKNOWN_IDNUM_DESC.arg(valueInt(FN_IDNUM));
    }
    return valueString(FN_IDSHORTDESC);
}


QString IdNumDescription::validationMethod() const
{
    if (!m_exists_in_db) {
        return "";
    }
    return valueString(FN_VALIDATION_METHOD);
}


bool IdNumDescription::setDescriptions(const QString& desc,
                                       const QString& shortdesc,
                                       const QString& validation_method)
{
    bool success = setValue(FN_IDDESC, desc);
    success = setValue(FN_IDSHORTDESC, shortdesc) && success;
    success = setValue(FN_VALIDATION_METHOD, validation_method) && success;
    return success;
}


void IdNumDescription::deleteAllDescriptions()
{
    m_db.deleteFrom(IDNUMDESC_TABLENAME);
}


void IdNumDescription::makeIndexes()
{
    m_db.createIndex("_idx_idnumdesc_idnum",
                     IDNUMDESC_TABLENAME,
                     {FN_IDNUM});
}


bool IdNumDescription::validateAsNhsNumber() const
{
    return validationMethod() == VALIDATION_METHOD_UK_NHS_NUMBER;
}
