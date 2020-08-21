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

#include "css.h"
#include <QColor>
#include <QDebug>
#include <QPen>

namespace css {


QString pixelCss(const int px)
{
    if (px <= 0) {
        return "0";  // no units for 0 in CSS
    }
    return QString("%1px").arg(px);
}


QString ptCss(const qreal pt)
{
    if (pt <= 0) {
        return "0";  // no units for 0 in CSS
    }
    return QString("%1pt").arg(pt);
}


QString colourCss(const QColor& colour)
{
    return QString("rgba(%1,%2,%3,%4)")
            .arg(colour.red())
            .arg(colour.green())
            .arg(colour.blue())
            .arg(colour.alpha());
}


QString penStyleCss(const QPen& pen)
{
    // http://doc.qt.io/qt-4.8/qpen.html#pen-style
    // https://www.w3schools.com/cssref/pr_border-style.asp
    switch (pen.style()) {
    case Qt::NoPen:
        return "none";
    case Qt::SolidLine:
        return "solid";
    case Qt::DashLine:
        return "dashed";
    case Qt::DotLine:
        return "dotted";
    case Qt::DashDotLine:
    case Qt::DashDotDotLine:
    case Qt::CustomDashLine:
    default:
        qWarning() << Q_FUNC_INFO << "Qt pen style not supported in CSS";
        return "dashed";
    }
}


QString penCss(const QPen& pen)
{
    if (pen.width() <= 0 || pen.style() == Qt::NoPen) {
        // http://stackoverflow.com/questions/2922909/should-i-use-border-none-or-border-0
        return "none";
    }
    return QString("%1 %2 %3")
            .arg(pixelCss(pen.width()),
                 penStyleCss(pen),
                 colourCss(pen.color()));
}


QString labelCss(const QColor& colour)
{
    return QString("background-color: rgba(0,0,0,0);"  // transparent
                   "border: 0;"
                   "color: %1;"
                   "margin: 0;"
                   "padding: 0;")
            .arg(colourCss(colour));
}


}  // namespace css
