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

// #define NHS_DEBUG_VALIDATOR

#ifdef NHS_DEBUG_VALIDATOR
#include <QDebug>
#endif
#include <QLocale>
#include <QString>
#include <QValidator>
#include <QVector>

// ============================================================================
// Namespace with declarations
// ============================================================================

namespace nhs {

// Calculates an NHS number check digit.
// Given a 9-digit number as a vector, return the 10th digit that would be
// required to make this into a valid (checksummed) NHS number, or
// "failure_code" if that's not possible.
int nhsCheckDigit(const QVector<int>& ninedigits, int failure_code = -1);

// Declare a validator for NHS numbers.
template<typename T>
QValidator::State validateNhsNumber(const QString& s, bool allow_empty = false);

}  // namespace nhs


// ============================================================================
// Templatized Qt validator function for NHS numbers
// ============================================================================

template<typename T>  // T is e.g. qint64
QValidator::State nhs::validateNhsNumber(const QString& s, bool allow_empty)
{
    if (s.isEmpty()) {
        if (allow_empty) {
#ifdef NUMERICFUNC_DEBUG_VALIDATOR
            qDebug() << Q_FUNC_INFO << "empty -> Acceptable (as allow_empty)";
#endif
            return QValidator::Acceptable;
        }
#ifdef NUMERICFUNC_DEBUG_VALIDATOR
        qDebug() << Q_FUNC_INFO << "empty -> Intermediate";
#endif
        return QValidator::Intermediate;
    }

    const int nhs_num_len = 10;
    const int len = s.length();
    QVector<int> main_digits;
    const int failure_code = -1;
    int check_digit = failure_code;
    for (int i = 0; i < len; ++i) {
        const QChar& c = s.at(i);
        if (!c.isDigit()) {
#ifdef NHS_DEBUG_VALIDATOR
        qDebug() << Q_FUNC_INFO << s << "-> Invalid (contains non-digit characters)";
#endif
            return QValidator::State::Invalid;
        }
        const int digit = QString(c).toInt();
        if (i == 0 && digit == 0) {
#ifdef NHS_DEBUG_VALIDATOR
        qDebug() << Q_FUNC_INFO << s << "-> Invalid (first digit is zero)";
#endif
            return QValidator::State::Invalid;
        }
        if (i == nhs_num_len - 1) {
            check_digit = digit;
        } else {
            main_digits.push_back(digit);
        }
    }

    if (len > nhs_num_len) {
#ifdef NHS_DEBUG_VALIDATOR
        qDebug() << Q_FUNC_INFO << s << "-> Invalid (>10 digits)";
#endif
            return QValidator::State::Invalid;
    }
    if (len < nhs_num_len) {
#ifdef NHS_DEBUG_VALIDATOR
        qDebug() << Q_FUNC_INFO << s << "-> Intermediate (<10 digits)";
#endif
            return QValidator::State::Intermediate;
    }

    // Now we're here, the number is a valid integer in our specified 10-digit
    // range, and we answer the additional question of whether it is a valid
    // NHS number too.
    const int expected_check_digit = nhsCheckDigit(main_digits, failure_code);
    if (expected_check_digit == failure_code) {
#ifdef NHS_DEBUG_VALIDATOR
        qDebug() << Q_FUNC_INFO << s << "-> Invalid (bug? Check digit calculation failed)";
#endif
        return QValidator::State::Invalid;
    }
    if (expected_check_digit == 10) {
#ifdef NHS_DEBUG_VALIDATOR
        qDebug().nospace()
                << Q_FUNC_INFO << " " << s
                << " -> Invalid (calculated check digit is 10, meaning a bad number)";
#endif
        return QValidator::State::Invalid;
    }
    if (check_digit != expected_check_digit) {
#ifdef NHS_DEBUG_VALIDATOR
        qDebug().nospace()
                << Q_FUNC_INFO << " " << s
                << " -> Invalid (bad check digit; expected "
                << expected_check_digit << ")";
#endif
        return QValidator::State::Invalid;
    }
#ifdef NHS_DEBUG_VALIDATOR
        qDebug() << Q_FUNC_INFO << s << "-> Acceptable";
#endif
    return QValidator::State::Acceptable;
}
