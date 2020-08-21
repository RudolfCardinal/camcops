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

// #define DEBUG_PAINTING
// #define DEBUG_ACTIVE_CONTENTS_RECT
// #define DEBUG_VERBOSE
// #define DEBUG_VERY_VERBOSE
// #define DEBUG_FULL_REPAINT
// #define DEBUG_INTERACTION
// #define DEBUG_SIZE_HINT
// #define DEBUG_SHOW_BACKGROUND

/*

Old scaling/painting method, prior to 2020-02-28:

    Scales each image separately. Stacks them. However, this causes a problem,
    e.g. if the thermometer has 101 images of 30 pixels height each (ideally
    3030 height in total), and the scale factor is (for example) 0.399895; each
    image gets scaled to 11 pixels, for a total height of 1111 pixels, rather
    than the 1211 you might expect.

Tried but rubbish, 2020-02-28:

    Calculate the entire image size as a rescaled version of the sum of all
    the components, but then scale each image separately during plotting.
    Looks dreadful because of tiny gaps.

Also a poor idea, 2020-02-28:

    Draw all images into an internal composite image; then scale that to the
    screen.

Finally, the right idea, 2020-03-01:

    Use QPainter's built-in scaling and translation transformations and draw
    everything to the primary painter.

    Also optimized rowForPoint() and various other drawing functions.

*/

#include "thermometer.h"
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QRegion>
#include <QtMath>
#include "graphics/graphicsfunc.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"

const int UNSELECTED = -1;


// ============================================================================
// Functions to increase legibility
// ============================================================================

inline qreal divide(const int& x, const int& divisor)
{
    return static_cast<qreal>(x) / static_cast<qreal>(divisor);
}


// ============================================================================
// Thermometer
// ============================================================================

Thermometer::Thermometer(const QVector<QPixmap>& active_images,
                         const QVector<QPixmap>& inactive_images,
                         const QStringList* left_strings,
                         const QStringList* right_strings,
                         int left_string_span,
                         int image_span,
                         int right_string_span,
                         bool allow_deselection,
                         bool read_only,
                         bool rescale_images,
                         double rescale_image_factor,
                         int text_gap_px,
                         QWidget* parent) :
    QWidget(parent),
    m_active_images(active_images),
    m_inactive_images(inactive_images),
    m_n_rows(active_images.length()),
    m_use_left_strings(left_strings != nullptr),
    m_use_right_strings(right_strings != nullptr),
    m_left_string_span(left_string_span),
    m_image_span(image_span),
    m_right_string_span(right_string_span),
    m_allow_deselection(allow_deselection),
    m_read_only(read_only),
    m_rescale_images(rescale_images),
    m_rescale_image_factor(rescale_image_factor),
    m_text_gap_px(text_gap_px),
    // m_unused_space_colour(QColor()),
    m_selected_index(UNSELECTED),
    m_touching_index(UNSELECTED),
    m_start_touch_index(UNSELECTED)
{
    // ------------------------------------------------------------------------
    // Set basic parameters.
    // ------------------------------------------------------------------------

    if (m_n_rows == 0) {
        uifunc::stopApp("No rows to Thermometer");
    }
    if (m_inactive_images.length() != m_n_rows) {
        uifunc::stopApp("Wrong inactive_images length to Thermometer");
    }
    if (left_strings) {
        m_left_strings = *left_strings;
        if (m_left_strings.length() != m_n_rows) {
            uifunc::stopApp("Wrong left_strings length to Thermometer");
        }
        if (m_left_string_span <= 0) {
            uifunc::stopApp("Thermometer: left_string_span <= 0 "
                            "but there are left strings");
        }
    } else {
        m_left_string_span = 0;
    }
    if (right_strings) {
        m_right_strings = *right_strings;
        if (m_right_strings.length() != m_n_rows) {
            uifunc::stopApp("Wrong right_strings length to Thermometer");
        }
        if (m_right_string_span <= 0) {
            uifunc::stopApp("Thermometer: right_string_span <= 0 "
                            "but there are right strings");
        }
    } else {
        m_right_string_span = 0;
    }
    if (m_image_span <= 0) {
        uifunc::stopApp("Image scale values to Thermometer must be >0");
    }
    if (m_left_string_span < 0 ||
            m_right_string_span < 0) {
        uifunc::stopApp("Negative string scale values to Thermometer");
    }

    // ------------------------------------------------------------------------
    // Set up layout: horizontal
    // ------------------------------------------------------------------------

    auto imageScale = [this](int x) -> qreal {
        return m_rescale_images
                ? (x * m_rescale_image_factor)
                : x;
    };
    auto spanScale = [this](int span) -> qreal {
        return static_cast<qreal>(span) *
                m_image_width /
                static_cast<qreal>(m_image_span);
    };

    // The image size (scaled) is our starting point.
    const int first_image_raw_width = m_active_images.at(0).width();
    m_image_width = imageScale(first_image_raw_width);

    // Then the other columns, by span allocation.
    // Left string width is to left string span as image width is to image span:
    m_lstring_width = spanScale(m_left_string_span);
    // Similarly on the right:
    m_rstring_width = spanScale(m_right_string_span);

    // Positions, left to right:
    m_lstring_left = 0;
    m_lstring_right = m_lstring_left + m_lstring_width;
    m_image_left = m_lstring_right + m_text_gap_px;
    m_image_right = m_image_left + m_image_width;
    m_rstring_left = m_image_right + m_text_gap_px;
    m_target_total_size.rwidth() = qCeil(m_rstring_left + m_rstring_width);

    // ------------------------------------------------------------------------
    // Set up layout: vertical.
    // Also create "being touched" images.
    // ------------------------------------------------------------------------

    const bool pressed_marker_behind = false;  // colour on top
    for (int i = 0; i < m_n_rows; ++i) {
        const QPixmap& active_image = m_active_images.at(i);
        const QPixmap& inactive_image = m_inactive_images.at(i);
        const int unscaled_height = active_image.height();
        const qreal scaled_height = imageScale(unscaled_height);
        if (i == 0) {
            m_raw_image_tops.append(0);
            m_image_top_bottom.append(QPair<qreal, qreal>(0, scaled_height));
        } else {
            m_raw_image_tops.append(
                        m_raw_image_tops[i - 1] +
                        m_active_images[i - 1].height());
            const qreal prev_bottom = m_image_top_bottom[i - 1].second;
            m_image_top_bottom.append(QPair<qreal, qreal>(
                            prev_bottom, prev_bottom + scaled_height));
        }

        // Checks
        if (inactive_image.height() != unscaled_height) {
               qWarning()
                    << Q_FUNC_INFO
                    << "image at index" << i
                    << "has active image height" << unscaled_height
                    << "but inactive image height" << inactive_image.height()
                    << "- may look strange!";
        }
        if (active_image.width() != first_image_raw_width) {
            qWarning()
                    << Q_FUNC_INFO
                    << "active image" << i
                    << "has discrepant width of" << active_image.width()
                    << "versus initial one of" << first_image_raw_width;
        }
        if (inactive_image.width() != first_image_raw_width) {
            qWarning()
                    << Q_FUNC_INFO
                    << "inactive image" << i
                    << "has discrepant width of" << inactive_image.width()
                    << "versus initial one of" << first_image_raw_width;
        }

        // Create "being touched" images.
        m_active_touched_images.append(
            uifunc::addPressedBackground(active_image, pressed_marker_behind));
        m_inactive_touched_images.append(
            uifunc::addPressedBackground(inactive_image, pressed_marker_behind));
    }
    m_target_total_size.rheight() = qCeil(
                m_image_top_bottom[m_n_rows - 1].second);

    // ------------------------------------------------------------------------
    // Final layout calculations
    // ------------------------------------------------------------------------

    m_aspect_ratio = divide(m_target_total_size.width(),
                            m_target_total_size.height());

    // ------------------------------------------------------------------------
    // Debugging
    // ------------------------------------------------------------------------

#ifdef DEBUG_VERBOSE
    qDebug().nospace()
            << "m_n_rows " << m_n_rows
            << ", m_use_left_strings " << m_use_left_strings
            << ", m_use_right_strings " << m_use_right_strings
            << ", m_left_string_span " << m_left_string_span
            << ", m_image_span " << m_image_span
            << ", m_right_string_span " << m_right_string_span
            << ", m_rescale_images " << m_rescale_images
            << ", m_rescale_image_factor " << m_rescale_image_factor
            << ", m_text_gap_px " << m_text_gap_px;
    qDebug().nospace()
            << "m_lstring_width " << m_lstring_width
            << ", m_image_width " << m_image_width
            << ", m_rstring_width " << m_rstring_width
            << ", m_lstring_left " << m_lstring_left
            << ", m_image_left " << m_image_left
            << ", m_image_right " << m_image_right
            << ", m_rstring_left " << m_rstring_left
            << ", m_image_top_bottom " << m_image_top_bottom
            << ", m_target_total_size " << m_target_total_size
            << ", m_aspect_ratio " << m_aspect_ratio;
#endif

#ifdef DEBUG_SHOW_BACKGROUND
    QPalette palette = QPalette();
    palette.setColor(QPalette::Background, Qt::yellow);
    setAutoFillBackground(true);
    setPalette(palette);
#endif

    // ------------------------------------------------------------------------
    // Other
    // ------------------------------------------------------------------------

    // Set Qt size policy
    setSizePolicy(sizehelpers::maximumMaximumHFWPolicy());
}


// ----------------------------------------------------------------------------
// Standard Qt widget overrides
// ----------------------------------------------------------------------------

bool Thermometer::hasHeightForWidth() const
{
    return true;
}


int Thermometer::heightForWidth(const int width) const
{
    // We work this based on aspect ratio, which is width/height.

    const int hfw = qMin(qCeil(static_cast<qreal>(width) / m_aspect_ratio),
                         m_target_total_size.height());
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << "width" << width << "-> hfw" << hfw;
#endif
    return hfw;
}


QSize Thermometer::sizeHint() const
{
#ifdef DEBUG_SIZE_HINT
    qDebug() << Q_FUNC_INFO << m_target_total_size;
#endif
    return m_target_total_size;
}


QSize Thermometer::minimumSizeHint() const
{
    return QSize(0, 0);
}


// ----------------------------------------------------------------------------
// Picking an image
// ----------------------------------------------------------------------------

void Thermometer::setSelectedIndex(int selected_index)
{
#ifdef DEBUG_INTERACTION
    qDebug() << Q_FUNC_INFO << selected_index;
#endif
    const int old_selected_index = m_selected_index;
    if (selected_index < 0) {
        m_selected_index = UNSELECTED;
    } else if (selected_index < m_n_rows) {
        m_selected_index = selected_index;
    } else {
        qWarning()
                 << Q_FUNC_INFO
                << "Bad index:" << selected_index
                << "but number of rows is" << m_n_rows;
        m_selected_index = UNSELECTED;
    }
    if (m_selected_index == old_selected_index) {
        // Nothing to do
#ifdef DEBUG_INTERACTION
        qDebug()
            << Q_FUNC_INFO
            << "Nothing to do; m_selected_index unchanged at"
            << m_selected_index;
#endif
        return;
    }

    // Tell clients
    emit selectionIndexChanged(m_selected_index);

    // Trigger refresh
#ifdef DEBUG_INTERACTION
    qDebug() << Q_FUNC_INFO
             << "repainting for m_selected_index" << m_selected_index;
#endif
#ifdef DEBUG_FULL_REPAINT
    repaint();
#else
    QRegion redraw_region;
    if (old_selected_index != UNSELECTED) {
        redraw_region += imageRect(old_selected_index);
    }
    if (m_selected_index != UNSELECTED) {
        redraw_region += imageRect(m_selected_index);
    }
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << "redraw_region" << redraw_region;
#endif
    if (!redraw_region.isEmpty()) {
        repaint(redraw_region);
    }
#endif
}


// ----------------------------------------------------------------------------
// Event handling
// ----------------------------------------------------------------------------

void Thermometer::mousePressEvent(QMouseEvent* event)
{
#ifdef DEBUG_INTERACTION
    qDebug() << Q_FUNC_INFO << event;
#endif
    if (m_read_only) {
        return;
    }
    // Which row is this event in?
    int in_row = rowForPoint(event->pos());

    if (in_row != UNSELECTED) {
        // User has clicked in a row. Start of a new touch.
        setTouchedIndex(in_row);
        m_start_touch_index = in_row;
    }
}


void Thermometer::mouseReleaseEvent(QMouseEvent* event)
{
#ifdef DEBUG_INTERACTION
    qDebug() << Q_FUNC_INFO << event;
#endif
    if (m_read_only) {
        return;
    }
    int in_row = rowForPoint(event->pos());
    // User has released mouse in a row.
    setTouchedIndex(UNSELECTED);
    // If it's the same row they started in, that's a selection toggle.
    if (in_row == m_start_touch_index) {
        const bool was_selected = m_selected_index == in_row;
#ifdef DEBUG_INTERACTION
        qDebug() << Q_FUNC_INFO
                 << "toggle selection; was_selected" << was_selected;
#endif
        setSelectedIndex(was_selected && m_allow_deselection
                         ? UNSELECTED
                         : in_row);
    }
}


void Thermometer::mouseMoveEvent(QMouseEvent* event)
{
#ifdef DEBUG_INTERACTION
    qDebug() << Q_FUNC_INFO << event;
#endif
    if (m_read_only) {
        return;
    }
    int in_row = rowForPoint(event->pos());
    // Moved. Still touching.
    // May or may not be in the row that they *started* touching.
    if (in_row == m_start_touch_index) {
        setTouchedIndex(in_row);
    } else {
        setTouchedIndex(UNSELECTED);
    }
}


// ignore QEvent::MouseButtonDblClick for now


void Thermometer::paintEvent(QPaintEvent* event)
{
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO;
#endif
    QPainter painter(this);
    const QRect acr = activeContentsRect();
    const QRect external_redraw_rect = event->rect();
    const QRectF internal_redraw_rect = internalRect(external_redraw_rect, acr);
    const Qt::Alignment leftstring_align = Qt::AlignRight | Qt::AlignVCenter;
    const Qt::Alignment rightstring_align = Qt::AlignLeft | Qt::AlignVCenter;
    // Note that using AlignVCenter throughout looks better (despite some
    // clipping) than switching to top alignment for the top string and bottom
    // alignment for the bottom string. Ideally we'd get rid of the clipping
    // too by rescaling the whole image part of the widget further, but not bad
    // as it is.

    // painter.save();

    // Apply translations so we can draw using internal coordinates.
    // The translations work in an "internal to external" direction; see
    // https://doc.qt.io/qt-5/qtwidgets-painting-transformations-example.html.
    // First, we scale:
    QSize displaysize = acr.size();  // starting size
    qreal scale = static_cast<qreal>(displaysize.height()) /
            static_cast<qreal>(m_target_total_size.height());
    painter.scale(scale, scale);
    // Then we translate from internal (0,0) to the contentrect:
    painter.translate(acr.topLeft());

    // If we are scaling the images small, the text becomes tiny. Scale the
    // text back:
    QFont font = painter.font();
    // qDebug() << "font before:" << font;
    // Looks like: QFont( "Sans Serif,12,-1,5,50,0,0,0,0,0" )
    // See qfont.cpp.
    // family, pointsize, pixelsize, stylehint, ...
    // Use points, not pixels.
    font.setPointSizeF(font.pointSizeF() / scale);
    // qDebug() << "font after:" << font;
    painter.setFont(font);

    // Draw text
    for (int row = 0; row < m_n_rows; ++row) {
        const qreal row_top = m_image_top_bottom[row].first;
        const qreal row_bottom = m_image_top_bottom[row].second;
        const qreal row_height = row_bottom - row_top;
        const qreal vertical_midpoint = row_top + row_height / 2;
#ifdef DEBUG_VERY_VERBOSE
        qDebug().nospace()
            << "row " << row
            << ", row_height " << row_height
            << ", top " << row_top
            << ", vertical_midpoint " << vertical_midpoint;
#endif

        // Draw left string, vertically centred
        if (m_use_left_strings) {
            const QRectF leftstring_rect(m_lstring_left, row_top,
                                         m_lstring_width, row_height);
            // Now compensate for
            const QString& text = m_left_strings.at(row);
            if (!text.isEmpty() &&
                    internal_redraw_rect.intersects(leftstring_rect)) {
#ifdef DEBUG_PAINTING
                qDebug() << "Drawing left string for row" << row;
#endif
                graphicsfunc::drawText(
                    painter,
                    QPointF(m_lstring_right, vertical_midpoint),
                    leftstring_align,
                    text
                    // bounding rectangle? Not sure. Probably OK without (text
                    // will overlap when scaled very small)
                );
            }
        }

        // Draw right string
        if (m_use_right_strings) {
            const QRectF rightstring_rect(m_rstring_left, row_top,
                                          m_rstring_width, row_height);
            const QString& text = m_right_strings.at(row);
            if (!text.isEmpty() &&
                    internal_redraw_rect.intersects(rightstring_rect)) {
#ifdef DEBUG_PAINTING
                qDebug() << "Drawing right string for row" << row;
#endif
                graphicsfunc::drawText(
                    painter,
                    QPointF(m_rstring_left, vertical_midpoint),
                    rightstring_align,
                    text
                    // bounding rectangle? Not sure. Probably OK without (text
                    // will overlap when scaled very small)
                );
            }
        }
    }

    // Choose images to draw
    QVector<const QPixmap*> chosen_images;
    for (int row = 0; row < m_n_rows; ++row) {
        const bool touching = m_touching_index == row;
        const bool selected = m_selected_index == row;
        const QPixmap* image = selected
                    ? (touching ? &m_active_touched_images.at(row)
                                : &m_active_images.at(row))
                    : (touching ? &m_inactive_touched_images.at(row)
                                : &m_inactive_images.at(row));
        chosen_images.append(image);
    }

    // Draw images
    painter.translate(m_image_left, 0.0);
    if (m_rescale_images) {
        painter.scale(m_rescale_image_factor, m_rescale_image_factor);
    }
    for (int row = 0; row < m_n_rows; ++row) {
        const QRectF image_intcoords(
            m_image_left,
            m_image_top_bottom[row].first,
            m_image_width,
            m_image_top_bottom[row].second - m_image_top_bottom[row].first
        );
        if (internal_redraw_rect.intersects(image_intcoords)) {
#ifdef DEBUG_PAINTING
            qDebug() << "Drawing image for row" << row;
#endif
            const QPointF topleft_imagecoords(0, m_raw_image_tops[row]);
            painter.drawPixmap(topleft_imagecoords, *chosen_images[row]);
        }
    }

    // Paint unused region? Nope -- if you don't, it looks fine and just
    // shows whatever's behind.
    /*
    painter.restore();
    const QRect cr = contentsRect();
    QRegion unused(cr);
    unused -= QRegion(acr);
    painter.setClipRegion(unused);
    const QBrush brush_unused(m_unused_space_colour);
    painter.fillRect(cr, brush_unused);
    */
}


void Thermometer::setTouchedIndex(int touched_index)
{
#ifdef DEBUG_INTERACTION
    qDebug() << Q_FUNC_INFO << touched_index;
#endif
    const int old_touching_index = m_touching_index;
    if (touched_index < 0) {
        m_touching_index = UNSELECTED;
    } else if (touched_index < m_n_rows) {
        m_touching_index = touched_index;
    } else {
        qWarning()
                 << Q_FUNC_INFO
                << "Bad index:" << touched_index
                << "but number of rows is" << m_n_rows;
        m_touching_index = UNSELECTED;
    }
    if (m_touching_index == old_touching_index) {
        // Nothing to do
#ifdef DEBUG_INTERACTION
        qDebug()
            << Q_FUNC_INFO
            << "Nothing to do; m_touching_index unchanged at"
            << m_touching_index;
#endif
        return;
    }

    // Trigger refresh
#ifdef DEBUG_INTERACTION
    qDebug() << Q_FUNC_INFO
             << "repainting for m_touching_index" << m_touching_index;
#endif
#ifdef DEBUG_FULL_REPAINT
    repaint();
#else
    QRegion redraw_region;
    if (old_touching_index != UNSELECTED) {
        redraw_region += imageRect(old_touching_index);
    }
    if (m_touching_index != UNSELECTED) {
        redraw_region += imageRect(m_touching_index);
    }
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << "redraw_region" << redraw_region;
#endif
    if (!redraw_region.isEmpty()) {
        repaint(redraw_region);
    }
#endif
}


// ----------------------------------------------------------------------------
// Coordinate calculations
// ----------------------------------------------------------------------------

QRect Thermometer::activeContentsRect() const
{
    const QRect cr = contentsRect();
    QSize displaysize = m_target_total_size;
    displaysize.scale(cr.size(), Qt::KeepAspectRatio);
    const QRect acr(cr.topLeft(), displaysize);
#ifdef DEBUG_ACTIVE_CONTENTS_RECT
    qDebug() << Q_FUNC_INFO
             << "contentsRect() = " << cr
             << "-> activeContentsRect() = " << acr;
#endif
    return acr;
}


QRect Thermometer::imageRect(int row) const
{
    // Returns an image's rectangle in EXTERNAL (SCREEN) coordinates.
    // Used to calculate regions for redrawing.
    if (row == UNSELECTED || row >= m_n_rows) {
        qWarning() << Q_FUNC_INFO << "Bad row parameter";
        return QRect();
    }
    const QPointF internal_left_top(m_image_left, m_image_top_bottom[row].first);
    const QPointF internal_right_bottom(m_image_right, m_image_top_bottom[row].second);
    const QRect acr = activeContentsRect();
    const QPoint external_left_top = externalPt(internal_left_top, acr);
    const QPoint external_right_bottom = externalPt(internal_right_bottom, acr);
    return QRect(external_left_top, external_right_bottom);
}


int Thermometer::rowForPoint(const QPoint& pt) const
{
    // Which row is this event in?
    // Used to find rows corresponding to a mouse/touch event.

    const QRect acr = activeContentsRect();
    const QPointF ip = internalPt(pt, acr);

    // Out of range horizontally?
    if (ip.x() < m_image_left || ip.x() > m_image_right) {
        return UNSELECTED;
    }

    // Within a row?
    const qreal y = ip.y();
    for (int r = 0; r < m_n_rows; ++r) {
        const QPair<qreal, qreal>& tb = m_image_top_bottom[r];
        if (y < tb.first) {
            // Above our top (and we're proceeding top to bottom).
            return UNSELECTED;
        }
        if (y <= tb.second) {
            // Within this row.
            return r;
        }
    }

    // Below widget.
    return UNSELECTED;
}


qreal Thermometer::widgetScaleFactor(const QRect& activecontentsrect) const
{
    return divide(activecontentsrect.width(), m_target_total_size.width());
}


QPoint Thermometer::externalPt(const QPointF& internal_pt,
                               const QRect& activecontentsrect) const
{
    const qreal wsf = widgetScaleFactor(activecontentsrect);
    return QPoint(
        activecontentsrect.left() + internal_pt.x() * wsf,
        activecontentsrect.top() + internal_pt.y() * wsf
    );
}


QPointF Thermometer::internalPt(const QPoint& external_pt,
                                const QRect& activecontentsrect) const
{
    const qreal wsf = widgetScaleFactor(activecontentsrect);
    return QPointF(
        (external_pt.x() - activecontentsrect.left()) / wsf,
        (external_pt.y() - activecontentsrect.top()) / wsf
    );
}


QRectF Thermometer::internalRect(const QRect& external_rect,
                                 const QRect& activecontentsrect) const
{
    const qreal wsf = widgetScaleFactor(activecontentsrect);
    return QRectF(  // left, top, width, height
        (external_rect.left() - activecontentsrect.left()) / wsf,
        (external_rect.top() - activecontentsrect.top()) / wsf,
        external_rect.width() / wsf,
        external_rect.height() / wsf
    );
}
