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

// Eigen 3.3.3 doesn't compile with gcc 7 with "-Wall", so we disable the
// warning that it fails with:

#include "common/preprocessor_aid.h"

#ifdef GCC_HAS_WARNING_INT_IN_BOOL_CONTEXT
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wint-in-bool-context"
#endif

#include <Eigen/Core>

#ifdef GCC_HAS_WARNING_INT_IN_BOOL_CONTEXT
    #pragma GCC diagnostic pop
#endif
