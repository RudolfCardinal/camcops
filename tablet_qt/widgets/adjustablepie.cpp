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

// #define DEBUG_CHANGES
// #define DEBUG_DRAW
// #define DEBUG_EVENTS
// #define DEBUG_MOVE

#include "adjustablepie.h"
#include <cmath>  // for std::fmod
#include <QDebug>
#include <QFrame>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include "common/colourdefs.h"
#include "graphics/geometry.h"
#include "graphics/graphicsfunc.h"
#include "graphics/linesegment.h"
#include "graphics/paintertranslaterotatecontext.h"
#include "lib/containers.h"
#include "lib/timerfunc.h"
#include "lib/uifunc.h"
using containers::forceVectorSize;
using geometry::convertHeadingToTrueNorth;
using geometry::DEG_0;
using geometry::DEG_90;
using geometry::DEG_180;
using geometry::DEG_270;
using geometry::DEG_360;
using geometry::distanceBetween;
using geometry::headingInRange;
using geometry::headingNearlyEq;
using geometry::headingToPolarThetaDeg;
using geometry::lineFromPointInHeadingWithRadius;
using geometry::normalizeHeading;
using geometry::polarToCartesian;
using geometry::polarThetaDeg;
using geometry::polarThetaToHeading;
using graphicsfunc::drawSector;
using graphicsfunc::drawText;

// ============================================================================
// Constants
// ============================================================================

const PenBrush DEFAULT_SECTOR_PENBRUSH(QCOLOR_BLACK, QCOLOR_GREEN);
const PenBrush DEFAULT_CURSOR_PENBRUSH(QCOLOR_BLACK, QCOLOR_RED);
const PenBrush DEFAULT_CURSOR_ACTIVE_PENBRUSH(QCOLOR_BLUE, QCOLOR_YELLOW);
const QColor DEFAULT_LABEL_COLOUR(QCOLOR_DARKBLUE);

// ============================================================================
// #defines
// ============================================================================

#define ENSURE_SECTOR_INDEX_OK_OR_RETURN(sector_index) \
    if ((sector_index) < 0 || (sector_index) >= m_n_sectors) { \
        qWarning() << Q_FUNC_INFO << "Bad sector index:" << (sector_index); \
        return; \
    }
#define ENSURE_CURSOR_INDEX_OK_OR_RETURN(cursor_index) \
    if ((cursor_index) < 0 || (cursor_index) >= m_n_sectors - 1) { \
        qWarning() << Q_FUNC_INFO << "Bad cursor index:" << (cursor_index); \
        return; \
    }
#define ENSURE_VECTOR_SIZE_MATCHES_SECTORS(vec) \
    if ((vec).size() != m_n_sectors) { \
        qWarning() << Q_FUNC_INFO << "Bad vector size:" << (vec).size() \
                   << "- should match #sectors of" << m_n_sectors; \
        return; \
    }
#define ENSURE_VECTOR_SIZE_MATCHES_CURSORS(vec) \
    if ((vec).size() != m_n_sectors - 1) { \
        qWarning() << Q_FUNC_INFO << "Bad vector size:" << (vec).size() \
                   << "- should match #cursors of" << m_n_sectors - 1; \
        return; \
    }


// ============================================================================
// Construction and configuration
// ============================================================================

AdjustablePie::AdjustablePie(const int n_sectors, QWidget* parent) :
    QWidget(parent),
    m_background_brush(QBrush(QCOLOR_TRANSPARENT)),
    m_centre_label_colour(QCOLOR_BLACK),
    m_sector_radius(75),
    m_cursor_inner_radius(75),
    m_cursor_outer_radius(125),
    m_cursor_angle_degrees(30),
    m_label_start_radius(125),
    m_overall_radius(200),
    m_base_compass_heading_deg(180),
    m_reporting_delay_ms(0),
    m_rotate_labels(true),
    m_user_dragging_cursor(false),
    m_cursor_num_being_dragged(-1)
{
    uifunc::setBackgroundColour(this, QCOLOR_TRANSPARENT);
    setContentsMargins(0, 0, 0, 0);

    setNSectors(n_sectors);
    timerfunc::makeSingleShotTimer(m_timer);
    connect(m_timer.data(), &QTimer::timeout,
            this, &AdjustablePie::report);
}


void AdjustablePie::setNSectors(int n_sectors)
{
    if (n_sectors < 1) {
        qWarning() << Q_FUNC_INFO << "Bad n_sectors:" << n_sectors;
        return;
    }
    m_n_sectors = n_sectors;
    normalize();
}


int AdjustablePie::nSectors() const
{
    return m_n_sectors;
}


void AdjustablePie::setBackgroundBrush(const QBrush& brush)
{
    m_background_brush = brush;
}


void AdjustablePie::setSectorPenBrush(const int sector_index,
                                      const PenBrush& penbrush)
{
    ENSURE_SECTOR_INDEX_OK_OR_RETURN(sector_index);
    m_sector_penbrushes[sector_index] = penbrush;
    update();
}


void AdjustablePie::setSectorPenBrushes(const QVector<PenBrush>& penbrushes)
{
    ENSURE_VECTOR_SIZE_MATCHES_SECTORS(penbrushes);
    m_sector_penbrushes = penbrushes;
    update();
}


void AdjustablePie::setLabel(const int sector_index, const QString& label)
{
    ENSURE_SECTOR_INDEX_OK_OR_RETURN(sector_index);
    m_labels[sector_index] = label;
    update();
}


void AdjustablePie::setLabels(const QVector<QString>& labels)
{
    ENSURE_VECTOR_SIZE_MATCHES_SECTORS(labels);
    m_labels = labels;
    update();
}


void AdjustablePie::setLabelColour(const int sector_index, const QColor& colour)
{
    ENSURE_SECTOR_INDEX_OK_OR_RETURN(sector_index);
    m_label_colours[sector_index] = colour;
    update();
}


void AdjustablePie::setLabelColours(const QVector<QColor>& colours)
{
    ENSURE_VECTOR_SIZE_MATCHES_SECTORS(colours);
    m_label_colours = colours;
    update();
}


void AdjustablePie::setLabelRotation(bool rotate)
{
    m_rotate_labels = rotate;
}


int AdjustablePie::nCursors() const
{
    return m_n_sectors - 1;
}


void AdjustablePie::setCursorPenBrush(const int cursor_index,
                                      const PenBrush& penbrush)
{
    ENSURE_CURSOR_INDEX_OK_OR_RETURN(cursor_index);
    m_cursor_penbrushes[cursor_index] = penbrush;
    update();
}


void AdjustablePie::setCursorPenBrushes(const QVector<PenBrush>& penbrushes)
{
    ENSURE_VECTOR_SIZE_MATCHES_CURSORS(penbrushes);
    m_cursor_penbrushes = penbrushes;
    update();
}


void AdjustablePie::setCursorActivePenBrush(const int cursor_index,
                                            const PenBrush& penbrush)
{
    ENSURE_CURSOR_INDEX_OK_OR_RETURN(cursor_index);
    m_cursor_active_penbrushes[cursor_index] = penbrush;
    update();
}


void AdjustablePie::setCursorActivePenBrushes(
        const QVector<PenBrush>& penbrushes)
{
    ENSURE_VECTOR_SIZE_MATCHES_CURSORS(penbrushes);
    m_cursor_active_penbrushes = penbrushes;
    update();
}


void AdjustablePie::setOuterLabelFont(const QFont& font)
{
    m_outer_label_font = font;
}


void AdjustablePie::setSectorRadius(const qreal radius)
{
    m_sector_radius = radius;
    updateGeometry();
}


void AdjustablePie::setCursorRadius(qreal inner_radius, qreal outer_radius)
{
    if (inner_radius > outer_radius) {
        std::swap(inner_radius, outer_radius);
    }
    m_cursor_inner_radius = inner_radius;
    m_cursor_outer_radius = outer_radius;
    updateGeometry();
}


void AdjustablePie::setCursorAngle(const qreal degrees)
{
    m_cursor_angle_degrees = degrees;
    update();
}


void AdjustablePie::setLabelStartRadius(const qreal radius)
{
    m_label_start_radius = radius;
}


void AdjustablePie::setCentreLabel(const QString& label)
{
    m_centre_label = label;
    update();
}


void AdjustablePie::setCentreLabelFont(const QFont& font)
{
    m_centre_label_font = font;
    update();
}


void AdjustablePie::setCentreLabelColour(const QColor& colour)
{
    m_centre_label_colour = colour;
    update();
}


void AdjustablePie::setOverallRadius(const qreal radius)
{
    m_overall_radius = radius;
}


void AdjustablePie::setBaseCompassHeading(const qreal degrees)
{
    m_base_compass_heading_deg = degrees;
}


void AdjustablePie::setReportingDelay(const int delay_ms)
{
    m_reporting_delay_ms = delay_ms;
}


void AdjustablePie::setProportionCumulative(const int cursor_index,
                                            const qreal proportion)
{
    ENSURE_CURSOR_INDEX_OK_OR_RETURN(cursor_index);
    if (proportion < 0.0 || proportion > 1.0) {
        qWarning() << Q_FUNC_INFO << "Bad proportion:" << proportion;
        return;
    }

    m_cursor_props_cum[cursor_index] = proportion;
    for (int i = 0; i < m_n_sectors - 1; ++i) {
        if (i < cursor_index) {
            m_cursor_props_cum[i] = qBound(
                        0.0, m_cursor_props_cum.at(i), proportion);
        } else if (i > cursor_index) {
            m_cursor_props_cum[i] = qBound(
                        proportion, m_cursor_props_cum.at(i), 1.0);
        }
    }
#ifdef DEBUG_CHANGES
    qDebug() << Q_FUNC_INFO << m_cursor_props_cum;
#endif
    update();
}


void AdjustablePie::setProportions(const QVector<qreal>& proportions)
{
    for (const qreal p : proportions) {
        if (p < 0.0 || p > 1.0) {
            qWarning() << Q_FUNC_INFO << "Bad proportion:" << p;
            return;
        }
    }
    const int n = proportions.size();
    QVector<qreal> props;
    if (n == m_n_sectors - 1) {
        // Set cursor proportions directly
        props = proportions;
    } else if (n == m_n_sectors) {
        // Set from all but the last
        props = proportions.toList().mid(0, n - 1).toVector();
    } else {
        qWarning() << Q_FUNC_INFO << "proportions has a bad size of" << n;
        return;
    }
    m_cursor_props_cum.clear();
    qreal sum = 0.0;
    for (auto p : props) {
        sum += p;
        m_cursor_props_cum.append(sum);
    }
    normalizeProportions();
    update();
}


void AdjustablePie::setProportionsCumulative(const QVector<qreal>& proportions)
{
    for (qreal p : proportions) {
        if (p < 0.0 || p > 1.0) {
            qWarning() << Q_FUNC_INFO << "Bad proportion:" << p;
            return;
        }
    }
    const int n = proportions.size();
    if (n == m_n_sectors - 1) {
        // Set cursor proportions directly
        m_cursor_props_cum = proportions;
    } else if (n == m_n_sectors) {
        // Set from all but the last
        m_cursor_props_cum = proportions.toList().mid(0, n - 1).toVector();
    } else {
        qWarning() << Q_FUNC_INFO << "proportions has a bad size of" << n;
        return;
    }
    normalizeProportions();
    update();
}


qreal AdjustablePie::sectorProportionCumulative(const int sector_index) const
{
    if (sector_index < m_n_sectors - 1) {
        return m_cursor_props_cum.at(sector_index);
    }
    return 1.0;
}


// ============================================================================
// Widget information and events
// ============================================================================

QSize AdjustablePie::sizeHint() const
{
    auto diameter = static_cast<int>(m_overall_radius * 2);
    return QSize(diameter, diameter);
}


void AdjustablePie::paintEvent(QPaintEvent* event)
{
    // We use virtual coordinates with the pie centred at (0,0).
    // Then we translate to the actual centre (by adding centre).
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRect cr = contentsRect();
    const QPoint widget_centre = cr.center();

    // Paint background
    p.setPen(QPen(Qt::PenStyle::NoPen));
    p.setBrush(m_background_brush);
    p.drawRect(cr);

#ifdef DEBUG_DRAW
    qDebug() << Q_FUNC_INFO
             << "contentsRect()" << cr
             << "widget_centre" << widget_centre
             << "m_cursor_props_cum" << m_cursor_props_cum;
#endif

    // ------------------------------------------------------------------------
    // Sectors, cursors, labels
    // ------------------------------------------------------------------------
    // Draw them separately, in case they overlap (e.g. thick pens).

    const QPointF sector_tip = widget_centre;
    const qreal cursor_radius = m_cursor_outer_radius - m_cursor_inner_radius;

    qreal sector_start_angle;
    qreal sector_end_angle;
    auto startLoop = [this, &sector_start_angle, &sector_end_angle] (int i) -> void {
        const qreal prev_prop = i == 0 ? 0.0 : sectorProportionCumulative(i - 1);
        sector_start_angle = prev_prop * DEG_360;
        const qreal prop = sectorProportionCumulative(i);
        sector_end_angle = prop * DEG_360;
    };
    auto endLoop = [&sector_start_angle, &sector_end_angle] () -> void {
        sector_start_angle = sector_end_angle;  // for the next one
    };

    // ------------------------------------------------------------------------
    // Sectors
    // ------------------------------------------------------------------------
    for (int i = 0; i < m_n_sectors; ++i) {
        startLoop(i);
        // Sector
        const PenBrush& spb = m_sector_penbrushes.at(i);
        if (m_n_sectors == 1) {
            // Avoid the "first cut" line:
            p.setPen(spb.pen);
            p.setBrush(spb.brush);
            p.drawEllipse(sector_tip, m_sector_radius, m_sector_radius);
        } else {
            drawSector(p, sector_tip, m_sector_radius,
                       convertAngleToQt(sector_start_angle),
                       convertAngleToQt(sector_end_angle),
                       true,
                       spb.pen, spb.brush);
        }
        endLoop();
    }
    // ------------------------------------------------------------------------
    // Cursors
    // ------------------------------------------------------------------------
    for (int i = 0; i < m_n_sectors; ++i) {
        startLoop(i);
        if (i < m_n_sectors - 1) {
            const qreal cursor_half_angle = m_cursor_angle_degrees / 2.0;
            const qreal cursor_start_angle = sector_end_angle - cursor_half_angle;
            const qreal cursor_end_angle = sector_end_angle + cursor_half_angle;
            const QPointF cursor_tip = widget_centre +
                    polarToCartesian(m_cursor_inner_radius,
                                     convertAngleToQt(sector_end_angle));
            const PenBrush& cpb = (
                        (m_user_dragging_cursor && i == m_cursor_num_being_dragged)
                        ? m_cursor_active_penbrushes
                        : m_cursor_penbrushes).at(i);
            drawSector(p, cursor_tip, cursor_radius,
                       convertAngleToQt(cursor_start_angle),
                       convertAngleToQt(cursor_end_angle), true,
                       cpb.pen, cpb.brush);
        }
        endLoop();
    }
    // ------------------------------------------------------------------------
    // Labels
    // ------------------------------------------------------------------------
    for (int i = 0; i < m_n_sectors; ++i) {
        // Label
        startLoop(i);
        const qreal sector_mid_angle = sector_end_angle -
                (sector_end_angle - sector_start_angle) / 2;
        const QPointF label_tip = widget_centre +
                polarToCartesian(m_label_start_radius,
                                 convertAngleToQt(sector_mid_angle));
        const QString label = m_labels.at(i);
        if (!label.isEmpty()) {
            const qreal abs_heading = convertHeadingToTrueNorth(
                        sector_mid_angle, m_base_compass_heading_deg);
            // 0 up, 90 right...
            const qreal rotation = abs_heading;
            // Easiest way to think of it: something at 180 is at the top
            // and shouldn't be rotated.
            // Something at 90 is on the left and should be rotated
            // anticlockwise.
            // QPainter::rotate() rotates clockwise.
            p.setPen(m_label_colours.at(i));
            if (m_rotate_labels) {
#ifdef DEBUG_DRAW
            qDebug() << "... label:" << label
                     << "label_tip" << label_tip
                     << "sector_mid_angle" << sector_mid_angle
                     << "rotation" << rotation;
#endif
                PainterTranslateRotateContext ptrc(p, label_tip, rotation);
                // rotation is clockwise

                drawText(p, QPointF(0, 0), label, m_outer_label_font,
                         Qt::AlignHCenter | Qt::AlignBottom);
            } else {
                PainterTranslateRotateContext ptrc(p, label_tip, 0);
                // ... relative to North = up
                const bool hcentre = headingNearlyEq(abs_heading, DEG_0) ||
                        headingNearlyEq(abs_heading, DEG_180);
                const bool left = !hcentre &&
                        headingInRange(DEG_180, abs_heading, DEG_360);
                const bool vcentre = headingNearlyEq(abs_heading, DEG_90) ||
                        headingNearlyEq(abs_heading, DEG_270);
                const bool bottom = !vcentre &&
                        headingInRange(DEG_90, abs_heading, DEG_270);
                Qt::Alignment halign = hcentre ? Qt::AlignHCenter
                                               : (left ? Qt::AlignRight
                                                       : Qt::AlignLeft);
                Qt::Alignment valign = vcentre ? Qt::AlignVCenter
                                               : (bottom ? Qt::AlignTop
                                                         : Qt::AlignBottom);
#ifdef DEBUG_DRAW
            qDebug() << "... label:" << label
                     << "label_tip" << label_tip
                     << "sector_mid_angle" << sector_mid_angle
                     << "hcentre" << hcentre
                     << "left" << left
                     << "vcentre" << vcentre
                     << "bottom" << bottom
                     << "halign" << halign
                     << "valign" << valign;
#endif
                drawText(p, QPointF(0, 0), label, m_outer_label_font,
                         halign | valign);
            }
        }
        endLoop();
    }

    // ------------------------------------------------------------------------
    // Centre label
    // ------------------------------------------------------------------------
    if (!m_centre_label.isEmpty()) {
        p.setPen(m_centre_label_colour);
        drawText(p, widget_centre, m_centre_label, m_centre_label_font,
                 Qt::AlignHCenter | Qt::AlignVCenter);
    }
}


void AdjustablePie::mousePressEvent(QMouseEvent* event)
{
    // We draw the cursors from 0 upwards, so we detect their touching in the
    // reverse order, in case they're stacked.
    const QPoint pos = event->pos();
#ifdef DEBUG_EVENTS
    qDebug() << Q_FUNC_INFO << pos;
#endif
    for (int i = m_n_sectors - 2; i >= 0; --i) {
        if (posInCursor(pos, i)) {
#ifdef DEBUG_MOVE
            qDebug() << "mousePressEvent: in cursor" << i;
#endif
            m_user_dragging_cursor = true;
            m_cursor_num_being_dragged = i;
            m_last_mouse_pos = event->pos();
            qreal mouse_angle = angleOfPos(m_last_mouse_pos);
            qreal cursor_angle = cursorAngle(i);
            m_angle_offset_from_cursor_centre = mouse_angle - cursor_angle;
            update();
            break;
        }
    }
}


void AdjustablePie::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_user_dragging_cursor) {
        // ... for example: an irrelevant mouse click/drag on an inactive
        // part of our widget
        return;
    }
#ifdef DEBUG_EVENTS
    qDebug() << Q_FUNC_INFO;
#endif
    const QPoint newpos = event->pos();
    const QPoint oldpos = m_last_mouse_pos;
    m_last_mouse_pos = newpos;
    const qreal mouse_angle = angleOfPos(newpos);
    const qreal new_cursor_angle = mouse_angle - m_angle_offset_from_cursor_centre;
    const qreal oldprop = m_cursor_props_cum.at(m_cursor_num_being_dragged);
    qreal target_prop = angleToProportion(new_cursor_angle);
    // Post-processing magic since target_prop will never be 1.0:
    if (target_prop <= 0.0 && oldprop > 0.5) {
        target_prop = 1.0;
    }

    if ((oldprop <= 0.0 && target_prop > 0.5) ||
            (oldprop >= 1.0 && target_prop < 0.5)) {
#ifdef DEBUG_MOVE
        qDebug() << "DECISION: already at end stop; ignored";
#endif
        return;
    }

    qreal prop;
    const QPoint pie_centre = contentsRect().center();
    const LineSegment baseline = lineFromPointInHeadingWithRadius(
                pie_centre,
                DEG_0,
                m_base_compass_heading_deg);
    const LineSegment movement(oldpos, newpos);
    const bool from_on = baseline.pointOn(oldpos);
    const bool to_on = baseline.pointOn(newpos);
    const bool crosses = movement.intersects(baseline) && !from_on && !to_on;

    if (oldprop < 0.5 && target_prop > 0.75 &&
            !(oldprop > 0.25 && !crosses)) {
#ifdef DEBUG_MOVE
        qDebug() << "DECISION: hit bottom end stop";
#endif
        prop = 0.0;
    } else if (oldprop > 0.5 && target_prop < 0.25 &&
               !(oldprop < 0.75 && !crosses)) {
#ifdef DEBUG_MOVE
        qDebug() << "DECISION: hit top end stop";
#endif
        prop = 1.0;
    } else {
#ifdef DEBUG_MOVE
        qDebug() << "DECISION: free:" << target_prop;
#endif
        prop = target_prop;
    }

#ifdef DEBUG_MOVE
    qDebug() << "... setting cursor" << m_cursor_num_being_dragged
             << "cumulative proportion to" << prop;
#endif
    setProportionCumulative(m_cursor_num_being_dragged, prop);
    scheduleReport();
}


void AdjustablePie::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (m_user_dragging_cursor) {
        m_user_dragging_cursor = false;
#ifdef DEBUG_EVENTS
        qDebug() << Q_FUNC_INFO;
#endif
        update();
    }
}


// ============================================================================
// Readout
// ============================================================================

QVector<qreal> AdjustablePie::cursorProportionsCumulative() const
{
    return m_cursor_props_cum;
}


QVector<qreal> AdjustablePie::cursorProportions() const
{
    QVector<qreal> props;
    qreal previous = 0.0;
    for (auto p : props) {
        qreal diff = p - previous;
        props.append(diff);
        previous = p;
    }
    return props;
}


QVector<qreal> AdjustablePie::allProportionsCumulative() const
{
    QVector<qreal> props = m_cursor_props_cum;
    qreal sum = 0.0;
    for (qreal p : props) {
        sum += p;
    }
    props.append(1.0 - sum);
    return props;
}


QVector<qreal> AdjustablePie::allProportions() const
{
    QVector<qreal> cum = allProportionsCumulative();
    QVector<qreal> props;
    qreal previous = 0.0;
    for (auto p : cum) {
        qreal diff = p - previous;
        props.append(diff);
        previous = p;
    }
    return props;
}


// ============================================================================
// Internals
// ============================================================================

qreal AdjustablePie::convertAngleToQt(const qreal degrees) const
{
    return headingToPolarThetaDeg(degrees, m_base_compass_heading_deg, false);
}


qreal AdjustablePie::convertAngleToInternal(const qreal degrees) const
{
    return polarThetaToHeading(degrees, m_base_compass_heading_deg);
}


bool AdjustablePie::posInCursor(const QPoint& pos,
                                const int cursor_index) const
{
    const qreal angle = angleOfPos(pos);
    const qreal cursor_angle_centre = cursorAngle(cursor_index);
    const qreal cursor_half_angle = m_cursor_angle_degrees / 2;
    const qreal cursor_min_angle = cursor_angle_centre - cursor_half_angle;
    const qreal cursor_max_angle = cursor_angle_centre + cursor_half_angle;
    if (!headingInRange(cursor_min_angle, angle, cursor_max_angle)) {
        return false;
    }
    qreal radius = radiusOfPos(pos);
    if (radius < m_cursor_inner_radius || radius > m_cursor_outer_radius) {
        return false;
    }
    // Could be refined! This allows the user to grab a cursor by the "missing"
    // bit within its "zone" but not within its true pie shape.
    return true;
}


qreal AdjustablePie::angleToProportion(const qreal angle_degrees) const
{
    // BEWARE that this will never produce 1.0, so some post-processing
    // magic is required for that; see mouseMoveEvent().
    return qBound(0.0, normalizeHeading(angle_degrees) / DEG_360, 1.0);
}


qreal AdjustablePie::proportionToAngle(const qreal proportion) const
{
    return DEG_360 * proportion;
}


qreal AdjustablePie::angleOfPos(const QPoint& pos) const
{
    const QPoint pie_centre = contentsRect().center();
    return convertAngleToInternal(polarThetaDeg(pie_centre, pos));
}


qreal AdjustablePie::radiusOfPos(const QPoint& pos) const
{
    return distanceBetween(pos, contentsRect().center());
}


qreal AdjustablePie::cursorAngle(const int cursor_index) const
{
    const qreal prop = m_cursor_props_cum.at(cursor_index);
    return proportionToAngle(prop);
}


void AdjustablePie::scheduleReport()
{
    if (m_reporting_delay_ms > 0) {
        m_timer->start(m_reporting_delay_ms);
    } else {
        report();
    }
}


void AdjustablePie::report()
{
    emit proportionsChanged(allProportions());
    emit cumulativeProportionsChanged(allProportionsCumulative());
}


void AdjustablePie::normalize()
{
    forceVectorSize(m_sector_penbrushes, m_n_sectors, DEFAULT_SECTOR_PENBRUSH);
    forceVectorSize(m_labels, m_n_sectors);
    forceVectorSize(m_label_colours, m_n_sectors, DEFAULT_LABEL_COLOUR);
    forceVectorSize(m_cursor_penbrushes, m_n_sectors - 1,
                    DEFAULT_CURSOR_PENBRUSH);
    forceVectorSize(m_cursor_active_penbrushes, m_n_sectors - 1,
                    DEFAULT_CURSOR_ACTIVE_PENBRUSH);
    forceVectorSize(m_cursor_props_cum, m_n_sectors - 1, 0.0);
    normalizeProportions();
    update();
}


void AdjustablePie::normalizeProportions()
{
    if (m_n_sectors == 1) {
        return;
    }
    m_cursor_props_cum[0] = qBound(0.0, m_cursor_props_cum[0], 1.0);
    for (int i = 1; i < m_n_sectors - 1; ++i) {
        m_cursor_props_cum[i] = qBound(m_cursor_props_cum.at(i - 1),
                                       m_cursor_props_cum.at(i), 1.0);
    }
}
