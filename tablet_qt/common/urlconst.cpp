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

#include "urlconst.h"

namespace urlconst {

// const QString CAMCOPS_URL("http://camcops.org");
const QString CAMCOPS_URL("https://camcops.readthedocs.io");
const QString CAMCOPS_DOCS_BASE_URL(CAMCOPS_URL + "/en/latest");
const QString CAMCOPS_DOCS_URL(CAMCOPS_DOCS_BASE_URL + "/index.html");
const QString CAMCOPS_LICENCES_URL(CAMCOPS_DOCS_BASE_URL +
                                   "/licences/licences.html");

QString taskDocUrl(const QString& stem)
{
    return QString("%1/tasks/%2.html").arg(CAMCOPS_DOCS_BASE_URL, stem);
}


}  // namespace urlconst
