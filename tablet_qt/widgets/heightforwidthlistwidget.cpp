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

#include "heightforwidthlistwidget.h"
#include <QDebug>
#include <QEvent>
#ifdef DEBUG_LAYOUT
#include "lib/layoutdumper.h"
#endif


HeightForWidthListWidget::HeightForWidthListWidget(QWidget* parent) :
    QListWidget(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // ... disable horizontal scroll bar (on small screens, we will word-wrap)
    setResizeMode(QListView::Adjust);
}


bool HeightForWidthListWidget::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::Resize:  // Alternative is resizeEvent() but we need to trap other events too
    case QEvent::LayoutRequest:  // See QWidget::updateGeometry() -- they will come here
        {
            // At this point, we have already been resized, so can use width();
            // http://doc.qt.io/qt-4.8/qwidget.html#resizeEvent
            const int n_items = count();
            for (int row = 0; row < n_items; ++row) {
                QListWidgetItem* lwi = item(row);
                if (!lwi) {
                    qWarning() << Q_FUNC_INFO << "null item()";
                    continue;
                }
                QWidget* widget = itemWidget(lwi);
                if (!widget) {
                    qWarning() << Q_FUNC_INFO << "null itemWidget()";
                    continue;
                }
                const QSize size_hint = widgetSizeHint(widget);
                lwi->setSizeHint(size_hint);
#ifdef DEBUG_LAYOUT
                qDebug() << Q_FUNC_INFO << "resizing list widget to" << size()
                         << "; setting QListWidgetItem sizeHint for widget"
                         << LayoutDumper::getWidgetDescriptor(widget) << "to"
                         << size_hint;
#endif
            }
        }
        break;
    default:
        break;
    }
    return QListWidget::event(e);
}


QSize HeightForWidthListWidget::widgetSizeHint(QWidget* widget) const
{
    if (!widget) {
        return QSize();
    }
    if (!widget->hasHeightForWidth()) {
        return widget->sizeHint();
    }
    const QRect list_contents_rect = contentsRect();
    const int list_width = list_contents_rect.width();
    const QSize widget_size_hint = widget->sizeHint();
    // Default implementation, QWidget::sizeHint, returns
    // its layout's totalSizeHint(), or an invalid size.

    const int widget_preferred_width = widget_size_hint.width();
    const int widget_new_width = qMin(widget_preferred_width, list_width);
    const int widget_new_height = widget->heightForWidth(widget_new_width);
    QSize result(widget_new_width, widget_new_height);
#ifdef DEBUG_LAYOUT
    qDebug() << Q_FUNC_INFO << "widget"
             << LayoutDumper::getWidgetDescriptor(widget)
             << "widget_size_hint" << widget_size_hint
             << "result" << result;
#endif
    return result;
}

// This was not perfect; menu item heights swap between e.g. 66 and 70
// pixels as you create the top menu and refresh the list (e.g. by clicking
// the lock button)... fixed by catching QEvent::LayoutRequest as well as
// QEvent::Resize.
