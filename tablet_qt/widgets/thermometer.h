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

#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>


class Thermometer : public QWidget
{
    /*

    Represents clickable images/text in a vertical stack, e.g.:

        image0      text0
        image1      text1
        image2      text2

    and has two images (active + inactive) for each slot.
    (It also applies a "being touched" colour.)

    - The images may be pre-scaled.
    - The widget scales up to the maximum size of the images/text.
    - The aspect ratio of images is preserved.

    - No use yet for adding images on the fly.
    - Fonts currently via stylesheets.
    - No current support for vertical gaps between images (generally the point
      is to have no gap).

    The layout of each row is as follows:

        left_text IMAGE_IMAGE right_text
        |       | |         | |        |
        1       2 3         4 5        6
        aaaaaaaaa bbbbbbbbbbb cccccccccc
                 g           g
        dddddddddddddddddddddddddddddddd

    The widget draws to a pixmap, then draws that pixmap to the screen.
    In internal (pixmap) coordinates:

    Positions:

        [1] m_lstring_left = 0
        [2] m_lstring_right
        [3] m_image_left
        [4] m_image_right
        [5] m_rstring_left
        [6]

    Widths:
        [a] m_lstring_width;
            left_string_span / (left_string_span + image_span + right_string_span)
        [b] m_image_width;
            image_span / (left_string_span + image_span + right_string_span)
        [c] m_rstring_width;
            right_string_span / (left_string_span + image_span + right_string_span)
        [d] m_target_total_size.width()
        [g] text_gap_px, m_text_gap_px

    */

    Q_OBJECT
public:

    // Constructor.
    explicit Thermometer(
            const QVector<QPixmap>& active_images,  // top to bottom
            const QVector<QPixmap>& inactive_images,  // top to bottom
            const QStringList* left_strings = nullptr,  // top to bottom
            const QStringList* right_strings = nullptr,  // top to bottom
            int left_string_span = 1,  // arbitrary int representing "left text column proportion"
            int image_span = 1,  // arbitrary int representing "image column proportion"
            int right_string_span = 1,  // arbitrary int representing "right text column proportion"
            bool allow_deselection = true,  // allow images to be re-clicked to deselect them?
            bool read_only = false,  // read-only mode?
            bool rescale_images = false,  // rescale from images' intrinsic size?
            double rescale_image_factor = 1.0,  // if rescale: scale factor
            int text_gap_px = 4,  // gap between images and adjacent text
            QWidget* parent = nullptr);

    // ------------------------------------------------------------------------
    // Standard Qt widget overrides.
    // ------------------------------------------------------------------------
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int width) const override;
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

    // ------------------------------------------------------------------------
    // Picking an image
    // ------------------------------------------------------------------------
    // Set the selected image (negative means "none selected") and update
    // the display accordingly.
    void setSelectedIndex(int selected_index);

signals:

    // "The user has changed the selection."
    void selectionIndexChanged(int index);

protected:

    // ------------------------------------------------------------------------
    // Event handling
    // ------------------------------------------------------------------------

    // Standard Qt widget overrides.
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;

    // Update the display to indicate which image is being *touched*. The user
    // can touch lots (e.g. moving finger up/down on the stack) but until they
    // release their finger, the selection won't change. This handles the
    // finger-moving stuff.
    void setTouchedIndex(int touched_index);

    // ------------------------------------------------------------------------
    // Coordinate calculations
    // ------------------------------------------------------------------------

    // Return the part of the contentsRect() that fits our aspect ratio, in
    // case we are sized oddly by our owner.
    QRect activeContentsRect() const;

    // Returns the image rectangle for a given row, in external (screen) space.
    // Used to calculate regions for redrawing.
    QRect imageRect(int row) const;

    // Returns the row number containing the screen coordinates specified, or
    // -1 if none do.
    int rowForPoint(const QPoint& pt) const;

    // Scale factor, as ratio: external/internal.
    qreal widgetScaleFactor(const QRect& activecontentsrect) const;

    // Convert internal (pixmap) coordinates to external (screen) coordinates:
    QPoint externalPt(const QPointF& internal_pt,
                      const QRect& activecontentsrect) const;

    // Convert external (screen) coordinates to internal (pixmap) coordinates.
    QPointF internalPt(const QPoint& external_pt,
                       const QRect& activecontentsrect) const;
    QRectF internalRect(const QRect& external_rect,
                        const QRect& activecontentsrect) const;

    // ------------------------------------------------------------------------
    // Data
    // ------------------------------------------------------------------------
protected:
    // Config:
    QVector<QPixmap> m_active_images;  // all active (selected) images, top to bottom
    QVector<QPixmap> m_inactive_images;  // all inactive (unselected) images, top to bottom
    int m_n_rows;  // number of rows (each with image + text)
    bool m_use_left_strings;  // show text on the left of the images?
    bool m_use_right_strings;  // show text on the right of the images?
    QStringList m_left_strings;  // list of "left" strings
    QStringList m_right_strings;  // list of "right" strings
    int m_left_string_span;  // relative width of "left text" column
    int m_image_span;  // relative width of "image" column
    int m_right_string_span;  // relative width of "right text" column
    bool m_allow_deselection;  // allow returning to "none selected" state?
    bool m_read_only;  // read-only mode?
    bool m_rescale_images;  // rescale images?
    double m_rescale_image_factor;  // if rescale: by what factor?
    int m_text_gap_px;  // gap between images and adjacent text
    // QColor m_unused_space_colour;  // colour for any "unpainted" area

    // Details of the current selection:
    int m_selected_index;  // -1 for none selected, or zero-based index of selected row
    int m_touching_index;  // similarly, for row being touched now
    int m_start_touch_index;  // row in which the current touch began

    // Calculated layout, in raw image coordinates:
    QVector<int> m_raw_image_tops;  // top coordinate of each image

    // Calculated layout, in internal (pixmap) coordinates:
    qreal m_lstring_width;  // width of "left string" column
    qreal m_image_width;  // width of "image" column
    qreal m_rstring_width;  // width of "right string" column
    qreal m_lstring_left;  // left edge of left string; always 0
    qreal m_lstring_right;  // right edge of left string
    qreal m_image_left;  // left edge of image
    qreal m_image_right;  // right edge of image
    qreal m_rstring_left;  // left edge of right string
    QVector<QPair<qreal, qreal>> m_image_top_bottom;
    QSize m_target_total_size;  // final target size
    qreal m_aspect_ratio;  // widget aspect ratio; width / height

    // Modified images (modified to show "currently being touched" shading):
    QVector<QPixmap> m_active_touched_images;  // "selected and being touched" images
    QVector<QPixmap> m_inactive_touched_images;  // "unselected and being touched" images
};
