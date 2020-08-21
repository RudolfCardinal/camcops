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
#include <QObject>


class ShowWatcher : public QObject
{
    // Object to watch for a showEvent() on a widget.
    // If you ARE a QWidget, you can overload QWidget::showEvent() instead.
    // If you OWN a QWidget, you can use this.
    // The watcher is OWNED BY and WATCHES the same thing.

    Q_OBJECT

public:
    // Constructor, taking the object to watch.
    explicit ShowWatcher(QObject* parent, bool debug_layout = false);

    // Receive incoming events.
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    // "The watched object is being shown."
    void showing();

protected:
    bool m_debug_layout;
};
