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
#include <algorithm>
#include <random>
#include <QtGlobal>
#include <QStringList>
#include <QVector>


namespace ccrandom {  // not "random"; conflicts

// Device to generate randomness
extern std::random_device rd;

// Random number generator
extern std::mt19937 rng;

// Flip a (biased) coin; return true with probability p, and false with
// probability (1 - p). See
// https://en.cppreference.com/w/cpp/numeric/random/bernoulli_distribution.
bool coin(qreal p = 0.5);

// Returns a random integer in the range [minimum, maximum], i.e. inclusive.
int randomInt(int minimum, int maximum);

// Returns a random double in the range [minimum, maximum),
// i.e. includes lower but not upper bound
double randomRealExcUpper(double minimum, double maximum);

// Returns the smallest possible float that is larger than f.
// This operates with the bitwise representation of floating-point numbers.
float nextFloatAbove(float x);

// Returns the smallest possible double that is larger than x.
// This operates with the bitwise representation of floating-point numbers.
double nextDoubleAboveManual(double x);

// Returns the smallest possible double that is larger than x.
// This uses std::nextafter(), except for operating systems that don't offer
// it, in which case it uses nextDoubleAboveManual().
double nextDoubleAbove(double x);

// Returns a random double in the range [minimum, maximum], i.e. inclusive.
double randomRealIncUpper(double minimum, double maximum);

// Returns a random index for the supplied vector.
template<typename T>
int randomIndex(const QVector<T>& vec)
{
    if (vec.isEmpty()) {
        return -1;
    }
    return randomInt(0, vec.size() - 1);
}


// Draw without replacement: removes and returns a random element from the
// vector.
template<typename T>
T dwor(QVector<T>& bucket)
{
    if (bucket.isEmpty()) {
        return T();
    }
    int index = randomIndex(bucket);
    return bucket.takeAt(index);
}


// Draw with replacement: returns a random element from the vector, and leaves
// it in there.
template<typename T>
T drawreplace(const QVector<T>& bucket)
{
    if (bucket.isEmpty()) {
        return T();
    }
    int index = randomIndex(bucket);
    return bucket.at(index);
}


// Randomly shuffles a vector.
template<typename T>
void shuffle(QVector<T>& vec)
{
    std::shuffle(vec.begin(), vec.end(), rng);
}


// Test randomness functions.
QStringList testRandom();


}  // namespace ccrandom
