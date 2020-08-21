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

#include "khandakermojomedicationitem.h"

const QString KhandakerMojoMedicationItem::KHANDAKERMOJOMEDICATIONITEM_TABLENAME("khandaker_mojo_medication_item");

const QString KhandakerMojoMedicationItem::FN_FK_NAME("medicationtable_id");
const QString KhandakerMojoMedicationItem::FN_SEQNUM("seqnum");
const QString KhandakerMojoMedicationItem::FN_MEDICATION_NAME("medication_name");
const QString KhandakerMojoMedicationItem::FN_CHEMICAL_NAME("chemical_name");
const QString KhandakerMojoMedicationItem::FN_DOSE("dose");
const QString KhandakerMojoMedicationItem::FN_FREQUENCY("frequency");
const QString KhandakerMojoMedicationItem::FN_DURATION_MONTHS("duration_months");
const QString KhandakerMojoMedicationItem::FN_INDICATION("indication");
const QString KhandakerMojoMedicationItem::FN_RESPONSE("response");

const QStringList KhandakerMojoMedicationItem::TABLE_FIELDNAMES{
    KhandakerMojoMedicationItem::FN_MEDICATION_NAME,
    KhandakerMojoMedicationItem::FN_CHEMICAL_NAME,
    KhandakerMojoMedicationItem::FN_DOSE,
    KhandakerMojoMedicationItem::FN_FREQUENCY,
    KhandakerMojoMedicationItem::FN_DURATION_MONTHS,
    KhandakerMojoMedicationItem::FN_INDICATION,
    KhandakerMojoMedicationItem::FN_RESPONSE,
 };

KhandakerMojoMedicationItem::KhandakerMojoMedicationItem(
    CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    DatabaseObject(app, db, KHANDAKERMOJOMEDICATIONITEM_TABLENAME,
                   dbconst::PK_FIELDNAME,  // pk_fieldname
                   true,  // has_modification_timestamp
                   false,  // has_creation_timestamp
                   true,  // has_move_off_tablet_field
                   true)  // triggers_need_upload
{
    addField(FN_FK_NAME, QVariant::Int);
    addField(FN_SEQNUM, QVariant::Int);
    addField(FN_MEDICATION_NAME, QVariant::String);
    addField(FN_CHEMICAL_NAME, QVariant::String);
    addField(FN_DOSE, QVariant::String);
    addField(FN_FREQUENCY, QVariant::String);
    addField(FN_DURATION_MONTHS, QVariant::Double);
    addField(FN_INDICATION, QVariant::String);
    addField(FN_RESPONSE, QVariant::Int);

    load(load_pk);
}


KhandakerMojoMedicationItem::KhandakerMojoMedicationItem(
    const int owner_fk, CamcopsApp& app, DatabaseManager& db) :
    KhandakerMojoMedicationItem(app, db)  // delegating constructor
{
    setValue(FN_FK_NAME, owner_fk);
}


void KhandakerMojoMedicationItem::setSeqnum(const int seqnum)
{
    setValue(FN_SEQNUM, seqnum);
}


void KhandakerMojoMedicationItem::setChemicalName(
    const QString chemical_name)
{
    setValue(FN_CHEMICAL_NAME, chemical_name);
}


bool KhandakerMojoMedicationItem::isComplete() const
{
    return noValuesNull(TABLE_FIELDNAMES);
}


bool KhandakerMojoMedicationItem::isEmpty() const
{
    for (const QString& fieldname : TABLE_FIELDNAMES) {
        if (!valueIsNullOrEmpty(fieldname)) {
            return false;
        }
    }

    return true;
}
