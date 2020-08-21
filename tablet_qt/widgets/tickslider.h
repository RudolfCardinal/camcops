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
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include "common/uiconst.h"
#include "lib/margins.h"

class QHoverEvent;
class QKeyEvent;
class QMouseEvent;


// Slider with tick marks/labels.
//
// Tick marks vanish on a styled QSlider. So:
// http://stackoverflow.com/questions/27531542/tick-marks-disappear-on-styled-qslider
// ... and then modification including labels, and making it work with
// vertical sliders.

/*

Terminology:

Horizontal:

        1  two  3   4       6           tick labels
        |   |   |   |   |   |           ticks
    ----------------XX------------      the slider, with its handle
        |   |   |   |   |   |           ticks
        1  two  3   4       6           tick labels

Vertical:

     |
    -|- 6
     |
    -|- 5
     |
    -X- 4
     |
    -|- 4
     |
    -|- 3
     |
    -|- two
     |
    -|- 1
     |

*/

class TickSlider : public QWidget
{
    // As soon as we start re-jigging the layout of the slider itself, e.g.
    // to ensure labels have enough space at the edges, we need to override
    // functions like initStyleOption() that are not virtual in QSlider.
    // At that point, we have to go the whole hog and inherit from
    // QAbstractSlider instead, and re-implement QSlider.
    //
    // ... Unless we can fake the mouse coordinates.
    // ... Nope; mouse hovering involves QSlider::event(), which calls
    //     private functions.
    // ... Although maybe we only need to re-implement
    //     QSliderPrivate::updateHoverControl().
    // ... Tried hard; failed.
    // Reimplementation it is.
    //
    // The thing that makes it very hard is that QSlider has deep connections
    // to QSliderPrivate, which inherits from QAbstractSliderPrivate (with
    // deep links to QAbstractSlider), which inherits from QWidgetPrivate...
    // etc. (Even if we wanted to cheat, none of the *private*.h files are
    // exposed in Qt builds...)
    // No... it's an absolute nightmare; we regress into the bowels of
    // QWidgetPrivate and can't modify the data.
    // This is perhaps the weakest aspect of Qt.
    //
    // So: it has to be layouts, or layout-free widget composition.
    //
    // And that worked beautifully.
    //
    // Re "jump navigation":
    // - We embed a QSlider. Clicks within it are not visible to our mouse
    //   event captures. So no fiddling with that, unless we completely
    //   re-implement QSlider.

    Q_OBJECT

    // ========================================================================
    // Public interface
    // ========================================================================
public:
    // Create a TickSlider with the default (vertical) orientation.
    // - groove_margin_px is the width of the margin of the slider's groove, in
    //   pixels. (We can't read this, so we need to be told.)
    // - Note that default arguments are evaluated at call time. Good C++.
    TickSlider(QWidget* parent = nullptr,
               int groove_margin_px = uiconst::SLIDER_GROOVE_MARGIN_PX);

    // Create a TickSlider, specifying the orientation.
    TickSlider(Qt::Orientation orientation, QWidget* parent = nullptr,
               int groove_margin_px = uiconst::SLIDER_GROOVE_MARGIN_PX);

    // Set the tick colour.
    virtual void setTickColor(const QColor& colour);

    // Set the tick thickness (width), in pixels.
    virtual void setTickThickness(int thickness);

    // Sets the tick length, in pixels (perpendicular to the slider).
    virtual void setTickLength(int length);

    // Set the gap between ticks and tick labels, in pixels
    // (measured perpendicular to the slider).
    virtual void setTickLabelGap(int gap);

    // Set the minimum gap between labels, in pixels
    // (measured parallel to the slider).
    virtual void setInterlabelGap(int gap);

    // Set the gap between the slider and labels/ticks (whichever comes first),
    // in pixels (measured perpendicular to the slider).
    virtual void setGapToSlider(int gap);

    // Make the extreme labels align with the ends of the slider?
    // For a horizontal slider, that means "left-align the left label with its
    // tick, and right-align the right label with its tick" (rather than the
    // default of centre-aligning all labels with their ticks).
    // Similarly for vertical sliders.
    // Default is false.
    virtual void setEdgeInExtremeLabels(bool edge_in_extreme_labels);

    // [Only applicable if setEdgeInExtremeLabels() is set to false.]
    // Using a horizontal slider example, suppose the leftmost label is
    // very long and the rightmost label is very short. The label will
    // "overspill" a lot on the left and not much on the right. This will mean
    // that the whole slider is not centred. If you call
    // setSymmetricOverspill(true), extra space will be added on the right so
    // that the whole thing is symmetric.
    virtual void setSymmetricOverspill(bool symmetric_overspill);

    // Sets the absolute length of the slider's active range, in pixels.
    // If can_shrink is true, the slider can get smaller (for small screens).
    // See also setAbsoluteLengthCm().
    virtual void setAbsoluteLengthPx(int px, bool can_shrink = true);

    // Sets the absolute length of the slider's active range, in cm, given also
    // the screen's current dpi setting (which you must provide).
    // - Convenience function that calls setAbsoluteLengthPx().
    // - Use this to say "make the slider exactly 10cm".
    // - Beware on small screens! Suggest setting can_shrink to true.
    virtual void setAbsoluteLengthCm(qreal abs_length_cm, qreal dpi,
                                     bool can_shrink = true);

    // Standard QWidget size hint.
    virtual QSize sizeHint() const override;

    // Standard QWidget minimum size hint
    virtual QSize minimumSizeHint() const override;

    // Chooses whether tick labels are shown left/right/both (vertical sliders)
    // or above/below/both (horizontal sliders).
    virtual void setTickLabelPosition(QSlider::TickPosition position);

    // Reads what was set by setTickLabelPosition().
    virtual QSlider::TickPosition tickLabelPosition() const;

    // Adds a label at an integer slider position.
    virtual void addTickLabel(int position, const QString& text);

    // Sets all tick labels simultaneously. The map maps integer slider
    // position to text.
    virtual void setTickLabels(const QMap<int, QString>& labels);

    // Adds some default numerical tick labels (at the tickInterval(), or if
    // that's not set, the pageStep()).
    virtual void addDefaultTickLabels();

    // Set the CSS name of this widget and its QSlider child widget.
    virtual void setCssName(const QString& name);

    // Is it horizontal?
    bool isHorizontal() const;

    // ========================================================================
    // Passing on calls to/from our slider
    // ========================================================================
public:

    // ------------------------------------------------------------------------
    // From QAbstractSlider (q.v.):
    // ------------------------------------------------------------------------

    Qt::Orientation orientation() const { return m_slider.orientation(); }

    void setMinimum(int minimum) { m_slider.setMinimum(minimum); }
    int minimum() const { return m_slider.minimum(); }

    void setMaximum(int maximum) { m_slider.setMaximum(maximum); }
    int maximum() const { return m_slider.maximum(); }

    void setSingleStep(int step) { m_slider.setSingleStep(step); }
    int singleStep() const { return m_slider.singleStep(); }

    void setPageStep(int step) { m_slider.setPageStep(step); }
    int pageStep() const { return m_slider.pageStep(); }

    void setTracking(bool enable) { m_slider.setTracking(enable); }
    bool hasTracking() const { return m_slider.hasTracking(); }

    void setSliderDown(bool down) { m_slider.setSliderDown(down); }
    bool isSliderDown() const { return m_slider.isSliderDown(); }

    void setSliderPosition(int pos) { m_slider.setSliderPosition(pos); }
    int sliderPosition() const { return m_slider.sliderPosition(); }

    // Reverse the direction of the slider.
    // Default is left (low) -> right (high), and bottom (low) -> top (high).
    void setInvertedAppearance(bool inverted) { m_slider.setInvertedAppearance(inverted); }
    bool invertedAppearance() const { return m_slider.invertedAppearance(); }

    // Reverse the behaviour of key/mouse wheel events.
    // See http://doc.qt.io/qt-5/qabstractslider.html
    void setInvertedControls(bool inverted) { m_slider.setInvertedControls(inverted); }
    bool invertedControls() const { return m_slider.invertedControls(); }

    int value() const { return m_slider.value(); }

    void triggerAction(QSlider::SliderAction action) { return m_slider.triggerAction(action); }

public slots:
    void setValue(int value) { m_slider.setValue(value); }
    void setOrientation(Qt::Orientation orientation);
    void setRange(int min, int max) { m_slider.setRange(min, max); }

signals:
    void valueChanged(int value);

    void sliderPressed();
    void sliderMoved(int position);
    void sliderReleased();

    void rangeChanged(int min, int max);

    void actionTriggered(int action);

    // ------------------------------------------------------------------------
    // From QSlider (q.v.):
    // ------------------------------------------------------------------------
public:

    void setTickPosition(QSlider::TickPosition pos) { m_slider.setTickPosition(pos); }
    QSlider::TickPosition tickPosition() const { return m_slider.tickPosition();}

    void setTickInterval(int ti) { m_slider.setTickInterval(ti); }
    int tickInterval() const { return m_slider.tickInterval(); }

    // ========================================================================
    // Our internals
    // ========================================================================

protected:

    // Get the size of the biggest label (more accurately, a size that will
    // hold all our labels).
    QSize maxLabelSize() const;

    // Are we using ticks?
    bool usingTicks() const;

    // Are we using labels?
    bool usingLabels() const;

    // Given an integer value from the slider, get the position along the
    // slider as a proportion (0-1), in a  standard drawing direction
    // (x left->right, y top->bottom).
    double getDrawingProportion(int value) const;

    // Tick position (vertical or horizontal) along the slider.
    int getTickPos(double drawing_proportion, int active_groove_start,
                   int active_groove_extent) const;

    // The extent to which labels "overspill" the boundaries of the slider
    // (in the direction along its length.)
    //
    // - If m_edge_in_extreme_labels, these are all zero. Otherwise:
    // - Horizontal:
    //   - If we have a label for the leftmost (minimum) value, "left" is set
    //     to half the width of the leftmost label.
    //   - If we have a label for the rightmost (maximum) value, "right" is set
    //     to half the width of the rightmost label.
    // - Vertical:
    //   - If we have a label for the topmost (maximum) value, "top" is set to
    //     half the width of the topmost label.
    //   - If we have a label for the bottom (minimum) value, "bottom" is set
    //     to half the width of the bottom label.
    virtual Margins getLabelOverspill() const;

    // Returns the size of all the extra things we draw around the QSlider.
    Margins getSurround() const;

    // Clear cached information
    void clearCache();

    // Return the "active" part of the groove.
    QRect getSliderActiveGroove() const;

    // Calculate where our slider widget should be (it is a sub-rectangle of
    // our main widget).
    QRect getSliderRect() const;

    // Tells the slider to reposition and/or resize itself.
    void repositionSlider();

    // Reset the widget size policy
    void resetSizePolicy();

    // Calculate the slider's size *including* its handle -- it seems to ignore
    // this via sizeHint() or minimumSizeHint()!
    QSize sliderSizeWithHandle(bool minimum_size) const;

    // Expand a starting "slider" size to the size required by the whole widget.
    QSize wholeWidgetSize(const QSize& slider_size) const;

    // Event handlers
    virtual bool event(QEvent* event) override;
    virtual void moveEvent(QMoveEvent* event) override;
    virtual void paintEvent(QPaintEvent* ev) override;
    virtual void resizeEvent(QResizeEvent* event) override;

    // Replicated from QSlider (where it's protected):
    void initStyleOption(QStyleOptionSlider* option) const;

protected:
    QSlider m_slider;  // our slider
    int m_groove_margin_px;  // width of the margin of the slider's groove
    QColor m_tick_colour;  // tick colour
    int m_tick_thickness;  // tick thickness (parallel to slider)
    int m_tick_length;  // tick length (perpendicular to slider)
    int m_tick_label_gap;  // gap between ticks and labels (perpendicular to slider)
    int m_min_interlabel_gap;  // minimum gap between labels (parallel to slider)
    int m_gap_to_slider;  // gap adjacent to slider (to ticks or labels)
    QSlider::TickPosition m_label_position;  // labels left/right/both (etc.)
    QMap<int, QString> m_tick_labels;  // maps from slider position to label text
    bool m_edge_in_extreme_labels;  // see setEdgeInExtremeLabels()
    bool m_symmetric_overspill;  // see setSymmetricOverspill()
    int m_slider_target_length_px;  // absolute target length; <=0 means don't use this
    bool m_absolute_size_can_shrink;
        // ... if an absolute length is set, can we shrink smaller if we have
        // to? May be preferable on physically small screens.
    mutable bool m_is_overspill_cached;  // is m_cached_overspill valid?
    mutable Margins m_cached_overspill;  // cached margins for overspill; see setSymmetricOverspill()
};
