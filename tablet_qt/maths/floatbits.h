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

// Builds on ieee754.h slightly.

#pragma once
#include <cstdint>
#include <float.h>  // for FLT_MAX etc.
#include "maths/endian.h"


/*

This is to get round the lack of std::nextafter when compiling for Android.
See ccrandom.cpp.

- https://randomascii.wordpress.com/2012/01/23/stupid-float-tricks-2/
- Note that this requires "type-punning" via a union:
- https://stackoverflow.com/questions/11373203/accessing-inactive-union-member-and-undefined-behavior
- https://stackoverflow.com/questions/11639947/is-type-punning-through-a-union-unspecified-in-c99-and-has-it-become-specified
- https://stackoverflow.com/questions/3529394/obtain-minimum-negative-float-value-in-c

Note also that we cannot derive from a union, so we can't add an integer type
to the union of ieee754_float. So, we have to do our own...

Detecting endianness across compilers: do it at runtime.
- https://stackoverflow.com/questions/8978935/detecting-endianness

*/



union BitRepresentationFloat
{
public:
    // Based on RUNTIME, not compile-time, checks for endian-ness.
    explicit BitRepresentationFloat(float num) { f = num; }
    bool isNegative() { return f < 0; }
    bool isMaximum() const { return f == FLT_MAX; }
    bool isMinimum() const { return f == -FLT_MAX; }
    uint32_t getNegative(Endian endian)
    {
        return endian == Endian::BigEndian ? ieee_bigendian.negative
                                           : ieee_littleendian.negative;
    }
    uint32_t getExponent(Endian endian)
    {
        return endian == Endian::BigEndian ? ieee_bigendian.exponent
                                           : ieee_littleendian.exponent;
    }
    uint32_t getMantissa(Endian endian)
    {
        return endian == Endian::BigEndian ? ieee_bigendian.mantissa
                                           : ieee_littleendian.mantissa;
    }

public:
    float f;

    // This is the IEEE 754 single-precision format.
    struct {
        unsigned int negative : 1;
        unsigned int exponent : 8;
        unsigned int mantissa : 23;
    } ieee_bigendian;
    struct {
        unsigned int mantissa : 23;
        unsigned int exponent : 8;
        unsigned int negative : 1;
    } ieee_littleendian;

    // This format makes it easier to see if a NaN is a signalling NaN.
    struct {
        unsigned int negative : 1;
        unsigned int exponent : 8;
        unsigned int quiet_nan : 1;
        unsigned int mantissa : 22;
    } ieee_nan_bigendian;
    struct {
        unsigned int mantissa : 22;
        unsigned int quiet_nan : 1;
        unsigned int exponent : 8;
        unsigned int negative : 1;
    } ieee_nan_littleendian;

    // RNC: integer versions
    int32_t i;
    uint32_t ui;
};


union BitRepresentationDouble
{
public:
    // Based on RUNTIME, not compile-time, checks for endian-ness.
    explicit BitRepresentationDouble(double num) { d = num; }
    bool isNegative() { return d < 0; }
    bool isMaximum() const { return d == DBL_MAX; }
    bool isMinimum() const { return d == -DBL_MAX; }
    uint64_t getMantissa(Endian byte, Endian word) const
    {
        return makeMantissa(getMantissa0(byte, word), getMantissa1(byte, word));
    }
    uint64_t getMantissa0(Endian byte, Endian word) const
    {
        return byte == Endian::BigEndian
            ? ieee_bytebigendian.mantissa0
            : (word == Endian::BigEndian
               ? ieee_bytelittle_floatwordbigendian.mantissa0
               : ieee_bytelittle_floatwordlittleendian.mantissa0);
    }
    uint64_t getMantissa1(Endian byte, Endian word) const
    {
        return byte == Endian::BigEndian
            ? ieee_bytebigendian.mantissa1
            : (word == Endian::BigEndian
               ? ieee_bytelittle_floatwordbigendian.mantissa1
               : ieee_bytelittle_floatwordlittleendian.mantissa1);
    }
    uint64_t getExponent(Endian byte, Endian word) const
    {
        return byte == Endian::BigEndian
            ? ieee_bytebigendian.exponent
            : (word == Endian::BigEndian
               ? ieee_bytelittle_floatwordbigendian.exponent
               : ieee_bytelittle_floatwordlittleendian.exponent);
    }
    uint64_t getNegative(Endian byte, Endian word) const
    {
        return byte == Endian::BigEndian
            ? ieee_bytebigendian.negative
            : (word == Endian::BigEndian
               ? ieee_bytelittle_floatwordbigendian.negative
               : ieee_bytelittle_floatwordlittleendian.negative);
    }

private:
    uint64_t makeMantissa(uint64_t mantissa0, uint64_t mantissa1) const
    {
        // See the big-endian format, which involves no mental reversals:
        // mantissa0 contains the HIGH bits and mantissa1 the LOW bits.
        return mantissa0 << 32 | mantissa1;
    }

public:
    double d;

    // This is the IEEE 754 double-precision format.
    struct {
        unsigned int negative : 1;
        unsigned int exponent : 11;
        // Together these comprise the mantissa:
        unsigned int mantissa0 : 20;
        unsigned int mantissa1 : 32;
    } ieee_bytebigendian;
    struct {
        unsigned int mantissa0 : 20;
        unsigned int exponent : 11;
        unsigned int negative : 1;
        unsigned int mantissa1 : 32;
    } ieee_bytelittle_floatwordbigendian;
    struct {
        // Together these comprise the mantissa.
        unsigned int mantissa1 : 32;
        unsigned int mantissa0 : 20;
        unsigned int exponent : 11;
        unsigned int negative : 1;
    } ieee_bytelittle_floatwordlittleendian;

    // This format makes it easier to see if a NaN is a signalling NaN.
    struct {
        unsigned int negative : 1;
        unsigned int exponent : 11;
        unsigned int quiet_nan : 1;
        // Together these comprise the mantissa:
        unsigned int mantissa0 : 19;
        unsigned int mantissa1 : 32;
    } ieee_nan_bytebigendian;
    struct {
            unsigned int mantissa0 : 19;
            unsigned int quiet_nan : 1;
            unsigned int exponent : 11;
            unsigned int negative : 1;
            unsigned int mantissa1 : 32;
    } ieee_nan_bytelittle_floatwordbigendian;
    struct {
        // Together these comprise the mantissa.
        unsigned int mantissa1 : 32;
        unsigned int mantissa0 : 19;
        unsigned int quiet_nan : 1;
        unsigned int exponent : 11;
        unsigned int negative : 1;
    } ieee_nan_bytelittle_floatwordlittleendian;

    // RNC: integer versions
    int64_t i;
    uint64_t ui;
};
