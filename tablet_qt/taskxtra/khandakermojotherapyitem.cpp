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

#include "khandakermojotherapyitem.h"

const QString KhandakerMojoTherapyItem::KHANDAKER2MOJOTHERAPYITEM_TABLENAME("khandaker_mojo_therapy_item");

const QString KhandakerMojoTherapyItem::FN_FK_NAME("medicationtable_id");
const QString KhandakerMojoTherapyItem::FN_SEQNUM("seqnum");
const QString KhandakerMojoTherapyItem::FN_THERAPY("therapy");
const QString KhandakerMojoTherapyItem::FN_FREQUENCY("frequency");
const QString KhandakerMojoTherapyItem::FN_SESSIONS_COMPLETED("sessions_completed");
const QString KhandakerMojoTherapyItem::FN_SESSIONS_PLANNED("sessions_planned");
const QString KhandakerMojoTherapyItem::FN_INDICATION("indication");
const QString KhandakerMojoTherapyItem::FN_RESPONSE("response");

const QStringList KhandakerMojoTherapyItem::TABLE_FIELDNAMES{
    KhandakerMojoTherapyItem::FN_THERAPY,
    KhandakerMojoTherapyItem::FN_FREQUENCY,
    KhandakerMojoTherapyItem::FN_SESSIONS_COMPLETED,
    KhandakerMojoTherapyItem::FN_SESSIONS_PLANNED,
    KhandakerMojoTherapyItem::FN_INDICATION,
    KhandakerMojoTherapyItem::FN_RESPONSE,
};


KhandakerMojoTherapyItem::KhandakerMojoTherapyItem(
    CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    DatabaseObject(app, db, KHANDAKER2MOJOTHERAPYITEM_TABLENAME,
                   dbconst::PK_FIELDNAME,  // pk_fieldname
                   true,  // has_modification_timestamp
                   false,  // has_creation_timestamp
                   true,  // has_move_off_tablet_field
                   true)  // triggers_need_upload
{
    addField(FN_FK_NAME, QVariant::Int);
    addField(FN_SEQNUM, QVariant::Int);
    addField(FN_THERAPY, QVariant::String);
    addField(FN_FREQUENCY, QVariant::String);
    addField(FN_SESSIONS_COMPLETED, QVariant::Int);
    addField(FN_SESSIONS_PLANNED, QVariant::Int);
    addField(FN_INDICATION, QVariant::String);
    addField(FN_RESPONSE, QVariant::Int);

    load(load_pk);
}


KhandakerMojoTherapyItem::KhandakerMojoTherapyItem(
    const int owner_fk, CamcopsApp& app, DatabaseManager& db) :
    KhandakerMojoTherapyItem(app, db)  // delegating constructor
{
    setValue(FN_FK_NAME, owner_fk);
}


void KhandakerMojoTherapyItem::setSeqnum(const int seqnum)
{
    setValue(FN_SEQNUM, seqnum);
}


bool KhandakerMojoTherapyItem::isComplete() const
{
    return noValuesNull(TABLE_FIELDNAMES);
}


bool KhandakerMojoTherapyItem::isEmpty() const
{
    for (const QString& fieldname : TABLE_FIELDNAMES) {
        if (!valueIsNullOrEmpty(fieldname)) {
            return false;
        }
    }

    return true;
}
