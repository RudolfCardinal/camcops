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
#include <QVector>


namespace containers
{

// ============================================================================
// Force a QVector to be a certain size, by removing items at the end or by
// adding default items.
// ============================================================================

template<typename T>
void forceVectorSize(QVector<T>& vec,
                     int new_size,
                     const T& default_value = T())
{
    int old_size = vec.size();
    // Remove surplus
    for (int i = old_size; i > new_size; --i) {
        vec.pop_back();
    }
    // Add new
    for (int i = old_size; i < new_size; ++i) {
        vec.append(default_value);
    }
}


// ============================================================================
// subtract(a, b) -> items in a, in order, that are not in b
// ============================================================================
// - works for QVector, QList

template<typename ContainerType>
ContainerType subtract(const ContainerType& first, const ContainerType& second)
{
    ContainerType difference;
    for (auto first_item : first) {
        if (!second.contains(first_item)) {
            difference.append(first_item);
        }
    }
    return difference;
}


// ============================================================================
// setSubtract(a, b) -> items in a, in order, that are not in b, eliminating
//                      duplicates in a
// ============================================================================
// - works for QVector, QList

template<typename ContainerType>
ContainerType setSubtract(const ContainerType& first,
                          const ContainerType& second)
{
    ContainerType difference;
    for (auto first_item : first) {
        if (!second.contains(first_item) && !difference.contains(first_item)) {
            difference.append(first_item);
        }
    }
    return difference;
}



// ============================================================================
// at(a, indices) -> container of items in "a" at locations "indices"
//                 = generalization of "at" for multiple indices
// ============================================================================
/*
Example:

    QVector<QString> v{"zero", "one", "two", "three", "four"};
    QVector<QString> v2 = at(v, {1, 3});
    assert(v2 == QVector<QString>{"one", "three"});
*/

template<typename ContainerType>
ContainerType at(const ContainerType& vec, const QVector<int>& indices)
{
    ContainerType subset;
    for (int index : indices) {
        subset.append(vec.at(index));
    }
    return subset;
}


// ============================================================================
// containsAll(a, b) -> does a contain all elements of b?
// ============================================================================

template<typename ContainerType>
bool containsAll(const ContainerType& a, const ContainerType& b)
{
    // Does a contain all elements of b?
    for (auto b_element : b) {
        if (!a.contains(b_element)) {
            return false;
        }
    }
    return true;
}


// ============================================================================
// rotateSequence(a, n) -> container of elements of a, rotated
// ... e.g. if a is {1, 2, 3, 4}, then rotateSequence(a, 2) is {3, 4, 1, 2}
// ============================================================================

template<typename ContainerType>
ContainerType rotateSequence(const ContainerType& v, int n_rotate)
{
    int size = v.size();
    n_rotate = n_rotate % size;  // don't do unnecessary work
    if (n_rotate <= 0) {
        return v;
    }
    ContainerType newvec(size);
    int new_index;
    for (int old_index = 0; old_index < size; ++old_index) {
        new_index = (old_index + n_rotate) % size;
        newvec[new_index] = v.at(old_index);
    }
    return newvec;
}


// ============================================================================
// rotateSequenceInPlace(a, n) -> in-place version of rotateSequence()
// ============================================================================

template<typename ContainerType>
void rotateSequenceInPlace(ContainerType& v, int n_rotate)
{
    v = rotateSequence(v, n_rotate);
}


}  // namespace containers
