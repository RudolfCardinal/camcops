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

#include <QVector>
#include "maths/include_eigen_dense.h"


class LogisticDescriptives
{
    // Deals with multiple ways of specifying a simple logistic regression
    // equation with a SINGLE predictor.
    //
    // This class just stores the coefficients. It can be initialized directly
    // with them, or given data with which to perform the regression (via
    // the LogisticRegression class).

public:
    // "Invalid" initialization:
    LogisticDescriptives();

    // Initialize from coefficients:
    LogisticDescriptives(const Eigen::VectorXd& coefficients);
    LogisticDescriptives(const QVector<qreal>& coefficients);
    LogisticDescriptives(double intercept, double slope);

    // Initialize from data:
    LogisticDescriptives(const QVector<qreal>& x, const QVector<int>& y,
                         bool verbose = false);

    // OK?
    bool ok() const;

    // Values:
    double b0() const;
    double b1() const;
    double intercept() const;
    double slope() const;
    double k() const;
    double theta() const;
    double x50() const;

    // Prediction:
    double p(double x) const;
    double x(double p) const;

protected:
    void setFromGlmCoefficients(double b0, double b1);

protected:
    bool m_ok;

    double m_b0;
    double m_b1;

    /*

    1.  These parameters are the GLM versions. We start with these.
        The GLM, ignoring error terms, is:

            Y = logistic(Xb) = logistic(b0 + b1*x)
            logit(Y) = Xb = b0 + b1*x

        The Y value, here, is the probability p, so:

            p = logistic(b0 + b1*x)
              = 1 / (1 + exp(-[b0 + b1*x]));
            logit(p) = b0 + b1*x

        Reversing:

            x = (logit(p) - b0) / b1

        Or, reversing in full:

            P = 1 / (1 + exp(-intercept - slope * X))
            1 = P + P * exp(-intercept - slope * X)
            -intercept - slope*X = ln((1 - P) / P)
            intercept + slope * X = ln(P / (1 - P))
            X = (ln(P / (1 - P)) - intercept) / slope

    2.  From an alternative formulation:
        These parameters define a linear equation in logits,
            L(X) = intercept + slope * X
        The logistic function itself is
            P = plogis(L) = 0.5 * (1 + tanh(L/2)) = 1 / (1 + exp(-L))
        So that's
            P = 1 / (1 + exp(-intercept - slope * X))
        Comparing to Lecluyse & Meddis (2009)'s function,
            p = 1 / (1 + exp(-k(X - theta)))
              = 1 / (1 + exp(-k*X + k*theta))),
        we have
            k = slope
        and
            theta = -intercept/k = -intercept/slope

        Comparing back to the GLM function:

            intercept = b0
            slope = b1
            k = slope = b1
            theta = -intercept/k = -b0/b1      [... since k*theta = -intercept]

    3.  We will have p = 0.5 when

            0.5 = 1 / (1 + exp(-[b0 + b1*x]))
            1 + exp(-[b0 + b1*x]) = 2
            exp(-[b0 + b1*x]) = 1
            -[b0 + b1*x] = 0
            b0 + b1*x = 0
            x = -b0/b1
              = theta

    */
};
