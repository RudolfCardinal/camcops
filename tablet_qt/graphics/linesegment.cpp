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

// #define DEBUG_TRANSFORM

#include "linesegment.h"
#include <QDebug>
#include <QTransform>
#include <QtMath>
#include "maths/mathfunc.h"
using mathfunc::sgn;
using mathfunc::rangesOverlap;


LineSegment::LineSegment(const QPointF& from, const QPointF& to) :
    from(from),
    to(to)
{
    // http://stackoverflow.com/questions/385305/efficient-maths-algorithm-to-calculate-intersections
    const qreal x0 = from.x();
    const qreal x1 = to.x();
    const qreal y0 = from.y();
    const qreal y1 = to.y();

    // Stash normalized coordinates:
    xlow = x0;
    xhigh = x1;
    if (x0 > x1) {
        std::swap(xlow, xhigh);
    }
    ylow = y0;
    yhigh = y1;
    if (y0 > y1) {
        std::swap(ylow, yhigh);
    }

    // Stash equation parameters:
    xm = (x0 + x1) / 2;
    ym = (y0 + y1) / 2;
    a = y1 - y0;
    b = x0 - x1;
}


qreal LineSegment::c(const qreal x, const qreal y) const
{
    // The line has equation a * (x - xm) + b * (y - ym) = c = 0
    return a * (x - xm) + b * (y - ym);
}


qreal LineSegment::c(const QPointF& pt) const
{
    return c(pt.x(), pt.y());
}


int LineSegment::side(const QPointF& pt) const
{
    return sgn(c(pt));
}


bool LineSegment::isPoint() const
{
    return from == to;
}


bool LineSegment::xRangesOverlap(const LineSegment& other) const
{
    return rangesOverlap(xlow, xhigh, other.xlow, other.xhigh);
}


bool LineSegment::yRangesOverlap(const LineSegment& other) const
{
    return rangesOverlap(ylow, yhigh, other.ylow, other.yhigh);
}


bool LineSegment::intersects(const LineSegment& other) const
{
    if (isPoint() || other.isPoint()) {
        return false;
    }
    // Don't use QRectF::intersects(); that returns false when using a
    // rectangle without width (even if it has height) or vice versa.
    if (!xRangesOverlap(other) || !yRangesOverlap(other)) {
        return false;
    }
    // http://stackoverflow.com/questions/385305/efficient-maths-algorithm-to-calculate-intersections
    // See also: http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
    if (sgn(c(other.from)) == sgn(c(other.to))) {
        // Both other.from and other.to are on the same side of our
        // line, and therefore there can be no intersection.
        return false;
    }
    if (sgn(other.c(from)) == sgn(other.c(to))) {
        // Both from and to are on the same side of the other
        // line, and therefore there can be no intersection.
        return false;
    }
    // There must be an intersection.
    return true;
}


bool LineSegment::pointOn(const QPointF& point) const
{
    const qreal x = point.x();
    const qreal y = point.y();
    if (x < xlow || x > xhigh || y < ylow || y > yhigh) {
        return false;
    }
    return qFuzzyIsNull(c(point));
}


qreal LineSegment::angleRad() const
{
    const qreal dx = to.x() - from.x();
    const qreal dy = to.y() - from.y();
    return qAtan2(dy, dx);
}


QRectF LineSegment::rect() const
{
    const QRectF r(from, to);
    return r.normalized();
}


bool LineSegment::pointInPerpendicularArea(const QPointF& point) const
{
    const qreal angle = angleRad();
    QTransform tr;
    tr = tr.rotateRadians(angle);  // rotate ANTICLOCKWISE by angle
    // The direction should now be in the positive x direction.
    const QRectF rotated_rect = tr.mapRect(rect());
    const QPointF rotated_point = tr.map(point);
#ifdef DEBUG_TRANSFORM
    qDebug() << "pointInPerpendicularArea:"
             << "point" << point
             << "angle" << angle
             << "in degrees" << qRadiansToDegrees(angle)
             << "rotated_rect" << rotated_rect
             << "rotated_point" << rotated_point;
#endif
    const qreal x = rotated_point.x();
    return x >= rotated_rect.left() && x <= rotated_rect.right();
}
