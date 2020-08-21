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

namespace gbocommon
{

extern const int AGENT_PATIENT;
extern const int AGENT_PARENT_CARER;
extern const int AGENT_CLINICIAN;
extern const int AGENT_OTHER;

extern const QString AGENTSTR_PATIENT;
extern const QString AGENTSTR_PARENT_CARER;
extern const QString AGENTSTR_CLINICIAN;
extern const QString AGENTSTR_OTHER;

QString agentDescription(int agent, const QString& other_detail);

extern const int PROGRESS_MIN;
extern const int PROGRESS_MAX;

}  // namespace gbocommon
