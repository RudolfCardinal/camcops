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

#include "imagebutton.h"
#include <QPainter>
#include "common/uiconst.h"
#include "lib/uifunc.h"


ImageButton::ImageButton(QWidget* parent, const QSize& size) :
    QPushButton(parent),
    m_image_size(size)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}


ImageButton::ImageButton(const QString& normal_filename,
                         const QString& pressed_filename,
                         const QSize& size,
                         QWidget* parent) :
    ImageButton(parent, size)  // delegating constructor
{
    setNormalImage(normal_filename, size);
    setPressedImage(pressed_filename, size);
    resizeIfNoSize();
}


ImageButton::ImageButton(const QString& base_filename,
                         const bool filename_is_camcops_stem,
                         const bool alter_unpressed_image,
                         const bool disabled,
                         QWidget* parent) :
    ImageButton(parent, uiconst::g_iconsize)  // delegating constructor
{
    setImages(base_filename, filename_is_camcops_stem, alter_unpressed_image,
              true, disabled);
    resizeIfNoSize();
}


void ImageButton::setImages(const QString& base_filename,
                            const bool filename_is_camcops_stem,
                            const bool alter_unpressed_image,
                            const bool pressed_marker_behind,
                            const bool disabled,
                            const bool read_only)
{
    // Old way: use two images
    // setNormalImage(UiFunc::iconPngFilename(stem), size);
    // setPressedImage(UiFunc::iconTouchedPngFilename(stem), scale);

    // New way: use one image and apply the background(s) programmatically
    const QString filename = filename_is_camcops_stem
            ? uifunc::iconFilename(base_filename)
            : base_filename;
    const QPixmap base = uifunc::getPixmap(filename, m_image_size);
    if (disabled) {
        const QPixmap img = uifunc::makeDisabledIcon(base);
        setNormalImage(img, false);
        setPressedImage(img, false);
    } else if (read_only) {
        setNormalImage(base, false);
        setPressedImage(base, false);
    } else {
        const QPixmap fore = alter_unpressed_image
                ? uifunc::addUnpressedBackground(base)
                : base;
        setNormalImage(fore, false);
        const QPixmap pressed = uifunc::addPressedBackground(
                    base, pressed_marker_behind);
        setPressedImage(pressed, false);
    }
    resizeIfNoSize();
}


void ImageButton::setNormalImage(const QString& filename, const QSize& size,
                                 const bool cache)
{
    setNormalImage(uifunc::getPixmap(filename, size, cache), false);
}


void ImageButton::setNormalImage(const QPixmap& pixmap, const bool scale)
{
    m_normal_pixmap = pixmap;
    if (scale) {
        rescale(m_normal_pixmap);
    }
    update();
}


void ImageButton::setPressedImage(const QString& filename, const QSize& size,
                                  const bool cache)
{
    setPressedImage(uifunc::getPixmap(filename, size, cache), false);
}


void ImageButton::setPressedImage(const QPixmap& pixmap, const bool scale)
{
    m_pressed_pixmap = pixmap;
    if (scale) {
        rescale(m_pressed_pixmap);
    }
    update();
}


void ImageButton::rescale(QPixmap& pm)
{
    pm = pm.scaled(m_image_size, Qt::IgnoreAspectRatio);
}


void ImageButton::resizeIfNoSize()
{
    if (m_image_size.isEmpty()) {
        m_image_size = m_normal_pixmap.size();
        updateGeometry();
    }
}


void ImageButton::resizeImages(const double factor)
{
    m_image_size = QSize(
        static_cast<int>(factor * static_cast<double>(m_normal_pixmap.size().width())),
        static_cast<int>(factor * static_cast<double>(m_normal_pixmap.size().height()))
    );
    rescale(m_normal_pixmap);
    rescale(m_pressed_pixmap);
    updateGeometry();
}


QSize ImageButton::sizeHint() const
{
    return m_image_size;
}


void ImageButton::setImageSize(const QSize &size, const bool scale)
{
    m_image_size = size;
    if (scale) {
        rescale(m_normal_pixmap);
        rescale(m_pressed_pixmap);
    }
    updateGeometry();
}


void ImageButton::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e)
    QPainter p(this);
    QPixmap& pm = isDown() ? m_pressed_pixmap : m_normal_pixmap;
    p.drawPixmap(0, 0, pm);
}
