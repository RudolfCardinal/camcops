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
#include <QWidget>  // for QWIDGETSIZE_MAX
#include "graphics/linesegment.h"


namespace geometry
{

/*

Standard Cartesian/polar coordinate systems
===============================================================================

- Positive x is to the right.
- Positive y is UP.
- Positive theta is ANTICLOCKWISE, and theta = 0 is at the 3 o'clock position.
  ... so for a point (x=1, y=0), positive rotation moves it in the direction of
  INCREASING y.
  When you rotate by theta, you rotate anticlockwise.
  https://en.wikipedia.org/wiki/Rotation_of_axes

The Qt coordinate system
===============================================================================

- Positive x is to the right.
- Positive y is DOWN. (This matches commonplace screen coordinates; the origin
  is at the top left.)
  http://doc.qt.io/qt-5.8/coordsys.html

- When you rotate a coordinate system, rotation angles are CLOCKWISE.
  http://doc.qt.io/qt-5.8/qpainter.html#rotate
  ... so for a point (x=1, y=0), positive rotation moves it in the direction of
  INCREASING y.

- But when you draw a pie, rotation angles are ANTICLOCKWISE, and zero degrees
  is in the 3 o'clock position.
  http://doc.qt.io/qt-5.8/qpainter.html#drawPie

- Other ANTICLOCKWISE bits:
  - QTranform::rotate
    http://doc.qt.io/qt-5.8/qtransform.html#rotate

- Qt also uses a "positive ANTICLOCKWISE" system for its graphs, though that's
  more obvious as it's mimicking standard graph geometry here.
  https://doc.qt.io/qt-5/qtcharts-polarchart-example.html

Which representation to use internally for polar coordinates?
===============================================================================

- Any sophisticated representations are going to assume a standard Cartesian
  system and the most important part of that isn't "up"/"down" but the fact
  that positive angles are anticlockwise WITH RESPECT TO "x right, y up", i.e.
  that positive rotation moves the point (x=1, y=0) in the direction of
  INCREASING y.

  That's helpful so we can use standard representations like
        x = r * cos(theta)      y = r * sin(theta)
  not
        x = r * cos(theta)      y = -r * sin(theta)

- That means angles are clockwise in the standard Qt coordinates.

- So we'll use that when we refer to "polar", and convert explicitly for those
  places (like pie drawing) where anticlockwise angles are required.

Compass headings
===============================================================================

- These are based on the idea of "North up" (though also support a
  transformation via an "alternative North"), and positive rotation is
  CLOCKWISE.

Other notes on Qt coordinates
===============================================================================

- QPainter::drawText()

  "The y-position is used as the baseline of the font."

   0123456789
  0   |
  1   SSOOMMEE  TTEEXXTT
  2   SSOOMMEE  TTEEXXTT
  3 - SSOOMMEE  TTEEXXTT -      [descenders go below line?]
  4   |

  So if you draw at y = 3, it'll be bottom-aligned there.
  To top-align it, add its height to the y coordinate.
  To vcentre-align it, add half its height to the y coordinate.

  To left-align it, plot at the unmodified x coordinate.
  To centre-align it, subtract half its width from the x coordinate.
  To right-align it, subtract its width from the x coordinate.

*/

extern const qreal DEG_0;
extern const qreal DEG_90;
extern const qreal DEG_270;
extern const qreal DEG_180;
extern const qreal DEG_360;

// Converts degrees to sixteenths of a degree.
int sixteenthsOfADegree(qreal degrees);

// Converts clockwise to anticlockwise degrees (!).
inline qreal clockwiseToAnticlockwise(qreal degrees) { return -degrees; }

// Converts anticlockwise to clockwise degrees (!).
inline qreal anticlockwiseToClockwise(qreal degrees) { return -degrees; }

// Returns a heading normalized to [0, 360).
qreal normalizeHeading(qreal heading_deg);

// Are the two headings fuzzy-equal?
bool headingNearlyEq(qreal heading_deg, qreal value_deg);

// Is heading_deg in the range (first_bound_deg, second_bound_deg)?
// Or, if inclusive is true, in [first_bound_deg, second_bound_deg]?
bool headingInRange(qreal first_bound_deg,
                    qreal heading_deg,
                    qreal second_bound_deg,
                    bool inclusive = false);

// Converts a compass heading from a "true" to a "pseudo" system, based on
// pseudo_north_deg.
qreal convertHeadingFromTrueNorth(qreal true_north_heading_deg,
                                  qreal pseudo_north_deg,
                                  bool normalize = true);

// Converts a compass heading from a "pseudo" to a "true" system, based on
// pseudo_north_deg.
qreal convertHeadingToTrueNorth(qreal pseudo_north_heading_deg,
                                qreal pseudo_north_deg,
                                bool normalize = true);

// Returns a point (relative to the origin) equivalent to the specified
// polar coordinates.
QPointF polarToCartesian(qreal r, qreal theta_degrees);

// Returns the distance between two points.
qreal distanceBetween(const QPointF& from, const QPointF& to);

// Returns the heading (in polar degrees, 0 = along x axis), "from" -> "to".
qreal polarThetaDeg(const QPointF& from, const QPointF& to);

// Returns the heading (in polar degrees, 0 = along x axis), origin -> "to".
qreal polarThetaDeg(const QPointF& to);

// Converts a polar angle to a compass heading.
qreal polarThetaToHeading(qreal theta_deg, qreal north_deg = 0.0);

// Converts a compass heading to a polar angle.
qreal headingToPolarThetaDeg(qreal heading_deg, qreal north_deg = 0.0,
                             bool normalize = true);

// Returns a compass heading, "from" -> "to".
qreal headingDegrees(const QPointF& from, const QPointF& to,
                     qreal north_deg = 0.0);

// Return a line segment starting at "point", travelling in compass direction
// "heading" (where north_deg indicates our North direction), with line length
// "radius".
LineSegment lineFromPointInHeadingWithRadius(const QPointF& point,
                                             qreal heading_deg,
                                             qreal north_deg = 0.0,
                                             qreal radius = QWIDGETSIZE_MAX);

// (1) Draw a line from "from" to "to".
// (2) Draw a line from "point" in direction "heading", where increasing
//     values of "heading" are clockwise, and a heading of 0 degrees is
//     the North direction (where that is defined by north_deg degrees
//     clockwise of "screen up").
// (3) Do the two lines cross?
bool lineCrossesHeadingWithinRadius(const QPointF& from, const QPointF& to,
                                    const QPointF& point, qreal heading_deg,
                                    qreal north_deg = 0.0,
                                    qreal radius = QWIDGETSIZE_MAX);

// Does the line "from" -> "to" pass below "point"?
bool linePassesBelowPoint(const QPointF& from, const QPointF& to,
                          const QPointF& point);

}  // namespace geometry
