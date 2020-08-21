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

#include "logbox.h"
#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "common/textconst.h"
#include "lib/uifunc.h"

const QSize MIN_SIZE(600, 600);

LogBox::LogBox(QWidget* parent,
               const QString& title,
               const bool offer_cancel,
               const bool offer_ok_at_end,
               const int maximum_block_count,
               const bool scroll_to_end_on_insert,
               const bool word_wrap) :
    QDialog(parent),
    m_use_wait_cursor(true),
    m_editor(nullptr),
    m_ok(nullptr),
    m_cancel(nullptr),
    m_ack_fail(nullptr),
    m_wait_cursor_on(false),
    m_scroll_to_end_on_insert(scroll_to_end_on_insert)
{
    // qDebug() << Q_FUNC_INFO;
    setWindowTitle(title);
    setMinimumSize(MIN_SIZE);

    auto mainlayout = new QVBoxLayout();
    setLayout(mainlayout);

    m_editor = new QPlainTextEdit();
    // QPlainTextEdit better than QTextEdit because it supports
    // maximumBlockCount while still allowing HTML (via appendHtml,
    // not insertHtml).
    m_editor->setReadOnly(true);
    m_editor->setTextInteractionFlags(Qt::NoTextInteraction);
    m_editor->setLineWrapMode(word_wrap ? QPlainTextEdit::WidgetWidth
                                        : QPlainTextEdit::NoWrap);
    m_editor->setMaximumBlockCount(maximum_block_count);
    mainlayout->addWidget(m_editor);
    uifunc::applyScrollGestures(m_editor->viewport());
    // ... https://stackoverflow.com/questions/23675845/qt-conflict-between-qscroller-and-qscrollbar

    auto buttonlayout = new QHBoxLayout();
    auto copybutton = new QPushButton(TextConst::copy());
    buttonlayout->addWidget(copybutton);
    connect(copybutton, &QPushButton::clicked, this, &LogBox::copyClicked);

    if (offer_cancel) {
        m_cancel = new QPushButton(TextConst::cancel());
        buttonlayout->addWidget(m_cancel);
        connect(m_cancel, &QPushButton::clicked, this, &LogBox::reject);
    }

    buttonlayout->addStretch();
    // Don't have cancel button on the right (user might hit it thinking
    // it's the OK button, based on shared location).

    if (offer_ok_at_end) {
        m_ok = new QPushButton(TextConst::ok());
        buttonlayout->addWidget(m_ok);
        connect(m_ok, &QPushButton::clicked, this, &LogBox::okClicked);
        m_ok->hide();
    }

    m_ack_fail = new QPushButton(tr("Acknowledge failure"));
    buttonlayout->addWidget(m_ack_fail);
    connect(m_ack_fail, &QPushButton::clicked, this, &LogBox::okClicked);
    m_ack_fail->hide();

    mainlayout->addLayout(buttonlayout);
}


LogBox::~LogBox()
{
    if (m_wait_cursor_on) {
        QApplication::restoreOverrideCursor();
    }
}


void LogBox::useWaitCursor(bool use_wait_cursor)
{
    m_use_wait_cursor = use_wait_cursor;
}


void LogBox::open()
{
    // qDebug() << Q_FUNC_INFO;
    if (m_use_wait_cursor) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        m_wait_cursor_on = true;
    }
    QDialog::open();
}


void LogBox::statusMessage(const QString& msg, const bool as_html)
{
    if (!m_editor) {
        return;
    }
    if (as_html) {
        m_editor->appendHtml(msg);
    } else {
        m_editor->appendPlainText(msg);
    }
    if (m_scroll_to_end_on_insert) {
        uifunc::scrollToEnd(m_editor.data());
    }
}


void LogBox::finish(const bool success)
{
    // qDebug() << Q_FUNC_INFO;
    // If we're waiting for the user to press OK (so they can look at the log,
    // enable the OK button and await the accepted() signal via that button).
    // Otherwise, accept() now. Either way, restore the cursor.
    QApplication::restoreOverrideCursor();
    m_wait_cursor_on = false;
    if (m_cancel) {
        m_cancel->hide();
    }
    if (success && m_ok) {
        m_ok->show();  // and await the accepted() signal via the button
    } else if (!success) {
        m_ack_fail->show();  // and await the accepted() signal via the button
    } else {
        // success, but caller didn't want an OK button
        accept();  // will emit accepted()
    }
}


void LogBox::okClicked()
{
    // qDebug() << Q_FUNC_INFO;
    accept();
    hide();  // hide explicitly, as we may be called using open() not exec()
}


void LogBox::copyClicked()
{
    m_editor->selectAll();
    m_editor->copy();
    m_editor->moveCursor(QTextCursor::End);
    uifunc::scrollToEnd(m_editor.data());
}
