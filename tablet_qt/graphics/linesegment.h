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
#include <QRectF>
#include <QTransform>


// Represents a line segment: a line from one piont to another.
//
// Its equation is
//      a(x - xm) + b(y - ym) = c       [1]
//
// http://stackoverflow.com/questions/385305/efficient-maths-algorithm-to-calculate-intersections

class LineSegment {
public:
    LineSegment(const QPointF& from, const QPointF& to);

    // Return c for given values of x and y; see [1] above
    qreal c(qreal x, qreal y) const;  // 0 if point is on the line; otherwise, sign gives side

    // Return c for a point pt = (x, y); see [1] above
    qreal c(const QPointF& pt) const;

    // Which side of the line is the point on?
    // Returns -1 if c < 0; 0 if c == 0; +1 if c > 0; see [1] above.
    int side(const QPointF& pt) const;

    // Is this a point, not a line (i.e. start and end points are identical)?
    bool isPoint() const;

    // Is there overlap in the ranges defined by this line segment's
    // x coordinates and the other's?
    bool xRangesOverlap(const LineSegment& other) const;

    // Is there overlap in the ranges defined by this line segment's
    // y coordinates and the other's?
    bool yRangesOverlap(const LineSegment& other) const;

    // Does this line intersect the other?
    bool intersects(const LineSegment& other) const;

    // Is the point on the line?
    bool pointOn(const QPointF& point) const;

    // What's the angle of this line (heading from "from" to "to")?
    // The angle is in radians where 0 is the direction of the x axis.
    qreal angleRad() const;

    // Return the rectangle (with sides parallel to the x and y axes) that
    // just encloses this line.
    QRectF rect() const;

    /*
    Is the point in the area swept out by the line (swept perpendicular to it)?

    Example 1:

        n       y   y   y           n

         n  ----------y-----------     n

        n     y             y           n

    Example 2:

               n   n
                        n
              y             n
                   /      n
                  /
                 / y    y
         n      /

              n  n

    */
    bool pointInPerpendicularArea(const QPointF& point) const;

protected:
    QPointF from;
    QPointF to;

    qreal xlow;  // minimum x coordinate
    qreal xhigh;  // maximum x coordinate
    qreal ylow;  // minimum y coordinate
    qreal yhigh;  // maximum coordinate

    qreal a;  // y span of line segment
    qreal xm;  // x median of line segment
    qreal b;  // x span of line segment
    qreal ym;  // y median of line segment
};
