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
#include "questionnairelib/qulineedit.h"


class QuLineEditInt64 : public QuLineEdit
{
    // Offers a one-line text editor, for a 64-bit signed integer
    // (qlonglong = qint64).

    Q_OBJECT
public:

    // Constructor for unconstrained numbers
    QuLineEditInt64(FieldRefPtr fieldref, bool allow_empty = true);

    // Constructor for constrained numbers.
    // - allow_empty: OK to be blank?
    QuLineEditInt64(FieldRefPtr fieldref, qint64 minimum,
                       qint64 maximum, bool allow_empty = true);

protected:
    virtual void extraLineEditCreation(QLineEdit* editor) override;

protected:
    qint64 m_minimum;  // minimum; may be std::numeric_limits<qint64>::min()
    qint64 m_maximum;  // maximum; may be std::numeric_limits<qint64>::max()
    bool m_allow_empty;  // allow an empty field?
};
