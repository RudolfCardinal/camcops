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

#include "db/fieldcreationplan.h"
#include "db/field.h"


QDebug operator<<(QDebug debug, const FieldCreationPlan& plan)
{
    debug.nospace()
        << "FieldCreationPlan(name=" << plan.name
        << ", intended base type=";
    if (plan.intended_field) {
        debug.nospace() << plan.intended_field->sqlColumnType();
    } else {
        debug.nospace() << "<none>";
    }
    debug.nospace()
        << ", intended full def=";
    if (plan.intended_field) {
        debug.nospace() << plan.intended_field->sqlColumnDef();
    } else {
        debug.nospace() << "<none>";
    }
    debug.nospace()
        << ", exists_in_db=" << plan.exists_in_db
        << ", existing_type=" << plan.existing_type
        << ", existing_not_null=" << plan.existing_not_null
        << ", add=" << plan.add
        << ", drop=" << plan.drop
        << ", change=" << plan.change << ")";
    return debug;
}


