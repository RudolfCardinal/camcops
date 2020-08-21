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

#include "whiskerinboundmessage.h"
#include "whisker/whiskerconstants.h"
using namespace whiskerconstants;


WhiskerInboundMessage::WhiskerInboundMessage(const QString& msg,
                                             bool immediate_socket,
                                             const QDateTime& timestamp,
                                             bool has_timestamp,
                                             quint64 timestamp_ms) :
    m_msg(msg),
    m_immediate_socket(immediate_socket),
    m_timestamp(timestamp),
    m_has_server_timestamp(has_timestamp),
    m_server_timestamp_ms(timestamp_ms)
{
    splitServerTimestamp();
    parseMainSocketMessages();
}


void WhiskerInboundMessage::splitServerTimestamp()
{
    QRegularExpressionMatch match = TIMESTAMP_REGEX.match(m_msg);
    if (match.hasMatch()) {
        const QString msg = match.captured(1);
        const QString timestamp_str = match.captured(2);
        bool ok;
        const quint64 timestamp_ms = timestamp_str.toULongLong(&ok);
        if (ok) {
            m_has_server_timestamp = true;
            m_msg = msg;
            m_server_timestamp_ms = timestamp_ms;
        } else {
            qWarning() << Q_FUNC_INFO << "Bad timestamp:" << timestamp_str;
            m_has_server_timestamp = false;
        }
    }
}


QString WhiskerInboundMessage::message() const
{
    return m_msg;
}


bool WhiskerInboundMessage::fromImmediateSocket() const
{
    return m_immediate_socket;
}


QString WhiskerInboundMessage::causalCommand() const
{
    return m_causal_command;
}


void WhiskerInboundMessage::setCausalCommand(const QString& causal_command)
{
    m_causal_command = causal_command;
}


bool WhiskerInboundMessage::immediateReplySucceeded() const
{
    return m_msg == RESPONSE_SUCCESS;
}


QDateTime WhiskerInboundMessage::timestamp() const
{
    return m_timestamp;
}


bool WhiskerInboundMessage::hasServerTimestamp() const
{
    return m_has_server_timestamp;
}


quint64 WhiskerInboundMessage::serverTimestampMs() const
{
    return m_server_timestamp_ms;
}


void WhiskerInboundMessage::parseMainSocketMessages()
{
    if (m_immediate_socket) {
        return;
    }

    QRegularExpressionMatch event_match = EVENT_REGEX.match(m_msg);
    if (event_match.hasMatch()) {
        m_is_event = true;
        m_event = event_match.captured(1);
        return;
    }

    QRegularExpressionMatch key_event_match = KEY_EVENT_REGEX.match(m_msg);
    if (key_event_match.hasMatch()) {
        m_is_key_event = true;
        m_key_code = key_event_match.captured(1).toInt();
        const QString updown = key_event_match.captured(2);
        m_key_down = updown == VAL_KEYEVENT_DOWN;
        m_key_up = updown == VAL_KEYEVENT_UP;
        m_key_doc = key_event_match.captured(3);
        return;
        // Whisker docs had an error in prior to 2018-09-04, and claimed "1"
        // for key depressed and "0" for key released, but is actually "down"
        // for key depressed and "up" for key released.
        // In the server source these are WS_VAL_UP, WS_VAL_DOWN
        // (in whiskermessages.h).
    }

    QRegularExpressionMatch client_msg_match = CLIENT_MESSAGE_REGEX.match(m_msg);
    if (client_msg_match.hasMatch()) {
        m_is_client_message = true;
        m_client_message_source_clientnum = client_msg_match.captured(1).toInt();
        m_client_message = client_msg_match.captured(2);
        return;
    }

    QRegularExpressionMatch warning_match = WARNING_REGEX.match(m_msg);
    if (warning_match.hasMatch()) {
        m_is_warning = true;
        return;
    }

    QRegularExpressionMatch syntax_error_match = SYNTAX_ERROR_REGEX.match(m_msg);
    if (syntax_error_match.hasMatch()) {
        m_is_syntax_error = true;
        return;
    }

    QRegularExpressionMatch error_match = ERROR_REGEX.match(m_msg);
    if (error_match.hasMatch()) {
        m_is_error = true;
        return;
    }

    if (m_msg == PING_ACK) {
        m_is_ping_ack = true;
        return;
    }
}


bool WhiskerInboundMessage::isEvent() const
{
    return m_is_event;
}


QString WhiskerInboundMessage::event() const
{
    return m_event;
}


bool WhiskerInboundMessage::isKeyEvent() const
{
    return m_is_key_event;
}


int WhiskerInboundMessage::keyEventCode() const
{
    return m_key_code;
}


bool WhiskerInboundMessage::keyEventDown() const
{
    return m_key_down;
}


bool WhiskerInboundMessage::keyEventUp() const
{
    return m_key_up;
}


QString WhiskerInboundMessage::keyEventDoc() const
{
    return m_key_doc;
}


bool WhiskerInboundMessage::isClientMessage() const
{
    return m_is_client_message;
}


int WhiskerInboundMessage::clientMessageSourceClientNum() const
{
    return m_client_message_source_clientnum;
}


QString WhiskerInboundMessage::clientMessage() const
{
    return m_client_message;
}


bool WhiskerInboundMessage::isWarning() const
{
    return m_is_warning;
}


bool WhiskerInboundMessage::isSyntaxError() const
{
    return m_is_syntax_error;
}


bool WhiskerInboundMessage::isError() const
{
    return m_is_error;
}


bool WhiskerInboundMessage::isPingAck() const
{
    return m_is_ping_ack;
}


QDebug operator<<(QDebug debug, const WhiskerInboundMessage& m)
{
    debug.nospace()
            << "InboundMessage(msg=" << m.m_msg
            << ", immediate_socket=" << m.m_immediate_socket
            << ", timestamp=" << m.m_timestamp
            << ", has_server_timestamp=" << m.m_has_server_timestamp
            << ", server_timestamp_ms=" << m.m_server_timestamp_ms
            << ", causal_command=" << m.m_causal_command
            << ")";
    return debug;
}

