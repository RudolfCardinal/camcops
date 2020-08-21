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


class PhotoSequencePhoto : public DatabaseObject
{
    Q_OBJECT
public:
    PhotoSequencePhoto(CamcopsApp& app, DatabaseManager& db,
                       int load_pk = dbconst::NONEXISTENT_PK);
    PhotoSequencePhoto(int owner_fk, CamcopsApp& app, DatabaseManager& db);
    void setSeqnum(int seqnum);
    int seqnum() const;
    QString description() const;
public:
    static const QString PHOTOSEQUENCEPHOTO_TABLENAME;
    static const QString FK_NAME;
    static const QString SEQNUM;
    static const QString DESCRIPTION;
    static const QString PHOTO_BLOBID;
    // static const QString ROTATION;  // DEFUNCT in v2
protected:
};
