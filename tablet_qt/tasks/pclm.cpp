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

#include "pclm.h"
#include "tasklib/taskfactory.h"

const QString PclM::PCLM_TABLENAME("pclm");


void initializePclM(TaskFactory& factory)
{
    static TaskRegistrar<PclM> registered(factory);
}


PclM::PclM(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    PclCommon(app, db, PCLM_TABLENAME, "m", false, load_pk)
{
}


// ============================================================================
// Class info
// ============================================================================

QString PclM::shortname() const
{
    return "PCL-M";
}


QString PclM::longname() const
{
    return tr("PTSD Checklist, Military version");
}
