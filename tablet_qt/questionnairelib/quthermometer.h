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

#include <QList>
#include "db/fieldref.h"
#include "questionnairelib/quelement.h"
#include "questionnairelib/quthermometeritem.h"
#include "widgets/thermometer.h"

class ImageButton;


class QuThermometer : public QuElement
{
    // Offers a stack of images, allowing the user to select one (and
    // displaying an alternative image at the chosen location), such as for
    // something in the style of a distress thermometer.
    //
    // The thermometer operates on name/value pairs; the thing that gets stored
    // in the field is the value() part of the QuThermometerItem.
    //
    // It's recommended to disable scrolling for pages using one of these.

    Q_OBJECT
public:

    // Constructors
    QuThermometer(FieldRefPtr fieldref,
                  const QVector<QuThermometerItem>& items);
    QuThermometer(FieldRefPtr fieldref,
                  std::initializer_list<QuThermometerItem> items);

    // Rescale the thermometer? (That is, alter its maximum display size?)
    //
    // - rescale: rescale images or not?
    // - rescale_factor: scale factor relative to original images
    // - adjust_for_dpi: additionally adjust for DPI?
    QuThermometer* setRescale(bool rescale, double rescale_factor = 1.0,
                              bool adjust_for_dpi = true);

protected:
    // Sets the widget state from our fieldref.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Convert value (see QuThermometerItem) to zero-based index.
    int indexFromValue(const QVariant& value) const;

    // Convert zero-based index to value (see QuThermometerItem).
    QVariant valueFromIndex(int index) const;

protected slots:
    // "User has selected a new part of the thermometer."
    void thermometerSelectionChanged(int thermometer_index);  // top-to-bottom index

    // "The field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref);

protected:
    FieldRefPtr m_fieldref;  // our fieldref
    QVector<QuThermometerItem> m_items;  // our image/text/value tuples
    bool m_rescale;  // see setRescale()
    double m_rescale_factor;  // see setRescale()
    QPointer<Thermometer> m_thermometer;  // our widget
};
