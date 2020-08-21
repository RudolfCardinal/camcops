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
#include "db/fieldref.h"
#include "questionnairelib/quelement.h"

class AspectRatioPixmap;


class QuImage : public QuElement
{
    // Displays an image (from a static filename or a field).
    // No user response offered.

    Q_OBJECT

protected:
    // Protected constructor
    QuImage(const QString& filename, FieldRefPtr fieldref, const QSize& size);

public:

    // Constructor to display a static image, from a filename.
    QuImage(const QString& filename, const QSize& size = QSize());

    // Constructor to display a dynamic image, from a field.
    // - "field" provides raw image data
    // - The default value of "size", QSize(), means "take the image's own
    //   size".
    QuImage(FieldRefPtr fieldref, const QSize& size = QSize());

    // Should the image be scaled according to our current DPI settings? Set
    // this to true if you want the image to be roughly the same size
    // regardless of the device. (It uses logical DPI, though, not physical
    // DPI.)
    QuImage* setAdjustForDpi(bool adjust_for_dpi);

    // Sets the image size. Using QSize() means "take the image's own size".
    QuImage* setSize(const QSize& size);

    // If the user shrinks the window, do we allow the image to be scaled down?
    QuImage* setAllowShrink(bool allow_shrink);

protected slots:
    // "The field's [image] data has changed."
    void valueChanged(const FieldRef* fieldref);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;

    // Returns the image from a field, with appropriate scaling as per our
    // settings.
    QPixmap getScaledImage(const FieldRef* fieldref = nullptr) const;

    // Scales a QSize according to our DPI settings.
    QSize dpiScaledSize(const QSize& size) const;

protected:
    QString m_filename;  // image filename, for static images
    FieldRefPtr m_fieldref;  // fieldref, for dynamic images
    QPointer<AspectRatioPixmap> m_label;  // our image widget
    QSize m_size;  // image size, or QSize() for the image's own size
    bool m_adjust_for_dpi;  // see setAdjustForDpi()
    bool m_allow_shrink;  // see setAllowShrink()
};
