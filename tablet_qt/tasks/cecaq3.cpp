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

#include "cecaq3.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionwithonefield.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qumcqgriddouble.h"
#include "questionnairelib/qumultipleresponse.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "questionnairelib/quverticalcontainer.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::anyTrue;
using mathfunc::noneNull;
using stringfunc::strnum;

const QString CecaQ3::CECAQ3_TABLENAME("cecaq3");

// Fieldnames:
const QString S1A_MOTHERFIGURE_BIRTHMOTHER("s1a_motherfigure_birthmother");
const QString S1A_MOTHERFIGURE_STEPMOTHER("s1a_motherfigure_stepmother");
const QString S1A_MOTHERFIGURE_FEMALERELATIVE("s1a_motherfigure_femalerelative");
const QString S1A_MOTHERFIGURE_FEMALERELATIVE_DETAIL("s1a_motherfigure_femalerelative_detail");
const QString S1A_MOTHERFIGURE_FAMILYFRIEND("s1a_motherfigure_familyfriend");
const QString S1A_MOTHERFIGURE_FOSTERMOTHER("s1a_motherfigure_fostermother");
const QString S1A_MOTHERFIGURE_ADOPTIVEMOTHER("s1a_motherfigure_adoptivemother");
const QString S1A_MOTHERFIGURE_OTHER("s1a_motherfigure_other");
const QString S1A_MOTHERFIGURE_OTHER_DETAIL("s1a_motherfigure_other_detail");
const QString S1A_FATHERFIGURE_BIRTHFATHER("s1a_fatherfigure_birthfather");
const QString S1A_FATHERFIGURE_STEPFATHER("s1a_fatherfigure_stepfather");
const QString S1A_FATHERFIGURE_MALERELATIVE("s1a_fatherfigure_malerelative");
const QString S1A_FATHERFIGURE_MALERELATIVE_DETAIL("s1a_fatherfigure_malerelative_detail");
const QString S1A_FATHERFIGURE_FAMILYFRIEND("s1a_fatherfigure_familyfriend");
const QString S1A_FATHERFIGURE_FOSTERFATHER("s1a_fatherfigure_fosterfather");
const QString S1A_FATHERFIGURE_ADOPTIVEFATHER("s1a_fatherfigure_adoptivefather");
const QString S1A_FATHERFIGURE_OTHER("s1a_fatherfigure_other");
const QString S1A_FATHERFIGURE_OTHER_DETAIL("s1a_fatherfigure_other_detail");
const QString S1B_INSTITUTION("s1b_institution");
const QString S1B_INSTITUTION_TIME_YEARS("s1b_institution_time_years");
const QString S1C_MOTHER_DIED("s1c_mother_died");
const QString S1C_FATHER_DIED("s1c_father_died");
const QString S1C_MOTHER_DIED_SUBJECT_AGED("s1c_mother_died_subject_aged");
const QString S1C_FATHER_DIED_SUBJECT_AGED("s1c_father_died_subject_aged");
const QString S1C_SEPARATED_FROM_MOTHER("s1c_separated_from_mother");
const QString S1C_SEPARATED_FROM_FATHER("s1c_separated_from_father");
const QString S1C_FIRST_SEPARATED_FROM_MOTHER_AGED("s1c_first_separated_from_mother_aged");
const QString S1C_FIRST_SEPARATED_FROM_FATHER_AGED("s1c_first_separated_from_father_aged");
const QString S1C_MOTHER_HOW_LONG_FIRST_SEPARATION_YEARS("s1c_mother_how_long_first_separation_years");
const QString S1C_FATHER_HOW_LONG_FIRST_SEPARATION_YEARS("s1c_father_how_long_first_separation_years");
const QString S1C_MOTHER_SEPARATION_REASON("s1c_mother_separation_reason");
const QString S1C_FATHER_SEPARATION_REASON("s1c_father_separation_reason");
const QString S1C_DESCRIBE_EXPERIENCE("s1c_describe_experience");
const QString S2A_WHICH_MOTHER_FIGURE("s2a_which_mother_figure");
const QString S2A_WHICH_MOTHER_FIGURE_OTHER_DETAIL("s2a_which_mother_figure_other_detail");
const QString S2A_Q1("s2a_q1");
const QString S2A_Q2("s2a_q2");
const QString S2A_Q3("s2a_q3");
const QString S2A_Q4("s2a_q4");
const QString S2A_Q5("s2a_q5");
const QString S2A_Q6("s2a_q6");
const QString S2A_Q7("s2a_q7");
const QString S2A_Q8("s2a_q8");
const QString S2A_Q9("s2a_q9");
const QString S2A_Q10("s2a_q10");
const QString S2A_Q11("s2a_q11");
const QString S2A_Q12("s2a_q12");
const QString S2A_Q13("s2a_q13");
const QString S2A_Q14("s2a_q14");
const QString S2A_Q15("s2a_q15");
const QString S2A_Q16("s2a_q16");
const QString S2A_EXTRA("s2a_extra");
const QString S2B_Q1("s2b_q1");
const QString S2B_Q2("s2b_q2");
const QString S2B_Q3("s2b_q3");
const QString S2B_Q4("s2b_q4");
const QString S2B_Q5("s2b_q5");
const QString S2B_Q6("s2b_q6");
const QString S2B_Q7("s2b_q7");
const QString S2B_Q8("s2b_q8");
const QString S2B_Q9("s2b_q9");
const QString S2B_Q10("s2b_q10");
const QString S2B_Q11("s2b_q11");
const QString S2B_Q12("s2b_q12");
const QString S2B_Q13("s2b_q13");
const QString S2B_Q14("s2b_q14");
const QString S2B_Q15("s2b_q15");
const QString S2B_Q16("s2b_q16");
const QString S2B_Q17("s2b_q17");
const QString S2B_Q1_FREQUENCY("s2b_q1_frequency");
const QString S2B_Q2_FREQUENCY("s2b_q2_frequency");
const QString S2B_Q3_FREQUENCY("s2b_q3_frequency");
const QString S2B_Q4_FREQUENCY("s2b_q4_frequency");
const QString S2B_Q5_FREQUENCY("s2b_q5_frequency");
const QString S2B_Q6_FREQUENCY("s2b_q6_frequency");
const QString S2B_Q7_FREQUENCY("s2b_q7_frequency");
const QString S2B_Q8_FREQUENCY("s2b_q8_frequency");
const QString S2B_Q9_FREQUENCY("s2b_q9_frequency");
const QString S2B_Q10_FREQUENCY("s2b_q10_frequency");
const QString S2B_Q11_FREQUENCY("s2b_q11_frequency");
const QString S2B_Q12_FREQUENCY("s2b_q12_frequency");
const QString S2B_Q13_FREQUENCY("s2b_q13_frequency");
const QString S2B_Q14_FREQUENCY("s2b_q14_frequency");
const QString S2B_Q15_FREQUENCY("s2b_q15_frequency");
const QString S2B_Q16_FREQUENCY("s2b_q16_frequency");
const QString S2B_Q17_FREQUENCY("s2b_q17_frequency");
const QString S2B_AGE_BEGAN("s2b_age_began");
const QString S2B_EXTRA("s2b_extra");
const QString S3A_WHICH_FATHER_FIGURE("s3a_which_father_figure");
const QString S3A_WHICH_FATHER_FIGURE_OTHER_DETAIL("s3a_which_father_figure_other_detail");
const QString S3A_Q1("s3a_q1");
const QString S3A_Q2("s3a_q2");
const QString S3A_Q3("s3a_q3");
const QString S3A_Q4("s3a_q4");
const QString S3A_Q5("s3a_q5");
const QString S3A_Q6("s3a_q6");
const QString S3A_Q7("s3a_q7");
const QString S3A_Q8("s3a_q8");
const QString S3A_Q9("s3a_q9");
const QString S3A_Q10("s3a_q10");
const QString S3A_Q11("s3a_q11");
const QString S3A_Q12("s3a_q12");
const QString S3A_Q13("s3a_q13");
const QString S3A_Q14("s3a_q14");
const QString S3A_Q15("s3a_q15");
const QString S3A_Q16("s3a_q16");
const QString S3A_EXTRA("s3a_extra");
const QString S3B_Q1("s3b_q1");
const QString S3B_Q2("s3b_q2");
const QString S3B_Q3("s3b_q3");
const QString S3B_Q4("s3b_q4");
const QString S3B_Q5("s3b_q5");
const QString S3B_Q6("s3b_q6");
const QString S3B_Q7("s3b_q7");
const QString S3B_Q8("s3b_q8");
const QString S3B_Q9("s3b_q9");
const QString S3B_Q10("s3b_q10");
const QString S3B_Q11("s3b_q11");
const QString S3B_Q12("s3b_q12");
const QString S3B_Q13("s3b_q13");
const QString S3B_Q14("s3b_q14");
const QString S3B_Q15("s3b_q15");
const QString S3B_Q16("s3b_q16");
const QString S3B_Q17("s3b_q17");
const QString S3B_Q1_FREQUENCY("s3b_q1_frequency");
const QString S3B_Q2_FREQUENCY("s3b_q2_frequency");
const QString S3B_Q3_FREQUENCY("s3b_q3_frequency");
const QString S3B_Q4_FREQUENCY("s3b_q4_frequency");
const QString S3B_Q5_FREQUENCY("s3b_q5_frequency");
const QString S3B_Q6_FREQUENCY("s3b_q6_frequency");
const QString S3B_Q7_FREQUENCY("s3b_q7_frequency");
const QString S3B_Q8_FREQUENCY("s3b_q8_frequency");
const QString S3B_Q9_FREQUENCY("s3b_q9_frequency");
const QString S3B_Q10_FREQUENCY("s3b_q10_frequency");
const QString S3B_Q11_FREQUENCY("s3b_q11_frequency");
const QString S3B_Q12_FREQUENCY("s3b_q12_frequency");
const QString S3B_Q13_FREQUENCY("s3b_q13_frequency");
const QString S3B_Q14_FREQUENCY("s3b_q14_frequency");
const QString S3B_Q15_FREQUENCY("s3b_q15_frequency");
const QString S3B_Q16_FREQUENCY("s3b_q16_frequency");
const QString S3B_Q17_FREQUENCY("s3b_q17_frequency");
const QString S3B_AGE_BEGAN("s3b_age_began");
const QString S3B_EXTRA("s3b_extra");
const QString S3C_Q1("s3c_q1");
const QString S3C_Q2("s3c_q2");
const QString S3C_Q3("s3c_q3");
const QString S3C_Q4("s3c_q4");
const QString S3C_Q5("s3c_q5");
const QString S3C_Q6("s3c_q6");
const QString S3C_Q7("s3c_q7");
const QString S3C_Q8("s3c_q8");
const QString S3C_Q9("s3c_q9");
const QString S3C_Q10("s3c_q10");
const QString S3C_Q11("s3c_q11");
const QString S3C_Q12("s3c_q12");
const QString S3C_Q13("s3c_q13");
const QString S3C_Q14("s3c_q14");
const QString S3C_Q15("s3c_q15");
const QString S3C_Q16("s3c_q16");
const QString S3C_Q17("s3c_q17");
const QString S3C_WHICH_PARENT_CARED_FOR("s3c_which_parent_cared_for");
const QString S3C_PARENT_MENTAL_PROBLEM("s3c_parent_mental_problem");
const QString S3C_PARENT_PHYSICAL_PROBLEM("s3c_parent_physical_problem");
const QString S4A_ADULTCONFIDANT("s4a_adultconfidant");
const QString S4A_ADULTCONFIDANT_MOTHER("s4a_adultconfidant_mother");
const QString S4A_ADULTCONFIDANT_FATHER("s4a_adultconfidant_father");
const QString S4A_ADULTCONFIDANT_OTHERRELATIVE("s4a_adultconfidant_otherrelative");
const QString S4A_ADULTCONFIDANT_FAMILYFRIEND("s4a_adultconfidant_familyfriend");
const QString S4A_ADULTCONFIDANT_RESPONSIBLEADULT("s4a_adultconfidant_responsibleadult");
const QString S4A_ADULTCONFIDANT_OTHER("s4a_adultconfidant_other");
const QString S4A_ADULTCONFIDANT_OTHER_DETAIL("s4a_adultconfidant_other_detail");
const QString S4A_ADULTCONFIDANT_ADDITIONAL("s4a_adultconfidant_additional");
const QString S4B_CHILDCONFIDANT("s4b_childconfidant");
const QString S4B_CHILDCONFIDANT_SISTER("s4b_childconfidant_sister");
const QString S4B_CHILDCONFIDANT_BROTHER("s4b_childconfidant_brother");
const QString S4B_CHILDCONFIDANT_OTHERRELATIVE("s4b_childconfidant_otherrelative");
const QString S4B_CHILDCONFIDANT_CLOSEFRIEND("s4b_childconfidant_closefriend");
const QString S4B_CHILDCONFIDANT_OTHERFRIEND("s4b_childconfidant_otherfriend");
const QString S4B_CHILDCONFIDANT_OTHER("s4b_childconfidant_other");
const QString S4B_CHILDCONFIDANT_OTHER_DETAIL("s4b_childconfidant_other_detail");
const QString S4B_CHILDCONFIDANT_ADDITIONAL("s4b_childconfidant_additional");
const QString S4C_CLOSEST_MOTHER("s4c_closest_mother");
const QString S4C_CLOSEST_FATHER("s4c_closest_father");
const QString S4C_CLOSEST_SIBLING("s4c_closest_sibling");
const QString S4C_CLOSEST_OTHERRELATIVE("s4c_closest_otherrelative");
const QString S4C_CLOSEST_ADULTFRIEND("s4c_closest_adultfriend");
const QString S4C_CLOSEST_CHILDFRIEND("s4c_closest_childfriend");
const QString S4C_CLOSEST_OTHER("s4c_closest_other");
const QString S4C_CLOSEST_OTHER_DETAIL("s4c_closest_other_detail");
const QString S4C_CLOSEST_ADDITIONAL("s4c_closest_additional");
const QString S5C_PHYSICALABUSE("s5c_physicalabuse");
const QString S5C_ABUSED_BY_MOTHER("s5c_abused_by_mother"); // RNC extra
const QString S5C_ABUSED_BY_FATHER("s5c_abused_by_father"); // RNC extra
const QString S5C_MOTHER_ABUSE_AGE_BEGAN("s5c_mother_abuse_age_began");
const QString S5C_FATHER_ABUSE_AGE_BEGAN("s5c_father_abuse_age_began");
const QString S5C_MOTHER_HIT_MORE_THAN_ONCE("s5c_mother_hit_more_than_once");
const QString S5C_FATHER_HIT_MORE_THAN_ONCE("s5c_father_hit_more_than_once");
const QString S5C_MOTHER_HIT_HOW("s5c_mother_hit_how");
const QString S5C_FATHER_HIT_HOW("s5c_father_hit_how");
const QString S5C_MOTHER_INJURED("s5c_mother_injured");
const QString S5C_FATHER_INJURED("s5c_father_injured");
const QString S5C_MOTHER_OUT_OF_CONTROL("s5c_mother_out_of_control");
const QString S5C_FATHER_OUT_OF_CONTROL("s5c_father_out_of_control");
const QString S5C_PARENTAL_ABUSE_DESCRIPTION("s5c_parental_abuse_description");
const QString S5C_ABUSE_BY_NONPARENT("s5c_abuse_by_nonparent");
const QString S5C_NONPARENT_ABUSE_DESCRIPTION("s5c_nonparent_abuse_description");
const QString S6_ANY_UNWANTED_SEXUAL_EXPERIENCE("s6_any_unwanted_sexual_experience");
const QString S6_UNWANTED_INTERCOURSE("s6_unwanted_intercourse");
const QString S6_UPSETTING_SEXUAL_ADULT_AUTHORITY("s6_upsetting_sexual_adult_authority");
const QString S6_FIRST_AGE("s6_first_age");
const QString S6_OTHER_AGE("s6_other_age");
const QString S6_FIRST_PERSON_KNOWN("s6_first_person_known");
const QString S6_OTHER_PERSON_KNOWN("s6_other_person_known");
const QString S6_FIRST_RELATIVE("s6_first_relative");
const QString S6_OTHER_RELATIVE("s6_other_relative");
const QString S6_FIRST_IN_HOUSEHOLD("s6_first_in_household");
const QString S6_OTHER_IN_HOUSEHOLD("s6_other_in_household");
const QString S6_FIRST_MORE_THAN_ONCE("s6_first_more_than_once");
const QString S6_OTHER_MORE_THAN_ONCE("s6_other_more_than_once");
const QString S6_FIRST_TOUCH_PRIVATES_SUBJECT("s6_first_touch_privates_subject");
const QString S6_OTHER_TOUCH_PRIVATES_SUBJECT("s6_other_touch_privates_subject");
const QString S6_FIRST_TOUCH_PRIVATES_OTHER("s6_first_touch_privates_other");
const QString S6_OTHER_TOUCH_PRIVATES_OTHER("s6_other_touch_privates_other");
const QString S6_FIRST_INTERCOURSE("s6_first_intercourse");
const QString S6_OTHER_INTERCOURSE("s6_other_intercourse");
const QString S6_UNWANTED_SEXUAL_DESCRIPTION("s6_unwanted_sexual_description");
const QString ANY_OTHER_COMMENTS("any_other_comments");

// Prefixes
const QString FP_S2A("s2a_q");
const QString FP_S2B("s2b_q");
const QString FP_S3A("s3a_q");
const QString FP_S3B("s3b_q");
// Suffixes
const QString FS_FREQUENCY = "_frequency";

// Task:
const QString TAG_1A_PEOPLE("1a_people");
const QString TAG_4A_CHOSEN("4a_chosen");
const QString TAG_4B_CHOSEN("4b_chosen");
const QString PAGETAG_2B("page_2b");
const QString PAGETAG_3B("page_3b");


void initializeCecaQ3(TaskFactory& factory)
{
    static TaskRegistrar<CecaQ3> registered(factory);
}


CecaQ3::CecaQ3(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CECAQ3_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(S1A_MOTHERFIGURE_BIRTHMOTHER, QVariant::Bool);
    addField(S1A_MOTHERFIGURE_STEPMOTHER, QVariant::Bool);
    addField(S1A_MOTHERFIGURE_FEMALERELATIVE, QVariant::Bool);
    addField(S1A_MOTHERFIGURE_FEMALERELATIVE_DETAIL, QVariant::String);
    addField(S1A_MOTHERFIGURE_FAMILYFRIEND, QVariant::Bool);
    addField(S1A_MOTHERFIGURE_FOSTERMOTHER, QVariant::Bool);
    addField(S1A_MOTHERFIGURE_ADOPTIVEMOTHER, QVariant::Bool);
    addField(S1A_MOTHERFIGURE_OTHER, QVariant::Bool);
    addField(S1A_MOTHERFIGURE_OTHER_DETAIL, QVariant::String);
    addField(S1A_FATHERFIGURE_BIRTHFATHER, QVariant::Bool);
    addField(S1A_FATHERFIGURE_STEPFATHER, QVariant::Bool);
    addField(S1A_FATHERFIGURE_MALERELATIVE, QVariant::Bool);
    addField(S1A_FATHERFIGURE_MALERELATIVE_DETAIL, QVariant::String);
    addField(S1A_FATHERFIGURE_FAMILYFRIEND, QVariant::Bool);
    addField(S1A_FATHERFIGURE_FOSTERFATHER, QVariant::Bool);
    addField(S1A_FATHERFIGURE_ADOPTIVEFATHER, QVariant::Bool);
    addField(S1A_FATHERFIGURE_OTHER, QVariant::Bool);
    addField(S1A_FATHERFIGURE_OTHER_DETAIL, QVariant::String);
    addField(S1B_INSTITUTION, QVariant::Bool);
    addField(S1B_INSTITUTION_TIME_YEARS, QVariant::Double);
    addField(S1C_MOTHER_DIED, QVariant::Bool);
    addField(S1C_FATHER_DIED, QVariant::Bool);
    addField(S1C_MOTHER_DIED_SUBJECT_AGED, QVariant::Double);
    addField(S1C_FATHER_DIED_SUBJECT_AGED, QVariant::Double);
    addField(S1C_SEPARATED_FROM_MOTHER, QVariant::Bool);
    addField(S1C_SEPARATED_FROM_FATHER, QVariant::Bool);
    addField(S1C_FIRST_SEPARATED_FROM_MOTHER_AGED, QVariant::Double);
    addField(S1C_FIRST_SEPARATED_FROM_FATHER_AGED, QVariant::Double);
    addField(S1C_MOTHER_HOW_LONG_FIRST_SEPARATION_YEARS, QVariant::Double);
    addField(S1C_FATHER_HOW_LONG_FIRST_SEPARATION_YEARS, QVariant::Double);
    addField(S1C_MOTHER_SEPARATION_REASON, QVariant::Int);
    addField(S1C_FATHER_SEPARATION_REASON, QVariant::Int);
    addField(S1C_DESCRIBE_EXPERIENCE, QVariant::String);
    addField(S2A_WHICH_MOTHER_FIGURE, QVariant::Int);
    addField(S2A_WHICH_MOTHER_FIGURE_OTHER_DETAIL, QVariant::String);
    addField(S2A_Q1, QVariant::Int);
    addField(S2A_Q2, QVariant::Int);
    addField(S2A_Q3, QVariant::Int);
    addField(S2A_Q4, QVariant::Int);
    addField(S2A_Q5, QVariant::Int);
    addField(S2A_Q6, QVariant::Int);
    addField(S2A_Q7, QVariant::Int);
    addField(S2A_Q8, QVariant::Int);
    addField(S2A_Q9, QVariant::Int);
    addField(S2A_Q10, QVariant::Int);
    addField(S2A_Q11, QVariant::Int);
    addField(S2A_Q12, QVariant::Int);
    addField(S2A_Q13, QVariant::Int);
    addField(S2A_Q14, QVariant::Int);
    addField(S2A_Q15, QVariant::Int);
    addField(S2A_Q16, QVariant::Int);
    addField(S2A_EXTRA, QVariant::String);
    addField(S2B_Q1, QVariant::Int);
    addField(S2B_Q2, QVariant::Int);
    addField(S2B_Q3, QVariant::Int);
    addField(S2B_Q4, QVariant::Int);
    addField(S2B_Q5, QVariant::Int);
    addField(S2B_Q6, QVariant::Int);
    addField(S2B_Q7, QVariant::Int);
    addField(S2B_Q8, QVariant::Int);
    addField(S2B_Q9, QVariant::Int);
    addField(S2B_Q10, QVariant::Int);
    addField(S2B_Q11, QVariant::Int);
    addField(S2B_Q12, QVariant::Int);
    addField(S2B_Q13, QVariant::Int);
    addField(S2B_Q14, QVariant::Int);
    addField(S2B_Q15, QVariant::Int);
    addField(S2B_Q16, QVariant::Int);
    addField(S2B_Q17, QVariant::Int);
    addField(S2B_Q1_FREQUENCY, QVariant::Int);
    addField(S2B_Q2_FREQUENCY, QVariant::Int);
    addField(S2B_Q3_FREQUENCY, QVariant::Int);
    addField(S2B_Q4_FREQUENCY, QVariant::Int);
    addField(S2B_Q5_FREQUENCY, QVariant::Int);
    addField(S2B_Q6_FREQUENCY, QVariant::Int);
    addField(S2B_Q7_FREQUENCY, QVariant::Int);
    addField(S2B_Q8_FREQUENCY, QVariant::Int);
    addField(S2B_Q9_FREQUENCY, QVariant::Int);
    addField(S2B_Q10_FREQUENCY, QVariant::Int);
    addField(S2B_Q11_FREQUENCY, QVariant::Int);
    addField(S2B_Q12_FREQUENCY, QVariant::Int);
    addField(S2B_Q13_FREQUENCY, QVariant::Int);
    addField(S2B_Q14_FREQUENCY, QVariant::Int);
    addField(S2B_Q15_FREQUENCY, QVariant::Int);
    addField(S2B_Q16_FREQUENCY, QVariant::Int);
    addField(S2B_Q17_FREQUENCY, QVariant::Int);
    addField(S2B_AGE_BEGAN, QVariant::Double);
    addField(S2B_EXTRA, QVariant::String);
    addField(S3A_WHICH_FATHER_FIGURE, QVariant::Int);
    addField(S3A_WHICH_FATHER_FIGURE_OTHER_DETAIL, QVariant::String);
    addField(S3A_Q1, QVariant::Int);
    addField(S3A_Q2, QVariant::Int);
    addField(S3A_Q3, QVariant::Int);
    addField(S3A_Q4, QVariant::Int);
    addField(S3A_Q5, QVariant::Int);
    addField(S3A_Q6, QVariant::Int);
    addField(S3A_Q7, QVariant::Int);
    addField(S3A_Q8, QVariant::Int);
    addField(S3A_Q9, QVariant::Int);
    addField(S3A_Q10, QVariant::Int);
    addField(S3A_Q11, QVariant::Int);
    addField(S3A_Q12, QVariant::Int);
    addField(S3A_Q13, QVariant::Int);
    addField(S3A_Q14, QVariant::Int);
    addField(S3A_Q15, QVariant::Int);
    addField(S3A_Q16, QVariant::Int);
    addField(S3A_EXTRA, QVariant::String);
    addField(S3B_Q1, QVariant::Int);
    addField(S3B_Q2, QVariant::Int);
    addField(S3B_Q3, QVariant::Int);
    addField(S3B_Q4, QVariant::Int);
    addField(S3B_Q5, QVariant::Int);
    addField(S3B_Q6, QVariant::Int);
    addField(S3B_Q7, QVariant::Int);
    addField(S3B_Q8, QVariant::Int);
    addField(S3B_Q9, QVariant::Int);
    addField(S3B_Q10, QVariant::Int);
    addField(S3B_Q11, QVariant::Int);
    addField(S3B_Q12, QVariant::Int);
    addField(S3B_Q13, QVariant::Int);
    addField(S3B_Q14, QVariant::Int);
    addField(S3B_Q15, QVariant::Int);
    addField(S3B_Q16, QVariant::Int);
    addField(S3B_Q17, QVariant::Int);
    addField(S3B_Q1_FREQUENCY, QVariant::Int);
    addField(S3B_Q2_FREQUENCY, QVariant::Int);
    addField(S3B_Q3_FREQUENCY, QVariant::Int);
    addField(S3B_Q4_FREQUENCY, QVariant::Int);
    addField(S3B_Q5_FREQUENCY, QVariant::Int);
    addField(S3B_Q6_FREQUENCY, QVariant::Int);
    addField(S3B_Q7_FREQUENCY, QVariant::Int);
    addField(S3B_Q8_FREQUENCY, QVariant::Int);
    addField(S3B_Q9_FREQUENCY, QVariant::Int);
    addField(S3B_Q10_FREQUENCY, QVariant::Int);
    addField(S3B_Q11_FREQUENCY, QVariant::Int);
    addField(S3B_Q12_FREQUENCY, QVariant::Int);
    addField(S3B_Q13_FREQUENCY, QVariant::Int);
    addField(S3B_Q14_FREQUENCY, QVariant::Int);
    addField(S3B_Q15_FREQUENCY, QVariant::Int);
    addField(S3B_Q16_FREQUENCY, QVariant::Int);
    addField(S3B_Q17_FREQUENCY, QVariant::Int);
    addField(S3B_AGE_BEGAN, QVariant::Double);
    addField(S3B_EXTRA, QVariant::String);
    addField(S3C_Q1, QVariant::Int);
    addField(S3C_Q2, QVariant::Int);
    addField(S3C_Q3, QVariant::Int);
    addField(S3C_Q4, QVariant::Int);
    addField(S3C_Q5, QVariant::Int);
    addField(S3C_Q6, QVariant::Int);
    addField(S3C_Q7, QVariant::Int);
    addField(S3C_Q8, QVariant::Int);
    addField(S3C_Q9, QVariant::Int);
    addField(S3C_Q10, QVariant::Int);
    addField(S3C_Q11, QVariant::Int);
    addField(S3C_Q12, QVariant::Int);
    addField(S3C_Q13, QVariant::Int);
    addField(S3C_Q14, QVariant::Int);
    addField(S3C_Q15, QVariant::Int);
    addField(S3C_Q16, QVariant::Int);
    addField(S3C_Q17, QVariant::Int);
    addField(S3C_WHICH_PARENT_CARED_FOR, QVariant::Int);
    addField(S3C_PARENT_MENTAL_PROBLEM, QVariant::Int);
    addField(S3C_PARENT_PHYSICAL_PROBLEM, QVariant::Int);
    addField(S4A_ADULTCONFIDANT, QVariant::Bool);
    addField(S4A_ADULTCONFIDANT_MOTHER, QVariant::Bool);
    addField(S4A_ADULTCONFIDANT_FATHER, QVariant::Bool);
    addField(S4A_ADULTCONFIDANT_OTHERRELATIVE, QVariant::Bool);
    addField(S4A_ADULTCONFIDANT_FAMILYFRIEND, QVariant::Bool);
    addField(S4A_ADULTCONFIDANT_RESPONSIBLEADULT, QVariant::Bool);
    addField(S4A_ADULTCONFIDANT_OTHER, QVariant::Bool);
    addField(S4A_ADULTCONFIDANT_OTHER_DETAIL, QVariant::String);
    addField(S4A_ADULTCONFIDANT_ADDITIONAL, QVariant::String);
    addField(S4B_CHILDCONFIDANT, QVariant::Bool);
    addField(S4B_CHILDCONFIDANT_SISTER, QVariant::Bool);
    addField(S4B_CHILDCONFIDANT_BROTHER, QVariant::Bool);
    addField(S4B_CHILDCONFIDANT_OTHERRELATIVE, QVariant::Bool);
    addField(S4B_CHILDCONFIDANT_CLOSEFRIEND, QVariant::Bool);
    addField(S4B_CHILDCONFIDANT_OTHERFRIEND, QVariant::Bool);
    addField(S4B_CHILDCONFIDANT_OTHER, QVariant::Bool);
    addField(S4B_CHILDCONFIDANT_OTHER_DETAIL, QVariant::String);
    addField(S4B_CHILDCONFIDANT_ADDITIONAL, QVariant::String);
    addField(S4C_CLOSEST_MOTHER, QVariant::Bool);
    addField(S4C_CLOSEST_FATHER, QVariant::Bool);
    addField(S4C_CLOSEST_SIBLING, QVariant::Bool);
    addField(S4C_CLOSEST_OTHERRELATIVE, QVariant::Bool);
    addField(S4C_CLOSEST_ADULTFRIEND, QVariant::Bool);
    addField(S4C_CLOSEST_CHILDFRIEND, QVariant::Bool);
    addField(S4C_CLOSEST_OTHER, QVariant::Bool);
    addField(S4C_CLOSEST_OTHER_DETAIL, QVariant::String);
    addField(S4C_CLOSEST_ADDITIONAL, QVariant::String);
    addField(S5C_PHYSICALABUSE, QVariant::Bool);
    addField(S5C_ABUSED_BY_MOTHER, QVariant::Bool); // RNC extra
    addField(S5C_ABUSED_BY_FATHER, QVariant::Bool); // RNC extra
    addField(S5C_MOTHER_ABUSE_AGE_BEGAN, QVariant::Double);
    addField(S5C_FATHER_ABUSE_AGE_BEGAN, QVariant::Double);
    addField(S5C_MOTHER_HIT_MORE_THAN_ONCE, QVariant::Bool);
    addField(S5C_FATHER_HIT_MORE_THAN_ONCE, QVariant::Bool);
    addField(S5C_MOTHER_HIT_HOW, QVariant::Int);
    addField(S5C_FATHER_HIT_HOW, QVariant::Int);
    addField(S5C_MOTHER_INJURED, QVariant::Bool);
    addField(S5C_FATHER_INJURED, QVariant::Bool);
    addField(S5C_MOTHER_OUT_OF_CONTROL, QVariant::Bool);
    addField(S5C_FATHER_OUT_OF_CONTROL, QVariant::Bool);
    addField(S5C_PARENTAL_ABUSE_DESCRIPTION, QVariant::String);
    addField(S5C_ABUSE_BY_NONPARENT, QVariant::Bool);
    addField(S5C_NONPARENT_ABUSE_DESCRIPTION, QVariant::String);
    addField(S6_ANY_UNWANTED_SEXUAL_EXPERIENCE, QVariant::Int);  // not bool
    addField(S6_UNWANTED_INTERCOURSE, QVariant::Int);  // not bool
    addField(S6_UPSETTING_SEXUAL_ADULT_AUTHORITY, QVariant::Int);  // not bool
    addField(S6_FIRST_AGE, QVariant::Double);
    addField(S6_OTHER_AGE, QVariant::Double);
    addField(S6_FIRST_PERSON_KNOWN, QVariant::Bool);
    addField(S6_OTHER_PERSON_KNOWN, QVariant::Bool);
    addField(S6_FIRST_RELATIVE, QVariant::Bool);
    addField(S6_OTHER_RELATIVE, QVariant::Bool);
    addField(S6_FIRST_IN_HOUSEHOLD, QVariant::Bool);
    addField(S6_OTHER_IN_HOUSEHOLD, QVariant::Bool);
    addField(S6_FIRST_MORE_THAN_ONCE, QVariant::Bool);
    addField(S6_OTHER_MORE_THAN_ONCE, QVariant::Bool);
    addField(S6_FIRST_TOUCH_PRIVATES_SUBJECT, QVariant::Bool);
    addField(S6_OTHER_TOUCH_PRIVATES_SUBJECT, QVariant::Bool);
    addField(S6_FIRST_TOUCH_PRIVATES_OTHER, QVariant::Bool);
    addField(S6_OTHER_TOUCH_PRIVATES_OTHER, QVariant::Bool);
    addField(S6_FIRST_INTERCOURSE, QVariant::Bool);
    addField(S6_OTHER_INTERCOURSE, QVariant::Bool);
    addField(S6_UNWANTED_SEXUAL_DESCRIPTION, QVariant::String);
    addField(ANY_OTHER_COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CecaQ3::shortname() const
{
    return "CECA-Q3";
}


QString CecaQ3::longname() const
{
    return tr("Childhood Experience of Care and Abuse Questionnaire, v3");
}


QString CecaQ3::description() const
{
    return tr("Family relationships in childhood. Version 3 with "
              "psychological abuse and role reversal.");
}


// ============================================================================
// Instance info
// ============================================================================

bool CecaQ3::isComplete() const
{
    return complete1A() &&
            complete1B() &&
            complete1C() &&
            complete2A() &&
            complete2B() &&
            complete3A() &&
            complete3B() &&
            complete3C() &&
            complete4A() &&
            complete4B() &&
            complete4C() &&
            complete5() &&
            complete6();
}


QStringList CecaQ3::summary() const
{
    return QStringList{TextConst::noSummarySeeFacsimile()};
}


QStringList CecaQ3::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* CecaQ3::editor(const bool read_only)
{
    using CallbackFn = void (CecaQ3::*)();
    const NameValueOptions options_yesno = CommonOptions::noYesBoolean();
    const NameValueOptions options_separation{
        {xstring("1c_separation_reason1"), 1},
        {xstring("1c_separation_reason2"), 2},
        {xstring("1c_separation_reason3"), 3},
        {xstring("1c_separation_reason4"), 4},
        {xstring("1c_separation_reason5"), 5},
        {xstring("1c_separation_reason6"), 6},
    };
    const NameValueOptions options_2a_whichmotherfigure{
        {xstring("2a_which_option1"), 1},
        {xstring("2a_which_option2"), 2},
        {xstring("2a_which_option3"), 3},
        {xstring("2a_which_option4"), 4},
        {xstring("2a_which_option5"), 5},
        {xstring("2a_which_option0"), 0},
    };
    const NameValueOptions options_3a_whichfatherfigure{
        {xstring("3a_which_option1"), 1},
        {xstring("3a_which_option2"), 2},
        {xstring("3a_which_option3"), 3},
        {xstring("3a_which_option4"), 4},
        {xstring("3a_which_option5"), 5},
        {xstring("3a_which_option0"), 0},
    };
    const NameValueOptions options_5way_no_to_yes{  // presented in yes-to-no order
        {xstring("options5way_notoyes_5"), 5},
        {"", 4},
        {xstring("options5way_notoyes_3"), 3},
        {"", 2},
        {xstring("options5way_notoyes_1"), 1},
    };
    const NameValueOptions options3way_no_to_yes{  // presented in yes-to-no order
        {xstring("options3way_noto_yes_2"), 2},
        {xstring("options3way_noto_yes_1"), 1},
        {xstring("options3way_noto_yes_0"), 0},
    };
    const NameValueOptions optionsfrequency{
        {xstring("optionsfrequency0"), 0},
        {xstring("optionsfrequency1"), 1},
        {xstring("optionsfrequency2"), 2},
        {xstring("optionsfrequency3"), 3},
    };
    const NameValueOptions options_whichparentcaredfor{
        {xstring("3c_whichparentcaredfor_option1"), 1},
        {xstring("3c_whichparentcaredfor_option2"), 2},
        {xstring("3c_whichparentcaredfor_option3"), 3},
        {xstring("3c_whichparentcaredfor_option4"), 4},
        {xstring("3c_whichparentcaredfor_option0"), 0},
    };
    const NameValueOptions options_hit{
        {xstring("5_hit_option_1"), 1},
        {xstring("5_hit_option_2"), 2},
        {xstring("5_hit_option_3"), 3},
        {xstring("5_hit_option_4"), 4},
    };
    const QVector<McqGridSubtitle> generic_subtitles{
        McqGridSubtitle(5, ""),
        McqGridSubtitle(10, ""),
        McqGridSubtitle(15, ""),
        McqGridSubtitle(20, ""),
        McqGridSubtitle(25, ""),
    };
    QVector<QuPagePtr> pages;
    const QString asterisk = " (*)";

    auto connectedfr = [this](CallbackFn callback,
                              const QString& fieldname,
                              bool mandatory = false) -> FieldRefPtr {
        FieldRefPtr fr = fieldRef(fieldname, mandatory);
        connect(fr.data(), &FieldRef::valueChanged,
                this, callback);
        return fr;
    };
    auto text = [this](const QString& stringname) -> QuElement* {
        return new QuText(xstring(stringname));
    };
    auto boldtext = [this](const QString& stringname) -> QuElement* {
        return (new QuText(xstring(stringname)))->setBold(true);
    };
    auto headingRaw = [](const QString& text) -> QuElement* {
        return new QuHeading(text);
    };
    auto heading = [this](const QString& stringname) -> QuElement* {
        return new QuHeading(xstring(stringname));
    };
    auto q1f = [this, &connectedfr]
            (CallbackFn callback,
            const QString& fieldname,
            const QString& xstringname,
            const QString& suffix = "",
            bool mandatory = false) -> QuestionWithOneField {
        return QuestionWithOneField(
                    connectedfr(callback, fieldname, mandatory),
                    xstring(xstringname) + suffix);
    };
    auto q2f = [this, &connectedfr]
            (CallbackFn callback,
            const QString& fieldname1,
            const QString& fieldname2,
            const QString& xstringname,
            const QString& suffix = "",
            bool mandatory = false) -> QuestionWithTwoFields {
        return QuestionWithTwoFields(
                    xstring(xstringname) + suffix,
                    connectedfr(callback, fieldname1, mandatory),
                    connectedfr(callback, fieldname2, mandatory));
    };
    auto textedit = [this, &connectedfr]
            (CallbackFn callback,
            const QString& fieldname,
            const QString& hint = "",
            bool mandatory = false) -> QuElement* {
        QuTextEdit* e = new QuTextEdit(
                    connectedfr(callback, fieldname, mandatory));
        if (hint.isEmpty()) {
            e->setHint("");
        } else {
            e->setHint(xstring(hint));
        }
        return e;
    };
    auto realedit = [this, &connectedfr]
            (CallbackFn callback,
            const QString& fieldname,
            const QString& hint = "",
            bool mandatory = false) -> QuElement* {
        QuLineEditDouble* e = new QuLineEditDouble(
                    connectedfr(callback, fieldname, mandatory));
        if (!hint.isEmpty()) {
            e->setHint(xstring(hint));
        }
        return e;
    };
    auto yn = [&connectedfr, &options_yesno]
            (CallbackFn callback, const QString& fieldname,
             bool mandatory = false) -> QuElement* {
        return (new QuMcq(connectedfr(callback, fieldname, mandatory),
                          options_yesno))->setHorizontal(true);
    };
    auto horizline = []() -> QuElement* {
        return new QuHorizontalLine();
    };
    auto mcq = [&connectedfr]
            (CallbackFn callback,
            const QString& fieldname,
            const NameValueOptions& options,
            bool mandatory = false) -> QuElement* {
        return new QuMcq(connectedfr(callback, fieldname, mandatory), options);
    };
    auto pagetitle = [](const QString& title) -> QString {
        if (title.isEmpty()) {
            return "CECA-Q3";
        }
        return QString("CECA-Q3 (%1)").arg(title);
    };

    // ------------------------------------------------------------------------
    // Shorthand for callback functions
    // ------------------------------------------------------------------------
    const CallbackFn cb1a = &CecaQ3::dataChanged1A;
    const CallbackFn cb1b = &CecaQ3::dataChanged1B;
    const CallbackFn cb1c = &CecaQ3::dataChanged1C;
    const CallbackFn cb2a = &CecaQ3::dataChanged2A;
    const CallbackFn cb2b = &CecaQ3::dataChanged2B;
    const CallbackFn cb3a = &CecaQ3::dataChanged3A;
    const CallbackFn cb3b = &CecaQ3::dataChanged3B;
    const CallbackFn cb3c = &CecaQ3::dataChanged3C;
    const CallbackFn cb4a = &CecaQ3::dataChanged4A;
    const CallbackFn cb4b = &CecaQ3::dataChanged4B;
    const CallbackFn cb4c = &CecaQ3::dataChanged4C;
    const CallbackFn cb5 = &CecaQ3::dataChanged5;
    const CallbackFn cb6 = &CecaQ3::dataChanged6;
    const CallbackFn cbdummy = &CecaQ3::dataChangedDummy;

    // ------------------------------------------------------------------------
    // Preamble
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("title"),
        text("instruction1"),
        text("instruction2"),
    })->setTitle(pagetitle(""))));

    // ------------------------------------------------------------------------
    // 1A
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("1a_q"),
        text("1a_instruction"),
        new QuGridContainer(2, {
            new QuVerticalContainer{
                boldtext("1a_motherfigures"),
                (new QuMultipleResponse{
                    q1f(cb1a, S1A_MOTHERFIGURE_BIRTHMOTHER, "1a_mf_birthmother"),
                    q1f(cb1a, S1A_MOTHERFIGURE_STEPMOTHER, "1a_mf_stepmother"),
                    q1f(cb1a, S1A_MOTHERFIGURE_FEMALERELATIVE, "1a_mf_femalerelative", asterisk),
                    q1f(cb1a, S1A_MOTHERFIGURE_FAMILYFRIEND, "1a_mf_familyfriend"),
                    q1f(cb1a, S1A_MOTHERFIGURE_FOSTERMOTHER, "1a_mf_fostermother"),
                    q1f(cb1a, S1A_MOTHERFIGURE_ADOPTIVEMOTHER, "1a_mf_adoptivemother"),
                    q1f(cb1a, S1A_MOTHERFIGURE_OTHER, "other", asterisk),
                })->setMinimumAnswers(1)->addTag(TAG_1A_PEOPLE),
                text("rnc_1a_femalerelative_or_other"),
                text("1a_mf_femalerelative"),
                textedit(cb1a, S1A_MOTHERFIGURE_FEMALERELATIVE_DETAIL),
                text("other"),
                textedit(cb1a, S1A_MOTHERFIGURE_OTHER_DETAIL),
            },
            new QuVerticalContainer{
                boldtext("1a_fatherfigures"),
                (new QuMultipleResponse{
                    q1f(cb1a, S1A_FATHERFIGURE_BIRTHFATHER, "1a_ff_birthfather"),
                    q1f(cb1a, S1A_FATHERFIGURE_STEPFATHER, "1a_ff_stepfather"),
                    q1f(cb1a, S1A_FATHERFIGURE_MALERELATIVE, "1a_ff_malerelative", asterisk),
                    q1f(cb1a, S1A_FATHERFIGURE_FAMILYFRIEND, "1a_ff_familyfriend"),
                    q1f(cb1a, S1A_FATHERFIGURE_FOSTERFATHER, "1a_ff_fosterfather"),
                    q1f(cb1a, S1A_FATHERFIGURE_ADOPTIVEFATHER, "1a_ff_adoptivefather"),
                    q1f(cb1a, S1A_FATHERFIGURE_OTHER, "other", asterisk),
                })->setMinimumAnswers(1)->addTag(TAG_1A_PEOPLE),
                text("rnc_1a_malerelative_or_other"),
                text("1a_ff_malerelative"),
                textedit(cb1a, S1A_FATHERFIGURE_MALERELATIVE_DETAIL),
                text("other"),
                textedit(cb1a, S1A_FATHERFIGURE_OTHER_DETAIL),
            },
        }),
    })->setTitle(pagetitle("1A"))));

    // ------------------------------------------------------------------------
    // 1B
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("1b_q"),
        yn(cb1b, S1B_INSTITUTION, true),
        boldtext("1b_q_how_long"),
        realedit(cb1b, S1B_INSTITUTION_TIME_YEARS, "1b_how_long_prompt"),
    })->setTitle(pagetitle("1B"))));

    // ------------------------------------------------------------------------
    // 1C
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("1c_heading"),
        new QuGridContainer(3, {
            new QuSpacer(),
            boldtext("mother"),
            boldtext("father"),
            // ---
            boldtext("1c_parentdied"),
            yn(cb1c, S1C_MOTHER_DIED, true),
            yn(cb1c, S1C_FATHER_DIED, true),
            // ---
            text("1c_parentdiedage"),
            realedit(cb1c, S1C_MOTHER_DIED_SUBJECT_AGED, "age_years"),
            realedit(cb1c, S1C_FATHER_DIED_SUBJECT_AGED, "age_years"),
            // ---
            horizline(),
            horizline(),
            horizline(),
            // ---
            boldtext("1c_separated"),
            yn(cb1c, S1C_SEPARATED_FROM_MOTHER, true),
            yn(cb1c, S1C_SEPARATED_FROM_FATHER, true),
            // ---
            text("1c_if_separated"),
            boldtext("mother"),
            boldtext("father"),
            // ---
            text("1c_age_first_separated"),
            realedit(cb1c, S1C_FIRST_SEPARATED_FROM_MOTHER_AGED, "age_years"),
            realedit(cb1c, S1C_FIRST_SEPARATED_FROM_FATHER_AGED, "age_years"),
            // ---
            text("1c_how_long_separation"),
            realedit(cb1c, S1C_MOTHER_HOW_LONG_FIRST_SEPARATION_YEARS, "1c_years"),
            realedit(cb1c, S1C_FATHER_HOW_LONG_FIRST_SEPARATION_YEARS, "1c_years"),
            // ---
            text("1c_separation_reason"),
            mcq(cb1c, S1C_MOTHER_SEPARATION_REASON, options_separation),
            mcq(cb1c, S1C_FATHER_SEPARATION_REASON, options_separation),
        }),
        text("please_describe_experience"),
        textedit(cb1c, S1C_DESCRIBE_EXPERIENCE),
    })->setTitle(pagetitle("1C"))));

    // ------------------------------------------------------------------------
    // 2A
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("2a_heading"),
        text("2a_instruction"),
        boldtext("2a_which"),
        mcq(cb2a, S2A_WHICH_MOTHER_FIGURE, options_2a_whichmotherfigure, true),
        text("rnc_if_other_describe"),
        textedit(cb2a, S2A_WHICH_MOTHER_FIGURE_OTHER_DETAIL, "hint_description"),
        (new QuMcqGrid(QVector<QuestionWithOneField>{
            q1f(cb2a, S2A_Q1, "2a_q1"),
            q1f(cb2a, S2A_Q2, "2a_q2"),
            q1f(cb2a, S2A_Q3, "2a_q3"),
            q1f(cb2a, S2A_Q4, "2a_q4"),
            q1f(cb2a, S2A_Q5, "2a_q5"),
            q1f(cb2a, S2A_Q6, "2a_q6"),
            q1f(cb2a, S2A_Q7, "2a_q7"),
            q1f(cb2a, S2A_Q8, "2a_q8"),
            q1f(cb2a, S2A_Q9, "2a_q9"),
            q1f(cb2a, S2A_Q10, "2a_q10"),
            q1f(cb2a, S2A_Q11, "2a_q11"),
            q1f(cb2a, S2A_Q12, "2a_q12"),
            q1f(cb2a, S2A_Q13, "2a_q13"),
            q1f(cb2a, S2A_Q14, "2a_q14"),
            q1f(cb2a, S2A_Q15, "2a_q15"),
            q1f(cb2a, S2A_Q16, "2a_q16"),
        }, options_5way_no_to_yes))->setSubtitles(generic_subtitles),
        text("2a_add_anything"),
        textedit(cb2a, S2A_EXTRA),
    })->setTitle(pagetitle("2A"))));

    // ------------------------------------------------------------------------
    // 2B
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("2b_heading"),
        text("2b_instruction"),
        (new QuMcqGridDouble(
            QVector<QuestionWithTwoFields>{
                q2f(cb2b, S2B_Q1, S2B_Q1_FREQUENCY, "2b_q1"),
                q2f(cb2b, S2B_Q2, S2B_Q2_FREQUENCY, "2b_q2"),
                q2f(cb2b, S2B_Q3, S2B_Q3_FREQUENCY, "2b_q3"),
                q2f(cb2b, S2B_Q4, S2B_Q4_FREQUENCY, "2b_q4"),
                q2f(cb2b, S2B_Q5, S2B_Q5_FREQUENCY, "2b_q5"),
                q2f(cb2b, S2B_Q6, S2B_Q6_FREQUENCY, "2b_q6"),
                q2f(cb2b, S2B_Q7, S2B_Q7_FREQUENCY, "2b_q7"),
                q2f(cb2b, S2B_Q8, S2B_Q8_FREQUENCY, "2b_q8"),
                q2f(cb2b, S2B_Q9, S2B_Q9_FREQUENCY, "2b_q9"),
                q2f(cb2b, S2B_Q10, S2B_Q10_FREQUENCY, "2b_q10"),
                q2f(cb2b, S2B_Q11, S2B_Q11_FREQUENCY, "2b_q11"),
                q2f(cb2b, S2B_Q12, S2B_Q12_FREQUENCY, "2b_q12"),
                q2f(cb2b, S2B_Q13, S2B_Q13_FREQUENCY, "2b_q13"),
                q2f(cb2b, S2B_Q14, S2B_Q14_FREQUENCY, "2b_q14"),
                q2f(cb2b, S2B_Q15, S2B_Q15_FREQUENCY, "2b_q15"),
                q2f(cb2b, S2B_Q16, S2B_Q16_FREQUENCY, "2b_q16"),
                q2f(cb2b, S2B_Q17, S2B_Q17_FREQUENCY, "2b_q17"),
            },
            options3way_no_to_yes,
            optionsfrequency
        ))
            ->setStems("", xstring("how_frequent"))
            ->setSubtitles(generic_subtitles),
        text("if_any_what_age"),
        realedit(cb2b, S2B_AGE_BEGAN, "age_years"),
        text("is_there_more_you_want_to_say"),
        textedit(cb2b, S2B_EXTRA),
    })->setTitle(pagetitle("2B"))->addTag(PAGETAG_2B)));

    // ------------------------------------------------------------------------
    // 3A
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("3a_heading"),
        text("3a_instruction"),
        boldtext("3a_which"),
        mcq(cb3a, S3A_WHICH_FATHER_FIGURE, options_3a_whichfatherfigure, true),
        text("rnc_if_other_describe"),
        textedit(cb3a, S3A_WHICH_FATHER_FIGURE_OTHER_DETAIL, "hint_description"),
        (new QuMcqGrid(QVector<QuestionWithOneField>{
            q1f(cb3a, S3A_Q1, "3a_q1"),
            q1f(cb3a, S3A_Q2, "3a_q2"),
            q1f(cb3a, S3A_Q3, "3a_q3"),
            q1f(cb3a, S3A_Q4, "3a_q4"),
            q1f(cb3a, S3A_Q5, "3a_q5"),
            q1f(cb3a, S3A_Q6, "3a_q6"),
            q1f(cb3a, S3A_Q7, "3a_q7"),
            q1f(cb3a, S3A_Q8, "3a_q8"),
            q1f(cb3a, S3A_Q9, "3a_q9"),
            q1f(cb3a, S3A_Q10, "3a_q10"),
            q1f(cb3a, S3A_Q11, "3a_q11"),
            q1f(cb3a, S3A_Q12, "3a_q12"),
            q1f(cb3a, S3A_Q13, "3a_q13"),
            q1f(cb3a, S3A_Q14, "3a_q14"),
            q1f(cb3a, S3A_Q15, "3a_q15"),
            q1f(cb3a, S3A_Q16, "3a_q16"),
        }, options_5way_no_to_yes))->setSubtitles(generic_subtitles),
        text("3a_add_anything"),
        textedit(cb3a, S3A_EXTRA),
    })->setTitle(pagetitle("3A"))));

    // ------------------------------------------------------------------------
    // 3B
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("3b_heading"),
        text("3b_instruction"),
        (new QuMcqGridDouble(
            QVector<QuestionWithTwoFields>{
                q2f(cb3b, S3B_Q1, S3B_Q1_FREQUENCY, "3b_q1"),
                q2f(cb3b, S3B_Q2, S3B_Q2_FREQUENCY, "3b_q2"),
                q2f(cb3b, S3B_Q3, S3B_Q3_FREQUENCY, "3b_q3"),
                q2f(cb3b, S3B_Q4, S3B_Q4_FREQUENCY, "3b_q4"),
                q2f(cb3b, S3B_Q5, S3B_Q5_FREQUENCY, "3b_q5"),
                q2f(cb3b, S3B_Q6, S3B_Q6_FREQUENCY, "3b_q6"),
                q2f(cb3b, S3B_Q7, S3B_Q7_FREQUENCY, "3b_q7"),
                q2f(cb3b, S3B_Q8, S3B_Q8_FREQUENCY, "3b_q8"),
                q2f(cb3b, S3B_Q9, S3B_Q9_FREQUENCY, "3b_q9"),
                q2f(cb3b, S3B_Q10, S3B_Q10_FREQUENCY, "3b_q10"),
                q2f(cb3b, S3B_Q11, S3B_Q11_FREQUENCY, "3b_q11"),
                q2f(cb3b, S3B_Q12, S3B_Q12_FREQUENCY, "3b_q12"),
                q2f(cb3b, S3B_Q13, S3B_Q13_FREQUENCY, "3b_q13"),
                q2f(cb3b, S3B_Q14, S3B_Q14_FREQUENCY, "3b_q14"),
                q2f(cb3b, S3B_Q15, S3B_Q15_FREQUENCY, "3b_q15"),
                q2f(cb3b, S3B_Q16, S3B_Q16_FREQUENCY, "3b_q16"),
                q2f(cb3b, S3B_Q17, S3B_Q17_FREQUENCY, "3b_q17"),
            },
            options3way_no_to_yes,
            optionsfrequency
        ))
            ->setStems("", xstring("how_frequent"))
            ->setSubtitles(generic_subtitles),
        text("if_any_what_age"),
        realedit(cb3b, S3B_AGE_BEGAN, "age_years"),
        text("is_there_more_you_want_to_say"),
        textedit(cb3b, S3B_EXTRA),
    })->setTitle(pagetitle("3B"))->addTag(PAGETAG_3B)));

    // ------------------------------------------------------------------------
    // 3C
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("3c_heading"),
        (new QuMcqGrid(QVector<QuestionWithOneField>{
            q1f(cb3c, S3C_Q1, "3c_q1", "", true),
            q1f(cb3c, S3C_Q2, "3c_q2", "", true),
            q1f(cb3c, S3C_Q3, "3c_q3", "", true),
            q1f(cb3c, S3C_Q4, "3c_q4", "", true),
            q1f(cb3c, S3C_Q5, "3c_q5", "", true),
            q1f(cb3c, S3C_Q6, "3c_q6", "", true),
            q1f(cb3c, S3C_Q7, "3c_q7", "", true),
            q1f(cb3c, S3C_Q8, "3c_q8", "", true),
            q1f(cb3c, S3C_Q9, "3c_q9", "", true),
            q1f(cb3c, S3C_Q10, "3c_q10", "", true),
            q1f(cb3c, S3C_Q11, "3c_q11", "", true),
            q1f(cb3c, S3C_Q12, "3c_q12", "", true),
            q1f(cb3c, S3C_Q13, "3c_q13", "", true),
            q1f(cb3c, S3C_Q14, "3c_q14", "", true),
            q1f(cb3c, S3C_Q15, "3c_q15", "", true),
            q1f(cb3c, S3C_Q16, "3c_q16", "", true),
            q1f(cb3c, S3C_Q17, "3c_q17", "", true),
        }, options_5way_no_to_yes))->setSubtitles(generic_subtitles),
        boldtext("3c_which_parent_cared_for"),
        mcq(cb3c, S3C_WHICH_PARENT_CARED_FOR, options_whichparentcaredfor, true),
        boldtext("3c_parent_mental_problem"),
        mcq(cb3c, S3C_PARENT_MENTAL_PROBLEM, options3way_no_to_yes, true),
        boldtext("3c_parent_physical_problem"),
        mcq(cb3c, S3C_PARENT_PHYSICAL_PROBLEM, options3way_no_to_yes, true),
    })->setTitle(pagetitle("3C"))));

    // ------------------------------------------------------------------------
    // 4A
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("4_heading"),
        boldtext("4a_q"),
        mcq(cb4a, S4A_ADULTCONFIDANT, options_yesno, true),
        boldtext("4_if_so_who"),
        (new QuMultipleResponse{
            q1f(cb4a, S4A_ADULTCONFIDANT_MOTHER, "4a_option_mother"),
            q1f(cb4a, S4A_ADULTCONFIDANT_FATHER, "4a_option_father"),
            q1f(cb4a, S4A_ADULTCONFIDANT_OTHERRELATIVE, "4a_option_relative"),
            q1f(cb4a, S4A_ADULTCONFIDANT_FAMILYFRIEND, "4a_option_friend"),
            q1f(cb4a, S4A_ADULTCONFIDANT_RESPONSIBLEADULT, "4a_option_responsibleadult"),
            q1f(cb4a, S4A_ADULTCONFIDANT_OTHER, "4a_option_other"),
        })->setMinimumAnswers(1)->addTag(TAG_4A_CHOSEN),
        text("rnc_if_other_describe"),
        textedit(cb4a, S4A_ADULTCONFIDANT_OTHER_DETAIL),
        text("4_note_anything"),
        textedit(cb4a, S4A_ADULTCONFIDANT_ADDITIONAL),
    })->setTitle(pagetitle("4A"))));

    // ------------------------------------------------------------------------
    // 4B
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("4_heading"),
        boldtext("4b_q"),
        mcq(cb4b, S4B_CHILDCONFIDANT, options_yesno, true),
        boldtext("4_if_so_who"),
        (new QuMultipleResponse{
            q1f(cb4b, S4B_CHILDCONFIDANT_SISTER, "4b_option_sister"),
            q1f(cb4b, S4B_CHILDCONFIDANT_BROTHER, "4b_option_brother"),
            q1f(cb4b, S4B_CHILDCONFIDANT_OTHERRELATIVE, "4b_option_relative"),
            q1f(cb4b, S4B_CHILDCONFIDANT_CLOSEFRIEND, "4b_option_closefriend"),
            q1f(cb4b, S4B_CHILDCONFIDANT_OTHERFRIEND, "4b_option_otherfriend"),
            q1f(cb4b, S4B_CHILDCONFIDANT_OTHER, "4b_option_other"),
        })->setMinimumAnswers(1)->addTag(TAG_4B_CHOSEN),
        text("rnc_if_other_describe"),
        textedit(cb4b, S4B_CHILDCONFIDANT_OTHER_DETAIL),
        text("4_note_anything"),
        textedit(cb4b, S4B_CHILDCONFIDANT_ADDITIONAL),
    })->setTitle(pagetitle("4B"))));

    // ------------------------------------------------------------------------
    // 4C
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("4_heading"),
        boldtext("4c_q"),
        (new QuMultipleResponse{
            q1f(cb4c, S4C_CLOSEST_MOTHER, "4c_option_mother"),
            q1f(cb4c, S4C_CLOSEST_FATHER, "4c_option_father"),
            q1f(cb4c, S4C_CLOSEST_SIBLING, "4c_option_sibling"),
            q1f(cb4c, S4C_CLOSEST_OTHERRELATIVE, "4c_option_relative"),
            q1f(cb4c, S4C_CLOSEST_ADULTFRIEND, "4c_option_adultfriend"),
            q1f(cb4c, S4C_CLOSEST_CHILDFRIEND, "4c_option_youngfriend"),
            q1f(cb4c, S4C_CLOSEST_OTHER, "4c_option_other"),
        })->setMinimumAnswers(2)->setMaximumAnswers(2),
        text("rnc_if_other_describe"),
        textedit(cb4c, S4C_CLOSEST_OTHER_DETAIL),
        text("4_note_anything"),
        textedit(cb4c, S4C_CLOSEST_ADDITIONAL),
    })->setTitle(pagetitle("4C"))));

    // ------------------------------------------------------------------------
    // 5
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("5_heading"),
        boldtext("5_mainq"),
        yn(cb5, S5C_PHYSICALABUSE, true),
        boldtext("if_no_move_on"),
        new QuGridContainer(3, {
            boldtext("if_yes"),
            boldtext("5_motherfigure"),
            boldtext("5_fatherfigure"),
            // ---
            // RNC extra bit:
            text("5_did_this_person_hurt_you"),
            yn(cb5, S5C_ABUSED_BY_MOTHER),
            yn(cb5, S5C_ABUSED_BY_FATHER),
            // ---
            text("5_how_old"),
            realedit(cb5, S5C_MOTHER_ABUSE_AGE_BEGAN, "age_years"),
            realedit(cb5, S5C_FATHER_ABUSE_AGE_BEGAN, "age_years"),
            // ---
            text("5_hit_more_than_once"),
            yn(cb5, S5C_MOTHER_HIT_MORE_THAN_ONCE),
            yn(cb5, S5C_FATHER_HIT_MORE_THAN_ONCE),
            // ---
            text("5_how_hit"),
            mcq(cb5, S5C_MOTHER_HIT_HOW, options_hit),
            mcq(cb5, S5C_FATHER_HIT_HOW, options_hit),
            // ---
            text("5_injured"),
            yn(cb5, S5C_MOTHER_INJURED),
            yn(cb5, S5C_FATHER_INJURED),
            // ---
            text("5_outofcontrol"),
            yn(cb5, S5C_MOTHER_OUT_OF_CONTROL),
            yn(cb5, S5C_FATHER_OUT_OF_CONTROL),
        }),
        text("5_can_you_describe_1"),
        textedit(cb5, S5C_PARENTAL_ABUSE_DESCRIPTION),
        text("5_anyone_else"),
        yn(cb5, S5C_ABUSE_BY_NONPARENT),
        text("5_can_you_describe_2"),
        textedit(cb5, S5C_NONPARENT_ABUSE_DESCRIPTION),
    })->setTitle(pagetitle("5"))));

    // ------------------------------------------------------------------------
    // 6
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        heading("6_heading"),
        boldtext("6_any_unwanted"),
        mcq(cb6, S6_ANY_UNWANTED_SEXUAL_EXPERIENCE, options3way_no_to_yes, true),
        boldtext("6_intercourse"),
        mcq(cb6, S6_UNWANTED_INTERCOURSE, options3way_no_to_yes, true),
        boldtext("6_upset_adult_authority"),
        mcq(cb6, S6_UPSETTING_SEXUAL_ADULT_AUTHORITY, options3way_no_to_yes, true),
        boldtext("6_if_none_move_on"),
        boldtext("6_if_yes_or_unsure"),
        new QuGridContainer(3, {
            new QuSpacer(),
            boldtext("6_first_experience"),
            boldtext("6_other_experience"),
            // ---
            text("6_q1"),
            realedit(cb6, S6_FIRST_AGE, "age_years"),
            realedit(cb6, S6_OTHER_AGE, "age_years"),
            // ---
            text("6_q2"),
            yn(cb6, S6_FIRST_PERSON_KNOWN),
            yn(cb6, S6_OTHER_PERSON_KNOWN),
            // ---
            text("6_q3"),
            yn(cb6, S6_FIRST_RELATIVE),
            yn(cb6, S6_OTHER_RELATIVE),
            // ---
            text("6_q4"),
            yn(cb6, S6_FIRST_IN_HOUSEHOLD),
            yn(cb6, S6_OTHER_IN_HOUSEHOLD),
            // ---
            text("6_q5"),
            yn(cb6, S6_FIRST_MORE_THAN_ONCE),
            yn(cb6, S6_OTHER_MORE_THAN_ONCE),
            // ---
            text("6_q6"),
            yn(cb6, S6_FIRST_TOUCH_PRIVATES_SUBJECT),
            yn(cb6, S6_OTHER_TOUCH_PRIVATES_SUBJECT),
            // ---
            text("6_q7"),
            yn(cb6, S6_FIRST_TOUCH_PRIVATES_OTHER),
            yn(cb6, S6_OTHER_TOUCH_PRIVATES_OTHER),
            // ---
            text("6_q8"),
            yn(cb6, S6_FIRST_INTERCOURSE),
            yn(cb6, S6_OTHER_INTERCOURSE),
        }),
        text("5_can_you_describe_1"),
        textedit(cb6, S6_UNWANTED_SEXUAL_DESCRIPTION),
    })->setTitle(pagetitle("6"))));

    // ------------------------------------------------------------------------
    // end
    // ------------------------------------------------------------------------
    pages.append(QuPagePtr((new QuPage{
        headingRaw(TextConst::thankYou()),
        text("final_1"),
        text("final_2"),
        text("any_other_comments"),
        textedit(cbdummy, ANY_OTHER_COMMENTS),
    })->setTitle(pagetitle("end"))));

    // ------------------------------------------------------------------------
    // Questionnaire
    // ------------------------------------------------------------------------
    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    dataChanged1A();
    dataChanged1B();
    dataChanged1C();
    dataChanged2A();
    dataChanged2B();
    dataChanged3A();
    dataChanged3B();
    dataChanged3C();
    dataChanged4A();
    dataChanged4B();
    dataChanged4C();
    dataChanged5();
    dataChanged6();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

bool CecaQ3::complete1A() const
{
    if (!complete1ASomebodySelected()) {
        return false;
    }
    if (valueBool(S1A_MOTHERFIGURE_OTHER) &&
            !valueBool(S1A_MOTHERFIGURE_OTHER_DETAIL)) {
        return false;
    }
    if (valueBool(S1A_MOTHERFIGURE_FEMALERELATIVE) &&
            !valueBool(S1A_MOTHERFIGURE_FEMALERELATIVE_DETAIL)) {
        return false;
    }
    if (valueBool(S1A_FATHERFIGURE_OTHER) &&
            !valueBool(S1A_FATHERFIGURE_OTHER_DETAIL)) {
        return false;
    }
    if (valueBool(S1A_FATHERFIGURE_MALERELATIVE) &&
            !valueBool(S1A_FATHERFIGURE_MALERELATIVE_DETAIL)) {
        return false;
    }
    return true;
}


bool CecaQ3::complete1ASomebodySelected() const
{
    return anyTrue(values(QStringList{
        S1A_MOTHERFIGURE_BIRTHMOTHER,
        S1A_MOTHERFIGURE_STEPMOTHER,
        S1A_MOTHERFIGURE_FEMALERELATIVE,
        S1A_MOTHERFIGURE_FAMILYFRIEND,
        S1A_MOTHERFIGURE_FOSTERMOTHER,
        S1A_MOTHERFIGURE_ADOPTIVEMOTHER,
        S1A_MOTHERFIGURE_OTHER,
        S1A_FATHERFIGURE_BIRTHFATHER,
        S1A_FATHERFIGURE_STEPFATHER,
        S1A_FATHERFIGURE_MALERELATIVE,
        S1A_FATHERFIGURE_FAMILYFRIEND,
        S1A_FATHERFIGURE_FOSTERFATHER,
        S1A_FATHERFIGURE_ADOPTIVEFATHER,
        S1A_FATHERFIGURE_OTHER
    }));
}


bool CecaQ3::complete1B() const
{
    if (valueIsNull(S1B_INSTITUTION)) {
        return false;
    }
    if (valueBool(S1B_INSTITUTION) && valueIsNull(S1B_INSTITUTION_TIME_YEARS)) {
        return false;
    }
    return true;
}


bool CecaQ3::complete1C() const
{
    if (valueIsNull(S1C_MOTHER_DIED) || valueIsNull(S1C_FATHER_DIED)) {
        return false;
    }
    if (valueBool(S1C_MOTHER_DIED) &&
            valueIsNull(S1C_MOTHER_DIED_SUBJECT_AGED)) {
        return false;
    }
    if (valueBool(S1C_FATHER_DIED) &&
            valueIsNull(S1C_FATHER_DIED_SUBJECT_AGED)) {
        return false;
    }
    if (valueIsNull(S1C_SEPARATED_FROM_MOTHER) ||
            valueIsNull(S1C_SEPARATED_FROM_FATHER)) {
        return false;
    }
    if (valueBool(S1C_SEPARATED_FROM_MOTHER)) {
        if (anyNull(values(QStringList{
                           S1C_FIRST_SEPARATED_FROM_MOTHER_AGED,
                           S1C_MOTHER_HOW_LONG_FIRST_SEPARATION_YEARS,
                           S1C_MOTHER_SEPARATION_REASON}))) {
            return false;
        }
    }
    if (valueBool(S1C_SEPARATED_FROM_FATHER)) {
        if (anyNull(values(QStringList{
                           S1C_FIRST_SEPARATED_FROM_FATHER_AGED,
                           S1C_FATHER_HOW_LONG_FIRST_SEPARATION_YEARS,
                           S1C_FATHER_SEPARATION_REASON}))) {
            return false;
        }
    }
    return true;
}


bool CecaQ3::complete2A() const
{
    if (valueIsNull(S2A_WHICH_MOTHER_FIGURE)) {
        return false;
    }
    if (!valueIsNull(S2A_WHICH_MOTHER_FIGURE) &&
            valueInt(S2A_WHICH_MOTHER_FIGURE) == 0) {  // "skip this section"
        return true;
    }
    if (valueInt(S2A_WHICH_MOTHER_FIGURE) == 5 &&
            valueIsNull(S2A_WHICH_MOTHER_FIGURE_OTHER_DETAIL)) {
        return false;
    }
    for (int i = 1; i <= 15; ++i) {  // not q16 (siblings)
        if (valueIsNull(strnum(FP_S2A, i))) {
            return false;
        }
    }
    return true;
}


bool CecaQ3::complete2B() const
{
    bool abuse = false;
    if (!valueIsNull(S2A_WHICH_MOTHER_FIGURE) &&
            valueInt(S2A_WHICH_MOTHER_FIGURE) == 0) {  // "skip this section"
        return true;
    }
    for (int i = 1; i <= 17; ++i) {
        const QString qstr = strnum(FP_S2B, i);
        if (valueIsNull(qstr)) {
            return false;
        }
        if (valueInt(qstr) != 0) {
            abuse = true;
            if (valueIsNull(qstr + FS_FREQUENCY)) {
                return false;
            }
        }
    }
    if (abuse && valueIsNull(S2B_AGE_BEGAN)) {
        return false;
    }
    return true;
}


bool CecaQ3::complete3A() const
{
    if (valueIsNull(S3A_WHICH_FATHER_FIGURE)) {
        return false;
    }
    if (!valueIsNull(S3A_WHICH_FATHER_FIGURE) &&
            valueInt(S3A_WHICH_FATHER_FIGURE) == 0) {  // "skip this section"
        return true;
    }
    if (valueInt(S3A_WHICH_FATHER_FIGURE) == 5 &&
            valueIsNull(S3A_WHICH_FATHER_FIGURE_OTHER_DETAIL)) {
        return false;
    }
    for (int i = 1; i <= 15; ++i) {  // not q16 (siblings)
        if (valueIsNull(strnum(FP_S3A, i))) {
            return false;
        }
    }
    return true;
}


bool CecaQ3::complete3B() const
{
    bool abuse = false;
    if (!valueIsNull(S3A_WHICH_FATHER_FIGURE) &&
            valueInt(S3A_WHICH_FATHER_FIGURE) == 0) {  // "skip this section"
        return true;
    }
    for (int i = 1; i <= 17; ++i) {
        const QString qstr = strnum(FP_S3B, i);
        if (valueIsNull(qstr)) {
            return false;
        }
        if (valueInt(qstr) != 0) {
            abuse = true;
            if (valueIsNull(qstr + FS_FREQUENCY)) {
                return false;
            }
        }
    }
    if (abuse && valueIsNull(S3B_AGE_BEGAN)) {
        return false;
    }
    return true;
}


bool CecaQ3::complete3C() const
{
    return noneNull(values(QStringList{
                               S3C_Q1,
                               S3C_Q2,
                               S3C_Q3,
                               S3C_Q4,
                               S3C_Q5,
                               S3C_Q6,
                               S3C_Q7,
                               S3C_Q8,
                               S3C_Q9,
                               S3C_Q10,
                               S3C_Q11,
                               S3C_Q12,
                               S3C_Q13,
                               S3C_Q14,
                               S3C_Q15,
                               S3C_Q16,
                               S3C_Q17,
                               S3C_WHICH_PARENT_CARED_FOR,
                               S3C_PARENT_MENTAL_PROBLEM,
                               S3C_PARENT_PHYSICAL_PROBLEM}));
}


bool CecaQ3::complete4A() const
{
    if (valueIsNull(S4A_ADULTCONFIDANT)) {
        return false;
    }
    if (!valueBool(S4A_ADULTCONFIDANT)) {
        return true;
    }
    if (!anyTrue(values(QStringList{
                        S4A_ADULTCONFIDANT_MOTHER,
                        S4A_ADULTCONFIDANT_FATHER,
                        S4A_ADULTCONFIDANT_OTHERRELATIVE,
                        S4A_ADULTCONFIDANT_FAMILYFRIEND,
                        S4A_ADULTCONFIDANT_RESPONSIBLEADULT,
                        S4A_ADULTCONFIDANT_OTHER}))) {
        return false;
    }
    if (valueBool(S4A_ADULTCONFIDANT_OTHER) &&
            valueString(S4A_ADULTCONFIDANT_OTHER_DETAIL).isEmpty()) {
        return false;
    }
    return true;
}


bool CecaQ3::complete4B() const
{
    if (valueIsNull(S4B_CHILDCONFIDANT)) {
        return false;
    }
    if (!valueBool(S4B_CHILDCONFIDANT)) {
        return true;
    }
    if (!anyTrue(values(QStringList{
                        S4B_CHILDCONFIDANT_SISTER,
                        S4B_CHILDCONFIDANT_BROTHER,
                        S4B_CHILDCONFIDANT_OTHERRELATIVE,
                        S4B_CHILDCONFIDANT_CLOSEFRIEND,
                        S4B_CHILDCONFIDANT_OTHERFRIEND,
                        S4B_CHILDCONFIDANT_OTHER}))) {
        return false;
    }
    if (valueBool(S4B_CHILDCONFIDANT_OTHER) &&
            valueString(S4B_CHILDCONFIDANT_OTHER_DETAIL).isEmpty()) {
        return false;
    }
    return true;
}


bool CecaQ3::complete4C() const
{
    int n = valueBool(S4C_CLOSEST_MOTHER) +
            valueBool(S4C_CLOSEST_FATHER) +
            valueBool(S4C_CLOSEST_SIBLING) +
            valueBool(S4C_CLOSEST_OTHERRELATIVE) +
            valueBool(S4C_CLOSEST_ADULTFRIEND) +
            valueBool(S4C_CLOSEST_CHILDFRIEND) +
            valueBool(S4C_CLOSEST_OTHER);
    if (n < 2) {
        return false;
    }
    if (valueBool(S4C_CLOSEST_OTHER) &&
            valueString(S4C_CLOSEST_OTHER_DETAIL).isEmpty()) {
        return false;
    }
    return true;
}


bool CecaQ3::complete5() const
{
    if (valueIsNull(S5C_PHYSICALABUSE)) {
        return false;
    }
    if (!valueBool(S5C_PHYSICALABUSE)) {
        return true;
    }
    if (valueIsNull(S5C_ABUSED_BY_MOTHER) ||
            valueIsNull(S5C_ABUSED_BY_FATHER) ||
            valueIsNull(S5C_ABUSE_BY_NONPARENT)) {
        return false;
    }
    if (valueBool(S5C_ABUSED_BY_MOTHER)) {
        if (anyNull(values(QStringList{
                           S5C_MOTHER_ABUSE_AGE_BEGAN,
                           S5C_MOTHER_HIT_MORE_THAN_ONCE,
                           S5C_MOTHER_HIT_HOW,
                           S5C_MOTHER_INJURED,
                           S5C_MOTHER_OUT_OF_CONTROL}))) {
            return false;
        }
    }
    if (valueBool(S5C_ABUSED_BY_FATHER)) {
        if (anyNull(values(QStringList{
                           S5C_FATHER_ABUSE_AGE_BEGAN,
                           S5C_FATHER_HIT_MORE_THAN_ONCE,
                           S5C_FATHER_HIT_HOW,
                           S5C_FATHER_INJURED,
                           S5C_FATHER_OUT_OF_CONTROL}))) {
            return false;
        }
    }
    return true;
}


bool CecaQ3::complete6() const
{
    if (valueIsNull(S6_ANY_UNWANTED_SEXUAL_EXPERIENCE) ||
            valueIsNull(S6_UNWANTED_INTERCOURSE) ||
            valueIsNull(S6_UPSETTING_SEXUAL_ADULT_AUTHORITY)) {
        return false;
    }
    if (!valueBool(S6_ANY_UNWANTED_SEXUAL_EXPERIENCE) &&
            !valueBool(S6_UNWANTED_INTERCOURSE) &&
            !valueBool(S6_UPSETTING_SEXUAL_ADULT_AUTHORITY)) {
        return true;
    }
    if (anyNull(values(QStringList{
                       S6_FIRST_AGE,
                       S6_FIRST_PERSON_KNOWN,
                       S6_FIRST_RELATIVE,
                       S6_FIRST_IN_HOUSEHOLD,
                       S6_FIRST_MORE_THAN_ONCE,
                       S6_FIRST_TOUCH_PRIVATES_SUBJECT,
                       S6_FIRST_TOUCH_PRIVATES_OTHER,
                       S6_FIRST_INTERCOURSE}))) {
        return false;
    }
    // no checks for "other experience"
    return true;
}


// ============================================================================
// Signal handlers
// ============================================================================

void CecaQ3::dataChanged1A()
{
    if (!m_questionnaire) {
        return;
    }
    // 1. Do we need more people?
    // We want at least one overall.
    const int n_req = complete1ASomebodySelected() ? 0 : 1;
    setMultipleResponseMinAnswers(TAG_1A_PEOPLE, n_req);
    // 2. Simpler things
    fieldRef(S1A_MOTHERFIGURE_FEMALERELATIVE_DETAIL)->setMandatory(
                valueBool(S1A_MOTHERFIGURE_FEMALERELATIVE));
    fieldRef(S1A_MOTHERFIGURE_OTHER_DETAIL)->setMandatory(
                valueBool(S1A_MOTHERFIGURE_OTHER));
    fieldRef(S1A_FATHERFIGURE_MALERELATIVE_DETAIL)->setMandatory(
                valueBool(S1A_FATHERFIGURE_MALERELATIVE));
    fieldRef(S1A_FATHERFIGURE_OTHER_DETAIL)->setMandatory(
                valueBool(S1A_FATHERFIGURE_OTHER));
}


void CecaQ3::dataChanged1B()
{
    fieldRef(S1B_INSTITUTION_TIME_YEARS)->setMandatory(
                valueBool(S1B_INSTITUTION));
}


void CecaQ3::dataChanged1C()
{
    fieldRef(S1C_MOTHER_DIED_SUBJECT_AGED)->setMandatory(
                valueBool(S1C_MOTHER_DIED));
    fieldRef(S1C_FATHER_DIED_SUBJECT_AGED)->setMandatory(
                valueBool(S1C_FATHER_DIED));
    setMandatory(valueBool(S1C_SEPARATED_FROM_MOTHER), QStringList{
                     S1C_FIRST_SEPARATED_FROM_MOTHER_AGED,
                     S1C_MOTHER_HOW_LONG_FIRST_SEPARATION_YEARS,
                     S1C_MOTHER_SEPARATION_REASON});
    setMandatory(valueBool(S1C_SEPARATED_FROM_FATHER), QStringList{
                     S1C_FIRST_SEPARATED_FROM_FATHER_AGED,
                     S1C_FATHER_HOW_LONG_FIRST_SEPARATION_YEARS,
                     S1C_FATHER_SEPARATION_REASON});
}


void CecaQ3::dataChanged2A()
{
    fieldRef(S2A_WHICH_MOTHER_FIGURE_OTHER_DETAIL)->setMandatory(
                valueInt(S2A_WHICH_MOTHER_FIGURE) == 5);
    const bool needed = valueInt(S2A_WHICH_MOTHER_FIGURE) != 0;
    for (int i = 1; i <= 15; ++i) {
        fieldRef(strnum(FP_S2A, i))->setMandatory(needed);
    }
    // q16 never mandatory

    if (!m_questionnaire) {
        return;
    }
    m_questionnaire->setPageSkip(PAGETAG_2B, !needed, true);
}


void CecaQ3::dataChanged2B()
{
    bool abuse = false;
    for (int i = 1; i <= 17; ++i) {
        const QString qstr = strnum(FP_S2B, i);
        fieldRef(qstr)->setMandatory(true);
        const int v = valueInt(qstr);
        abuse = abuse || v;
        fieldRef(qstr + FS_FREQUENCY)->setMandatory(v);  // not null and not zero
    }
    fieldRef(S2B_AGE_BEGAN)->setMandatory(abuse);
}


void CecaQ3::dataChanged3A()
{
    fieldRef(S3A_WHICH_FATHER_FIGURE_OTHER_DETAIL)->setMandatory(
                valueInt(S3A_WHICH_FATHER_FIGURE) == 5);
    const bool needed = valueInt(S3A_WHICH_FATHER_FIGURE) != 0;
    for (int i = 1; i <= 15; ++i) {
        fieldRef(strnum(FP_S3A, i))->setMandatory(needed);
    }
    // q16 never mandatory

    if (!m_questionnaire) {
        return;
    }
    m_questionnaire->setPageSkip(PAGETAG_3B, !needed, true);
}


void CecaQ3::dataChanged3B()
{
    bool abuse = false;
    for (int i = 1; i <= 17; ++i) {
        const QString qstr = strnum(FP_S3B, i);
        fieldRef(qstr)->setMandatory(true);
        const int v = valueInt(qstr);
        abuse = abuse || v;
        fieldRef(qstr + FS_FREQUENCY)->setMandatory(v);  // not null and not zero
    }
    fieldRef(S3B_AGE_BEGAN)->setMandatory(abuse);
}


void CecaQ3::dataChanged3C()
{
    // nothing of interest
}


void CecaQ3::dataChanged4A()
{
    // 1. Multiple response
    const int n_req = valueBool(S4A_ADULTCONFIDANT) ? 1 : 0;
    setMultipleResponseMinAnswers(TAG_4A_CHOSEN, n_req);
    // 2. Other
    fieldRef(S4A_ADULTCONFIDANT_OTHER_DETAIL)->setMandatory(
                valueBool(S4A_ADULTCONFIDANT_OTHER));
}


void CecaQ3::dataChanged4B()
{
    // 1. Multiple response
    const int n_req = valueBool(S4B_CHILDCONFIDANT) ? 1 : 0;
    setMultipleResponseMinAnswers(TAG_4B_CHOSEN, n_req);
    // 2. Other
    fieldRef(S4B_CHILDCONFIDANT_OTHER_DETAIL)->setMandatory(
                valueBool(S4B_CHILDCONFIDANT_OTHER));
}


void CecaQ3::dataChanged4C()
{
    fieldRef(S4C_CLOSEST_OTHER_DETAIL)->setMandatory(
                valueBool(S4C_CLOSEST_OTHER));
}


void CecaQ3::dataChanged5()
{
    const bool physical_abuse = valueBool(S5C_PHYSICALABUSE);
    const bool by_mother = physical_abuse && valueBool(S5C_ABUSED_BY_MOTHER);
    const bool by_father = physical_abuse && valueBool(S5C_ABUSED_BY_FATHER);
    // Free-text descriptions should not be mandatory.
    setMandatory(physical_abuse, QStringList{
                     S5C_ABUSED_BY_MOTHER,
                     S5C_ABUSED_BY_FATHER,
                     // S5C_PARENTAL_ABUSE_DESCRIPTION,  // is generic
                     S5C_ABUSE_BY_NONPARENT});
    setMandatory(by_mother, QStringList{
                     S5C_MOTHER_ABUSE_AGE_BEGAN,
                     S5C_MOTHER_HIT_MORE_THAN_ONCE,
                     S5C_MOTHER_HIT_HOW,
                     S5C_MOTHER_INJURED,
                     S5C_MOTHER_OUT_OF_CONTROL});
    setMandatory(by_father, QStringList{
                     S5C_FATHER_ABUSE_AGE_BEGAN,
                     S5C_FATHER_HIT_MORE_THAN_ONCE,
                     S5C_FATHER_HIT_HOW,
                     S5C_FATHER_INJURED,
                     S5C_FATHER_OUT_OF_CONTROL});
    // setMandatory(by_nonparent, QStringList{S5C_NONPARENT_ABUSE_DESCRIPTION});
}


void CecaQ3::dataChanged6()
{
    const bool somesex = valueBool(S6_ANY_UNWANTED_SEXUAL_EXPERIENCE) ||
            valueBool(S6_UNWANTED_INTERCOURSE) ||
            valueBool(S6_UPSETTING_SEXUAL_ADULT_AUTHORITY);
    setMandatory(somesex, QStringList{
                     S6_FIRST_AGE,
                     // S6_OTHER_AGE,
                     S6_FIRST_PERSON_KNOWN,
                     // S6_OTHER_PERSON_KNOWN,
                     S6_FIRST_RELATIVE,
                     // S6_OTHER_RELATIVE,
                     S6_FIRST_IN_HOUSEHOLD,
                     // S6_OTHER_IN_HOUSEHOLD,
                     S6_FIRST_MORE_THAN_ONCE,
                     // S6_OTHER_MORE_THAN_ONCE,
                     S6_FIRST_TOUCH_PRIVATES_SUBJECT,
                     // S6_OTHER_TOUCH_PRIVATES_SUBJECT,
                     S6_FIRST_TOUCH_PRIVATES_OTHER,
                     // S6_OTHER_TOUCH_PRIVATES_OTHER,
                     S6_FIRST_INTERCOURSE,
                     // S6_OTHER_INTERCOURSE
                 });
}


void CecaQ3::dataChangedDummy()
{
    // do nothing
}


void CecaQ3::setMandatory(const bool mandatory,
                          const QStringList& fieldnames)
{
    for (const QString& fieldname : fieldnames) {
        fieldRef(fieldname)->setMandatory(mandatory);
    }
}


void CecaQ3::setMultipleResponseMinAnswers(const QString& tag,
                                           const int min_answers)
{
    if (!m_questionnaire) {
        return;
    }
    QVector<QuElement*> elements = m_questionnaire->getElementsByTag(tag, false);
    for (auto e : elements) {
        auto mr = dynamic_cast<QuMultipleResponse*>(e);
        if (!mr) {
            continue;
        }
        mr->setMinimumAnswers(min_answers);
    }
}
