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

#include "dqrls.h"
using namespace Eigen;

namespace dqrls {


DqrlsResult Cdqrls(const MatrixXd& x,  // n,p
                   const MatrixXd& y,  // n,ny
                   const double tol,
                   const bool check)
{
    DqrlsResult result;
    const Index n = x.rows();  // number of observations
    // const Index p = x.cols();  // number of predictors
    // const Index ny = y.cols();  // number of dependent variables
    if (check) {
        if (y.rows() != n) {
            result.errors.append(QString(
                "Y vector has %1 rows but this should match the number of "
                "observations (number of X rows), %2").arg(
                    QString::number(y.rows()),
                    QString::number(n)));
            return result;
        }
    }
    result.tol = tol;

    // Here's the equivalent of the F77_CALL(dqrls, ...) code:
    result.qr = x.fullPivHouseholderQr();
    result.qr.setThreshold(tol);  // I think...
    result.coefficients = result.qr.solve(y);

    // result.residuals = XXX;
    // result.effects = XXX;
    result.rank = result.qr.rank();
    result.pivoted = result.qr.nonzeroPivots() > 0;
    result.success = true;
    return result;
}


}  // namespace dqrls
