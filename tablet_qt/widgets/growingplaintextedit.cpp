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

// See growingtextedit.cpp for notes
// This is to PlainTextEdit as GrowingTextEdit is to TextEdit.

#include "growingplaintextedit.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>


GrowingPlainTextEdit::GrowingPlainTextEdit(QWidget* parent) :
    QPlainTextEdit(parent),
    m_auto_resize(true)
{
    connect(document(), &QTextDocument::contentsChanged,
            this, &GrowingPlainTextEdit::contentsChanged);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}


GrowingPlainTextEdit::GrowingPlainTextEdit(const QString& text,
                                           QWidget* parent) :
    GrowingPlainTextEdit(parent)  // delegating constructor
{
    setPlainText(text);
}


GrowingPlainTextEdit::~GrowingPlainTextEdit()
{
}


void GrowingPlainTextEdit::setAutoResize(const bool auto_resize)
{
    m_auto_resize = auto_resize;
}


QSize GrowingPlainTextEdit::sizeHint() const
{
    ensurePolished();
    QSize size_hint;
    if (m_auto_resize) {
        // https://stackoverflow.com/questions/9506586/qtextedit-resize-to-fit
        // ... nope, not too good.
        // 1. WRONG:
        // size_hint = QPlainTextEdit::sizeHint();
        // 2. WRONG:
        size_hint = document()->size().toSize();
        // 3. WRONG:
        // const QTextDocument* doc = document();
        // const QFont font = doc->defaultFont();
        // const QFontMetrics fm(font);
        // const QString text = doc->toPlainText() + "\n";  // or without "\n"
        // size_hint = fm.size(0, text);
        // 4. WRONG:
        // ... as above but
        // size_hint = fm.size(Qt::TextWordWrap, text);
        // 5. CLOSE, BUT WRONG:
        // const QTextDocument* doc = document();
        // const QFont font = doc->defaultFont();
        // const QFontMetrics fm(font);
        // const QString text = doc->toPlainText();
        // const QRect br = fm.boundingRect(
        //             QRect(0, 0, QWIDGETSIZE_MAX, QWIDGETSIZE_MAX),
        //             0,  // definitely not Qt::TextWordWrap
        //             text);
        // size_hint = br.size();
    } else {
        size_hint = QPlainTextEdit::sizeHint();
    }
    qDebug() << Q_FUNC_INFO << size_hint;
    return size_hint;
}


QSize GrowingPlainTextEdit::minimumSizeHint() const
{
    QSize minsize = QPlainTextEdit::minimumSizeHint();
    minsize.setHeight(sizeHint().height());
    return minsize;
}


void GrowingPlainTextEdit::contentsChanged()
{
    document()->setTextWidth(viewport()->width());
    updateGeometry();
}
