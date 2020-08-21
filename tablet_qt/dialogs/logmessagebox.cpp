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

#include "logmessagebox.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "common/textconst.h"
#include "lib/uifunc.h"

const int MIN_WIDTH = 600;
const int MIN_HEIGHT = 600;


LogMessageBox::LogMessageBox(QWidget* parent,
                             const QString& title,
                             const QString& text,
                             const bool as_html,
                             const bool word_wrap) :
    QDialog(parent)
{
    setWindowTitle(title);
    setMinimumWidth(MIN_WIDTH);
    setMinimumHeight(MIN_HEIGHT);

    auto mainlayout = new QVBoxLayout();
    setLayout(mainlayout);

    m_editor = new QPlainTextEdit();
    m_editor->setReadOnly(true);
    m_editor->setTextInteractionFlags(Qt::NoTextInteraction);
    m_editor->setLineWrapMode(word_wrap ? QPlainTextEdit::WidgetWidth
                                        : QPlainTextEdit::NoWrap);
    mainlayout->addWidget(m_editor);
    uifunc::applyScrollGestures(m_editor->viewport());

    if (as_html) {
        m_editor->appendHtml(text);
    } else {
        m_editor->appendPlainText(text);
    }

    auto buttonlayout = new QHBoxLayout();
    auto copybutton = new QPushButton(TextConst::copy());
    buttonlayout->addWidget(copybutton);
    connect(copybutton, &QPushButton::clicked, this, &LogMessageBox::copyClicked);

    buttonlayout->addStretch();

    auto okbutton = new QPushButton(TextConst::ok());
    buttonlayout->addWidget(okbutton);
    connect(okbutton, &QPushButton::clicked, this, &LogMessageBox::accept);

    mainlayout->addLayout(buttonlayout);

    uifunc::scrollToStart(m_editor.data());  // NOT WORKING. And exec() isn't virtual.
}


void LogMessageBox::copyClicked()
{
    m_editor->selectAll();
    m_editor->copy();
    m_editor->moveCursor(QTextCursor::End);
    uifunc::scrollToEnd(m_editor.data());
}
