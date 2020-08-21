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
#include <QString>


namespace appstrings
{

// Names of strings in the "appstring" zone of the downloaded strings (i.e.
// those used across CamCOPS, rather than relating to a specific task, for
// the most part).
//
// See also internationalization.rst for which strings should live where.

// ============================================================================
// NHS Data Dictionary elements; for details see camcops.xml
// ============================================================================

extern const QString NHS_PERSON_MARITAL_STATUS_CODE_S;
extern const QString NHS_PERSON_MARITAL_STATUS_CODE_M;
extern const QString NHS_PERSON_MARITAL_STATUS_CODE_D;
extern const QString NHS_PERSON_MARITAL_STATUS_CODE_W;
extern const QString NHS_PERSON_MARITAL_STATUS_CODE_P;
extern const QString NHS_PERSON_MARITAL_STATUS_CODE_N;

extern const QString NHS_ETHNIC_CATEGORY_CODE_A;
extern const QString NHS_ETHNIC_CATEGORY_CODE_B;
extern const QString NHS_ETHNIC_CATEGORY_CODE_C;
extern const QString NHS_ETHNIC_CATEGORY_CODE_D;
extern const QString NHS_ETHNIC_CATEGORY_CODE_E;
extern const QString NHS_ETHNIC_CATEGORY_CODE_F;
extern const QString NHS_ETHNIC_CATEGORY_CODE_G;
extern const QString NHS_ETHNIC_CATEGORY_CODE_H;
extern const QString NHS_ETHNIC_CATEGORY_CODE_J;
extern const QString NHS_ETHNIC_CATEGORY_CODE_K;
extern const QString NHS_ETHNIC_CATEGORY_CODE_L;
extern const QString NHS_ETHNIC_CATEGORY_CODE_M;
extern const QString NHS_ETHNIC_CATEGORY_CODE_N;
extern const QString NHS_ETHNIC_CATEGORY_CODE_P;
extern const QString NHS_ETHNIC_CATEGORY_CODE_R;
extern const QString NHS_ETHNIC_CATEGORY_CODE_S;
extern const QString NHS_ETHNIC_CATEGORY_CODE_Z;

// ============================================================================
// String elements for specific restricted tasks (see camcops.xml)
// ============================================================================

extern const QString BDI_WHICH_SCALE;
extern const QString GAF_SCORE;
extern const QString HADS_ANXIETY_SCORE;
extern const QString HADS_DEPRESSION_SCORE;
extern const QString IESR_A_PREFIX;
extern const QString WSAS_A_PREFIX;
extern const QString ZBI_A_PREFIX;

// ============================================================================
// Strings shared across several tasks
// ============================================================================

extern const QString DATA_COLLECTION_ONLY;
extern const QString DATE_PERTAINS_TO;
extern const QString ICD10_SYMPTOMATIC_DISCLAIMER;
extern const QString SATIS_BAD_Q;
extern const QString SATIS_BAD_S;
extern const QString SATIS_GOOD_Q;
extern const QString SATIS_GOOD_S;
extern const QString SATIS_PT_RATING_Q;
extern const QString SATIS_REF_GEN_RATING_Q;
extern const QString SATIS_REF_SPEC_RATING_Q;
extern const QString SATIS_RATING_A_PREFIX;
extern const QString SATIS_SERVICE_BEING_RATED;


}  // namespace appstrings
