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
// #define USE_MLPACK  // DEPRECATED in favour of Eigen (which is headers-only)
#include <QVector>

#ifdef USE_MLPACK


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
// ............................................................................

#include <mlpack/core.hpp>

// ............................................................................
#pragma GCC diagnostic pop
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


namespace mlpackfunc
{

// ============================================================================
// Conversion between Qt and Armadillo types
// ============================================================================

template<typename DestContainerT, typename SourceContentsT>
DestContainerT armaVectorFromQVector(const QVector<SourceContentsT>& v)
{
    int n = v.size();
    DestContainerT m(n);
    for (int i = 0; i < n; ++i) {
        m[i] = v.at(i);
    }
    return m;
}


template<typename DestContentsT, typename SourceContentsT>
arma::Col<DestContentsT> armaColumnVectorFromQVector(
        const QVector<SourceContentsT>& v)
{
    return armaVectorFromQVector<arma::Col<DestContentsT>>(v);
}


template<typename DestContentsT, typename SourceContentsT>
arma::Row<DestContentsT> armaRowVectorFromQVector(
        const QVector<SourceContentsT>& v)
{
    return armaVectorFromQVector<arma::Row<DestContentsT>>(v);
}


template<typename DestContentsT, typename SourceContainerT>
QVector<DestContentsT> qVectorFromArmaVector(const SourceContainerT& m)
{
    int n = m.size();
    QVector<DestContentsT> v(n);
    for (int i = 0; i < n; ++i) {
        v[i] = m[i];
    }
    return v;
}


// ============================================================================
// Logistic regression
// ============================================================================

arma::vec getParamsLogisticFitSinglePredictor(
        const arma::vec& predictors,
        const arma::Row<size_t>& responses);


}  // namespace mlpackfunc
#endif
