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

// #define DEBUG_GEOMETRIC_MEAN

#include "mathfunc.h"
#include <QtMath>  // for e.g. qSqrt()
#include <QObject>
#include <QString>
#include "common/textconst.h"
#include "lib/convert.h"
#include "maths/eigenfunc.h"
#include "maths/floatingpoint.h"
#include "maths/mlpackfunc.h"
#include "maths/statsfunc.h"

namespace mathfunc {


// ============================================================================
// Basic sums
// ============================================================================

bool rangesOverlap(qreal a0, qreal a1, qreal b0, qreal b1)
{
    // There are two ranges: (a0, a1) and (b0, b1). Is there overlap?
    if (a0 > a1) {
        std::swap(a0, a1);
    }
    if (b0 > b1) {
        std::swap(b0, b1);
    }
    if (a1 < b0 || b1 < a0) {
        // A is entirely less than B, or B is entirely less than A.
        return false;
    }
    // Otherwise, there's overlap.
    return true;
}


bool nearlyEqual(const qreal x, const qreal y)
{
    // LESS GOOD: return qFuzzyIsNull(x - y);
    // BETTER:
    FloatingPoint<qreal> fx(x);
    FloatingPoint<qreal> fy(y);
    return fx.AlmostEquals(fy);
}


QVariant meanOrNull(const QVector<QVariant>& values, const bool ignore_null)
{
    double total = 0;
    int n = 0;
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (v.isNull()) {
            if (ignore_null) {
                continue;
            }
            return QVariant();  // mean of something including null is null
        }
        n += 1;
        total += v.toDouble();
    }
    if (n == 0) {
        return QVariant();
    }
    return total / n;
}


qreal mean(const qreal a, const qreal b)
{
    return (a + b) / 2;
}


int centile(const qreal x, const qreal minimum, const qreal maximum)
{
    const qreal fraction = (x - minimum) / (maximum - minimum);
    const qreal centile = 100 * fraction;
    if (!qIsFinite(centile)) {
        return -1;
    }
    return static_cast<int>(centile);  // truncates to int, which is what we want
}


double kahanSum(const QVector<double>& vec)
{
    // https://en.wikipedia.org/wiki/Kahan_summation_algorithm
    // https://codereview.stackexchange.com/questions/56532/kahan-summation
    double sum = 0.0;
    double c = 0.0;  // running compensation for lost low-order bits
    for (const double value : vec) {
        double y = value - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    return sum;
}


#if 1

// Simpler form of geometricMean()
double geometricMean(const QVector<double>& data)
{
    // The nth root of x1 * x2 * ... * xn.
    // Based on the simple method used by scipy.stats.mstats.gmean.
    // The principle is that:
    //      (x1 * x2 * ... * xn) ^ (1/n)
    //      = exp( log( (x1 * x2 * ... * xn) ^ (1/n) ) )
    //      = exp( (1/n) * log(x1 * x2 * ... * xn) )
    //          by log(x^y) = y * log(x)
    //      = exp( (log(x1) + log(x2) + ... + log(xn)) / n )
    //          by log(x*y) = log(x) + log(y)
    //      = exp(mean of log(x) elements)
    const int n = data.size();
    QVector<double> log_data(n);
    for (int i = 0; i < n; ++i) {
        log_data[i] = std::log(data.at(i));
    }
    const double mean_log = mean(log_data);
    return std::exp(mean_log);
}

#else

// More complex form of geometricMean()
double geometricMean(const QVector<double>& data)
{
    // The nth root of x1 * x2 * ... * xn.
    // See https://stackoverflow.com/questions/19980319/efficient-way-to-compute-geometric-mean-of-many-numbers
    // Modified because Qt uses int rather than std::size_t for its sizes.
    // Also clarified slightly.

    const int n = data.size();  // number of data points
    const double inv_n = 1.0 / n;
    double m = 1.0;  // cumulative mantissa
    long long ex = 0;  // cumulative exponent

    // This function iterates through part of "data" and multiplies our
    // running total (m * 2 ^ ex) by each part -- or at least, adds to the
    // running exponent total and returns the mantissa part (which the caller
    // then uses to alter m).
    auto doBucket = [&data, &ex](const int first, const int last) -> double
    {
        double ans = 1.0;
        int exponent;
        for (int i = first; i != last; ++i) {
#ifdef DEBUG_GEOMETRIC_MEAN
            const double old_ex = ex;
            const double old_ans = ans;
#endif
            ans *= std::frexp(data[i], &exponent);
            // See https://en.cppreference.com/w/cpp/numeric/math/frexp.
            // It decomposes its first argument into a normalized fraction
            // (return value) and an integral power of two. For example,
            // maps 123.45 to 0.964453 * 2^7.
            ex += exponent;
#ifdef DEBUG_GEOMETRIC_MEAN
            qDebug() << "doBucket: ex:" << old_ex << "->" << ex;
            qDebug() << "doBucket: ans:" << old_ans << "->" << ans;
#endif
        }
#ifdef DEBUG_GEOMETRIC_MEAN
        qDebug() << "doBucket: returning ans =" << ans;
#endif
        return ans;
    };

    // bucket_size = -log2(smallest double), i.e. a high positive number
    // See https://en.cppreference.com/w/cpp/types/numeric_limits
    const std::size_t bucket_size_t = static_cast<std::size_t>(
                -std::log2(std::numeric_limits<double>::min()));
    const int bucket_size = static_cast<int>(bucket_size_t);
    // Number of complete buckets
    const int n_buckets = n / bucket_size;  // integer division

    // Do all complete buckets
    for (int i = 0; i < n_buckets; ++i) {
#ifdef DEBUG_GEOMETRIC_MEAN
        const double old_m = m;
#endif
        m *= std::pow(doBucket(i * bucket_size, (i + 1) * bucket_size), inv_n);
#ifdef DEBUG_GEOMETRIC_MEAN
        qDebug() << "m:" << old_m << "->" << m;
#endif
    }
    // Finish off any residual elements
#ifdef DEBUG_GEOMETRIC_MEAN
    const double old_m = m;
#endif
    m *= std::pow(doBucket(n_buckets * bucket_size, n), inv_n);
#ifdef DEBUG_GEOMETRIC_MEAN
    qDebug() << "m:" << old_m << "->" << m;
#endif

    const int radix = std::numeric_limits<double>::radix;
    // QUESTION: will this function still work if radix is not 2, given that
    // frexp() guarantees to use base 2?

    // At this point, the product of our data is represented as
    // m * 2 ^ ex. We want to raise that to the power 1/n, so we use
    // m * 2 ^ (ex * [1/n]).
    const double result = std::pow(radix, ex * inv_n) * m;
#ifdef DEBUG_GEOMETRIC_MEAN
    qDebug().nospace()
            << "data = " << data
            << ", n = " << n
            << ", inv_n = " << inv_n
            << ", bucket_size_t = " << bucket_size_t
            << ", bucket_size = " << bucket_size
            << ", n_buckets = " << n_buckets
            << ", radix = " << radix
            << ", m = " << m
            << ", ex = " << ex
            << ", result = " << result;
#endif
    return result;
}

#endif


// ============================================================================
// QVariant operations, and QVariant collections
// ============================================================================

int sumInt(const QVector<QVariant>& values)
{
    int total = 0;
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        total += v.toInt();  // gives 0 if it is NULL
    }
    return total;
}


double sumDouble(const QVector<QVariant>& values)
{
    double total = 0;
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        total += v.toDouble();  // gives 0 if it is NULL
    }
    return total;
}


bool falseNotNull(const QVariant& value)
{
    if (value.isNull() || value.toBool()) {  // null or true
        return false;
    }
    return true;
}


bool allTrue(const QVector<QVariant>& values)
{
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (!v.toBool()) {
            return false;
        }
    }
    return true;
}


bool anyTrue(const QVector<QVariant>& values)
{
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (v.toBool()) {
            return true;
        }
    }
    return false;
}


bool allFalseOrNull(const QVector<QVariant>& values)
{
    return !anyTrue(values);
}


bool allFalse(const QVector<QVariant>& values)
{
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (v.isNull() || v.toBool()) {  // null or true
            return false;
        }
    }
    return true;
}


bool anyFalse(const QVector<QVariant> &values)
{
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (!v.isNull() && !v.toBool()) {  // not null and not true
            return true;
        }
    }
    return false;
}


bool anyNull(const QVector<QVariant>& values)
{
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (v.isNull()) {
            return true;
        }
    }
    return false;
}


bool noneNull(const QVector<QVariant>& values)
{
    return !anyNull(values);
}


bool anyNullOrEmpty(const QVector<QVariant>& values)
{
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (v.isNull() || v.toString().isEmpty()) {
            return true;
        }
    }
    return false;
}


bool noneNullOrEmpty(const QVector<QVariant>& values)
{
    return !anyNullOrEmpty(values);
}


int countTrue(const QVector<QVariant>& values)
{
    int n = 0;
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (v.toBool()) {
            n += 1;
        }
    }
    return n;
}


int countFalse(const QVector<QVariant>& values)
{
    int n = 0;
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (falseNotNull(v)) {
            n += 1;
        }
    }
    return n;
}


int countNull(const QVector<QVariant>& values)
{
    int n = 0;
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (v.isNull()) {
            n += 1;
        }
    }
    return n;
}


int countNotNull(const QVector<QVariant>& values)
{
    int n = 0;
    const int length = values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = values.at(i);
        if (!v.isNull()) {
            n += 1;
        }
    }
    return n;
}


bool eq(const QVariant& x, const int test)
{
    // SQL principle: NULL is not equal to anything
    return !x.isNull() && x.toInt() == test;
}


bool eq(const QVariant& x, const bool test)
{
    return !x.isNull() && x.toBool() == test;
}


bool eqOrNull(const QVariant& x, const int test)
{
    return x.isNull() || x.toInt() != test;
}


bool eqOrNull(const QVariant& x, const bool test)
{
    return x.isNull() || x.toBool() != test;
}


bool containsRespectingNull(const QVector<QVariant>& v, const QVariant& x)
{
    for (const QVariant& t : v) {
        if (t.isNull() != x.isNull()) {
            // this test is NOT performed by QVector::contains()
            continue;  // different
        }
        if (t == x) {
            // this test is performed by QVector::contains()
            return true;  // same
        }
    }
    return false;  // none the same
}


int countWhere(const QVector<QVariant>& test_values,
               const QVector<QVariant>& where_values)
{
    int n = 0;
    const int length = test_values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = test_values.at(i);
        if (containsRespectingNull(where_values, v)) {
            n += 1;
        }
    }
    return n;
}


int countWhereNot(const QVector<QVariant>& test_values,
                  const QVector<QVariant>& where_not_values)
{
    int n = 0;
    const int length = test_values.length();
    for (int i = 0; i < length; ++i) {
        const QVariant& v = test_values.at(i);
        if (!containsRespectingNull(where_not_values, v)) {
            n += 1;
        }
    }
    return n;
}


// ============================================================================
// Functions for scoring
// ============================================================================

QString percent(const double numerator,
                const double denominator,
                const int dp)
{
    const double pct = 100 * numerator / denominator;
    return convert::toDp(pct, dp) + "%";
}


QString scoreString(const int numerator,
                    const int denominator,
                    const bool show_percent,
                    const int dp)
{
    QString result = QString("<b>%1</b>/%2").arg(numerator).arg(denominator);
    if (show_percent) {
        result += " (" + percent(numerator, denominator, dp) + ")";
    }
    return result;
}


QString scoreString(const double numerator,
                    const int denominator,
                    const bool show_percent,
                    const int dp)
{
    QString result = QString("<b>%1</b>/%2")
            .arg(convert::toDp(numerator, dp))
            .arg(denominator);
    if (show_percent) {
        result += " (" + percent(numerator, denominator, dp) + ")";
    }
    return result;
}


QString scoreStringVariant(const QVariant& numerator, const int denominator,
                           const bool show_percent, const int dp)
{
    QString result = QString("<b>%1</b>/%2")
            .arg(convert::prettyValue(numerator, dp))
            .arg(denominator);
    if (show_percent) {
        result += " (" + percent(numerator.toDouble(), denominator, dp) + ")";
    }
    return result;
}


QString scoreStringWithPercent(const int numerator,
                               const int denominator,
                               const int dp)
{
    return scoreString(numerator, denominator, true, dp);
}


QString scorePhrase(const QString& description,
                    const int numerator, const int denominator,
                    const QString& separator, const QString& suffix)
{
    return QString("%1%2%3%4")
            .arg(description,
                 separator,
                 scoreString(numerator, denominator, false),
                 suffix);
}


QString scorePhrase(const QString& description,
                    const double numerator, const int denominator,
                    const QString& separator, const QString& suffix,
                    const int dp)
{
    return QString("%1%2%3%4")
            .arg(description,
                 separator,
                 scoreString(numerator, denominator, false, dp),
                 suffix);
}


QString scorePhraseVariant(const QString& description,
                           const QVariant& numerator, const int denominator,
                           const QString& separator, const QString& suffix,
                           const int dp)
{
    return QString("%1%2%3%4")
            .arg(description,
                 separator,
                 scoreStringVariant(numerator, denominator, false, dp),
                 suffix);
}


QString totalScorePhrase(const int numerator, const int denominator,
                         const QString& separator, const QString& suffix)
{
    return scorePhrase(TextConst::totalScore(), numerator, denominator,
                       separator, suffix);
}


QString totalScorePhrase(const double numerator, const int denominator,
                         const QString& separator, const QString& suffix,
                         const int dp)
{
    return scorePhrase(TextConst::totalScore(), numerator, denominator,
                       separator, suffix, dp);
}


// ============================================================================
// Sequence and range generation
// ============================================================================

QVector<int> range(const int start, const int end)
{
    return seq(start, end - 1, 1);
}


QVector<int> range(const int n)
{
    // returns 0 to n-1 inclusive
    return range(0, n);
}


// ============================================================================
// Range description (cosmetic)
// ============================================================================

QString describeAsRanges(QVector<int> numbers,
                         const QString& element_prefix,
                         const QString& element_separator,
                         const QString& range_separator)
{
    // Converts e.g. 1, 2, 3, 5, 6, 7, 10 to "1-3, 5-7, 10"
    std::sort(numbers.begin(), numbers.end());
    const int n = numbers.size();
    QString result;
    bool in_range = false;
    int previous = 0;  // value is arbitrary; removes a linter warning
    for (int i = 0; i < n; ++i) {
        const int current = numbers.at(i);
        if (i == n - 1) {
            // Last number. Must print it.
            if (in_range) {
                result += range_separator;
            } else if (i > 0) {
                result += element_separator;
            }
            result += element_prefix + QString::number(current);
        }
        else if (i != 0 && current == previous + 1) {
            // Current number is the continuation of a range. Don't print it.
            in_range = true;
        } else {
            // Current number is not a continuation of a range, or is the last
            // number. Print it somehow.
            if (in_range) {
                // Finishing a previous range.
                result += range_separator + element_prefix + QString::number(previous)
                        + element_separator + element_prefix + QString::number(current);
                in_range = false;
            } else {
                // Starting a new standalone number (or the start of a range).
                if (i > 0) {
                    result += element_separator;
                }
                result += element_prefix + QString::number(current);
            }
        }
        previous = current;
    }
    return result;
}

// ============================================================================
// Spacing things out
// ============================================================================

QVector<qreal> distribute(const int n, qreal minimum, qreal maximum)
{
    QVector<qreal> posts;
    if (n <= 0) {
        return posts;  // or we'll have division by zero shortly
    }
    if (maximum < minimum) {
        std::swap(minimum, maximum);
    }
    const qreal extent = maximum - minimum;
    const qreal each = extent / n;
    // ... (double / int) gives double
    // https://stackoverflow.com/questions/5563000/implicit-type-conversion-rules-in-c-operators
    const qreal centre_offset = each / 2;
    for (int i = 0; i < n; ++i) {
        posts.append(minimum + i * each + centre_offset);
    }
    return posts;
}


QPair<int, int> gridDimensions(const int n, const qreal aspect)
{
    const int y = qCeil(qSqrt(n / aspect));
    const int x = qCeil(n / y);
    return QPair<int, int>(x, y);
}


// ============================================================================
// Numerical conversions
// ============================================================================

int proportionToByte(const qreal proportion)
{
    // convert 0.0-1.0 to 0-255
    const int a = qRound(qBound(0.0, proportion, 1.0) * 255);
    return qBound(0, a, 255);
}


qreal byteToProportion(const int byte)
{
    // convert 0-255 to 0.0-1.0
    return qBound(0, byte, 255) / 255.0;
}


int proportionToIntPercent(const qreal proportion)
{
    // convert 0.0-1.0 to 0-100
    const int a = qRound(qBound(0.0, proportion, 1.0) * 100);
    return qBound(0, a, 100);
}


qreal intPercentToProportion(const int percent)
{
    // convert 0-100 to 0.0-1.0
    return qBound(0, percent, 100) / 100.0;
}


QStringList testMaths()
{
    QStringList lines;

    // geometricMean()
    const QVector<QPair<QVector<double>, double>> gm_tests{
        {{1, 4}, 2},
        {{2, 8}, 4},  // geometric mean of 2 and 8 is 4
        {{4, 9}, 6},
        {{1, 2, 3, 4, 5, 6, 7}, 3.3800151591412964},  // scipy gmean example
    };
    for (const auto& pair : gm_tests) {
        const auto& q = pair.first;
        const double correct_a = pair.second;
        const double a = geometricMean(q);
        const bool ok = qFuzzyCompare(a, correct_a);
        lines.append(QString("geometricMean(%1) -> %2 [%3]").arg(
                         convert::numericVectorToCsvString(q),
                         QString::number(a),
                         ok ? QObject::tr("true") : QObject::tr("WRONG")));
    }

    return lines;
}


}  // namespace mathfunc
