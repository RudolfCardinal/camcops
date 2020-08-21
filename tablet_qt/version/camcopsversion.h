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
#include "lib/version.h"
#include <QDate>


namespace camcopsversion {

// Master version constants for the CamCOPS client.

extern const Version CAMCOPS_CLIENT_VERSION;  // Client version
extern const QDate CAMCOPS_CLIENT_CHANGEDATE;  // When was the client last changed?
extern const Version MINIMUM_SERVER_VERSION;  // What's the minimum acceptable server version?

}  // namespace camcopsversion
