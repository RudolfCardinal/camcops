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

#include "quthermometer.h"
#include <QLabel>
#include "layouts/layouts.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/imagebutton.h"
#include "widgets/thermometer.h"

const int DEFAULT_TEXT_GAP_PX = 5;


QuThermometer::QuThermometer(FieldRefPtr fieldref,
                             const QVector<QuThermometerItem>& items) :
    m_fieldref(fieldref),
    m_items(items),
    m_rescale(false),
    m_rescale_factor(0),
    m_thermometer(nullptr)
{
    Q_ASSERT(m_fieldref);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuThermometer::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuThermometer::fieldValueChanged);
}


QuThermometer::QuThermometer(FieldRefPtr fieldref,
                             std::initializer_list<QuThermometerItem> items) :
    QuThermometer(fieldref, QVector<QuThermometerItem>(items))  // delegating constructor
{
}


QuThermometer* QuThermometer::setRescale(const bool rescale,
                                         const double rescale_factor,
                                         const bool adjust_for_dpi)
{
    m_rescale = rescale;
    m_rescale_factor = rescale_factor;
    if (adjust_for_dpi) {
        m_rescale_factor *= uiconst::g_logical_dpi.mean() /
                            uiconst::DEFAULT_DPI.mean();
    }
    return this;
}


QPointer<QWidget> QuThermometer::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();
    const int n = m_items.size();

    QVector<QPixmap> active_images;
    QVector<QPixmap> inactive_images;
    QStringList right_strings;
    // In reverse order:
    for (int i = n - 1; i >= 0; --i) {  // i (item index): 0 bottom, n - 1 top
        // ... iterating top to bottom
        const QuThermometerItem& item = m_items.at(i);
        active_images.append(uifunc::getPixmap(item.activeFilename()));
        inactive_images.append(uifunc::getPixmap(item.inactiveFilename()));
        right_strings.append(item.text());
    }
    m_thermometer = new Thermometer(
        active_images,
        inactive_images,
        nullptr,  // left_strings
        &right_strings,
        0,  // left_string_scale
        1,  // image_scale
        1,  // right_string_scale
        false,  // allow_deselection
        read_only,  // read_only
        m_rescale,  // rescale
        m_rescale_factor,  // rescale_factor
        DEFAULT_TEXT_GAP_PX,  // text_gap_px
        nullptr  // parent
    );

    connect(m_thermometer.data(), &Thermometer::selectionIndexChanged,
            this, &QuThermometer::thermometerSelectionChanged);
    setFromField();
    return m_thermometer.data();
}


void QuThermometer::setFromField()
{
    fieldValueChanged(m_fieldref.data());
}


void QuThermometer::thermometerSelectionChanged(int thermometer_index)
{
    // thermometer_index: thermometer's top-to-bottom index
    const int n = m_items.size();
    const int index = (n - 1) - thermometer_index;  // QuThermometer internal index
    if (index < 0 || index >= n) {
        qWarning() << Q_FUNC_INFO << "- out of range";
        return;
    }
    const QVariant newvalue = m_items.at(index).value();
    const bool changed = m_fieldref->setValue(newvalue);  // Will trigger valueChanged
    if (changed) {
        emit elementValueChanged();
    }
}


int QuThermometer::indexFromValue(const QVariant &value) const
{
    if (value.isNull()) {
        return -1;
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).value() == value) {
            return i;
        }
    }
    return -1;
}


QVariant QuThermometer::valueFromIndex(const int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QVariant();
    }
    return m_items.at(index).value();
}


void QuThermometer::fieldValueChanged(const FieldRef* fieldref)
{
    const int index = indexFromValue(fieldref->value());
    const int n = m_items.size();
    const int index_row = (n - 1) - index;  // operating in reverse

    if (!m_thermometer) {
        return;
    }
    m_thermometer->setSelectedIndex(index_row);
}


FieldRefPtrList QuThermometer::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}
