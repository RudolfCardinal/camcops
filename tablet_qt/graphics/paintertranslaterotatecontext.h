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
#include <QPointF>
#include <QtGlobal>
class QPainter;

// Implements a translation/rotation context for a QPainter.
// When the class is constructed, it (1) translates the painter by "at" and
// (2) rotates by "rotate_clockwise_deg". When it's destroyed, it reverses
// those transformations.

class PainterTranslateRotateContext
{
public:
    PainterTranslateRotateContext(QPainter& painter, const QPointF& at,
                                  qreal rotate_clockwise_deg);
    ~PainterTranslateRotateContext();
protected:
    QPainter& m_painter;
    QPointF m_at;
    qreal m_rotate_clockwise_deg;
};
