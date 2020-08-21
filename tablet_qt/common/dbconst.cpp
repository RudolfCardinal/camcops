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

#include "dbconst.h"
#include <QDebug>


namespace dbconst {

const QString PK_FIELDNAME("id");
const QString MODIFICATION_TIMESTAMP_FIELDNAME("when_last_modified");
const QString MOVE_OFF_TABLET_FIELDNAME("_move_off_tablet");
// ... must match database.py on server

const QString CREATION_TIMESTAMP_FIELDNAME("when_created");
const int NONEXISTENT_PK = -1;

const QString UNKNOWN_IDNUM_DESC("<ID_number_%1>");


bool isValidWhichIdnum(int which_idnum)
{
    const bool valid = which_idnum >= 1;
    if (!valid) {
        qWarning() << Q_FUNC_INFO << "bad idnum" << which_idnum;
    }
    return valid;
}


}  // namespace dbconst
