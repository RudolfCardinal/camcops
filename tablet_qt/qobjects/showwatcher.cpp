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

#include "showwatcher.h"
#include <QEvent>
#include "lib/layoutdumper.h"


ShowWatcher::ShowWatcher(QObject* parent, const bool debug_layout) :
    QObject(parent),  // owned by parent henceforth
    m_debug_layout(debug_layout)
{
    Q_ASSERT(parent);
    parent->installEventFilter(this);
}


bool ShowWatcher::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj)
    if (event->type() == QEvent::Show) {
        emit showing();
        if (m_debug_layout) {
            auto w = dynamic_cast<QWidget*>(obj);
            if (w) {
                layoutdumper::dumpWidgetHierarchy(w);
            }
        }
    }
    return false;  // continue processing the event
}
