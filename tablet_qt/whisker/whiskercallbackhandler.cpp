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

#include "whiskercallbackhandler.h"
#include "whisker/whiskerinboundmessage.h"


WhiskerCallbackHandler::WhiskerCallbackHandler()
{
}


void WhiskerCallbackHandler::add(
        const QString& event,
        const WhiskerCallbackDefinition::CallbackFunction& callback,
        const QString& name,
        WhiskerCallbackDefinition::ExpiryType how_expires,
        int target_n_calls,
        qint64 lifetime_ms,
        bool swallow_event)
{
    WhiskerCallbackDefinition cb(event, callback, name,
                                 how_expires, target_n_calls, lifetime_ms,
                                 swallow_event);
    m_callbacks.append(cb);
}


void WhiskerCallbackHandler::addSingle(
        const QString& event,
        const WhiskerCallbackDefinition::CallbackFunction& callback,
        const QString& name,
        bool swallow_event)
{
    // Adds a single-shot callback.
    add(event, callback, name,
        WhiskerCallbackDefinition::ExpiryType::Count, 1, 0,
        swallow_event);
}


void WhiskerCallbackHandler::addPersistent(
        const QString& event,
        const WhiskerCallbackDefinition::CallbackFunction& callback,
        const QString& name,
        bool swallow_event)
{
    // Adds a persistent callback.
    add(event, callback, name,
        WhiskerCallbackDefinition::ExpiryType::Infinite, 0, 0,
        swallow_event);
}


void WhiskerCallbackHandler::removeByEvent(const QString& event)
{
    if (event.isEmpty()) {
        return;
    }
    QMutableVectorIterator<WhiskerCallbackDefinition> it(m_callbacks);
    while (it.hasNext()) {
        if (it.next().event() == event) {
            it.remove();
        }
    }
}


void WhiskerCallbackHandler::removeByName(const QString& name)
{
    if (name.isEmpty()) {
        return;
    }
    QMutableVectorIterator<WhiskerCallbackDefinition> it(m_callbacks);
    while (it.hasNext()) {
        if (it.next().name() == name) {
            it.remove();
        }
    }
}


void WhiskerCallbackHandler::removeByEventAndName(const QString& event,
                                                  const QString& name)
{
    if (event.isEmpty() || name.isEmpty()) {
        return;
    }
    QMutableVectorIterator<WhiskerCallbackDefinition> it(m_callbacks);
    while (it.hasNext()) {
        const WhiskerCallbackDefinition& callback = it.next();
        if (callback.event() == event && callback.name() == name) {
            it.remove();
        }
    }
}


void WhiskerCallbackHandler::clearCallbacks()
{
    // Removes all callbacks.
    m_callbacks.clear();
}


bool WhiskerCallbackHandler::processEvent(const WhiskerInboundMessage& msg)
{
    // Calls any callbacks for the event.
    // Returns whether or not the event was swallowed (dealt with).

    // Remove expired callbacks first, as they may have expired by time.
    removeExpiredCallbacks(msg.timestamp());

    const QString& event = msg.event();
    for (WhiskerCallbackDefinition& callback : m_callbacks) {
        if (callback.event() == event) {
            callback.call(msg);
            if (callback.swallowEvent()) {
                return true;
            }
        }
    }
    return false;
}


void WhiskerCallbackHandler::removeExpiredCallbacks(const QDateTime& now)
{
    // Remove any single-shot (or otherwise expired) events.
    // http://doc.qt.io/qt-5/containers.html#java-style-iterators
    QMutableVectorIterator<WhiskerCallbackDefinition> it(m_callbacks);
    while (it.hasNext()) {
        if (it.next().hasExpired(now)) {
            it.remove();
        }
    }
}
