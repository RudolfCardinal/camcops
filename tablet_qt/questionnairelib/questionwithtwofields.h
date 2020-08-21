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
#include "db/fieldref.h"


class QuestionWithTwoFields
{
    // Encapsulates a question with two associated fields.
    // Used by e.g. QuMCQGridDouble.

public:

    // Default constructor, so it can live in a QVector
    QuestionWithTwoFields();  // so it can live in a QVector

    // Standard constructor
    QuestionWithTwoFields(const QString& question,
                          FieldRefPtr first_field,
                          FieldRefPtr second_field);

    // Return the question (text)
    QString question() const;

    // Return the first fieldref.
    FieldRefPtr firstFieldRef() const;

    // Return the second fieldref.
    FieldRefPtr secondFieldRef() const;

    // Return either fieldref.
    FieldRefPtr fieldref(bool first_field) const;

protected:
    QString m_question;  // question text
    FieldRefPtr m_first_field;  // first fieldref
    FieldRefPtr m_second_field;  // second fieldref
};
