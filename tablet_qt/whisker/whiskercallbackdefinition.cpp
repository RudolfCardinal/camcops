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

#include "whiskercallbackdefinition.h"
#include <QDebug>
#include "common/preprocessor_aid.h"
#include "whisker/whiskerinboundmessage.h"


WhiskerCallbackDefinition::WhiskerCallbackDefinition(
        const QString& event,
        const CallbackFunction& callback,
        const QString& name,
        ExpiryType how_expires,
        int target_n_calls,
        qint64 lifetime_ms,
        bool swallow_event) :
    m_event(event),
    m_callback(callback),
    m_name(name),
    m_how_expires(how_expires),
    m_target_n_calls(target_n_calls),
    m_lifetime_ms(lifetime_ms),
    m_when_created(QDateTime::currentDateTime()),
    m_swallow_event(swallow_event),
    m_n_calls(0)
{
    m_when_expires = m_when_created.addMSecs(lifetime_ms);
}


WhiskerCallbackDefinition::WhiskerCallbackDefinition() :
    m_how_expires(ExpiryType::Infinite),
    m_target_n_calls(0),
    m_lifetime_ms(0),
    m_swallow_event(false),
    m_n_calls(0)
{
    // nasty default constructor used by QVector; UNSAFE
    // See http://doc.qt.io/qt-5/containers.html#default-constructed-value
    qWarning() << "Unsafe use of WhiskerCallbackDefinition::WhiskerCallbackDefinition()";
}


QString WhiskerCallbackDefinition::event() const
{
    return m_event;
}


QString WhiskerCallbackDefinition::name() const
{
    return m_name;
}


bool WhiskerCallbackDefinition::hasExpired(const QDateTime& now) const
{
    switch (m_how_expires) {
    case ExpiryType::Infinite:
        return false;
    case ExpiryType::Count:
        return m_n_calls >= m_target_n_calls;
    case ExpiryType::Time:
        return now > m_when_expires;
    case ExpiryType::TimeOrCount:
        return m_n_calls >= m_target_n_calls || now > m_when_expires;
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
    default:
        return true;  // a bug, so we may as well delete it!
#endif
    }
}


bool WhiskerCallbackDefinition::swallowEvent() const
{
    return m_swallow_event;
}


void WhiskerCallbackDefinition::call(const WhiskerInboundMessage& msg)
{
    ++m_n_calls;
    m_callback(msg);
}
