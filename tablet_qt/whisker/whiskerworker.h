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
#include <QAbstractSocket>
#include <QMutex>
#include <QString>
#include <QVector>
#include <QWaitCondition>
#include "whisker/whiskerconnectionstate.h"
#include "whisker/whiskerconstants.h"
#include "whisker/whiskerinboundmessage.h"
#include "whisker/whiskeroutboundcommand.h"
class QTcpSocket;


class WhiskerWorker : public QObject
{
    // Object to manage communication with a Whisker server via TCP/IP.
    // Once created, this object is moved (by WhiskerManager) into a NEW
    // THREAD. Its functionality is then driven by Qt events.

    Q_OBJECT

public:

    // Constructor.
    WhiskerWorker();

    // Wait for an immediate reply to arrive, then return it.
    // Called from OTHER THREADS.
    WhiskerInboundMessage getPendingImmediateReply();

    // Is the main socket connected?
    bool isMainConnected() const;

    // Is the immediate socket connected?
    bool isImmediateConnected() const;

    // Are both sockets connected?
    bool isFullyConnected() const;

    // Are both sockets disconnected?
    bool isFullyDisconnected() const;

signals:
    // "The connection state has changed." (See WhiskerConnectionState.)
    void connectionStateChanged(WhiskerConnectionState state);

    // "We are now fully connected."
    void onFullyConnected();

    // "Message received on main socket." (Will be connected to WhiskerManager.)
    void receivedFromServerMainSocket(const WhiskerInboundMessage& msg);

    // "A socket error has occurred."
    void socketError(const QString& msg);

public slots:
    // "Please connect to the specified Whisker server."
    void connectToServer(
            const QString& host,
            quint16 main_port = whiskerconstants::WHISKER_DEFAULT_PORT);

    // "Disconnect from the Whisker server."
    void disconnectFromServer();

    // "Send this message to the server." (See WhiskerOutboundCommand.)
    void sendToServer(const WhiskerOutboundCommand& cmd);  // from WhiskerManager

protected slots:
    // "The main socket is connected."
    void onMainSocketConnected();

    // "The immediate socket is connected."
    void onImmSocketConnected();

    // "One of our sockets has been disconnected."
    void onAnySocketDisconnected();

    // "Data is ready to be read from the main socket."
    void onDataReadyFromMainSocket();

    // "Data is ready to be read from the immediate socket."
    void onDataReadyFromImmediateSocket();

    // "An error has occurred on the main socket."
    void onMainSocketError(QAbstractSocket::SocketError error);

    // "An error has occurred on the immediate socket."
    void onImmSocketError(QAbstractSocket::SocketError error);

protected:
    // Set the connection state. If we're fully connected, emit
    // onFullyConnected().
    void setConnectionState(WhiskerConnectionState state);

    // Returns all inbound messages currently available for a given socket.
    QVector<WhiskerInboundMessage> getIncomingMessagesFromSocket(
            bool via_immediate_socket);

    // Returns all inbound messages currently available for a given socket.
    // Lower-level function than the other one with the same name.
    QVector<WhiskerInboundMessage> getIncomingMessagesFromBuffer(
            QString& buffer, bool via_immediate_socket,
            const QDateTime& timestamp);

    // Handle the low-level connection messages, and pass anything else on
    // via our signals.
    void processMainSocketMessage(const WhiskerInboundMessage& msg);

    // Push a reply, received from the immediate socket, into our "replies
    // awaiting collection" queue.
    void pushImmediateReply(WhiskerInboundMessage& msg);

protected:
    quint16 m_main_port;  // main port number
    quint16 m_imm_port;  // immediate port number
    QString m_host;  // hostname
    QString m_code;  // security code given to us by the server
    QTcpSocket* m_main_socket;  // main socket
    QTcpSocket* m_immediate_socket;  // immediate socket
    WhiskerConnectionState m_connection_state;  // overall connection status
    QString m_inbound_buffer_main;  // inbound message buffer for main socket
    QString m_inbound_buffer_imm;  // inbound message buffer for immediate socket
    QVector<WhiskerOutboundCommand> m_imm_commands_awaiting_reply;
        // ... outbound commands waiting to be matched to a reply
    QVector<WhiskerInboundMessage> m_imm_replies_awaiting_collection;
        // ... inbound replies, already matched with the outbound command that
        //     triggered them, now awaiting collection
    QMutex m_mutex_imm;
        // ... mutex for m_imm_replies_awaiting_collection AND
        //     m_imm_commands_awaiting_reply
    QWaitCondition m_immediate_reply_arrived;
        // ... "a reply has arrived"
};
