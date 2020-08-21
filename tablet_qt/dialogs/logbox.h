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
#include <QDialog>
#include <QPointer>
class QPlainTextEdit;
class QPushButton;


class LogBox : public QDialog
{
    // Modal (but NON-BLOCKING) dialogue with a textual log window, used for
    // displaying progress, e.g. during network operations (see
    // NetworkManager).
    //
    // Compare LogMessageBox for a modal and blocking version.

    Q_OBJECT
public:
    // Constructor
    LogBox(QWidget* parent, const QString& title, bool offer_cancel = true,
           bool offer_ok_at_end = true, int maximum_block_count = 1000,
           bool scroll_to_end_on_insert = true, bool word_wrap = true);

    // Destructor
    ~LogBox() override;

    // Choose whether a wait cursor is shown
    void useWaitCursor(bool use_wait_cursor = true);

    // Write a message to the log
    void statusMessage(const QString& msg, bool as_html = false);

    // Finish (with success or failure)
    void finish(bool success = true);

public slots:
    virtual void open() override;
    void okClicked();
    void copyClicked();

signals:
    void cancelled();
    void finished();

protected:
    bool m_use_wait_cursor;
    QPointer<QPlainTextEdit> m_editor;
    QPointer<QPushButton> m_ok;
    QPointer<QPushButton> m_cancel;
    QPointer<QPushButton> m_ack_fail;
    bool m_wait_cursor_on;
    bool m_scroll_to_end_on_insert;
};
