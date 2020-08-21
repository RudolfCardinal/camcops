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

#include "referrersatisfactionspec.h"
#include "common/appstrings.h"
#include "tasklib/taskfactory.h"

const QString ReferrerSatisfactionSpec::REF_SATIS_SPEC_TABLENAME("ref_satis_spec");


void initializeReferrerSatisfactionSpec(TaskFactory& factory)
{
    static TaskRegistrar<ReferrerSatisfactionSpec> registered(factory);
}


ReferrerSatisfactionSpec::ReferrerSatisfactionSpec(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    SatisfactionCommon(app, db, REF_SATIS_SPEC_TABLENAME, false, load_pk)
{
}


// ============================================================================
// Class info
// ============================================================================

QString ReferrerSatisfactionSpec::shortname() const
{
    return "ReferrerSatisfactionSpecific";
}


QString ReferrerSatisfactionSpec::longname() const
{
    return tr("Referrer Satisfaction Scale (patient-specific)");
}


QString ReferrerSatisfactionSpec::description() const
{
    return tr("Short rating of a clinical service received (patient-specific).");
}


QString ReferrerSatisfactionSpec::infoFilenameStem() const
{
    return "rss";
}


// ============================================================================
// Instance info
// ============================================================================

OpenableWidget* ReferrerSatisfactionSpec::editor(const bool read_only)
{
    return satisfactionEditor(appstring(appstrings::SATIS_REF_SPEC_RATING_Q),
                              read_only);
}
