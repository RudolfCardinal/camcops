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

#include "questionwithtwofields.h"


QuestionWithTwoFields::QuestionWithTwoFields(const QString& question,
                                             FieldRefPtr first_field,
                                             FieldRefPtr second_field) :
    m_question(question),
    m_first_field(first_field),
    m_second_field(second_field)
{
    Q_ASSERT(!m_question.isEmpty());
    Q_ASSERT(!m_first_field.isNull());
    Q_ASSERT(!m_second_field.isNull());
}


QuestionWithTwoFields::QuestionWithTwoFields() :
    m_first_field(nullptr),
    m_second_field(nullptr)
{
}


QString QuestionWithTwoFields::question() const
{
    return m_question;
}


FieldRefPtr QuestionWithTwoFields::firstFieldRef() const
{
    return m_first_field;
}


FieldRefPtr QuestionWithTwoFields::secondFieldRef() const
{
    return m_second_field;
}


FieldRefPtr QuestionWithTwoFields::fieldref(const bool first_field) const
{
    return first_field ? m_first_field : m_second_field;
}
