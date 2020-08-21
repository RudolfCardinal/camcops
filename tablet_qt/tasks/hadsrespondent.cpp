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

#include "hadsrespondent.h"
#include "tasklib/taskfactory.h"

const QString HadsRespondent::HADSRESPONDENT_TABLENAME("hads_respondent");


void initializeHadsRespondent(TaskFactory& factory)
{
    static TaskRegistrar<HadsRespondent> registered(factory);
}


HadsRespondent::HadsRespondent(CamcopsApp& app, DatabaseManager& db,
                               const int load_pk) :
    Hads(app, db, HADSRESPONDENT_TABLENAME, true, load_pk)
{
    // Hads constructor will load for us.
}


// ============================================================================
// Class info
// ============================================================================

QString HadsRespondent::shortname() const
{
    return "HADS-Respondent";
}


QString HadsRespondent::longname() const
{
    return tr("Hospital Anxiety and Depression Scale, "
              "non-patient respondent version");
}


QString HadsRespondent::infoFilenameStem() const
{
    return "hads";
}


QString HadsRespondent::xstringTaskname() const
{
    return "hads";
}
