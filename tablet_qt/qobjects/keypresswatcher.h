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
#include <functional>
#include <QMap>
#include <QObject>
class QDialog;


class KeyPressWatcher : public QObject
{
    // Object to watch for keypresses on another widget.
    // If you ARE a QWidget, you can overload its event functions instead.
    // If you OWN a QWidget, you can use this.
    // The watcher is OWNED BY and WATCHES the same thing.

    Q_OBJECT
public:
    using CallbackFunction = std::function<void()>;

public:
    // Constructor, taking the object to watch.
    explicit KeyPressWatcher(QDialog* parent);

    // Receive incoming events.
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

    // "Please call my callback function when the widget receives this
    // keypress."
    void addKeyEvent(int key, const CallbackFunction& callback);

signals:
    // "The watched widget has received a keypress."
    void keypress(int key);

protected:
    QMap<int, CallbackFunction> m_map;
};
