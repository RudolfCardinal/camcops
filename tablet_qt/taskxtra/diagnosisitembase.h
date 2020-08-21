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


class DiagnosisItemBase : public DatabaseObject
{
    Q_OBJECT
public:
    DiagnosisItemBase(CamcopsApp& app, DatabaseManager& db,
                      const QString& tablename,
                      const QString& fkname,
                      int load_pk = dbconst::NONEXISTENT_PK);
    DiagnosisItemBase(int owner_fk,
                      CamcopsApp& app, DatabaseManager& db,
                      const QString& tablename,
                      const QString& fkname);
    void setSeqnum(int seqnum);
    int seqnum() const;
    QString code() const;
    QString description() const;
    QString comment() const;
    bool isEmpty() const;
public:
    static const QString SEQNUM;
    static const QString CODE;
    static const QString DESCRIPTION;
    static const QString COMMENT;
protected:
    const QString m_fkname;
};
