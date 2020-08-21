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

#include "verticalscrollareaviewport.h"
#include <QDebug>
#include <QResizeEvent>
#include "lib/layoutdumper.h"
#include "lib/sizehelpers.h"
#include "widgets/basewidget.h"


VerticalScrollAreaViewport::VerticalScrollAreaViewport(QWidget* parent) :
    QWidget(parent)
{
}


void VerticalScrollAreaViewport::checkChildSize() const
{
    if (layout()) {
        qWarning() << Q_FUNC_INFO << "- shouldn't have a layout!";
        return;
    }
    const QObjectList& children_list = children();
    if (children_list.length() == 0) {
        qWarning() << Q_FUNC_INFO << "- no children!";
        return;
    }
    if (children_list.length() > 1) {
        qWarning() << Q_FUNC_INFO << "- multiple children!";
        return;
    }
    QObject* child_object = children_list.at(0);
    auto child = dynamic_cast<QWidget*>(child_object);
    if (!child) {
        qWarning() << Q_FUNC_INFO << "- child is not a QWidget!";
        return;
    }

    const QSize our_size = size();
#ifdef DEBUG_LAYOUT
    qDebug() << Q_FUNC_INFO << "our_size:" << our_size;
#endif

    QSize desired_child_size;
    if (child->hasHeightForWidth()) {
        desired_child_size = our_size;
        desired_child_size.rheight() = child->heightForWidth(our_size.width());
    } else {
        desired_child_size = child->sizeHint();
    }
#ifdef DEBUG_LAYOUT
    qDebug() << Q_FUNC_INFO << "Child widget:"
             << layoutdumper::getWidgetInfo(child);
    qDebug() << Q_FUNC_INFO << "desired_child_size:" << desired_child_size;
#endif

    // Don't try setting the child size here.
    // BoxLayoutHfw and GridLayoutHfw, from their setGeometry(), call
    // their parent->setFixedHeight() and parent->updateGeometry(). So anything
    // we do here to alter the child size is just overridden again, I think.

#ifdef DEBUG_LAYOUT
    qDebug() << Q_FUNC_INFO << "(3) Child widget after:"
             << layoutdumper::getWidgetInfo(child);
#endif
    const QSize child_size = child->size();
    if (child_size != desired_child_size) {
        qWarning()
                << Q_FUNC_INFO
                << "... child size problem! We expected" << desired_child_size
                << "and got" << child_size;
        if (child_size.height() > desired_child_size.height()) {
            qDebug() << Q_FUNC_INFO << "An unnecessary scroll bar is likely.";
        }
    }
}
