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

#include "keypresswatcher.h"
#include <QDialog>
#include <QKeyEvent>
#include <QObject>


KeyPressWatcher::KeyPressWatcher(QDialog* parent) :
    QObject(parent)
{
    if (parent) {
        parent->installEventFilter(this);
    }
}


bool KeyPressWatcher::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj)
    if (event->type() == QEvent::KeyPress) {
        auto key_event = static_cast<QKeyEvent*>(event);
        int key = key_event->key();
        emit keypress(key);
        if (m_map.contains(key)) {
            const CallbackFunction& func = m_map[key];
            func();
        }
    }
    return false;  // continue processing the event
}


void KeyPressWatcher::addKeyEvent(const int key,
                                  const CallbackFunction& callback)
{
    m_map[key] = callback;
}
