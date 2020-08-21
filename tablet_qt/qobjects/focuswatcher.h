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

class QEvent;


class FocusWatcher : public QObject
{
    // Object to watch for change of focus on another widget.
    // - If you ARE a widget, you can overload QWidget::focusOutEvent().
    // - If you OWN a widget, use this. (You can't connect to the widget's
    //   QWidget::focusOutEvent(), because that's protected.)
    // The watcher is OWNED BY and WATCHES the same thing.

    // http://stackoverflow.com/questions/17818059/what-is-the-signal-for-when-a-widget-loses-focus
    Q_OBJECT

public:
    // Constructor, taking the object to watch.
    explicit FocusWatcher(QObject* parent);

    // Receive incoming events.
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    // "The watched widget's focus has changed."
    void focusChanged(bool in);
};
