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


// #define DEBUG_LAYOUT

#include "basewidget.h"
#include <QDebug>
#include <QLayout>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOption>
#include "lib/sizehelpers.h"


BaseWidget::BaseWidget(QWidget* parent) :
    QWidget(parent)
{
    // As for LabelWordWrapWide:
#ifdef GUI_USE_RESIZE_FOR_HEIGHT
    setSizePolicy(sizehelpers::expandingFixedHFWPolicy());
#endif
}


BaseWidget::~BaseWidget()
{
    // qDebug() << Q_FUNC_INFO;
}


#ifdef GUI_USE_RESIZE_FOR_HEIGHT
void BaseWidget::resizeEvent(QResizeEvent* event)
{
#ifdef DEBUG_LAYOUT
    qDebug() << Q_FUNC_INFO << event->size();
#endif
    QWidget::resizeEvent(event);  // doesn't actually do anything
    sizehelpers::resizeEventForHFWParentWidget(this);
}
#endif


void BaseWidget::paintEvent(QPaintEvent*)
{
    // REQUIRED for class to support stylesheets
    // http://www.qtcentre.org/threads/37976-Q_OBJECT-and-CSS-background-image
    QStyleOption o;
    o.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);
}
