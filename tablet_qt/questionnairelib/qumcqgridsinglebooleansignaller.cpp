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

#include "qumcqgridsinglebooleansignaller.h"
#include "db/fieldref.h"
#include "questionnairelib/qumcqgridsingleboolean.h"


QuMcqGridSingleBooleanSignaller::QuMcqGridSingleBooleanSignaller(
        QuMcqGridSingleBoolean* recipient, const int question_index) :
    m_recipient(recipient),
    m_question_index(question_index)
{
}


void QuMcqGridSingleBooleanSignaller::mcqFieldValueOrMandatoryChanged(
        const FieldRef* fieldref)
{
    m_recipient->mcqFieldValueOrMandatoryChanged(m_question_index, fieldref);
}


void QuMcqGridSingleBooleanSignaller::booleanFieldValueOrMandatoryChanged(
        const FieldRef* fieldref)
{
    m_recipient->booleanFieldValueOrMandatoryChanged(m_question_index, fieldref);
}
