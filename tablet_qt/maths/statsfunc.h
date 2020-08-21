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

// #define STATSFUNC_OFFER_AIC

#include "maths/include_eigen_dense.h"

class LinkFunctionFamily;
class QStringList;


namespace statsfunc {

// ============================================================================
// Eigen statistical calculations
// ============================================================================

// Calculates the (sample) variance of an Eigen array.
template<typename Derived>
double variance(const Eigen::ArrayBase<Derived>& a)
{
    double n = a.size();
    double mean = a.mean();
    double ss = (a - mean).square().sum();
    // ... sum of squared deviations from the mean
    return ss / (n - 1);
}


// Calculates the (sample) variance of an Eigen matrix.
template<typename Derived>
double variance(const Eigen::MatrixBase<Derived>& m)
{
    // For elementwise operations, we need an Array, not a Matrix.
    return variance(m.array());
}


// ============================================================================
// Elementary functions
// ============================================================================

// Returns x unmodified.
double identity(double x);

// Returns x unmodified.
Eigen::ArrayXXd identityArray(const Eigen::ArrayXXd& x);

// Returns 1. (Derivative of the identity function.)
double one(double x);

// Returns an array of ones of the same size as x.
Eigen::ArrayXXd oneArray(const Eigen::ArrayXXd& x);

// Returns the natural log of x, where x is an array.
Eigen::ArrayXXd logArray(const Eigen::ArrayXXd& x);

// Returns e^x, where x is an array.
Eigen::ArrayXXd expArray(const Eigen::ArrayXXd& x);

// Returns the logistic function of x.
// = 1 / (1 + exp(-x))
// = exp(x) / (1 + exp(x))
// - The core logistic function, a sigmoid.
// - Transforms logit -> probability; inverse of logit()
double logistic(double x);

// Applies the logistic function to x, an array.
Eigen::ArrayXXd logisticArray(const Eigen::ArrayXXd& x);

// Returns logistic(intercept + slope * x).
double logisticInterceptSlope(double x, double intercept, double slope);

// Generalized logistic function with k steepness, x0 midpoint.
// https://en.wikipedia.org/wiki/Logistic_function
// = logistic(k * (x - x0));
double logisticX0K(double x, double x0, double k);

// Derivative of logistic function
// = exp(x) / (1 + exp(x))^2
// = f(x)(1 - f(x))      where f(x) = logistic(x) = 1 / (1 + exp(-x))
// https://en.wikipedia.org/wiki/Logistic_function#Derivative
double derivativeOfLogistic(double x);

// Derivative of logistic function, applied to an array.
Eigen::ArrayXXd derivativeOfLogisticArray(const Eigen::ArrayXXd& x);

// Logit function
// = inverse of logistic function
// = log(p / (1 - p))
// - Transforms probability -> logit; inverse of logistic()
// - https://en.wikipedia.org/wiki/Logit
// - Uses natural logs (std::log).
double logit(double p);

// Logit function, applied to an array.
Eigen::ArrayXXd logitArray(const Eigen::ArrayXXd& p);

// Returns true
bool alwaysTrue(const Eigen::ArrayXd& x);

// Are all of the array elements integer (or within threshold of an integer)?
bool allInteger(const Eigen::ArrayXd& x, double threshold = 0.001);


// ============================================================================
// Functions for specific GLM families
// ============================================================================

// ----------------------------------------------------------------------------
// binomial
// ----------------------------------------------------------------------------

// Binomial variance function.
// - R: binomial()$variance
// - https://en.wikipedia.org/wiki/Variance_function#Example_.E2.80.93_Bernoulli
Eigen::ArrayXXd binomialVariance(const Eigen::ArrayXXd& x);

// R: binomial_dev_resids()
Eigen::ArrayXd binomialDevResids(const Eigen::ArrayXd& y,
                                 const Eigen::ArrayXd& mu,
                                 const Eigen::ArrayXd& wt);

// R: binomial()$validmu
bool binomialValidMu(const Eigen::ArrayXd& mu);

// R: binomial()$initialize
// Returns: OK?
bool binomialInitialize(QStringList& errors,
                        const LinkFunctionFamily& family,
                        Eigen::ArrayXd& y,
                        Eigen::ArrayXd& n,
                        Eigen::ArrayXd& m,
                        Eigen::ArrayXd& weights,
                        Eigen::ArrayXd& start,
                        Eigen::ArrayXd& etastart,
                        Eigen::ArrayXd& mustart);

#ifdef STATSFUNC_OFFER_AIC

// As per R's dbinom()
double dbinom(double x, int n, double p, bool log = false);

// As per R's dbinom()
Eigen::ArrayXd dbinom(const Eigen::ArrayXd& x,
                      const Eigen::ArrayXi& n,
                      const Eigen::ArrayXd& p,
                      bool log = false);

// R: binomial()$aic
double binomialAIC(const Eigen::ArrayXd& y,
                   const Eigen::ArrayXd& n,
                   const Eigen::ArrayXd& mu,
                   const Eigen::ArrayXd& wt,
                   double dev);

#endif

// ----------------------------------------------------------------------------
// gaussian
// ----------------------------------------------------------------------------

// R: gaussian()$dev.resids
Eigen::ArrayXd gaussianDevResids(const Eigen::ArrayXd& y,
                                 const Eigen::ArrayXd& mu,
                                 const Eigen::ArrayXd& wt);

// R: gaussian()$initialize
// Returns: OK?
bool gaussianInitialize(QStringList& errors,
                        const LinkFunctionFamily& family,
                        Eigen::ArrayXd& y,
                        Eigen::ArrayXd& n,
                        Eigen::ArrayXd& m,
                        Eigen::ArrayXd& weights,
                        Eigen::ArrayXd& start,
                        Eigen::ArrayXd& etastart,
                        Eigen::ArrayXd& mustart);

#ifdef STATSFUNC_OFFER_AIC

// R: gaussian()$aic
double gaussianAIC(const Eigen::ArrayXd& y,
                   const Eigen::ArrayXd& n,
                   const Eigen::ArrayXd& mu,
                   const Eigen::ArrayXd& wt,
                   double dev);

#endif

// ----------------------------------------------------------------------------
// poisson
// ----------------------------------------------------------------------------

// R: poisson()$validmu
bool poissonValidMu(const Eigen::ArrayXd& mu);


// R: poisson()$dev.resids
Eigen::ArrayXd poissonDevResids(const Eigen::ArrayXd& y,
                                const Eigen::ArrayXd& mu,
                                const Eigen::ArrayXd& wt);


// R: poisson()$initialize
// Returns: OK?
bool poissonInitialize(QStringList& errors,
                       const LinkFunctionFamily& family,
                       Eigen::ArrayXd& y,
                       Eigen::ArrayXd& n,
                       Eigen::ArrayXd& m,
                       Eigen::ArrayXd& weights,
                       Eigen::ArrayXd& start,
                       Eigen::ArrayXd& etastart,
                       Eigen::ArrayXd& mustart);


#ifdef STATSFUNC_OFFER_AIC

// R: poisson()$aic
double poissonAIC(const Eigen::ArrayXd& y,
                  const Eigen::ArrayXd& n,
                  const Eigen::ArrayXd& mu,
                  const Eigen::ArrayXd& wt,
                  double dev);  // NOT YET IMPLEMENTED

#endif


// ============================================================================
// Solving
// ============================================================================

// Singular value decomposition (SVD) solving.
// Solves Ax = b [or b = Ax + e], for x, minimizing e (in a least-squares
// sense).
Eigen::VectorXd svdSolve(const Eigen::MatrixXd& A,
                         const Eigen::VectorXd& b);  // solves Ax = b [or b = Ax + e], for x, minimizing e


// ============================================================================
// GLM support
// ============================================================================
// ... see glm.h

/*
Eigen::Array<Eigen::Index, Eigen::Dynamic, 1> svdsubsel(
        const Eigen::MatrixXd& A, int k = -1);
*/


}  // namespace statsfunc
