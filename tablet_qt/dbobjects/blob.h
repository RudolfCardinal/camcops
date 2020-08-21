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
#include <QByteArray>
#include <QImage>
#include "db/databaseobject.h"


// Represents a BLOB (binary large object) record.
//
// Being big, BLOBs get their own table.
// For example, the Photo class contains records that are small and contain
// descriptions, creation dates, etc. -- and they cross-refer to the BLOB
// table via an integer FK. The BLOB table contains some back-references (via
// src_table, src_pk, src_field).

class Blob : public DatabaseObject
{
public:
    // Args:
    //  app: the CamcopsApp
    //  db: the database manager for the database we'll live in
    //  src_table: the table for which we are providing the BLOB (source table)
    //  src_pk: the PK of the source record (within the source table)
    //  src_field: the field containing the FK to us
    Blob(CamcopsApp& app,
         DatabaseManager& db,
         const QString& src_table = "",  // defaults for specimen construction
         int src_pk = -1,
         const QString& src_field = "");

    // Destructor
    virtual ~Blob();

    // ========================================================================
    // Basic BLOB access
    // ========================================================================

    // Sets the BLOB itself.
    // Returns: changed?
    bool setBlob(const QVariant& value,
                 bool save_to_db = true,
                 const QString& extension_without_dot = "png",
                 const QString& mimetype = "image/png");

    // Returns the BLOB as a QVariant.
    QVariant blobVariant() const;

    // Returns the BLOB as a QByteArray.
    QByteArray blobByteArray() const;

    // ========================================================================
    // Handling BLOBs as images:
    // ========================================================================

    // Returns the BLOB as a QImage.
    QImage image(bool* p_loaded) const;

    // Rotates the BLOB -- a very fast operation because we just alter the
    // value of our rotation field.
    void rotateImage(int angle_degrees_clockwise, bool save_to_db);

    // Sets the BLOB from a QImage.
    // Returns: changed?
    bool setImage(const QImage& image, bool save_to_db);

    // Sets the BLOB from a QByteArray.
    // Returns: changed?
    bool setRawImage(const QByteArray& data,
                     bool save_to_db,
                     const QString& extension_without_dot,
                     const QString& mimetype);

    // Makes indexes for the BLOB table.
    // (Resembles a Python classmethod; sort-of static function.)
    void makeIndexes();

protected:
    // Rotates the memory copy of the image.
    void rotateCachedImage(int angle_degrees_clockwise) const;

public:
    static const QString TABLENAME;
    static const QString SRC_TABLE_FIELDNAME;
    static const QString SRC_PK_FIELDNAME;

protected:
    // What would the BLOB be called on disk? (Without a path.)
    QString m_filename_stem;

    // Cached image, as conversion to/from QByteArray is slow
    mutable QImage m_image;

    // Was the image set from data?
    // [Have forgotten what this was important for...]
    mutable bool m_image_loaded_from_data;
};
