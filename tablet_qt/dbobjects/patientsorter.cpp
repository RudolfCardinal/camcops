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

#include "patientsorter.h"
#include "dbobjects/patient.h"

PatientSorter::PatientSorter()
{
    // could use this to implement specific sorting methods; see
    // https://forum.qt.io/topic/4877/sorting-a-qlist-with-a-comparator/4
}


bool PatientSorter::operator()(const PatientPtr& left,
                               const PatientPtr& right) const
{
    // Implements: LEFT < RIGHT ?
    // Sort by surname, then forename, then DOB.
    const QString l_surname = left->surname().toUpper();
    const QString r_surname = right->surname().toUpper();
    if (l_surname != r_surname) {
        return l_surname < r_surname;  // ascending
    }
    const QString l_forename = left->forename().toUpper();
    const QString r_forename = right->forename().toUpper();
    if (l_forename != r_forename) {
        return l_forename < r_forename;  // ascending
    }
    return left->dob() < right->dob();  // ascending
}
