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

#include "growingtextedit.h"


GrowingTextEdit::GrowingTextEdit(QWidget* parent) :
    QTextEdit(parent),
    m_auto_resize(true)
{
    connect(document(), &QTextDocument::contentsChanged,
            this, &GrowingTextEdit::contentsChanged);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}


GrowingTextEdit::GrowingTextEdit(const QString& text, QWidget* parent) :
    GrowingTextEdit(parent)  // delegating constructor
{
    setText(text);
}


GrowingTextEdit::~GrowingTextEdit()
{
}


void GrowingTextEdit::setAutoResize(const bool auto_resize)
{
    m_auto_resize = auto_resize;
}


QSize GrowingTextEdit::sizeHint() const
{
    ensurePolished();
    QSize size_hint;
    if (m_auto_resize) {
        size_hint = document()->size().toSize();
    } else {
        size_hint = QTextEdit::sizeHint();
    }
    // qDebug() << Q_FUNC_INFO << "-" << size_hint;
    return size_hint;
}


QSize GrowingTextEdit::minimumSizeHint() const
{
    // Implementing this reduces to a satisfactory level (though doesn't
    // entirely eliminate...) the tendency of the widget to develop a scroll
    // bar, rather than enlarging.
    //
    // However, don't just return sizeHint(), or you can get an escalating
    // width.
    QSize minsize = QTextEdit::minimumSizeHint();
    minsize.setHeight(sizeHint().height());
    return minsize;
}


void GrowingTextEdit::contentsChanged()
{
    // qDebug() << Q_FUNC_INFO;
    document()->setTextWidth(viewport()->width());
    updateGeometry();
}

// The final piece of the puzzle is that the Questionnaire's scroll area
// needs to resize itself when the widget sizes change.
// That requires:
//      http://doc.qt.io/qt-5.7/qscrollarea.html#widgetResizable-prop
// ... and (in VerticalScrollArea) a call to updateGeometry() when its widget
// size changes, it seems.
// ... no, calling updateGeometry() from VerticalScrollArea::resizeEvent is
// a recipe for a crash, I think.
