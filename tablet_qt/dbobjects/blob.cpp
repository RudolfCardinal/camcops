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

// #define DEBUG_ROTATION

#include "blob.h"
#include <QTransform>
#include "db/databasemanager.h"
#include "lib/convert.h"

const QString Blob::TABLENAME("blobs");  // as per DBCONSTANTS.js

const QString Blob::SRC_TABLE_FIELDNAME("tablename");  // as per Blob.js
const QString Blob::SRC_PK_FIELDNAME("tablepk");  // as per Blob.js
const QString SRC_FIELD_FIELDNAME("fieldname");  // as per Blob.js
const QString FILENAME_FIELDNAME("filename");  // as per Blob.js
const QString MIMETYPE_FIELDNAME("mimetype");  // new in v2.0.0
const QString BLOB_FIELDNAME("theblob"); // as per dbupload.js
// ... was a "virtual" field under Titanium and file-based BLOBs.
const QString ROTATION_FIELDNAME("image_rotation_deg_cw");
// ... rotation is anticlockwise for "x up, y up", but clockwise for "y down",
//     which is the computing norm.


Blob::Blob(CamcopsApp& app,
           DatabaseManager& db,
           const QString& src_table,
           const int src_pk,
           const QString& src_field) :
    DatabaseObject(app,
                   db,
                   TABLENAME,
                   dbconst::PK_FIELDNAME,
                   true,  // modification timestamp
                   false)  // creation timestamp
{
    // ------------------------------------------------------------------------
    // Define fields
    // ------------------------------------------------------------------------
    addField(SRC_TABLE_FIELDNAME, QVariant::String, true);
    addField(SRC_PK_FIELDNAME, QVariant::Int, true);
    addField(SRC_FIELD_FIELDNAME, QVariant::String, true);
    addField(FILENAME_FIELDNAME, QVariant::String);
    addField(MIMETYPE_FIELDNAME, QVariant::String);
    // ... maximum length 255; https://stackoverflow.com/questions/643690
    addField(BLOB_FIELDNAME, QVariant::ByteArray);
    addField(ROTATION_FIELDNAME, QVariant::Int);

    // ------------------------------------------------------------------------
    // Load from database (or create/save), unless this is a specimen
    // ------------------------------------------------------------------------
    if (!src_table.isEmpty() && !src_field.isEmpty() && src_pk >= 0) {
        // Not a specimen; load, or set defaults and save
        WhereConditions where;
        where.add(SRC_TABLE_FIELDNAME, src_table);
        where.add(SRC_PK_FIELDNAME, src_pk);
        where.add(SRC_FIELD_FIELDNAME, src_field);
        bool success = load(where);  // will load the BLOB, if present
        if (!success) {
            setValue(SRC_TABLE_FIELDNAME, src_table);
            setValue(SRC_PK_FIELDNAME, src_pk);
            setValue(SRC_FIELD_FIELDNAME, src_field);
            save();
        }
    }

    // We could extend DatabaseObject::makeTable to call subclasses for extra
    // index/constraint requirements. Specifically, they need to go into the
    // CREATE TABLE syntax for SQLite:
    //      http://www.sqlite.org/syntaxdiagrams.html#table-constraint
    // However, we can equally implement the constraint by not screwing up the
    // C++ code, which is perhaps simpler (because: if the database implements
    // the code, then duff C++ code has its bacon partly saved by the database,
    // but leaving a potentially ambiguous state from the C++ perspective when
    // insertion overwrites rather than creatng.

    m_filename_stem = QString("blob_%1_%2_%3").arg(src_table)
                                              .arg(src_pk)
                                              .arg(src_field);
    // ... as per Blob.js

    m_image_loaded_from_data = false;
}


Blob::~Blob()
{
}


bool Blob::setBlob(const QVariant& value,
                   const bool save_to_db,
                   const QString& extension_without_dot,
                   const QString& mimetype)
{
    bool changed = setValue(BLOB_FIELDNAME, value);
    changed = setValue(FILENAME_FIELDNAME,
                       QString("%1.%2").arg(m_filename_stem,
                                            extension_without_dot)) || changed;
    changed = setValue(MIMETYPE_FIELDNAME, mimetype) || changed;
    changed = setValue(ROTATION_FIELDNAME, 0) || changed;

    if (save_to_db) {
        save();
    }
    m_image = QImage();  // clear cached image
    return changed;
}


QVariant Blob::blobVariant() const
{
    return value(BLOB_FIELDNAME);
}


QByteArray Blob::blobByteArray() const
{
    return valueByteArray(BLOB_FIELDNAME);
}


void Blob::makeIndexes()
{
    m_db.createIndex("_idx_blob_srctable_srcpk_srcfield",
                     TABLENAME,
                     {SRC_TABLE_FIELDNAME,
                      SRC_PK_FIELDNAME,
                      SRC_FIELD_FIELDNAME});
}


void Blob::rotateCachedImage(int angle_degrees_clockwise) const
{
    // http://doc.qt.io/qt-4.8/qtransform.html#rotate
    angle_degrees_clockwise %= 360;
    if (angle_degrees_clockwise == 0 || m_image.isNull()) {
        return;
    }
    QTransform matrix;
    matrix.rotate(angle_degrees_clockwise);
#ifdef DEBUG_ROTATION
    qDebug().nospace() << "Blob::rotateImage: rotating image of size "
                       << m_image.size() << "...";
#endif
    m_image = m_image.transformed(matrix);
#ifdef DEBUG_ROTATION
    qDebug() << "Blob::rotateImage: ... rotated to image of size"
             << m_image.size();
#endif
}


QImage Blob::image(bool* p_loaded) const
{
    if (m_image.isNull()) {
        m_image = convert::byteArrayToImage(blobByteArray(),
                                            &m_image_loaded_from_data);
        const int angle_deg_cw = valueInt(ROTATION_FIELDNAME);
        rotateCachedImage(angle_deg_cw);
    }
    if (p_loaded) {
        *p_loaded = m_image_loaded_from_data;
    }
    return m_image;
}


void Blob::rotateImage(const int angle_degrees_clockwise,
                       const bool save_to_db)
{
    int rotation = valueInt(ROTATION_FIELDNAME);
    rotation = (rotation + angle_degrees_clockwise) % 360;
    setValue(ROTATION_FIELDNAME, rotation);
    if (save_to_db) {
        save();
    }
    // We may have cached an image, so rotate that too:
    rotateCachedImage(angle_degrees_clockwise);
}


bool Blob::setImage(const QImage& image, const bool save_to_db)
{
    m_image = image;
    m_image_loaded_from_data = true;
    const QVariant value = convert::imageToVariant(image);
    bool changed = setBlob(value, save_to_db, "png", "image/png");
    return changed;
}


bool Blob::setRawImage(const QByteArray& data,
                       const bool save_to_db,
                       const QString& extension_without_dot,
                       const QString& mimetype)
{
    return setBlob(data,  // will autoconvert from QByteArray to QVariant
                   save_to_db, extension_without_dot, mimetype);
}
