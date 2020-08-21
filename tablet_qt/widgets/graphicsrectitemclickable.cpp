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

#include "graphicsrectitemclickable.h"

/*

#include <QGraphicsSceneMouseEvent>
#include "common/colourdefs.h"


GraphicsRectItemClickable::GraphicsRectItemClickable(QGraphicsItem* parent) :
    QGraphicsRectItem(parent)
{
}


GraphicsRectItemClickable::GraphicsRectItemClickable(const QRectF& rect,
                                                     QGraphicsItem* parent) :
    QGraphicsRectItem(rect, parent)
{
}


GraphicsRectItemClickable::GraphicsRectItemClickable(
        qreal x, qreal y, qreal w, qreal h,
        QGraphicsItem* parent) :
    QGraphicsRectItem(x, y, w, h, parent)
{
}


void GraphicsRectItemClickable::commonConstructor()
{
    m_resting_pen = QPen(Qt::NoPen);
    m_resting_brush = QBrush(QCOLOR_TRANSPARENT);
    m_clicked_pen = QPen(Qt::NoPen);
    m_clicked_brush = QBrush(QCOLOR_TRANSPARENT);
    m_pressed = false;
    m_pressing_inside = false;
}


void GraphicsRectItemClickable::setRestingAppearance(const QPen& pen,
                                                     const QBrush& brush)
{
    m_resting_pen = pen;
    m_resting_brush = brush;
    updateColour();
}


void GraphicsRectItemClickable::setClickedAppearance(const QPen& pen,
                                                     const QBrush& brush)
{
    m_clicked_pen = pen;
    m_clicked_brush = brush;
    updateColour();
}


void GraphicsRectItemClickable::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event)
    m_pressed = true;
    m_pressing_inside = true;
    emit pressed();
    updateColour();
}


void GraphicsRectItemClickable::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_pressed) {
        bool was_pressing_inside = m_pressing_inside;
        m_pressing_inside = rect().contains(event->pos());
        if (m_pressing_inside != was_pressing_inside) {
            update();
        }
    }
}


void GraphicsRectItemClickable::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    m_pressed = false;
    if (rect().contains(event->pos())) {
        // release occurred inside widget
        emit clicked();
    }
    update();
}


void GraphicsRectItemClickable::updateColour()
{
    bool clicking = m_pressed && m_pressing_inside;
    setPen(clicking ? m_clicked_pen : m_resting_pen);
    setBrush(clicking ? m_clicked_brush : m_resting_brush);
    update();
}

*/
