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

#include "ccrandom.h"
#include <cfloat>
#include <QDebug>
#include <QMultiMap>
#include "maths/countingcontainer.h"
#include "maths/floatbits.h"
#include "maths/mathfunc.h"

namespace ccrandom {


std::random_device rd;
std::mt19937 rng(rd());


bool coin(const qreal p)
{
    std::bernoulli_distribution dist(p);
    return dist(rng);
}


int randomInt(const int minimum, const int maximum)
{
    // [minimum, maximum] -- i.e. inclusive
    std::uniform_int_distribution<int> dist(minimum, maximum);
    return dist(rng);
}


double randomRealExcUpper(const double minimum, const double maximum)
{
    // [minimum, maximum) -- i.e. includes lower but not upper bound
    // - http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
    //
    // Make this explicitly "double", not float, given some "float"
    // implemetation bugs, as per
    // - http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
    //
    // Nope - we still get the bug with double (GCC 5.4.0).
    // - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63176
    // - http://open-std.org/JTC1/SC22/WG21/docs/lwg-active.html#2524
    //
    // So, implement GCC suggestion 2: re-run if you get the maximum.

    std::uniform_real_distribution<double> dist(minimum, maximum);
    double result;
    do {
        result = dist(rng);
    } while (result == maximum);
    return result;

    // Yup, that works.
}


float nextFloatAbove(const float x)
{
    // https://stackoverflow.com/questions/16335992/alternative-to-c11s-stdnextafter-and-stdnexttoward-for-c03
    // https://randomascii.wordpress.com/2012/01/23/stupid-float-tricks-2/
    if (std::numeric_limits<float>::is_iec559) {
        // http://en.cppreference.com/w/cpp/types/numeric_limits/is_iec559
        BitRepresentationFloat brf(x);
        if (!brf.isMaximum()) {
            ++brf.i;
        }
        return brf.f;
    }
    qFatal("nextFloatAbove: machine/compiler not using IEC559 (IEEE 754) "
           "for float");
}


double nextDoubleAboveManual(const double x)
{
    if (std::numeric_limits<double>::is_iec559) {
        BitRepresentationDouble brd(x);
        if (!brd.isMaximum()) {
            ++brd.i;
        }
        return brd.d;
    }
    qFatal("nextDoubleAbove: machine/compiler not using IEC559 (IEEE 754) "
           "for double");
}


double nextDoubleAbove(const double x)
{
    // Detecting the build type:
    // https://stackoverflow.com/questions/6374523/how-to-detect-compilation-by-android-ndk-in-a-c-c-file
#ifdef __ANDROID__
    return nextDoubleAboveManual(x);
#else
    return std::nextafter(x, std::numeric_limits<double>::max());
#endif
}


double randomRealIncUpper(const double minimum, const double maximum)
{
    // [minimum, maximum] -- i.e. inclusive
    // http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
    const double adjusted_max = nextDoubleAbove(maximum);
    return randomRealExcUpper(minimum, adjusted_max);
}


QStringList testRandom()
{
    QStringList lines;

    // https://stackoverflow.com/questions/16839658/printf-width-specifier-to-maintain-precision-of-floating-point-value
    auto fullFloat = [](float f) -> QString {
        return QString::number(static_cast<double>(f), 'g', 9);
    };
    auto fullDouble = [](double d) -> QString {
        return QString::number(d, 'g', 17);
    };

    auto testNextFloatAbove = [&fullFloat, &lines](float f) -> void {
        const float nf = nextFloatAbove(f);
        const BitRepresentationFloat brf(f);
        const BitRepresentationFloat brnf(nf);
        lines.append(QString("nextFloatAbove(%1 [integer representation %2]) "
                             "-> %3 [integer representation %4]")
                     .arg(fullFloat(f),
                          QString::number(brf.ui),
                          fullFloat(nf),
                          QString::number(brnf.ui)));
    };
    auto testNextDoubleAbove = [&fullDouble, &lines](double d) -> void {
        const double dam = nextDoubleAboveManual(d);
        const double da = nextDoubleAbove(d);
        const BitRepresentationDouble brd(d);
        const BitRepresentationDouble brdam(dam);
        const BitRepresentationDouble brda(da);
        lines.append(
            QString("nextDoubleAboveManual(%1 [integer representation %2]) "
                    "-> %3 [integer representation %4]")
                     .arg(fullDouble(d),
                          QString::number(brd.ui),
                          fullDouble(dam),
                          QString::number(brdam.ui)));
        lines.append(
            QString("nextDoubleAbove(%1 [integer representation %2]) "
                    "-> %3 [integer representation %4]")
                     .arg(fullDouble(d),
                          QString::number(brd.ui),
                          fullDouble(da),
                          QString::number(brda.ui)));
    };

    auto testRangeSampling = [&fullDouble, &lines]
            (qreal range_min, qreal range_max, int range_n) -> void {
        qreal exc_min = std::numeric_limits<double>::max();  // start high
        qreal exc_max = -exc_min;  // start low
        qreal inc_min = exc_min;
        qreal inc_max = exc_max;
        CountingContainer<int> exc_centiles;
        CountingContainer<int> inc_centiles;
        for (int i = 0; i < range_n; ++i) {
            qreal draw_exc = randomRealExcUpper(range_min, range_max);
            exc_min = qMin(exc_min, draw_exc);
            exc_max = qMax(exc_max, draw_exc);
            int exc_centile = mathfunc::centile(draw_exc, range_min, range_max);
            exc_centiles.add(exc_centile);

            qreal draw_inc = randomRealIncUpper(range_min, range_max);
            inc_min = qMin(inc_min, draw_inc);
            inc_max = qMax(inc_max, draw_inc);
            int inc_centile = mathfunc::centile(draw_inc, range_min, range_max);
            inc_centiles.add(inc_centile);
        }
        lines.append(QString("Draw from upper-exclusive range [%1–%2): "
                             "min %3, max %4, centiles %5")
                     .arg(fullDouble(range_min),
                          fullDouble(range_max),
                          fullDouble(exc_min),
                          fullDouble(exc_max),
                          exc_centiles.asString()));
        lines.append(QString("Draw from upper-inclusive range [%1–%2]: "
                             "min %3, max %4, centiles %5")
                     .arg(fullDouble(range_min),
                          fullDouble(range_max),
                          fullDouble(inc_min),
                          fullDouble(inc_max),
                          inc_centiles.asString()));
    };

    // ========================================================================

    lines.append("Testing std::nextafter() [if available on this platform, "
                 "via nextDoubleAbove()], and manual versions: "
                 "nextFloatAbove(), nextDoubleAboveManual()");
    const QVector<float>  fv{1.0, 100.0, 1.0e10};
    const QVector<double> dv{1.0, 100.0, 1.0e10, 1.0e100};
    for (float f : fv) {
        testNextFloatAbove(f);
    }
    for (double d : dv) {
        testNextDoubleAbove(d);
    }

    lines.append("");
    lines.append("Testing random number generation functions");
    lines.append("");

    const int coin_n = 2000;
    const qreal coin_p = 0.5;
    CountingContainer<bool> coins;
    for (int i = 0; i < coin_n; ++i) {
        coins.add(coin(coin_p));
    }
    lines.append(QString("Coin flips (n=%1, p=%2): %3")
                 .arg(QString::number(coin_n),
                      QString::number(coin_p),
                      coins.asString()));
    lines.append("");

    const int die_n = 6000;
    CountingContainer<int> die;
    for (int i = 0; i < die_n; ++i) {
        die.add(randomInt(1, 6));
    }
    lines.append(QString("Rolls of a fair die (n=%1): %2")
                 .arg(QString::number(die_n),
                      die.asString()));
    lines.append("");

    const int range_n = 100000;
    testRangeSampling(0.0,
                      1.0,
                      range_n);
    testRangeSampling(1.0,
                      nextDoubleAbove(nextDoubleAbove(nextDoubleAbove(1.0))),
                      range_n);

    return lines;
}


}  // namespace ccrandom
