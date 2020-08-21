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

#include "logisticregression.h"
#include <QDebug>
#include "maths/linkfunctionfamily.h"
using namespace Eigen;


/*

-------------------------------------------------------------------------------
For logistic regression: Choosing a library (or roll our own?)
-------------------------------------------------------------------------------
The Javascript implementation was based on
- http://statpages.info/logistic.html

Theory:
- http://people.csail.mit.edu/jrennie/writing/lr.pdf

Relevant C libraries include
- GSL
  https://lists.gnu.org/archive/html/help-gsl/2010-04/msg00021.html
  https://www.gnu.org/software/gsl/manual/html_node/Linear-regression-with-a-constant-term.html

A few C++ implementations:
- https://stackoverflow.com/questions/33976729/logistic-regression-for-fault-detection-in-an-image
- https://github.com/bluekingsong/logistic-regression-cpp/blob/master/code/logistic_regression.cpp
- https://github.com/liyanghua/logistic-regression-in-c--/blob/master/lr.cpp
- OpenCV
  http://docs.opencv.org/3.0-beta/modules/ml/doc/logistic_regression.html
- mlpack [OF INTEREST]
  http://mlpack.org/
  http://mlpack.org/docs/mlpack-2.2.3/doxygen.php?doc=namespacemlpack_1_1regression.html
  ... uses Armadillo
      http://www.mlpack.org/docs/mlpack-1.0.0/doxygen.php?doc=build.html
      http://arma.sourceforge.net/docs.html
      ... needs cross-compilation and tries to use system LAPACK/BLAS
          see output of "cmake ." in Armadillo base directory
          https://stackoverflow.com/questions/21263427/cross-compiling-armadillo-linear-algebra-library
- Dlib [OF INTEREST]
  http://dlib.net/ml.html
  https://sourceforge.net/p/dclib/discussion/442518/thread/8f16e2e2/
- Overview of libraries
  https://en.wikipedia.org/wiki/List_of_numerical_libraries#C.2B.2B
- ALGLIB
  http://www.alglib.net/dataanalysis/logit.php
- Eigen
  see below

-------------------------------------------------------------------------------
DLib notes
-------------------------------------------------------------------------------

http://dlib.net/matrix_ex.cpp.html

type is:    dlib::matrix<type, nrows, ncols>
    ... but specify 0 for "don't know yet"

e.g.
            dlib::matrix<double, 0, 1> m(n);  // creates column vector, size n

element-access shorthand m(i) is available for column vectors

-------------------------------------------------------------------------------
DLib example code
-------------------------------------------------------------------------------

#include <dlib/matrix.h>


template<typename T>
dlib::matrix<double, 0, 1> dlibColumnVectorFromQVector(const QVector<T>& v)
{
    int n = v.size();
    dlib::matrix<double, 0, 1> m(n);
    for (int i = 0; i < n; ++i) {
        m(i) = v.at(i);
    }
    return m;
}


template<typename T>
dlib::matrix<double, 1, 0> dlibRowVectorFromQVector(const QVector<T>& v)
{
    int n = v.size();
    dlib::matrix<double, 1, 0> m(n);
    for (int i = 0; i < n; ++i) {
        m(1, i) = v.at(i);
    }
    return m;
}


For logistic regression in DLib:
    see https://sourceforge.net/p/dclib/discussion/442518/thread/8f16e2e2/

-------------------------------------------------------------------------------
MLPACK notes
-------------------------------------------------------------------------------

arma::mat is shorthand for arma::Mat<double>
    constructor: mat(n_rows, n_cols)

arma::vec is shorthand for arma::Col<double>, equiv. to mat(n, 1)

http://arma.sourceforge.net/docs.html#element_access

BUT it uses BLAS and lots of other compiled things...

    https://en.m.wikipedia.org/wiki/Basic_Linear_Algebra_Subprograms

-------------------------------------------------------------------------------
Eigen notes
-------------------------------------------------------------------------------

- http://eigen.tuxfamily.org
- https://codereview.stackexchange.com/questions/112750/logistic-regression-with-eigen
- https://github.com/wepe/MachineLearning/tree/master/logistic%20regression/use_cpp_and_eigen
- https://forum.kde.org/viewtopic.php?f=74&t=129644

Note also that Douglas Bates's lme4 is implemented using Eigen (Bates et al.
2015, J Stat Soft 67:1), which is a fair endorsement!

-------------------------------------------------------------------------------
DECISION
-------------------------------------------------------------------------------

Implement a GLM in Eigen, and subspecialize it for logistic regression.

*/


LogisticRegression::LogisticRegression(
        const SolveMethod solve_method,
        const int max_iterations,
        const double tolerance,
        const RankDeficiencyMethod rank_deficiency_method) :
    Glm(LINK_FN_FAMILY_LOGIT, solve_method, max_iterations, tolerance,
        rank_deficiency_method)
{
}


void LogisticRegression::fitAddingIntercept(
        const MatrixXd& X,  // predictors, EXCLUDING intercept
        const VectorXi& y)  // depvar
{
    const MatrixXd predictors = addInterceptToPredictors(X);
    fitDirectly(predictors, y);
}


void LogisticRegression::fitDirectly(
        const MatrixXd& X,  // predictors, INCLUDING intercept
        const VectorXi& y)  // depvar
{
    const VectorXd dependent_variable = y.cast<double>();
    Glm::fit(X, dependent_variable);
}


VectorXd LogisticRegression::predictProb() const
{
    return predict();
}


VectorXd LogisticRegression::predictProb(const MatrixXd& X,
                                         bool add_intercept) const
{
    if (add_intercept) {
        return predict(addInterceptToPredictors(X));
    } else {
        return predict(X);
    }
}


VectorXi LogisticRegression::binaryFromP(const VectorXd& p,
                                         const double threshold) const
{
    const ArrayXXd a = p.array();
    const Array<bool, Dynamic, Dynamic> b = a >= threshold;
    // Boundary conditions: if p == 0, should always return 0;
    // if p == 1, should always return 1
    return b.cast<int>().matrix();
}


VectorXi LogisticRegression::predictBinary(const double threshold) const
{
    return binaryFromP(predict(), threshold);
}


VectorXi LogisticRegression::predictBinary(const MatrixXd& X,
                                           const double threshold,
                                           bool add_intercept) const
{
    return binaryFromP(predictProb(X, add_intercept), threshold);
}


VectorXd LogisticRegression::predictLogit() const
{
    return predictEta().matrix();
}


VectorXd LogisticRegression::predictLogit(const MatrixXd& X,
                                          bool add_intercept) const
{
    if (add_intercept) {
        return predictEta(addInterceptToPredictors(X)).matrix();
    } else {
        return predictEta(X).matrix();
    }
}
