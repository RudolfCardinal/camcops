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

#include "graphicspixmapitemwithopacity.h"
#include <QPainter>


GraphicsPixmapItemWithOpacity::GraphicsPixmapItemWithOpacity(
        QGraphicsItem* parent) :
    QGraphicsPixmapItem(parent),
    m_opacity(1.0)
{
}


GraphicsPixmapItemWithOpacity::GraphicsPixmapItemWithOpacity(
        const QPixmap& pixmap, QGraphicsItem* parent) :
    QGraphicsPixmapItem(pixmap, parent),
    m_opacity(1.0)
{
}


void GraphicsPixmapItemWithOpacity::setOpacity(const qreal opacity)
{
    m_opacity = opacity;
    update();
}


void GraphicsPixmapItemWithOpacity::paint(
        QPainter* painter,
        const QStyleOptionGraphicsItem* option,
        QWidget* widget)
{
    const qreal old_opacity = painter->opacity();
    painter->setOpacity(m_opacity);
    QGraphicsPixmapItem::paint(painter, option, widget);
    painter->setOpacity(old_opacity);  // unsure if this is necessary
}
