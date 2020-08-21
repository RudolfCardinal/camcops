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
#include "maths/glm.h"


// Implements logistic regression, via a GLM.

class LogisticRegression : public Glm
{
public:
    // Construct
    LogisticRegression(
            SolveMethod solve_method = SolveMethod::IRLS_R_glmfit,
            int max_iterations = GLM_DEFAULT_MAX_ITERATIONS,
            double tolerance = GLM_DEFAULT_TOLERANCE,
            RankDeficiencyMethod rank_deficiency_method = RankDeficiencyMethod::Error);

    // Fit
    void fitAddingIntercept(
            const Eigen::MatrixXd& X,  // predictors, EXCLUDING intercept; n_observations x (n_predictors - 1)
            const Eigen::VectorXi& y);  // depvar; n_observations x 1
    void fitDirectly(
            const Eigen::MatrixXd& X,  // predictors, INCLUDING intercept; n_observations x n_predictors
            const Eigen::VectorXi& y);  // depvar; n_observations x 1

    // Predict probabilities:
    Eigen::VectorXd predictProb() const;  // synonym for predict()
    Eigen::VectorXd predictProb(const Eigen::MatrixXd& X,
                                bool add_intercept = true) const;  // with new predictors

    // Predict binary outcomes:
    Eigen::VectorXi predictBinary(double threshold = 0.5) const;  // with original predictors
    Eigen::VectorXi predictBinary(const Eigen::MatrixXd& X,
                                  double threshold = 0.5,
                                  bool add_intercept = true) const;  // with new predictors

    // Predict logit:
    Eigen::VectorXd predictLogit() const;  // synonym for predictEta()
    Eigen::VectorXd predictLogit(const Eigen::MatrixXd& X,
                                 bool add_intercept = true) const;  // with new predictors

protected:
    // Convert probabilities to binary using a threshold:
    Eigen::VectorXi binaryFromP(const Eigen::VectorXd& p,
                                double threshold = 0.5) const;
};
