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

// #define WHISKERWORKER_DEBUG_SOCKETS

#include "whiskerworker.h"
#include <QDebug>
#include <QTcpSocket>
#include <QTextStream>
#include "lib/datetime.h"
#include "whisker/whiskerapi.h"
#include "whisker/whiskermanager.h"
using namespace whiskerconstants;


// ============================================================================
// Helper functions
// ============================================================================

// Disable the Nagle algorithm for a TCP socket, putting the socket into
// "no-delay" mode.
void disableNagle(QTcpSocket* socket)
{
    Q_ASSERT(socket);
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
}


// ============================================================================
// WhiskerWorker
// ============================================================================

WhiskerWorker::WhiskerWorker() :
    QObject(nullptr),  // no QObject parent; see docs for QObject::moveToThread()
    m_imm_port(0),
    m_main_socket(new QTcpSocket(this)),  // will be autodeleted by QObject
    m_immediate_socket(new QTcpSocket(this))  // will be autodeleted by QObject
{
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif

    disableNagle(m_main_socket);
    disableNagle(m_immediate_socket);

    connect(m_main_socket, &QTcpSocket::connected,
            this, &WhiskerWorker::onMainSocketConnected);
    connect(m_main_socket, &QTcpSocket::readyRead,
            this, &WhiskerWorker::onDataReadyFromMainSocket);
    connect(m_main_socket, &QTcpSocket::disconnected,
            this, &WhiskerWorker::onAnySocketDisconnected);
    connect(m_main_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &WhiskerWorker::onMainSocketError);

    connect(m_immediate_socket, &QTcpSocket::connected,
            this, &WhiskerWorker::onImmSocketConnected);
    connect(m_immediate_socket, &QTcpSocket::readyRead,
            this, &WhiskerWorker::onDataReadyFromImmediateSocket);
    connect(m_immediate_socket, &QTcpSocket::disconnected,
            this, &WhiskerWorker::onAnySocketDisconnected);
    connect(m_immediate_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &WhiskerWorker::onImmSocketError);

    setConnectionState(WhiskerConnectionState::A_Disconnected);
}


void WhiskerWorker::connectToServer(const QString& host, quint16 main_port)
{
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif
    qInfo().nospace()
            << "Connecting to Whisker server: host " << host
            << ", main port " << main_port;
    if (m_connection_state != WhiskerConnectionState::A_Disconnected) {
        disconnectFromServer();
    }
    m_host = host;
    m_main_port = main_port;
    m_main_socket->connectToHost(host, main_port);
    setConnectionState(WhiskerConnectionState::B_RequestingMain);
}


void WhiskerWorker::disconnectFromServer()
{
    // This function may be called directly and triggered by sockets closing,
    // including as a result of what we do here, so make sure it's happy with
    // recursive/multiple calls.
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif
    if (m_immediate_socket->state() != QTcpSocket::UnconnectedState) {
        m_immediate_socket->disconnectFromHost();
    }
    if (m_main_socket->state() != QTcpSocket::UnconnectedState) {
        m_main_socket->disconnectFromHost();
    }
    if (m_connection_state != WhiskerConnectionState::A_Disconnected) {
        qInfo() << "Disconnecting from Whisker server";
    }
    setConnectionState(WhiskerConnectionState::A_Disconnected);
}


void WhiskerWorker::sendToServer(const WhiskerOutboundCommand& cmd)
{
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO << cmd;
#endif
    if (cmd.m_immediate_socket) {
        if (!isImmediateConnected()) {
            qWarning() << Q_FUNC_INFO << "Attempt to write to closed immediate socket";
            return;
        }
        // WHETHER OR NOT we want the reply, we push the command.
        // (We throw away the result later if we didn't want it.)
        m_mutex_imm.lock();
        m_imm_commands_awaiting_reply.push_back(cmd);
        m_mutex_imm.unlock();
#ifdef WHISKERWORKER_DEBUG_SOCKETS
        qDebug() << Q_FUNC_INFO << "Writing to immediate socket:" << cmd.bytes();
#endif
        m_immediate_socket->write(cmd.bytes());
    } else {
        if (!isMainConnected()) {
            qWarning() << Q_FUNC_INFO << "Attempt to write to closed main socket";
            return;
        }
#ifdef WHISKERWORKER_DEBUG_SOCKETS
        qDebug() << Q_FUNC_INFO << "Writing to main socket:" << cmd.bytes();
#endif
        m_main_socket->write(cmd.bytes());
    }
}


void WhiskerWorker::setConnectionState(WhiskerConnectionState state)
{
    if (state == m_connection_state) {
        return;
    }
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << "New Whisker connection state:" << whiskerConnectionStateDescription(state);
#endif
    m_connection_state = state;
    emit connectionStateChanged(state);
    if (state == WhiskerConnectionState::G_FullyConnected) {
        emit onFullyConnected();
    }
}


bool WhiskerWorker::isMainConnected() const
{
    return m_connection_state != WhiskerConnectionState::A_Disconnected &&
            m_connection_state != WhiskerConnectionState::B_RequestingMain;
}


bool WhiskerWorker::isImmediateConnected() const
{
    return m_connection_state == WhiskerConnectionState::F_BothConnectedAwaitingLink ||
            m_connection_state == WhiskerConnectionState::G_FullyConnected;
}


bool WhiskerWorker::isFullyConnected() const
{
    return m_connection_state == WhiskerConnectionState::G_FullyConnected;
}


bool WhiskerWorker::isFullyDisconnected() const
{
    return m_connection_state == WhiskerConnectionState::A_Disconnected;
}


void WhiskerWorker::onMainSocketConnected()
{
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif
    setConnectionState(WhiskerConnectionState::C_MainConnectedAwaitingImmPort);
}


void WhiskerWorker::onImmSocketConnected()
{
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif
    setConnectionState(WhiskerConnectionState::F_BothConnectedAwaitingLink);
    // Special command follows! See pushImmediateReply()
    WhiskerOutboundCommand cmd({CMD_LINK, m_code}, true, true);
    sendToServer(cmd);  // will send only when we quit back to the event loop
}


void WhiskerWorker::onAnySocketDisconnected()
{
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif
    disconnectFromServer();
}


void WhiskerWorker::onMainSocketError(QAbstractSocket::SocketError error)
{
    QString msg;
    QTextStream s(&msg);
    s << "Whisker main socket error:" << error;
    qWarning() << msg;
    emit socketError(msg);
    disconnectFromServer();
}


void WhiskerWorker::onImmSocketError(QAbstractSocket::SocketError error)
{
    QString msg;
    QTextStream s(&msg);
    s << "Whisker immediate socket error:" << error;
    qWarning() << msg;
    emit socketError(msg);
    disconnectFromServer();
}


void WhiskerWorker::onDataReadyFromMainSocket()
{
    // We get here from a QTcpSocket event.
    const QVector<WhiskerInboundMessage> messages = getIncomingMessagesFromSocket(false);
    for (const WhiskerInboundMessage& msg : messages) {
        processMainSocketMessage(msg);
    }
}


void WhiskerWorker::onDataReadyFromImmediateSocket()
{
    // We get here from a QTcpSocket event.
    QVector<WhiskerInboundMessage> messages = getIncomingMessagesFromSocket(true);
    for (WhiskerInboundMessage& msg : messages) {
        pushImmediateReply(msg);
    }
}


void WhiskerWorker::processMainSocketMessage(const WhiskerInboundMessage& msg)
{
    // Handle the low-level connection messages, and pass anything else on
    // via our signals.

#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO << msg;
#endif

    const QString& line = msg.message();

    const QRegularExpressionMatch immport_match = IMMPORT_REGEX.match(line);
    if (immport_match.hasMatch()) {
        if (m_connection_state != WhiskerConnectionState::C_MainConnectedAwaitingImmPort) {
            qWarning() << "ImmPort message received at wrong stage";
            disconnectFromServer();
            return;
        }
        m_imm_port = static_cast<quint16>(immport_match.captured(1).toUInt());
#ifdef WHISKERWORKER_DEBUG_SOCKETS
        qDebug() << "Whisker server offers immediate port" << m_imm_port;
#endif
        setConnectionState(WhiskerConnectionState::D_MainConnectedAwaitingCode);
        return;
    }

    const QRegularExpressionMatch code_match = CODE_REGEX.match(line);
    if (code_match.hasMatch()) {
        if (m_connection_state != WhiskerConnectionState::D_MainConnectedAwaitingCode) {
            qWarning() << "Code message received at wrong stage";
            disconnectFromServer();
            return;
        }
        m_code = code_match.captured(1);
#ifdef WHISKERWORKER_DEBUG_SOCKETS
        qDebug() << "Whisker server has provided code for immediate port";
#endif
        qInfo().nospace()
                << "Connecting immediate socket to Whisker server: host "
                << m_host << ", immediate port " << m_imm_port;
        m_immediate_socket->connectToHost(m_host, m_imm_port);
        setConnectionState(WhiskerConnectionState::E_MainConnectedRequestingImmediate);
        return;
    }

    if (line == PING) {
        WhiskerOutboundCommand cmd(PING_ACK, false);
        sendToServer(cmd);
        return;
    }

    emit receivedFromServerMainSocket(msg);
}


void WhiskerWorker::pushImmediateReply(WhiskerInboundMessage& msg)
{
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif

    bool wake = false;

    m_mutex_imm.lock();
    Q_ASSERT(!m_imm_commands_awaiting_reply.isEmpty());
    const WhiskerOutboundCommand& cmd = m_imm_commands_awaiting_reply.front();
    if (!cmd.m_immediate_ignore_reply) {
        msg.setCausalCommand(cmd.m_command);
        m_imm_replies_awaiting_collection.push_back(msg);
        wake = true;
    }
    m_imm_commands_awaiting_reply.pop_front();
    m_mutex_imm.unlock();

    if (m_connection_state == WhiskerConnectionState::F_BothConnectedAwaitingLink) {
        // Special!
        if (msg.immediateReplySucceeded()) {
            qInfo().nospace()
                    << "Fully connected to Whisker server: host " << m_host
                    << ", main port " << m_main_port
                    << ", immediate port " << m_imm_port;
            setConnectionState(WhiskerConnectionState::G_FullyConnected);
        } else {
            qWarning() << "Failed to execute Link command; reply was"
                       << msg.message();
            disconnectFromServer();
        }
        return;
    }

    if (wake) {
        m_immediate_reply_arrived.wakeAll();  // wakes: waitForImmediateReply()
    }
}


WhiskerInboundMessage WhiskerWorker::getPendingImmediateReply()
{
    // CALLED FROM A DIFFERENT THREAD
#ifdef WHISKERWORKER_DEBUG_SOCKETS
    qDebug() << Q_FUNC_INFO;
#endif
    m_mutex_imm.lock();
    if (m_imm_replies_awaiting_collection.isEmpty()) {  // must hold mutex to read this
#ifdef WHISKERWORKER_DEBUG_SOCKETS
        qDebug() << Q_FUNC_INFO << "waiting for a reply...";
#endif
        m_immediate_reply_arrived.wait(&m_mutex_imm);  // woken by: pushImmediateReply()
        // ... this mutex is UNLOCKED as we go to sleep, and LOCKED
        //     as we wake: http://doc.qt.io/qt-5/qwaitcondition.html#wait
        Q_ASSERT(!m_imm_replies_awaiting_collection.isEmpty());
#ifdef WHISKERWORKER_DEBUG_SOCKETS
        qDebug() << Q_FUNC_INFO << "... reply ready";
#endif
    }
    WhiskerInboundMessage msg = m_imm_replies_awaiting_collection.front();
    m_imm_replies_awaiting_collection.pop_front();
    m_mutex_imm.unlock();
    return msg;
}


QVector<WhiskerInboundMessage> WhiskerWorker::getIncomingMessagesFromSocket(
        bool via_immediate_socket)
{
    const QDateTime timestamp = datetime::now();
    QTcpSocket* socket = via_immediate_socket ? m_immediate_socket : m_main_socket;
    QString& buffer = via_immediate_socket ? m_inbound_buffer_imm : m_inbound_buffer_main;
    const QByteArray bytes = socket->readAll();
    const QString& string = QString::fromLatin1(bytes);
    buffer += string;
    return getIncomingMessagesFromBuffer(buffer, via_immediate_socket, timestamp);
}


QVector<WhiskerInboundMessage> WhiskerWorker::getIncomingMessagesFromBuffer(
        QString& buffer, bool via_immediate_socket, const QDateTime& timestamp)
{
    QStringList strings = buffer.split(EOL);
    // If the buffer contains complete responses, the last string will be
    // empty. In all cases, the last string is the residual.
    const int length = strings.length();
    Q_ASSERT(length > 0);
    buffer = strings.at(length - 1);  // the residual
    QVector<WhiskerInboundMessage> messages;
    for (int i = 0; i < length - 1; ++i) {  // the messages
        const QString& content = strings.at(i);
        WhiskerInboundMessage msg(content, via_immediate_socket, timestamp);
        messages.append(msg);
    }
    return messages;
}


