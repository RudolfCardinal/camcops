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

#include <QStringList>
#include "maths/include_eigen_dense.h"

// See:
// - http://adv-r.had.co.nz/C-interface.html
// - R's lm.c

namespace dqrls {


// Represents the result of Cdqrls()
struct DqrlsResult {
    bool success = false;  // did we succeed?
    Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qr;  // QR decomposition
    Eigen::MatrixXd coefficients;  // the results; B in "XB = Y"; see below
    // Eigen::ArrayXXd residuals;
    // Eigen::ArrayXXd effects;
    Eigen::Index rank;  // rank of the QR decomposition
    // Eigen::ArrayXi pivot;  // INDICES, 0-based here, 1-based in R
    // Eigen::ArrayXd qraux;
    double tol;  // tolerance
    bool pivoted;  // did the QR decomposition have nonzero pivots?
        // ... I have no idea what that means.
    QStringList errors;
};


// Solves XB = Y, for B.
//
// Provides the C++ equivalent of the Fortran dqrls code.
// Calculates a least-squares solution to this matrix equation.
//
// - X has size (n, p)
// - Y has size (n, ny)
// - n: number of observations
// - p: number of predictors
// - ny: number of dependent variables
//
// - B will have size (p, ny).
//
// Returns a DqrlsResult object in which B is called "coefficients".
DqrlsResult Cdqrls(const Eigen::MatrixXd& x,  // size (n, p)
                   const Eigen::MatrixXd& y,  // size (n, ny)
                   double tol,  // tolerance
                   bool check = true);  // check dimensions of result are right


}  // namespace dqrls
