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

#include "patientsatisfaction.h"
#include "common/appstrings.h"
#include "tasklib/taskfactory.h"

const QString PatientSatisfaction::PT_SATIS_TABLENAME("pt_satis");


void initializePatientSatisfaction(TaskFactory& factory)
{
    static TaskRegistrar<PatientSatisfaction> registered(factory);
}


PatientSatisfaction::PatientSatisfaction(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    SatisfactionCommon(app, db, PT_SATIS_TABLENAME, false, load_pk)
{
}


// ============================================================================
// Class info
// ============================================================================

QString PatientSatisfaction::shortname() const
{
    return "PatientSatisfaction";
}


QString PatientSatisfaction::longname() const
{
    return tr("Patient Satisfaction Scale");
}


QString PatientSatisfaction::description() const
{
    return tr("Short rating of a clinical service received.");
}


QString PatientSatisfaction::infoFilenameStem() const
{
    return "pss";
}


// ============================================================================
// Instance info
// ============================================================================

OpenableWidget* PatientSatisfaction::editor(const bool read_only)
{
    return satisfactionEditor(appstring(appstrings::SATIS_PT_RATING_Q),
                              read_only);
}
