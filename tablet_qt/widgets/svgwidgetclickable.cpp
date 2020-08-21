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

#include "svgwidgetclickable.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include "common/colourdefs.h"
#include "lib/uifunc.h"


SvgWidgetClickable::SvgWidgetClickable(QWidget* parent) :
    QSvgWidget(parent),
    m_background_colour(QCOLOR_TRANSPARENT),
    m_pressed_background_colour(QCOLOR_TRANSPARENT),
    m_pressed(false),
    m_pressing_inside(false)
{
    setTransparentForMouseEvents(false);
    uifunc::setBackgroundColour(this, QCOLOR_TRANSPARENT);
    setContentsMargins(0, 0, 0, 0);
}


SvgWidgetClickable::SvgWidgetClickable(const QString& filename,
                                       QWidget* parent) :
    SvgWidgetClickable(parent)  // delegating constructor
{
    load(filename);
}


void SvgWidgetClickable::setSvgFromString(const QString& svg)
{
    load(svg.toUtf8());
}


void SvgWidgetClickable::setSvgFromFile(const QString& filename)
{
    load(filename);
}


void SvgWidgetClickable::setBackgroundColour(const QColor& colour)
{
    m_background_colour = colour;
    update();
}


void SvgWidgetClickable::setPressedBackgroundColour(const QColor& colour)
{
    m_pressed_background_colour = colour;
    update();
}


void SvgWidgetClickable::setTransparentForMouseEvents(const bool transparent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, transparent);
    // only applies in QWidget mode, not when it's a QGraphicsItem
}


void SvgWidgetClickable::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    m_pressed = true;
    m_pressing_inside = true;
    emit pressed();
    update();
}


void SvgWidgetClickable::mouseMoveEvent(QMouseEvent* event)
{
    if (m_pressed) {
        const bool was_pressing_inside = m_pressing_inside;
        m_pressing_inside = contentsRect().contains(event->pos());
        if (m_pressing_inside != was_pressing_inside) {
            update();
        }
    }
}


void SvgWidgetClickable::mouseReleaseEvent(QMouseEvent* event)
{
    m_pressed = false;
    if (contentsRect().contains(event->pos())) {
        // release occurred inside widget
        emit clicked();
    }
    update();
}


void SvgWidgetClickable::paintEvent(QPaintEvent* event)
{
    {
        const QColor& bg = m_pressed && m_pressing_inside
                ? m_pressed_background_colour
                : m_background_colour;
        QPainter p(this);
        p.setPen(QPen(Qt::PenStyle::NoPen));
        p.setBrush(QBrush(bg));
        const QRect cr = contentsRect();
        p.drawRect(cr);
    }

    QSvgWidget::paintEvent(event);
}
