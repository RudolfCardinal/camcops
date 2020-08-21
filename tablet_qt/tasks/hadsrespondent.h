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
#include "tasks/hads.h"

void initializeHadsRespondent(TaskFactory& factory);


class HadsRespondent : public Hads
{
    // See Hads for reasoning about this class structure.

    Q_OBJECT
public:
    HadsRespondent(CamcopsApp& app, DatabaseManager& db,
                   int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString infoFilenameStem() const override;
    virtual QString xstringTaskname() const override;
public:
    static const QString HADSRESPONDENT_TABLENAME;
};
