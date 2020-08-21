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

// #define DEBUG_EVENTS

#include "sizewatcher.h"
#include <QDebug>
#include <QEvent>
#include <QResizeEvent>
#include <QWidget>


SizeWatcher::SizeWatcher(QObject* parent) :
    QObject(parent)  // owned by parent henceforth
{
    Q_ASSERT(parent);
    parent->installEventFilter(this);
}


bool SizeWatcher::eventFilter(QObject* obj, QEvent* event)
{
#ifdef DEBUG_EVENTS
    qDebug() << Q_FUNC_INFO << event;
#endif
    const QEvent::Type type = event->type();
    if (type == QEvent::Resize) {
        auto resize_event = static_cast<QResizeEvent*>(event);
        emit resized(resize_event->size());
    } else if (type == QEvent::Show) {
        auto w = dynamic_cast<QWidget*>(obj);
        emit shown(w ? w->size() : QSize());
    }
    return false;  // continue processing the event
}
