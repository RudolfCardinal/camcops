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
#include <QGraphicsPixmapItem>


// An extension of QGraphicsPixmapItem that implement opacity.

class GraphicsPixmapItemWithOpacity : public QGraphicsPixmapItem
{
    // Don't use the Q_OBJECT macro! QGraphicsItem is not a QObject.
    // If you do, you'll get:
    // error: ‘staticMetaObject’ is not a member of ‘QGraphicsPixmapItem’
public:
    GraphicsPixmapItemWithOpacity(QGraphicsItem* parent = nullptr);
    GraphicsPixmapItemWithOpacity(const QPixmap& pixmap,
                                  QGraphicsItem* parent = nullptr);
    void setOpacity(qreal opacity);
    virtual void paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option,
                       QWidget* widget) override;
protected:
    qreal m_opacity;
};
