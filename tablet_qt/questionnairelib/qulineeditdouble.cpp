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

#include "qulineeditdouble.h"
#include <limits>
#include <QDoubleValidator>
#include <QLineEdit>
#include "qobjects/strictdoublevalidator.h"
#include "questionnairelib/qulineeditinteger.h"


QuLineEditDouble::QuLineEditDouble(FieldRefPtr fieldref,
                                   const bool allow_empty) :
    QuLineEdit(fieldref),
    /* Compare
       https://en.cppreference.com/w/cpp/types/numeric_limits/min
       https://en.cppreference.com/w/cpp/types/numeric_limits/lowest
       "For floating-point types with denormalization, min returns the minimum
       positive normalized value. Note that this behavior may be unexpected,
       especially when compared to the behavior of min for integral types.
       To find the value that has no values less than it, use
       numeric_limits::lowest."
    */
    m_minimum(std::numeric_limits<double>::lowest()),
    m_maximum(std::numeric_limits<double>::max()),
    m_decimals(2),
    m_allow_empty(allow_empty),
    m_strict_validator(true)
{
    setHint(tr("real number, %1 dp").arg(m_decimals));
}


QuLineEditDouble::QuLineEditDouble(FieldRefPtr fieldref,
                                   const double minimum,
                                   const double maximum,
                                   const int decimals,
                                   const bool allow_empty) :
    QuLineEdit(fieldref),
    m_minimum(minimum),
    m_maximum(maximum),
    m_decimals(decimals),
    m_allow_empty(allow_empty),
    m_strict_validator(true)
{
    setHint(tr("real number, %1 to %2, %3 dp")
            .arg(m_minimum)
            .arg(m_maximum)
            .arg(m_decimals));
}


QuLineEditDouble* QuLineEditDouble::setStrictValidator(const bool strict)
{
    m_strict_validator = strict;
    return this;
}


void QuLineEditDouble::extraLineEditCreation(QLineEdit* editor)
{
    if (m_strict_validator) {
        editor->setValidator(new StrictDoubleValidator(
            m_minimum, m_maximum, m_decimals, m_allow_empty, this));
    } else {
        editor->setValidator(new QDoubleValidator(
            m_minimum, m_maximum, m_decimals, this));
    }
    editor->setInputMethodHints(Qt::ImhFormattedNumbersOnly);
}
