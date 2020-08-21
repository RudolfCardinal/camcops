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

#include "gbocommon.h"

namespace gbocommon
{

const int AGENT_PATIENT = 1;  // In original: child/young person
const int AGENT_PARENT_CARER = 2;
const int AGENT_CLINICIAN = 3;
const int AGENT_OTHER = 4;

const QString AGENTSTR_PATIENT = "Patient/service user";  // In original: "Child/young person"
const QString AGENTSTR_PARENT_CARER = "Parent/carer";
const QString AGENTSTR_CLINICIAN = "Practitioner/clinician";
const QString AGENTSTR_OTHER = "Other: ";
const QString AGENTSTR_UNKNOWN = "Unknown";


QString agentDescription(const int agent, const QString& other_detail)
{
    switch (agent) {
    case AGENT_PATIENT:
        return AGENTSTR_PATIENT;
    case AGENT_PARENT_CARER:
        return AGENTSTR_PARENT_CARER;
    case AGENT_CLINICIAN:
        return AGENTSTR_CLINICIAN;
    case AGENT_OTHER:
        return AGENTSTR_OTHER + (other_detail.isEmpty() ? "?" : other_detail);
    default:
        return AGENTSTR_UNKNOWN;
    }
}


const int PROGRESS_MIN = 0;
const int PROGRESS_MAX = 10;

}  // namespace gbocommon
