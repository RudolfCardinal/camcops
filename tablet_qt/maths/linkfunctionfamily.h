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

// #define LINK_FUNCTION_FAMILY_USE_AIC

#include <functional>
#include <QString>
#include "maths/include_eigen_dense.h"


// Represents a link function family for a generalized linear model.

class LinkFunctionFamily
{
public:
    using LinkFnType = std::function<Eigen::ArrayXXd(const Eigen::ArrayXXd&)>;
    using InvLinkFnType = std::function<Eigen::ArrayXXd(const Eigen::ArrayXXd&)>;
    using DerivativeInvLinkFnType = std::function<Eigen::ArrayXXd(const Eigen::ArrayXXd&)>;
    using VarianceFnType = std::function<Eigen::ArrayXXd(const Eigen::ArrayXXd&)>;
    using DevResidsFnType = std::function<Eigen::ArrayXd(
            const Eigen::ArrayXd&,  // y
            const Eigen::ArrayXd&,  // mu
            const Eigen::ArrayXd&  // wt
        )>;
    using ValidEtaFnType = std::function<bool(const Eigen::ArrayXd&)>;
    using ValidMuFnType = std::function<bool(const Eigen::ArrayXd&)>;
    using InitializeFnType = std::function<bool(  // returns: OK?
        QStringList&,  // errors
        const LinkFunctionFamily&,  // family
        Eigen::ArrayXd&,  // y
        Eigen::ArrayXd&,  // n
        Eigen::ArrayXd&,  // m
        Eigen::ArrayXd&,  // weights
        Eigen::ArrayXd&,  // start
        Eigen::ArrayXd&,  // etastart
        Eigen::ArrayXd&   // mustart
        )>;
#ifdef LINK_FUNCTION_FAMILY_USE_AIC
    using AICFnType = std::function<double(
            const Eigen::ArrayXd&,  // y, definitely vector (from glm.fit)
            const Eigen::ArrayXd&,  // n (NO IDEA from R glm.fit)
            const Eigen::ArrayXd&,  // mu, definitely vector (from glm.fit)
            const Eigen::ArrayXd&,  // wt, definitely vector (from glm.fit)
            double  // dev, definitely scalar (from glm.fit)
        )>;
#endif

    LinkFunctionFamily(
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
            );
public:
    // For nasty hacks, like R does ;)
    QString family_name;  // "family" in R

    // Link function, giving eta = link(mu):
    LinkFnType link_fn;

    // Inverse link function, giving mu = invlink(eta):
    InvLinkFnType inv_link_fn;

    // Derivative of the inverse link function ("mu.eta" in R).
    // This gives d(mu)/d(eta) as a function of eta.
    DerivativeInvLinkFnType derivative_inv_link_fn;

    // Variance function: gives the variance as a function of the mean; "the
    // part of the variance that depends on" the mean; V(mu).
    // https://en.wikipedia.org/wiki/Variance_function
    // If the variance is independent of the mean, then this should return a
    // constant, probably 1.
    VarianceFnType variance_fn;

    // As per "dev.resids" in "?family" in R:
    //      Function giving the deviance for each observation as a
    //      function of ‘(y, mu, wt)’, used by the ‘residuals’ method
    //      when computing deviance residuals.
    // For example, the unit deviance for the normal distribution is given by
    //      d(y, mu) = (y - mu)^2
    // ... https://en.wikipedia.org/wiki/Deviance_(statistics)
    // and so that is what statsfunc::gaussianDevResids() provides, as one of
    // the functions that might be used here.
    DevResidsFnType dev_resids_fn;

    // Validate the linear predictors
    ValidEtaFnType valid_eta_fn;

    // Validate the means
    ValidMuFnType valid_mu_fn;

    // GLM initialization (ugly eval() code in R)
    InitializeFnType initialize_fn;

#ifdef LINK_FUNCTION_FAMILY_USE_AIC
    // AIC (Aikake information criterion) calculation ("aic" in R)
    AICFnType aic_fn;
#endif
};


extern const QString LINK_FAMILY_NAME_GAUSSIAN;
extern const QString LINK_FAMILY_NAME_BINOMIAL;
extern const QString LINK_FAMILY_NAME_POISSON;

// Some concrete link function families:

extern const LinkFunctionFamily LINK_FN_FAMILY_GAUSSIAN;  // default for glm() in R
extern const LinkFunctionFamily LINK_FN_FAMILY_LOGIT;
extern const LinkFunctionFamily LINK_FN_FAMILY_POISSON;
