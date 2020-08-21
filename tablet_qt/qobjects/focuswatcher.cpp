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

// #define DEBUG_FOCUS

#include "focuswatcher.h"
#include <QDebug>
#include <QEvent>


FocusWatcher::FocusWatcher(QObject* parent) :
    QObject(parent)  // owned by parent henceforth
{
    Q_ASSERT(parent);
    parent->installEventFilter(this);
}


bool FocusWatcher::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj)
    const QEvent::Type type = event->type();
    if (type == QEvent::FocusIn) {

#ifdef DEBUG_FOCUS
        qDebug() << obj->objectName() << "FocusIn";
#endif
        emit focusChanged(true);

    } else if (type == QEvent::FocusOut) {

#ifdef DEBUG_FOCUS
        qDebug() << obj->objectName() << "FocusOut";
#endif
        emit focusChanged(false);

    }
    return false;  // continue processing the event
}
