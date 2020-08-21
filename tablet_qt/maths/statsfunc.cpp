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

#include "statsfunc.h"
#include <cmath>
#include <limits>
#include "maths/eigenfunc.h"
using namespace Eigen;
using eigenfunc::ArrayXb;


// ============================================================================
// Static (file-local) definitions
// ============================================================================
// As per R's family.c:
// - https://github.com/wch/r-source/blob/trunk/src/library/stats/src/family.c

static const double PI = 3.141592653589793238462643383279502884197169399375;  // as per R's Constants.h
static const double DOUBLE_EPS = std::numeric_limits<double>::epsilon();
static const double THRESH = 30.;
static const double MTHRESH = -30.;
static const double INVEPS = 1/DOUBLE_EPS;


static inline
double x_d_opx(const double x)
{
    return x / (1 + x);
}


static inline
double x_d_omx(const double x)
{
    return x / (1 - x);
}


static inline
double y_log_y(const double y, const double mu)
{
    return (y != 0.) ? (y * std::log(y / mu)) : 0;
}



namespace statsfunc {

// ============================================================================
// Elementary functions
// ============================================================================

double identity(const double x)
{
    return x;
}


ArrayXXd identityArray(const ArrayXXd& x)
{
    return x;
}


double one(const double x)
{
    // Derivative of the identity function:
    //      y = x
    //      y' = 1
    Q_UNUSED(x)
    return 1.0;
}


ArrayXXd oneArray(const ArrayXXd& x)
{
    ArrayXXd result(x.rows(), x.cols());
    result.setConstant(1.0);
    return result;
}


Eigen::ArrayXXd logArray(const Eigen::ArrayXXd& x)
{
    return x.log();  // or Eigen::log(x)
}


Eigen::ArrayXXd expArray(const Eigen::ArrayXXd& x)
{
    return x.exp();  // or Eigen::exp(x);
}


double logistic(const double x)
{
    // = 1 / (1 + exp(-x))
    // = exp(x) / (1 + exp(x))
    // - The core logistic function, a sigmoid.
    // - Transforms logit -> probability; inverse of logit()
    // - In R's family.c, the equivalent is logit_linkinv().
    // - Curiously, that does exp(x)/(1 + exp(x)), which is mathematically
    //   equivalent but maybe performs better; I shall trust R.
    //   It also checks for numerical limits, as follows.
    const double tmp = x < MTHRESH ? DOUBLE_EPS
                                   : (x > THRESH ? INVEPS : exp(x));
    return x_d_opx(tmp);
}


ArrayXXd logisticArray(const ArrayXXd& x)
{
    return x.unaryExpr(&logistic);
}


double logisticInterceptSlope(const double x,
                              const double intercept, const double slope)
{
    return logistic(intercept + slope * x);
}


double logisticX0K(const double x, const double x0, const double k)
{
    // Generalized logistic function with k steepness, x0 midpoint
    // https://en.wikipedia.org/wiki/Logistic_function
    return logistic(k * (x - x0));

    // HOWEVER, note that there are other formulations of slope/intercept:
    // see e.g. mathfunc::LogisticDescriptives, and as above.
}


double derivativeOfLogistic(const double x)
{
    // = exp(x) / (1 + exp(x))^2
    // = f(x)(1 - f(x))      where f(x) = logistic(x) = 1 / (1 + exp(-x))
    // https://en.wikipedia.org/wiki/Logistic_function#Derivative
    // In R's family.c, logit_mu_eta
    // Let's follow R's method, but improve its sequencing (it calculates opexp
    // when it may ignore the result).
    // Notation: "opexp" = "one plus exp()".
    if (x > THRESH || x < MTHRESH) {
        return DOUBLE_EPS;
    }
    const double opexp = 1 + std::exp(x);
    return std::exp(x) / (opexp * opexp);
}


ArrayXXd derivativeOfLogisticArray(const ArrayXXd& x)
{
    return x.unaryExpr(&derivativeOfLogistic);
}


double logit(const double p)
{
    // = log(p / (1 - p))
    // - Transforms probability -> logit; inverse of logistic()
    // - https://en.wikipedia.org/wiki/Logit
    // - Uses natural logs (std::log).
    // - In R's family.c, the equivalent is logit_link().
    return std::log(x_d_omx(p));
}


ArrayXXd logitArray(const ArrayXXd& p)
{
    return p.unaryExpr(&logit);
}


bool alwaysTrue(const ArrayXd& x)
{
    Q_UNUSED(x)
    return true;
}


bool allInteger(const Eigen::ArrayXd& x, double threshold)
{
    return ((x - x.round()).abs() <= threshold).all();
    //       ^^^^^^^^^^^^^
    //       non-integer part
    //      ^^^^^^^^^^^^^^^^^^^^^
    //      absolute non-integer part
}


// ============================================================================
// Functions for specific GLM families
// ============================================================================

// ----------------------------------------------------------------------------
// binomial
// ----------------------------------------------------------------------------

ArrayXXd binomialVariance(const ArrayXXd& mu)
{
    // - R: binomial()$variance
    // - https://en.wikipedia.org/wiki/Variance_function#Example_.E2.80.93_Bernoulli
    return mu * (1.0 - mu);
}


ArrayXd binomialDevResids(const ArrayXd& y,
                          const ArrayXd& mu,
                          const ArrayXd& wt)
{
    // R: binomial_dev_resids() in src/library/stats/src/family.c
    // ... but it's much simpler in Eigen! At least if you assume conformable
    // arrays. That means "lmu > 1" and "lwt > 1", effectively.
    const Index n = y.size();
    ArrayXd yly_y(n);
    ArrayXd yly_omy(n);
    for (Index i = 0; i < n; ++i) {
        const double y_i = y(i);
        const double mu_i = mu(i);
        yly_y(i)   = y_log_y(    y_i,     mu_i);
        yly_omy(i) = y_log_y(1 - y_i, 1 - mu_i);
    }

    return 2 * wt * (yly_y + yly_omy);
}


bool binomialValidMu(const ArrayXd& mu)
{
    // R: binomial()$validmu
    return mu.isFinite().all() && (mu > 0 && mu < 1).all();
}


bool binomialInitialize(QStringList& errors,
                        const LinkFunctionFamily& family,
                        Eigen::ArrayXd& y,
                        Eigen::ArrayXd& n,
                        Eigen::ArrayXd& m,
                        Eigen::ArrayXd& weights,
                        Eigen::ArrayXd& start,
                        Eigen::ArrayXd& etastart,
                        Eigen::ArrayXd& mustart)
{
    // returns: OK?
    // R: binomial()$initialize
    Q_UNUSED(family)
    Q_UNUSED(start)
    Q_UNUSED(etastart)

    const Index ncol_y = y.cols();
    const Index nobs = y.size();

    if (ncol_y == 1) {
        // NOT HANDLED: factors
        n = ArrayXd::Ones(nobs);
        y = (weights == 0).select(0, y);
        if ((y < 0 || y > 1).any()) {
            errors.append("y values must be 0 <= y <= 1");
            return false;
        }
        mustart = (weights * y + 0.5) / (weights + 1);
        m = weights * y;
        if (!allInteger(m)) {
            errors.append("warning: non-integer #successes in a binomial glm!");
            // ... but continue
        }
    } else if (ncol_y == 2) {
        if (!allInteger(y)) {
            errors.append("warning: non-integer counts in a binomial glm!");
            // ... but continue
        }
        n = y.col(0) + y.col(1);
        y = (n == 0).select(0, y.col(0) / n);
        weights = weights * n;
        mustart = (n * y + 0.5) / (n + 1);
    } else {
        errors.append(
                    "for the 'binomial' family, y must be a vector of 0 and "
                    "1's or a 2 column matrix where col 1 is no. successes and "
                    "col 2 is no. failures [THOUGH probably not all of that "
                    "implemented!]");
        return false;
    }

    return true;
}


#ifdef STATSFUNC_OFFER_AIC
double dbinom(const double x,
              const int n, const double p, const bool log)
{
    // As per R's dbinom.c
}


ArrayXd dbinom(const ArrayXd& x, const ArrayXi& n, const ArrayXd& p,
               const bool log)
{
    // R recycles arguments, I think; we'll ignore that for now.
    Q_ASSERT(n.size() == x.size());
    Q_ASSERT(p.size() == x.size());
    const int len = x.size();
    ArrayXd d(len);
    for (int i = 0; i < len; ++i) {
        d[i] = dbinom(x[i], n[i], p[i], log);
    }
}


double binomialAIC(const ArrayXd& y,
                   const ArrayXd& n,
                   const ArrayXd& mu,
                   const ArrayXd& wt,
                   const double dev)
{
    // R: binomial()$aic
    const int nobs = y.size();

    ArrayXd m(nobs);
    if ((n > 1).any()) {
        m = n;
    } else {
        m = wt;
    }

    // -2 * sum(ifelse(m > 0,
    //                 (wt/m),
    //                 0)      * dbinom(round(m * y),   // x
    //                                  round(m),       // size
    //                                  mu,             // prob
    //                                  log = TRUE)   )

    ArrayXd wt_over_m = (m > 0).select(wt / m, 0);
    ArrayXd binom_dens = dbinom((m * y).round(), m.round(), mu, true);
    return -2 * (wt_over_m * binom_dens).sum();
}
#endif


// ----------------------------------------------------------------------------
// gaussian
// ----------------------------------------------------------------------------

ArrayXd gaussianDevResids(const ArrayXd& y,
                          const ArrayXd& mu,
                          const ArrayXd& wt)
{
    // R: gaussian()$dev.resids
    return wt * ((y - mu).square());
}


bool gaussianInitialize(QStringList& errors,
                        const LinkFunctionFamily& family,
                        Eigen::ArrayXd& y,
                        Eigen::ArrayXd& n,
                        Eigen::ArrayXd& m,
                        Eigen::ArrayXd& weights,
                        Eigen::ArrayXd& start,
                        Eigen::ArrayXd& etastart,
                        Eigen::ArrayXd& mustart)
{
    // returns: OK?
    // R: gaussian()$initialize

    Q_UNUSED(errors)
    Q_UNUSED(family)
    Q_UNUSED(m)
    Q_UNUSED(weights)
    Q_UNUSED(start)
    Q_UNUSED(etastart)

    // NOT IMPLEMENTED: some other options for inverse/log links; q.v.

    const Index nobs = y.size();
    n = ArrayXd::Ones(nobs);
    mustart = y;

    return true;
}


#ifdef STATSFUNC_OFFER_AIC

double gaussianAIC(const ArrayXd& y,
                   const ArrayXd& n,
                   const ArrayXd& mu,
                   const ArrayXd& wt,
                   const double dev)
{
    // R: gaussian()$aic
    Q_UNUSED(n)
    Q_UNUSED(mu)
    const Index nobs = y.size();
    return nobs * (std::log(dev / nobs * 2 * PI) + 1) + 2 - wt.log().sum();
}

#endif


// ----------------------------------------------------------------------------
// poisson
// ----------------------------------------------------------------------------

bool poissonValidMu(const Eigen::ArrayXd& mu)
{
    return mu.isFinite().all() && (mu > 0).all();
}


ArrayXd poissonDevResids(const ArrayXd& y,
                         const ArrayXd& mu,
                         const ArrayXd& wt)
{
    // R: poisson()$dev.resids
    // Original:
    //      r <- mu * wt
    //      p <- which(y > 0)
    //      r[p] <- (wt * (y * log(y/mu) - (y - mu)))[p]
    //      2 * r

    const ArrayXb p = y > 0;
    const ArrayXd r = p.select(
        // where p true (y > 0):
        wt * (y * log(y / mu) - (y - mu)),
        // where p false (y = 0):
        wt * mu
    );
    return 2 * r;
}


bool poissonInitialize(QStringList& errors,
                       const LinkFunctionFamily& family,
                       Eigen::ArrayXd& y,
                       Eigen::ArrayXd& n,
                       Eigen::ArrayXd& m,
                       Eigen::ArrayXd& weights,
                       Eigen::ArrayXd& start,
                       Eigen::ArrayXd& etastart,
                       Eigen::ArrayXd& mustart)
{
    // R: poisson()$initialize

    Q_UNUSED(errors)
    Q_UNUSED(family)
    Q_UNUSED(m)
    Q_UNUSED(weights)
    Q_UNUSED(start)
    Q_UNUSED(etastart)

    if ((y < 0).any()) {
        qWarning() << "negative values not allowed for the 'Poisson' family";
        return false;
    }

    const Index nobs = y.size();
    n = ArrayXd::Ones(nobs);
    mustart = y + 0.1;

    return true;
}


// ============================================================================
// Solving
// ============================================================================

VectorXd svdSolve(const MatrixXd& A, const VectorXd& b)
{
    // solves Ax = b [or b = Ax + e], for x, minimizing e (in a least-squares sense)
    // https://eigen.tuxfamily.org/dox/group__LeastSquares.html
    return A.jacobiSvd(ComputeThinU | ComputeThinV).solve(b);
}


}  // namespace statsfunc
