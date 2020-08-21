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

#include "diagnosisicd9cmitem.h"

const QString DiagnosisIcd9CMItem::DIAGNOSIS_ICD9CM_ITEM_TABLENAME("diagnosis_icd9cm_item");
const QString DiagnosisIcd9CMItem::FK_NAME("diagnosis_icd9cm_id");  // FK to diagnosis_icd9cm.id


DiagnosisIcd9CMItem::DiagnosisIcd9CMItem(
        CamcopsApp& app, DatabaseManager& db, int load_pk) :
    DiagnosisItemBase(app, db,
                      DIAGNOSIS_ICD9CM_ITEM_TABLENAME, FK_NAME, load_pk)
{
}


DiagnosisIcd9CMItem::DiagnosisIcd9CMItem(const int owner_fk, CamcopsApp& app,
                                         DatabaseManager& db) :
    DiagnosisItemBase(owner_fk, app, db,
                      DIAGNOSIS_ICD9CM_ITEM_TABLENAME, FK_NAME)  // delegating constructor
{
}
