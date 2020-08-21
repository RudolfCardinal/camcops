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
#include <QColor>
#include <QDebug>
#include <QDateTime>
#include <QRect>
#include <QString>
#include <QStringList>
#include "whisker/whiskerconstants.h"

namespace whiskerapi {

// Define the Whisker client API in C++.
// See http://www.whiskercontrol.com/
//      -> Current versions and manuals
//      -> Whisker help.

// ============================================================================
// Helper functions
// ============================================================================

// Maps a boolean to Whisker's convention of "on" or "off".
QString onVal(bool on);

// ============================================================================
// Helper structs
// ============================================================================

struct Pen {
    // Describes a pen to draw with.

    // Constructor. See member variables below.
    Pen(int width = 1,
        const QColor& colour = whiskerconstants::WHITE,
        whiskerconstants::PenStyle style = whiskerconstants::PenStyle::Solid);

    // Return the Whisker parameters, e.g.
    // "-pencolour 255 0 0 -penwidth 1 -penstyle solid".
    QString whiskerOptionString() const;

    int width;  // width in pixels
    QColor colour;  // pen colour
    whiskerconstants::PenStyle style;  // solid, dashed, etc.
};


struct Brush {
    // Describes a brush to fill with.

    // Constructor. See member variables below.
    Brush(const QColor& colour = whiskerconstants::WHITE,
          const QColor& bg_colour = whiskerconstants::BLACK,
          bool opaque = true,
          whiskerconstants::BrushStyle style = whiskerconstants::BrushStyle::Solid,
          whiskerconstants::BrushHatchStyle hatch_style = whiskerconstants::BrushHatchStyle::Cross);

    // Return the Whisker parameters, e.g. "-brushsolid 0 0 100".
    QString whiskerOptionString() const;

    QColor colour;  // brush colour
    QColor bg_colour;  // background colour for hatched brushes (if opaque)
    bool opaque;  // for hatched brushes: opaque?
    whiskerconstants::BrushStyle style;  // e.g. solid, hollow, hatched
    whiskerconstants::BrushHatchStyle hatch_style;  // for hatched: e.g. horizontal, fdiagonal, cross
};


struct LogOptions {
    // Options to control what's written to a Whisker server disk log.

    bool events = true;  // log events?
    bool key_events = true;  // log keyboard events?
    bool client_client = true;  // log client-to-client communications?
    bool comms = false;  // log communications?
    bool signature = true;  // sign the log?
};


struct DisplayCreationOptions {
    // Options to control how a Whisker dynamic display window is created.

    bool resize = true;  // allow the window to be resized?
    bool directdraw = true;  // use DirectDraw?
    bool debug_touches = false;  // debug touches to the window?
    QRect rectangle;  // window rectangle on the server's screen
};


// ============================================================================
// Display object definition classes
// ============================================================================

class DisplayObject {
    // Base class for graphical objects.

public:

    // Return the Whisker options string, beginning with the type of the
    // display object (as a list).
    virtual QStringList options() const = 0;

    // Default destructor.
    virtual ~DisplayObject() = default;

    // Return the Whisker options string, beginning with the type of the
    // display object (as a single string).
    QString optionString() const;
};


class Arc : public DisplayObject {
    // Draws an arc.

public:
    Arc(const QRect& rect, const QPoint& start, const QPoint& end,
        const Pen& pen = Pen());
    virtual QStringList options() const override;

    QRect rect;  // The arc fits into the rect.
    QPoint start;  // Start of the arc.
    QPoint end;  // End of the arc.
    Pen pen;  // Pen to draw with.
};


class Bezier : public DisplayObject {
    // Draws a Bezier curve.

public:
    Bezier(const QPoint& start, const QPoint& control1, const QPoint& control2,
           const QPoint& end, const Pen& pen = Pen());
    virtual QStringList options() const override;

    QPoint start;  // Start of the curve.
    QPoint control1;  // The control points "pull" the curve.
    QPoint control2;
    QPoint end;  // End of the curve.
    Pen pen;  // Pen to draw with.
};


class Bitmap : public DisplayObject {
    // Displays a bitmap, loaded by the server via a filename.

public:
    Bitmap(const QPoint& pos, const QString& filename,
           bool stretch = false, int height = -1, int width = -1,
           whiskerconstants::VerticalAlign valign = whiskerconstants::VerticalAlign::Top,
           whiskerconstants::HorizontalAlign halign = whiskerconstants::HorizontalAlign::Left);
    virtual QStringList options() const override;

    QPoint pos;  // Coordinate of the bitmap (meaning depends on valign/halign).
    QString filename;  // Filename
    bool stretch = false;  // Stretch, rather than clip?
    int height = -1;  // Height (or -1 for the bitmap's own)
    int width = -1;  // Width (or -1 for the bitmap's own)
    whiskerconstants::VerticalAlign valign = whiskerconstants::VerticalAlign::Top;  // vertical alignment
    whiskerconstants::HorizontalAlign halign = whiskerconstants::HorizontalAlign::Left;  // horizontal alignment
};


class CamcogQuadPattern : public DisplayObject {
    // Displays a CamcogQuadPattern, being a 2x2 grid (four quadrants), each of
    // 8x8 grids of chunky pixels in a quadrant colour.

public:
    CamcogQuadPattern(const QPoint& pos, const QSize& pixel_size,
                      const QVector<uint8_t>& top_left_patterns,
                      const QVector<uint8_t>& top_right_patterns,
                      const QVector<uint8_t>& bottom_left_patterns,
                      const QVector<uint8_t>& bottom_right_patterns,
                      const QColor& top_left_colour,
                      const QColor& top_right_colour,
                      const QColor& bottom_left_colour,
                      const QColor& bottom_right_colour,
                      const QColor& bg_colour);
    virtual QStringList options() const override;

    // See Whisker docs.
    QPoint pos;
    QSize pixel_size;
    QVector<uint8_t> top_left_patterns;
    QVector<uint8_t> top_right_patterns;
    QVector<uint8_t> bottom_left_patterns;
    QVector<uint8_t> bottom_right_patterns;
    QColor top_left_colour;
    QColor top_right_colour;
    QColor bottom_left_colour;
    QColor bottom_right_colour;
    QColor bg_colour;
};


class Chord : public DisplayObject {
    // Displays a chord (an ellipse with a bit sliced off).

public:
    Chord(const QRect& rect, const QPoint& line_start, const QPoint& line_end,
          const Pen& pen = Pen(), const Brush& brush = Brush());
    virtual QStringList options() const override;

    // The chord is the intersection of an ellipse (defined by the rect)
    // and a line that intersects it.
    QRect rect;
    QPoint line_start;
    QPoint line_end;
    Pen pen;
    Brush brush;
};


class Ellipse : public DisplayObject {
    // Displays an ellipse.

public:
    Ellipse(const QRect& rect,
            const Pen& pen = Pen(), const Brush& brush = Brush());
    virtual QStringList options() const override;

    // The ellipse fits into the rectangle (and its centre is at the centre
    // of the rectangle).
    QRect rect;
    Pen pen;
    Brush brush;
};


class Line : public DisplayObject {
    // Displays a line.

public:
    Line(const QPoint& start, const QPoint& end, const Pen& pen = Pen());
    virtual QStringList options() const override;

    QPoint start;
    QPoint end;
    Pen pen;
};


class Pie : public DisplayObject {
    // Displays a pie slice (a "solid" arc).

public:
    Pie(const QRect& rect, const QPoint& arc_start, const QPoint& arc_end,
        const Pen& pen = Pen(), const Brush& brush = Brush());
    virtual QStringList options() const override;

    // See Whisker docs.
    QRect rect;
    QPoint arc_start;
    QPoint arc_end;
    Pen pen;
    Brush brush;
};


class Polygon : public DisplayObject {
    // Displays a polygon.

public:
    Polygon(const QVector<QPoint>& points,
            const Pen& pen = Pen(), const Brush& brush = Brush(),
            bool alternate = false);
    virtual QStringList options() const override;

    // See Whisker docs.
    QVector<QPoint> points;
    Pen pen;
    Brush brush;
    bool alternate = false;
};


class Rectangle : public DisplayObject {
    // Displays a rectangle.

public:
    Rectangle(const QRect& rect,
              const Pen& pen = Pen(), const Brush& brush = Brush());
    virtual QStringList options() const override;

    // See Whisker docs.
    QRect rect;
    Pen pen;
    Brush brush;
};


class RoundRect : public DisplayObject {
    // Displays a rectangle with rounded corners.

public:
    RoundRect(const QRect& rect, const QSize& ellipse_size,
              const Pen& pen = Pen(), const Brush& brush = Brush());
    virtual QStringList options() const override;

    // See Whisker docs.
    QRect rect;
    QSize ellipse_size;
    Pen pen;
    Brush brush;
};


class Text : public DisplayObject {
    // Displays text.

public:
    Text(const QPoint& pos, const QString& text, int height = 0,
         const QString& font = "");
    virtual QStringList options() const override;

    QPoint pos;  // Coordinates of the text (meaning depends on valign/halign).
    QString text;  // Text to display
    int height = 0;  // Height in pixels; 0 means some vaguely sensible default
    QString font;  // Font name; empty gives a system font
    bool italic = false;
    bool underline = false;
    int weight = 0;  // Weight ("bold"); 0 for default weight
    QColor colour = whiskerconstants::WHITE;  // Text colour
    bool opaque = false;  // Opaque, not transparent?
    QColor bg_colour = whiskerconstants::BLACK;  // Background colour (if opaque)
    whiskerconstants::TextVerticalAlign valign = whiskerconstants::TextVerticalAlign::Top;  // vertical alignment
    whiskerconstants::TextHorizontalAlign halign = whiskerconstants::TextHorizontalAlign::Left;  // horizontal alignment
};


class Video : public DisplayObject {
    // Displays a video.

public:
    Video(const QPoint& pos, const QString& filename, bool loop = false,
          whiskerconstants::VideoPlayMode playmode = whiskerconstants::VideoPlayMode::Wait,
          int width = -1, int height = -1);
    virtual QStringList options() const override;

    // See Whisker docs.
    QPoint pos;  // Coordinates of the video (meaning depends on valign/halign).
    QString filename;  // Filename, as seen by the Whisker server.
    bool loop = false;  // Loop once it's finished?
    whiskerconstants::VideoPlayMode playmode = whiskerconstants::VideoPlayMode::Wait;  // How to start?
    int width = -1;  // Width in pixels (-1 for the video's own)
    int height = -1;  // Height in pixels (-1 for the video's own)
    bool play_audio = true;  // Play any audio track?
    whiskerconstants::VerticalAlign valign = whiskerconstants::VerticalAlign::Top;  // vertical alignment
    whiskerconstants::HorizontalAlign halign = whiskerconstants::HorizontalAlign::Left;  // horizontal alignment
    QColor bg_colour = whiskerconstants::BLACK;  // background colour, e.g. before playback (see docs)
};


// ============================================================================
// Helper functions
// ============================================================================

// Converts Whisker's "on"/"off" strings to a bool.
bool onOffToBoolean(const QString& msg);

// Returns a quoted version of a string. Not optimal!
QString quote(const QString& s);

// Converts a list of arguments into a space-separated string, ignoring empty
// arguments.
QString msgFromArgs(const QStringList& args);

// Converts a Qt colour into a "red green blue" string like "255 0 0" for red.
QString rgbFromColour(const QColor& colour);

// Converts a QPoint into an "x y" string.
QString pointCoordinates(const QPoint& point);

// Converts a QRect into a "left top right bottom" string.
QString rectCoordinates(const QRect& rect);

// Converts a QSize into a "width height" string.
QString sizeCoordinates(const QSize& size);


}  // namespace whiskerapi
