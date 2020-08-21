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

#include "quslider.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "lib/timerfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"


const int WRITE_DELAY_MS = 50;  // 10 is a bit low (sliders look slow)


QuSlider::QuSlider(FieldRefPtr fieldref,
                   const int minimum, const int maximum, const int step) :
    // Core
    m_fieldref(fieldref),
    m_minimum(minimum),
    m_maximum(maximum),
    m_step(step),
    m_convert_for_real_field(false),
    m_field_minimum(minimum),
    m_field_maximum(maximum),
    m_null_apparent_value(minimum),
    // Visuals
    m_horizontal(true),
    m_show_value(false),
    m_tick_interval(1),
    m_tick_position(QSlider::NoTicks),
    m_use_default_labels(false),
    m_tick_label_position(QSlider::NoTicks),
    m_edge_in_extreme_labels(false),
    m_symmetric(false),
    m_inverted(false),
    m_abs_length_cm(-1),
    m_abs_length_can_shrink(true),
    // Internals
    m_value_label(nullptr),
    m_slider(nullptr),
    m_field_write_pending(false)
{
    if (m_minimum > m_maximum) {
        qCritical() << Q_FUNC_INFO << "min > max; swapping";
        std::swap(m_minimum, m_maximum);
    }
    if (m_minimum == m_maximum) {
        uifunc::stopApp("QuSlider: minimum == maximum; bug!");
    }
    Q_ASSERT(m_fieldref);
    m_big_step = 2 * step;
    timerfunc::makeSingleShotTimer(m_timer);
    connect(m_timer.data(), &QTimer::timeout,
            this, &QuSlider::completePendingFieldWrite);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuSlider::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuSlider::fieldValueChanged);
}


QuSlider* QuSlider::setBigStep(const int big_step)
{
    m_big_step = qMin(m_step, big_step);
    return this;
}


QuSlider* QuSlider::setTickInterval(const int tick_interval)
{
    m_tick_interval = tick_interval;
    return this;
}


QuSlider* QuSlider::setTickPosition(const QSlider::TickPosition position)
{
    m_tick_position = position;
    return this;
}


QuSlider* QuSlider::setNullApparentValue(const int null_apparent_value)
{
    m_null_apparent_value = qBound(m_minimum, null_apparent_value, m_maximum);
    return this;
}


QuSlider* QuSlider::setNullApparentValueMin()
{
    m_null_apparent_value = m_minimum;
    return this;
}


QuSlider* QuSlider::setNullApparentValueMax()
{
    m_null_apparent_value = m_maximum;
    return this;
}


QuSlider* QuSlider::setNullApparentValueCentre()
{
    m_null_apparent_value = (m_minimum + m_maximum) / 2;
    return this;
}


QuSlider* QuSlider::setConvertForRealField(const bool convert_for_real_field,
                                           const double field_minimum,
                                           const double field_maximum,
                                           const int display_dp)
{
    m_convert_for_real_field = convert_for_real_field;
    m_field_minimum = field_minimum;
    m_field_maximum = field_maximum;
    m_display_dp = display_dp;
    return this;
}


QuSlider* QuSlider::setHorizontal(const bool horizontal)
{
    m_horizontal = horizontal;
    return this;
}


QuSlider* QuSlider::setShowValue(const bool show_value)
{
    m_show_value = show_value;
    return this;
}


QuSlider* QuSlider::setTickLabels(const QMap<int, QString>& labels)
{
    m_tick_labels = labels;
    m_use_default_labels = false;
    return this;
}


QuSlider* QuSlider::setTickLabelPosition(const QSlider::TickPosition position)
{
    m_tick_label_position = position;
    return this;
}


QuSlider* QuSlider::setUseDefaultTickLabels(const bool use_default)
{
    m_use_default_labels = use_default;
    return this;
}


QuSlider* QuSlider::setEdgeInExtremeLabels(const bool edge_in_extreme_labels)
{
    m_edge_in_extreme_labels = edge_in_extreme_labels;
    return this;
}


QuSlider* QuSlider::setSymmetric(const bool symmetric)
{
    m_symmetric = symmetric;
    return this;
}


QuSlider* QuSlider::setInverted(bool inverted)
{
    m_inverted = inverted;
    return this;
}


QuSlider* QuSlider::setAbsoluteLengthCm(const qreal abs_length_cm,
                                        bool can_shrink)
{
    m_abs_length_cm = abs_length_cm;
    m_abs_length_can_shrink = can_shrink;
    return this;
}


void QuSlider::setFromField()
{
    fieldValueChanged(m_fieldref.data(), nullptr);
    // special; pretend "it didn't come from us" to disable the efficiency
    // check in fieldValueChanged
}


int QuSlider::sliderValueFromField(const QVariant& field_value) const
{
    if (field_value.isNull()) {
        return m_null_apparent_value;
    }
    if (!m_convert_for_real_field) {
        return field_value.toInt();
    }
    const double field_from_left = field_value.toDouble() - m_field_minimum;
    const double slider_range = m_maximum - m_minimum;
    const double field_range = m_field_maximum - m_field_minimum;
    const int slider_pos = static_cast<int>(
                field_from_left * slider_range / field_range) + m_minimum;
    return slider_pos;
}


QVariant QuSlider::fieldValueFromSlider(const int slider_value) const
{
    if (!m_convert_for_real_field) {
        return slider_value;
    }
    const double slider_from_left = slider_value - m_minimum;
    const double slider_range = m_maximum - m_minimum;
    const double field_range = m_field_maximum - m_field_minimum;
    const double field_pos = (slider_from_left * field_range / slider_range) +
            m_field_minimum;
    return field_pos;
}


QPointer<QWidget> QuSlider::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();
    m_container_widget = new QWidget();
    m_value_label = nullptr;

    // 1. Value label
    if (m_show_value) {
        m_value_label = new QLabel();
        m_value_label->setObjectName(cssconst::SLIDER);
    }

    // 2. Slider (with labels)
    m_slider = new TickSlider(m_horizontal ? Qt::Horizontal : Qt::Vertical);
    m_slider->setMinimum(m_minimum);
    m_slider->setMaximum(m_maximum);
    m_slider->setSingleStep(m_step);
    m_slider->setPageStep(m_big_step);
    m_slider->setTickInterval(m_tick_interval);
    m_slider->setTickPosition(m_tick_position);
    if (m_use_default_labels) {
        m_slider->addDefaultTickLabels();
    } else {
        m_slider->setTickLabels(m_tick_labels);
    }
    m_slider->setTickLabelPosition(m_tick_label_position);
    m_slider->setEdgeInExtremeLabels(m_edge_in_extreme_labels);
    if (m_symmetric) {
        m_slider->setCssName(cssconst::SLIDER_SYMMETRIC);
        m_slider->setSymmetricOverspill(true);
    }
    m_slider->setInvertedAppearance(m_inverted);
    if (m_abs_length_cm > 0) {
        const qreal dpi = m_horizontal ? uiconst::g_physical_dpi.x
                                       : uiconst::g_physical_dpi.y;
        m_slider->setAbsoluteLengthCm(m_abs_length_cm, dpi,
                                      m_abs_length_can_shrink);
    };

    if (!read_only) {
        connect(m_slider.data(), &TickSlider::valueChanged,
                this, &QuSlider::sliderValueChanged);
    }
    m_slider->setEnabled(!read_only);

    // Layout
    const QSizePolicy::Policy parallel_policy = m_abs_length_cm > 0
            ? (m_abs_length_can_shrink ? QSizePolicy::Maximum
                                       : QSizePolicy::Fixed)
            : (m_horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred);
    const QSizePolicy::Policy perpendicular_policy = QSizePolicy::Fixed;
    if (m_horizontal) {
        // --------------------------------------------------------------------
        // Horizontal
        // --------------------------------------------------------------------
        auto layout = new QVBoxLayout();
        layout->setContentsMargins(uiconst::NO_MARGINS);
        if (m_value_label) {
            layout->addWidget(m_value_label, 0,
                              Qt::AlignHCenter | Qt::AlignVCenter);
        }
        layout->addWidget(m_slider);
        m_container_widget->setLayout(layout);
        m_container_widget->setSizePolicy(parallel_policy,
                                          perpendicular_policy);
    } else {
        // --------------------------------------------------------------------
        // Vertical
        // --------------------------------------------------------------------
        auto outerlayout = new QHBoxLayout();
        outerlayout->setContentsMargins(uiconst::NO_MARGINS);
        // Even for a vertical slider, have the numerical label above it,
        // or as it changes from "9" to "10" and its width changes, the
        // slider jiggles.
        auto innerlayout = new QVBoxLayout();
        innerlayout->setContentsMargins(uiconst::NO_MARGINS);
        if (m_value_label) {
            innerlayout->addWidget(m_value_label, 0,
                                   Qt::AlignHCenter | Qt::AlignVCenter);
        }
        innerlayout->addWidget(m_slider);
        outerlayout->addLayout(innerlayout);
        outerlayout->addStretch();
        m_container_widget->setLayout(outerlayout);
        m_container_widget->setSizePolicy(perpendicular_policy,
                                          parallel_policy);
    }

    setFromField();
    return m_container_widget;
}


void QuSlider::sliderValueChanged(const int slider_value)
{
    // Now, watch out. This can really screw up the UI performance.
    // QSlider objects can trigger lots of valueChanged signals very quickly.
    // http://stackoverflow.com/questions/26371571/qt-qslider-not-smooth
    m_field_write_slider_value = slider_value;
    m_field_write_pending = true;
    m_timer->start(WRITE_DELAY_MS);  // fires in same thread via event loop
    // ... goes to completePendingFieldWrite()
}


void QuSlider::completePendingFieldWrite()
{
    if (!m_field_write_pending) {
        return;
    }
    const QVariant newvalue = fieldValueFromSlider(m_field_write_slider_value);
    const bool changed = m_fieldref->setValue(newvalue, this);  // Will trigger valueChanged
    m_field_write_pending = false;
    if (changed) {
        emit elementValueChanged();
    }
}


void QuSlider::closing()
{
    completePendingFieldWrite();
}


void QuSlider::fieldValueChanged(const FieldRef* fieldref,
                                 const QObject* originator)
{
    if (m_container_widget) {
        uifunc::setPropertyMissing(m_container_widget,
                                   fieldref->missingInput());
    }

    // Slider
    if (m_slider) {
        // Optimization: no point in setting the value of a slider whose
        // change was the immediate reason we're here.
        if (originator != this) {
            // Imperative that the slider doesn't generate an infinite loop
            // by emitting further "valueChanged" signals, which it will do
            // when you use its setValue() command, unless you use blockSignals
            // (and the safe way to do that is with a QSignalBlocker).
            const QSignalBlocker blocker(m_slider);
            int slider_value = sliderValueFromField(fieldref->value());
            // qDebug() << "Setting slider value to:" << slider_value;
            m_slider->setValue(slider_value);
        }
        m_slider->update();
    }

    // Text
    if (m_value_label) {
        QString text;
        if (fieldref->isNull()) {
            text = "?";
        } else if (m_convert_for_real_field) {
            text = QString::number(fieldref->valueDouble(), 'f', m_display_dp);
        } else {
            text = QString::number(fieldref->valueInt());
        }
        m_value_label->setText(text);
    }
}


FieldRefPtrList QuSlider::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}
