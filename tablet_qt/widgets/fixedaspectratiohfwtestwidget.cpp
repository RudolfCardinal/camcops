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

#include "fixedaspectratiohfwtestwidget.h"
#include <cmath>
#include <QBrush>
#include <QPainter>
#include <QPen>
#include "lib/sizehelpers.h"


FixedAspectRatioHfwTestWidget::FixedAspectRatioHfwTestWidget(
        const qreal aspect_ratio,
        const int preferred_width,
        const QSize& min_size,
        const QColor& background_colour,
        const int border_thickness,
        const QColor& border_colour,
        const QColor& text_colour,
        QWidget* parent) :
    QWidget(parent),
    m_aspect_ratio(aspect_ratio),
    m_preferred_width(preferred_width),
    m_min_size(min_size),
    m_background_colour(background_colour),
    m_border_thickness(border_thickness),
    m_border_colour(border_colour),
    m_text_colour(text_colour)
{
    setSizePolicy(sizehelpers::expandingFixedHFWPolicy());
}


QSize FixedAspectRatioHfwTestWidget::sizeHint() const
{
    return QSize(m_preferred_width, heightForWidth(m_preferred_width));
}


QSize FixedAspectRatioHfwTestWidget::minimumSizeHint() const
{
    return m_min_size;
}


bool FixedAspectRatioHfwTestWidget::hasHeightForWidth() const
{
    return true;
}


int FixedAspectRatioHfwTestWidget::heightForWidth(const int width) const
{
    //      aspect_ratio = width / height
    // =>   height = width / aspect_ratio
    return qRound(static_cast<qreal>(width) / m_aspect_ratio);
}


void FixedAspectRatioHfwTestWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    const QSize s = size();
    QRectF rect(QPoint(0, 0), s);
    const qreal penwidth = m_border_thickness;
    const qreal halfpen = penwidth / 2;
    rect.adjust(halfpen, halfpen, -halfpen, -halfpen);

    const int w = s.width();
    const int h = s.height();
    const int hfw = heightForWidth(w);
    const QString hfw_description = hfw == h
        ? "matches HFW"
        : QString("MISMATCH to HFW %1").arg(hfw);
    const QString description = QString(
        "Fixed aspect ratio; %1 w x %2 h (%3)")
            .arg(w)
            .arg(h)
            .arg(hfw_description);

    QPen border_pen(m_border_colour);
    border_pen.setWidth(m_border_thickness);
    const QPen text_pen(m_text_colour);
    const QBrush brush(m_background_colour, Qt::SolidPattern);

    QPainter painter(this);
    painter.setPen(border_pen);
    painter.setBrush(brush);
    painter.drawRect(rect);
    painter.setPen(text_pen);
    painter.drawText(
        rect,
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        description
    );
}
