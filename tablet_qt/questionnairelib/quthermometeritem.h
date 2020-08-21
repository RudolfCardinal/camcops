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
#include <QVariant>


class QuThermometerItem {
    // Describes part of a thermometer-style display. Used by QuThermometer.
    //
    // On the left is an image, which can take one of two states (active or
    // inactive). On the right is some optional text.

public:
    // Default constructor, so it can live in a QVector
    QuThermometerItem();

    // Normal constructor.
    // - active_filename: image to use when this part is active
    // - inactive_filename: image to use when this part is inactive
    // - text: associated text to display
    // - value: value to send to the field when this part is chosen
    // - overspill_rows: ignored if QUTHERMOMETER_USE_THERMOMETER_WIDGET is defined
    // - text_alignment: alignment of "text"
    QuThermometerItem(
            const QString& active_filename,
            const QString& inactive_filename,
            const QString& text,
            const QVariant& value,
            Qt::Alignment text_alignment = Qt::AlignLeft | Qt::AlignVCenter);

    // Returns the active image filename.
    QString activeFilename() const;

    // Returns the inactive image filename.
    QString inactiveFilename() const;

    // Returns the associated text.
    QString text() const;

    // Returns the associated field value.
    QVariant value() const;

    // Number of grid rows that the text should be allowed to spill over onto
    // (for large text with vertically small pictures):
    int overspillRows() const;

    // Alignment of text object:
    Qt::Alignment textAlignment() const;

protected:
    QString m_active_filename;
    QString m_inactive_filename;
    QString m_text;
    QVariant m_value;
    Qt::Alignment m_text_alignment;
};
