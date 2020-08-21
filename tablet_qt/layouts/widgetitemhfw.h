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

#include <QLayout>
#include <QMap>


class WidgetItemHfw : public QWidgetItemV2
{
    // Replacement for QWidgetItemV2 (inheriting from QWidgetItem, inheriting
    // from QLayoutItem) -- used to encapsulate widgets in layouts.
    // This one handles height-for-width widgets better.
    //
    // Made by createWidgetItem() or directly.
    //
    //

public:
    WidgetItemHfw(QWidget* widget);
    virtual QSize sizeHint() const override;
    virtual QSize minimumSize() const override;
    virtual QSize maximumSize() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int w) const override;
    virtual void invalidate() override;
    virtual void setGeometry(const QRect& rect) override;
protected:
    mutable QSize m_cached_sizehint;
    mutable QSize m_cached_minsize;
    mutable QSize m_cached_maxsize;
    mutable QMap<int, int> m_width_to_height;
};
