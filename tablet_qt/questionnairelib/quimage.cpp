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

// #define DEBUG_SIZE

#include "quimage.h"
#include "lib/convert.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/aspectratiopixmap.h"


QuImage::QuImage(const QString& filename, FieldRefPtr fieldref,
                 const QSize& size) :
    m_filename(filename),
    m_fieldref(fieldref),
    m_label(nullptr),
    m_size(size),
    m_adjust_for_dpi(true),
    m_allow_shrink(true)
{
    if (fieldref) {
        connect(m_fieldref.data(), &FieldRef::valueChanged,
                this, &QuImage::valueChanged);
    }
}


QuImage::QuImage(const QString& filename, const QSize& size) :
    QuImage(filename, nullptr, size)  // delegating constructor
{
}


QuImage::QuImage(FieldRefPtr fieldref, const QSize& size) :
    QuImage("", fieldref, size)  // delegating constructor
{
    Q_ASSERT(m_fieldref);
}


QuImage* QuImage::setAdjustForDpi(const bool adjust_for_dpi)
{
    m_adjust_for_dpi = adjust_for_dpi;
    return this;
}


QuImage* QuImage::setSize(const QSize& size)
{
    m_size = size;
    return this;
}


QuImage* QuImage::setAllowShrink(const bool allow_shrink)
{
    m_allow_shrink = allow_shrink;
    return this;
}


QPointer<QWidget> QuImage::makeWidget(Questionnaire* questionnaire)
{
    Q_UNUSED(questionnaire)
    const QPixmap image = getScaledImage();
    m_label = new AspectRatioPixmap();
    if (!m_allow_shrink) {
        m_label->setFixedSize(image.size());
    }
    m_label->setPixmap(image);
    return QPointer<QWidget>(m_label);
}


QPixmap QuImage::getScaledImage(const FieldRef* fieldref) const
{
    // Fetch image
    QPixmap image;
    const FieldRef* fieldref_to_use = fieldref
            ? fieldref
            : (m_fieldref && m_fieldref->valid() ? m_fieldref.data() : nullptr);
    if (fieldref_to_use) {
        image = fieldref_to_use->pixmap();
    } else {
        image = uifunc::getPixmap(m_filename);
    }

    // Set size: (a) image size or m_size override; (b) +/- scale for DPI
#ifdef DEBUG_SIZE
    qDebug() << Q_FUNC_INFO << "Initial image size:" << image.size();
#endif
    QSize size = m_size.isValid() ? m_size : image.size();
    if (m_adjust_for_dpi) {
        size = dpiScaledSize(size);
    }

    // Scale image if required
    if (size != image.size()) {
        image = image.scaled(size);
    }
#ifdef DEBUG_SIZE
    qDebug().nospace()
            << Q_FUNC_INFO << " Final size (after m_size=" << m_size
            << ", m_adjust_for_dpi=" << m_adjust_for_dpi << "): " << size;
#endif
    return image;
}


void QuImage::valueChanged(const FieldRef* fieldref)
{
    if (!m_label) {
        return;
    }
    m_label->setPixmap(getScaledImage(fieldref));
}


QSize QuImage::dpiScaledSize(const QSize& size) const
{
    return convert::convertSizeByLogicalDpi(size);
}
