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

// #define DEBUG_GUI_GUARD

#include "slowguiguard.h"
#include <QApplication>
#ifdef DEBUG_GUI_GUARD
#include <QDebug>
#endif
#include <QWidget>
#include "dialogs/waitbox.h"


bool SlowGuiGuard::s_waiting = false;


#ifdef _MSC_VER  // Compiling under Microsoft Visual C++
// Fix a Visual C++ warning bug (Visual Studio 14 version).
// 'app' is clearly used, but the warning appears nonetheless.
#pragma warning(push)
#pragma warning(disable: 4100)  // C4100: 'app': unreferenced formal parameter
#endif

SlowGuiGuard::SlowGuiGuard(QApplication& app,
                           QWidget* parent,
                           const QString& text,
                           const QString& title,
                           const int minimum_duration_ms) :
    m_wait_box(nullptr)
{
    if (!s_waiting) {
#ifdef DEBUG_GUI_GUARD
        qDebug() << Q_FUNC_INFO << "Making wait box";
#endif
        s_waiting = true;
        m_wait_box = new WaitBox(parent, text, title, minimum_duration_ms);
        m_wait_box->show();
    } else {
#ifdef DEBUG_GUI_GUARD
        qDebug() << Q_FUNC_INFO
                 << "Not making another wait box; one is already open";
#endif
    }
    app.processEvents();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif


SlowGuiGuard::~SlowGuiGuard()
{
#ifdef DEBUG_GUI_GUARD
    if (s_waiting) {
        qDebug() << Q_FUNC_INFO << "Closing wait box";
    }
#endif
    delete m_wait_box;
    s_waiting = false;
}
