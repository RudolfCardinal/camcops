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

#include "linkfunctionfamily.h"
#include "maths/statsfunc.h"


LinkFunctionFamily::LinkFunctionFamily(
        const QString& family_name,
        const LinkFnType& link_fn,
        const InvLinkFnType& inv_link_fn,
        const DerivativeInvLinkFnType& derivative_inv_link_fn,
        const VarianceFnType& variance_fn,
        const DevResidsFnType& dev_resids_fn,
        const ValidEtaFnType& valid_eta_fn,
        const ValidMuFnType& valid_mu_fn,
        const InitializeFnType& initialize_fn
#ifdef LINK_FUNCTION_FAMILY_USE_AIC
        , const AICFnType& aic_fn
#endif
        ) :
    family_name(family_name),
    link_fn(link_fn),
    inv_link_fn(inv_link_fn),
    derivative_inv_link_fn(derivative_inv_link_fn),
    variance_fn(variance_fn),
    dev_resids_fn(dev_resids_fn),
    valid_eta_fn(valid_eta_fn),
    valid_mu_fn(valid_mu_fn),
    initialize_fn(initialize_fn)
#ifdef LINK_FUNCTION_FAMILY_USE_AIC
    , aic_fn(aic_fn)
#endif
{
}


const QString LINK_FAMILY_NAME_GAUSSIAN("gaussian");
const QString LINK_FAMILY_NAME_BINOMIAL("binomial");
const QString LINK_FAMILY_NAME_POISSON("poisson");


// Disambiguating overloaded functions:
// - https://stackoverflow.com/questions/10111042/wrap-overloaded-function-via-stdfunction

const LinkFunctionFamily LINK_FN_FAMILY_LOGIT(
        // R: binomial(), or binomial(link = "logit") to be explicit.
        LINK_FAMILY_NAME_BINOMIAL,  // family_name; binomial()$family
        statsfunc::logitArray,  // link function; binomial()$linkfun; eta = logit(mu)
        statsfunc::logisticArray,  // inverse link function; binomial()$linkinv; mu = logistic(eta)
        statsfunc::derivativeOfLogisticArray,  // derivative of inverse link function; binomial()$mu.eta
        statsfunc::binomialVariance,  // variance function; binomial()$variance; V(mu) = mu * (1 - mu)
        statsfunc::binomialDevResids,  // dev_resids_fn; binomial()$dev.resids
        statsfunc::alwaysTrue,  // valid_eta_fn; binomial()$valideta
        statsfunc::binomialValidMu,  // valid_mu_fn; binomial()validmu
        statsfunc::binomialInitialize  // initialize_fn; binomial()$initialize
#ifdef LINK_FUNCTION_FAMILY_USE_AIC
        , statsfunc::binomialAIC  // aic_fn; binomial()$aic
#endif
);


const LinkFunctionFamily LINK_FN_FAMILY_GAUSSIAN(
        // R: gaussian(), or gaussian(link = "identity").
        LINK_FAMILY_NAME_GAUSSIAN,  // family_name; gaussian()$family
        statsfunc::identityArray,  // link function; gaussian()$linkfun; eta = me
        statsfunc::identityArray,  // inverse link function; gaussian()$linkinv; mu = eta
        statsfunc::oneArray,  // derivative of inverse link function; gaussian()$mu.eta; y = x => dy/dx = y' = 1
        statsfunc::oneArray,  // variance function; gaussian()$variance; V(mu) = 1
            // ... variance is independent of the mean
            // ... https://en.wikipedia.org/wiki/Variance_function#Example_.E2.80.93_normal
        statsfunc::gaussianDevResids,  // dev_resids_fn; gaussian()$dev.resids
        statsfunc::alwaysTrue,  // valid_eta_fn; gaussian()$valideta
        statsfunc::alwaysTrue,  // valid_mu_fn; gaussian()validmu
        statsfunc::gaussianInitialize  // initialize_fn; gaussian()$initialize
#ifdef LINK_FUNCTION_FAMILY_USE_AIC
        , statsfunc::gaussianAIC  // aic_fn; gaussian()$aic
#endif
);


const LinkFunctionFamily LINK_FN_FAMILY_POISSON(
        // R: poisson(), or poisson(link = "log").
        LINK_FAMILY_NAME_POISSON,  // family_name; poisson()$family
        statsfunc::logArray,  // link function; poisson()$linkfun; eta = log(mu)
        statsfunc::expArray,  // inverse link function; poisson()$linkinv; mu = exp(eta)
        statsfunc::expArray,  // derivative of inverse link function; poisson()$mu.eta; mu' = exp(eta)
        statsfunc::identityArray,  // variance function; poisson()$variance; V(mu) = mu
        statsfunc::poissonDevResids,  // dev_resids_fn; poisson()$dev.resids
        statsfunc::alwaysTrue,  // valid_eta_fn; poisson()$valideta
        statsfunc::poissonValidMu,  // valid_mu_fn; poisson()$validmu
        statsfunc::poissonInitialize  // initialize_fn; poisson()$initialize
#ifdef LINK_FUNCTION_FAMILY_USE_AIC
        , statsfunc::poissonAIC  // aic_fn; poisson()$aic
#endif
);

// For link function families, see also:
// - https://stats.stackexchange.com/questions/212430/what-are-the-error-distribution-and-link-functions-of-a-model-family-in-r
// - https://en.wikipedia.org/wiki/Generalized_linear_model#Link_function
// - R: ?family
