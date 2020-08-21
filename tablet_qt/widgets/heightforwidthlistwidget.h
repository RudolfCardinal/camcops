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
#include <QListWidget>


class HeightForWidthListWidget : public QListWidget
{
    // Version of QListWidget that can cope with its items using the
    // height-for-width system. Used by CamCOPS menus. Compare BaseWidget.

    Q_OBJECT
public:
    HeightForWidthListWidget(QWidget* parent = nullptr);
    virtual bool event(QEvent* event) override;
    virtual QSize widgetSizeHint(QWidget* widget) const;
};
