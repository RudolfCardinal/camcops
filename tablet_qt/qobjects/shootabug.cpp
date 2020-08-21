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

#include "shootabug.h"
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include "lib/debugfunc.h"


bool ShootABug::eventFilter(QObject* recv, QEvent* event)
{
    if (event->type() != QEvent::MouseButtonPress) {
        return false;  // pass it on
    }
    auto mevent = static_cast<QMouseEvent*>(event);
    if ((mevent->modifiers() & Qt::ControlModifier) &&
            (mevent->button() & Qt::LeftButton)) {  // Ctrl + left mouse click.
        debugfunc::dumpQObject(recv);
        // Return false, if you want the application to receive the event;
        // return true to block.
        return true;  // Block
    }
    return false;
}
