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

#include "pcls.h"
#include "tasklib/taskfactory.h"

const QString PclS::PCLS_TABLENAME("pcls");


void initializePclS(TaskFactory& factory)
{
    static TaskRegistrar<PclS> registered(factory);
}


PclS::PclS(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    PclCommon(app, db, PCLS_TABLENAME, "s", true, load_pk)
{
}


// ============================================================================
// Class info
// ============================================================================

QString PclS::shortname() const
{
    return "PCL-S";
}


QString PclS::longname() const
{
    return tr("PTSD Checklist, Stressor-specific version");
}
