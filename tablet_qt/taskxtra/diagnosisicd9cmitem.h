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
#include "diagnosisitembase.h"


class DiagnosisIcd9CMItem : public DiagnosisItemBase
{
public:
    DiagnosisIcd9CMItem(CamcopsApp& app, DatabaseManager& db,
                        int load_pk = dbconst::NONEXISTENT_PK);
    DiagnosisIcd9CMItem(int owner_fk, CamcopsApp& app, DatabaseManager& db);
public:
    static const QString DIAGNOSIS_ICD9CM_ITEM_TABLENAME;
    static const QString FK_NAME;
};
