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
#include <QPen>
#include <QBrush>


// Represents a pen and a brush together. (It's a common combination!)

struct PenBrush
{
    PenBrush()
    {}
    PenBrush(const QPen& pen, const QBrush& brush) :
        pen(pen),
        brush(brush)
    {}
    PenBrush(const QColor& pen_colour, const QColor& brush_colour) :
        pen(pen_colour),
        brush(brush_colour)
    {}
    PenBrush(const QColor& colour) :
        pen(colour),
        brush(colour)
    {}

    QPen pen;
    QBrush brush;
};
