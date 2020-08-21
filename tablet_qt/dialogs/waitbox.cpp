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

#include "waitbox.h"
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QThread>
#include "lib/uifunc.h"

/*

  - Wait cursor:
    http://stackoverflow.com/questions/13495283/change-cursor-to-hourglass-wait-busy-cursor-and-back-in-qt

  - Doing something and showing a wait indicator:

    - All Qt UI elements must be created in the GUI thread.
        http://doc.qt.io/qt-5/thread-basics.html#gui-thread-and-worker-thread

    - So the wait box must be run from the main thread.

    - A QProgressDialog is a bit unreliable; it seems to require an uncertain
      number of calls to setValue(), even with setMinimumDuration(0), before
      it's fully painted. If you create it and give a single call (or 5, or 10)
      to setValue(), you can get just part of the dialog painted.

      Looks nice, though, with min = max = 0 for an "infinite wait" bar.

    - So, better would be a different QDialog?
      ... No, that too fails to be painted properly.

    - Therefore, threads:
      (1) Start on GUI thread.
          - GUI thread starts worker thread (2).
          - GUI thread opens progress dialog modally, and sits in its exec()
            loop, thus processing events but blocking from the point of view
            of the calling code.
          - GUI thread returns when signalled.
      (2) Worker thread starts, taking callback as argument.
          - Worker thread does work.
          - Worker thread signals GUI thread when done.

    - OK! That's great for non-GUI work.

    - Others' thoughts (for non-GUI work), using QtConcurrent:
      http://stackoverflow.com/questions/22670564/reliably-showing-a-please-wait-dialog-while-doing-a-lengthy-blocking-operation

    - Any way to pop up a wait dialogue when we're waiting for a slow GUI
      operation? That's less obvious...
      Achieved pretty well using SlowGuiGard class; q.v.


*/


WaitBox::WaitBox(QWidget* parent, const QString& text, const QString& title,
                 const int minimum_duration_ms) :
    QProgressDialog(text, "", 0, 0, parent)
{
    // if min = max = 0, you get an infinite wait bar.

    // qDebug() << Q_FUNC_INFO;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setWindowTitle(title);
    setMinimumSize(uifunc::minimumSizeForTitle(this));

    // Prevent user interaction with what's behind:
    setWindowModality(Qt::WindowModal);

    // Remove the cancel button:
    setCancelButton(nullptr);

    // Prevent the user from closing via the close button:
    // - https://stackoverflow.com/questions/16920412/qprogressdialog-without-close-button
    // - PLAY WITH THE QT EXAMPLE in qt5/qtbase/tests/manual/windowflags
    // - Under Linux/XFCE, it seems that you have to have FramelessWindowHint
    //   set in order to remove the "close" button.
    // - Ah, no! You have to have CustomizeWindowHint set to manipulate the
    //   individual properties. We'd like a title, too.
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // Without the setMinimumDuration() call, you never see the dialog.
    setMinimumDuration(minimum_duration_ms);
}


WaitBox::~WaitBox()
{
    // qDebug() << Q_FUNC_INFO;
    QApplication::restoreOverrideCursor();
    // qDebug() << Q_FUNC_INFO << "done";
}


void WaitBox::keyPressEvent(QKeyEvent* event)
{
    // Ignore the Escape key
    if (event->key() != Qt::Key_Escape) {
        QProgressDialog::keyPressEvent(event);
    }
}
