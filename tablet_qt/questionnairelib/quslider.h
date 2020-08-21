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
#include <QMap>
#include <QPointer>
#include <QSharedPointer>
#include <QVariant>
#include "db/fieldref.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/quelement.h"
#include "widgets/tickslider.h"  // or style sheets + tick marks don't mix

class QLabel;
class QTimer;


class QuSlider : public QuElement
{
    // Offers a slider to choose a numerical value.

    Q_OBJECT
public:
    // Create a slider ranging from "minimum" to "maximum" with step size
    // "step". The slider always uses integers internally, but can display as
    // a float (see setConvertForRealField).
    QuSlider(FieldRefPtr fieldref, int minimum, int maximum, int step = 1);

    // Set the "page step" size, if the user uses the PgUp/PgDn keys.
    // The default is twice the slider's step size.
    QuSlider* setBigStep(int big_step);

    // Interval between tick marks. (Default is 1.)
    QuSlider* setTickInterval(int tick_interval);  // 0 for none

    // Visually, where are the tick marks (e.g. left/right, above/below)?
    QuSlider* setTickPosition(QSlider::TickPosition position);

    // When the slider contains a null value, where should the handle sit?
    QuSlider* setNullApparentValue(int null_apparent_value);

    // Shortcuts for setNullApparentValue():
    // - Set null apparent value to the lowest (e.g. leftmost) value
    QuSlider* setNullApparentValueMin();
    // - Set null apparent value to the highest (e.g. rightmost) value
    QuSlider* setNullApparentValueMax();
    // - Set null apparent value to the centre value. Prefer this for centred
    //   visual analogue scales.
    QuSlider* setNullApparentValueCentre();

    // Choose whether the slider should display its contents as a float
    // (convert_for_real_field). If so, the underlying integer (from minimum to
    // maximum) is mapped to a float range (from field_minimum to
    // field_maximum), and shown with the specified number of decimal places
    // (display_dp).
    QuSlider* setConvertForRealField(bool convert_for_real_field,
                                     double field_minimum = 0,
                                     double field_maximum = 1,
                                     int display_dp = 2);

    // Should the slider be horizontal or vertical?
    QuSlider* setHorizontal(bool horizontal);

    // Should the slider show its current numerical value?
    QuSlider* setShowValue(bool show_value);

    // Determine where tick labels should be shown (at which integer values of
    // the slider) and the strings used for the tick labels.
    // Calling this also (effectively) calls setUseDefaultTickLabels(false).
    QuSlider* setTickLabels(const QMap<int, QString>& labels);

    // Visually, where are the tick labels (e.g. left/right, above/below)?
    QuSlider* setTickLabelPosition(QSlider::TickPosition position);

    // Chooses whether default labels should be shown. Default labels are
    // integers from the minimum to the maximum, spaced by the tick interval
    // (or if there isn't one, the "big" step).
    // See also setTickLabels().
    QuSlider* setUseDefaultTickLabels(bool use_default);

    // Should the far left/right labels be edged in visually so that they don't
    // overspill the boundaries of the slider?
    QuSlider* setEdgeInExtremeLabels(bool edge_in_extreme_labels);

    // Should the slider be symmetric, with no colour below (vertical) or to
    // the left (horizontal) of the slider handle? If not, the slider will
    // show red left/below and white right/above, so the higher the value, the
    // more red is shown.
    QuSlider* setSymmetric(bool symmetric);

    // Invert the direction of the slider.
    // Default is left (low) -> right (high), and bottom (low) -> top (high).
    QuSlider* setInverted(bool inverted);

    // Sets the absolute length of the slider's active range, in cm.
    // - Use this to say "make the slider exactly 10cm".
    // - Beware on small screens!
    // - If can_shrink is true, the slider can get smaller (for small screens).
    // - If a value <= 0 is passed, the slider returns to its normal sizing
    //   behaviour.
    QuSlider* setAbsoluteLengthCm(qreal abs_length_cm,
                                  bool can_shrink = true);

protected:

    // Sets the widget state from our fieldref.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Return the slider's integer position corresponding to a value in
    // "field space".
    int sliderValueFromField(const QVariant& field_value) const;

    // Return the field's intended value given our slider's position.
    QVariant fieldValueFromSlider(int slider_value) const;

    virtual void closing() override;

protected slots:
    // "The slider has been moved."
    void sliderValueChanged(int slider_value);

    // "The slider finished moving a while ago; write the data."
    void completePendingFieldWrite();

    // "The field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref,
                           const QObject* originator = nullptr);

protected:
    // Core
    FieldRefPtr m_fieldref;  // our field
    int m_minimum;  // minimum value in slider space
    int m_maximum;  // maximum value in slider space
    int m_step;  // step size in slider space
    int m_big_step;  // "big step" (PgUp/PgDn) in slider space
    bool m_convert_for_real_field;  // translate to real numbers in slider space?
    double m_field_minimum;  // minimum in "real number field" space
    double m_field_maximum;  // maximum in "real number field" space
    int m_display_dp;  // number of decimal places to display value in "real number field" space
    int m_null_apparent_value;  // where (in slider space) should the slider be when the field is NULL?

    // Visuals
    bool m_horizontal;  // horizontal, not vertical?
    bool m_show_value;  // show the numerical value too?
    int m_tick_interval;  // intertick interval (in slider space)
    QSlider::TickPosition m_tick_position;  // ticks above/below/both/none, or left/right/both/none?
    bool m_use_default_labels;  // use default numerical labels?
    QMap<int, QString> m_tick_labels;  // manually specified position/label pairs
    QSlider::TickPosition m_tick_label_position;  // labels above/below/both/none, or left/right/both/none?
    bool m_edge_in_extreme_labels;  // see setEdgeInExtremeLabels() above
    bool m_symmetric;  // see setSymmetric() above
    bool m_inverted;  // inverted direction? See setInverted() above.
    qreal m_abs_length_cm;  // absolute length in cm, or <=0 for default size
    bool m_abs_length_can_shrink;
        // ... if an absolute length is set, can we shrink smaller if we have
        // to? May be preferable on physically small screens.

    // Internals
    QPointer<QWidget> m_container_widget;  // outer widget
    QPointer<QLabel> m_value_label;  // value indicator
    QPointer<TickSlider> m_slider;  // slider
    bool m_field_write_pending;  // is a field writes pending?
    int m_field_write_slider_value;  // the value to be written when m_timer expires
    QSharedPointer<QTimer> m_timer;  // timer to delay writes for visual performance
};
