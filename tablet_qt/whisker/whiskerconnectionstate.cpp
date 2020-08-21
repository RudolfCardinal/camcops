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

#include "whiskerconnectionstate.h"


QString whiskerConnectionStateDescription(WhiskerConnectionState state)
{
    switch (state) {
    case WhiskerConnectionState::A_Disconnected:
        return "Disconnected";
    case WhiskerConnectionState::B_RequestingMain:
        return "RequestingMain";
    case WhiskerConnectionState::C_MainConnectedAwaitingImmPort:
        return "MainConnectedAwaitingImmPort";
    case WhiskerConnectionState::D_MainConnectedAwaitingCode:
        return "MainConnectedAwaitingCode";
    case WhiskerConnectionState::E_MainConnectedRequestingImmediate:
        return "MainConnectedRequestingImmediate";
    case WhiskerConnectionState::F_BothConnectedAwaitingLink:
        return "BothConnectedAwaitingLink";
    case WhiskerConnectionState::G_FullyConnected:
        return "FullyConnected";
    }
    return "?";
}
