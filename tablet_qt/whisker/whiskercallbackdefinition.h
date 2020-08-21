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
#include <QDateTime>
#include <QString>
class WhiskerInboundMessage;


class WhiskerCallbackDefinition
{
    // Represents a Whisker callback, i.e. a user function that will be called
    // when Whisker receives an appropriate event.

public:
    using CallbackFunction = std::function<void(const WhiskerInboundMessage&)>;
    // ... a function that is called with one parameter, a
    // const WhiskerInboundMessage&, and returns void.
    //
    // To pass other arguments, use std::bind to bind them before passing here.
    //
    // Note that the function doesn't even need to accept the
    // WhiskerInboundMessage, if called via std::bind.
    // See https://stackoverflow.com/questions/29159140/why-stdbind-can-be-assigned-to-argument-mismatched-stdfunction.

    // How should our callback behave?
    enum class ExpiryType {
        Infinite,  // Always call
        Count,  // Call a certain number of times, then stop calling
        Time,  // Call during a specified lifetime, then stop calling
        TimeOrCount  // Call until either a lifetime expires or a count is exceeded
    };

public:
    // Connstructor. Args:
    //
    // event:
    //      Whisker event name
    // callback:
    //      function to call
    // name:
    //      name of this callback [no purpose, except it's returned by name();
    //      the caller may wish to use this]
    // how_expires:
    //      see ExpiryType
    // target_n_calls:
    //      number of calls permitted, for ExpiryType::Count or ::TimeOrCount
    // lifetime_ms:
    //      lifetime in ms, for ExpiryType::Time or ::TimeOrCount
    // swallow_event:
    //      returned by swallowEvent(); meaning is: "if this callback fires,
    //      should processing of this event cease?" (so, if false, the event
    //      may be offered to other callbacks).
    WhiskerCallbackDefinition(const QString& event,
                              const CallbackFunction& callback,
                              const QString& name = "",
                              ExpiryType how_expires = ExpiryType::Infinite,
                              int target_n_calls = 0,
                              qint64 lifetime_ms = 0,
                              bool swallow_event = false);

    // Default constructor, so we can live in a QVector.
    WhiskerCallbackDefinition();

    // Returns the Whisker event string.
    QString event() const;

    // Returns the callback's name.
    QString name() const;

    // Has the callback exceeded its lifetime or call limit?
    bool hasExpired(const QDateTime& now) const;

    // Is the callback set to swallow events that it handles (see above)?
    bool swallowEvent() const;

    // Call the callback function with an inbound message.
    void call(const WhiskerInboundMessage& msg);

protected:
    QString m_event;  // Whisker event name
    CallbackFunction m_callback;  // user's callback function
    QString m_name;  // our name
    ExpiryType m_how_expires;  // how do we expire?
    int m_target_n_calls;  // number of calls permitted; see above
    qint64 m_lifetime_ms;  // lifetime (ms); see above
    QDateTime m_when_created;  // when was this callback created?
    QDateTime m_when_expires;  // when does this callback expire?
    bool m_swallow_event;  // is this callback swallowing events?
    int m_n_calls;  // how many times have we called our callback function?
};
