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

namespace urlconst {

// URLs of, and within, the online documentation.
extern const QString CAMCOPS_URL;
extern const QString CAMCOPS_DOCS_BASE_URL;
extern const QString CAMCOPS_DOCS_URL;
extern const QString CAMCOPS_LICENCES_URL;

// Returns the URL to find a task in the online CamCOPS docs.
// The stem is something like "ace3"; it's typically the task's table name.
QString taskDocUrl(const QString& stem);

}  // namespace urlconst
