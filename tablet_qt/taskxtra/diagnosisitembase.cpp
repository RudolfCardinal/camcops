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

#include "diagnosisitembase.h"

const QString DiagnosisItemBase::SEQNUM("seqnum");
const QString DiagnosisItemBase::CODE("code");
const QString DiagnosisItemBase::DESCRIPTION("description");
const QString DiagnosisItemBase::COMMENT("comment");  // new in v2.0.0


DiagnosisItemBase::DiagnosisItemBase(CamcopsApp& app, DatabaseManager& db,
                                     const QString& tablename,
                                     const QString& fkname,
                                     const int load_pk) :
    DatabaseObject(app, db, tablename,
                   dbconst::PK_FIELDNAME,  // pk_fieldname
                   true,  // has_modification_timestamp
                   false,  // has_creation_timestamp
                   true,  // has_move_off_tablet_field
                   true),  // triggers_need_upload
    m_fkname(fkname)
{
    addField(m_fkname, QVariant::Int);
    addField(SEQNUM, QVariant::Int);
    addField(CODE, QVariant::String);
    addField(DESCRIPTION, QVariant::String);
    addField(COMMENT, QVariant::String);  // new in v2.0.0

    load(load_pk);
}


DiagnosisItemBase::DiagnosisItemBase(const int owner_fk,
                                     CamcopsApp& app, DatabaseManager& db,
                                     const QString& tablename,
                                     const QString& fkname) :
    DiagnosisItemBase(app, db, tablename, fkname)  // delegating constructor
{
    setValue(m_fkname, owner_fk);
}


void DiagnosisItemBase::setSeqnum(const int seqnum)
{
    setValue(SEQNUM, seqnum);
}


int DiagnosisItemBase::seqnum() const
{
    return valueInt(SEQNUM);
}


QString DiagnosisItemBase::code() const
{
    return valueString(CODE);
}


QString DiagnosisItemBase::description() const
{
    return valueString(DESCRIPTION);
}


QString DiagnosisItemBase::comment() const
{
    return valueString(COMMENT);
}


bool DiagnosisItemBase::isEmpty() const
{
    return code().isEmpty();
}
