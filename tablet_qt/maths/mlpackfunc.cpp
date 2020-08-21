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

#include "mlpackfunc.h"
#ifdef USE_MLPACK

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Disable warnings for external code:
// - https://stackoverflow.com/questions/3378560/how-to-disable-gcc-warnings-for-a-few-lines-of-code
// - https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
// (It seems, empirically, that the warnings have to be disable for the
// #include rather than the use of template functions.)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
// ............................................................................

#include <mlpack/methods/logistic_regression/logistic_regression.hpp>
// unused parameters in op_sum::apply_noalias_proxy_mp() from op_sum_meat.hpp


using namespace mlpack::regression;


namespace mlpackfunc
{


// ============================================================================
// Logistic regression
// ============================================================================

arma::vec getParamsLogisticFitSinglePredictor(
        const arma::vec& predictors,
        const arma::Row<size_t>& responses)
{
    LogisticRegression<arma::vec> lr(predictors, responses);
    // the template type is the type of predictors
    return lr.Parameters();
}


}  // namespace mlpackfunc

// ............................................................................
#pragma GCC diagnostic pop
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#endif
