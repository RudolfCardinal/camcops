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
#include <QPixmap>
#include <QPushButton>
#include <QSize>


class ImageButton : public QPushButton
{
    // Button that shows an icon image, and another when being pressed (to
    // give visual feedback).
    // This should be more efficient than an equivalent method using
    // stylesheets, and also allows the use of the global QPixmapCache.
    //
    // Don't use for text; use ClickableLabel or ClickableLabelWordWrapWide
    // for that.

    Q_OBJECT
public:

    // Plain constructor.
    ImageButton(QWidget* parent = nullptr, const QSize& size = QSize());

    // Construct with a pair of images.
    // Args:
    //      normal_filename:
    //          displayed at rest
    //      pressed_filename:
    //          displayed while the user is pressing
    //      size:
    //          if specified, overrides the images' size
    ImageButton(const QString& normal_filename,
                const QString& pressed_filename,
                const QSize& size = QSize(),
                QWidget* parent = nullptr);

    // Construct with single image, making the "normal" and "pressed" images
    // from it. This is the default way that CamCOPS makes its buttons.
    // Args:
    //      base_filename:
    //          image filename
    //      filename_is_camcops_stem:
    //          treat the filename as the substrate for uifunc::iconFilename()
    //      alter_unpressed_image:
    //          add a standard "unpressed" background to the image (which
    //          makes it look more like a button than a flat image)?
    //      disabled:
    //          makes both images identical and in a "disabled" style, via
    //          uifunc::makeDisabledIcon().
    ImageButton(const QString& base_filename,
                bool filename_is_camcops_stem = true,
                bool alter_unpressed_image = true,
                bool disabled = false,
                QWidget* parent = nullptr);  // Default button maker

    // Set the unpressed and pressed images.
    // Args:
    //      base_filename:
    //          see above
    //      filename_is_camcops_stem:
    //          see above
    //      alter_unpressed_image:
    //          see above
    //      disabled:
    //          see above
    //      read_only:
    //          applicable if not disabled; sets the unpressed/pressed images
    //          to the base image, without modification
    void setImages(const QString& base_filename,
                   bool filename_is_camcops_stem = true,
                   bool alter_unpressed_image = true,
                   bool pressed_marker_behind = true,
                   bool disabled = false,
                   bool read_only = false);

    // Sets the "normal" ("unpressed") image from a filename.
    void setNormalImage(const QString& filename, const QSize& size = QSize(),
                        bool cache = true);

    // Sets the "normal" ("unpressed") image from a pixmap.
    void setNormalImage(const QPixmap& pixmap, bool scale = true);

    // Sets the "pressed" image from a filename.
    void setPressedImage(const QString& filename, const QSize& size = QSize(),
                         bool cache = true);

    // Sets the "pressed" image from a pixmal.
    void setPressedImage(const QPixmap& pixmap, bool scale = true);

    // Standard Qt widget override.
    virtual QSize sizeHint() const override;

    // Sets the overall size (optionally rescaling our images).
    void setImageSize(const QSize& size, bool scale = false);

    // Resizes/rescales our images by the specified factor.
    void resizeImages(double factor);

protected:
    // Standard Qt widget override.
    virtual void paintEvent(QPaintEvent* e) override;

    // Rescales a pixmap to m_image_size.
    void rescale(QPixmap& pm);

    // Resizes the widget if m_image_size doesn't have a size.
    void resizeIfNoSize();

protected:
    QPixmap m_normal_pixmap;  // our "normal" ("unpressed") image
    QPixmap m_pressed_pixmap;  // our "pressed" image
    QSize m_image_size;  // our image size
};
