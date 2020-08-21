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

#include "nhs.h"

const QVector<int> NHS_DIGIT_WEIGHTINGS{10, 9, 8, 7, 6, 5, 4, 3, 2};

namespace nhs {

int nhsCheckDigit(const QVector<int>& ninedigits, int failure_code)
{
    // Calculates an NHS number check digit.
    //
    //    1. Multiply each of the first nine digits by the corresponding
    //       digit weighting (see NHS_DIGIT_WEIGHTINGS).
    //    2. Sum the results.
    //    3. Take remainder after division by 11.
    //    4. Subtract the remainder from 11
    //    5. If this is 11, use 0 instead
    //    If it's 10, the number is invalid
    //    If it doesn't match the actual check digit, the number is invalid

    const int len = ninedigits.length();
    if (len != 9) {
        return failure_code;
    }
    int total = 0;
    for (int i = 0; i < len; ++i) {
        const int digit = ninedigits.at(i);
        if (digit < 0 || digit > 9) {
            return failure_code;
        }
        total += digit * NHS_DIGIT_WEIGHTINGS.at(i);
    }
    const int mod_total = total % 11;
    // ... % 11 yields something in the range 0-10
    int check_digit = 11 - mod_total;
    // ... (11 - that) yields something in the range 1-11
    if (check_digit == 11) {
        check_digit = 0;
    }
    return check_digit;
}

}  // namespace nhs
