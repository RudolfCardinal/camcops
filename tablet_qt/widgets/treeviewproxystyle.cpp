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

/*

See also:
- http://doc.qt.io/qt-5/qtwidgets-widgets-styles-example.html

DOES NOT WORK PROPERLY because QFusionStyle::drawPrimitive() doesn't call its
proxy() for the specific things we want.

However, it does read the arrow size from option->rect
... ah, no, it doesn't, it constrains to the size of the image at a maximum.

*Very* tricky!
Reported as bug: https://bugreports.qt.io/browse/QTBUG-62323

*/

#include "treeviewproxystyle.h"
#include <QDebug>

TreeViewProxyStyle::TreeViewProxyStyle(QStyle* style) :
    QProxyStyle(style)
{
}


void TreeViewProxyStyle::drawPrimitive(PrimitiveElement element,
                                       const QStyleOption* option,
                                       QPainter* painter,
                                       const QWidget* widget) const
{
    switch (element) {
    case QStyle::PE_IndicatorArrowUp:
        qDebug() << "arrow up";
        break;
    case QStyle::PE_IndicatorArrowDown:
        qDebug() << "arrow down";
        break;
    case QStyle::PE_IndicatorArrowRight:
        qDebug() << "arrow right";
        break;
    case QStyle::PE_IndicatorArrowLeft:
        qDebug() << "arrow left";
        break;
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}
