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
#include <QMetaType>
#include <QString>


enum class WhiskerConnectionState {
    // Possible states of our connection to a Whisker server.

    A_Disconnected,
    B_RequestingMain,
    C_MainConnectedAwaitingImmPort,
    D_MainConnectedAwaitingCode,
    E_MainConnectedRequestingImmediate,
    F_BothConnectedAwaitingLink,
    G_FullyConnected,
};

Q_DECLARE_METATYPE(WhiskerConnectionState)

QString whiskerConnectionStateDescription(WhiskerConnectionState state);
