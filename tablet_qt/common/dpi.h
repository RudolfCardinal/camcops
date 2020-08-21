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
#include <QtGlobal>
#include <QString>


struct Dpi {
    // Constructor
    Dpi(qreal x, qreal y);

    // Returns the mean of the x and y DPI.
    qreal mean() const;

    // Returns a description in the format "(x=<x>,y=<y>)".
    QString description() const;

    qreal x;  // DPI in the x direction
    qreal y;  // DPI in the y direction
};
