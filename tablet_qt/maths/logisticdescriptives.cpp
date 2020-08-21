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

#include "logisticdescriptives.h"
#include <limits>
#include "maths/eigenfunc.h"
#include "maths/logisticregression.h"
#include "maths/statsfunc.h"


// ============================================================================
// Constructors and associated internals
// ============================================================================

LogisticDescriptives::LogisticDescriptives() :
    m_ok(false),
    m_b0(std::numeric_limits<double>::quiet_NaN()),
    m_b1(std::numeric_limits<double>::quiet_NaN())
{
}


LogisticDescriptives::LogisticDescriptives(
        const Eigen::VectorXd& coefficients) :
    LogisticDescriptives()  // delegating constructor
{
    if (coefficients.size() == 2) {
        setFromGlmCoefficients(coefficients(0), coefficients(1));
    }
}


LogisticDescriptives::LogisticDescriptives(
        const QVector<qreal>& coefficients) :
    LogisticDescriptives()  // delegating constructor
{
    if (coefficients.length() == 2) {
        setFromGlmCoefficients(coefficients.at(0), coefficients.at(1));
    }
}


LogisticDescriptives::LogisticDescriptives(const double intercept,
                                           const double slope) :
    LogisticDescriptives()  // delegating constructor
{
    setFromGlmCoefficients(intercept, slope);
}


LogisticDescriptives::LogisticDescriptives(const QVector<qreal>& x,
                                           const QVector<int>& y,
                                           const bool verbose) :
    LogisticDescriptives()  // delegating constructor
{
    using namespace eigenfunc;
    if (x.size() != y.size()) {
        qCritical("Size-mismatched data set passed to LogisticDescriptives");
        return;
    }
    if (x.empty()) {
        qWarning("Empty data set passed to LogisticDescriptives");
        return;
    }
    const ColumnVector<qreal> predictors = eigenColumnVectorFromQVector<qreal>(x);
    const RowVector<int> responses = eigenColumnVectorFromQVector<int>(y);
    LogisticRegression lr;
    lr.setVerbose(verbose);
    lr.fitAddingIntercept(predictors, responses);
    ColumnVector<double> coefficients = lr.coefficients();
    if (coefficients.size() != 2) {
        return;
    }
    setFromGlmCoefficients(coefficients(0), coefficients(1));
}


void LogisticDescriptives::setFromGlmCoefficients(const double b0,
                                                  const double b1)
{
    m_b0 = b0;
    m_b1 = b1;
    m_ok = true;
}


// ============================================================================
// Values
// ============================================================================

double LogisticDescriptives::b0() const
{
    return m_b0;
}


double LogisticDescriptives::b1() const
{
    return m_b1;
}


double LogisticDescriptives::intercept() const
{
    return m_b0;  // intercept = b0
}


double LogisticDescriptives::slope() const
{
    return m_b1;  // slope = b1
}


double LogisticDescriptives::k() const
{
    return m_b1;  // k = slope = b1
}


double LogisticDescriptives::theta() const
{
    return -m_b0 / m_b1;  // theta = -intercept/k = -b0/b1
}


// ============================================================================
// Prediction
// ============================================================================

double LogisticDescriptives::p(const double x) const
{
    return statsfunc::logistic(m_b0 + m_b1 * x);
}


double LogisticDescriptives::x(const double p) const
{
    return (statsfunc::logit(p) - m_b0) / m_b1;
}


double LogisticDescriptives::x50() const
{
    // theta is the value of x for which p = 0.5
    return theta();
}
