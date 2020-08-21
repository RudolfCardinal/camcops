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

#include "questionwithonefield.h"


QuestionWithOneField::QuestionWithOneField(const QString& question,
                                           FieldRefPtr fieldref) :
    m_question(question),
    m_fieldref(fieldref)
{
    Q_ASSERT(!m_question.isEmpty());
    Q_ASSERT(!m_fieldref.isNull());
}


QuestionWithOneField::QuestionWithOneField(FieldRefPtr fieldref,
                                           const QString& question) :
    m_question(question),
    m_fieldref(fieldref)
{
    Q_ASSERT(!m_question.isEmpty());
    Q_ASSERT(!m_fieldref.isNull());
}


QuestionWithOneField::QuestionWithOneField() :
    m_fieldref(nullptr)
{
}


QString QuestionWithOneField::question() const
{
    return m_question;
}


QString QuestionWithOneField::text() const
{
    return m_question;
}


FieldRefPtr QuestionWithOneField::fieldref() const
{
    return m_fieldref;
}
