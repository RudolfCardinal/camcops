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

#include "roman.h"
#include <QChar>
#include <QDebug>
#include <QMap>
#include <QPair>
#include <QVector>

// http://blog.stevenlevithan.com/archives/javascript-roman-numeral-converter

const QVector<QPair<QString, int>> ENCODER{  // used for int -> Roman
    {"M", 1000},
    {"CM", 900},
    {"D", 500},
    {"CD", 400},
    {"C", 100},
    {"XC", 90},
    {"L", 50},
    {"XL", 40},
    {"X", 10},
    {"IX", 9},
    {"V", 5},
    {"IV", 4},
    {"I", 1},
};
const QMap<QString, int> DECODER{  // used for Roman -> int
    {"I", 1},
    {"V", 5},
    {"X", 10},
    {"L", 50},
    {"C", 100},
    {"D", 500},
    {"M", 1000},
};


namespace roman
{


QString romanize(int num)
{
    QString roman;
    const int n_lookups = ENCODER.size();
    // Traverse the Roman numerals (including the "one-before" ones) in
    // descending order, building up the string.
    for (int i = 0; i < n_lookups; ++i) {
        const QPair<QString, int>& pair = ENCODER.at(i);
        const QString& r = pair.first;
        const int n = pair.second;
        while (num >= n) {
            roman += r;
            num -= n;
        }
    }
    return roman;

    /*
    This is not the most concise, perhaps; for example, it converts 1999
    to MCMXCIX, rather than MIM. Still, good enough for what we want.

    Anyway, what's "correct"?

    https://www.infoplease.com/askeds/1999-roman-numerals

    Q. Is the "official" Roman numeral for 1999 MCMXCIX or MIM?
    A. According to librarians at the National Institute of Standards and
       Technology, while MIM is more convenient, MCMXCIX is favored because of
       earlier precedents with numbers such as 49 (written as XLIX rather than
       IL); however, the librarians point out that purists use neither MIM nor
       MCMXCIX, opting instead for MCMXCVIIII. The ancient Romans, they
       explain, did not use the 20th century convention of IX for the number
       nine.
        - The Editors
    */
}


int deromanize(const QString& roman)
{
    int arabic = 0;
    int i = roman.length();
    const QString ru = roman.toUpper();
    // We work from the right to the left of the string.
    int value_to_right = 0;
    while (i--) {
        // ... the "while" test operates from "length" down to 1
        // ... while (because of the post-decrement), within the loop, i has
        //     the values "length - 1" down to 0
        // ... so at(i) is always valid
        const QString current_char = ru.at(i);
        if (!DECODER.contains(current_char)) {
            qWarning() << "Roman" << ru
                       << "contains bad character" << current_char;
            continue;
        }
        int current_value = DECODER[current_char];
        if (current_value < value_to_right) {
            // ... will never be true the first time through
            arabic -= current_value;
        } else {
            arabic += current_value;
        }
        value_to_right = current_value;
    }
    return arabic;
}


}  // namespace roman
