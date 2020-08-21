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
#include <QPlainTextEdit>

class GrowingPlainTextEdit : public QPlainTextEdit
{
    // This is to PlainTextEdit as GrowingTextEdit is to TextEdit.
    // HOWEVER, IT DOES NOT AUTO_SIZE PROPERLY YET (see sizeHint).
    // NOT CURRENTLY IN USE.

    Q_OBJECT
public:
    GrowingPlainTextEdit(QWidget* parent = nullptr);
    GrowingPlainTextEdit(const QString& text, QWidget* parent = nullptr);
    virtual ~GrowingPlainTextEdit() override;
    void setAutoResize(bool auto_resize);
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
protected slots:
    void contentsChanged();
protected:
    bool m_auto_resize;
};
