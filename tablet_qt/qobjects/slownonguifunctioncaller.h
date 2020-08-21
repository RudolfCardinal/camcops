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
#include <QThread>
#include "common/textconst.h"
#include "dialogs/waitbox.h"
#include "qobjects/threadworker.h"


class SlowNonGuiFunctionCaller : public QObject
{
    // Executes a function in blocking fashion, but in a separate thread,
    // while displaying an infinite-wait (uncertain-wait) progress dialogue
    // from the calling thread. (This class creates and manages the worker
    // thread.)
    //
    // Must be created from the GUI thread.
    //
    // When the constructor has finished, the work is done.
    //
    // DO NOT PERFORM GUI OPERATIONS IN THE WORKER FUNCTION.

    Q_OBJECT
public:
    SlowNonGuiFunctionCaller(const ThreadWorker::PlainWorkerFunction& func,
                             QWidget* parent,
                             const QString& text = "Operation in progress...",
                             const QString& title = TextConst::pleaseWait());
    ~SlowNonGuiFunctionCaller();
protected:
    QThread m_worker_thread;
    ThreadWorker* m_worker;
    WaitBox m_waitbox;
};
