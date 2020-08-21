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
#include <QString>
#include "db/databaseobject.h"


class KhandakerMojoMedicationItem : public DatabaseObject
{
    Q_OBJECT
public:
    KhandakerMojoMedicationItem(CamcopsApp& app, DatabaseManager& db,
                                int load_pk = dbconst::NONEXISTENT_PK);
    KhandakerMojoMedicationItem(int owner_fk, CamcopsApp& app, DatabaseManager& db);
    void setSeqnum(int seqnum);
    void setChemicalName(const QString chemical_name);
    bool isComplete() const;
    bool isEmpty() const;
public:
    static const QString KHANDAKERMOJOMEDICATIONITEM_TABLENAME;
    static const QString FN_FK_NAME;
    static const QString FN_SEQNUM;
    static const QString FN_MEDICATION_NAME;
    static const QString FN_CHEMICAL_NAME;
    static const QString FN_DOSE;
    static const QString FN_FREQUENCY;
    static const QString FN_DURATION_MONTHS;
    static const QString FN_INDICATION;
    static const QString FN_RESPONSE;
    static const QStringList TABLE_FIELDNAMES;
protected:
};
