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
#include <QImage>
#include <QPen>
#include <QPoint>
#include <QSize>
#include <QFrame>

class QColor;
class QPaintEvent;
class QMouseEvent;

// See also http://stackoverflow.com/questions/28947235/qt-draw-on-canvas


class CanvasWidget : public QFrame
{
    // Widget for users to draw on a canvas (either blank, or with a starting
    // image).

    Q_OBJECT
public:

    // Construct with a blank size.
    CanvasWidget(QImage::Format format = QImage::Format_RGB32,
                 QWidget* parent = nullptr);

    // Construct with a known size.
    CanvasWidget(const QSize& size,
                 QImage::Format format = QImage::Format_RGB32,
                 QWidget* parent = nullptr);

    // Destructor.
    ~CanvasWidget() override;

    // Set image to a new, blank image of the specified size.
    void setImageSizeAndClearImage(const QSize& size);

    // Choose whether the widget is allowed to shrink beyond its target size
    // (for small screens).
    void setAllowShrink(bool allow_shrink);

    // If we can shrink [see setAllowShrink()], what's our minimum height?
    void setMinimumShrinkHeight(int height);

    // Set the width of the widget's border.
    void setBorderWidth(int width);

    // Set the colour of the widget's border.
    void setBorderColour(const QColor& colour);

    // Set the width/colour of the widget's border.
    void setBorder(int width, const QColor& colour);

    // If the active canvas is smaller than the widget, what colour should we
    // use for the unused space?
    void setUnusedSpaceColour(const QColor& colour);

    // Set the pen that the user draws with.
    void setPen(const QPen& pen);

    // Clear the canvas image to a backgroudn colour.
    void clear(const QColor& background);

    // Set the canvas image to another image (clearing the user's drawing);
    // this is like clear() when you're drawing on top of a base image.
    void setImage(const QImage& image);

    // Should we resize our image according to the DPI setting of the display?
    void setAdjustDisplayForDpi(bool adjust_display_for_dpi);

    // Draw from our current drawing point, m_point, to a new point.
    // Then set m_point to the new point.
    void drawTo(const QPoint& pt);

    // Standard Qt widget overrides.
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

    // Return the canvas image.
    QImage image() const;

signals:

    // "The image has changed as a result of user drawing."
    void imageChanged();

protected:

    // Standard Qt widget overrides.
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;

    // Transform a screen coordinate to a coordinate within our image.
    // Takes account of margins etc., then any image scaling.
    QPoint transformDisplayToImageCoords(QPoint point) const;

    // Sets CSS for our widget's border.
    void setBorderCss();

    // Returns the size of our image, m_image.
    QSize imageSize() const;

    // How big would we like our image to be?
    QSize desiredDisplaySize() const;

protected:

    // There are three relevant sizes:
    // - imageSize() = m_image.size(): the size of the image being edited
    // - the size of the entire canvas area, from contentsRect()
    // - a third size, displaysize, the display size of the image, such that
    //      image_size = m_image_to_display_ratio * displaysize
    //   The displaysize may be different from the widget contentsRect()
    //   because we maintain the aspect ratio of the image.

    QImage::Format m_format;  // underlying image format
    QImage m_image;  // our image
    bool m_allow_shrink;  // allow the widget/image to shrink, for small screens?
    int m_minimum_shrink_height;  // if m_allow_shrink: what's our minimum height?
    bool m_adjust_display_for_dpi;  // adjust image size for the current DPI setting?

    int m_border_width_px;  // border width, in pixels
    QColor m_border_colour;  // border colour
    QColor m_unused_space_colour;  // see setUnusedSpaceColour()
    QPen m_pen;  // pen that the user draws with

    double m_image_to_display_ratio;  // scaling factor
    QPoint m_point;  // last point that the user drew at
};
