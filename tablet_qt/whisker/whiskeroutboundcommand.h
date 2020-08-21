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
#include <QByteArray>
#include <QDebug>
#include <QStringList>


class WhiskerOutboundCommand {

    // Represents a command heading to the Whisker server.

public:
    // Construct with a command.
    // - An all-default constructor allows its use in QVector.
    // Args:
    //      immediate_socket:
    //          send via the immediate socket, not the main socket?
    //      immediate_ignore_reply:
    //          for immediate-socket commands: ignore the reply?
    WhiskerOutboundCommand(const QString& command = "",
                           bool immediate_socket = false,
                           bool immediate_ignore_reply = false);

    // Construct with a list of command arguments.
    WhiskerOutboundCommand(const QStringList& args,
                           bool immediate_socket = false,
                           bool immediate_ignore_reply = false);
    WhiskerOutboundCommand(std::initializer_list<QString> args,
                           bool immediate_socket = false,
                           bool immediate_ignore_reply = false);

    // Return the final LF-terminated command.
    QString terminatedCommand() const;

    // Returns the terminated command in raw bytes format.
    QByteArray bytes() const;

public:
    QString m_command;  // the full command
    bool m_immediate_socket;  // send via the immediate socket, not the main socket?
    bool m_immediate_ignore_reply;  // for immediate-socket commands: ignore the reply?

public:
    // Debugging description
    friend QDebug operator<<(QDebug debug, const WhiskerOutboundCommand& s);
};

Q_DECLARE_METATYPE(WhiskerOutboundCommand)
