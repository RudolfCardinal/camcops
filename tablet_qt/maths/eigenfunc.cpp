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

#include "eigenfunc.h"
#include <cmath>
#include <QDebug>
#include <QtMath>
#include "maths/logisticregression.h"
using namespace Eigen;


namespace eigenfunc
{

// ============================================================================
// Making Eigen containers from std::vector
// ============================================================================

IndexArray makeIndexArray(const std::vector<Index>& v)
{
    return eigenIndexVectorFromStdVector(v).array();
}


IndexArray makeIndexArray(std::initializer_list<Index> vlist)
{
    return makeIndexArray(std::vector<Index>(vlist));
}


ArrayXb makeBoolArray(const std::vector<bool>& v)
{
    return eigenColumnVectorFromStdVector<bool>(v).array();
}


ArrayXb makeBoolArray(std::initializer_list<bool> vlist)
{
    return makeBoolArray(std::vector<bool>(vlist));
}


// ============================================================================
// Miscellaneous helpers
// ============================================================================

IndexArray indexSeq(const Index first, const Index last, const Index step)
{
    const Index n = (1 + last - first) / step;
    IndexArray indices(n);
    Index idx = 0;
    if (step > 0) {
        for (Index i = first; i <= last; i += step) {
            indices(idx++) = i;
        }
    } else if (step < 0) {
        for (Index i = first; i >= last; i -= step) {
            indices(idx++) = i;
        }
    }
    return indices;
}


ArrayXb selectBoolFromIndices(const IndexArray& indices, const Index size)
{
    ArrayXb select_bool(size);
    select_bool.setConstant(false);
    const Index n_indices = indices.size();
    for (Index i = 0; i < n_indices; ++i) {
        Eigen::Index idx = normalizeIndex(indices(i), size);
        select_bool(idx) = true;
    }
    return select_bool;
}


ArrayXXb selectBoolFromIndices(const IndexArray& indices,
                               const Index n_rows,
                               const Index n_cols)
{
    ArrayXXb select_bool(n_rows, n_cols);
    select_bool.setConstant(false);
    const Index n_indices = indices.size();
    const Index size = select_bool.size();
    Index row, col;
    for (Index i = 0; i < n_indices; ++i) {
        calcRowColFromIndex(indices(i), row, col, n_rows, size);
        select_bool(row, col) = true;
    }
    return select_bool;
}


MatrixXd addOnesAsFirstColumn(const MatrixXd& m)
{
    MatrixXd X_design(m.rows(), m.cols() + 1);
    X_design << MatrixXd::Ones(m.rows(), 1.0), m;  // first column is 1.0
    return X_design;
}


// ============================================================================
// Subsetting Eigen arrays and matrices
// ============================================================================

Eigen::Index normalizeIndex(Eigen::Index idx, Eigen::Index size)
{
    // Takes an index idx, makes sure it fits within size (cycling if need be),
    // and returns it.
    while (idx < 0) {
        idx += size;
    }  // because (negative % positive) gives negative
    return idx % size;
}


void calcRowColFromIndex(Eigen::Index idx,
                         Eigen::Index& row,
                         Eigen::Index& col,
                         Eigen::Index n_rows,
                         Eigen::Index size)
{
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

    idx = normalizeIndex(idx, size);
#ifdef __ANDROID__
    std::ldiv_t division = std::ldiv(idx, n_rows);
#else
    std::lldiv_t division = std::lldiv(idx, n_rows);
#endif
    col = division.quot;
    row = division.rem;
}


// ============================================================================
// Other R functions
// ============================================================================

MatrixXd scale(const MatrixXd& x,
               const bool centre_on_column_mean,
               const bool scale_divide_by_column_rms,
               const ArrayXd& centre_values,
               const ArrayXd& scale_values)
{
    // To see R code:
    //      scale
    //      methods(scale)
    //      getAnywhere(scale.default)
    Index nc = x.cols();
    ArrayXXd xa = x.array();

    // 1. Centre
    if (centre_values.size() != nc && centre_values.size() != 0) {
        qCritical() << Q_FUNC_INFO << "centre_values.size() is"
                    << centre_values.size()
                    << "which doesn't match number of columns" << nc;
    } else if (centre_on_column_mean) {
        // Centre each column on its mean
        for (Index i = 0; i < nc; ++i) {
            xa.col(i) -= xa.col(i).mean();
        }
    } else if (centre_values.size() == nc) {
        // Centre each column on the specified value
        for (Index i = 0; i < nc; ++i) {
            xa.col(i) -= centre_values(i);
        }
    }

    // 2. Scale
    if (scale_values.size() != nc && scale_values.size() != 0) {
        qCritical() << Q_FUNC_INFO << "scale_values.size() is"
                    << scale_values.size()
                    << "which doesn't match number of columns" << nc;
    } else if (scale_divide_by_column_rms) {
        // Scale each column on its mean
        for (Index i = 0; i < nc; ++i) {
            double divisor = std::sqrt(xa.col(i).square().mean());
            // If "centre" is true, this will be the standard deviation.
            // If not, it's just the root mean square.
            // (This is because the SD is the RMS of deviations from the mean.)
            xa.col(i) /= divisor;
        }
    } else if (scale_values.size() == nc) {
        // Scale each column by dividing by the specified value
        for (Index i = 0; i < nc; ++i) {
            xa.col(i) /= scale_values(i);
        }
    }

    // Note that even though xa is made with an ArrayWrapper() from x, it's not
    // a "view" on x in the sense of a C++ reference.
    // At this point, xa.matrix() is entirely different from x.
    return xa.matrix();
}


Eigen::MatrixXd chol(const Eigen::MatrixXd& x, const bool pivot)
{
    if (x.rows() != x.cols()) {
        qFatal("Cholesky decomposition requires a SQUARE matrix");
    }
    // R's chol() returns the upper triangular matrix; see ?chol
    // - https://stats.stackexchange.com/questions/117661/why-use-upper-triangular-cholesky
    // - https://eigen.tuxfamily.org/dox/group__Cholesky__Module.html
    if (pivot) {
        // Eigen's LDLT: "Robust Cholesky decomposition of a matrix with pivoting"
        LDLT<MatrixXd> c(x);
        return c.matrixU();  // upper triangular
    }
    // Eigen's LLT: "Standard Cholesky decomposition (LL^T) of a matrix..."
    LLT<MatrixXd> c(x);
    return c.matrixU();
}


Eigen::MatrixXd backsolve(const Eigen::MatrixXd& r,
                          const Eigen::MatrixXd& x,
                          const Eigen::Index k,
                          const bool transpose,
                          const bool upper_tri)
{
    return forwardOrBackSolve(r, x, k, transpose, upper_tri);
}


Eigen::MatrixXd forwardsolve(const Eigen::MatrixXd& l,
                             const Eigen::MatrixXd& x,
                             const Eigen::Index k,
                             const bool transpose,
                             const bool upper_tri)
{
    return forwardOrBackSolve(l, x, k, transpose, upper_tri);
}


Eigen::MatrixXd forwardOrBackSolve(Eigen::MatrixXd lr,
                                   Eigen::MatrixXd x,
                                   Eigen::Index k,
                                   const bool transpose,
                                   bool upper_tri)
{
    // - http://lists.r-forge.r-project.org/pipermail/rcpp-devel/2014-June/007781.html
    // - http://lists.r-forge.r-project.org/pipermail/rcpp-devel/attachments/20140627/2034608b/attachment.cpp
    // - http://skip.ucsc.edu/leslie_MOUSE/r/R-1.8.1/src/library/base/R/backsolve.R
    // PROPER SOURCE:
    // - https://github.com/krlmlr/cxxr/blob/master/src/appl/bakslv.c
    if (k < 0) {
        k = lr.cols();
    }
    if (lr.rows() < k || lr.cols() < k) {
        qCritical() << Q_FUNC_INFO << "lr too small (#rows or #cols < k)";
        return MatrixXd();
    }
    if (x.rows() < k) {
        qCritical() << Q_FUNC_INFO << "x too small (#rows < k)";
        return MatrixXd();
    }
    if (transpose) {
        // lr = lr.transpose() crashes:
        // "aliasing detected during transposition, use transposeInPlace() or
        // evaluate the rhs into a temporary using .eval()"
        lr.transposeInPlace();

        upper_tri = !upper_tri;  // a hunch, but a correct one
        // ... See
        // https://github.com/krlmlr/cxxr/blob/master/src/appl/bakslv.c
        // The phrasing is:
        //        if job is
        //          00	 solve t  * x = b,	t lower triangular,
        //          01	 solve t  * x = b,	t upper triangular,
        //          10	 solve t' * x = b,	t lower triangular,
        //          11	 solve t' * x = b,	t upper triangular.
    }
    if (k < lr.cols() || k < x.rows()) {
        lr = lr.block(0, 0, lr.rows(), k);
        x = x.block(0, 0, k, x.cols());
    }
    if (lr.cols() != x.rows()) {
        qCritical() << Q_FUNC_INFO << "Size mismatch: lr.cols() != x.rows()";
        return MatrixXd();
    }
    // qDebug() << Q_FUNC_INFO << "lr:" << qStringFromEigenMatrixOrArray(lr);
    if (upper_tri) {
        return lr.triangularView<Upper>().solve(x);
    }
    return lr.triangularView<Lower>().solve(x);
}


// ============================================================================
// Testing
// ============================================================================

const QString LINE("===============================================================================");
#define ASSERT_ARRAYS_SAME(a, b) Q_ASSERT((a).matrix() == (b).matrix());
#define OUTPUT_LINE() lines.append(LINE);
#define STATE(x) lines.append(x);
#define REPORT(x) lines.append(QString("%1: %2").arg(#x, qStringFromEigenMatrixOrArray(x)));

QStringList testEigenFunctions()
{
    QStringList lines;
    STATE("Testing eigenfunc...");

    QVector<int> qv1{-1, 0, 1, 2};

    OUTPUT_LINE();
    VectorXi ev1_a = eigenColumnVectorFromQVector<int>(qv1);
    VectorXi ev1_b(4);  // must pre-size vector for <<
    ev1_b << -1, 0, 1, 2;
    Q_ASSERT(ev1_a == ev1_b);
    Q_ASSERT(qVectorFromEigenVector<int>(ev1_a) == qv1);
    STATE("Example column vector:");
    REPORT(ev1_a);

    RowVectorXi ev2_a = eigenColumnVectorFromQVector<int>(qv1);
    RowVectorXi ev2_b(4);  // must pre-size vector for <<
    ev2_b << -1, 0, 1, 2;
    Q_ASSERT(ev2_a == ev2_b);
    Q_ASSERT(qVectorFromEigenVector<int>(ev2_a) == qv1);
    STATE("Example row vector:");
    REPORT(ev2_a);

    IndexArray idxarr1_a = (IndexArray(3) << 3, 4, 5).finished();
    IndexArray idxarr1_b = indexSeq(3, 5);
    ASSERT_ARRAYS_SAME(idxarr1_a, idxarr1_b);

    OUTPUT_LINE();
    MatrixXi m1(4, 3);
    m1 << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12;
    // m1:              m1 "indices", cf. R:
    //      1  2  3       0 4 8
    //      4  5  6       1 5 9
    //      7  8  9       2 6 10
    //      10 11 12      3 7 11
    IndexArray m1_which_a = which(m1.array() > 7);
    IndexArray m1_which_b(5);
    m1_which_b << 3, 6, 7, 10, 11;
    REPORT(m1);
    REPORT(m1_which_a);
    REPORT(m1_which_b);
    ASSERT_ARRAYS_SAME(m1_which_a, m1_which_b);

    OUTPUT_LINE();
    IndexArray m1_subset_cols = makeIndexArray({1, 2});
    MatrixXi m2_a = subsetByColumnIndex(m1, m1_subset_cols);
    MatrixXi m2_b(4, 2);
    m2_b << 2, 3, 5, 6, 8, 9, 11, 12;
    REPORT(m2_a);
    Q_ASSERT(m2_a == m2_b);

    OUTPUT_LINE();
    IndexArray m1_subset_rows = makeIndexArray({1, 2});
    MatrixXi m3_a = subsetByRowIndex(m1, m1_subset_rows);
    MatrixXi m3_b(2, 3);
    m3_b << 4, 5, 6, 7, 8, 9;
    REPORT(m3_a);
    Q_ASSERT(m3_a == m3_b);

    OUTPUT_LINE();
    IndexArray m1_subset_elements = makeIndexArray({1, 2, 10, 11});
    ColumnArray<int> m4_a = subsetByElementIndex(m1, m1_subset_elements);
    ColumnArray<int> m4_b(m1_subset_elements.size());
    m4_b << 4, 7, 9, 12;
    REPORT(m4_a);
    ASSERT_ARRAYS_SAME(m4_a, m4_b);

    OUTPUT_LINE();
    ArrayXb m1_subset_cols_bool = makeBoolArray({false, true, true});
    MatrixXi m5_a = subsetByColumnBoolean(m1, m1_subset_cols_bool);
    REPORT(m5_a);
    Q_ASSERT(m5_a == m2_b);  // re-use

    OUTPUT_LINE();
    ArrayXb m1_subset_rows_bool = makeBoolArray({false, true, true, false});
    MatrixXi m6_a = subsetByRowBoolean(m1, m1_subset_rows_bool);
    REPORT(m6_a);
    Q_ASSERT(m6_a == m3_b);  // re-use

    OUTPUT_LINE();
    ArrayXXb m1_subset_elements_bool(4, 3);
    m1_subset_elements_bool <<  // fill rows then cols
        false, false, false,
        true, false, false,
        true, false, true,
        false, false, true;  // total of 4 true values
    ColumnArray<int> m7_a = subsetByElementBoolean(m1, m1_subset_elements_bool);
    REPORT(m7_a);
    ASSERT_ARRAYS_SAME(m7_a, m4_b);  // re-use

    OUTPUT_LINE();
    MatrixXi m8 = m1;
    ArrayXi m9(4);
    m9 << 100, 101, 102, 103;
    MatrixXi m10(4, 3);
    m10 << 1, 2, 3, 100, 5, 6, 101, 8, 102, 10, 11, 103;
    assignByBooleanSequentially(m8, m1_subset_elements_bool, m9);
    REPORT(m8);
    REPORT(m9);
    REPORT(m10);
    Q_ASSERT(m8 == m10);

    OUTPUT_LINE();
    MatrixXi m11 = m1;
    assignByIndexSequentially(m11, m1_subset_elements, m9);
    REPORT(m11);
    Q_ASSERT(m11 == m10);  // re-use

    OUTPUT_LINE();
    MatrixXi m12 = sorted(m11);
    MatrixXi m13(4, 3);
    m13 <<  1, 6, 100,
            2, 8, 101,
            3, 10, 102,
            5, 11, 103;
    REPORT(m12);
    Q_ASSERT(m12 == m13);

    OUTPUT_LINE();
    MatrixXd m14 = m13.cast<double>();  // move into the double arena, not int
    MatrixXd m15 = scale(m14);
    STATE("[NOT TESTED BY AN ASSERT] Testing scale():");
    REPORT(m15);

/*

R code:

m16 <- matrix(c(4, 12, -16, 12, 37, -43, -16, -43, 98), nrow=3)
chol(m16, pivot=FALSE)
chol(m16, pivot=TRUE)

*/
    OUTPUT_LINE();
    Matrix3d m16;
    // https://en.wikipedia.org/wiki/Cholesky_decomposition#Example
    m16 << 4, 12, -16,
           12, 37, -43,
           -16, -43, 98;
    STATE("Matrix to undergo Cholesky decomposition:");
    REPORT(m16);
    Matrix3d m16_llt_u;
    m16_llt_u << 2, 6, -8,  // the U = Upper triangular version
                 0, 1, 5,
                 0, 0, 3;
    // Matrix3d m16_ldlt_u;
    // m16_ldlt_u << 1, 3, -4,  // the U = Upper triangular version
    //               0, 1, 5,
    //               0, 0, 1;
    STATE("Testing chol(pivot=false):");
    Matrix3d m17 = chol(m16, false);
    REPORT(m17);
    Q_ASSERT(m17 == m16_llt_u);
    STATE("[NOT TESTED BY AN ASSERT] Testing chol(pivot=true):");
    Matrix3d m18 = chol(m16, true);
    REPORT(m18);
    // Q_ASSERT(m18 == m16_ldlt_u);
    // Doesn't match Wikipedia; nor does it match R; nor does R match
    // Wikipedia; nor are we using the pivot=false version...

    OUTPUT_LINE();
    STATE("Testing backsolve:");
    // ?backsolve
    Matrix3d r;
    r << 1, 2, 3,
         0, 1, 1,
         0, 0, 2;
    Vector3d x;
    x << 8, 4, 2;
    Vector3d backsolve_solution_a;
    backsolve_solution_a << -1, 3, 1;
    Vector3d backsolve_solution_b = backsolve(r, x);
    REPORT(r);
    REPORT(x);
    REPORT(backsolve_solution_b);
    Q_ASSERT(backsolve_solution_a == backsolve_solution_b);

    Vector3d backsolve_tr_solution_a;
    backsolve_tr_solution_a << 8, -12, -5;
    Vector3d backsolve_tr_solution_b = backsolve(r, x, -1, true);
    REPORT(backsolve_tr_solution_b);
    Q_ASSERT(backsolve_tr_solution_a == backsolve_tr_solution_b);

    Vector3d other_backsolve_solution;
    other_backsolve_solution << 8, 4, 1;

    // transpose, upper_tri:
    Q_ASSERT(backsolve(r, x, -1, false, true) == backsolve_solution_a);
    Q_ASSERT(backsolve(r, x, -1, true, true) == backsolve_tr_solution_a);
    Q_ASSERT(backsolve(r, x, -1, true, false) == other_backsolve_solution);
    Q_ASSERT(backsolve(r, x, -1, false, false) == other_backsolve_solution);

    Matrix3d tr = r.transpose();
    Q_ASSERT(backsolve(tr, x, -1, false, true) == other_backsolve_solution);
    Q_ASSERT(backsolve(tr, x, -1, true, true) == other_backsolve_solution);
    Q_ASSERT(backsolve(tr, x, -1, true, false) == backsolve_solution_a);
    Q_ASSERT(backsolve(tr, x, -1, false, false) == backsolve_tr_solution_a);

    OUTPUT_LINE();
    STATE("Testing multiply:");
    ArrayXXi m19(4, 4);
    m19 << 1, 5,  9, 13,
           2, 6, 10, 14,
           3, 7, 11, 15,
           4, 8, 12, 16;
    ArrayXi m20(3);
    m20 << 1, 10, 100;
    ArrayXXi m21_a = multiply(m19, m20);
    ArrayXXi m21_b = multiply(m20, m19);
    ArrayXXi m21_c(4, 4);
    m21_c <<   1,  50,  900,   13,
              20, 600,   10,  140,
             300,   7,  110, 1500,
               4,  80, 1200,   16;
    REPORT(m19);
    REPORT(m20);
    REPORT(m21_a);
    REPORT(m21_b);
    ASSERT_ARRAYS_SAME(m21_a, m21_c);
    ASSERT_ARRAYS_SAME(m21_b, m21_c);

    OUTPUT_LINE();
    STATE("... all eigenfunc tests completed correctly.");

    return lines;
}


}  // namespace eigenfunc
