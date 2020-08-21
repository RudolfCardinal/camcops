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
#include <QBrush>
#include <QPen>
#include <QPoint>
#include <QWidget>
#include "graphics/penbrush.h"


class AdjustablePie : public QWidget
{
    // A pie widget. The user can drag "handles" to adjust the size of each
    // slice (sector).

    Q_OBJECT

public:
    // ========================================================================
    // Construction and configuration
    // ========================================================================

    // ------------------------------------------------------------------------
    // The pie has a number of sectors
    // ------------------------------------------------------------------------

    // Constructor. How many sectors?
    // (For n_sectors == 1, no cursors are shown; that's a fixed, whole pie.)
    AdjustablePie(int n_sectors, QWidget* parent = nullptr);

    // Set the number of sectors.
    void setNSectors(int n_sectors);

    // Returns the number of sectors.
    int nSectors() const;

    // ------------------------------------------------------------------------
    // Sectors have colours/brushes
    // ------------------------------------------------------------------------

    // Set the background brush (e.g. background colour).
    void setBackgroundBrush(const QBrush& brush);

    // Set the colours (pen/brush) of an individual sector.
    void setSectorPenBrush(int sector_index, const PenBrush& penbrush);

    // Set the colours (pen/brush) of all the sectors.
    // The size of the vector must match nSectors().
    void setSectorPenBrushes(const QVector<PenBrush>& penbrushes);

    // ------------------------------------------------------------------------
    // Sectors have text labels
    // ------------------------------------------------------------------------

    // Set text label for a single sector.
    void setLabel(int sector_index, const QString& label);

    // Set label for all sectors.
    // The size of the vector must match nSectors().
    void setLabels(const QVector<QString>& labels);

    // Set label text colour for a single sector.
    void setLabelColour(int sector_index, const QColor& colour);

    // Set label text colour for all sectors.
    // The size of the vector must match nSectors().
    void setLabelColours(const QVector<QColor>& colours);

    // Should the labels rotate, so the text baseline is always perpendicular
    // to the radius where they sit, or not (so the baseline is horizontal)?
    void setLabelRotation(bool rotate);

    // Font for sector labels:
    void setOuterLabelFont(const QFont& font);

    // ------------------------------------------------------------------------
    // Cursors (handles) sit as fenceposts between each sector
    // ------------------------------------------------------------------------

    // Return the number of cursors.
    // This is nSectors() - 1.
    int nCursors() const;

    // Set the standard pen/brush for a single cursor.
    void setCursorPenBrush(int cursor_index, const PenBrush& penbrush);

    // Set the standard pen/brush for all cursors.
    // The size of the vector must match nCursors().
    void setCursorPenBrushes(const QVector<PenBrush>& penbrushes);

    // Set the "active" pen/brush for a single cursor.
    // This is the colour to paint when the user is manipulating the cursor.
    void setCursorActivePenBrush(int cursor_index, const PenBrush& penbrush);

    // Set the "active" pen/brush for a single cursor.
    // This is the colour to paint when the user is manipulating the cursor.
    // The size of the vector must match nCursors().
    void setCursorActivePenBrushes(const QVector<PenBrush>& penbrushes);

    // ------------------------------------------------------------------------
    // Proportions for each sector.
    // ------------------------------------------------------------------------

    // Set the proportions, each in the range [0, 1], for the sectors.
    // For n sectors, this function accepts n or (n - 1) proportions; if n are
    // supplied, the last one is ignored.
    // For example, for 4 sectors, you could supply {0.1, 0.6, 0.2} or
    // {0.1, 0.6, 0.2, 0.1}.
    // Proportions will be normalized.
    void setProportions(const QVector<qreal>& proportions);

    // Set the cumulative proportions for the sectors.
    // For n sectors, this function accepts n or (n - 1) proportions; if n are
    // supplied, the last one is ignored.
    // For example, for 4 sectors, you could supply {0.1, 0.7, 0.9} or
    // {0.1, 0.7, 0.9, 1.0} to have the same effect as the setProportions()
    // example above.
    // Proportions will be normalized.
    void setProportionsCumulative(const QVector<qreal>& proportions);

    // Sets the cumulative proportion for a given cursor.
    // This will adjust other cursors/sectors accordingly. It's the function
    // that is used when you drag a single cursor around.
    // Proportions will be normalized.
    void setProportionCumulative(int cursor_index, qreal proportion);

    // ------------------------------------------------------------------------
    // If n_sectors == 1, you may want a centre label.
    // ------------------------------------------------------------------------

    // Set the text for the centre label.
    void setCentreLabel(const QString& label);

    // Set the font for the centre label.
    void setCentreLabelFont(const QFont& font);

    // Set the colour for the centre label.
    void setCentreLabelColour(const QColor& colour);

    // ------------------------------------------------------------------------
    // Overall size/shape
    // ------------------------------------------------------------------------

    // Set the radius of the sectors, in screen coordinate units.
    void setSectorRadius(qreal radius);

    // Set the radii for the inner and outer circles that define the cursors.
    // (For example, setting the inner radius to be a bit bigger than the
    // sector radius, and then the outer radius to be a bit bigger still, gives
    // cursors that are slightly set out from the pie itself.)
    void setCursorRadius(qreal inner_radius, qreal outer_radius);

    // Set the angle that each cursor (itself a sector shape) subtends.
    // Typical might be <= 90; the smaller, the pointier.
    void setCursorAngle(qreal degrees);

    // Set the radius for the labels (typically, for autorotating labels, of
    // the bottom of the text).
    void setLabelStartRadius(qreal radius);

    // Set the radius of the whole widget.
    // (If this is too small, elements such as long text may be clipped.)
    void setOverallRadius(qreal radius);

    // Set the rotation of the pie in degrees: where's the "zero" point?
    // 0 is up, 180 is down. The default is 180.
    void setBaseCompassHeading(qreal degrees);

    // ------------------------------------------------------------------------
    // Dynamic behaviour
    // ------------------------------------------------------------------------

    // Set the delay (is ms) between cursor movement and event generation.
    void setReportingDelay(int delay_ms);

    // ========================================================================
    // Widget information and events: standard Qt overrides.
    // ========================================================================

    virtual QSize sizeHint() const override;
    virtual bool hasHeightForWidth() const override { return false; }
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;

    // ========================================================================
    // Readout
    // ========================================================================

    // Get the n - 1 proportions for each cursor (equivalently, all sectors
    // except the last).
    QVector<qreal> cursorProportions() const;

    // Get the n - 1 cumulative proportions for each cursor (equivalently, all
    // sectors except the last).
    QVector<qreal> cursorProportionsCumulative() const;

    // Get the n sector proportions (which will add up to 1).
    QVector<qreal> allProportions() const;

    // Get the cumulative proportions for the n sectors.
    QVector<qreal> allProportionsCumulative() const;

signals:
    // When a cursor is moved, this signal is emitted, with all n proportions:
    void proportionsChanged(QVector<qreal> all_proportions);

    // When a cursor is moved, this signal is emitted, with all n cumulative
    // proportions:
    void cumulativeProportionsChanged(QVector<qreal> all_proportions);

    // ========================================================================
    // Internals
    // ========================================================================
protected:

    // Ensures that all configuration vectors are the same length, and
    // proportions are normalized. (This "lots of vectors" technique is not
    // especially elegant code.)
    void normalize();

    // Ensure that m_cursor_props_cum is sensible.
    void normalizeProportions();

    // Return the cumulative proportion up to and including the specified
    // sector.
    qreal sectorProportionCumulative(int sector_index) const;

    // Qt uses geometric angles that start at 3 o'clock and go anticlockwise.
    // ... http://doc.qt.io/qt-5/qpainter.html#drawPie
    // In our minds we're using angles that start at 6 o'clock and go clockwise.
    // This takes angles from the second to the first.
    qreal convertAngleToQt(qreal degrees) const;

    // Reverses convertAngleToQt().
    qreal convertAngleToInternal(qreal degrees) const;

    // Is a screen coordinate within the specified cursor?
    bool posInCursor(const QPoint& pos, int cursor_index) const;

    // Converts an "internal" pie angle to a cumulative pie proportion.
    qreal angleToProportion(qreal angle_degrees) const;

    // Reverses angleToProportion().
    qreal proportionToAngle(qreal proportion) const;

    // Returns the pie angle corresponding to a given screen point
    // (using "internal" angles).
    qreal angleOfPos(const QPoint& pos) const;

    // Returns the radius (from the pie's centre) corresponding to a given
    // screen point.
    qreal radiusOfPos(const QPoint& pos) const;

    // Return the "internal" angle at which a given cursor is centred.
    qreal cursorAngle(int cursor_index) const;

    // Call report() after a delay determined by m_reporting_delay_ms.
    void scheduleReport();

    // Emit proportionsChanged() and cumulativeProportionsChanged().
    void report();

protected:

    // Fairly static:

    int m_n_sectors;  // number of sectors, n
    QBrush m_background_brush;  // background of the widget
    QVector<PenBrush> m_sector_penbrushes;  // sector colour; length n
    QVector<QString> m_labels;  // sector labels; length n
    QVector<QColor> m_label_colours;  // sector label colours; length n
    QVector<PenBrush> m_cursor_penbrushes;  // cursor colours; length n - 1
    QVector<PenBrush> m_cursor_active_penbrushes;  // "active" cursor colours; length n - 1
    QFont m_outer_label_font;  // font for sector labels
    QFont m_centre_label_font;  // font for centre label
    QColor m_centre_label_colour;  // colour for centre label
    qreal m_sector_radius;  // radius of the sectors
    qreal m_cursor_inner_radius;  // inner radius of the cursors
    qreal m_cursor_outer_radius;  // outer radius of the cursors
    qreal m_cursor_angle_degrees;  // sector angle of the cursors (visual style)
    qreal m_label_start_radius;  // radius at which to draw sector labels
    QString m_centre_label;  // text for centre label
    qreal m_overall_radius;  // overall widget radius
    qreal m_base_compass_heading_deg;  // pie orientation; see setBaseCompassHeading()
    int m_reporting_delay_ms;  // delay between user movement and reporting
    bool m_rotate_labels;  // rotate the labels? See setLabelRotation().

    // Dynamic:

    QVector<qreal> m_cursor_props_cum;  // cumulative proportions for each cursor; length n - 1

    // Internal state:

    bool m_user_dragging_cursor;  // is the user currently dragging a cursor?
    int m_cursor_num_being_dragged;  // which cursor number is being dragged?
    QPoint m_last_mouse_pos;  // last mouse/touch coordinate
    qreal m_angle_offset_from_cursor_centre;
        // ... if a cursor is "picked up" by the user by touching somewhere
        // other than the very centre of the cursor, we need to know this
        // offset (and it's the angle we care about), so that as the user
        // drags the cursor, it doesn't appear to slip under their
        // mouse/finger.
    QSharedPointer<QTimer> m_timer;  // timer for reporting delay
};
