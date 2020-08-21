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

// #define SCREENLIKEGRAPHICSVIEW_REDO_FIT_IN_VIEW

#include <QColor>
#include <QGraphicsView>


class ScreenLikeGraphicsView : public QGraphicsView
{
    // This is a QGraphicsView that is intended to show the whole of
    // QGraphicsScene, zooming as necessary/possible and using no scroll bars,
    // but keeping the scene's aspect ratio without distortion.
    // Used e.g. by the ID/ED-3D task and similar.

public:
    // Default constructor.
    ScreenLikeGraphicsView(QWidget* parent = nullptr);

    // Construct with a scene.
    ScreenLikeGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr);

    // Set the background colour.
    void setBackgroundColour(const QColor& colour);

    // Standard Qt event overrides.
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;

protected:
#ifdef SCREENLIKEGRAPHICSVIEW_REDO_FIT_IN_VIEW
    // Not currently used.
    void fitInView2(
            const QRectF& rect,
            Qt::AspectRatioMode aspect_ratio_mode = Qt::IgnoreAspectRatio);

    // Not currently used.
    void fitInView2(
            qreal x, qreal y, qreal w, qreal h,
            Qt::AspectRatioMode aspect_ratio_mode = Qt::IgnoreAspectRatio);
#endif

    // "Fit the contents to our view."
    void fitView();
};
