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

#include "whiskeroutboundcommand.h"
#include "whisker/whiskerapi.h"
#include "whisker/whiskerconstants.h"
using whiskerapi::msgFromArgs;


WhiskerOutboundCommand::WhiskerOutboundCommand(const QString& command,
                                               bool immediate_socket,
                                               bool immediate_ignore_reply) :
    m_command(command),
    m_immediate_socket(immediate_socket),
    m_immediate_ignore_reply(immediate_ignore_reply)
{
}


WhiskerOutboundCommand::WhiskerOutboundCommand(const QStringList& args,
                                               bool immediate_socket,
                                               bool immediate_ignore_reply) :
    m_command(msgFromArgs(args)),
    m_immediate_socket(immediate_socket),
    m_immediate_ignore_reply(immediate_ignore_reply)
{
}


WhiskerOutboundCommand::WhiskerOutboundCommand(
        std::initializer_list<QString> args,
        bool immediate_socket,
        bool immediate_ignore_reply) :
    m_command(msgFromArgs(QStringList(args))),
    m_immediate_socket(immediate_socket),
    m_immediate_ignore_reply(immediate_ignore_reply)
{
}


QString WhiskerOutboundCommand::terminatedCommand() const
{
    return m_command + whiskerconstants::EOL;
}


QByteArray WhiskerOutboundCommand::bytes() const
{
    // QString via QByteArray to const char*:
    return terminatedCommand().toLatin1();
    // If you want const char*, use QByteArray::constData().
    // But that is likely to lead to out-of-scope errors and data corruption!
    // Moving from const char* to QByteArray immediately fixed this.
}


QDebug operator<<(QDebug debug, const WhiskerOutboundCommand& c)
{
    debug.nospace()
            << "WhiskerOutboundCommand(command=" << c.m_command
            << ", immediate_socket=" << c.m_immediate_socket
            << ", immediate_ignore_reply=" << c.m_immediate_ignore_reply
            << ")";
    return debug;
}
