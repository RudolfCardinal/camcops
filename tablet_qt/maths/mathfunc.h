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

#include <cmath>  // for std::floor, etc.
// #include <QtAlgorithms>  // for qsort()
#include <QVariant>
#include <QVector>


namespace mathfunc {

// ============================================================================
// Basic sums
// ============================================================================

// Sign function. Returns -1 if val is negative, 0 if zero, and +1 if positive.
template<typename T>
int sgn(T val)
{
    // http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
    return (T(0) < val) - (val < T(0));
}


// Returns x mod y, coping with negatives.
template<typename T>
T mod(T x, T y)
{
    // http://stackoverflow.com/questions/11980292/how-to-wrap-around-a-range
    if (y == 0) {
        return 0;  // stupid caller
    }
    return x - y * std::floor(x / y);
}

// Implemented in GNU C++11 standard library but not available for Android
// (e.g. https://github.com/android-ndk/ndk/issues/82 ),
// and not offered by <QtGlobal>

// Truncate towards zero.
template<typename T>
T trunc(const T& x)
{
    // http://en.cppreference.com/w/cpp/numeric/math/trunc
    // http://en.cppreference.com/w/cpp/numeric/math/floor
    // For x >= 0, floor and trunc are the same.
    // For x < 0, floor moves towards -Inf and trunc moves towards 0.
    // Example: floor(1.5) == trunc(1.5) == 1.0
    // Example: floor(-1.5) == -2.0; trunc(-1.5) == -1.0
    // The answer:
    // https://stackoverflow.com/questions/6709405/is-there-a-trunc-function-in-c

    return x > 0 ? std::floor(x) : std::ceil(x);
}


// Does the range [a0, a1] overlap with [b0, b1]?
bool rangesOverlap(qreal a0, qreal a1, qreal b0, qreal b1);

// Are two floating-point numbers nearly equal?
// (Several ways of doing this; see code.)
bool nearlyEqual(qreal x, qreal y);

// Return the mean of the supplied values.
// - ignore_null true: return the mean of the values, ignoring any NULLs.
// - ignore_null false: return the mean, or NULL if any are NULL.
QVariant meanOrNull(const QVector<QVariant>& values, bool ignore_null = false);

// Return the mean of two numbers.
qreal mean(qreal a, qreal b);

// Return the mean of a numeric QVector
template<typename T>
double mean(const QVector<T>& data)
{
    // https://codereview.stackexchange.com/questions/109994/mean-median-and-mode-of-a-qvector
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

// Returns the (integer) centile of x within the range [minimum, maximum].
// (So if x == minimum, this will be 0; if x == maximum, it will be 100.)
int centile(qreal x, qreal minimum, qreal maximum);

// Adds up numbers, minimizing error. See code.
double kahanSum(const QVector<double>& vec);

// Geometric mean (the nth root of x1 * x2 * ... * xn)
double geometricMean(const QVector<double>& data);

// ============================================================================
// QVariant operations, and QVariant collections
// ============================================================================

// Sum of integers from QVariant objects.
int sumInt(const QVector<QVariant>& values);

// Sum of doubles from QVariant objects.
double sumDouble(const QVector<QVariant>& values);

// Is the QVariant false, but not null?
bool falseNotNull(const QVariant& value);

// Are all the values true?
bool allTrue(const QVector<QVariant>& values);

// Are any of the values true?
bool anyTrue(const QVector<QVariant>& values);

// Are all of the values false or null?
bool allFalseOrNull(const QVector<QVariant>& values);

// Are all of the values false (not true or null)?
bool allFalse(const QVector<QVariant>& values);

// Are any of the values false (not true or null)?
bool anyFalse(const QVector<QVariant>& values);

// Are any of the values null?
bool anyNull(const QVector<QVariant>& values);

// Are none of the values null?
bool noneNull(const QVector<QVariant>& values);

// Are any of the values null or empty strings?
bool anyNullOrEmpty(const QVector<QVariant>& values);

// Are none of the values null or empty strings?
bool noneNullOrEmpty(const QVector<QVariant>& values);

// Return the number of values that are true.
int countTrue(const QVector<QVariant>& values);

// Return the number of values that are false (not true or null).
int countFalse(const QVector<QVariant>& values);

// Return the number of values that are null.
int countNull(const QVector<QVariant>& values);

// Return the number of values that are not null.
int countNotNull(const QVector<QVariant>& values);

// Does x equal test, using the SQL principle that null is not equal to anything?
bool eq(const QVariant& x, int test);

// Does x equal test, using the SQL principle that null is not equal to anything?
bool eq(const QVariant& x, bool test);

// Is x null or equal to test?
bool eqOrNull(const QVariant& x, int test);

// Is x null or equal to test?
bool eqOrNull(const QVariant& x, bool test);

// Does the vector v contains the value x?
// This differs from QVector::contains() in that QVector::contains() uses
// operator==(), which gives true for e.g. QVariant() == QVariant(0), i.e.
// it allows null values to compare equal to their non-null "equivalents".
// This function does not.
bool containsRespectingNull(const QVector<QVariant>& v, const QVariant& x);

// Return the number of values in test_values that are present in where_values.
// 2019-08-20: respects the difference between NULL and not-NULL values.
int countWhere(const QVector<QVariant>& test_values,
               const QVector<QVariant>& where_values);

// Return the number of values in test_values that are not present in
// where_not_values.
// 2019-08-20: respects the difference between NULL and not-NULL values.
int countWhereNot(const QVector<QVariant>& test_values,
                  const QVector<QVariant>& where_not_values);

// ============================================================================
// Functions for scoring
// ============================================================================

// Returns numerator/denominator as a percentage, e.g. "53.2%".
QString percent(double numerator, double denominator, int dp = 1);

// Returns e.g. "<b>27</b>/30"; optionally add " (90%)"
QString scoreString(int numerator, int denominator,
                    bool show_percent = false, int dp = 1);

// Returns e.g. "<b>27.5</b>/30"; optionally add " (91.7%)"
QString scoreString(double numerator, int denominator,
                    bool show_percent = false, int dp = 1);

// Returns e.g. "<b>27.5</b>/30"; optionally add " (91.7%)"
QString scoreStringVariant(const QVariant& numerator, int denominator,
                           bool show_percent = false, int dp = 1);

// Returns e.g. "<b>27</b>/30 (90%)"
QString scoreStringWithPercent(int numerator, int denominator, int dp = 1);

// Returns e.g. "<b>27.5</b>/30 (91.7%)"
QString scoreStringWithPercent(double numerator, int denominator, int dp = 1);

// Returns e.g. "Description: <b>27</b>/30."
QString scorePhrase(const QString& description, int numerator, int denominator,
                    const QString& separator = ": ",
                    const QString& suffix = ".");

// Returns e.g. "Description: <b>27.5</b>/30."
QString scorePhrase(const QString& description, double numerator, int denominator,
                    const QString& separator = ": ",
                    const QString& suffix = ".",
                    int dp = 1);

// Returns e.g. "Description: <b>27.5</b>/30."
QString scorePhraseVariant(
        const QString& description, const QVariant& numerator, int denominator,
        const QString& separator = ": ",
        const QString& suffix = ".",
        int dp = 1);

// Returns e.g. "Total score: <b>27</b>/30."
QString totalScorePhrase(int numerator, int denominator,
                         const QString& separator = ": ",
                         const QString& suffix = ".");

// Returns e.g. "Total score: <b>27.5</b>/30."
QString totalScorePhrase(double numerator, int denominator,
                         const QString& separator = ": ",
                         const QString& suffix = ".",
                         int dp = 1);

// ============================================================================
// Sequence and range generation
// ============================================================================

// Generates a vector (e.g. of numbers) from "first" to "last", step "step".
template<typename T>
QVector<T> seq(T first, T last, T step = 1)
{
    QVector<T> v;
    if (step > 0) {
        for (T i = first; i <= last; i += step) {
            v.append(i);
        }
    } else if (step < 0) {
        for (T i = first; i >= last; i -= step) {
            v.append(i);
        }
    }
    return v;
}


// Generates a vector of integers: [start, end),
// i.e. start to (end - 1) inclusive
QVector<int> range(int start, int end);


// Generates a vector of integers: [0, n), i.e. 0 to (n - 1) inclusive
QVector<int> range(int n);


// Generates a vector containing n copies of x.
template<typename T>
QVector<T> rep(const T& x, int n)
{
    return QVector<T>(n, x);
}


// Like R. For example, rep(QVector<int>{1,2,3}, 2, 4) gives
// {1, 1, 2, 2, 3, 3, 1, 1, 2, 2, 3, 3, 1, 1, 2, 2, 3, 3, 1, 1, 2, 2, 3, 3}.
template<typename T>
QVector<T> rep(const QVector<T>& values, int each, int times)
{
    QVector<T> result;
    for (int t = 0; t < times; ++t) {
        for (const T& x : values) {
            for (int e = 0; e < each; ++e) {
                result.append(x);
            }
        }
    }
    return result;
}


// ============================================================================
// Range description (cosmetic)
// ============================================================================

// Takes a vector like {1, 2, 3, 4, 6, 7, 8, 10} and returns a descriptive
// string like "1-4, 6-8, 10". Optionally add a prefix to each part, like
QString describeAsRanges(QVector<int> numbers,
                         const QString& element_prefix = "",
                         const QString& element_separator = ", ",
                         const QString& range_separator = "–");

// ============================================================================
// Spacing things out
// ============================================================================

// Fence/fence-post problem; return the centre of each of n fence segments
// spanning [minimum, maximum].
QVector<qreal> distribute(int n, qreal minimum, qreal maximum);

// Work out the dimensions of the smallest grid that will hold n objects and
// is of (approximately) a given aspect ratio.
//
// Solve the equations:
//      x * y >= n
//      aspect ~= x / y
// ... for smallest x, y. Thus:
//      x = aspect * y
//      aspect * y * y >= n
//
// Returns x, y.
QPair<int, int> gridDimensions(int n, qreal aspect = 1.0);

// ============================================================================
// Numerical conversions
// ============================================================================

// Converts 0.0-1.0 to 0-255
int proportionToByte(qreal proportion);

// Converts 0-255 to 0.0-1.0
qreal byteToProportion(int byte);

// Converts 0.0-1.0 to 0-100
int proportionToIntPercent(qreal proportion);

// Converts 0-100 to 0.0-1.0
qreal intPercentToProportion(int percent);

// Test maths functions
QStringList testMaths();

}  // namespace mathfunc
