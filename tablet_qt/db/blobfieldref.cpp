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

#include "blobfieldref.h"
#include "dbobjects/blob.h"


BlobFieldRef::BlobFieldRef(DatabaseObject* p_dbobject,
                           const QString& fieldname,
                           const bool mandatory,
                           CamcopsApp* p_app) :
    FieldRef(p_dbobject, fieldname, mandatory,
             true,  // autosave
             true,  // blob
             p_app)
{
    Q_ASSERT(m_blob);
}


BlobFieldRef::BlobFieldRef(QSharedPointer<Blob> blob,
                           const bool mandatory,
                           const bool disable_creation_warning) :
    FieldRef(blob, mandatory, disable_creation_warning)
{
    Q_ASSERT(m_blob);
}


QImage BlobFieldRef::image(bool* p_loaded) const
{
    return m_blob->image(p_loaded);
}


QPixmap BlobFieldRef::pixmap(bool* p_loaded) const
{
    return QPixmap::fromImage(image(p_loaded));
}


void BlobFieldRef::rotateImage(const int angle_degrees_clockwise,
                               const QObject* originator)
{
    m_blob->rotateImage(angle_degrees_clockwise, true);
    setFkToBlob();  // see discussion in FieldRef::setValue
    signalSetValue(true, originator);
}


bool BlobFieldRef::setImage(const QImage& image, const QObject* originator)
{
    const bool changed = m_blob->setImage(image, true);
    if (changed) {
        setFkToBlob();  // see discussion in FieldRef::setValue
    }
    return signalSetValue(changed, originator);
}


bool BlobFieldRef::setRawImage(const QByteArray& data,
                               const QString& extension_without_dot,
                               const QString& mimetype,
                               const QObject* originator)
{
    const bool changed = m_blob->setRawImage(data, true,
                                             extension_without_dot, mimetype);
    if (changed) {
        setFkToBlob();  // see discussion in FieldRef::setValue
    }
    return signalSetValue(changed, originator);
}
