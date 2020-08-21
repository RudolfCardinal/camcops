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

// #define DEBUG_TRANSLATIONS

#include "canvaswidget.h"
#include <QColor>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QRegion>
#include <QStyle>
#include <QStyleOption>
#include "common/colourdefs.h"
#include "lib/convert.h"
#include "lib/margins.h"
#include "lib/sizehelpers.h"

const QPoint INVALID_POINT(-1, -1);

const int DEFAULT_MIN_SHRINK_HEIGHT = 200;
const QColor DEFAULT_BORDER_COLOR(QCOLOR_SILVER);
const QColor DEFAULT_UNUSED_SPACE_COLOR(QCOLOR_SILVER);


CanvasWidget::CanvasWidget(const QImage::Format format, QWidget* parent) :
    CanvasWidget(QSize(0, 0), format, parent)
{
}


CanvasWidget::CanvasWidget(const QSize& size,
                           const QImage::Format format,
                           QWidget* parent) :
    QFrame(parent)
{
    m_format = format;
    setAllowShrink(false);
    m_minimum_shrink_height = DEFAULT_MIN_SHRINK_HEIGHT;
    m_adjust_display_for_dpi = true;

    m_border_width_px = 2;
    m_border_colour = DEFAULT_BORDER_COLOR;
    m_unused_space_colour = DEFAULT_UNUSED_SPACE_COLOR;
    // Default pen:
    m_pen.setColor(Qt::blue);
    m_pen.setWidth(2);

    m_point = INVALID_POINT;
    m_image_to_display_ratio = 1;

    setBorderCss();
    setImageSizeAndClearImage(size);
}


CanvasWidget::~CanvasWidget()
{
}


void CanvasWidget::setImageSizeAndClearImage(const QSize& size)
{
    // qDebug() << Q_FUNC_INFO;
    m_image = QImage(size, m_format);
    update();
}


void CanvasWidget::setAllowShrink(const bool allow_shrink)
{
    m_allow_shrink = allow_shrink;
    if (m_allow_shrink) {
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        // Can be shrunk in either direction.
        // We can't have a width-for-height constraint as well as a HFW
        // constraint; see http://doc.qt.io/qt-5/qsizepolicy.html#setWidthForHeight
        // Instead, we can draw according to our *actual* height...
        // Similarly, we don't need a HFW constraint, which will (in many of
        // our layouts) make the effective height *fixed* once the width is
        // determined; we do this as a widget that accepts any size up to its
        // maximum, and then just draws in a subset.
    } else {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
}


void CanvasWidget::setMinimumShrinkHeight(const int height)
{
    m_minimum_shrink_height = height;
}


void CanvasWidget::setBorderWidth(const int width)
{
    m_border_width_px = width;
    setBorderCss();
}


void CanvasWidget::setBorderColour(const QColor& colour)
{
    m_border_colour = colour;
    setBorderCss();
}


void CanvasWidget::setBorder(const int width, const QColor& colour)
{
    m_border_width_px = width;
    m_border_colour = colour;
    setBorderCss();
}


void CanvasWidget::setUnusedSpaceColour(const QColor& colour)
{
    m_unused_space_colour = colour;
}


QSize CanvasWidget::sizeHint() const
{
    // Size of m_image (which is m_size), plus size of borders.
    // To do this, we have to derive from QFrame rather than QWidget, I think.

    // Several ways don't work.
    // - QWidget::sizeHint() returns QSize(-1, -1) despite stylesheet borders,
    //   even after ensurePolished().
    // - getContentsMargins() returns 0, 0, 0, 0 despite stylesheet borders, if
    //   you inherit from a QWidget. But if you inherit from a QFrame... yup,
    //   it works!

    const Margins m = Margins::getContentsMargins(this);
    return m.addMarginsTo(desiredDisplaySize());
}


QSize CanvasWidget::minimumSizeHint() const
{
    if (!m_allow_shrink) {
        return desiredDisplaySize();
    }
    QSize minsize = imageSize();
    minsize.scale(QSize(minsize.width(), m_minimum_shrink_height),
                  Qt::KeepAspectRatio);
    return minsize;
}


void CanvasWidget::setPen(const QPen& pen)
{
    m_pen = pen;
}


void CanvasWidget::clear(const QColor& background)
{
    // qDebug() << Q_FUNC_INFO;
    m_image.fill(background);
    update();
}


void CanvasWidget::setImage(const QImage& image)
{
    // qDebug() << Q_FUNC_INFO;
    if (image.isNull()) {
        qWarning() << Q_FUNC_INFO << "Asked to set null image!";
    }
    m_image = image;
    update();
}


void CanvasWidget::setAdjustDisplayForDpi(const bool adjust_display_for_dpi)
{
    m_adjust_display_for_dpi = adjust_display_for_dpi;
    update();
}


void CanvasWidget::resizeEvent(QResizeEvent* event)
{
    QSize displaysize = imageSize();
    displaysize.scale(contentsRect().size(), Qt::KeepAspectRatio);
    // Store the ratio in a format that allows the most common operations to
    // use multiplication, not division:
    // http://stackoverflow.com/questions/4125033/floating-point-division-vs-floating-point-multiplication
    m_image_to_display_ratio = static_cast<double>(imageSize().width()) /
                               static_cast<double>(displaysize.width());

#ifdef DEBUG_TRANSLATIONS
    qDebug().nospace()
            << Q_FUNC_INFO
            << "- widget size " << event->size()
            << "; contents rect " << contentsRect()
            << "; m_image_to_display_ratio " << m_image_to_display_ratio;
#else
    Q_UNUSED(event)
#endif
}


void CanvasWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    // If you derive from a QWidget, you can't find out how big the stylesheet
    // borders are, so you can't help overwriting them. So, derive from a
    // QFrame, and draw inside its contentsRect().
    // - https://forum.qt.io/topic/18325
    // - http://stackoverflow.com/questions/22415057

    // 1. The standard bits: background via stylesheet, etc.
    // - http://stackoverflow.com/questions/18344135

    QStyleOption o;
    o.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &painter, this);

    // 2. Our bits
    const QRect cr = contentsRect();
    const QSize imagesize = imageSize();
    if (m_allow_shrink && cr.size() != imagesize) {
        // Scale
        QSize displaysize = imagesize;
        displaysize.scale(cr.size(), Qt::KeepAspectRatio);
        const QRect dest_active_rect = QRect(cr.topLeft(), displaysize);
        const QRect source_all_image(QPoint(0, 0), imagesize);
        painter.drawImage(dest_active_rect, m_image, source_all_image);

        // Optimizations are possible: we don't have to draw all of it...
        // http://blog.qt.io/blog/2006/05/13/fast-transformed-pixmapimage-drawing/
        // ... but I haven't implemented those optimizations.

//#ifdef DEBUG_TRANSLATIONS
//        QRect exposed_rect = painter.matrix().inverted()
//                             .mapRect(event->rect())
//                             .adjusted(-1, -1, 1, 1);
//        qDebug().nospace()
//                << Q_FUNC_INFO << " - contentsRect = " << cr
//                << ", exposed = " << exposed_rect;
//#endif

        // Paint unused space:
        QRegion unused(cr);
        unused -= QRegion(dest_active_rect);
        painter.setClipRegion(unused);
        const QBrush brush_unused(m_unused_space_colour);
        painter.fillRect(cr, brush_unused);

    } else {
        // No need to scale
        painter.drawImage(cr.left(), cr.top(), m_image);
    }
}


QPoint CanvasWidget::transformDisplayToImageCoords(QPoint point) const
{
    // Convert from widget coordinates (NB there's a frame) to contentsRect
    // coordinates:
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    point.rx() -= left;
    point.ry() -= top;

    // Now transform, if required, to account for any scaling that we're
    // doing:
    if (!m_allow_shrink) {
        return point;
    }
    QPoint result = QPoint(
        qRound(static_cast<double>(point.x()) * m_image_to_display_ratio),
        qRound(static_cast<double>(point.y()) * m_image_to_display_ratio)
    );
#ifdef DEBUG_TRANSLATIONS
    qDebug() << Q_FUNC_INFO << point << "->" << result;
#endif
    return result;
}


void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        m_point = INVALID_POINT;
        drawTo(transformDisplayToImageCoords(event->pos()));
        update();
    }
}


void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        drawTo(transformDisplayToImageCoords(event->pos()));
        update();
    }
}


void CanvasWidget::drawTo(const QPoint& pt)
{
    // The coordinates are IMAGE coordinates.
    if (m_image.isNull()) {
        qWarning() << Q_FUNC_INFO << "null image";
        return;
    }

    // Draw
    QPainter p(&m_image);
    p.setPen(m_pen);
    QPoint from = m_point;
    if (from == INVALID_POINT) {
        from = pt;
    }
    p.drawLine(from, pt);
    m_point = pt;

    emit imageChanged();
}


QImage CanvasWidget::image() const
{
    return m_image;
}


QSize CanvasWidget::imageSize() const
{
    return m_image.size();
}


QSize CanvasWidget::desiredDisplaySize() const
{
    if (m_adjust_display_for_dpi) {
        return convert::convertSizeByLogicalDpi(imageSize());
    }
    return imageSize();
}


void CanvasWidget::setBorderCss()
{
    QString css = QString("border: %1px solid rgba(%2,%3,%4,%5);")
            .arg(m_border_width_px)
            .arg(m_border_colour.red())
            .arg(m_border_colour.green())
            .arg(m_border_colour.blue())
            .arg(m_border_colour.alpha());
    setStyleSheet(css);
}
