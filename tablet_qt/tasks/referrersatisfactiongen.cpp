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

#include "referrersatisfactiongen.h"
#include "common/appstrings.h"
#include "tasklib/taskfactory.h"

const QString ReferrerSatisfactionGen::REF_SATIS_GEN_TABLENAME("ref_satis_gen");


void initializeReferrerSatisfactionGen(TaskFactory& factory)
{
    static TaskRegistrar<ReferrerSatisfactionGen> registered(factory);
}


ReferrerSatisfactionGen::ReferrerSatisfactionGen(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    SatisfactionCommon(app, db, REF_SATIS_GEN_TABLENAME, true, load_pk)
{
}


// ============================================================================
// Class info
// ============================================================================

QString ReferrerSatisfactionGen::shortname() const
{
    return "ReferrerSatisfactionSurvey";
}


QString ReferrerSatisfactionGen::longname() const
{
    return tr("Referrer Satisfaction Scale (anonymous survey)");
}


QString ReferrerSatisfactionGen::description() const
{
    return tr("Short rating of a clinical service received (survey).");
}


QString ReferrerSatisfactionGen::infoFilenameStem() const
{
    return "rss";
}


// ============================================================================
// Instance info
// ============================================================================

OpenableWidget* ReferrerSatisfactionGen::editor(const bool read_only)
{
    return satisfactionEditor(appstring(appstrings::SATIS_REF_GEN_RATING_Q),
                              read_only);
}
