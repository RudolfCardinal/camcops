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

#include "diagnosisicd10item.h"

const QString DiagnosisIcd10Item::DIAGNOSIS_ICD10_ITEM_TABLENAME("diagnosis_icd10_item");
const QString DiagnosisIcd10Item::FK_NAME("diagnosis_icd10_id");  // FK to diagnosis_icd10.id


DiagnosisIcd10Item::DiagnosisIcd10Item(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    DiagnosisItemBase(app, db,
                      DIAGNOSIS_ICD10_ITEM_TABLENAME, FK_NAME, load_pk)
{
}


DiagnosisIcd10Item::DiagnosisIcd10Item(const int owner_fk, CamcopsApp& app,
                                       DatabaseManager& db) :
    DiagnosisItemBase(owner_fk, app, db,
                      DIAGNOSIS_ICD10_ITEM_TABLENAME, FK_NAME)  // delegating constructor
{
}
