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

#include "fixedareahfwtestwidget.h"
#include <cmath>
#include <QBrush>
#include <QPainter>
#include <QPen>
#include "lib/sizehelpers.h"


FixedAreaHfwTestWidget::FixedAreaHfwTestWidget(const int area,
                                               const int preferred_width,
                                               const QSize& min_size,
                                               const QColor& background_colour,
                                               const int border_thickness,
                                               const QColor& border_colour,
                                               const QColor& text_colour,
                                               QWidget* parent) :
    QWidget(parent),
    m_area(area),
    m_preferred_width(preferred_width),
    m_min_size(min_size),
    m_background_colour(background_colour),
    m_border_thickness(border_thickness),
    m_border_colour(border_colour),
    m_text_colour(text_colour),
    m_min_area(INT_MAX),
    m_max_area(-1)
{
    setSizePolicy(sizehelpers::preferredPreferredHFWPolicy());
}


QSize FixedAreaHfwTestWidget::sizeHint() const
{
    return QSize(m_preferred_width, heightForWidth(m_preferred_width));
}


QSize FixedAreaHfwTestWidget::minimumSizeHint() const
{
    return m_min_size;
}


bool FixedAreaHfwTestWidget::hasHeightForWidth() const
{
    return true;
}


int FixedAreaHfwTestWidget::heightForWidth(const int width) const
{
    return qRound(static_cast<double>(m_area) / static_cast<double>(width));
    // http://stackoverflow.com/questions/13663545/does-one-double-promote-every-int-in-the-equation-to-double
    // http://stackoverflow.com/questions/5563000/implicit-type-conversion-rules-in-c-operators
}


void FixedAreaHfwTestWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    const QSize s = size();
    QRectF rect(QPoint(0, 0), s);
    const qreal penwidth = m_border_thickness;
    const qreal halfpen = penwidth / 2;
    rect.adjust(halfpen, halfpen, -halfpen, -halfpen);

    const int w = s.width();
    const int h = s.height();
    const int a = w * h;
    const int hfw = heightForWidth(w);
    const QString hfw_description = hfw == h
        ? "matches HFW"
        : QString("MISMATCH to HFW %1").arg(hfw);
    m_min_area = qMin(m_min_area, a);
    m_max_area = qMax(m_max_area, a);
    const QString description = QString(
        "Fixed area; %1 w x %2 h (%3) = area %4 [range %5-%6]")
            .arg(w)
            .arg(h)
            .arg(hfw_description)
            .arg(a)
            .arg(m_min_area)
            .arg(m_max_area);

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
