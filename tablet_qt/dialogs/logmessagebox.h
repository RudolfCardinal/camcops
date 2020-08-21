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


class LogMessageBox : public QDialog
{
    // Modal and BLOCKING dialogue, used for displaying console-style text
    // and allowing copy/paste. Construct and call exec().
    // Requires that you know the text in advance, because you have to pass
    // it to the constructor.
    //
    // Compare LogBox for a modal but non-blocking version.

    Q_OBJECT
public:
    LogMessageBox(QWidget* parent, const QString& title, const QString& text,
                  bool as_html = false, bool word_wrap = false);
protected slots:
    void copyClicked();
protected:
    QPointer<QPlainTextEdit> m_editor;
};
