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
#include <QFont>
#include <QMap>
#include <QObject>
#include <QPen>
#include "common/colourdefs.h"
#include "graphics/buttonconfig.h"
#include "graphics/textconfig.h"
class AdjustablePie;
class SvgWidgetClickable;
class QBrush;
class QGraphicsPixmapItem;
class QGraphicsProxyWidget;
class QGraphicsScene;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QLabel;
class QPaintEvent;
class QPushButton;
class QRectF;
class QString;
class QSvgRenderer;
class QWidget;


namespace graphicsfunc
{

// ============================================================================
// Constants
// ============================================================================

extern const QString TEST_SVG;

// ============================================================================
// Support structures
// ============================================================================
// These associate QWidget-derived objects and their QGraphicsProxyWidget.

struct ButtonAndProxy {
    // Ownership of QGraphicsProxyWidget/QWidget pairs is shared, i.e. if
    // either is destroyed, the other is automatically destroyed.
    // Since the proxy is owned by the scene when added to the scene (which
    // happens as it's created), I think all these things are owned by the
    // scene.
    QPushButton* button = nullptr;
    QGraphicsProxyWidget* proxy = nullptr;
    // No success in implementing this:
    /*
    QMetaObject::Connection connect(
            const QObject* receiver,
            const QMetaMethod& method,
            Qt::ConnectionType type = Qt::AutoConnection);
    */
    // ... get "static assertion failed: Signal and slot arguments are not compatible."
};


struct LabelAndProxy {
    QLabel* label = nullptr;
    QGraphicsProxyWidget* proxy = nullptr;
};


struct AdjustablePieAndProxy {
    AdjustablePie* pie = nullptr;
    QGraphicsProxyWidget* proxy = nullptr;
};


struct SvgWidgetAndProxy {
    SvgWidgetClickable* widget = nullptr;
    QGraphicsProxyWidget* proxy = nullptr;
};


// ============================================================================
// SvgTransform
// ============================================================================

// Represents a combination of SVG transformations.

class SvgTransform {
public:
    SvgTransform();

    // Matrix transformation
    // https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/transform#Matrix
    SvgTransform& matrix(qreal a, qreal b, qreal c, qreal d, qreal e, qreal f);

    // Translation
    SvgTransform& translate(qreal x, qreal y = 0.0);

    // Non-distorting scale
    SvgTransform& scale(qreal xy);

    // Distorting scale (separate x/y values)
    SvgTransform& scale(qreal x, qreal y);

    // Rotation about the origin
    SvgTransform& rotate(qreal a);

    // Rotation about a point
    SvgTransform& rotate(qreal a, qreal x, qreal y);

    // Skew along the X axis
    // https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/transform#SkewX
    SvgTransform& skewX(qreal a);

    // Skew along the Y axis
    // https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/transform#SkewY
    SvgTransform& skewY(qreal a);

    // Returns the string form of the combined transformations
    QString string() const;

    // Are there any transformations?
    bool active() const;

protected:
    QStringList transformations;
};


// ============================================================================
// SVG
// ============================================================================

// Returns an XML element string.
QString xmlElement(const QString& tag, const QString& contents,
                   const QMap<QString, QString>& attributes = QMap<QString, QString>());

// Returns an XML attribute string (with name=value pairs).
QString xmlAttributes(const QMap<QString, QString>& attributes);

// Returns an SVG string, being an XML element containing other elements.
QString svg(const QStringList& elements);

// Returns an SVG path XML element.
QString svgPath(const QString& contents,
                const QColor& stroke, int stroke_width,
                const QColor& fill,
                const SvgTransform& transform,
                const QString& element_id = "");

// Returns an SVG XML element from path details.
QString svgFromPathContents(const QString& path_contents,
                            const QColor& stroke, int stroke_width,
                            const QColor& fill,
                            const SvgTransform& transform,
                            const QString& element_id = "");

// Returns the SVG opacity [0-1] representation of a QColor's alpha (0-255).
QString opacity(const QColor& colour);

// Converts opacity [0-1] to alpha [0-255].
int alpha(qreal opacity);


// ============================================================================
// Graphics calculations and painting
// ============================================================================

// Modifies a rectangle by aligning it with its current top-left point.
// The assumed starting point is that the user wishes to have a rectangle
// aligned at point (x,y), and that (x,y) is currently the top left point
// of rect.
void alignRect(QRectF& rect, Qt::Alignment alignment);

// Returns a rectangle centred on "centre", with width "w" and height "h".
QRectF centredRect(const QPointF& centre, qreal w, qreal h);

// Draws a sector, defined by its tip (the centre of the circle of which it's
// part), radius, and start/end angles.
void drawSector(QPainter& painter,
                const QPointF& tip,
                qreal radius,
                qreal start_angle_deg,  // zero is 3 o'clock
                qreal end_angle_deg,  // zero is 3 o'clock
                bool treat_as_clockwise_angles,
                const QPen& pen,
                const QBrush& brush);

// Returns the bounding rectangle of a piece of text in a certain font.
QRectF textRectF(const QString& text, const QFont& font);

// Draws text aligned with a point ("point").
void drawText(QPainter& painter, const QPointF& point, const QString& text,
              const QFont& font, Qt::Alignment align);

// Draws text aligned with a point ("x", "y"), returning the bounding
// rectangle of the text if bounding_rect is specified.
void drawText(QPainter& painter, qreal x, qreal y, Qt::Alignment flags,
              const QString& text, QRectF* bounding_rect = nullptr);

// Draws text aligned with a point ("point"), returning the bounding
// rectangle of the text if bounding_rect is specified.
void drawText(QPainter& painter, const QPointF& point, Qt::Alignment flags,
              const QString& text, QRectF* bounding_rect = nullptr);

// Paints a pixmap so that it fits within a rectangle, maintaining the aspect
// ratio of the pixmap.
void paintPixmapKeepingAspectRatio(QPainter& painter,
                                   const QPixmap& pixmap,
                                   const QRect& destination,
                                   QPaintEvent* paint_event = nullptr);

#if 0  // currently unused
// Returns a QRegion whose coordinates have been scaled (multiplied) by a
// factor.
QRegion scaleRegion(const QRegion& region, qreal factor);
#endif


// ============================================================================
// Creating QGraphicsScene objects
// ============================================================================

// Makes a text button: a rounded rectangle with word-wrapping text in it.
ButtonAndProxy makeTextButton(
        QGraphicsScene* scene,  // button is added to scene
        const QRectF& rect,
        const ButtonConfig& config,
        const QString& text,
        QFont font = QFont(),
        QWidget* parent = nullptr);

// Makes a text label (word-wrapping if required).
LabelAndProxy makeText(
        QGraphicsScene* scene,  // text is added to scene
        const QPointF& point,
        const TextConfig& config,
        const QString& text,
        QFont font = QFont(),
        QWidget* parent = nullptr);

// Makes an "adjustable pie" widget.
AdjustablePieAndProxy makeAdjustablePie(
        QGraphicsScene* scene,  // pie is added to scene
        const QPointF& centre,
        int n_sectors,
        qreal diameter,
        QWidget* parent = nullptr);

// Makes a clickable SVG image.
SvgWidgetAndProxy makeSvg(
        QGraphicsScene* scene,  // SVG is added to scene
        const QPointF& centre,
        const QString& svg,
        const QColor& pressed_background_colour = QCOLOR_TRANSPARENT,
        const QColor& background_colour = QCOLOR_TRANSPARENT,
        bool transparent_for_mouse = false,
        QWidget* parent = nullptr);

// Makes a translucent rectangle.
QGraphicsRectItem* makeObscuringRect(
        QGraphicsScene* scene,
        const QRectF& rect,
        qreal opacity = 0.5,  // 0-1
        const QColor& colour_ignoring_opacity = QCOLOR_BLACK);

// Makes a graphics object from a disk image.
QGraphicsPixmapItem* makeImage(
        QGraphicsScene* scene,
        const QRectF& rect,
        const QString& filename,
        qreal opacity = 1.0,
        Qt::AspectRatioMode aspect_ratio_mode = Qt::KeepAspectRatio,
        Qt::TransformationMode transformation_mode_1 = Qt::FastTransformation,
        Qt::TransformationMode transformation_mode_2 = Qt::FastTransformation);

}  // namespace graphicsfunc
