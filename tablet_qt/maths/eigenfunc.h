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
#include <cstdlib>
#include <functional>
#include <QDebug>
#include <QStringList>
#include <QVector>
#include <vector>
#include "maths/include_eigen_core.h"
#include "maths/include_eigen_dense.h"

namespace eigenfunc
{

/*

Reminders about Eigen types:

    Matrix<ContentsT, 3, 4>
        fixed rows=3 x cols=3 matrix of type ContentsT
    Matrix<ContentsT, Dynamic, 1>
        column vector, size unknown at compile-time, of type ContentsT
    MatrixXd
        shorthand for Matrix<double, Dynamic, Dynamic>

    Array<ContentsT, Dynamic, Dynamic>
        arbitrary 2x2 matrix of type ContentsT
    ArrayXXd
        shorthand for Array<double, Dynamic, Dynamic>
    ArrayXd
        shorthand for Array<double, Dynamic, 1>

The common base class of Matrix and Array is DenseBase.

SVD:

    with JacobiSVD<...> S:

        R       Eigen
        ----------------------------------------------
        S$d     S.singularValues()  // column vector
        S$u     S.matrixU()         // matrix
        S$v     S.matrixV()         // matrix

Inheritance and templates

    - ArrayBase<> and MatrixBase<> are both derived from DenseBase<> and
      ultimately from EigenBase<>

    - If you just want to use any old Eigen container based on its behaviour,
      you can use
            template<typename EigenContainerT>
            void doSomething(const EigenContainerT& m) { ... }

    - You can access the contents type from DenseBase<Derived> like this:
            template<typename Derived>
            void doSomething(const Eigen::DenseBase<Derived>& m)
            {
                using Scalar = typename Derived::Scalar;
                // ...
            }

    - But can you define a return value like this, where the thing you're
      returning uses the Scalar part, but is of a different type to the
      parameters? For example:

            Array<Scalar, Dynamic, 1> returnSomething(
                const ArrayOrMatrix<Scalar, ...>& m) { ... }

      Turns out you can:

            template<typename Derived>
            ColumnArray<typename Derived::Scalar> something(
                    const Eigen::DenseBase<Derived>& m)
            {
                Eigen::Array<typename Derived::Scalar, Eigen::Dynamic, 1> v;
                v << m(0, 0);
                return v;
            }

Conditional assignment

    No need for special functions for many cases; use

        X = boolean_array.select(Y, X)

    to achieve X = boolean_array ? Y : X, but elementwise.
    Note that either parameter of select() can be a scalar.
    HOWEVER, when it's a matrix, it must match by size.

    For size mismatch, we have assignByBoolean and assignByIndex.

Index

    Eigen::Index is of type std::ptrdiff_t
        http://eigen.tuxfamily.org/index.php?title=3.3#Index_typedef
        ... which is of type long (on my machine, anyway).
    So when dividing them, use
        std::ldiv_t division = std::ldiv(idx, nr);

Comma initializer

    - You have to pre-size a vector before using <<.
    - You can't insert elements one by one using <<; you can do

            v << 1, 2, 3, 4, 5;

      but not
            v << 1;
            v << 2;
            // ...

      ("Too few coefficients passed to comma initializer (operator<<)")

Comparison

    - It seems you can do
            vector1 == vector2

      but not
            array1 == array2

      ... which doesn't seem to match
      https://eigen.tuxfamily.org/dox/group__QuickRefPage.html

      ... or is it that you can compare, but only if the sizes are
      identical?

*/

// ============================================================================
// Type shorthands
// ============================================================================

template<typename ContentsT>
using ColumnVector = Eigen::Matrix<ContentsT, Eigen::Dynamic, 1>;

template<typename ContentsT>
using RowVector = Eigen::Matrix<ContentsT, 1, Eigen::Dynamic>;

template<typename ContentsT>
using ColumnArray = Eigen::Array<ContentsT, Eigen::Dynamic, 1>;

template<typename ContentsT>
using RowArray = Eigen::Array<ContentsT, 1, Eigen::Dynamic>;

template<typename ContentsT>
using GenericMatrix = Eigen::Matrix<ContentsT, Eigen::Dynamic, Eigen::Dynamic>;

template<typename ContentsT>
using GenericArray = Eigen::Matrix<ContentsT, Eigen::Dynamic, Eigen::Dynamic>;

using IndexArray = Eigen::Array<Eigen::Index, Eigen::Dynamic, 1>;  // 1-dimensional (column) array of indices
// Default storage is column-major, i.e. column vectors should be faster
// (though you can change this on a per-object basis);
// https://eigen.tuxfamily.org/dox/group__TopicStorageOrders.html
using IndexVector = Eigen::Matrix<Eigen::Index, Eigen::Dynamic, 1>;  // 1-dimensional (column) vector of indices

using ArrayXb = Eigen::Array<bool, Eigen::Dynamic, 1>;
// Eigen doesn't define ArrayXb, but it's helpful as a column vector of bool

using ArrayXXb = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>;
// Eigen doesn't define ArrayXXb, but it's helpful as a n x n vector of bool


// ============================================================================
// Conversion between Qt and Eigen types
// ============================================================================

template<typename DestContainerT, typename SourceContentsT>
DestContainerT eigenVectorFromQVector(const QVector<SourceContentsT>& qv)
{
    // Takes a Qt QVector and returns an Eigen vector with the same contents.
    // The precise Eigen vector type (e.g. column vector, row vector) is
    // determined by the template.
    // This function is usually called indirectly, e.g. from
    //      eigenColumnVectorFromQVector()
    //      eigenRowVectorFromQVector()
    int n = qv.size();
    DestContainerT ev(n);
    for (int i = 0; i < n; ++i) {
        ev(i) = qv.at(i);
    }
    return ev;
}


template<typename DestContentsT, typename SourceContentsT>
ColumnVector<DestContentsT> eigenColumnVectorFromQVector(
        const QVector<SourceContentsT>& qv)
{
    // Takes a Qt QVector and returns an Eigen Vector (i.e. an Eigen Matrix
    // of dimensions nx1).
    return eigenVectorFromQVector<ColumnVector<DestContentsT>>(qv);
}


template<typename DestContentsT, typename SourceContentsT>
RowVector<DestContentsT> eigenRowVectorFromQVector(
        const QVector<SourceContentsT>& v)
{
    // Takes a Qt QVector and returns an Eigen RowVector (i.e. an Eigen Matrix
    // of dimensions 1xn).
    return eigenVectorFromQVector<RowVector<DestContentsT>>(v);
}


template<typename DestContentsT, typename SourceContainerT>
QVector<DestContentsT> qVectorFromEigenVector(const SourceContainerT& ev)
{
    // Takes an Eigen vector (row vector or column vector) and returns a Qt
    // QVector.
    int n = ev.size();
    QVector<DestContentsT> qv(n);
    for (int i = 0; i < n; ++i) {
        qv[i] = ev(i);
    }
    return qv;
}


template<typename Derived>
QString qStringFromEigenMatrixOrArray(const Eigen::DenseBase<Derived>& m,
                                      const QString type_name = "DenseBase")
{
    // Formats an Eigen Matrix or Array for display using a Qt QString.
    // - https://eigen.tuxfamily.org/dox/structEigen_1_1IOFormat.html
    // - C++ std::type_traits doesn't allow detection of template
    //   specializations, I don't think! Hence the type_name system, via which
    //   the detection is done at compile-time.
    Eigen::IOFormat heavy_fmt(
                Eigen::FullPrecision,  // precision
                0,  // flags
                ", ",  // coeffSeparator
                ";\n",  // rowSeparator
                "[",  // rowPrefix
                "]",  // rowSuffix
                "[",  // matPrefix
                "]");  // matSuffix
    std::stringstream ss;
    ss << m.format(heavy_fmt);
    QString description = QString("%1 (%2 rows x %3 cols)")
            .arg(type_name).arg(m.rows()).arg(m.cols());
    return description + "\n" + QString::fromStdString(ss.str());
}


// Specialization of qStringFromEigenMatrixOrArray for a Matrix.
template<typename Derived>
QString qStringFromEigenMatrixOrArray(const Eigen::MatrixBase<Derived>& m)
{
    return qStringFromEigenMatrixOrArray(m, "Matrix");
}


// Specialization of qStringFromEigenMatrixOrArray for an Array.
template<typename Derived>
QString qStringFromEigenMatrixOrArray(const Eigen::ArrayBase<Derived>& m)
{
    return qStringFromEigenMatrixOrArray(m, "Array");
}


// ============================================================================
// Making Eigen containers from std::vector
// ============================================================================

template<typename DestContainerT, typename SourceContentsT>
DestContainerT eigenVectorFromStdVector(const std::vector<SourceContentsT>& sv)
{
    // Takes a C++ std::vector and returns an Eigen vector with the same
    // contents. The precise Eigen vector type (e.g. column vector, row vector)
    // is determined by the template.
    // This function is usually called indirectly, e.g. from
    //      eigenColumnVectorFromStdVector()
    //      eigenRowVectorFromStdVector()
    //
    // Note that std::vector::size() returns size_t, but Eigen containers use
    // int for their dimensions. We need to make sure that everything is
    // compatible without any compiler warnings.
    // Remember: size_t is unsigned (but its size is implementation-specific);
    // int is of course signed (and at least 16 bits, but it could be larger).
    // More generally, re integer overflow in C/C++:
    //      http://www.cs.utah.edu/~regehr/papers/overflow12.pdf
    // and signed/unsigned comparisons:
    //      https://peter.bourgon.org/blog/2009/05/01/comparison-between-signed-and-unsigned-integer-expressions.html
    const size_t n_as_size_t = sv.size();
    const int n = static_cast<int>(n_as_size_t);
    // ... cannot be negative; if it is, there's overflow
    if (n < 0 || static_cast<size_t>(n) != n_as_size_t) {
        qWarning()
                << "Unable to represent std::vector of size_t" << n_as_size_t
                << "in Eigen container of int size" << n
                << "(likely large container overflowing int dimension). "
                   "RESULTS WILL BE WRONG.";
    }
    DestContainerT ev(n);
    for (int i = 0; i < n; ++i) {
        ev(i) = sv.at(i);
    }
    return ev;
}


template<typename DestContentsT, typename SourceContentsT>
ColumnVector<DestContentsT> eigenColumnVectorFromStdVector(
        const std::vector<SourceContentsT>& sv)
{
    // Takes a C++ std::vector and returns an Eigen Vector (i.e. an Eigen
    // Matrix of dimensions nx1).
    return eigenVectorFromStdVector<ColumnVector<DestContentsT>>(sv);
}


template<typename Scalar>
ColumnVector<Scalar> eigenColumnVectorFromInitList(
        std::initializer_list<Scalar> vlist)
{
    // Equivalent, but taking an initializer list
    return eigenColumnVectorFromStdVector<Scalar>(std::vector<Scalar>(vlist));
}


template<typename DestContentsT, typename SourceContentsT>
RowVector<DestContentsT> eigenRowVectorFromStdVector(
        const std::vector<SourceContentsT>& sv)
{
    // Takes a C++ std::vector and returns an Eigen RowVector (i.e. an Eigen
    // Matrix of dimensions 1xn).
    return eigenVectorFromStdVector<RowVector<DestContentsT>>(sv);
}


template<typename Scalar>
ColumnVector<Scalar> eigenRowVectorFromInitList(
        std::initializer_list<Scalar> vlist)
{
    // Equivalent, but taking an initializer list
    return eigenRowVectorFromStdVector<Scalar>(std::vector<Scalar>(vlist));
}


// A quick shorthand:
template<typename SourceContentsT>
ColumnVector<Eigen::Index> eigenIndexVectorFromStdVector(
        const std::vector<SourceContentsT>& sv)
{
    // Takes a C++ std::vector and returns an Eigen column Vector (Matrix)
    // containing Eigen::Index (= long int, typically).
    return eigenVectorFromStdVector<ColumnVector<Eigen::Index>>(sv);
}


template<typename DestContentsT, typename SourceContentsT>
ColumnArray<DestContentsT> eigenColumnArrayFromStdVector(
        const std::vector<SourceContentsT>& sv)
{
    // Takes a C++ std::vector and returns an Eigen column Array containing
    // the same contents.
    return eigenVectorFromStdVector<ColumnArray<DestContentsT>>(sv);
}


// Quick functions to make Eigen arrays of Eigen::Index or bool, from
// std::vector objects or initializer lists:
IndexArray makeIndexArray(const std::vector<Eigen::Index>& v);
IndexArray makeIndexArray(std::initializer_list<Eigen::Index> vlist);
ArrayXb makeBoolArray(const std::vector<bool>& v);
ArrayXb makeBoolArray(std::initializer_list<bool> vlist);


// ============================================================================
// Miscellaneous helpers
// ============================================================================

// Make a sequence of Eigen::Index, and return it in a column array:
IndexArray indexSeq(Eigen::Index first, Eigen::Index last,
                    Eigen::Index step = 1);

// Take a sequence of Eigen::Index, and return a sequence of bool for use as
// the condition in an Eigen "condition.select(then, _else)" statement. There's
// a version for vectors and another for 2d arrays:
ArrayXb selectBoolFromIndices(const IndexArray& indices, Eigen::Index size);
ArrayXXb selectBoolFromIndices(const IndexArray& indices, Eigen::Index n_rows,
                               Eigen::Index n_cols);

// Add a column of ones as the first column, for creating design matrices in
// which an intercept term is required.
Eigen::MatrixXd addOnesAsFirstColumn(const Eigen::MatrixXd& m);


// ============================================================================
// Subsetting Eigen arrays and matrices
// ============================================================================

// Takes an index idx, makes sure it fits within size (cycling if need be),
// and returns it.
Eigen::Index normalizeIndex(Eigen::Index idx, Eigen::Index size);


// Take an index idx; treat it using R's matrix approach, where the index
// increases down columns, then across rows, i.e. COLUMN-MAJOR ORDER:
//
//      0 3 6
//      1 4 7
//      2 5 8
//
// (which is like R except zero-based). Calculate the row and column
// indices (also zero-based), using "size" and "n_rows". Return them in
// "row" and "col".
void calcRowColFromIndex(Eigen::Index idx,
                         Eigen::Index& row,
                         Eigen::Index& col,
                         Eigen::Index n_rows,
                         Eigen::Index size);


template<typename Derived>
void getRowColFromIndex(const Eigen::DenseBase<Derived>& m,
                        Eigen::Index idx,
                        Eigen::Index& row,
                        Eigen::Index& col)
{
    // Calculates row/col as per calcRowColFromIndex(), but taking an Eigen
    // Matrix/Array to work out the size from.

    const Eigen::Index nr = m.rows();
    const Eigen::Index size = m.size();
    calcRowColFromIndex(idx, row, col, nr, size);
}


template<typename Derived>
IndexArray which(const Eigen::DenseBase<Derived>& m)
{
    // As per R: produce a single vector of "indices"; go down columns before
    // going across rows (see calcRowColFromIndex, the converse). Tests the
    // truth value of every element in m, and put the index into the array if
    // its element is true.

    std::vector<Eigen::Index> indices;
    const Eigen::Index nr = m.rows();
    const Eigen::Index nc = m.cols();
    Eigen::Index i = 0;
    for (Eigen::Index c = 0; c < nc; ++c) {
        for (Eigen::Index r = 0; r < nr; ++r) {
            if (m(r, c)) {
                indices.push_back(i);
            }
            ++i;
        }
    }
    ColumnVector<Eigen::Index> v = eigenIndexVectorFromStdVector(indices);
    return v.array();
}


template<typename EigenContainerT>
EigenContainerT subsetByColumnIndex(const EigenContainerT& m,
                                    const IndexArray& column_indices)
{
    // Takes an Eigen Matrix/Array, and creates a corresponding object with
    // columns specified by "column_indices".
    // The number of output rows will be m.rows().
    // The number of output columns will be column_indices.size().

    const Eigen::Index m_ncols = m.cols();
    const Eigen::Index n_indices = column_indices.size();
    EigenContainerT subset(m.rows(), n_indices);
    for (Eigen::Index i = 0; i < n_indices; ++i) {
        Eigen::Index col_idx = normalizeIndex(column_indices(i), m_ncols);
        subset.col(i) = m.col(col_idx);
    }
    return subset;
}


template<typename EigenContainerT>
EigenContainerT subsetByRowIndex(const EigenContainerT& m,
                                 const IndexArray& row_indices)
{
    // Takes an Eigen Matrix/Array, and creates a corresponding object with
    // rows specified by "row_indices".
    // The number of output rows will be row_indices.size().
    // The number of output columns will be m.cols().

    const Eigen::Index m_nrows = m.rows();
    const Eigen::Index n_indices = row_indices.size();
    EigenContainerT subset(n_indices, m.cols());
    for (Eigen::Index i = 0; i < n_indices; ++i) {
        Eigen::Index row_idx = normalizeIndex(row_indices(i), m_nrows);
        subset.row(i) = m.row(row_idx);
    }
    return subset;
}


template<typename Derived>
ColumnArray<typename Derived::Scalar> subsetByElementIndex(
        const Eigen::DenseBase<Derived>& m,
        const IndexArray& indices)
{
    // Fetches elements of m by their index.
    // Treats "indices" as going down columns before going across rows.
    // Returns a column array, of size indices.size().

    const Eigen::Index nr = m.rows();
    const Eigen::Index oldlength = m.size();
    const Eigen::Index newlength = indices.size();
    ColumnArray<typename Derived::Scalar> result(newlength);
    for (Eigen::Index i = 0; i < newlength; ++i) {
        Eigen::Index idx = indices(i);
        Eigen::Index col, row;
        calcRowColFromIndex(idx, row, col, nr, oldlength);
        result(i) = m(row, col);
    }
    return result;
}


template<typename Derived>
GenericArray<typename Derived::Scalar> subsetByColumnBoolean(
        const Eigen::DenseBase<Derived>& m,
        const ArrayXb& use_column)
{
    // Takes an Eigen Matrix/Array, and creates an array whose columns are
    // specified by "use_column". Each element of "use_column" is true/false,
    // and columns are included or not depending on this.
    // CYCLES THROUGH use_column TO MAX LENGTH OF m.cols().
    // The number of output rows will be m.rows().
    // The number of output columns will be the number of "true" values
    // encounted while cycling through "use_column" up to m.cols().

    const Eigen::Index m_nc = m.cols();
    const Eigen::Index m_nr = m.rows();
    const Eigen::Index use_column_size = use_column.size();
    // Phase 1: calculate # columns
    Eigen::Index dest_nc = 0;
    for (Eigen::Index i = 0; i < m_nc; ++i) {
        Eigen::Index uc_idx = normalizeIndex(i, use_column_size);
        if (use_column(uc_idx)) {
            ++dest_nc;
        }
    }
    // Phase 2: assign
    GenericArray<typename Derived::Scalar> subset(m_nr, dest_nc);
    Eigen::Index dest_idx = 0;
    for (Eigen::Index i = 0; i < m_nc; ++i) {
        Eigen::Index uc_idx = normalizeIndex(i, use_column_size);
        if (use_column(uc_idx)) {
            subset.col(dest_idx++) = m.col(i);
        }
    }
    return subset;
}


template<typename Derived>
GenericArray<typename Derived::Scalar> subsetByRowBoolean(
        const Eigen::DenseBase<Derived>& m,
        const ArrayXb& use_row)
{
    // Takes an Eigen Matrix/Array, and creates an array whose rows are
    // specified by "use_row". Each element of "use_row" is true/false,
    // and columns are included or not depending on this.
    // CYCLES THROUGH use_row TO MAX LENGTH OF m.rows().
    // The number of output rows will be the number of "true" values
    // encounted while cycling through "use_row" up to m.rows().
    // The number of output columns will be m.cols().

    const Eigen::Index m_nc = m.cols();
    const Eigen::Index m_nr = m.rows();
    const Eigen::Index use_row_size = use_row.size();
    // Phase 1: calculate # rows
    Eigen::Index dest_nr = 0;
    for (Eigen::Index i = 0; i < m_nr; ++i) {
        Eigen::Index ur_idx = normalizeIndex(i, use_row_size);
        if (use_row(ur_idx)) {
            ++dest_nr;
        }
    }
    // Phase 2: assign
    GenericArray<typename Derived::Scalar> subset(dest_nr, m_nc);
    Eigen::Index dest_idx = 0;
    for (Eigen::Index i = 0; i < m_nr; ++i) {
        Eigen::Index ur_idx = normalizeIndex(i, use_row_size);
        if (use_row(ur_idx)) {
            subset.row(dest_idx++) = m.row(i);
        }
    }
    return subset;
}


template<typename Derived>
ColumnArray<typename Derived::Scalar> subsetByElementBoolean(
        const Eigen::DenseBase<Derived>& m,
        const ArrayXXb& which)
{
    // As per R (approximately): reads values of m for which "which" is true,
    // and spits them out into a vector, reading down columns first, then
    // across rows (i.e. column-wise, then row-wise).
    // Also following R: we don't care about the dimensionality of "which",
    // and will cycle through it. That is:
    // CYCLES THROUGH which TO SIZE OF m.

    using Scalar = typename Derived::Scalar;
    std::vector<Scalar> v;
    const Eigen::Index m_size = m.size();
    const Eigen::Index which_size = which.size();
    const Eigen::Index m_nr = m.rows();
    const Eigen::Index which_nr = which.rows();
    Eigen::Index m_row, m_col, which_row, which_col;
    for (Eigen::Index i = 0; i < m_size; ++i) {
        calcRowColFromIndex(i, m_row, m_col, m_nr, m_size);
        calcRowColFromIndex(i, which_row, which_col, which_nr, which_size);
        if (which(which_row, which_col)) {
            v.push_back(m(m_row, m_col));
        }
    }
    return eigenColumnArrayFromStdVector<Scalar>(v);
}


// ============================================================================
// Assigning to parts of Eigen object from source objects matching the "change"
// size, not the "recipient" size (for which select() works fine).
// Note that select() also works fine for assignment of scalars.
// ============================================================================

template <typename DerivedTo, typename DerivedFrom>
void assignByBooleanSequentially(Eigen::DenseBase<DerivedTo>& to,
                                 const ArrayXXb& which,
                                 const Eigen::DenseBase<DerivedFrom>& from)
{
    // Assigns values to "to", from "from", according to "which".
    // As a simple example:
    //      to    = [ 1  2  3  4  5  6  7  8  9 10]
    //      which = [ f  T  f  f  T  f  f  f  T  f]
    //      from  = [97 98 99]
    //
    //     result = [ 1 97  3  4 98  6  7  8 99 10]
    //
    // CYCLES THROUGH which TO MAX LENGTH OF to.
    // Also cycles through "from" as required.
    // (If "from" is two-dimensional, cycles through "from" in column-major
    // order.)

    const Eigen::Index num_to_replace = which.cast<int>().sum();
    if (num_to_replace == 0) {
        return;
    }
    const Eigen::Index from_size = from.size();
    if (from_size == 0) {
        qCritical() << Q_FUNC_INFO << "Empty 'from'";
        return;
    }
    if (from_size > num_to_replace || from_size % num_to_replace != 0) {
        qCritical() << Q_FUNC_INFO << "Number of items to replace is not a "
                                      "multiple of replacement length";
        return;
    }
    const Eigen::Index to_nr = to.rows();
    const Eigen::Index to_size = to.size();
    const Eigen::Index which_nr = which.rows();
    const Eigen::Index which_size = which.size();
    const Eigen::Index from_nr = from.rows();
    Eigen::Index to_row, to_col, which_row, which_col, from_row, from_col;
    Eigen::Index from_idx = 0;
    for (Eigen::Index i = 0; i < to_size; ++i) {
        calcRowColFromIndex(i, which_row, which_col, which_nr, which_size);
        if (!which(which_row, which_col)) {
            continue;
        }
        calcRowColFromIndex(i, to_row, to_col, to_nr, to_size);
        calcRowColFromIndex(from_idx, from_row, from_col, from_nr, from_size);
        to(to_row, to_col) = from(from_row, from_col);
        from_idx = normalizeIndex(++from_idx, from_size);
    }
}


template <typename DerivedTo, typename DerivedFrom>
void assignByIndexSequentially(Eigen::DenseBase<DerivedTo>& to,
                               const IndexArray& indices,
                               const Eigen::DenseBase<DerivedFrom>& from)
{
    // Assigns values to "to", according to element indices in "indices".
    // (Those indices are treated as down-columns-before-across-rows, i.e.
    // column-major order, zero-based.)
    // As a simple example:
    //      to      = [ 1  2  3  4  5  6  7  8  9 10]
    //      indices = [ 1 4 8 ]
    //      from    = [97 98 99]
    //
    //     result   = [ 1 97  3  4 98  6  7  8 99 10]
    //
    // Also cycles through "from" as required.

    const Eigen::Index n_indices = indices.size();
    const Eigen::Index from_size = from.size();
    if (n_indices == 0) {
        return;
    }

    // To mimic R behaviour:
    if (from_size > n_indices || from_size % n_indices != 0) {
        qCritical() << Q_FUNC_INFO << "Number of items to replace is not a "
                                      "multiple of replacement length";
        return;
    }

    const Eigen::Index to_nr = to.rows();
    const Eigen::Index to_size = to.size();
    Eigen::Index from_idx = 0;
    for (Eigen::Index i = 0; i < n_indices; ++i) {
        const Eigen::Index idx = indices(i);
        Eigen::Index row, col;
        calcRowColFromIndex(idx, row, col, to_nr, to_size);
        to(row, col) = from(from_idx++);
        if (from_idx >= from_size) {
            // Cycle round
            from_idx = 0;
        }
    }
}


// ============================================================================
// Array-by-vector elementwise operations, following R
// ============================================================================

template <typename Derived1, typename Derived2>
Eigen::Array<typename Derived1::Scalar,
             Eigen::Dynamic, Eigen::Dynamic> multiply(
        const Eigen::ArrayBase<Derived1>& a,
        const Eigen::ArrayBase<Derived2>& b)
{
    // Multiplies either (a) an array by an array of the same shape, or
    // (b) an array by a vector (not necessarily of the "right" length).
    //
    // # R example:
    // a = matrix(1:16, nrow=4)
    // b = c(1, 10, 100)
    // a * b
    //
    // # ... gives:
    // #     [,1] [,2] [,3] [,4]
    // # [1,]    1   50  900   13
    // # [2,]   20  600   10  140
    // # [3,]  300    7  110 1500
    // # [4,]    4   80 1200   16
    // # Warning message:
    // # In a * b : longer object length is not a multiple of shorter object length
    //
    // b * a  # same as a * b

    using ArrayType = Eigen::Array<typename Derived1::Scalar,
                                   Eigen::Dynamic, Eigen::Dynamic>;

    const Eigen::Index a_nr = a.rows();
    const Eigen::Index a_nc = a.cols();
    const Eigen::Index b_nr = b.rows();
    const Eigen::Index b_nc = b.cols();
    const Eigen::Index b_size = b.size();

    if (a_nr == b_nr && a_nc == b_nc) {
        // Arrays have the same dimensions
        return a * b;
    }

    bool a_is_vec = a_nr == 1 || a_nc == 1;
    bool b_is_vec = b_nr == 1 || b_nc == 1;
    if (!a_is_vec && !b_is_vec) {
        qFatal("multiply: non-conformable arrays");
    }
    if (!b_is_vec) {
        // It's very hard, in a template, to do some sort of neat swap.
        // For example, you can't create a reference or pointer and point to
        // either type (since the template isn't aware they are *of* the same
        // or a compatible type).
        return multiply(b, a);
    }

    ArrayType dest = a;
    Eigen::Index b_idx = 0;
    for (Eigen::Index c = 0; c < a_nc; ++c) {
        for (Eigen::Index r = 0; r < a_nr; ++r) {
            dest(r, c) *= b(b_idx++);
            b_idx %= b_size;  // cycle
        }
    }
    return dest;
}


#if 0
// ============================================================================
// Cyclic select()
// ============================================================================
// PROBLEM: mimicking R's   y = x[boolvec]  # achieved
//                          x[boolvec] = x  # achieved only indirectly:
// The problems are:
// (*) Eigen's select() requires conformable arrays.
// (*) R's x[boolvec], in contrast, cycles through boolvec up to the
//     length of x.
// (*) If we subset, e.g. with
//          const ArrayXd t_good = subsetByElementBoolean(t, good);
//     we have lots of short vectors floating around and it becomes hard
//     to be sure we're replacing them all correctly.
// Perhaps the answer is an extended select() function.
// See DenseBase.h, Select.h for the original.
// Let's write cyclicSelect();
//
// The original is condition.select(then, else).
// It can handle matrix-matrix, scalar-matrix, matrix-scalar

template <typename DerivedCondition, typename DerivedThenElse>
Eigen::Array<typename Derived::Scalar, Eigen::Dynamic, 1> cyclicSelect(
        const Eigen::DenseBase<DerivedCondition>& condition,
        const Eigen::DenseBase<DerivedThenElse>& then,
        const Eigen::DenseBase<DerivedThenElse>& _else)  // else is a C++ keyword
{
    // Produces a column array whose length is the maximum of the lengths of
    // (condition, then, else), cycling as necessary.

    using ArrayType = Eigen::Array<typename DerivedThenElse::Scalar,
                                   Eigen::Dynamic, Eigen::Dynamic>;

    // ABANDONED FOR NOW.
}
#endif


// ============================================================================
// Sorting Eigen things
// ============================================================================

template<typename Derived>
void sort(Eigen::PlainObjectBase<Derived>& m, bool decreasing = false)
{
    // Sorts an Eigen Matrix/Array in place.

    // An Array can be a view on a Matrix, I think, judging by
    //      array() functions at lines 326/329 of MatrixBase.h
    //          ... which return an ArrayWrapper to the derived() matrix object
    // The data() function is defined in PlainObjectBase.h and DenseStorage.h
    // Let's try using PlainObjectBase.
    using Scalar = typename Derived::Scalar;
    Scalar* p_data = m.data();
    Eigen::Index size = m.size();
    if (decreasing) {
        std::sort(p_data, p_data + size, std::greater<Scalar>());
    } else {
        std::sort(p_data, p_data + size);
    }
}


template<typename EigenContainerT>
EigenContainerT sorted(const EigenContainerT& vec)
{
    // Returns a sorted copy of an Eigen Matrix/Array.

    EigenContainerT newvec = vec;
    sort(newvec);
    return newvec;
}


// ============================================================================
// Other R functions
// ============================================================================
// See the identically named functions in R.

Eigen::MatrixXd scale(const Eigen::MatrixXd& x,
                      bool centre_on_column_mean = true,
                      bool scale_divide_by_column_rms = true,
                      const Eigen::ArrayXd& centre_values = Eigen::ArrayXd(),
                      const Eigen::ArrayXd& scale_values = Eigen::ArrayXd());

Eigen::MatrixXd chol(const Eigen::MatrixXd& x, bool pivot = false);

Eigen::MatrixXd backsolve(const Eigen::MatrixXd& r,
                          const Eigen::MatrixXd& x,
                          Eigen::Index k = -1,  // default to ncol(r)
                          bool transpose = false,
                          bool upper_tri = true);  // upper_tri = true MAKES IT A BACKSOLVE
Eigen::MatrixXd forwardsolve(const Eigen::MatrixXd& l,
                             const Eigen::MatrixXd& x,
                             Eigen::Index k = -1,  // default to ncol(l)
                             bool transpose = false,
                             bool upper_tri = false);  // upper_tri = false MAKES IT A FORWARDSOLVE
// I think that forwardsolve and backsolve are basically the same thing, except
// for the default relating to upper_tri. That's what R's documentation
// suggests:
//           Solves a system of linear equations where the coefficient matrix
//           is upper (or ‘right’, ‘R’) or lower (‘left’, ‘L’) triangular.
//           ‘x <- backsolve(R, b)’ solves R x = b [RNC: for x], and
//           ‘x <- forwardsolve(L, b)’ solves L x = b [RNC: for x], respectively.
// And this:
//      http://www.stat.berkeley.edu/~paciorek/research/techVignettes/techVignette6.pdf
//  "A quick note on backsolves and forwardsolves. If you have a
//  lower-triangular L, a forwardsolve is the calculation L^−1 b. A backsolve
//  is the calculation U^−1 b for upper-triangular U.
//  In R, we can do either type of solve with either L or U without an explicit
//  transpose:
//      U^−1 b: backsolve(U, b)
//      L^−1 b = U[T]^−1 b: backsolve(U, transpose = TRUE)
//      L^−1 b: forwardsolve(L, b)
//      U^−1 b = L[T]^−1 b: forwardsolve(L, b, transpose = TRUE)
// The "transpose" option:
// - In R help, it normally solves R x = b (giving X),
//   and the transpose option solves t(R) y = x (for y).
// - I wish it would be consistent within a single help page in its
//   use of variable names...

// My internal function to implement both forwardsolve() and backsolve():
Eigen::MatrixXd forwardOrBackSolve(Eigen::MatrixXd lr,
                                   Eigen::MatrixXd x,
                                   Eigen::Index k,
                                   bool transpose,
                                   bool upper_tri);


// ============================================================================
// Testing
// ============================================================================

// Test our Eigen functions.
QStringList testEigenFunctions();


}  // namespace eigenfunc
