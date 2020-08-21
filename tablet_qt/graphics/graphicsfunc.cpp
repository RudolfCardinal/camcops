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

// #define DEBUG_CSS
// #define DEBUG_COORDS
// #define DEBUG_SVG

#include "graphicsfunc.h"
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QLabel>
#include <QMetaMethod>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QRectF>
#include <QSvgRenderer>
#include <QtGlobal>
#include <QVBoxLayout>
#include "graphics/geometry.h"
#include "graphics/graphicspixmapitemwithopacity.h"
#include "lib/css.h"
#include "maths/mathfunc.h"
// #include "qobjects/stylenofocusrect.h"
#include "widgets/adjustablepie.h"
#include "widgets/svgwidgetclickable.h"
using css::colourCss;
using css::labelCss;
using css::penCss;
using css::pixelCss;
using geometry::clockwiseToAnticlockwise;
using geometry::sixteenthsOfADegree;


namespace graphicsfunc
{

// ============================================================================
// Constants
// ============================================================================

const QString TEST_SVG(
"<svg height=\"210\" width=\"210\">"
"    <polygon points=\"100,10 40,198 190,78 10,78 160,198\""
"     style=\"fill:lime;stroke:purple;stroke-width:5;fill-rule:evenodd;\"/>"
"</svg>"
);


// ============================================================================
// SvgTransform
// ============================================================================

SvgTransform::SvgTransform()
{
}


SvgTransform& SvgTransform::matrix(const qreal a, const qreal b, const qreal c,
                                   const qreal d, const qreal e, const qreal f)
{
    transformations.append(QString("matrix(%1 %2 %3 %4 %5 %6")
                           .arg(a)
                           .arg(b)
                           .arg(c)
                           .arg(d)
                           .arg(e)
                           .arg(f));
    return *this;
}


SvgTransform& SvgTransform::translate(const qreal x, const qreal y)
{
    transformations.append(QString("translate(%1 %2)")
                           .arg(x)
                           .arg(y));
    return *this;
}


SvgTransform& SvgTransform::scale(const qreal xy)
{
    transformations.append(QString("scale(%1)").arg(xy));
    return *this;
}


SvgTransform& SvgTransform::scale(const qreal x, const qreal y)
{
    transformations.append(QString("scale(%1 %2)")
                           .arg(x)
                           .arg(y));
    return *this;
}


SvgTransform& SvgTransform::rotate(const qreal a)
{
    transformations.append(QString("rotate(%1)").arg(a));
    return *this;
}


SvgTransform& SvgTransform::rotate(const qreal a, const qreal x, const qreal y)
{
    transformations.append(QString("rotate(%1 %2 %e)")
                           .arg(a)
                           .arg(x)
                           .arg(y));
    return *this;
}


SvgTransform& SvgTransform::skewX(const qreal a)
{
    transformations.append(QString("skewX(%1)").arg(a));
    return *this;
}


SvgTransform& SvgTransform::skewY(const qreal a)
{
    transformations.append(QString("skewY(%1)").arg(a));
    return *this;
}


QString SvgTransform::string() const
{
    return transformations.join(" ");
}


bool SvgTransform::active() const
{
    return !transformations.isEmpty();
}


// ============================================================================
// SVG
// ============================================================================

QString xmlElement(const QString& tag, const QString& contents,
                   const QMap<QString, QString>& attributes)
{
    const QString attr = xmlAttributes(attributes);
    if (contents.isEmpty()) {
        return QString("<%1%2 />").arg(tag, attr);
    }
    return QString("<%1%2>%3</%4>").arg(tag, attr, contents, tag);
}


QString xmlAttributes(const QMap<QString, QString>& attributes)
{
    if (attributes.isEmpty()) {
        return "";
    }
    QStringList attrlist;
    QMapIterator<QString, QString> i(attributes);
    while (i.hasNext()) {
        i.next();
        attrlist.append(QString("%1=\"%2\"").arg(i.key(),
                                                 i.value().toHtmlEscaped()));
    }
    return " " + attrlist.join(" ");
}


QString svg(const QStringList& elements)
{
    // https://www.w3schools.com/graphics/svg_intro.asp
    return xmlElement("svg", elements.join(""));
}


QString svgPath(const QString& contents,
                const QColor& stroke, const int stroke_width,
                const QColor& fill,
                const SvgTransform& transform,
                const QString& element_id)
{
    // https://www.w3schools.com/graphics/svg_path.asp
    // https://www.w3.org/TR/SVG/paths.html#PathElement
    // https://stackoverflow.com/questions/6042550/svg-fill-color-transparency-alpha
    QMap<QString, QString> attributes{
        {"d", contents},
        {"stroke", stroke.name(QColor::HexRgb)},
        {"stroke-width", QString::number(stroke_width)},
        {"stroke-opacity", opacity(stroke)},
        {"fill", fill.name(QColor::HexRgb)},
        {"fill-opacity", opacity(fill)},
    };
    if (!element_id.isEmpty()) {
        attributes["id"] = element_id;
    }
    if (transform.active()) {
        attributes["transform"] = transform.string();
    }
    return xmlElement("path", "", attributes);
}


QString svgFromPathContents(const QString& path_contents,
                            const QColor& stroke, int stroke_width,
                            const QColor& fill,
                            const SvgTransform& transform,
                            const QString& element_id)
{
#ifdef DEBUG_SVG
    Q_UNUSED(path_contents)
    Q_UNUSED(stroke)
    Q_UNUSED(stroke_width)
    Q_UNUSED(fill)
    Q_UNUSED(element_id)
    return TEST_SVG;
#else
    return svg({
        svgPath(path_contents, stroke, stroke_width, fill, transform,
                   element_id)
    });
#endif
}


QString opacity(const QColor& colour)
{
    return QString::number(mathfunc::byteToProportion(colour.alpha()));
}


int alpha(qreal opacity)
{
    return mathfunc::proportionToByte(opacity);
}


// ============================================================================
// Graphics calculations and painting
// ============================================================================

void alignRect(QRectF& rect, const Qt::Alignment alignment)
{
    // Horizontal
    qreal dx = 0;
    if (alignment & Qt::AlignLeft ||
            alignment & Qt::AlignJustify ||
            alignment & Qt::AlignAbsolute) {
        dx = 0;
    } else if (alignment & Qt::AlignHCenter) {
        dx = -rect.width() / 2;
    } else if (alignment & Qt::AlignRight) {
        dx = -rect.width();
    } else {
        qWarning() << Q_FUNC_INFO << "Unknown horizontal alignment";
    }

    // Vertical
    qreal dy = 0;
    if (alignment & Qt::AlignTop) {
        dy = 0;
    } else if (alignment & Qt::AlignVCenter) {
        dy = -rect.height() / 2;
    } else if (alignment & Qt::AlignBottom ||
               alignment & Qt::AlignBaseline) {
        dy = -rect.height();
    } else {
        qWarning() << Q_FUNC_INFO << "Unknown horizontal alignment";
    }

    rect.translate(dx, dy);
}


QRectF centredRect(const QPointF& centre, const qreal w, const qreal h)
{
    return QRectF(centre.x() - w / 2.0, centre.y() - h / 2.0, w, h);
}


void drawSector(QPainter& painter,
                const QPointF& tip,
                const qreal radius,
                qreal start_angle_deg,
                qreal end_angle_deg,
                const bool move_clockwise_from_start_to_end,
                const QPen& pen,
                const QBrush& brush)
{
#ifdef DEBUG_COORDS
    qDebug() << "drawSector:"
             << "tip" << tip
             << "radius" << radius
             << "start_angle_deg (polar)" << start_angle_deg
             << "end_angle_deg (polar)" << end_angle_deg
             << "move_clockwise_from_start_to_end" << move_clockwise_from_start_to_end;
#endif
    painter.setPen(pen);
    painter.setBrush(brush);
    const qreal diameter = radius * 2;
    const QRectF rect(tip - QPointF(radius, radius), QSizeF(diameter, diameter));
    if (!move_clockwise_from_start_to_end) {
        std::swap(start_angle_deg, end_angle_deg);
    }
    start_angle_deg = clockwiseToAnticlockwise(start_angle_deg);
    end_angle_deg = clockwiseToAnticlockwise(end_angle_deg);
    const qreal span_angle_deg = end_angle_deg - start_angle_deg;
#ifdef DEBUG_COORDS
    qDebug() << "... "
             << "tip" << tip
             << "rect" << rect
             << "start_angle_deg (for QPainter::drawPie)" << start_angle_deg
             << "span_angle_deg (for QPainter::drawPie)" << span_angle_deg;
#endif
    painter.drawPie(rect,
                    sixteenthsOfADegree(start_angle_deg),
                    sixteenthsOfADegree(span_angle_deg));
}


QRectF textRectF(const QString& text, const QFont& font)
{
    const QFontMetrics fm(font);
    // return fm.boundingRect(text);
    return fm.tightBoundingRect(text);
}


void drawText(QPainter& painter, const QPointF& point, const QString& text,
              const QFont& font, const Qt::Alignment align)
{
    const QRectF textrect = textRectF(text, font);

    qreal x = point.x();
    if (align & Qt::AlignRight) {
        x -= textrect.width();
    } else if (align & Qt::AlignHCenter) {
        x -= textrect.width() / 2.0;
    }

    qreal y = point.y();
    if (align & Qt::AlignTop) {
        y += textrect.height();
    } else if (align & Qt::AlignVCenter) {
        y += textrect.height() / 2.0;
    }

    painter.setFont(font);
    painter.drawText(static_cast<int>(x), static_cast<int>(y), text);
}


void drawText(QPainter& painter,
              const qreal x, const qreal y, Qt::Alignment flags,
              const QString& text, QRectF* bounding_rect)
{
    // http://stackoverflow.com/questions/24831484
    const qreal size = 32767.0;
    QPointF corner(x, y - size);

    if (flags & Qt::AlignHCenter) {
       corner.rx() -= size / 2.0;
    }
    else if (flags & Qt::AlignRight) {
       corner.rx() -= size;
    }

    if (flags & Qt::AlignVCenter) {
       corner.ry() += size / 2.0;
    }
    else if (flags & Qt::AlignTop) {
       corner.ry() += size;
    }
    else {
       flags |= Qt::AlignBottom;
    }

    const QRectF rect(corner, QSizeF(size, size));
    painter.drawText(rect, flags, text, bounding_rect);
    // http://doc.qt.io/qt-5/qpainter.html#drawText-4
}


void drawText(QPainter& painter, const QPointF& point, Qt::Alignment flags,
              const QString& text, QRectF* bounding_rect)
{
    // http://stackoverflow.com/questions/24831484
    drawText(painter, point.x(), point.y(), flags, text, bounding_rect);
}


void paintPixmapKeepingAspectRatio(QPainter& painter,
                                   const QPixmap& pixmap,
                                   const QRect& destination,
                                   QPaintEvent* paint_event)
{

    if (destination.size() != pixmap.size()) {
        // Scale
        QSize displaysize = pixmap.size();
        displaysize.scale(destination.size(), Qt::KeepAspectRatio);
        const QRect dest_active_rect = QRect(destination.topLeft(), displaysize);
        const QRect source_all_image(QPoint(0, 0), pixmap.size());
#ifdef DEBUG_COORDS
        qDebug().nospace()
                << Q_FUNC_INFO
                << " - Asked to draw to rect of size " << destination.size()
                << "; drawing to size " << displaysize;
#endif
        painter.drawPixmap(dest_active_rect, pixmap, source_all_image);
    } else {
        // No need to scale
        painter.drawPixmap(destination.left(), destination.top(), pixmap);
    }

    Q_UNUSED(paint_event);
    // Optimizations are possible: we don't have to draw all of it...
    // http://blog.qt.io/blog/2006/05/13/fast-transformed-pixmapimage-drawing/
    // ... but I haven't implemented those optimizations.
    // One would read paint_event->rect(), and paint only the relevant part.
    // See also CanvasWidget.
}


#if 0
QRegion scaleRegion(const QRegion& region, qreal factor)
{
    QRegion result;
    for (auto prect : region) {
        QRect scaled_rect(  // left, top, width, height
                    prect.left() * factor,
                    prect.top() * factor,
                    prect.width() * factor,
                    prect.height() * factor
        );
        result += scaled_rect;
    }
    return result;
}
#endif


// ============================================================================
// Creating QGraphicsScene objects
// ============================================================================

// ROUNDED BUTTONS
// Method 1:
// http://stackoverflow.com/questions/17295329/qt-add-a-round-rect-to-a-graphics-item-group

// http://falsinsoft.blogspot.co.uk/2015/11/qt-snippet-rounded-corners-qpushbutton.html
/*
auto pButtonWidget = new QPushButton();
pButtonWidget->setGeometry(QRect(0, 0, 150, 100));
pButtonWidget->setText("Test");
pButtonWidget->setFlat(true);
pButtonWidget->setAttribute(Qt::WA_TranslucentBackground);
pButtonWidget->setStyleSheet(
    "background-color: darkRed;"
    "border: 1px solid black;"
    "border-radius: 15px;"
    "color: lightGray; "
    "font-size: 25px;"
    );
QGraphicsProxyWidget* pButtonProxyWidget = scene()->addWidget(pButtonWidget);
*/

// https://dzone.com/articles/returning-multiple-values-from-functions-in-c


/*
// Doesn't work:
QMetaObject::Connection ButtonAndProxy::connect(const QObject* receiver,
                                                const QMetaMethod& method,
                                                Qt::ConnectionType type)
{
    return QObject::connect(button, &QPushButton::clicked,
                            receiver, method, type);
}
*/


ButtonAndProxy makeTextButton(QGraphicsScene* scene,  // button is added to scene
                              const QRectF& rect,
                              const ButtonConfig& config,
                              const QString& text,
                              QFont font,
                              QWidget* parent)
{
    Q_ASSERT(scene);
    // We want a button that can take word-wrapping text, but not with the more
    // sophisticated width-adjusting word wrap used by
    // ClickableLabelWordWrapWide.
    // So we add a QLabel, as per
    // - http://stackoverflow.com/questions/8960233/subclassing-qlabel-to-show-native-mouse-hover-button-indicator/8960548#8960548

    // We can't have a stylesheet with both plain "attribute: value;"
    // and "QPushButton:pressed { attribute: value; }"; we get an error
    // "Could not parse stylesheet of object 0x...".
    // So we probably need a full stylesheet, and note that the text is in
    // a QLabel, not a QPushButton. We could generalize with a QWidget or
    // specify them exactly ("QPushButton, QLabel"). But "QWidget:pressed"
    // doesn't work.
    // Also, blending the QPushButton and the QLabel stuff and installing it
    // on the button screws things up w.r.t. the "pressed" bit.
    // A QLabel can't have the "pressed" attribute, but it screws up the button
    // press.
    // Also, the QLabel also needs to have the "pressed" background.
    // Re padding etc., see https://www.w3schools.com/css/css_boxmodel.asp
    QString button_css = QString(
                "QPushButton {"
                " background-color: %1;"
                " border: %2;"
                " border-radius: %3;"
                " font-size: %4;"
                " margin: 0;"
                " outline: none; "
                // ... METHOD 1 of switching off the inner (dotted) focus rect
                " padding: %5; "
                "} "
                "QPushButton:pressed {"
                " background-color: %6;"
                "}")
            .arg(colourCss(config.background_colour),  // 1
                 penCss(config.border_pen),  // 2
                 pixelCss(config.corner_radius_px),  // 3
                 pixelCss(config.font_size_px),  // 4
                 pixelCss(config.padding_px),  // 5
                 colourCss(config.pressed_background_colour));  // 6
    // note CSS specifiers:
    // :checked
    // :focus
    // :hover
    // :pressed
    const QString label_css = labelCss(config.text_colour);
#ifdef DEBUG_CSS
    qDebug() << "makeGraphicsTextButton: button CSS:" << button_css;
    qDebug() << "makeGraphicsTextButton: label CSS:" << label_css;
#endif

    ButtonAndProxy result;

    result.button = new QPushButton(parent);
    result.button->setFlat(true);
    result.button->setAttribute(Qt::WA_TranslucentBackground);
    result.button->setStyleSheet(button_css);
    // result.button->setStyle(new StyleNoFocusRect());
    // ... METHOD 2 of switching off the inner (dotted) focus rectangle

    if (!text.isEmpty()) {
        auto label = new QLabel(result.button);
        label->setStyleSheet(label_css);
        font.setPixelSize(config.font_size_px);
        label->setFont(font);
        label->setText(text);
        label->setWordWrap(true);
        label->setAlignment(config.text_alignment);
        label->setMouseTracking(false);
        label->setTextInteractionFlags(Qt::NoTextInteraction);

        auto layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->addWidget(label);

        result.button->setLayout(layout);
    }

    result.proxy = scene->addWidget(result.button);
    result.proxy->setGeometry(rect);

    return result;
}


LabelAndProxy makeText(QGraphicsScene* scene,  // text is added to scene
                       const QPointF& pos,
                       const TextConfig& config,
                       const QString& text,
                       QFont font,
                       QWidget* parent)
{
    Q_ASSERT(scene);
    // QGraphicsTextItem does not support alignment.
    // http://stackoverflow.com/questions/29483125/does-qgraphicstextitem-support-vertical-center-alignment
    QString css = labelCss(config.colour);
#ifdef DEBUG_CSS
    qDebug() << "makeText: CSS:" << css;
#endif

    LabelAndProxy result;
    result.label = new QLabel(text, parent);
    result.label->setStyleSheet(css);
    font.setPixelSize(config.font_size_px);
    result.label->setFont(font);
    result.label->setOpenExternalLinks(false);
    result.label->setTextInteractionFlags(Qt::NoTextInteraction);
    result.label->setAlignment(config.alignment);  // alignment WITHIN label

    QRectF rect(pos, QSizeF());
    if (config.width == -1) {
        result.label->setWordWrap(false);
        rect.setSize(result.label->size());
    } else {
        // word wrap
        result.label->setWordWrap(true);
        rect.setSize(QSizeF(config.width,
                            result.label->heightForWidth(config.width)));
    }

    // Now fix alignment of WHOLE object
    alignRect(rect, config.alignment);

    result.proxy = scene->addWidget(result.label);
    result.proxy->setGeometry(rect);

    return result;
}


AdjustablePieAndProxy makeAdjustablePie(QGraphicsScene* scene,
                                        const QPointF& centre,
                                        const int n_sectors,
                                        const qreal diameter,
                                        QWidget* parent)
{
    const qreal radius = diameter / 2.0;
    const QPointF top_left(centre - QPointF(radius, radius));
    AdjustablePieAndProxy result;
    result.pie = new AdjustablePie(n_sectors, parent);
    result.pie->setOverallRadius(radius);
    const QRectF rect(top_left, QSizeF(diameter, diameter));
    result.proxy = scene->addWidget(result.pie);
    result.proxy->setGeometry(rect);
    return result;
}


SvgWidgetAndProxy makeSvg(
        QGraphicsScene* scene,  // SVG is added to scene
        const QPointF& centre,
        const QString& svg,
        const QColor& pressed_background_colour,
        const QColor& background_colour,
        const bool transparent_for_mouse,
        QWidget* parent)
{
    SvgWidgetAndProxy result;
    const QByteArray contents = svg.toUtf8();

    result.widget = new SvgWidgetClickable(parent);
    result.widget->load(contents);
    result.widget->setBackgroundColour(background_colour);
    result.widget->setPressedBackgroundColour(pressed_background_colour);
    result.widget->setTransparentForMouseEvents(transparent_for_mouse);  // irrelevant!

    const QSizeF size = result.widget->sizeHint();
    const QPointF top_left(centre.x() - size.width() / 2,
                           centre.y() - size.height() / 2);
    const QRectF rect(top_left, size);

    result.proxy = scene->addWidget(result.widget);
    result.proxy->setGeometry(rect);
    result.proxy->setAcceptedMouseButtons(transparent_for_mouse
                                          ? Qt::NoButton
                                          : Qt::LeftButton);

    return result;
}


QGraphicsRectItem* makeObscuringRect(QGraphicsScene* scene,
                                     const QRectF& rect, const qreal opacity,
                                     const QColor& colour_ignoring_opacity)
{
    const QPen pen(Qt::NoPen);
    QColor colour(colour_ignoring_opacity);
    colour.setAlpha(alpha(opacity));
    const QBrush brush(colour);
    return scene->addRect(rect, pen, brush);
}


QGraphicsPixmapItem* makeImage(
        QGraphicsScene* scene,
        const QRectF& rect,
        const QString& filename,
        const qreal opacity,
        const Qt::AspectRatioMode aspect_ratio_mode,
        const Qt::TransformationMode transformation_mode_1,
        const Qt::TransformationMode transformation_mode_2)
{
    // https://stackoverflow.com/questions/5960074/qimage-in-a-qgraphics-scene
    const QPointF top_left = rect.topLeft();
    const QSize size = QSize(qRound(rect.width()), qRound(rect.height()));  // convert float to int
    const QPixmap pixmap_raw = QPixmap(filename);
    const QPixmap pixmap_scaled = pixmap_raw.scaled(size, aspect_ratio_mode,
                                                    transformation_mode_1);
    QGraphicsPixmapItem* img;
    if (opacity < 1.0) {
        auto opacity_img = new GraphicsPixmapItemWithOpacity(pixmap_scaled);
        opacity_img->setOpacity(opacity);
        img = opacity_img;
        scene->addItem(img);  // the scene takes ownership: http://doc.qt.io/qt-5/qgraphicsscene.html#addItem
    } else {
        img = scene->addPixmap(pixmap_scaled);
    }
    img->setOffset(top_left);
    img->setTransformationMode(transformation_mode_2);
    return img;
}


}  // namespace graphicsfunc
