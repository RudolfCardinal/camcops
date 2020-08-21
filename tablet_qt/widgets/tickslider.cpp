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

// #define DEBUG_SIZE

#include "tickslider.h"
#include <cmath>  // for round()
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QStylePainter>
#include "graphics/graphicsfunc.h"
#include "lib/convert.h"


const QColor DEFAULT_TICK_COLOR(0, 0, 0, 255);  // black, opaque
const int DEFAULT_TICK_WIDTH = 0;
const int DEFAULT_TICK_LENGTH = 4;
const int DEFAULT_TICK_LABEL_GAP = 4;
const int DEFAULT_INTERLABEL_GAP = 4;
const int DEFAULT_GAP_TO_SLIDER = 6;

const int FM_TEXTFLAGS = 0;  // http://doc.qt.io/qt-5/qfontmetrics.html#size


// ============================================================================
// TickSlider main
// ============================================================================

TickSlider::TickSlider(QWidget* parent, const int groove_margin_px) :
    TickSlider(Qt::Vertical, parent, groove_margin_px)
{
}


TickSlider::TickSlider(Qt::Orientation orientation,
                       QWidget* parent,
                       const int groove_margin_px) :
    QWidget(parent),
    m_slider(orientation, this),
    m_groove_margin_px(groove_margin_px)
{
    connect(&m_slider, &QSlider::valueChanged,
            this, &TickSlider::valueChanged);

    connect(&m_slider, &QSlider::sliderPressed,
            this, &TickSlider::sliderPressed);
    connect(&m_slider, &QSlider::sliderMoved,
            this, &TickSlider::sliderMoved);
    connect(&m_slider, &QSlider::sliderReleased,
            this, &TickSlider::sliderReleased);

    connect(&m_slider, &QSlider::rangeChanged,
            this, &TickSlider::rangeChanged);

    connect(&m_slider, &QSlider::actionTriggered,
            this, &TickSlider::actionTriggered);

    m_tick_colour = DEFAULT_TICK_COLOR;
    m_tick_thickness = DEFAULT_TICK_WIDTH;
    m_tick_length = DEFAULT_TICK_LENGTH;
    m_tick_label_gap = DEFAULT_TICK_LABEL_GAP;
    m_min_interlabel_gap = DEFAULT_INTERLABEL_GAP;
    m_gap_to_slider = DEFAULT_GAP_TO_SLIDER;
    m_edge_in_extreme_labels = false;
    m_symmetric_overspill = true;
    m_slider_target_length_px = -1;  // <=0 means "ignore"
    m_absolute_size_can_shrink = true;

    m_is_overspill_cached = false;

    resetSizePolicy();
}


void TickSlider::setTickColor(const QColor& colour)
{
    m_tick_colour = colour;
}


void TickSlider::setTickThickness(const int thickness)
{
    m_tick_thickness = thickness;
}


void TickSlider::setTickLength(const int length)
{
    m_tick_length = length;
}


void TickSlider::setTickLabelGap(const int gap)
{
    m_tick_label_gap = gap;
}


void TickSlider::setInterlabelGap(const int gap)
{
    m_min_interlabel_gap = gap;
}


void TickSlider::setGapToSlider(const int gap)
{
    m_gap_to_slider = gap;
}


void TickSlider::setTickLabelPosition(const QSlider::TickPosition position)
{
    m_label_position = position;
}


QSlider::TickPosition TickSlider::tickLabelPosition() const
{
    return m_label_position;
}


void TickSlider::addTickLabel(const int position, const QString& text)
{
    m_tick_labels[position] = text;
}


void TickSlider::setTickLabels(const QMap<int, QString>& labels)
{
    m_tick_labels = labels;
}


void TickSlider::addDefaultTickLabels()
{
    int interval = tickInterval();
    if (interval == 0) {
        interval = pageStep();
    }
    for (int i = minimum(); i <= maximum(); i += interval) {
        m_tick_labels[i] = QString::number(i);
    }
}


void TickSlider::setEdgeInExtremeLabels(const bool edge_in_extreme_labels)
{
    m_edge_in_extreme_labels = edge_in_extreme_labels;
}


void TickSlider::setSymmetricOverspill(const bool symmetric_overspill)
{
    m_symmetric_overspill = symmetric_overspill;
}


void TickSlider::setAbsoluteLengthPx(const int px, const bool can_shrink)
{
    m_slider_target_length_px = px;
    m_absolute_size_can_shrink = can_shrink;
    resetSizePolicy();
}


void TickSlider::setAbsoluteLengthCm(const qreal abs_length_cm,
                                     const qreal dpi,
                                     const bool can_shrink)
{
    const int px = convert::convertCmToPx(abs_length_cm, dpi);
    qDebug().nospace()
            << "TickSlider: setting absolute length to "
            << abs_length_cm << " cm at " << dpi << " dpi, giving "
            << px << " pixels";
    setAbsoluteLengthPx(px, can_shrink);
}


bool TickSlider::usingTicks() const
{
    return tickPosition() != QSlider::NoTicks;
}


bool TickSlider::usingLabels() const
{
    return tickLabelPosition() != QSlider::NoTicks;
}


QSize TickSlider::maxLabelSize() const
{
    if (!usingLabels()) {
        return QSize(0, 0);
    }
    QSize maxsize;
    const QFontMetrics fm = fontMetrics();
    for (const QString& label : m_tick_labels) {
        QSize labelsize = fm.size(FM_TEXTFLAGS, label);
        maxsize = maxsize.expandedTo(labelsize);
    }
    return maxsize;
}


bool TickSlider::isHorizontal() const
{
    return m_slider.orientation() == Qt::Horizontal;
}


/*

Horizontal:

        |   |   |   |   |   |           ticks
    ----------------XX------------      the slider
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

I don't know how to make Qt do anything except draw its slider centrally,
so we have to expand the whole widget to allow for labels either side,
and similarly we have to right-justify the leftmost label and left-justify
the rightmost label (when in horizontal mode) or they dangle off the end;
neither is perfect.

Aha! 2019-01-14. We modify opt.rect as below.

*/


double TickSlider::getDrawingProportion(int value) const
{
    const double dvalue = static_cast<double>(value);
    const double min_value = static_cast<double>(minimum());
    const double max_value = static_cast<double>(maximum());
    double proportion = (dvalue - min_value) / (max_value - min_value);

    const bool inverted = invertedAppearance();
    const bool horizontal = isHorizontal();
    const bool vertical = !horizontal;
    bool flip = inverted;
    if (vertical) {
        flip = !flip;  // vertical ones: drawing direction is reversed
    }

    if (flip) {
        proportion = 1.0 - proportion;
    }
    return proportion;
}


void TickSlider::paintEvent(QPaintEvent* ev)
{
    // Coordinate systems: see http://doc.qt.io/qt-5.7/coordsys.html
    // ... positive is right and DOWN.

    Q_UNUSED(ev)

    // Draw the slider first...
    // ... will happen spontaneously.

    // Do we need to do more?
    if (!usingTicks() && !usingLabels()) {
        return;
    }

    const bool horizontal = isHorizontal();
    const bool inverted = invertedAppearance();
    const int min_value = minimum();
    const int max_value = maximum();
    const int first_value = inverted ? max_value : min_value;
    const int last_value = inverted ? min_value : max_value;
    const QSize max_label_size = maxLabelSize();
    const QSlider::TickPosition tickpos = tickPosition();
    const QSlider::TickPosition ticklabelpos = tickLabelPosition();

    QStylePainter p(this);
    const QRect& whole_rect = rect();
    const QRect active_groove = getSliderActiveGroove();
#ifdef DEBUG_SIZE
    qDebug() << "Whole widget:" << whole_rect;
    qDebug() << "Active part of slider groove:" << active_groove;
    if (horizontal) {
        qDebug() << "getTickPos(0, ...):" << getTickPos(
                        0, active_groove.left(), active_groove.width());
        qDebug() << "getTickPos(1, ...):" << getTickPos(
                        1, active_groove.left(), active_groove.width());
    } else {
        qDebug() << "getTickPos(0, ...):" << getTickPos(
                        0, active_groove.top(), active_groove.height());
        qDebug() << "getTickPos(1, ...):" << getTickPos(
                        1, active_groove.top(), active_groove.height());
    }
#endif

    // Draw tick marks (and labels).
    // Do this manually because they are very badly behaved with style sheets.
    int interval = tickInterval();
    if (interval == 0) {
        interval = pageStep();
    }
    QPen pen;
    pen.setColor(m_tick_colour);
    pen.setWidth(m_tick_thickness);
    p.setPen(pen);

    // We draw labels/ticks working from the outside of the bounding box in.
    if (horizontal) {
        // --------------------------------------------------------------------
        // HORIZONTAL
        // --------------------------------------------------------------------
        const int label_max_height = max_label_size.height();
        const bool has_top_tick = tickpos & QSlider::TicksAbove;
        const bool using_top_labels = ticklabelpos & QSlider::TicksAbove
                && label_max_height > 0;
        const bool has_bottom_tick = tickpos & QSlider::TicksBelow;
        const bool using_bottom_labels = ticklabelpos & QSlider::TicksBelow
                && label_max_height > 0;
        // Top
        const int whole_top = whole_rect.top();
        const int top_label_top = whole_top;
        const int top_tick_top = whole_top +
                (using_top_labels ? label_max_height + m_tick_label_gap : 0);
        const int top_tick_bottom = top_tick_top +
                (has_top_tick ? m_tick_length : 0);
        // Bottom, working up
        const int whole_bottom = whole_rect.bottom();
        const int bottom_label_bottom = whole_bottom;
        const int bottom_tick_bottom = whole_bottom -
                (using_bottom_labels ? label_max_height + m_tick_label_gap : 0);
        const int bottom_tick_top = bottom_tick_bottom -
                (has_bottom_tick ? m_tick_length : 0);
        // Draw it.
        // i: integer value
        // q: zero-based integer position left->right
        for (int i = min_value; i <= max_value; i += interval) {
            Qt::Alignment halign = Qt::AlignHCenter;
            if (m_edge_in_extreme_labels) {
                const bool leftmost = i == first_value;
                const bool rightmost = i == last_value;
                if (leftmost) {
                    halign = Qt::AlignLeft;
                } else if (rightmost) {
                    halign = Qt::AlignRight;
                }
            }
            const double prop = getDrawingProportion(i);
            // x position (tick position and horizontal alignment point of text)
            const int x = getTickPos(prop, active_groove.left(),
                                     active_groove.width());
            const bool has_label = m_tick_labels.contains(i);
            QString label_text;
            if (has_label) {
                label_text = m_tick_labels[i];
            }
            // Top tick
            if (has_top_tick) {
                p.drawLine(x, top_tick_top, x, top_tick_bottom);
            }
            // Top label
            if (has_label && using_top_labels) {
                graphicsfunc::drawText(p, x, top_label_top,
                                       halign | Qt::AlignTop, label_text);
            }
            // Bottom tick
            if (has_bottom_tick) {
                p.drawLine(x, bottom_tick_top, x, bottom_tick_bottom);
            }
            // Bottom label
            if (has_label && using_bottom_labels) {
                graphicsfunc::drawText(p, x, bottom_label_bottom,
                                       halign | Qt::AlignBottom, label_text);
            }
        }
    } else {
        // --------------------------------------------------------------------
        // VERTICAL
        // --------------------------------------------------------------------
        const int label_max_width = max_label_size.width();
        const bool has_left_tick = tickpos & QSlider::TicksLeft;
        const bool using_left_labels = ticklabelpos & QSlider::TicksLeft
                && label_max_width > 0;
        const bool has_right_tick = tickpos & QSlider::TicksRight;
        const bool using_right_labels = ticklabelpos & QSlider::TicksRight
                && label_max_width > 0;
        // Left
        const int whole_left = whole_rect.left();
        const int left_label_right = whole_left +
                (using_left_labels ? label_max_width : 0);
        const int left_tick_left = left_label_right +
                (using_left_labels ? m_tick_label_gap : 0);
        const int left_tick_right = left_tick_left +
                (has_left_tick ? m_tick_length : 0);
        // Right, working leftwards
        const int whole_right = whole_rect.right();
        const int right_label_left = whole_right -
                (using_right_labels ? label_max_width : 0);
        const int right_tick_right = right_label_left -
                (using_right_labels ? m_tick_label_gap : 0);
        const int right_tick_left = right_tick_right -
                (has_right_tick ? m_tick_length : 0);
        // OK:
        for (int i = min_value; i <= max_value; i += interval) {
            Qt::Alignment valign = Qt::AlignVCenter;
            if (m_edge_in_extreme_labels) {
                const bool bottommost = i == first_value;
                const bool topmost = i == last_value;
                if (bottommost) {
                    valign = Qt::AlignBottom;
                } else if (topmost) {
                    valign = Qt::AlignTop;
                }
            }
            const double prop = getDrawingProportion(i);
            // y position (tick position and vertical alignment point of text)
            const int y = getTickPos(prop, active_groove.top(),
                                     active_groove.height());
            const bool has_label = m_tick_labels.contains(i);
            QString label_text;
            if (has_label) {
                label_text = m_tick_labels[i];
            }
            // Left tick
            if (has_left_tick) {
                p.drawLine(left_tick_left, y, left_tick_right, y);
            }
            // Left label
            if (has_label && using_left_labels) {
                graphicsfunc::drawText(p, left_label_right, y,
                                       Qt::AlignRight | valign, label_text);
            }
            // Right tick
            if (has_right_tick) {
                p.drawLine(right_tick_left, y, right_tick_right, y);
            }
            // Right label
            if (has_label && using_right_labels) {
                graphicsfunc::drawText(p, right_label_left, y,
                                       Qt::AlignLeft | valign, label_text);
            }
        }
    }
}


int TickSlider::getTickPos(const double drawing_proportion,
                           const int active_groove_start,
                           const int active_groove_extent) const
{
    // - We have a slider that is drawn as slider_extent pixels long.
    // - That value includes overhang for the handle: half the handle can
    //   overhang at each end.
    const double start = static_cast<double>(active_groove_start);
    const double extent = static_cast<double>(active_groove_extent);

    const double tickpos = start + (drawing_proportion * extent);

    return static_cast<int>(round(tickpos));
}


QSize TickSlider::sizeHint() const
{
    const QSize slider_size = sliderSizeWithHandle(false);
    const QSize whole_size = wholeWidgetSize(slider_size);
#ifdef DEBUG_SIZE
    qDebug() << Q_FUNC_INFO
             << "// horizontal" << isHorizontal()
             << "slider_size" << slider_size
             << "whole_size" << whole_size;
#endif
    return whole_size;
}


QSize TickSlider::minimumSizeHint() const
{
    const QSize slider_size = sliderSizeWithHandle(true);
    const QSize whole_size = wholeWidgetSize(slider_size);
#ifdef DEBUG_SIZE
    qDebug() << Q_FUNC_INFO
             << "// horizontal" << isHorizontal()
             << "slider_size" << slider_size
             << "whole_size" << whole_size;
#endif
    return whole_size;
}


QSize TickSlider::sliderSizeWithHandle(bool minimum_size) const
{
    // Get base size
    ensurePolished();
    m_slider.ensurePolished();  // redundant?
    QSize slider_size = minimum_size ? m_slider.minimumSizeHint()
                                     : m_slider.sizeHint();
    const bool horizontal = isHorizontal();
    const bool vertical = !horizontal;
    int& perpendicular_size = horizontal ? slider_size.rheight() : slider_size.rwidth();
    int& parallel_size = horizontal ? slider_size.rwidth() : slider_size.rheight();

    // Expand for handle (in the perpendicular direction)
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    const QRect handle = m_slider.style()->subControlRect(
                QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, &m_slider);
    const int handle_perpendicular = horizontal ? handle.height() : handle.width();
    perpendicular_size = qMax(perpendicular_size, handle_perpendicular);

    // - If we have specified an absolute size [m_slider_target_length_px > 0],
    //   we calculate a fixed size...
    // - Unless both
    //   (a) we are asking for the minimumSizeHint(), not the sizeHint()
    //       [minimum_size], and
    //   (b) we allow ourselves to shrink if required
    //       [m_absolute_size_can_shrink].
    const bool use_fixed_size =
            m_slider_target_length_px > 0 &&
            !(m_absolute_size_can_shrink && minimum_size);

    if (use_fixed_size) {
        // Fixed size in the parallel-to-groove direction.
        const int handle_parallel = horizontal ? handle.width() : handle.height();
        parallel_size = m_slider_target_length_px +
                        2 * m_groove_margin_px +
                        handle_parallel;
        // +-------------------------
        // |        groove margin
        // | +-----------------------
        // | |  _
        // | | / \  groove              h = handle
        // | || h |
        // | | \_/
        // | +-----------------------
        // |
        // +-------------------------
        //
        //      | active region starts here
        //    | plus half-handle each side
        // | plus groove margin each side
        //
        // ... the active region is m_slider_target_length_px; then we have
        //     a half-handle each side, and the groove margin each side

    } else {
        // Expand for labels (in the parallel direction)
        const bool using_labels = usingLabels();
        const bool expand_for_labels = using_labels && (!minimum_size || vertical);

        if (expand_for_labels) {
            const QSize label = maxLabelSize();
            const int n_potential_labels = (maximum() - minimum()) / tickInterval();
            const int& parallel_label_size = horizontal ? label.width() : label.height();
            const int min_parallel_size_for_labels =
                    n_potential_labels * parallel_label_size +
                    (n_potential_labels - 1) * m_min_interlabel_gap;
            parallel_size = qMax(parallel_size, min_parallel_size_for_labels);
        }
    }

    return slider_size;
}


Margins TickSlider::getSurround() const
{
    ensurePolished();

    Margins surround;

    // Modify the size for labels etc.

    const QSize label = maxLabelSize();
    const bool horizontal = isHorizontal();
    int& perpendicular_la = horizontal ? surround.rtop() : surround.rleft();
    int& perpendicular_rb = horizontal ? surround.rbottom() : surround.rright();
    const int& perpendicular_label_size = horizontal ? label.height() : label.width();

    const QSlider::TickPosition tickpos = tickPosition();
    const QSlider::TickPosition labelpos = tickLabelPosition();
    // "la" left or above
    // "rb" right or below
    const bool ticks_la = horizontal ? (tickpos & QSlider::TicksAbove)
                                     : (tickpos & QSlider::TicksLeft);
    const bool ticks_rb = horizontal ? (tickpos & QSlider::TicksBelow)
                                     : (tickpos & QSlider::TicksRight);
    const bool labels_la = horizontal ? (labelpos & QSlider::TicksAbove)
                                      : (labelpos & QSlider::TicksLeft);
    const bool labels_rb = horizontal ? (labelpos & QSlider::TicksBelow)
                                      : (labelpos & QSlider::TicksRight);

    // PERPENDICULAR TO THE SLIDER: stuff around the edge
    // Left/above to right/bottom:
    if (labels_la) {
        perpendicular_la += perpendicular_label_size;
    }
    if (labels_la && ticks_la) {
        perpendicular_la += m_tick_label_gap;
    }
    if (ticks_la) {
        perpendicular_la += m_tick_length;
    }
    if (labels_la || ticks_la) {
        perpendicular_la += m_gap_to_slider;
    }
    // Then the slider (which we've already accounted for); then...
    if (labels_rb || ticks_rb) {
        perpendicular_rb += m_gap_to_slider;
    }
    if (ticks_rb) {
        perpendicular_rb += m_tick_length;
    }
    if (labels_rb && ticks_rb) {
        perpendicular_rb += m_tick_label_gap;
    }
    if (labels_rb) {
        perpendicular_rb += perpendicular_label_size;
    }

    // ON ALL SIDES: overspill
    const Margins overspill = getLabelOverspill();
    overspill.addMarginsToInPlace(surround);

#ifdef DEBUG_SIZE
    qDebug() << Q_FUNC_INFO
             << "// horizontal" << horizontal
             << "// surround size" << surround;
#endif

    return surround;
}


QSize TickSlider::wholeWidgetSize(const QSize& slider_size) const
{
    QSize size = slider_size;
    ensurePolished();

    const Margins surround = getSurround();
    surround.addMarginsToInPlace(size);

    return size;
}


Margins TickSlider::getLabelOverspill() const
{
    Margins& overspill = m_cached_overspill;  // shorthand
    if (m_is_overspill_cached) {
        return overspill;
    }
    overspill = Margins();
    if (m_edge_in_extreme_labels) {
        return overspill;
    }
    const bool horizontal = isHorizontal();
    const bool inverted = invertedAppearance();
    const int first_value = inverted ? maximum() : minimum();
    const int last_value = inverted ? minimum() : maximum();
    const QFontMetrics fm = fontMetrics();
    if (horizontal) {
        if (m_tick_labels.contains(first_value)) {
            const QString label_text = m_tick_labels[first_value];
            const QSize label_size = fm.size(FM_TEXTFLAGS, label_text);
            overspill.setLeft(label_size.width() / 2);
        }
        if (m_tick_labels.contains(last_value)) {
            const QString label_text = m_tick_labels[last_value];
            const QSize label_size = fm.size(FM_TEXTFLAGS, label_text);
            overspill.setRight(label_size.width() / 2);
        }
    } else {
        // Vertical
        if (m_tick_labels.contains(first_value)) {
            const QString label_text = m_tick_labels[first_value];
            const QSize label_size = fm.size(FM_TEXTFLAGS, label_text);
            overspill.setBottom(label_size.width() / 2);
        }
        if (m_tick_labels.contains(last_value)) {
            const QString label_text = m_tick_labels[last_value];
            const QSize label_size = fm.size(FM_TEXTFLAGS, label_text);
            overspill.setTop(label_size.width() / 2);
        }
    }
    if (m_symmetric_overspill) {
        overspill.rleft() =
                overspill.rright() =
                qMax(overspill.left(), overspill.right());
        overspill.rtop() =
                overspill.rbottom() =
                qMax(overspill.top(), overspill.bottom());
    }
    return overspill;
}


void TickSlider::clearCache()
{
    m_is_overspill_cached = false;
}


QRect TickSlider::getSliderRect() const
{
    const Margins surround = getSurround();
    const QRect entire_rect = rect();
    QRect slider_rect = surround.removeMarginsFrom(entire_rect);
    return slider_rect;
}


QRect TickSlider::getSliderActiveGroove() const
{
    // Ensure stylesheets are applied, etc.
    m_slider.ensurePolished();
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    // Read the handle. Its absolute position will be wrong (it'll be relative
    // to the slider) but we only care about its width/height.
    const QRect handle = m_slider.style()->subControlRect(
                QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, &m_slider);
    // Fine the entire extent of the groove. This includes borders around the
    // "active" part of the groove.
    // What we get first is wrong, in that it's relative to the QSlider:
    QRect groove = m_slider.style()->subControlRect(
                QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, &m_slider);
    const Margins surround = getSurround();
    surround.moveRectByTopLeftMarginsInPlace(groove);
#ifdef DEBUG_SIZE
    qDebug() << "Slider handle:" << handle;
    qDebug() << "Slider groove:" << groove;
#endif
    // We can't read the internal margins/borders.
    // So we have to do this by knowing we told it what to do in the first
    // place, and what we told it.
    Margins groove_margins(m_groove_margin_px);  // this much on all sides
    if (isHorizontal()) {
        const int half_handle_width = handle.width() / 2;
        groove_margins.addLeft(half_handle_width);
        groove_margins.addRight(half_handle_width);
    } else {
        const int half_handle_height = handle.height() / 2;
        groove_margins.addTop(half_handle_height);
        groove_margins.addBottom(half_handle_height);
    }
    const QRect active_groove = groove_margins.removeMarginsFrom(groove);
    return active_groove;
}


void TickSlider::repositionSlider()
{
    m_slider.setGeometry(getSliderRect());
}


void TickSlider::moveEvent(QMoveEvent* event)
{
    Q_UNUSED(event)
    repositionSlider();
}


void TickSlider::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
    repositionSlider();
}


bool TickSlider::event(QEvent* event)
{
    switch (event->type()) {

    // Need cache clearing:
    case QEvent::Type::ContentsRectChange:
    case QEvent::Type::DynamicPropertyChange:
    case QEvent::Type::FontChange:
    case QEvent::Type::Polish:
    case QEvent::Type::PolishRequest:
    case QEvent::Type::Resize:
    case QEvent::Type::StyleChange:
    case QEvent::Type::ScreenChangeInternal:  // undocumented? But see https://git.merproject.org/mer-core/qtbase/commit/49194275e02a9d6373767d6485bd8ebeeb0abba5
        clearCache();
        break;

    default:
        break;
    }
    return QWidget::event(event);
}


void TickSlider::initStyleOption(QStyleOptionSlider* option) const
{
    // http://doc.qt.io/qt-5/qstyleoptionslider.html

    // Fetch options; http://doc.qt.io/qt-5/qslider.html#initStyleOption.

    if (!option) {
        return;
    }

    const bool horizontal = isHorizontal();

    // This populates opt.rect with our bounding box, m_slider.rect().
    option->initFrom(&m_slider);

    option->subControls = QStyle::SC_None;
    option->activeSubControls = QStyle::SC_None;
    option->orientation = m_slider.orientation();
    option->maximum = m_slider.maximum();
    option->minimum = m_slider.minimum();
    option->tickPosition = m_slider.tickPosition();
    option->tickInterval = m_slider.tickInterval();
    option->upsideDown = horizontal
            ? (m_slider.invertedAppearance() != (option->direction == Qt::RightToLeft))
            : (!m_slider.invertedAppearance());
    option->direction = Qt::LeftToRight; // we use the upsideDown option instead

    // d->position is QAbstractSliderPrivate::position, which is an int
    // ... is it a zero-based int? Let's try this:
    option->sliderPosition = (m_slider.value() - m_slider.minimum()) / m_slider.singleStep();

    option->sliderValue = m_slider.value();
    option->singleStep = m_slider.singleStep();
    option->pageStep = m_slider.pageStep();
    if (horizontal) {
        option->state |= QStyle::State_Horizontal;
    }
}


void TickSlider::setCssName(const QString& name)
{
    setObjectName(name);
    m_slider.setObjectName(name);
}


void TickSlider::resetSizePolicy()
{
    const bool horizontal = isHorizontal();
    const bool fixed_length = m_slider_target_length_px > 0;
    const QSizePolicy::Policy parallel =
            fixed_length ? (m_absolute_size_can_shrink ? QSizePolicy::Maximum
                                                       : QSizePolicy::Fixed)
                         : QSizePolicy::Preferred;
    const QSizePolicy::Policy perpendicular = QSizePolicy::Fixed;
    QSizePolicy::Policy h = horizontal ? parallel : perpendicular;
    QSizePolicy::Policy v = horizontal ? perpendicular : parallel;
    setSizePolicy(h, v);
}


void TickSlider::setOrientation(const Qt::Orientation orientation)
{
    m_slider.setOrientation(orientation);
    resetSizePolicy();
}
