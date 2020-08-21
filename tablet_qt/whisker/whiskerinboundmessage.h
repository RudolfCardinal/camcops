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
#include <QDateTime>
#include <QDebug>
#include <QString>


class WhiskerInboundMessage {

    // A message inbound to us from a Whisker server.

public:
    // Constructor.
    // - A plain (no-arguments or default-arguments) constructor is required so
    //   we can put this into a QVector.
    // Args:
    //      msg:
    //          the message
    //      immediate_socket:
    //          is this from the "immediate" (not the "main") socket?
    //      timestamp:
    //          time of receipt by CamCOPS
    //      has_server_timestamp:
    //          is there an associated timestamp from the server?
    //      server_timestamp_ms:
    //          server timestamp, in ms
    WhiskerInboundMessage(const QString& msg = "",
                          bool immediate_socket = false,
                          const QDateTime& timestamp = QDateTime(),
                          bool has_server_timestamp = false,
                          quint64 server_timestamp_ms = 0);

    // Return the message.
    QString message() const;

    // Is this from the "immediate" (not the "main") socket?
    // See Whisker docs.
    bool fromImmediateSocket() const;

    // What command (CamCOPS -> Whisker) caused Whisker to send this message?
    QString causalCommand() const;

    // Sets the causal command (see above).
    void setCausalCommand(const QString& causal_command);

    // (For immediate replies) Did the command succeed, i.e. is the message
    // "Success"?
    bool immediateReplySucceeded() const;

    // Returns the time of receipt by CamCOPS.
    QDateTime timestamp() const;

    // Is there a server timestamp?
    bool hasServerTimestamp() const;

    // Returns the server timestamp in ms.
    quint64 serverTimestampMs() const;

    // Is this an event?
    bool isEvent() const;

    // (If event) Returns the event string.
    QString event() const;

    // Is this a key event?
    bool isKeyEvent() const;

    // (If key event) Returns the key code.
    int keyEventCode() const;

    // (If key event) Was the key depressed?
    bool keyEventDown() const;

    // (If key event) Was the key released?
    bool keyEventUp() const;

    // (If key event) Returns the Whisker document receiving the keypress.
    QString keyEventDoc() const;

    // Is this a client message?
    bool isClientMessage() const;

    // (If client message) Returns the source client's Whisker client number.
    int clientMessageSourceClientNum() const;

    // (If client message) Returns the message.
    QString clientMessage() const;

    // Is this a warning?
    bool isWarning() const;

    // Is this a syntax error?
    bool isSyntaxError() const;

    // Is this an error?
    bool isError() const;

    // Is this the acknowledgement from a "ping" command?
    bool isPingAck() const;

protected:

    // Parse m_msg into a server timestamp (if present) and the rest of the
    // message.
    void splitServerTimestamp();

    // If the message was received on the main socket, parse m_msg and set all
    // our other internal flags/variables.
    void parseMainSocketMessages();

protected:
    QString m_msg;  // the incoming message
    bool m_immediate_socket;  // was it from the immediate socket?
    QString m_causal_command;  // CamCOPS -> Whisker command that caused this
    QDateTime m_timestamp;  // Time of receipt
    bool m_has_server_timestamp;  // Is there a server timestamp?
    quint64 m_server_timestamp_ms;  // Server timestamp (ms)
    bool m_is_event = false;  // Is this an event message?
    QString m_event;  // (If event) The event string
    bool m_is_key_event = false;  // Is this a key event message?
    int m_key_code = 0;  // (If key event) Key code
    bool m_key_down = false;  // (If key event) Key being depressed?
    bool m_key_up = false;  // (If key event) Key being released?
    QString m_key_doc;  // (If key event) Whisker document receiving the keypress
    bool m_is_client_message = false;  // Is this a message from another Whisker client?
    int m_client_message_source_clientnum = -1;  // (If client message) The sender's client number
    QString m_client_message;  // (If client message) The message
    bool m_is_warning = false;  // Is this a warning?
    bool m_is_syntax_error = false;  // Is this a syntax error?
    bool m_is_error = false;  // Is this an error?
    bool m_is_ping_ack = false;  // Is this a ping acknowledgement?

public:
    // Debugging description.
    friend QDebug operator<<(QDebug debug, const WhiskerInboundMessage& s);
};

Q_DECLARE_METATYPE(WhiskerInboundMessage)
