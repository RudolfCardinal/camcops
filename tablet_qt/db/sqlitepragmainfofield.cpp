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

#include "sqlitepragmainfofield.h"
#include <QDebug>


QDebug operator<<(QDebug debug, const SqlitePragmaInfoField& info)
{
    debug.nospace()
        << "SqlitePragmaInfo(cid=" << info.cid
        << ", name=" << info.name
        << ", type=" << info.type
        << ", notnull=" << info.notnull
        << ", dflt_value=" << info.dflt_value
        << ", pk=" << info.pk << ")";
    return debug;
}
