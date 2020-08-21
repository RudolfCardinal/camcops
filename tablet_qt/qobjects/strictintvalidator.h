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
#include <QIntValidator>

// Validator for int numbers. Checks against the range [bottom, top].

class StrictIntValidator : public QIntValidator
{
    // Validates an integer being typed in.
    // Checks the characters against the specified bottom/top (min/max) values.

    Q_OBJECT
public:
    StrictIntValidator(int bottom, int top, bool allow_empty = false,
                       QObject* parent = nullptr);
    virtual QValidator::State validate(QString& s, int& pos) const override;
protected:
    bool m_allow_empty;
};


// What about validating a qulonglong = quint64 (unsigned 64-bit int), etc.?
// Normally we would use C++ templates, but you can't mix that with Q_OBJECT.
// So we have to faff a great deal to make StrictUInt64Validator (q.v.).
