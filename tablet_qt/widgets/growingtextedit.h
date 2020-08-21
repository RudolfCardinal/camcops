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
#include <QTextEdit>


class GrowingTextEdit : public QTextEdit
{
    // Text editor that expands vertically to its contents.

    // see http://stackoverflow.com/questions/11677499
    // http://stackoverflow.com/questions/3050537
    // http://stackoverflow.com/questions/1153714
    // http://www.qtcentre.org/threads/9840-QTextEdit-auto-resize
    // http://stackoverflow.com/questions/11851020
    Q_OBJECT
public:

    // Plain constructor
    GrowingTextEdit(QWidget* parent = nullptr);

    // Construct with text.
    GrowingTextEdit(const QString& text, QWidget* parent = nullptr);

    // Destructor
    virtual ~GrowingTextEdit() override;

    // Resize automatically? Default is true; that's the point of this widget.
    void setAutoResize(bool auto_resize);

    // Standard Qt widget overrides.
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

protected slots:
    // "The contents of the text editor has changed."
    void contentsChanged();

protected:
    bool m_auto_resize;  // should we resize ourself to our contents?
};
