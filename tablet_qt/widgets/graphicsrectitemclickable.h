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

/*

#include <QBrush>
#include <QGraphicsRectItem>
#include <QPen>


class GraphicsRectItemClickable : public QGraphicsRectItem
{
    // QGraphicsItem does *not* inherit from QObject
    Q_OBJECT
public:
    GraphicsRectItemClickable(QGraphicsItem* parent = nullptr);
    GraphicsRectItemClickable(const QRectF& rect,
                              QGraphicsItem* parent = nullptr);
    GraphicsRectItemClickable(qreal x, qreal y, qreal w, qreal h,
                              QGraphicsItem* parent = nullptr);
    void setRestingAppearance(const QPen& pen, const QBrush& brush);
    void setClickedAppearance(const QPen& pen, const QBrush& brush);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
signals:
    void pressed();  // start of mouse press
    void clicked();  // press -> release = click
protected:
    void commonConstructor();
    void updateColour();
protected:
    QPen m_resting_pen;
    QBrush m_resting_brush;
    QPen m_clicked_pen;
    QBrush m_clicked_brush;
    bool m_pressed;
    bool m_pressing_inside;
};

*/
