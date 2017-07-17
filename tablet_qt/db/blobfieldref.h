/*
    Copyright (C) 2012-2017 Rudolf Cardinal (rudolf@pobox.com).

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
#include "db/fieldref.h"


class BlobFieldRef : public FieldRef
{
    // A FieldRef that's restricted to BLOBs, so you can insist on/guarantee a
    // BLOB interface. Provides special interfaces for images.

    Q_OBJECT
public:
    BlobFieldRef(DatabaseObject* p_dbobject, const QString& fieldname,
                 bool mandatory, CamcopsApp* p_app);
    BlobFieldRef(QSharedPointer<Blob> blob, bool mandatory);

    QImage blobImage(bool* p_loaded = nullptr) const;
    void blobRotateImage(int angle_degrees_clockwise,
                         const QObject* originator = nullptr);
    bool blobSetImage(const QImage& image, const QObject* originator = nullptr);
    bool blobSetRawImage(const QByteArray& data,
                         const QString& extension_without_dot,
                         const QString& mimetype,
                         const QObject* originator = nullptr);
};
