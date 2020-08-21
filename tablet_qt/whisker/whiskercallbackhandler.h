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
#include <QVector>
#include "whisker/whiskercallbackdefinition.h"
class WhiskerInboundMessage;


class WhiskerCallbackHandler
{
    // Callback handler that maintains a list of WhiskerCallbackDefinition
    // objects, and processes an incoming Whisker event through them.

public:

    // Constructor.
    WhiskerCallbackHandler();

    // Add a callback. See WhiskerCallbackDefinition.
    void add(
            const QString& event,
            const WhiskerCallbackDefinition::CallbackFunction& callback,
            const QString& name = "",
            WhiskerCallbackDefinition::ExpiryType how_expires = WhiskerCallbackDefinition::ExpiryType::Infinite,
            int target_n_calls = 0,
            qint64 lifetime_ms = 0,
            bool swallow_event = true);

    // Add a single-shot callback.
    void addSingle(
            const QString& event,
            const WhiskerCallbackDefinition::CallbackFunction& callback,
            const QString& name = "",
            bool swallow_event = true);

    // Adds a callback that never expires.
    void addPersistent(
            const QString& event,
            const WhiskerCallbackDefinition::CallbackFunction& callback,
            const QString& name = "",
            bool swallow_event = true);

    // Remove all callbacks for a specific Whisker event.
    void removeByEvent(const QString& event);

    // Remove all callbacks with a specific name.
    void removeByName(const QString& name);

    // Remove all callbacks for a specific event that also have a specific name.
    void removeByEventAndName(const QString& event, const QString& name);

    // Remove all callbacks.
    void clearCallbacks();

    // Calls any callbacks for the event.
    // Returns whether or not the event was swallowed (dealt with).
    bool processEvent(const WhiskerInboundMessage& msg);

protected:
    // Remove any callbacks that have expired. (Housekeeping function.)
    void removeExpiredCallbacks(const QDateTime& now);

protected:
    QVector<WhiskerCallbackDefinition> m_callbacks;  // our callbacks
};
