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

#include "slownonguifunctioncaller.h"
#include "dialogs/waitbox.h"


SlowNonGuiFunctionCaller::SlowNonGuiFunctionCaller(
        const ThreadWorker::PlainWorkerFunction& func,
        QWidget* parent,
        const QString& text,
        const QString& title) :
    m_worker(new ThreadWorker(func)),
    m_waitbox(parent, text, title)
{
    m_worker->moveToThread(&m_worker_thread);
    connect(&m_worker_thread, &QThread::started, // 1
            m_worker, &ThreadWorker::work);  // 2
    connect(m_worker, &ThreadWorker::workComplete, // 3
            &m_waitbox, &WaitBox::accept);  // 4
    connect(&m_worker_thread, &QThread::finished,  // 5
            m_worker, &QObject::deleteLater);  // 6
    m_worker_thread.start();
    m_waitbox.exec();  // blocks until its accept()/reject() called (e.g. 4)
}


SlowNonGuiFunctionCaller::~SlowNonGuiFunctionCaller()
{
    m_worker_thread.quit();
    m_worker_thread.wait();
}
