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

#include <QGraphicsView>

class ZoomableGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    ZoomableGraphicsView(QGraphicsScene* scene,
                         bool can_scale_smaller_than_viewport,
                         qreal min_scale = 0.2,
                         qreal max_scale = 5.0,
                         qreal scale_step_factor = 1.1);

    // ------------------------------------------------------------------------
    // To zoom:
    // ------------------------------------------------------------------------

    // Mouse wheel has been rotated.
    virtual void wheelEvent(QWheelEvent* event) override;

    // We implement two-finger zoom here.
    virtual bool viewportEvent(QEvent* event) override;

    // ------------------------------------------------------------------------
    // Other events:
    // ------------------------------------------------------------------------

    // View has been resized.
    virtual void resizeEvent(QResizeEvent* event) override;

    // View is being shown
    virtual void showEvent(QShowEvent* event) override;

    // ------------------------------------------------------------------------
    // Scaling
    // ------------------------------------------------------------------------

    // Zoom to a specific scale factor (1.0 meaning "full size"), >1 meaning
    // bigger, etc.
    void rescale(qreal scale);

    // Ensure m_scale is sensible, then zoom ourselves accordingly.
    // (APPLIES m_scale to the view.)
    void rescale();

    // Zoom ourselves (altering m_scale) to fit the contents into the view as
    // best we can. If we have a big view, show the widget at 1:1 scale.
    // If we have a small view, show the widget as large as will fit.
    // We judge the contents size via sceneRect().
    // (Scales appropriately then READS the best m_scale from the view.)
    void fitView();

    // The size of the viewport's contents (being the size of the viewport,
    // minus the scrollbar size?).
    QSize viewportContentsSize() const;

protected:
    bool m_can_scale_smaller_than_viewport;
    qreal m_min_scale;
    qreal m_max_scale;
    qreal m_scale_step_factor;
    qreal m_previous_scale;
    qreal m_scale;
    qreal m_smallest_fit_scale;
    bool m_two_finger_zooming;
    qreal m_two_finger_start_scale;
};
