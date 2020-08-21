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

#include "whiskerapi.h"
#include <QDebug>
#include "lib/convert.h"
#include "whisker/whiskerconstants.h"

using namespace whiskerconstants;

namespace whiskerapi {

// ============================================================================
// Helper functions
// ============================================================================

QString onVal(bool on)
{
    return on ? VAL_ON : VAL_OFF;
}


// ============================================================================
// Helper structs
// ============================================================================

// ----------------------------------------------------------------------------
// Pen
// ----------------------------------------------------------------------------

Pen::Pen(int width, const QColor& colour, PenStyle style) :
    width(width),
    colour(colour),
    style(style)
{
}


QString Pen::whiskerOptionString() const
{
    const QStringList args{
        FLAG_PEN_COLOUR, rgbFromColour(colour),
        FLAG_PEN_WIDTH, QString::number(width),
        FLAG_PEN_STYLE, PEN_STYLE_FLAGS[style],
    };
    return msgFromArgs(args);
}


// ----------------------------------------------------------------------------
// Brush
// ----------------------------------------------------------------------------

Brush::Brush(const QColor& colour, const QColor& bg_colour, bool opaque,
             BrushStyle style, BrushHatchStyle hatch_style) :
    colour(colour),
    bg_colour(bg_colour),
    opaque(opaque),
    style(style),
    hatch_style(hatch_style)
{
}


QString Brush::whiskerOptionString() const
{
    QStringList args{BRUSH_STYLE_FLAGS[style]};
    if (style == BrushStyle::Solid) {
        args.append(rgbFromColour(colour));
    } else if (style == BrushStyle::Hatched) {
        args.append(BRUSH_HATCH_VALUES[hatch_style]);
        args.append(rgbFromColour(colour));
        if (opaque) {
            args.append(FLAG_BRUSH_OPAQUE);
            args.append(FLAG_BRUSH_BACKGROUND);
            args.append(rgbFromColour(bg_colour));
        } else {
            args.append(FLAG_BRUSH_TRANSPARENT);
        }
    }
    return msgFromArgs(args);
}


// ============================================================================
// Display object definition classes
// ============================================================================

QString DisplayObject::optionString() const
{
    return msgFromArgs(options());
}


Arc::Arc(const QRect& rect, const QPoint& start, const QPoint& end,
         const Pen& pen) :
    rect(rect),
    start(start),
    end(end),
    pen(pen)
{
}


QStringList Arc::options() const
{
    return QStringList{
        VAL_OBJTYPE_ARC,
        rectCoordinates(rect),
        pointCoordinates(start),
        pointCoordinates(end),
        pen.whiskerOptionString(),
    };
}


Bezier::Bezier(const QPoint& start, const QPoint& control1,
               const QPoint& control2, const QPoint& end, const Pen& pen) :
    start(start),
    control1(control1),
    control2(control2),
    end(end),
    pen(pen)
{
}


QStringList Bezier::options() const
{
    return QStringList{
        VAL_OBJTYPE_BEZIER,
        pointCoordinates(start),
        pointCoordinates(control1),
        pointCoordinates(control2),
        pointCoordinates(end),
        pen.whiskerOptionString(),
    };
}


Bitmap::Bitmap(const QPoint& pos, const QString& filename,
               bool stretch, int height, int width,
               VerticalAlign valign, HorizontalAlign halign) :
    pos(pos),
    filename(filename),
    stretch(stretch),
    height(height),
    width(width),
    valign(valign),
    halign(halign)
{
}


QStringList Bitmap::options() const
{
    return QStringList{
        VAL_OBJTYPE_BITMAP,
        pointCoordinates(pos),
        quote(filename),
        stretch ? FLAG_BITMAP_STRETCH : FLAG_BITMAP_CLIP,
        FLAG_HEIGHT, QString::number(height),
        FLAG_WIDTH, QString::number(width),
        HALIGN_FLAGS[halign],
        VALIGN_FLAGS[valign],
    };
}


CamcogQuadPattern::CamcogQuadPattern(const QPoint& pos,
                                     const QSize& pixel_size,
                                     const QVector<uint8_t>& top_left_patterns,
                                     const QVector<uint8_t>& top_right_patterns,
                                     const QVector<uint8_t>& bottom_left_patterns,
                                     const QVector<uint8_t>& bottom_right_patterns,
                                     const QColor& top_left_colour,
                                     const QColor& top_right_colour,
                                     const QColor& bottom_left_colour,
                                     const QColor& bottom_right_colour,
                                     const QColor& bg_colour) :
    pos(pos),
    pixel_size(pixel_size),
    top_left_patterns(top_left_patterns),
    top_right_patterns(top_right_patterns),
    bottom_left_patterns(bottom_left_patterns),
    bottom_right_patterns(bottom_right_patterns),
    top_left_colour(top_left_colour),
    top_right_colour(top_right_colour),
    bottom_left_colour(bottom_left_colour),
    bottom_right_colour(bottom_right_colour),
    bg_colour(bg_colour)
{
}


QStringList CamcogQuadPattern::options() const
{
    const int required_size = 8;
    if (top_left_patterns.size() != required_size ||
            top_right_patterns.size() != required_size ||
            bottom_left_patterns.size() != required_size ||
            bottom_right_patterns.size() != required_size) {
        qWarning() << "Whisker CamcogQuadPattern used with wrong vector size; will fail";
        return QStringList();
    }

    auto vectorPattern = [](const QVector<uint8_t>& v) -> QString {
        QStringList numbers;
        for (const uint8_t n : v) {
            numbers.append(QString::number(n));
        }
        return numbers.join(SPACE);
    };

    return QStringList{
        VAL_OBJTYPE_CAMCOGQUADPATTERN,
        pointCoordinates(pos),
        sizeCoordinates(pixel_size),
        vectorPattern(top_left_patterns),
        vectorPattern(top_right_patterns),
        vectorPattern(bottom_left_patterns),
        vectorPattern(bottom_right_patterns),
        rgbFromColour(top_left_colour),
        rgbFromColour(top_right_colour),
        rgbFromColour(bottom_left_colour),
        rgbFromColour(bottom_right_colour),
        rgbFromColour(bg_colour),
    };
}


Chord::Chord(const QRect& rect, const QPoint& line_start,
             const QPoint& line_end, const Pen& pen, const Brush& brush) :
    rect(rect),
    line_start(line_start),
    line_end(line_end),
    pen(pen),
    brush(brush)
{
}


QStringList Chord::options() const
{
    return QStringList{
        VAL_OBJTYPE_CHORD,
        rectCoordinates(rect),
        pointCoordinates(line_start),
        pointCoordinates(line_end),
        pen.whiskerOptionString(),
        brush.whiskerOptionString(),
    };
}


Ellipse::Ellipse(const QRect& rect, const Pen& pen, const Brush& brush) :
    rect(rect),
    pen(pen),
    brush(brush)
{
}


QStringList Ellipse::options() const
{
    return QStringList{
        VAL_OBJTYPE_ELLIPSE,
        rectCoordinates(rect),
        pen.whiskerOptionString(),
        brush.whiskerOptionString(),
    };
}


Line::Line(const QPoint& start, const QPoint& end, const Pen& pen) :
    start(start),
    end(end),
    pen(pen)
{
}


QStringList Line::options() const
{
    return QStringList{
        VAL_OBJTYPE_LINE,
        pointCoordinates(start),
        pointCoordinates(end),
        pen.whiskerOptionString(),
    };
}


Pie::Pie(const QRect& rect, const QPoint& arc_start, const QPoint& arc_end,
         const Pen& pen, const Brush& brush) :
    rect(rect),
    arc_start(arc_start),
    arc_end(arc_end),
    pen(pen),
    brush(brush)
{
}


QStringList Pie::options() const
{
    return QStringList{
        VAL_OBJTYPE_PIE,
        rectCoordinates(rect),
        pointCoordinates(arc_start),
        pointCoordinates(arc_end),
        pen.whiskerOptionString(),
        brush.whiskerOptionString(),
    };
}


Polygon::Polygon(const QVector<QPoint>& points,
                 const Pen& pen, const Brush& brush, bool alternate) :
    points(points),
    pen(pen),
    brush(brush),
    alternate(alternate)
{
}


QStringList Polygon::options() const
{
    if (points.size() < 3) {
        qWarning() << "Whisker polygon used with fewer than 3 points; will fail";
        return QStringList();
    }
    QStringList args{
        VAL_OBJTYPE_POLYGON,
        QString::number(points.size()),
    };
    for (const QPoint& point : points) {
        args.append(pointCoordinates(point));
    }
    args.append({
        alternate ? FLAG_POLYGON_ALTERNATE : FLAG_POLYGON_WINDING,
        pen.whiskerOptionString(),
        brush.whiskerOptionString(),
    });
    return args;
}


Rectangle::Rectangle(const QRect& rect, const Pen& pen, const Brush& brush) :
    rect(rect),
    pen(pen),
    brush(brush)
{
}


QStringList Rectangle::options() const
{
    return QStringList{
        VAL_OBJTYPE_RECTANGLE,
        rectCoordinates(rect),
        pen.whiskerOptionString(),
        brush.whiskerOptionString(),
    };
}


RoundRect::RoundRect(const QRect& rect, const QSize& ellipse_size,
                     const Pen& pen, const Brush& brush) :
    rect(rect),
    ellipse_size(ellipse_size),
    pen(pen),
    brush(brush)
{
}


QStringList RoundRect::options() const
{
    return QStringList{
        VAL_OBJTYPE_ROUNDRECT,
        rectCoordinates(rect),
        sizeCoordinates(ellipse_size),
        pen.whiskerOptionString(),
        brush.whiskerOptionString(),
    };
}


Text::Text(const QPoint& pos, const QString& text, int height,
           const QString& font) :
    pos(pos),
    text(text),
    height(height),
    font(font)
{
}


QStringList Text::options() const
{
    QStringList args{
        VAL_OBJTYPE_TEXT,
        pointCoordinates(pos),
        quote(text),
        FLAG_HEIGHT, QString::number(height),
        FLAG_TEXT_WEIGHT, QString::number(weight),
        italic ? FLAG_TEXT_ITALIC : "",
        underline ? FLAG_TEXT_UNDERLINE : "",
        opaque ? FLAG_TEXT_OPAQUE : "",
        FLAG_TEXT_COLOUR, rgbFromColour(colour),
        FLAG_BACKCOLOUR, rgbFromColour(bg_colour),
        TEXT_HALIGN_FLAGS[halign],
        TEXT_VALIGN_FLAGS[valign],
    };
    if (!font.isEmpty()) {
        args.append({FLAG_FONT, quote(font)});
    }
    return args;
}


Video::Video(const QPoint& pos, const QString& filename, bool loop,
             VideoPlayMode playmode, int width, int height) :
    pos(pos),
    filename(filename),
    loop(loop),
    playmode(playmode),
    width(width),
    height(height)
{
}



QStringList Video::options() const
{
    return QStringList{
        VAL_OBJTYPE_VIDEO,
        pointCoordinates(pos),
        quote(filename),
        loop ? FLAG_LOOP : FLAG_VIDEO_NOLOOP,
        VIDEO_PLAYMODE_FLAGS[playmode],
        FLAG_WIDTH, QString::number(width),
        FLAG_HEIGHT, QString::number(height),
        play_audio ? FLAG_VIDEO_AUDIO : FLAG_VIDEO_NOAUDIO,
        HALIGN_FLAGS[halign],
        VALIGN_FLAGS[valign],
        FLAG_BACKCOLOUR, rgbFromColour(bg_colour),
    };
}


// ============================================================================
// Helper functions
// ============================================================================

bool onOffToBoolean(const QString& msg)
{
    return msg == VAL_ON;
}


QString quote(const QString& s)
{
    return QUOTE + s + QUOTE;  // suboptimal! Doesn't escape quotes.
    // Mind you, don't think Whisker deals with that anyway.
}


QString msgFromArgs(const QStringList& args)
{
    QStringList nonempty_args;
    for (const QString& arg : args) {
        if (!arg.isEmpty()) {
            nonempty_args.append(arg);
        }
    }
    return nonempty_args.join(SPACE);
}


QString rgbFromColour(const QColor& colour)
{
    return QString("%1 %2 %3").arg(
                QString::number(colour.red()),
                QString::number(colour.green()),
                QString::number(colour.blue()));
}


QString pointCoordinates(const QPoint& point)
{
    return QString("%1 %2").arg(
                QString::number(point.x()),
                QString::number(point.y()));
}


QString rectCoordinates(const QRect& rect)
{
    return QString("%1 %2 %3 %4").arg(
        QString::number(rect.left()),
        QString::number(rect.top()),
        QString::number(rect.right()),
        QString::number(rect.bottom())
    );
}


QString sizeCoordinates(const QSize& size)
{
    return QString("%1 %2").arg(
                QString::number(size.width()),
                QString::number(size.height()));
}



}  // namespace whiskerapi

