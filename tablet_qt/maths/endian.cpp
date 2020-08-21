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

#include "endian.h"
#include <QDebug>
#include "maths/floatbits.h"


Endian endianByteOrder()
{
    const short int word = 0x0001;
    auto byte = reinterpret_cast<const char*>(&word);

    // A big-endian machine stores 0x1234 as 0x12, 0x34.
    // A little-endian machine stores it as 0x34, 0x12.
    // https://en.wikipedia.org/wiki/Endianness
    // Expect little-endian on x86. (ARM v3+ is switchable!)

    return (byte[0] ? Endian::LittleEndian : Endian::BigEndian);
}


Endian endianFloatWordOrder()
{
    /*
    IEEE 754's XDR standard is big-endian.
    IEEE 754 itself doesn't specify endianness.
    x86 uses little-endian for floating point.
    https://en.wikipedia.org/wiki/Endianness#Floating_point

    Excellent description of the bitwise formats:
    http://steve.hollasch.net/cgindex/coding/ieeefloat.html

    "The sign bit is 0 for positive, 1 for negative.
    The exponent base is two.
    The exponent field contains 127 plus the true exponent for single-
    precision, or 1023 plus the true exponent for double precision.
    The first bit of the mantissa is typically assumed to be 1.f, where f is
    the field of fraction bits."

    "Floating point numbers are typically stored in normalized form.
    This basically puts the radix point after the fist non-zero digit.
    In normalized form, five is represented as 5.000 x 10^0."

    RNC attempt:
    See http://www.binaryconvert.com/convert_double.html

    Take the specimen number 1.
    1 -> sign 0, exponent 0x3FF, mantissa 0
     ... full version: 0x3FF0000000000000 or:
    00111111 11110000 00000000 00000000 00000000 00000000 00000000 00000000

    Now, following ieee754.h and my equivalent in floatbits.h:
    With S sign, E exponent, M mantissa0 (high bits), m mantissa1 (low bits)
    Big-endian: memory left to right:

    00111111 11110000 00000000 00000000 00000000 00000000 00000000 00000000
    SEEEEEEE EEEEMMMM MMMMMMMM MMMMMMMM mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm

    Byte little-endian, float word big-endian:
    00000000 00000000 00000111 11111110 00000000 00000000 00000000 00000000
    MMMMMMMM MMMMMMMM MMMMEEEE EEEEEEES mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm

    Byte little-endian, float word little-endian:
    00000000 00000000 00000000 00000000 00000000 00000000 00000111 11111110
    mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm MMMMMMMM MMMMMMMM MMMMEEEE EEEEEEES

    */

    Endian byte;
    Endian word;
    const BitRepresentationDouble br(1.0);
    const uint64_t expected_exponent = 0x3FF;
    if (br.getExponent(Endian::BigEndian,
                       Endian::BigEndian) == expected_exponent) {
        byte = Endian::BigEndian;
        word = Endian::LittleEndian;
    } else if (br.getExponent(Endian::LittleEndian,
                              Endian::BigEndian) == expected_exponent) {
        byte = Endian::LittleEndian;
        word = Endian::BigEndian;
    } else {
        // Only one option left that I know about:
        Q_ASSERT(br.getExponent(Endian::LittleEndian,
                                Endian::BigEndian) == expected_exponent);
        byte = Endian::LittleEndian;
        word = Endian::BigEndian;
    }
    if (byte != endianByteOrder()) {
        qFatal("Lack of programmer understanding of byte order; fix me!");
    }
    return word;
}
