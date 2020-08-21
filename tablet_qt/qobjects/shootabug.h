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

#pragma once
#include <QObject>

/*

See
    - http://www.kdab.com/~volker/devdays/2011/EffectiveDebuggingAndProfilingForQtAndQtQuick.pdf
    - http://www.vikingsoft.eu/blog/?p=8

Install with
    qApp->installEventFilter(new ShootABug());

That should filter events for the entire application:
    http://doc.qt.io/qt-5.7/eventsandfilters.html
    ... "such global event filters are called before the object-specific
    filters"

When a widget is CTRL-clicked, it should report details of itself to the
console.

HOWEVER, what I'm getting is that every click is being reported as coming
from widget name QMainWindowClassWindow, widget class QWidgetWindow

*/

class ShootABug : public QObject
{
    // Object that can report debugging information, given a debug build of Qt.

    Q_OBJECT
public:
    bool eventFilter(QObject* recv, QEvent* event);
};
