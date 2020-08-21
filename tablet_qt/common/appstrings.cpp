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

#include "appstrings.h"

namespace appstrings
{

// These should match constants in cc_string.py on the server,
// and of course camcops.xml itself.

// ============================================================================
// NHS Data Dictionary elements; for details see camcops.xml
// ============================================================================

const QString NHS_PERSON_MARITAL_STATUS_CODE_S("nhs_person_marital_status_code_S");
const QString NHS_PERSON_MARITAL_STATUS_CODE_M("nhs_person_marital_status_code_M");
const QString NHS_PERSON_MARITAL_STATUS_CODE_D("nhs_person_marital_status_code_D");
const QString NHS_PERSON_MARITAL_STATUS_CODE_W("nhs_person_marital_status_code_W");
const QString NHS_PERSON_MARITAL_STATUS_CODE_P("nhs_person_marital_status_code_P");
const QString NHS_PERSON_MARITAL_STATUS_CODE_N("nhs_person_marital_status_code_N");

const QString NHS_ETHNIC_CATEGORY_CODE_A("nhs_ethnic_category_code_A");
const QString NHS_ETHNIC_CATEGORY_CODE_B("nhs_ethnic_category_code_B");
const QString NHS_ETHNIC_CATEGORY_CODE_C("nhs_ethnic_category_code_C");
const QString NHS_ETHNIC_CATEGORY_CODE_D("nhs_ethnic_category_code_D");
const QString NHS_ETHNIC_CATEGORY_CODE_E("nhs_ethnic_category_code_E");
const QString NHS_ETHNIC_CATEGORY_CODE_F("nhs_ethnic_category_code_F");
const QString NHS_ETHNIC_CATEGORY_CODE_G("nhs_ethnic_category_code_G");
const QString NHS_ETHNIC_CATEGORY_CODE_H("nhs_ethnic_category_code_H");
const QString NHS_ETHNIC_CATEGORY_CODE_J("nhs_ethnic_category_code_J");
const QString NHS_ETHNIC_CATEGORY_CODE_K("nhs_ethnic_category_code_K");
const QString NHS_ETHNIC_CATEGORY_CODE_L("nhs_ethnic_category_code_L");
const QString NHS_ETHNIC_CATEGORY_CODE_M("nhs_ethnic_category_code_M");
const QString NHS_ETHNIC_CATEGORY_CODE_N("nhs_ethnic_category_code_N");
const QString NHS_ETHNIC_CATEGORY_CODE_P("nhs_ethnic_category_code_P");
const QString NHS_ETHNIC_CATEGORY_CODE_R("nhs_ethnic_category_code_R");
const QString NHS_ETHNIC_CATEGORY_CODE_S("nhs_ethnic_category_code_S");
const QString NHS_ETHNIC_CATEGORY_CODE_Z("nhs_ethnic_category_code_Z");

// ============================================================================
// String elements for specific restricted tasks (see camcops.xml)
// ============================================================================

const QString BDI_WHICH_SCALE("bdi_which_scale");
const QString GAF_SCORE("gaf_score");
const QString HADS_ANXIETY_SCORE("hads_anxiety_score");
const QString HADS_DEPRESSION_SCORE("hads_depression_score");
const QString IESR_A_PREFIX("iesr_a");
const QString WSAS_A_PREFIX("wsas_a");
const QString ZBI_A_PREFIX("zbi_a");

// ============================================================================
// Strings shared across several tasks
// ============================================================================

const QString DATA_COLLECTION_ONLY("data_collection_only");
const QString DATE_PERTAINS_TO("date_pertains_to");
const QString ICD10_SYMPTOMATIC_DISCLAIMER("icd10_symptomatic_disclaimer");
const QString SATIS_BAD_Q("satis_bad_q");
const QString SATIS_BAD_S("satis_bad_s");
const QString SATIS_GOOD_Q("satis_good_q");
const QString SATIS_GOOD_S("satis_good_s");
const QString SATIS_PT_RATING_Q("satis_pt_rating_q");
const QString SATIS_REF_GEN_RATING_Q("satis_ref_gen_rating_q");
const QString SATIS_REF_SPEC_RATING_Q("satis_ref_spec_rating_q");
const QString SATIS_RATING_A_PREFIX("satis_rating_a");
const QString SATIS_SERVICE_BEING_RATED("satis_service_being_rated");


}  // namespace appstrings
