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
#include <QString>
#include "common/design_defines.h"

namespace dbconst {

// Fieldnames used across a range of tables.
extern const QString PK_FIELDNAME;
extern const QString MOVE_OFF_TABLET_FIELDNAME;
extern const QString MODIFICATION_TIMESTAMP_FIELDNAME;
extern const QString CREATION_TIMESTAMP_FIELDNAME;

// A PK pseudo-value to mean "nonexistent".
extern const int NONEXISTENT_PK;

// How to describe an unknown ID number type.
extern const QString UNKNOWN_IDNUM_DESC;

// Is this ID number type potentially valid?
bool isValidWhichIdnum(int which_idnum);

}  // namespace dbconst
