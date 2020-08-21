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

/*

===============================================================================
PRIMARY REFERENCE
===============================================================================

CIS-R: Lewis et al. 1992
- https://www.ncbi.nlm.nih.gov/pubmed/1615114

Helpful chronology:
- https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3347904/#b2-mjms-13-1-058

Primary IMPLEMENTATION reference:
- "BASIC CIS-R 02-03-2010.pqs"

===============================================================================
INTERNALS
===============================================================================

The internal methodology of the CIS-R is that the textfile
"BASIC CIS-R 02-03-2010.pqs" contains a sequence, of the type:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LABEL_A1

"Some question?"

[
1 "Option 1"
2 "Option 2"
3 "Option 3"
]

SOMEVAR := answer * 10
if answer == 1 then goto LABEL_B1;
if answer == 2 then goto LABEL_B2;
if answer == 3 then goto LABEL_B3;

&

LABEL_B1

...

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

===============================================================================
IMPLEMENTATION: EARLY THOUGHTS -- NOT USED
===============================================================================

We could in principle do this with a Questionnaire interface, but to be honest
it's going to be easiest to translate with a "direct" interface. Maybe
something like:


void start()
{
    goto_question(1);
}

void goto_question(int question)
{
    m_current_question = question;
    offer_question();
}

bool offer_question()
{
    // do interesting things
    connect(answer_1, answered, 1);
    return true;
}

bool answered(int answer)
{
    // returns: something we don't care about
    switch (m_current_question) {
    case 1:
        if (answer == 1) {
            m_diagnosis_blah = true;
            return goto_question(2);
            // C++11: can't do "return goto_question(2);" if goto_question() is
            // of void type, as you're not allowed to return anything from a
            // void function, even of void type:
            // http://stackoverflow.com/questions/35987493/return-void-type-in-c-and-c
            // ... so just make them bool or something.
            // It would be OK in C++14.
        }
        break;
    // ...
    }
}


===============================================================================
IMPLEMENTATION: FURTHER THOUGHTS
===============================================================================

FURTHER THOUGHTS: we'll implement a DynamicQuestionnaire class; q.v.

*/

// #define DEBUG_SHOW_PAGE_TAGS
#define SHOW_QUESTIONS_CONSIDERED  // helpful; leave it on

#include "cisr.h"
#include "lib/stringfunc.h"
#include "maths/mathfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/dynamicquestionnaire.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qupage.h"
#include "tasklib/taskfactory.h"

using namespace cisrconst;
#define CQ Cisr::CisrQuestion
// if you try "using", you get "'Cisr' is not a namespace or unscoped enum"

const QString Cisr::CISR_TABLENAME("cisr");

// FP field prefix; NUM field numbers; FN field name
// These match the electronic version of the CIS-R, as per "BASIC CIS-R 02-03-2010.pqs".

const QString FN_ETHNIC("ethnic");
const QString FN_MARRIED("married");
const QString FN_EMPSTAT("empstat");
const QString FN_EMPTYPE("emptype");
const QString FN_HOME("home");

const QString FN_APPETITE1("appetite1");
const QString FN_WEIGHT1("weight1");
const QString FN_WEIGHT2("weight2");
const QString FN_WEIGHT3("weight3");
const QString FN_APPETITE2("appetite2");
const QString FN_WEIGHT4("weight4");  // male/female responses unified (no "weight4a")
const QString FN_WEIGHT5("weight5");

const QString FN_GP_YEAR("gp_year");
const QString FN_DISABLE("disable");
const QString FN_ILLNESS("illness");

const QString FN_SOMATIC_MAND1("somatic_mand1");
const QString FN_SOMATIC_PAIN1("somatic_pain1");
const QString FN_SOMATIC_PAIN2("somatic_pain2");
const QString FN_SOMATIC_PAIN3("somatic_pain3");
const QString FN_SOMATIC_PAIN4("somatic_pain4");
const QString FN_SOMATIC_PAIN5("somatic_pain5");
const QString FN_SOMATIC_MAND2("somatic_mand2");
const QString FN_SOMATIC_DIS1("somatic_dis1");
const QString FN_SOMATIC_DIS2("somatic_dis2");
const QString FN_SOMATIC_DIS3("somatic_dis3");
const QString FN_SOMATIC_DIS4("somatic_dis4");
const QString FN_SOMATIC_DIS5("somatic_dis5");
const QString FN_SOMATIC_DUR("somatic_dur");

const QString FN_FATIGUE_MAND1("fatigue_mand1");
const QString FN_FATIGUE_CAUSE1("fatigue_cause1");
const QString FN_FATIGUE_TIRED1("fatigue_tired1");
const QString FN_FATIGUE_TIRED2("fatigue_tired2");
const QString FN_FATIGUE_TIRED3("fatigue_tired3");
const QString FN_FATIGUE_TIRED4("fatigue_tired4");
const QString FN_FATIGUE_MAND2("fatigue_mand2");
const QString FN_FATIGUE_CAUSE2("fatigue_cause2");
const QString FN_FATIGUE_ENERGY1("fatigue_energy1");
const QString FN_FATIGUE_ENERGY2("fatigue_energy2");
const QString FN_FATIGUE_ENERGY3("fatigue_energy3");
const QString FN_FATIGUE_ENERGY4("fatigue_energy4");
const QString FN_FATIGUE_DUR("fatigue_dur");

const QString FN_CONC_MAND1("conc_mand1");
const QString FN_CONC_MAND2("conc_mand2");
const QString FN_CONC1("conc1");
const QString FN_CONC2("conc2");
const QString FN_CONC3("conc3");
const QString FN_CONC_DUR("conc_dur");
const QString FN_CONC4("conc4");
const QString FN_FORGET_DUR("forget_dur");

const QString FN_SLEEP_MAND1("sleep_mand1");
const QString FN_SLEEP_LOSE1("sleep_lose1");
const QString FN_SLEEP_LOSE2("sleep_lose2");
const QString FN_SLEEP_LOSE3("sleep_lose3");
const QString FN_SLEEP_EMW("sleep_emw");
const QString FN_SLEEP_CAUSE("sleep_cause");
const QString FN_SLEEP_MAND2("sleep_mand2");
const QString FN_SLEEP_GAIN1("sleep_gain1");
const QString FN_SLEEP_GAIN2("sleep_gain2");
const QString FN_SLEEP_GAIN3("sleep_gain3");
const QString FN_SLEEP_DUR("sleep_dur");

const QString FN_IRRIT_MAND1("irrit_mand1");
const QString FN_IRRIT_MAND2("irrit_mand2");
const QString FN_IRRIT1("irrit1");
const QString FN_IRRIT2("irrit2");
const QString FN_IRRIT3("irrit3");
const QString FN_IRRIT4("irrit4");
const QString FN_IRRIT_DUR("irrit_dur");

const QString FN_HYPO_MAND1("hypo_mand1");
const QString FN_HYPO_MAND2("hypo_mand2");
const QString FN_HYPO1("hypo1");
const QString FN_HYPO2("hypo2");
const QString FN_HYPO3("hypo3");
const QString FN_HYPO4("hypo4");
const QString FN_HYPO_DUR("hypo_dur");

const QString FN_DEPR_MAND1("depr_mand1");
const QString FN_DEPR1("depr1");
const QString FN_DEPR_MAND2("depr_mand2");
const QString FN_DEPR2("depr2");
const QString FN_DEPR3("depr3");
const QString FN_DEPR4("depr4");
const QString FN_DEPR_CONTENT("depr_content");
const QString FN_DEPR5("depr5");
const QString FN_DEPR_DUR("depr_dur");
const QString FN_DEPTH1("depth1");
const QString FN_DEPTH2("depth2");
const QString FN_DEPTH3("depth3");
const QString FN_DEPTH4("depth4");
const QString FN_DEPTH5("depth5");
const QString FN_DEPTH6("depth6");
const QString FN_DEPTH7("depth7");
const QString FN_DEPTH8("depth8");
const QString FN_DEPTH9("depth9");
const QString FN_DEPTH10("depth10");
const QString FN_DOCTOR("doctor");

const QString FN_WORRY_MAND1("worry_mand1");
const QString FN_WORRY_MAND2("worry_mand2");
const QString FN_WORRY_CONT1("worry_cont1");
const QString FN_WORRY2("worry2");
const QString FN_WORRY3("worry3");
const QString FN_WORRY4("worry4");
const QString FN_WORRY5("worry5");
const QString FN_WORRY_DUR("worry_dur");

const QString FN_ANX_MAND1("anx_mand1");
const QString FN_ANX_MAND2("anx_mand2");
const QString FN_ANX_PHOBIA1("anx_phobia1");
const QString FN_ANX_PHOBIA2("anx_phobia2");
const QString FN_ANX2("anx2");
const QString FN_ANX3("anx3");
const QString FN_ANX4("anx4");
const QString FN_ANX5("anx5");
const QString FN_ANX_DUR("anx_dur");

const QString FN_PHOBIAS_MAND("phobias_mand");
const QString FN_PHOBIAS_TYPE1("phobias_type1");
const QString FN_PHOBIAS1("phobias1");
const QString FN_PHOBIAS2("phobias2");
const QString FN_PHOBIAS3("phobias3");
const QString FN_PHOBIAS4("phobias4");
const QString FN_PHOBIAS_DUR("phobias_dur");

const QString FN_PANIC_MAND("panic_mand");
const QString FN_PANIC1("panic1");
const QString FN_PANIC2("panic2");
const QString FN_PANIC3("panic3");
const QString FN_PANIC4("panic4");
const QString FN_PANSYM_A("pansym_a");
const QString FN_PANSYM_B("pansym_b");
const QString FN_PANSYM_C("pansym_c");
const QString FN_PANSYM_D("pansym_d");
const QString FN_PANSYM_E("pansym_e");
const QString FN_PANSYM_F("pansym_f");
const QString FN_PANSYM_G("pansym_g");
const QString FN_PANSYM_H("pansym_h");
const QString FN_PANSYM_I("pansym_i");
const QString FN_PANSYM_J("pansym_j");
const QString FN_PANSYM_K("pansym_k");
const QString FN_PANSYM_L("pansym_l");
const QString FN_PANSYM_M("pansym_m");
const QString FN_PANIC5("panic5");
const QString FN_PANIC_DUR("panic_dur");

const QString FN_COMP_MAND1("comp_mand1");
const QString FN_COMP1("comp1");
const QString FN_COMP2("comp2");
const QString FN_COMP3("comp3");
const QString FN_COMP4("comp4");
const QString FN_COMP_DUR("comp_dur");

const QString FN_OBSESS_MAND1("obsess_mand1");
const QString FN_OBSESS_MAND2("obsess_mand2");
const QString FN_OBSESS1("obsess1");
const QString FN_OBSESS2("obsess2");
const QString FN_OBSESS3("obsess3");
const QString FN_OBSESS4("obsess4");
const QString FN_OBSESS_DUR("obsess_dur");

const QString FN_OVERALL2("overall2");

const QString XSTRING_QUESTION_SUFFIX = "_q";
const QString XSTRING_EXTRA_STEM_SUFFIX = "_s";

// Define question types:

// Questions for which 1 = no, 2 = yes (+/- other options)
const QVector<CQ> QUESTIONS_1_NO_2_YES{
    CQ::APPETITE1_LOSS_PAST_MONTH,
    CQ::WEIGHT1_LOSS_PAST_MONTH,
    CQ::WEIGHT2_TRYING_TO_LOSE,
    CQ::APPETITE2_INCREASE_PAST_MONTH,
    CQ::WEIGHT4_INCREASE_PAST_MONTH,  // may also offer "yes but pregnant"
    CQ::SOMATIC_MAND1_PAIN_PAST_MONTH,
    CQ::SOMATIC_MAND2_DISCOMFORT,
    CQ::SOMATIC_PAIN3_GT_3H_ANY_DAY,
    CQ::SOMATIC_PAIN5_INTERRUPTED_INTERESTING,  // also has other options
    CQ::SOMATIC_DIS3_GT_3H_ANY_DAY,
    CQ::SOMATIC_DIS5_INTERRUPTED_INTERESTING,  // also has other options
    CQ::FATIGUE_MAND1_TIRED_PAST_MONTH,
    CQ::FATIGUE_TIRED2_GT_3H_ANY_DAY,
    CQ::FATIGUE_TIRED3_HAD_TO_PUSH,
    CQ::FATIGUE_TIRED4_DURING_ENJOYABLE,  // also has other options
    CQ::FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH,
    CQ::FATIGUE_ENERGY2_GT_3H_ANY_DAY,
    CQ::FATIGUE_ENERGY3_HAD_TO_PUSH,
    CQ::FATIGUE_ENERGY4_DURING_ENJOYABLE,  // also has other options
    CQ::CONC_MAND1_POOR_CONC_PAST_MONTH,
    CQ::CONC_MAND2_FORGETFUL_PAST_MONTH,
    CQ::CONC3_CONC_PREVENTED_ACTIVITIES,
    CQ::CONC4_FORGOTTEN_IMPORTANT,
    CQ::SLEEP_MAND1_LOSS_PAST_MONTH,
    CQ::SLEEP_EMW_PAST_WEEK,
    CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH,
    CQ::IRRIT2_GT_1H_ANY_DAY,
    CQ::HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH,
    CQ::HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS,
    CQ::HYPO2_WORRY_TOO_MUCH,
    CQ::DEPR_MAND1_LOW_MOOD_PAST_MONTH,
    CQ::DEPR1_LOW_MOOD_PAST_WEEK,
    CQ::DEPR4_GT_3H_ANY_DAY,
    CQ::DEPTH3_RESTLESS,
    CQ::DEPTH4_SLOWED,
    CQ::DEPTH6_WORSE_THAN_OTHERS,
    CQ::DEPTH7_HOPELESS,
    CQ::DEPTH10_SUICIDE_METHOD,
    CQ::WORRY_MAND2_ANY_WORRIES_PAST_MONTH,
    CQ::WORRY3_TOO_MUCH,
    CQ::WORRY5_GT_3H_ANY_DAY,
    CQ::ANX_MAND1_ANXIETY_PAST_MONTH,
    CQ::ANX_PHOBIA1_SPECIFIC_PAST_MONTH,
    CQ::ANX4_GENERAL_PHYSICAL_SYMPTOMS,
    CQ::ANX5_GENERAL_GT_3H_ANY_DAY,
    CQ::PHOBIAS_MAND_AVOIDANCE_PAST_MONTH,
    CQ::PHOBIAS2_PHYSICAL_SYMPTOMS,
    CQ::PHOBIAS3_AVOIDANCE,
    CQ::PANIC4_RAPID_ONSET,
    CQ::PANIC5_ALWAYS_SPECIFIC_TRIGGER,
    CQ::COMP2_TRIED_TO_STOP,
    CQ::COMP3_UPSETTING,
    CQ::OBSESS2_TRIED_TO_STOP,
    CQ::OBSESS3_UPSETTING,
};
// Questions for which 1 = yes, 2 = no (+/- other options)
const QVector<CQ> QUESTIONS_1_YES_2_NO{
    CQ::DISABLE,
    CQ::CONC2_CONC_FOR_TV_READING_CONVERSATION,
    CQ::HYPO4_CAN_DISTRACT,
};
// Yes-no (or no-yes) questions but with specific text
const QVector<CQ> QUESTIONS_YN_SPECIFIC_TEXT{
    CQ::WEIGHT2_TRYING_TO_LOSE,
    CQ::SOMATIC_PAIN3_GT_3H_ANY_DAY,
    CQ::SOMATIC_DIS3_GT_3H_ANY_DAY,
    CQ::FATIGUE_TIRED2_GT_3H_ANY_DAY,
    CQ::FATIGUE_TIRED3_HAD_TO_PUSH,
    CQ::FATIGUE_ENERGY2_GT_3H_ANY_DAY,
    CQ::FATIGUE_ENERGY3_HAD_TO_PUSH,
    CQ::CONC_MAND1_POOR_CONC_PAST_MONTH,
    CQ::CONC2_CONC_FOR_TV_READING_CONVERSATION,
    CQ::CONC4_FORGOTTEN_IMPORTANT,
    CQ::SLEEP_EMW_PAST_WEEK,
    CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH,
    CQ::IRRIT2_GT_1H_ANY_DAY,
    CQ::HYPO2_WORRY_TOO_MUCH,
    CQ::HYPO4_CAN_DISTRACT,
    CQ::DEPR1_LOW_MOOD_PAST_WEEK,
    CQ::DEPR4_GT_3H_ANY_DAY,
    CQ::DEPTH6_WORSE_THAN_OTHERS,
    CQ::DEPTH7_HOPELESS,
    CQ::WORRY3_TOO_MUCH,
    CQ::WORRY5_GT_3H_ANY_DAY,
    CQ::ANX4_GENERAL_PHYSICAL_SYMPTOMS,
    CQ::ANX5_GENERAL_GT_3H_ANY_DAY,
    CQ::PHOBIAS2_PHYSICAL_SYMPTOMS,
    CQ::PHOBIAS3_AVOIDANCE,
    CQ::COMP2_TRIED_TO_STOP,
    CQ::COMP3_UPSETTING,
    CQ::OBSESS2_TRIED_TO_STOP,
    CQ::OBSESS3_UPSETTING,
};
// Demographics questions (optional for diagnosis)
const QVector<CQ> QUESTIONS_DEMOGRAPHICS{
    CQ::ETHNIC,
    CQ::MARRIED,
    CQ::EMPSTAT,
    CQ::EMPTYPE,
    CQ::HOME,
};
// "Questions" that are just a prompt screen
const QMap<CQ, QString> QUESTIONS_PROMPT_ONLY{
    // Maps questions to their prompt's xstring name
    {CQ::INTRO_1, "intro_1"},
    {CQ::INTRO_2, "intro_2"},
    {CQ::INTRO_DEMOGRAPHICS, "intro_demographics_statement"},

    {CQ::HEALTH_WELLBEING, "health_wellbeing_statement"},
    {CQ::DOCTOR2_PLEASE_TALK_TO, "doctor2"},
    {CQ::DEPR_OUTRO, "depr_outro"},
    {CQ::WORRY1_INFO_ONLY, "worry1"},
    {CQ::ANX1_INFO_ONLY, "anx1"},
    {CQ::ANX_OUTRO, "anx_outro"},
    {CQ::OVERALL1_INFO_ONLY, "overall1"},
    {CQ::THANKS_FINISHED, "end"},
};
// "How many days per week" questions
// "Overall duration" questions
const QVector<CQ> QUESTIONS_OVERALL_DURATION{
    CQ::SOMATIC_DUR,
    CQ::FATIGUE_DUR,
    CQ::CONC_DUR,
    CQ::FORGET_DUR,
    CQ::SLEEP_DUR,
    CQ::IRRIT_DUR,
    CQ::HYPO_DUR,
    CQ::DEPR_DUR,
    CQ::WORRY_DUR,
    CQ::ANX_DUR_GENERAL,
    CQ::PHOBIAS_DUR,
    CQ::PANIC_DUR,
    CQ::COMP_DUR,
    CQ::OBSESS_DUR,
};
// Multi-way questions, other than yes/no ones.
const QMap<CQ, QPair<int, int>> QUESTIONS_MULTIWAY{
    // Maps questions to first and last number of answers.
    {CQ::WEIGHT3_LOST_LOTS, {1, 2}},
    {CQ::WEIGHT4_INCREASE_PAST_MONTH, {1, 2}},  // may be modified to 3 if female
    {CQ::WEIGHT5_GAINED_LOTS, {1, 2}},
    {CQ::GP_YEAR, {0, 4}},  // unusual; starts at 0
    {CQ::ILLNESS, {1, 8}},
    {CQ::SOMATIC_PAIN1_PSYCHOL_EXAC, {1, 3}},
    {CQ::SOMATIC_PAIN5_INTERRUPTED_INTERESTING, {1, 3}},
    {CQ::SOMATIC_DIS1_PSYCHOL_EXAC, {1, 3}},
    {CQ::SOMATIC_DIS5_INTERRUPTED_INTERESTING, {1, 3}},
    {CQ::FATIGUE_TIRED4_DURING_ENJOYABLE, {1, 3}},
    {CQ::FATIGUE_ENERGY4_DURING_ENJOYABLE, {1, 3}},
    {CQ::SLEEP_LOSE2_DIS_WORST_DURATION, {1, 4}},
    {CQ::SLEEP_CAUSE, {1, 6}},
    {CQ::SLEEP_MAND2_GAIN_PAST_MONTH, {1, 3}},
    {CQ::SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT, {1, 4}},
    {CQ::IRRIT_MAND2_THINGS_PAST_MONTH, {1, 3}},
    {CQ::IRRIT3_WANTED_TO_SHOUT, {1, 3}},
    {CQ::IRRIT4_ARGUMENTS, {1, 3}},
    {CQ::DEPR_MAND2_ENJOYMENT_PAST_MONTH, {1, 3}},
    {CQ::DEPR2_ENJOYMENT_PAST_WEEK, {1, 3}},
    {CQ::DEPR5_COULD_CHEER_UP, {1, 3}},
    {CQ::DEPTH1_DIURNAL_VARIATION, {1, 4}},
    {CQ::DEPTH2_LIBIDO, {1, 4}},
    {CQ::DEPTH5_GUILT, {1, 4}},
    {CQ::DEPTH8_LNWL, {1, 3}},
    {CQ::DEPTH9_SUICIDE_THOUGHTS, {1, 3}},
    {CQ::DOCTOR, {1, 3}},
    {CQ::ANX_PHOBIA2_SPECIFIC_OR_GENERAL, {1, 2}},
    {CQ::PHOBIAS_TYPE1, {1, 9}},
    {CQ::PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK, {1, 3}},
    {CQ::PANIC_MAND_PAST_MONTH, {1, 3}},
    {CQ::PANIC1_NUM_PAST_WEEK, {1, 3}},
    {CQ::PANIC2_HOW_UNPLEASANT, {1, 3}},
    {CQ::PANIC3_PANIC_GE_10_MIN, {1, 2}},
    {CQ::COMP4_MAX_N_REPETITIONS, {1, 3}},
    {CQ::OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL, {1, 2}},
    {CQ::OBSESS4_MAX_DURATION, {1, 2}},
    {CQ::OVERALL2_IMPACT_PAST_WEEK, {1, 4}},
};
const QMap<CQ, QPair<int, int>> QUESTIONS_MULTIWAY_WITH_EXTRA_STEM{
    // Maps questions to first and last number of answers.
    {CQ::ETHNIC, {1, 7}},  // 7 includes our additional "prefer not to say"
    {CQ::MARRIED, {1, 6}},  // 6 includes our additional "prefer not to say"
    {CQ::EMPSTAT, {1, 8}},  // 8 includes our additional "prefer not to say"
    {CQ::EMPTYPE, {1, 7}},  // 7 includes our additional "not applicable" + "prefer not to say"
    {CQ::HOME, {1, 7}},  // 7 includes our additional "prefer not to say"
};
const QVector<CQ> QUESTIONS_DAYS_PER_WEEK{
    CQ::SOMATIC_PAIN2_DAYS_PAST_WEEK,
    CQ::SOMATIC_DIS2_DAYS_PAST_WEEK,
    CQ::FATIGUE_TIRED1_DAYS_PAST_WEEK,
    CQ::FATIGUE_ENERGY1_DAYS_PAST_WEEK,
    CQ::CONC1_CONC_DAYS_PAST_WEEK,
    CQ::IRRIT1_DAYS_PER_WEEK,
    CQ::HYPO1_DAYS_PAST_WEEK,
    CQ::DEPR3_DAYS_PAST_WEEK,
    CQ::WORRY2_DAYS_PAST_WEEK,
    CQ::ANX2_GENERAL_DAYS_PAST_WEEK,
    CQ::PHOBIAS1_DAYS_PAST_WEEK,
    // not this: CQ::PHOBIAS4_AVOIDANCE_FREQUENCY -- different phrasing
    // not this: CQ::PANIC1_FREQUENCY
    CQ::COMP1_DAYS_PAST_WEEK,
    CQ::OBSESS1_DAYS_PAST_WEEK,
};
const QVector<CQ> QUESTIONS_NIGHTS_PER_WEEK{
    CQ::SLEEP_LOSE1_NIGHTS_PAST_WEEK,
    CQ::SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK,
    CQ::SLEEP_GAIN1_NIGHTS_PAST_WEEK,   // (*) see below
        // (*) Probably an error in the original:
        // "On how many nights in the PAST SEVEN NIGHTS did you have problems
        // with your sleep? (1) None. (2) Between one and three days. (3) Four
        // days or more." Note day/night confusion. Altered to "nights".
    CQ::SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK,
};
const QVector<CQ> QUESTIONS_HOW_UNPLEASANT_STANDARD{
    CQ::SOMATIC_PAIN4_UNPLEASANT,
    CQ::SOMATIC_DIS4_UNPLEASANT,
    CQ::HYPO3_HOW_UNPLEASANT,
    CQ::WORRY4_HOW_UNPLEASANT,
    CQ::ANX3_GENERAL_HOW_UNPLEASANT,
};
const QVector<CQ> QUESTIONS_FATIGUE_CAUSES{
    CQ::FATIGUE_CAUSE1_TIRED,
    CQ::FATIGUE_CAUSE2_LACK_ENERGY,
};
const QVector<CQ> QUESTIONS_STRESSORS{
    CQ::DEPR_CONTENT,
    CQ::WORRY_CONT1,
};
const QVector<CQ> QUESTIONS_NO_SOMETIMES_OFTEN{
    CQ::WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH,
    CQ::ANX_MAND2_TENSION_PAST_MONTH,
    CQ::COMP_MAND1_COMPULSIONS_PAST_MONTH,
    CQ::OBSESS_MAND1_OBSESSIONS_PAST_MONTH,
    // and no-sometimes-often values also used by:
    //      CQ::PANIC_MAND_PAST_MONTH
    // ... but with variations on the text.
};

// Next, we define constants for all answers, particularly as values are not
// consistent (e.g. some questions have no 1/yes 2, and some have yes 1/no 2).
// Note: "LT" less than, "LE" less than or equal to, "GT" greater than, "GE"
// greater than or equal to.

// Number of response values (numbered from 1 to N)
// const int N_ETHNIC = 7;  // RNC: 6 in original; added "prefer not to say"
// const int N_MARRIED = 6;  // RNC: 5 in original; added "prefer not to say"
const int N_DURATIONS = 5;
const int N_OPTIONS_DAYS_PER_WEEK = 3;
const int N_OPTIONS_NIGHTS_PER_WEEK = 3;
const int N_OPTIONS_HOW_UNPLEASANT = 4;
const int N_OPTIONS_FATIGUE_CAUSES = 8;
const int N_OPTIONS_STRESSORS = 9;
const int N_OPTIONS_NO_SOMETIMES_OFTEN = 3;
const int NUM_PANIC_SYMPTOMS = 13;  // from a to m

// Actual response values

// For e.g. SOMATIC_DUR, etc.: "How long have you..."
// const int V_DURATION_LT_2W = 1;
const int V_DURATION_2W_6M = 2;
// const int V_DURATION_6M_1Y = 3;
// const int V_DURATION_1Y_2Y = 4;
// const int V_DURATION_GE_2Y = 5;

// For quite a few: "on how many days in the past week...?"
const int V_DAYS_IN_PAST_WEEK_0 = 1;
const int V_DAYS_IN_PAST_WEEK_1_TO_3 = 2;
const int V_DAYS_IN_PAST_WEEK_4_OR_MORE = 3;

const int V_NIGHTS_IN_PAST_WEEK_0 = 1;
// const int V_NIGHTS_IN_PAST_WEEK_1_TO_3 = 2;
const int V_NIGHTS_IN_PAST_WEEK_4_OR_MORE = 3;

// const int V_HOW_UNPLEASANT_NOT_AT_ALL = 1;
// const int V_HOW_UNPLEASANT_A_LITTLE = 2;
const int V_HOW_UNPLEASANT_UNPLEASANT = 3;
// const int V_HOW_UNPLEASANT_VERY = 4;

// const int V_FATIGUE_CAUSE_SLEEP = 1;
// const int V_FATIGUE_CAUSE_MEDICATION = 2;
// const int V_FATIGUE_CAUSE_PHYSICAL_ILLNESS = 3;
// const int V_FATIGUE_CAUSE_OVERWORK = 4;
// const int V_FATIGUE_CAUSE_PSYCHOLOGICAL = 5;
const int V_FATIGUE_CAUSE_EXERCISE = 6;
// const int V_FATIGUE_CAUSE_OTHER = 7;
// const int V_FATIGUE_CAUSE_DONT_KNOW = 8;

// const int V_STRESSOR_FAMILY = 1;
// const int V_STRESSOR_FRIENDS_COLLEAGUES = 2;
// const int V_STRESSOR_HOUSING = 3;
// const int V_STRESSOR_MONEY = 4;
// const int V_STRESSOR_PHYSICAL_HEALTH = 5;
// const int V_STRESSOR_MENTAL_HEALTH = 6;
// const int V_STRESSOR_WORK = 7;
// const int V_STRESSOR_LEGAL = 8;
// const int V_STRESSOR_POLITICAL_NEWS = 9;

const int V_NSO_NO = 1;
const int V_NSO_SOMETIMES = 2;
// const int V_NSO_OFTEN = 3;

const int V_SLEEP_CHANGE_LT_15_MIN = 1;
const int V_SLEEP_CHANGE_15_MIN_TO_1_H = 2;
const int V_SLEEP_CHANGE_1_TO_3_H = 3;
const int V_SLEEP_CHANGE_GT_3_H = 4;

const int V_ANHEDONIA_ENJOYING_NORMALLY = 1;
const int V_ANHEDONIA_ENJOYING_LESS = 2;
// const int V_ANHEDONIA_NOT_ENJOYING = 3;

// Specific other question values:

// const int V_EMPSTAT_FT = 1;
// const int V_EMPSTAT_PT = 2;
// const int V_EMPSTAT_STUDENT = 3;
// const int V_EMPSTAT_RETIRED = 4;
// const int V_EMPSTAT_HOUSEPERSON = 5;
// const int V_EMPSTAT_UNEMPJOBSEEKER = 6;
// const int V_EMPSTAT_UNEMPILLHEALTH = 7;

// const int V_EMPTYPE_SELFEMPWITHEMPLOYEES = 1;
// const int V_EMPTYPE_SELFEMPNOEMPLOYEES = 2;
// const int V_EMPTYPE_EMPLOYEE = 3;
// const int V_EMPTYPE_SUPERVISOR = 4;
// const int V_EMPTYPE_MANAGER = 5;
// const int V_EMPTYPE_NOT_APPLICABLE = 6;
// ... the last one: added by RNC, in case pt never employed. (Mentioned to
// Glyn Lewis 2017-12-04. Not, in any case, part of the important bits of the
// CIS-R.)

// const int V_HOME_OWNER = 1;
// const int V_HOME_TENANT = 2;
// const int V_HOME_RELATIVEFRIEND = 3;
// const int V_HOME_HOSTELCAREHOME = 4;
// const int V_HOME_HOMELESS = 5;
// const int V_HOME_OTHER = 6;

const int V_WEIGHT2_WTLOSS_NOTTRYING = 1;
const int V_WEIGHT2_WTLOSS_TRYING = 2;

const int V_WEIGHT3_WTLOSS_GE_HALF_STONE = 1;
// const int V_WEIGHT3_WTLOSS_LT_HALF_STONE = 2;

// const int V_WEIGHT4_WTGAIN_YES_PREGNANT = 3;

const int V_WEIGHT5_WTGAIN_GE_HALF_STONE = 1;
// const int V_WEIGHT5_WTGAIN_LT_HALF_STONE = 2;

// const int V_GPYEAR_NONE = 0;
// const int V_GPYEAR_1_2 = 1;
// const int V_GPYEAR_3_5 = 2;
// const int V_GPYEAR_6_10 = 3;
// const int V_GPYEAR_GT_10 = 4;

// const int V_ILLNESS_DIABETES = 1;
// const int V_ILLNESS_ASTHMA = 2;
// const int V_ILLNESS_ARTHRITIS = 3;
// const int V_ILLNESS_HEART_DISEASE = 4;
// const int V_ILLNESS_HYPERTENSION = 5;
// const int V_ILLNESS_LUNG_DISEASE = 6;
// const int V_ILLNESS_MORE_THAN_ONE = 7;
// const int V_ILLNESS_NONE = 8;

const int V_SOMATIC_PAIN1_NEVER = 1;
// const int V_SOMATIC_PAIN1_SOMETIMES = 2;
// const int V_SOMATIC_PAIN1_ALWAYS = 3;

// const int V_SOMATIC_PAIN3_LT_3H = 1;
// const int V_SOMATIC_PAIN3_GT_3H = 2;

// const int V_SOMATIC_PAIN4_NOT_AT_ALL = 1;
// const int V_SOMATIC_PAIN4_LITTLE_UNPLEASANT = 2;
// const int V_SOMATIC_PAIN4_UNPLEASANT = 3;
// const int V_SOMATIC_PAIN4_VERY_UNPLEASANT = 4;

// const int V_SOMATIC_PAIN5_NO = 1;
// const int V_SOMATIC_PAIN5_YES = 2;
// const int V_SOMATIC_PAIN5_NOT_DONE_ANYTHING_INTERESTING = 3;

// const int V_SOMATIC_MAND2_NO = 1;
// const int V_SOMATIC_MAND2_YES = 2;

const int V_SOMATIC_DIS1_NEVER = 1;
// const int V_SOMATIC_DIS1_SOMETIMES = 2;
// const int V_SOMATIC_DIS1_ALWAYS = 3;

// const int V_SOMATIC_DIS2_NONE = 1;
// const int V_SOMATIC_DIS2_1_TO_3_DAYS = 2;
// const int V_SOMATIC_DIS2_4_OR_MORE_DAYS = 3;

// const int V_SOMATIC_DIS3_LT_3H = 1;
// const int V_SOMATIC_DIS3_GT_3H = 2;

// const int V_SOMATIC_DIS4_NOT_AT_ALL = 1;
// const int V_SOMATIC_DIS4_LITTLE_UNPLEASANT = 2;
// const int V_SOMATIC_DIS4_UNPLEASANT = 3;
// const int V_SOMATIC_DIS4_VERY_UNPLEASANT = 4;

// const int V_SOMATIC_DIS5_NO = 1;
// const int V_SOMATIC_DIS5_YES = 2;
// const int V_SOMATIC_DIS5_NOT_DONE_ANYTHING_INTERESTING = 3;

const int V_SLEEP_MAND2_NO = 1;
const int V_SLEEP_MAND2_YES_BUT_NOT_A_PROBLEM = 2;
// const int V_SLEEP_MAND2_YES = 3;

const int V_IRRIT_MAND2_NO = 1;
// const int V_IRRIT_MAND2_SOMETIMES = 2;
// const int V_IRRIT_MAND2_YES = 3;

// const int V_IRRIT3_SHOUTING_NO = 1;
const int V_IRRIT3_SHOUTING_WANTED_TO = 2;
// const int V_IRRIT3_SHOUTING_DID = 3;

// const int V_IRRIT4_ARGUMENTS_NO = 1;
// const int V_IRRIT4_ARGUMENTS_YES_JUSTIFIED = 2;
const int V_IRRIT4_ARGUMENTS_YES_UNJUSTIFIED = 3;

// const int V_DEPR5_COULD_CHEER_UP_YES = 1;
const int V_DEPR5_COULD_CHEER_UP_SOMETIMES = 2;
// const int V_DEPR5_COULD_CHEER_UP_NO = 3;

const int V_DEPTH1_DMV_WORSE_MORNING = 1;
const int V_DEPTH1_DMV_WORSE_EVENING = 2;
// const int V_DEPTH1_DMV_VARIES = 3;
// const int V_DEPTH1_DMV_NONE = 4;

// const int V_DEPTH2_LIBIDO_NA = 1;
// const int V_DEPTH2_LIBIDO_NO_CHANGE = 2;
// const int V_DEPTH2_LIBIDO_INCREASED = 3;
const int V_DEPTH2_LIBIDO_DECREASED = 4;

// const int V_DEPTH5_GUILT_NEVER = 1;
// const int V_DEPTH5_GUILT_WHEN_AT_FAULT = 2;
const int V_DEPTH5_GUILT_SOMETIMES = 3;
// const int V_DEPTH5_GUILT_OFTEN = 4;

const int V_DEPTH8_LNWL_NO = 1;
const int V_DEPTH8_LNWL_SOMETIMES = 2;
// const int V_DEPTH8_LNWL_ALWAYS = 3;

const int V_DEPTH9_SUICIDAL_THOUGHTS_NO = 1;
const int V_DEPTH9_SUICIDAL_THOUGHTS_YES_BUT_NEVER_WOULD = 2;
const int V_DEPTH9_SUICIDAL_THOUGHTS_YES = 3;

const int V_DOCTOR_YES = 1;
// const int V_DOCTOR_NO_BUT_OTHERS = 2;
// const int V_DOCTOR_NO = 3;

const int V_ANX_PHOBIA2_ALWAYS_SPECIFIC = 1;
// const int V_ANX_PHOBIA2_SOMETIMES_GENERAL = 2;

const int V_PHOBIAS_TYPE1_ALONE_PUBLIC_TRANSPORT = 1;
const int V_PHOBIAS_TYPE1_FAR_FROM_HOME = 2;
const int V_PHOBIAS_TYPE1_PUBLIC_SPEAKING_EATING = 3;
const int V_PHOBIAS_TYPE1_BLOOD = 4;
const int V_PHOBIAS_TYPE1_CROWDED_SHOPS = 5;
const int V_PHOBIAS_TYPE1_ANIMALS = 6;
const int V_PHOBIAS_TYPE1_BEING_WATCHED = 7;
const int V_PHOBIAS_TYPE1_ENCLOSED_SPACES_HEIGHTS = 8;
const int V_PHOBIAS_TYPE1_OTHER = 9;

const int V_PANIC1_N_PANICS_PAST_WEEK_0 = 1;
const int V_PANIC1_N_PANICS_PAST_WEEK_1 = 2;
const int V_PANIC1_N_PANICS_PAST_WEEK_GT_1 = 3;

// const int V_PANIC3_WORST_LT_10_MIN = 1;
const int V_PANIC3_WORST_GE_10_MIN = 2;

// const int V_COMP4_MAX_N_REPEATS_1 = 1;
// const int V_COMP4_MAX_N_REPEATS_2 = 2;
const int V_COMP4_MAX_N_REPEATS_GE_3 = 3;

// const int V_OBSESS_MAND1_SAME_THOUGHTS_REPEATED = 1;
const int V_OBSESS_MAND1_GENERAL_WORRIES = 2;

// const int V_OBSESS4_LT_15_MIN = 1;
const int V_OBSESS4_GE_15_MIN = 2;

// const int V_OVERALL_IMPAIRMENT_NONE = 1;
// const int V_OVERALL_IMPAIRMENT_DIFFICULT = 2;
// const int V_OVERALL_IMPAIRMENT_STOP_1_ACTIVITY = 3;
// const int V_OVERALL_IMPAIRMENT_STOP_GT_1_ACTIVITY = 4;

// Scoring constants:

const int MAX_TOTAL = 57;

const int MAX_SOMATIC = 4;
const int MAX_HYPO = 4;
const int MAX_IRRIT = 4;
const int MAX_CONC = 4;
const int MAX_FATIGUE = 4;
const int MAX_SLEEP = 4;
const int MAX_DEPR = 4;
const int MAX_DEPTHTS = 5;
const int MAX_PHOBIAS = 4;
const int MAX_WORRY = 4;
const int MAX_ANX = 4;
const int MAX_PANIC = 4;
const int MAX_COMP = 4;
const int MAX_OBSESS = 4;
// const int MAX_DEPCRIT1 = 3;
// const int MAX_DEPCRIT2 = 7;
// const int MAX_DEPCRIT3 = 8;


// ============================================================================
// Task registration
// ============================================================================

void initializeCisr(TaskFactory& factory)
{
    static TaskRegistrar<Cisr> registered(factory);
}


// ============================================================================
// Constructor
// ============================================================================

Cisr::Cisr(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CISR_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    const QStringList fieldnames{
        FN_ETHNIC,
        FN_MARRIED,
        FN_EMPSTAT,
        FN_EMPTYPE,
        FN_HOME,

        FN_APPETITE1,
        FN_WEIGHT1,
        FN_WEIGHT2,
        FN_WEIGHT3,
        FN_APPETITE2,
        FN_WEIGHT4,
        FN_WEIGHT5,

        FN_GP_YEAR,
        FN_DISABLE,
        FN_ILLNESS,

        FN_SOMATIC_MAND1,
        FN_SOMATIC_PAIN1,
        FN_SOMATIC_PAIN2,
        FN_SOMATIC_PAIN3,
        FN_SOMATIC_PAIN4,
        FN_SOMATIC_PAIN5,
        FN_SOMATIC_MAND2,
        FN_SOMATIC_DIS1,
        FN_SOMATIC_DIS2,
        FN_SOMATIC_DIS3,
        FN_SOMATIC_DIS4,
        FN_SOMATIC_DIS5,
        FN_SOMATIC_DUR,

        FN_FATIGUE_MAND1,
        FN_FATIGUE_CAUSE1,
        FN_FATIGUE_TIRED1,
        FN_FATIGUE_TIRED2,
        FN_FATIGUE_TIRED3,
        FN_FATIGUE_TIRED4,
        FN_FATIGUE_MAND2,
        FN_FATIGUE_CAUSE2,
        FN_FATIGUE_ENERGY1,
        FN_FATIGUE_ENERGY2,
        FN_FATIGUE_ENERGY3,
        FN_FATIGUE_ENERGY4,
        FN_FATIGUE_DUR,

        FN_CONC_MAND1,
        FN_CONC_MAND2,
        FN_CONC1,
        FN_CONC2,
        FN_CONC3,
        FN_CONC_DUR,
        FN_CONC4,
        FN_FORGET_DUR,

        FN_SLEEP_MAND1,
        FN_SLEEP_LOSE1,
        FN_SLEEP_LOSE2,
        FN_SLEEP_LOSE3,
        FN_SLEEP_EMW,
        FN_SLEEP_CAUSE,
        FN_SLEEP_MAND2,
        FN_SLEEP_GAIN1,
        FN_SLEEP_GAIN2,
        FN_SLEEP_GAIN3,
        FN_SLEEP_DUR,

        FN_IRRIT_MAND1,
        FN_IRRIT_MAND2,
        FN_IRRIT1,
        FN_IRRIT2,
        FN_IRRIT3,
        FN_IRRIT4,
        FN_IRRIT_DUR,

        FN_HYPO_MAND1,
        FN_HYPO_MAND2,
        FN_HYPO1,
        FN_HYPO2,
        FN_HYPO3,
        FN_HYPO4,
        FN_HYPO_DUR,

        FN_DEPR_MAND1,
        FN_DEPR1,
        FN_DEPR_MAND2,
        FN_DEPR2,
        FN_DEPR3,
        FN_DEPR4,
        FN_DEPR_CONTENT,
        FN_DEPR5,
        FN_DEPR_DUR,
        FN_DEPTH1,
        FN_DEPTH2,
        FN_DEPTH3,
        FN_DEPTH4,
        FN_DEPTH5,
        FN_DEPTH6,
        FN_DEPTH7,
        FN_DEPTH8,
        FN_DEPTH9,
        FN_DEPTH10,
        FN_DOCTOR,

        FN_WORRY_MAND1,
        FN_WORRY_MAND2,
        FN_WORRY_CONT1,
        FN_WORRY2,
        FN_WORRY3,
        FN_WORRY4,
        FN_WORRY5,
        FN_WORRY_DUR,

        FN_ANX_MAND1,
        FN_ANX_MAND2,
        FN_ANX_PHOBIA1,
        FN_ANX_PHOBIA2,
        FN_ANX2,
        FN_ANX3,
        FN_ANX4,
        FN_ANX5,
        FN_ANX_DUR,

        FN_PHOBIAS_MAND,
        FN_PHOBIAS_TYPE1,
        FN_PHOBIAS1,
        FN_PHOBIAS2,
        FN_PHOBIAS3,
        FN_PHOBIAS4,
        FN_PHOBIAS_DUR,

        FN_PANIC_MAND,
        FN_PANIC1,
        FN_PANIC2,
        FN_PANIC3,
        FN_PANIC4,
        FN_PANSYM_A,
        FN_PANSYM_B,
        FN_PANSYM_C,
        FN_PANSYM_D,
        FN_PANSYM_E,
        FN_PANSYM_F,
        FN_PANSYM_G,
        FN_PANSYM_H,
        FN_PANSYM_I,
        FN_PANSYM_J,
        FN_PANSYM_K,
        FN_PANSYM_L,
        FN_PANSYM_M,
        FN_PANIC5,
        FN_PANIC_DUR,

        FN_COMP_MAND1,
        FN_COMP1,
        FN_COMP2,
        FN_COMP3,
        FN_COMP4,
        FN_COMP_DUR,

        FN_OBSESS_MAND1,
        FN_OBSESS_MAND2,
        FN_OBSESS1,
        FN_OBSESS2,
        FN_OBSESS3,
        FN_OBSESS4,
        FN_OBSESS_DUR,

        FN_OVERALL2,
    };
    for (const QString& fn : fieldnames) {
        addField(fn, QVariant::Int);
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Cisr::shortname() const
{
    return "CIS-R";
}


QString Cisr::longname() const
{
    return tr("Clinical Interview Schedule – Revised");
}


QString Cisr::description() const
{
    return tr("Structured diagnostic interview, yielding ICD-10 diagnoses for "
              "depressive and anxiety disorders.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Cisr::isComplete() const
{
    CisrResult result = getResult();
    return !result.incomplete;
}


QStringList Cisr::summary() const
{
    CisrResult result = getResult();
    return summaryForResult(result);
}


QStringList Cisr::detail() const
{
    const QString decision_prefix = "+++ ";
    QStringList lines;
    CisrResult result = getResult();
    if (result.incomplete) {
        lines.append(incompleteMarker());
    }
    lines += summaryForResult(result);
    lines.append("");  // blank line
    lines += recordSummaryLines();
    lines.append("");  // blank line
    for (const QString& decision : result.decisions) {
        lines.append(decision_prefix + decision.toHtmlEscaped());
    }
    return lines;
}


OpenableWidget* Cisr::editor(const bool read_only)
{
    m_questionnaire = new DynamicQuestionnaire(
                m_app,
                std::bind(&Cisr::makePage, this, std::placeholders::_1),
                std::bind(&Cisr::morePagesToGo, this, std::placeholders::_1));
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);
    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QStringList Cisr::summaryForResult(const Cisr::CisrResult& result) const
{
    // Used so that we don't recalculate results again and again!
    using mathfunc::scorePhrase;
    using mathfunc::totalScorePhrase;
    using stringfunc::bold;

    QStringList lines;

    auto addLine = [&lines](const QString& q, const QString& a) -> void {
        const QString line = QString("%1: %2.").arg(q, bold(a));
        lines.append(line);
    };
    auto addScore = [this, &lines](const QString& xstringname, const int score,
                                   const int max_score) -> void {
        const QString line = scorePhrase(xstring(xstringname),
                                         score, max_score);
        lines.append(line);
    };

    if (!result.incomplete) {
        addLine("Probable primary diagnosis",
                result.diagnosisName(result.diagnosis_1));
        addLine("Probable secondary diagnosis",
                result.diagnosisName(result.diagnosis_1));

        lines.append(totalScorePhrase(result.getScore(), MAX_TOTAL));
        addLine(xstring("impair_label"), functionalImpairment(result));

        addScore("somatic_label", result.somatic_symptoms, MAX_SOMATIC);
        addScore("hypo_label", result.hypochondria, MAX_HYPO);
        addScore("irrit_label", result.irritability, MAX_IRRIT);
        addScore("conc_label", result.concentration_poor, MAX_CONC);
        addScore("fatigue_label", result.fatigue, MAX_FATIGUE);
        addScore("sleep_label", result.sleep_problems, MAX_SLEEP);
        addScore("depr_label", result.depression, MAX_DEPR);
        addScore("depthts_label", result.depressive_thoughts, MAX_DEPTHTS);
        addScore("somatic_label", result.somatic_symptoms, MAX_SOMATIC);
        addScore("phobias_label", result.phobias_score, MAX_PHOBIAS);
        addScore("worry_label", result.worry, MAX_WORRY);
        addScore("anx_label", result.anxiety, MAX_ANX);
        addScore("panic_label", result.panic, MAX_PANIC);
        addScore("comp_label", result.compulsions, MAX_COMP);
        addScore("obsess_label", result.obsessions, MAX_OBSESS);
    }
    lines.append(QString("CIS-R suicide intent: %1.").arg(
                     suicideIntent(result)));
    return lines;
}


// ============================================================================
// DynamicQuestionnaire callbacks
// ============================================================================

QuPagePtr Cisr::makePage(const int current_qnum)
{
    // current_qnum is zero-based.
    // Return nullptr to finish.
    CisrQuestion q = getPageEnum(current_qnum);
    return makePageFromEnum(q);
}


bool Cisr::morePagesToGo(const int current_qnum) const
{
    CisrQuestion q = getPageEnum(current_qnum + 1);
    return q != CQ::END_MARKER;
}


// ============================================================================
// Internals to build questionnaires
// ============================================================================
// For methods of enum iteration, see also
// - https://stackoverflow.com/questions/1390703/enumerate-over-an-enum-in-c
// - https://stackoverflow.com/questions/8357240/how-to-automatically-convert-strongly-typed-enum-into-int

Cisr::CisrQuestion Cisr::intToEnum(int qi) const
{
    auto start = static_cast<int>(CisrQuestion::START_MARKER);
    auto end = static_cast<int>(CisrQuestion::END_MARKER);
    Q_ASSERT(qi >= start && qi <= end);
    return static_cast<CisrQuestion>(qi);
}


int Cisr::enumToInt(CisrQuestion qe) const
{
    return static_cast<int>(qe);
}


QString Cisr::fieldnameForQuestion(CisrQuestion q) const
{
    switch (q) {
    // case CQ::INTRO_1:  // information only
    // case CQ::INTRO_2:  // information only
    // case CQ::INTRO_DEMOGRAPHICS:  // information only

    case CQ::ETHNIC:    return FN_ETHNIC;
    case CQ::MARRIED:   return FN_MARRIED;
    case CQ::EMPSTAT:   return FN_EMPSTAT;
    case CQ::EMPTYPE:   return FN_EMPTYPE;
    case CQ::HOME:      return FN_HOME;

    // case CQ::HEALTH_WELLBEING:  // information only

    case CQ::APPETITE1_LOSS_PAST_MONTH:     return FN_APPETITE1;
    case CQ::WEIGHT1_LOSS_PAST_MONTH:       return FN_WEIGHT1;
    case CQ::WEIGHT2_TRYING_TO_LOSE:        return FN_WEIGHT2;
    case CQ::WEIGHT3_LOST_LOTS:             return FN_WEIGHT3;
    case CQ::APPETITE2_INCREASE_PAST_MONTH: return FN_APPETITE2;
    case CQ::WEIGHT4_INCREASE_PAST_MONTH:   return FN_WEIGHT4;
    // case CQ::WEIGHT4A: not used (= WEIGHT4 + pregnancy option)
    case CQ::WEIGHT5_GAINED_LOTS:           return FN_WEIGHT5;
    case CQ::GP_YEAR:                       return FN_GP_YEAR;
    case CQ::DISABLE:                       return FN_DISABLE;
    case CQ::ILLNESS:                       return FN_ILLNESS;

    case CQ::SOMATIC_MAND1_PAIN_PAST_MONTH:         return FN_SOMATIC_MAND1;
    case CQ::SOMATIC_PAIN1_PSYCHOL_EXAC:            return FN_SOMATIC_PAIN1;
    case CQ::SOMATIC_PAIN2_DAYS_PAST_WEEK:          return FN_SOMATIC_PAIN2;
    case CQ::SOMATIC_PAIN3_GT_3H_ANY_DAY:           return FN_SOMATIC_PAIN3;
    case CQ::SOMATIC_PAIN4_UNPLEASANT:              return FN_SOMATIC_PAIN4;
    case CQ::SOMATIC_PAIN5_INTERRUPTED_INTERESTING: return FN_SOMATIC_PAIN5;
    case CQ::SOMATIC_MAND2_DISCOMFORT:              return FN_SOMATIC_MAND2;
    case CQ::SOMATIC_DIS1_PSYCHOL_EXAC:             return FN_SOMATIC_DIS1;
    case CQ::SOMATIC_DIS2_DAYS_PAST_WEEK:           return FN_SOMATIC_DIS2;
    case CQ::SOMATIC_DIS3_GT_3H_ANY_DAY:            return FN_SOMATIC_DIS3;
    case CQ::SOMATIC_DIS4_UNPLEASANT:               return FN_SOMATIC_DIS4;
    case CQ::SOMATIC_DIS5_INTERRUPTED_INTERESTING:  return FN_SOMATIC_DIS5;
    case CQ::SOMATIC_DUR:                           return FN_SOMATIC_DUR;

    case CQ::FATIGUE_MAND1_TIRED_PAST_MONTH:        return FN_FATIGUE_MAND1;
    case CQ::FATIGUE_CAUSE1_TIRED:                  return FN_FATIGUE_CAUSE1;
    case CQ::FATIGUE_TIRED1_DAYS_PAST_WEEK:         return FN_FATIGUE_TIRED1;
    case CQ::FATIGUE_TIRED2_GT_3H_ANY_DAY:          return FN_FATIGUE_TIRED2;
    case CQ::FATIGUE_TIRED3_HAD_TO_PUSH:            return FN_FATIGUE_TIRED3;
    case CQ::FATIGUE_TIRED4_DURING_ENJOYABLE:       return FN_FATIGUE_TIRED4;
    case CQ::FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH:  return FN_FATIGUE_MAND2;
    case CQ::FATIGUE_CAUSE2_LACK_ENERGY:            return FN_FATIGUE_CAUSE2;
    case CQ::FATIGUE_ENERGY1_DAYS_PAST_WEEK:        return FN_FATIGUE_ENERGY1;
    case CQ::FATIGUE_ENERGY2_GT_3H_ANY_DAY:         return FN_FATIGUE_ENERGY2;
    case CQ::FATIGUE_ENERGY3_HAD_TO_PUSH:           return FN_FATIGUE_ENERGY3;
    case CQ::FATIGUE_ENERGY4_DURING_ENJOYABLE:      return FN_FATIGUE_ENERGY4;
    case CQ::FATIGUE_DUR:                           return FN_FATIGUE_DUR;

    case CQ::CONC_MAND1_POOR_CONC_PAST_MONTH:           return FN_CONC_MAND1;
    case CQ::CONC_MAND2_FORGETFUL_PAST_MONTH:           return FN_CONC_MAND2;
    case CQ::CONC1_CONC_DAYS_PAST_WEEK:                 return FN_CONC1;
    case CQ::CONC2_CONC_FOR_TV_READING_CONVERSATION:    return FN_CONC2;
    case CQ::CONC3_CONC_PREVENTED_ACTIVITIES:           return FN_CONC3;
    case CQ::CONC_DUR:                                  return FN_CONC_DUR;
    case CQ::CONC4_FORGOTTEN_IMPORTANT:                 return FN_CONC4;
    case CQ::FORGET_DUR:                                return FN_FORGET_DUR;

    case CQ::SLEEP_MAND1_LOSS_PAST_MONTH:               return FN_SLEEP_MAND1;
    case CQ::SLEEP_LOSE1_NIGHTS_PAST_WEEK:              return FN_SLEEP_LOSE1;
    case CQ::SLEEP_LOSE2_DIS_WORST_DURATION:            return FN_SLEEP_LOSE2;
    case CQ::SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK:    return FN_SLEEP_LOSE3;
    case CQ::SLEEP_EMW_PAST_WEEK:                       return FN_SLEEP_EMW;
    case CQ::SLEEP_CAUSE:                               return FN_SLEEP_CAUSE;
    case CQ::SLEEP_MAND2_GAIN_PAST_MONTH:               return FN_SLEEP_MAND2;
    case CQ::SLEEP_GAIN1_NIGHTS_PAST_WEEK:              return FN_SLEEP_GAIN1;
    case CQ::SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT:        return FN_SLEEP_GAIN2;
    case CQ::SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK:  return FN_SLEEP_GAIN3;
    case CQ::SLEEP_DUR:                                 return FN_SLEEP_DUR;

    case CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH: return FN_IRRIT_MAND1;
    case CQ::IRRIT_MAND2_THINGS_PAST_MONTH: return FN_IRRIT_MAND2;
    case CQ::IRRIT1_DAYS_PER_WEEK:          return FN_IRRIT1;
    case CQ::IRRIT2_GT_1H_ANY_DAY:          return FN_IRRIT2;
    case CQ::IRRIT3_WANTED_TO_SHOUT:        return FN_IRRIT3;
    case CQ::IRRIT4_ARGUMENTS:              return FN_IRRIT4;
    case CQ::IRRIT_DUR:                     return FN_IRRIT_DUR;

    case CQ::HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH:   return FN_HYPO_MAND1;
    case CQ::HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS:     return FN_HYPO_MAND2;
    case CQ::HYPO1_DAYS_PAST_WEEK:                      return FN_HYPO1;
    case CQ::HYPO2_WORRY_TOO_MUCH:                      return FN_HYPO2;
    case CQ::HYPO3_HOW_UNPLEASANT:                      return FN_HYPO3;
    case CQ::HYPO4_CAN_DISTRACT:                        return FN_HYPO4;
    case CQ::HYPO_DUR:                                  return FN_HYPO_DUR;

    case CQ::DEPR_MAND1_LOW_MOOD_PAST_MONTH:    return FN_DEPR_MAND1;
    case CQ::DEPR1_LOW_MOOD_PAST_WEEK:          return FN_DEPR1;
    case CQ::DEPR_MAND2_ENJOYMENT_PAST_MONTH:   return FN_DEPR_MAND2;
    case CQ::DEPR2_ENJOYMENT_PAST_WEEK:         return FN_DEPR2;
    case CQ::DEPR3_DAYS_PAST_WEEK:              return FN_DEPR3;
    case CQ::DEPR4_GT_3H_ANY_DAY:               return FN_DEPR4;
    case CQ::DEPR_CONTENT:                      return FN_DEPR_CONTENT;
    case CQ::DEPR5_COULD_CHEER_UP:              return FN_DEPR5;
    case CQ::DEPR_DUR:                          return FN_DEPR_DUR;
    case CQ::DEPTH1_DIURNAL_VARIATION:          return FN_DEPTH1;
    case CQ::DEPTH2_LIBIDO:                     return FN_DEPTH2;
    case CQ::DEPTH3_RESTLESS:                   return FN_DEPTH3;
    case CQ::DEPTH4_SLOWED:                     return FN_DEPTH4;
    case CQ::DEPTH5_GUILT:                      return FN_DEPTH5;
    case CQ::DEPTH6_WORSE_THAN_OTHERS:          return FN_DEPTH6;
    case CQ::DEPTH7_HOPELESS:                   return FN_DEPTH7;
    case CQ::DEPTH8_LNWL:                       return FN_DEPTH8;
    case CQ::DEPTH9_SUICIDE_THOUGHTS:           return FN_DEPTH9;
    case CQ::DEPTH10_SUICIDE_METHOD:            return FN_DEPTH10;
    case CQ::DOCTOR:                            return FN_DOCTOR;
    // case CQ::DOCTOR2_PLEASE_TALK_TO:  // info only
    // case CQ::DEPR_OUTRO:  // info only

    case CQ::WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH:   return FN_WORRY_MAND1;
    case CQ::WORRY_MAND2_ANY_WORRIES_PAST_MONTH:        return FN_WORRY_MAND2;
    case CQ::WORRY_CONT1:                               return FN_WORRY_CONT1;
    // case CQ::WORRY1_INFO_ONLY:  // info only
    case CQ::WORRY2_DAYS_PAST_WEEK:                     return FN_WORRY2;
    case CQ::WORRY3_TOO_MUCH:                           return FN_WORRY3;
    case CQ::WORRY4_HOW_UNPLEASANT:                     return FN_WORRY4;
    case CQ::WORRY5_GT_3H_ANY_DAY:                      return FN_WORRY5;
    case CQ::WORRY_DUR:                                 return FN_WORRY_DUR;

    case CQ::ANX_MAND1_ANXIETY_PAST_MONTH:      return FN_ANX_MAND1;
    case CQ::ANX_MAND2_TENSION_PAST_MONTH:      return FN_ANX_MAND2;
    case CQ::ANX_PHOBIA1_SPECIFIC_PAST_MONTH:   return FN_ANX_PHOBIA1;
    case CQ::ANX_PHOBIA2_SPECIFIC_OR_GENERAL:   return FN_ANX_PHOBIA2;
    // case CQ::ANX1_INFO_ONLY:  // info only
    case CQ::ANX2_GENERAL_DAYS_PAST_WEEK:       return FN_ANX2;
    case CQ::ANX3_GENERAL_HOW_UNPLEASANT:       return FN_ANX3;
    case CQ::ANX4_GENERAL_PHYSICAL_SYMPTOMS:    return FN_ANX4;
    case CQ::ANX5_GENERAL_GT_3H_ANY_DAY:        return FN_ANX5;
    case CQ::ANX_DUR_GENERAL:                   return FN_ANX_DUR;

    case CQ::PHOBIAS_MAND_AVOIDANCE_PAST_MONTH: return FN_PHOBIAS_MAND;
    case CQ::PHOBIAS_TYPE1:                     return FN_PHOBIAS_TYPE1;
    case CQ::PHOBIAS1_DAYS_PAST_WEEK:           return FN_PHOBIAS1;
    case CQ::PHOBIAS2_PHYSICAL_SYMPTOMS:        return FN_PHOBIAS2;
    case CQ::PHOBIAS3_AVOIDANCE:                return FN_PHOBIAS3;
    case CQ::PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK: return FN_PHOBIAS4;
    case CQ::PHOBIAS_DUR:                       return FN_PHOBIAS_DUR;

    case CQ::PANIC_MAND_PAST_MONTH:             return FN_PANIC_MAND;
    case CQ::PANIC1_NUM_PAST_WEEK:              return FN_PANIC1;
    case CQ::PANIC2_HOW_UNPLEASANT:             return FN_PANIC2;
    case CQ::PANIC3_PANIC_GE_10_MIN:            return FN_PANIC3;
    case CQ::PANIC4_RAPID_ONSET:                return FN_PANIC4;
    // case CQ::PANSYM:  // multiple stems
    case CQ::PANIC5_ALWAYS_SPECIFIC_TRIGGER:    return FN_PANIC5;
    case CQ::PANIC_DUR:                 return FN_PANIC_DUR;

    // case CQ::ANX_OUTRO:  // info only

    case CQ::COMP_MAND1_COMPULSIONS_PAST_MONTH: return FN_COMP_MAND1;
    case CQ::COMP1_DAYS_PAST_WEEK:              return FN_COMP1;
    case CQ::COMP2_TRIED_TO_STOP:               return FN_COMP2;
    case CQ::COMP3_UPSETTING:                   return FN_COMP3;
    case CQ::COMP4_MAX_N_REPETITIONS:           return FN_COMP4;
    case CQ::COMP_DUR:                          return FN_COMP_DUR;

    case CQ::OBSESS_MAND1_OBSESSIONS_PAST_MONTH:    return FN_OBSESS_MAND1;
    case CQ::OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL: return FN_OBSESS_MAND2;
    case CQ::OBSESS1_DAYS_PAST_WEEK:                return FN_OBSESS1;
    case CQ::OBSESS2_TRIED_TO_STOP:                 return FN_OBSESS2;
    case CQ::OBSESS3_UPSETTING:                     return FN_OBSESS3;
    case CQ::OBSESS4_MAX_DURATION:                  return FN_OBSESS4;
    case CQ::OBSESS_DUR:                            return FN_OBSESS_DUR;

    // case CQ::OVERALL1:  // info only
    case CQ::OVERALL2_IMPACT_PAST_WEEK:     return FN_OVERALL2;

    default:  // e.g. for info-only fields
        return "";
    }
}


QString Cisr::tagForQuestion(CisrQuestion q) const
{
    const QString fieldname = fieldnameForQuestion(q);
    if (!fieldname.isEmpty()) {
        return fieldname.toUpper();
    }
    switch (q) {
    // Aim to be relatively cryptic; use the original CIS-R tags, not our
    // expanded explanatory versions (in case anyone uses the debug version for
    // patient testing!).
    case CQ::INTRO_1:                   return "INTRO_1";
    case CQ::INTRO_2:                   return "INTRO_2";
    case CQ::INTRO_DEMOGRAPHICS:        return "INTRO_DEMOGRAPHICS";
    case CQ::HEALTH_WELLBEING:          return "HEALTH_WELLBEING";
    case CQ::DOCTOR2_PLEASE_TALK_TO:    return "DOCTOR2";
    case CQ::DEPR_OUTRO:                return "DEPR_OUTRO";
    case CQ::WORRY1_INFO_ONLY:          return "WORRY1";
    case CQ::ANX1_INFO_ONLY:            return "ANX1";
    case CQ::PANSYM:                    return "PANSYM";
    case CQ::ANX_OUTRO:                 return "ANX_OUTRO";
    case CQ::OVERALL1_INFO_ONLY:        return "OVERALL1";
    case CQ::THANKS_FINISHED:           return "THANKS_FINISHED";
    case CQ::END_MARKER:                return "END_MARKER";
    default:                            return "UNKNOWN_TAG";
    }
}


QVariant Cisr::valueForQuestion(CisrQuestion q) const
{
    const QString fieldname = fieldnameForQuestion(q);
    Q_ASSERT(!fieldname.isEmpty());  // have we asked for a field from an info-only page?
    return value(fieldname);
}


int Cisr::intValueForQuestion(CisrQuestion q) const
{
    return valueForQuestion(q).toInt();
}


bool Cisr::answerIsNo(CisrQuestion q, int value) const
{
    if (value == V_UNKNOWN) {  // "Please look it up for me"
        value = intValueForQuestion(q);
    }
    if (QUESTIONS_1_NO_2_YES.contains(q)) {
        return value == 1;
    }
    if (QUESTIONS_1_YES_2_NO.contains(q)) {
        return value == 2;
    }
    qCritical() << "answerIsNo() called for inappropriate question"
                << enumToInt(q) << tagForQuestion(q);
    return false;
}


bool Cisr::answerIsYes(CisrQuestion q, int value) const
{
    if (value == V_UNKNOWN) {  // "Please look it up for me"
        value = intValueForQuestion(q);
    }
    if (QUESTIONS_1_NO_2_YES.contains(q)) {
        return value == 2;
    }
    if (QUESTIONS_1_YES_2_NO.contains(q)) {
        return value == 1;
    }
    qCritical() << "answerIsYes() called for inappropriate question"
                << enumToInt(q) << tagForQuestion(q);
    return false;
}


bool Cisr::answered(CisrQuestion q, int value) const
{
    if (value == V_UNKNOWN) {  // "Please look it up for me"
        value = intValueForQuestion(q);
    }
    return value != V_MISSING;
}


Cisr::CisrQuestion Cisr::nextQ(Cisr::CisrQuestion q, Cisr::CisrResult& r) const
{
    // ANY CHANGES HERE MUST BE REFLECTED IN THE PYTHON CODE AND VICE VERSA.

    // 1. Returns the next question to be offered.
    // 2. Scores as it goes, also flagging up if the data are incomplete.
    // Rationale: we have to apply a sequential logic to test for completeness
    // and to determine the next question in a sequence, so we may as well
    // score at the same time (very low overhead).
    // Also, this method allows direct comparison with the original CIS-R
    // code.

    auto chooseFinalPage = [&r]() -> Cisr::CisrQuestion {
        // If there have been significant symptoms:
        //      (1) OVERALL1_INFO_ONLY - "Thank you for answering..."
        //      (2) OVERALL2_IMPACT_PAST_WEEK - "What's the impact been?"
        //      (3) THANKS_FINISHED - "All done."
        // Otherwise:
        //      (1) THANKS_FINISHED - "All done."
        return r.needsImpairmentQuestion() ? CQ::OVERALL1_INFO_ONLY
                                           : CQ::THANKS_FINISHED;
    };

    QVariant var_q;
    int v = V_MISSING;
#ifdef SHOW_QUESTIONS_CONSIDERED
    r.decide(QString("Considering question %1: %2").arg(
                 QString::number(enumToInt(q)), tagForQuestion(q)));
#endif
    const QString fieldname = fieldnameForQuestion(q);
    if (!fieldname.isEmpty()) {  // eliminates prompt-only questions
        var_q = value(fieldname);
        if (var_q.isNull()) {
            if (!QUESTIONS_DEMOGRAPHICS.contains(q)) {
                // From a diagnostic point of view, OK to have missing
                // demographic information. Otherwise:
                r.decide("INCOMPLETE INFORMATION. STOPPING.");
                r.incomplete = true;
            }
        } else {
            v = var_q.toInt();
        }
    }

    int next_q = -1;
    auto jumpTo = [this, &next_q](CisrQuestion qe) {
        next_q = enumToInt(qe);
    };
    // If there is no special handling for a question, then after the
    // switch() statement we will move to the next question in sequence.
    // So only special "skip" situations are handled here.

    switch (q) {

    // FOLLOW THE EXACT SEQUENCE of the CIS-R. Don't agglomerate case
    // statements just because it's shorter (except empty ones when they are
    // in sequence). Clarity is key.

    // --------------------------------------------------------------------
    // Demographics/preamble
    // --------------------------------------------------------------------

    case CQ::INTRO_1:
    case CQ::INTRO_2:
    case CQ::INTRO_DEMOGRAPHICS:
    case CQ::ETHNIC:
    case CQ::MARRIED:
    case CQ::EMPSTAT:
    case CQ::EMPTYPE:
    case CQ::HOME:
    case CQ::HEALTH_WELLBEING:
        // Nothing special
        break;

    // --------------------------------------------------------------------
    // Appetite/weight
    // --------------------------------------------------------------------

    case CQ::APPETITE1_LOSS_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("No loss of appetite in past month.");
            jumpTo(CQ::APPETITE2_INCREASE_PAST_MONTH);
        } else if (answerIsYes(q, v)) {
            r.decide("Loss of appetite in past month. "
                     "Incrementing depr_crit_3_somatic_synd.");
            r.depr_crit_3_somatic_synd += 1;
            r.weight_change = WTCHANGE_APPETITE_LOSS;
        }
        break;

    case CQ::WEIGHT1_LOSS_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("No weight loss.");
            jumpTo(CQ::GP_YEAR);
        }
        break;

    case CQ::WEIGHT2_TRYING_TO_LOSE:
        if (v == V_WEIGHT2_WTLOSS_TRYING) {
            // Trying to lose weight. Move on.
            r.decide("Weight loss but it was deliberate.");
        } else if (v == V_WEIGHT2_WTLOSS_NOTTRYING) {
            r.decide("Non-deliberate weight loss.");
            r.weight_change = WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN;
        }
        break;

    case CQ::WEIGHT3_LOST_LOTS:
        if (v == V_WEIGHT3_WTLOSS_GE_HALF_STONE) {
            r.decide("Weight loss >=0.5st in past month. "
                     "Incrementing depr_crit_3_somatic_synd.");
            r.weight_change = WTCHANGE_WTLOSS_GE_HALF_STONE;
            r.depr_crit_3_somatic_synd += 1;
        }
        r.decide("Loss of weight, so skipping appetite/weight gain questions.");
        jumpTo(CQ::GP_YEAR);
        break;

    case CQ::APPETITE2_INCREASE_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("No increase in appetite in past month.");
            jumpTo(CQ::GP_YEAR);
        }
        break;

    case CQ::WEIGHT4_INCREASE_PAST_MONTH:
        if (answerIsYes(q, v)) {
            r.decide("Weight gain.");
            r.weight_change = WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN;
        } else if (answered(q, v)) {
            r.decide("No weight gain, or weight gain but pregnant.");
            jumpTo(CQ::GP_YEAR);
        }
        break;

    case CQ::WEIGHT5_GAINED_LOTS:
        if (v == V_WEIGHT5_WTGAIN_GE_HALF_STONE &&
                r.weight_change == WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN) {
            // ... redundant check on weight_change, I think!
            r.decide("Weight gain >=0.5 st in past month.");
            r.weight_change = WTCHANGE_WTGAIN_GE_HALF_STONE;
        }
        break;

    // --------------------------------------------------------------------
    // Somatic symptoms
    // --------------------------------------------------------------------

    case CQ::GP_YEAR:
        // Score the preceding block:
        if (r.weight_change == WTCHANGE_WTLOSS_GE_HALF_STONE &&
                answerIsYes(CQ::APPETITE1_LOSS_PAST_MONTH)) {
            r.decide("Appetite loss and weight loss >=0.5st in past month. "
                     "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.");
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
        }
        if (r.weight_change == WTCHANGE_WTGAIN_GE_HALF_STONE &&
                answerIsYes(CQ::APPETITE2_INCREASE_PAST_MONTH)) {
            r.decide("Appetite gain and weight gain >=0.5st in past month. "
                     "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.");
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
        }
        break;

    case CQ::DISABLE:
        if (answerIsNo(q)) {
            r.decide("No longstanding illness/disability/infirmity.");
            jumpTo(CQ::SOMATIC_MAND1_PAIN_PAST_MONTH);
        }
        break;

    case CQ::ILLNESS:
        break;

    case CQ::SOMATIC_MAND1_PAIN_PAST_MONTH:
        if (answerIsNo(q)) {
            r.decide("No aches/pains in past month.");
            jumpTo(CQ::SOMATIC_MAND2_DISCOMFORT);
        }
        break;

    case CQ::SOMATIC_PAIN1_PSYCHOL_EXAC:
        if (v == V_SOMATIC_PAIN1_NEVER) {
            r.decide("Pains never exacerbated by low mood/anxiety/stress.");
            jumpTo(CQ::SOMATIC_MAND2_DISCOMFORT);
        }
        break;

    case CQ::SOMATIC_PAIN2_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("No pain in last 7 days.");
            jumpTo(CQ::SOMATIC_MAND2_DISCOMFORT);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Pain on >=4 of last 7 days. "
                     "Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        break;

    case CQ::SOMATIC_PAIN3_GT_3H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Pain for >3h on any day in past week. "
                     "Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        break;

    case CQ::SOMATIC_PAIN4_UNPLEASANT:
        if (v >= V_HOW_UNPLEASANT_UNPLEASANT) {
            r.decide("Pain 'unpleasant' or worse in past week. "
                     "Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        break;

    case CQ::SOMATIC_PAIN5_INTERRUPTED_INTERESTING:
        if (answerIsYes(q, v)) {
            r.decide("Pain interrupted an interesting activity in past week. "
                     "Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        r.decide("There was pain, so skip 'discomfort' section.");
        jumpTo(CQ::SOMATIC_DUR);  // skip SOMATIC_MAND2
        break;

    case CQ::SOMATIC_MAND2_DISCOMFORT:
        if (answerIsNo(q, v)) {
            r.decide("No discomfort.");
            jumpTo(CQ::FATIGUE_MAND1_TIRED_PAST_MONTH);
        }
        break;

    case CQ::SOMATIC_DIS1_PSYCHOL_EXAC:
        if (v == V_SOMATIC_DIS1_NEVER) {
            r.decide("Discomfort never exacerbated by being "
                     "low/anxious/stressed.");
            jumpTo(CQ::FATIGUE_MAND1_TIRED_PAST_MONTH);
        }
        break;

    case CQ::SOMATIC_DIS2_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("No discomfort in last 7 days.");
            jumpTo(CQ::FATIGUE_MAND1_TIRED_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Discomfort on >=4 days in past week. "
                     "Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        break;

    case CQ::SOMATIC_DIS3_GT_3H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Discomfort for >3h on any day in past week. "
                     "Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        break;

    case CQ::SOMATIC_DIS4_UNPLEASANT:
        if (v >= V_HOW_UNPLEASANT_UNPLEASANT) {
            r.decide("Discomfort 'unpleasant' or worse in past week. "
                     "Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        break;

    case CQ::SOMATIC_DIS5_INTERRUPTED_INTERESTING:
        if (answerIsYes(q, v)) {
            r.decide("Discomfort interrupted an interesting activity in past "
                     "week. Incrementing somatic_symptoms.");
            r.somatic_symptoms += 1;
        }
        break;

    // --------------------------------------------------------------------
    // Fatigue/energy
    // --------------------------------------------------------------------

    case CQ::FATIGUE_MAND1_TIRED_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("Not tired.");
            jumpTo(CQ::FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH);
        }
        break;

    case CQ::FATIGUE_CAUSE1_TIRED:
        if (v == V_FATIGUE_CAUSE_EXERCISE) {
            r.decide("Tired due to exercise. Move on.");
            jumpTo(CQ::CONC_MAND1_POOR_CONC_PAST_MONTH);
        }
        break;

    case CQ::FATIGUE_TIRED1_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("Not tired in past week.");
            jumpTo(CQ::FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Tired on >=4 days in past week. Incrementing fatigue.");
            r.fatigue += 1;
        }
        break;

    case CQ::FATIGUE_TIRED2_GT_3H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Tired for >3h on any day in past week. "
                     "Incrementing fatigue.");
            r.fatigue += 1;
        }
        break;

    case CQ::FATIGUE_TIRED3_HAD_TO_PUSH:
        if (answerIsYes(q, v)) {
            r.decide("Tired enough to have to push self during past week. "
                     "Incrementing fatigue.");
            r.fatigue += 1;
        }
        break;

    case CQ::FATIGUE_TIRED4_DURING_ENJOYABLE:
        if (answerIsYes(q, v)) {
            r.decide("Tired during an enjoyable activity during past week. "
                     "Incrementing fatigue.");
            r.fatigue += 1;
        }
        r.decide("There was tiredness, so skip 'lack of energy' section.");
        jumpTo(CQ::FATIGUE_DUR);  // skip FATIGUE_MAND2
        break;

    case CQ::FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("Not lacking in energy.");
            jumpTo(CQ::CONC_MAND1_POOR_CONC_PAST_MONTH);
        }
        break;

    case CQ::FATIGUE_CAUSE2_LACK_ENERGY:
        if (v == V_FATIGUE_CAUSE_EXERCISE) {
            r.decide("Lacking in energy due to exercise. Move on.");
            jumpTo(CQ::CONC_MAND1_POOR_CONC_PAST_MONTH);
        }
        break;

    case CQ::FATIGUE_ENERGY1_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("Not lacking in energy during last week.");
            jumpTo(CQ::CONC_MAND1_POOR_CONC_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Lacking in energy on >=4 days in past week. "
                     "Incrementing fatigue.");
            r.fatigue += 1;
        }
        break;

    case CQ::FATIGUE_ENERGY2_GT_3H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Lacking in energy for >3h on any day in past week. "
                     "Incrementing fatigue.");
            r.fatigue += 1;
        }
        break;

    case CQ::FATIGUE_ENERGY3_HAD_TO_PUSH:
        if (answerIsYes(q, v)) {
            r.decide("Lacking in energy enough to have to push self during "
                     "past week. Incrementing fatigue.");
            r.fatigue += 1;
        }
        break;

    case CQ::FATIGUE_ENERGY4_DURING_ENJOYABLE:
        if (answerIsYes(q, v)) {
            r.decide("Lacking in energy during an enjoyable activity during "
                     "past week. Incrementing fatigue.");
            r.fatigue += 1;
        }
        break;

    case CQ::FATIGUE_DUR:
        // Score preceding:
        if (r.somatic_symptoms >= 2 && r.fatigue >= 2) {
            r.decide("somatic >= 2 and fatigue >= 2. "
                     "Incrementing neurasthenia.");
            r.neurasthenia += 1;
        }
        break;

    // --------------------------------------------------------------------
    // Concentration/memory
    // --------------------------------------------------------------------

    case CQ::CONC_MAND1_POOR_CONC_PAST_MONTH:
        // Score preceding:
        if (r.fatigue >= 2) {
            r.decide("fatigue >= 2. "
                     "Incrementing depr_crit_1_mood_anhedonia_energy.");
            r.depr_crit_1_mood_anhedonia_energy += 1;
        }
        break;

    case CQ::CONC_MAND2_FORGETFUL_PAST_MONTH:
        if (answerIsNo(CQ::CONC_MAND1_POOR_CONC_PAST_MONTH) && answerIsNo(q, v)) {
            r.decide("No problems with concentration or forgetfulness.");
            jumpTo(CQ::SLEEP_MAND1_LOSS_PAST_MONTH);
        }
        break;

    case CQ::CONC1_CONC_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("No concentration/memory problems in past week.");
            jumpTo(CQ::SLEEP_MAND1_LOSS_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Problems with concentration/memory problems on >=4 days "
                     "in past week. Incrementing concentration_poor.");
            r.concentration_poor += 1;
        }
        if (answerIsNo(CQ::CONC_MAND1_POOR_CONC_PAST_MONTH) &&
                answerIsYes(CQ::CONC_MAND2_FORGETFUL_PAST_MONTH)) {
            r.decide("Forgetfulness, not concentration, problems; skip over "
                     "more detailed concentration questions.");
            jumpTo(CQ::CONC4_FORGOTTEN_IMPORTANT);  // skip CONC2, CONC3, CONC_DUR
        }
        break;

    case CQ::CONC2_CONC_FOR_TV_READING_CONVERSATION:
        if (answerIsNo(q, v)) {
            r.decide("Couldn't concentrate on at least one of {TV, newspaper, "
                     "conversation}. Incrementing concentration_poor.");
            r.concentration_poor += 1;
        }
        break;

    case CQ::CONC3_CONC_PREVENTED_ACTIVITIES:
        if (answerIsYes(q, v)) {
            r.decide("Problems with concentration stopped usual/desired "
                     "activity. Incrementing concentration_poor.");
            r.concentration_poor += 1;
        }
        break;

    case CQ::CONC_DUR:
        if (answerIsNo(CQ::CONC_MAND2_FORGETFUL_PAST_MONTH)) {
            jumpTo(CQ::SLEEP_MAND1_LOSS_PAST_MONTH);
        }
        break;

    case CQ::CONC4_FORGOTTEN_IMPORTANT:
        if (answerIsYes(q, v)) {
            r.decide("Forgotten something important in past week. "
                     "Incrementing concentration_poor.");
            r.concentration_poor += 1;
        }
        break;

    case CQ::FORGET_DUR:
        break;

    // --------------------------------------------------------------------
    // Sleep
    // --------------------------------------------------------------------

    case CQ::SLEEP_MAND1_LOSS_PAST_MONTH:
        // Score previous block:
        if (r.concentration_poor >= 2) {
            r.decide("concentration >= 2. "
                     "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.");
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
        }
        // This question:
        if (answerIsNo(q, v)) {
            r.decide("No problems with sleep loss in past month. Moving on.");
            jumpTo(CQ::SLEEP_MAND2_GAIN_PAST_MONTH);
        }
        break;

    case CQ::SLEEP_LOSE1_NIGHTS_PAST_WEEK:
        if (v == V_NIGHTS_IN_PAST_WEEK_0) {
            r.decide("No problems with sleep in past week. Moving on.");
            jumpTo(CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH);
        } else if (v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Problems with sleep on >=4 nights in past week. "
                     "Incrementing sleep_problems.");
            r.sleep_problems += 1;
        }
        break;

    case CQ::SLEEP_LOSE2_DIS_WORST_DURATION:
        if (v == V_SLEEP_CHANGE_LT_15_MIN) {
            r.decide("Less than 15min maximum delayed initiation of sleep in "
                     "past week. Moving on.");
            jumpTo(CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH);
        } else if (v == V_SLEEP_CHANGE_15_MIN_TO_1_H) {
            r.decide("15min-1h maximum delayed initiation of sleep in past "
                     "week. Incrementing sleep_problems.");
            r.sleep_problems += 1;
        } else if (v == V_SLEEP_CHANGE_1_TO_3_H ||
                   v == V_SLEEP_CHANGE_GT_3_H) {
            r.decide(">=1h maximum delayed initiation of sleep in past week. "
                     "Adding 2 to sleep_problems.");
            r.sleep_problems += 2;
        }
        break;

    case CQ::SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK:
        if (v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide(">=4 nights in past week with >=3h delayed initiation of "
                     "sleep. Incrementing sleep_problems.");
            r.sleep_problems += 1;
        }
        break;

    case CQ::SLEEP_EMW_PAST_WEEK:
        if (answerIsYes(q, v)) {
            r.decide("EMW of >2h in past week. "
                     "Setting sleep_change to SLEEPCHANGE_EMW. "
                     "Incrementing depr_crit_3_somatic_synd.");
            // Was: SLEEPCH += answer - 1 (which only does anything for a "yes" (2) answer).
            // ... but at this point, SLEEPCH is always 0.
            r.sleep_change = SLEEPCHANGE_EMW;  // LIKELY REDUNDANT.
            r.depr_crit_3_somatic_synd += 1;
            if (r.sleep_problems >= 1) {
                r.decide("EMW of >2h in past week and sleep_problems >= 1; "
                         "setting sleep_change to SLEEPCHANGE_EMW.");
                r.sleep_change = SLEEPCHANGE_EMW;
            }
        } else if (answerIsNo(q, v)) {
            r.decide("No EMW of >2h in past week.");
            if (r.sleep_problems >= 1) {
                r.decide("No EMW of >2h in past week, and sleep_problems >= 1. "
                         "Setting sleep_change to SLEEPCHANGE_INSOMNIA_NOT_EMW.");
                r.sleep_change = SLEEPCHANGE_INSOMNIA_NOT_EMW;
            }
        }
        break;

    case CQ::SLEEP_CAUSE:
        r.decide("Problems with sleep loss; skipping over sleep gain.");
        jumpTo(CQ::SLEEP_DUR);
        break;

    case CQ::SLEEP_MAND2_GAIN_PAST_MONTH:
        if (v == V_SLEEP_MAND2_NO || v == V_SLEEP_MAND2_YES_BUT_NOT_A_PROBLEM) {
            r.decide("No problematic sleep gain. Moving on.");
            jumpTo(CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH);
        }
        break;

    case CQ::SLEEP_GAIN1_NIGHTS_PAST_WEEK:
        if (v == V_NIGHTS_IN_PAST_WEEK_0) {
            r.decide("No nights with sleep problems [gain] in past week.");
            jumpTo(CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH);
        } else if (v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Problems with sleep [gain] on >=4 nights in past week. "
                     "Incrementing sleep_problems.");
            r.sleep_problems += 1;
        }
        break;

    case CQ::SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT:
        if (v == V_SLEEP_CHANGE_LT_15_MIN) {
            r.decide("Sleep gain <15min. Moving on.");
            jumpTo(CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH);
        } else if (v == V_SLEEP_CHANGE_15_MIN_TO_1_H) {
            r.decide("Sleep gain 15min-1h. Incrementing sleep_problems.");
            r.sleep_problems += 1;
        } else if (v >= V_SLEEP_CHANGE_1_TO_3_H) {
            r.decide("Sleep gain >=1h. "
                     "Adding 2 to sleep_problems. "
                     "Setting sleep_change to SLEEPCHANGE_INCREASE.");
            r.sleep_problems += 2;
            r.sleep_change = SLEEPCHANGE_INCREASE;
            // Note that in the original, if the answer was 3
            // (V_SLEEP_CHANGE_1_TO_3_H) or greater, first 2 was added to
            // sleep, and then if sleep was >=1, sleepch [sleep_change] was set
            // to 3. However, sleep is never decremented/set below 0, so that
            // was a redundant test (always true).
        }
        break;

    case CQ::SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK:
        if (v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Sleep gain of >3h on >=4 nights in past week. "
                     "Incrementing sleep_problems.");
            r.sleep_problems += 1;
        }
        break;

    case CQ::SLEEP_DUR:
        break;

    // --------------------------------------------------------------------
    // Irritability
    // --------------------------------------------------------------------

    case CQ::IRRIT_MAND1_PEOPLE_PAST_MONTH:
        // Score previous block:
        if (r.sleep_problems >= 2) {
            r.decide("sleep_problems >= 2. "
                     "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.");
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
        }
        // This bit erroneously lived under IRRIT_DUR in the original; see
        // discussion there:
        if (r.sleep_problems >= 2 && r.fatigue >= 2) {
            r.decide("sleep_problems >=2 and fatigue >=2. "
                     "Incrementing neurasthenia.");
            r.neurasthenia += 1;
        }
        // This question:
        if (answerIsYes(q, v)) {
            r.decide("Irritability (people) in past month; exploring further.");
            jumpTo(CQ::IRRIT1_DAYS_PER_WEEK);
        }
        break;

    case CQ::IRRIT_MAND2_THINGS_PAST_MONTH:
        if (v == V_IRRIT_MAND2_NO) {
            r.decide("No irritability. Moving on.");
            jumpTo(CQ::HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH);
        } else if (answered(q, v)) {
            r.decide("Irritability (things) in past month; exploring further.");
        }
        break;

    case CQ::IRRIT1_DAYS_PER_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("No irritability in past week. Moving on.");
            jumpTo(CQ::HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Irritable on >=4 days in past week. "
                     "Incrementing irritability.");
            r.irritability += 1;
        }
        break;

    case CQ::IRRIT2_GT_1H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Irritable for >1h on any day in past week. "
                     "Incrementing irritability.");
            r.irritability += 1;
        }
        break;

    case CQ::IRRIT3_WANTED_TO_SHOUT:
        if (v >= V_IRRIT3_SHOUTING_WANTED_TO) {
            r.decide("Wanted to or did shout. Incrementing irritability.");
            r.irritability += 1;
        }
        break;

    case CQ::IRRIT4_ARGUMENTS:
        if (v == V_IRRIT4_ARGUMENTS_YES_UNJUSTIFIED) {
            r.decide("Arguments without justification. "
                     "Incrementing irritability.");
            r.irritability += 1;
        }
        break;

    case CQ::IRRIT_DUR:
        // Score recent things:
        if (r.irritability >= 2 && r.fatigue >= 2) {
            r.decide("irritability >=2 and fatigue >=2. "
                     "Incrementing neurasthenia.");
            r.neurasthenia += 1;
        }
        // In the original, we had the rule "sleep_problems >=2 and
        // fatigue >=2 -> incrementing neurasthenia" here, but that would mean
        // we would fail to score sleep if the patient didn't report
        // irritability (because if you say no at IRRIT_MAND2, you jump beyond
        // this point to HYPO_MAND1). Checked with Glyn Lewis 2017-12-04, who
        // agreed on 2017-12-05. Therefore, moved to IRRIT_MAND1 as above.
        // Note that the only implication would have been potential small
        // mis-scoring of the CFS criterion (not any of the diagnoses that
        // the CIS-R reports as its primary/secondary diagnoses).
        break;

    // --------------------------------------------------------------------
    // Hypochondriasis
    // --------------------------------------------------------------------

    case CQ::HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH:
        if (answerIsYes(q, v)) {
            r.decide("No worries about physical health in past month. Moving on.");
            jumpTo(CQ::HYPO1_DAYS_PAST_WEEK);
        }
        break;

    case CQ::HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS:
        if (answerIsNo(q, v)) {
            r.decide("No worries about having a serious illness. Moving on.");
            jumpTo(CQ::DEPR_MAND1_LOW_MOOD_PAST_MONTH);
        }
        break;

    case CQ::HYPO1_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("No days in past week worrying about health. Moving on.");
            jumpTo(CQ::DEPR_MAND1_LOW_MOOD_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Worries about health on >=4 days in past week. "
                     "Incrementing hypochondria.");
            r.hypochondria += 1;
        }
        break;

    case CQ::HYPO2_WORRY_TOO_MUCH:
        if (answerIsYes(q, v)) {
            r.decide("Worrying too much about health. "
                     "Incrementing hypochondria.");
            r.hypochondria += 1;
        }
        break;

    case CQ::HYPO3_HOW_UNPLEASANT:
        if (v >= V_HOW_UNPLEASANT_UNPLEASANT) {
            r.decide("Worrying re health 'unpleasant' or worse in past week. "
                     "Incrementing hypochondria.");
            r.hypochondria += 1;
        }
        break;

    case CQ::HYPO4_CAN_DISTRACT:
        if (answerIsNo(q, v)) {
            r.decide("Cannot take mind off health worries by doing something "
                     "else. Incrementing hypochondria.");
            r.hypochondria += 1;
        }
        break;

    case CQ::HYPO_DUR:
        break;

    // --------------------------------------------------------------------
    // Depression
    // --------------------------------------------------------------------

    case CQ::DEPR_MAND1_LOW_MOOD_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("Mood not low in past month. Moving to anhedonia.");
            jumpTo(CQ::DEPR_MAND2_ENJOYMENT_PAST_MONTH);
        }
        break;

    case CQ::DEPR1_LOW_MOOD_PAST_WEEK:
        break;

    case CQ::DEPR_MAND2_ENJOYMENT_PAST_MONTH:
        if (v == V_ANHEDONIA_ENJOYING_NORMALLY &&
                answerIsNo(CQ::DEPR1_LOW_MOOD_PAST_WEEK)) {
            r.decide("Neither low mood nor anhedonia in past month. Moving on.");
            jumpTo(CQ::WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH);
        }
        break;

    case CQ::DEPR2_ENJOYMENT_PAST_WEEK:
        if (v == V_ANHEDONIA_ENJOYING_NORMALLY &&
                answerIsNo(CQ::DEPR_MAND1_LOW_MOOD_PAST_MONTH)) {
            r.decide("No anhedonia in past week and no low mood in past month. "
                     "Moving on.");
            jumpTo(CQ::WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH);
        } else if (v >= V_ANHEDONIA_ENJOYING_LESS) {
            r.decide("Partial or complete anhedonia in past week. "
                     "Incrementing depression. "
                     "Incrementing depr_crit_1_mood_anhedonia_energy. "
                     "Incrementing depr_crit_3_somatic_synd.");
            r.depression += 1;
            r.depr_crit_1_mood_anhedonia_energy += 1;
            r.depr_crit_3_somatic_synd += 1;
        }
        break;

    case CQ::DEPR3_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Low mood or anhedonia on >=4 days in past week. "
                     "Incrementing depression.");
            r.depression += 1;

        }
        break;

    case CQ::DEPR4_GT_3H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Low mood or anhedonia for >3h/day on at least one day "
                     "in past week. Incrementing depression.");
            r.depression += 1;
            if (intValueForQuestion(CQ::DEPR3_DAYS_PAST_WEEK) &&
                    answerIsYes(CQ::DEPR1_LOW_MOOD_PAST_WEEK)) {
                r.decide("(A) Low mood in past week, and "
                         "(B) low mood or anhedonia for >3h/day on at least "
                         "one day in past week, and "
                         "(C) low mood or anhedonia on >=4 days in past week. "
                         "Incrementing depr_crit_1_mood_anhedonia_energy.");
                r.depr_crit_1_mood_anhedonia_energy += 1;
            }
        }
        break;

    case CQ::DEPR_CONTENT:
        break;

    case CQ::DEPR5_COULD_CHEER_UP:
        if (v >= V_DEPR5_COULD_CHEER_UP_SOMETIMES) {
            r.decide("'Sometimes' or 'never' cheered up by nice things. "
                     "Incrementing depression. "
                     "Incrementing depr_crit_3_somatic_synd.");
            r.depression += 1;
            r.depr_crit_3_somatic_synd += 1;
        }
        break;

    case CQ::DEPR_DUR:
        if (v >= V_DURATION_2W_6M) {
            r.decide("Depressive symptoms for >=2 weeks. "
                     "Setting depression_at_least_2_weeks.");
            r.depression_at_least_2_weeks = true;
        }
        // This code was at the start of DEPTH1, but involves skipping over
        // DEPTH1; since we never get to DEPTH1 without coming here, we can
        // move it here:
        if (r.depression == 0) {
            r.decide("Score for 'depression' is 0; skipping over depressive "
                     "thought content questions.");
            jumpTo(CQ::WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH);
        }
        break;

    case CQ::DEPTH1_DIURNAL_VARIATION:
        if (v == V_DEPTH1_DMV_WORSE_MORNING ||
                v == V_DEPTH1_DMV_WORSE_EVENING) {
            r.decide("Diurnal mood variation present.");
            r.diurnal_mood_variation = v == V_DEPTH1_DMV_WORSE_MORNING
                    ? DIURNAL_MOOD_VAR_WORSE_MORNING
                    : DIURNAL_MOOD_VAR_WORSE_EVENING;
            if (v == V_DEPTH1_DMV_WORSE_MORNING) {
                r.decide("Diurnal mood variation, worse in the mornings. "
                         "Incrementing depr_crit_3_somatic_synd.");
                r.depr_crit_3_somatic_synd += 1;
            }
        }
        break;

    case CQ::DEPTH2_LIBIDO:
        if (v == V_DEPTH2_LIBIDO_DECREASED) {
            r.decide("Libido decreased over past month. "
                     "Setting libido_decreased. "
                     "Incrementing depr_crit_3_somatic_synd.");
            r.libido_decreased = true;
            r.depr_crit_3_somatic_synd += 1;
        }
        break;

    case CQ::DEPTH3_RESTLESS:
        if (answerIsYes(q)) {
            r.decide("Psychomotor agitation.");
            r.psychomotor_changes = PSYCHOMOTOR_AGITATION;
        }
        break;

    case CQ::DEPTH4_SLOWED:
        if (answerIsYes(q)) {
            r.decide("Psychomotor retardation.");
            r.psychomotor_changes = PSYCHOMOTOR_RETARDATION;
        }
        if (r.psychomotor_changes > PSYCHOMOTOR_NONE) {
            r.decide("Psychomotor agitation or retardation. "
                     "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui. "
                     "Incrementing depr_crit_3_somatic_synd.");
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
            r.depr_crit_3_somatic_synd += 1;
        }
        break;

    case CQ::DEPTH5_GUILT:
        if (v >= V_DEPTH5_GUILT_SOMETIMES) {
            r.decide("Feel guilty when not at fault sometimes or often. "
                     "Incrementing depressive_thoughts. "
                     "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.");
            r.depressive_thoughts += 1;
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
        }
        break;

    case CQ::DEPTH6_WORSE_THAN_OTHERS:
        if (answerIsYes(q, v)) {
            r.decide("Feeling not as good as other people. "
                     "Incrementing depressive_thoughts. "
                     "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.");
            r.depressive_thoughts += 1;
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
        }
        break;

    case CQ::DEPTH7_HOPELESS:
        if (answerIsYes(q, v)) {
            r.decide("Hopelessness. "
                     "Incrementing depressive_thoughts. "
                     "Setting suicidality to SUICIDE_INTENT_HOPELESS_NO_SUICIDAL_THOUGHTS.");
            r.depressive_thoughts += 1;
            r.suicidality = SUICIDE_INTENT_HOPELESS_NO_SUICIDAL_THOUGHTS;
        }
        break;

    case CQ::DEPTH8_LNWL:
        if (v == V_DEPTH8_LNWL_NO) {
            r.decide("No thoughts of life not being worth living. Skipping to "
                     "end of depression section.");
            jumpTo(CQ::DEPR_OUTRO);
        } else if (v >= V_DEPTH8_LNWL_SOMETIMES) {
            r.decide("Sometimes or always feeling life isn't worth living. "
                     "Incrementing depressive_thoughts. "
                     "Setting suicidality to SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING.");
            r.depressive_thoughts += 1;
            r.suicidality = SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING;
        }
        break;

    case CQ::DEPTH9_SUICIDE_THOUGHTS:
        if (v == V_DEPTH9_SUICIDAL_THOUGHTS_NO) {
            r.decide("No thoughts of suicide. Skipping to end of depression "
                     "section.");
            jumpTo(CQ::DEPR_OUTRO);
        }
        if (v >= V_DEPTH9_SUICIDAL_THOUGHTS_YES_BUT_NEVER_WOULD) {
            r.decide("Suicidal thoughts present. "
                     "Setting suicidality to SUICIDE_INTENT_SUICIDAL_THOUGHTS.");
            r.suicidality = SUICIDE_INTENT_SUICIDAL_THOUGHTS;
        }
        if (v == V_DEPTH9_SUICIDAL_THOUGHTS_YES_BUT_NEVER_WOULD) {
            r.decide("Suicidal thoughts present but denies would ever act. "
                     "Skipping to talk-to-doctor section.");
            jumpTo(CQ::DOCTOR);
        }
        if (v == V_DEPTH9_SUICIDAL_THOUGHTS_YES) {
            r.decide(
                "Thoughts of suicide in past week. "
                "Incrementing depressive_thoughts. "
                "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.");
            r.depressive_thoughts += 1;
            r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1;
        }
        break;

    case CQ::DEPTH10_SUICIDE_METHOD:
        if (answerIsYes(q, v)) {
            r.decide("Suicidal thoughts without denying might ever act. "
                     "Setting suicidality to SUICIDE_INTENT_SUICIDAL_PLANS.");
            r.suicidality = SUICIDE_INTENT_SUICIDAL_PLANS;
        }
        break;

    case CQ::DOCTOR:
        if (v == V_DOCTOR_YES) {
            r.decide("Has spoken to doctor about suicidality. Skipping "
                     "exhortation to do so.");
            jumpTo(CQ::DEPR_OUTRO);
        }
        break;

    case CQ::DOCTOR2_PLEASE_TALK_TO:
    case CQ::DEPR_OUTRO:
        break;

    // --------------------------------------------------------------------
    // Worry/anxiety
    // --------------------------------------------------------------------

    case CQ::WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH:
        if (v >= V_NSO_SOMETIMES) {
            r.decide("Worrying excessively 'sometimes' or 'often'. "
                     "Exploring further.");
            jumpTo(CQ::WORRY_CONT1);
        }
        break;

    case CQ::WORRY_MAND2_ANY_WORRIES_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("No worries at all in the past month. Moving on.");
            jumpTo(CQ::ANX_MAND1_ANXIETY_PAST_MONTH);
        }
        break;

    case CQ::WORRY_CONT1:
    case CQ::WORRY1_INFO_ONLY:
        break;

    case CQ::WORRY2_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("Worry [other than re physical health] on 0 days in past "
                     "week. Moving on.");
            jumpTo(CQ::ANX_MAND1_ANXIETY_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Worry [other than re physical health] on >=4 days in "
                     "past week. Incrementing worry.");
            r.worry += 1;
        }
        break;

    case CQ::WORRY3_TOO_MUCH:
        if (answerIsYes(q, v)) {
            r.decide("Worrying too much. Incrementing worry.");
            r.worry += 1;
        }
        break;

    case CQ::WORRY4_HOW_UNPLEASANT:
        if (v >= V_HOW_UNPLEASANT_UNPLEASANT) {
            r.decide("Worry [other than re physical health] 'unpleasant' or "
                     "worse in past week. Incrementing worry.");
            r.worry += 1;
        }
        break;

    case CQ::WORRY5_GT_3H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Worry [other than re physical health] for >3h on any "
                     "day in past week. Incrementing worry.");
            r.worry += 1;
        }
        break;

    case CQ::WORRY_DUR:
        break;

    case CQ::ANX_MAND1_ANXIETY_PAST_MONTH:
        if (answerIsYes(q, v)) {
            r.decide("Anxious/nervous in past month. "
                     "Skipping tension question.");
            jumpTo(CQ::ANX_PHOBIA1_SPECIFIC_PAST_MONTH);
        }
        break;

    case CQ::ANX_MAND2_TENSION_PAST_MONTH:
        if (v == V_NSO_NO) {
            r.decide("No tension in past month (and no anxiety, from previous "
                     "question). Moving on.");
            jumpTo(CQ::PHOBIAS_MAND_AVOIDANCE_PAST_MONTH);
        }
        break;

    case CQ::ANX_PHOBIA1_SPECIFIC_PAST_MONTH:
        if (answerIsNo(q, v)) {
            r.decide("No phobias. Moving on to general anxiety.");
            jumpTo(CQ::ANX2_GENERAL_DAYS_PAST_WEEK);
        } else if (answerIsYes(q, v)) {
            // This was in ANX_PHOBIA2; PHOBIAS_FLAG was set by arriving
            // there (but that only happens when we get a 'yes' answer here).
            r.decide("Phobias. Exploring further. Setting phobias flag.");
            r.phobias_flag = true;
        }
        break;

    case CQ::ANX_PHOBIA2_SPECIFIC_OR_GENERAL:
        if (v == V_ANX_PHOBIA2_ALWAYS_SPECIFIC) {
            r.decide("Anxiety always specific. Skipping generalized anxiety.");
            jumpTo(CQ::PHOBIAS_TYPE1);
        }
        break;

    case CQ::ANX1_INFO_ONLY:
        break;

    case CQ::ANX2_GENERAL_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            if (r.phobias_flag) {
                r.decide("No generalized anxiety in past week. "
                         "Skipping further generalized anxiety questions.");
                jumpTo(CQ::PHOBIAS1_DAYS_PAST_WEEK);
            } else {
                r.decide("No generalized anxiety in past week. Moving on.");
                jumpTo(CQ::COMP_MAND1_COMPULSIONS_PAST_MONTH);
            }
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Generalized anxiety on >=4 days in past week. "
                     "Incrementing anxiety.");
            r.anxiety += 1;
        }
        break;

    case CQ::ANX3_GENERAL_HOW_UNPLEASANT:
        if (v >= V_HOW_UNPLEASANT_UNPLEASANT) {
            r.decide("Anxiety 'unpleasant' or worse in past week. "
                     "Incrementing anxiety.");
            r.anxiety += 1;
        }
        break;

    case CQ::ANX4_GENERAL_PHYSICAL_SYMPTOMS:
        if (answerIsYes(q, v)) {
            r.decide("Physical symptoms of anxiety. "
                     "Setting anxiety_physical_symptoms. "
                     "Incrementing anxiety.");
            r.anxiety_physical_symptoms = true;
            r.anxiety += 1;
        }
        break;

    case CQ::ANX5_GENERAL_GT_3H_ANY_DAY:
        if (answerIsYes(q, v)) {
            r.decide("Anxiety for >3h on any day in past week. "
                     "Incrementing anxiety.");
            r.anxiety += 1;
        }
        break;

    case CQ::ANX_DUR_GENERAL:
        if (v >= V_DURATION_2W_6M) {
            r.decide("Anxiety for >=2 weeks. "
                     "Setting anxiety_at_least_2_weeks.");
            r.anxiety_at_least_2_weeks = true;
        }
        if (r.phobias_flag) {
            r.decide("Phobias flag set. Exploring further.");
            jumpTo(CQ::PHOBIAS_TYPE1);
        } else {
            if (r.anxiety <= 1) {
                r.decide("Anxiety score <=1. Moving on to compulsions.");
                jumpTo(CQ::COMP_MAND1_COMPULSIONS_PAST_MONTH);
            } else {
                r.decide("Anxiety score >=2. Exploring panic.");
                jumpTo(CQ::PANIC_MAND_PAST_MONTH);
            }
        }
        break;

    case CQ::PHOBIAS_MAND_AVOIDANCE_PAST_MONTH:
        if (answerIsNo(q, v)) {
            if (r.anxiety <= 1) {
                r.decide("Anxiety score <=1. Moving on to compulsions.");
                jumpTo(CQ::COMP_MAND1_COMPULSIONS_PAST_MONTH);
            } else {
                r.decide("Anxiety score >=2. Exploring panic.");
                jumpTo(CQ::PANIC_MAND_PAST_MONTH);
            }
        }
        break;

    case CQ::PHOBIAS_TYPE1:
        switch (v) {
        case V_PHOBIAS_TYPE1_ALONE_PUBLIC_TRANSPORT:
        case V_PHOBIAS_TYPE1_FAR_FROM_HOME:
        case V_PHOBIAS_TYPE1_CROWDED_SHOPS:
            r.decide("Phobia type category: agoraphobia.");
            r.phobias_type = PHOBIATYPES_AGORAPHOBIA;
            break;

        case V_PHOBIAS_TYPE1_PUBLIC_SPEAKING_EATING:
        case V_PHOBIAS_TYPE1_BEING_WATCHED:
            r.decide("Phobia type category: social.");
            r.phobias_type = PHOBIATYPES_SOCIAL;
            break;

        case V_PHOBIAS_TYPE1_BLOOD:
            r.decide("Phobia type category: blood/injury.");
            r.phobias_type = PHOBIATYPES_BLOOD_INJURY;
            break;

        case V_PHOBIAS_TYPE1_ANIMALS:
        case V_PHOBIAS_TYPE1_ENCLOSED_SPACES_HEIGHTS:
            r.decide("Phobia type category: animals/enclosed spaces/heights.");
            r.phobias_type = PHOBIATYPES_ANIMALS_ENCLOSED_HEIGHTS;
            break;

        case V_PHOBIAS_TYPE1_OTHER:
            r.decide("Phobia type category: other.");
            r.phobias_type = PHOBIATYPES_OTHER;
            break;

        default:
            break;
        }
        break;

    case CQ::PHOBIAS1_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Phobic anxiety on >=4 days in past week. "
                     "Incrementing phobias_score.");
            r.phobias_score += 1;
        }
        break;

    case CQ::PHOBIAS2_PHYSICAL_SYMPTOMS:
        if (answerIsYes(q, v)) {
            r.decide("Physical symptoms during phobic anxiety in past week. "
                     "Incrementing phobias_score.");
            r.phobias_score += 1;
        }
        break;

    case CQ::PHOBIAS3_AVOIDANCE:
        if (answerIsNo(q, v)) {  // no avoidance in past week
            if (r.anxiety <= 1 && r.phobias_score == 0) {
                r.decide("No avoidance in past week; "
                         "anxiety <= 1 and phobias_score == 0. "
                         "Finishing anxiety section.");
                jumpTo(CQ::ANX_OUTRO);
            } else {
                r.decide("No avoidance in past week; "
                         "anxiety >= 2 or phobias_score >= 1. "
                         "Moving to panic section.");
                jumpTo(CQ::PANIC_MAND_PAST_MONTH);
            }
        } else if (answerIsYes(q, v)) {
            r.decide("Setting phobic_avoidance.");
            r.phobic_avoidance = true;
        }
        break;

    case CQ::PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_1_TO_3) {
            r.decide("Phobic avoidance on 1-3 days in past week. "
                     "Incrementing phobias_score.");
            r.phobias_score += 1;
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Phobic avoidance on >=4 days in past week. "
                     "Adding 2 to phobias_score.");
            r.phobias_score += 2;
        }
        if (r.anxiety <= 1 &&
                intValueForQuestion(CQ::PHOBIAS1_DAYS_PAST_WEEK) ==
                V_DAYS_IN_PAST_WEEK_0) {
            r.decide("anxiety <= 1 and no phobic anxiety in past week. "
                     "Finishing anxiety section.");
            jumpTo(CQ::ANX_OUTRO);
        }
        break;

    case CQ::PHOBIAS_DUR:
        break;

    case CQ::PANIC_MAND_PAST_MONTH:
        if (v == V_NSO_NO) {
            r.decide("No panic in the past month. Finishing anxiety section.");
            jumpTo(CQ::ANX_OUTRO);
        }
        break;

    case CQ::PANIC1_NUM_PAST_WEEK:
        if (v == V_PANIC1_N_PANICS_PAST_WEEK_0) {
            r.decide("No panic in past week. Finishing anxiety section.");
            jumpTo(CQ::ANX_OUTRO);
        } else if (v == V_PANIC1_N_PANICS_PAST_WEEK_1) {
            r.decide("One panic in past week. Incrementing panic.");
            r.panic += 1;
        } else if (v == V_PANIC1_N_PANICS_PAST_WEEK_GT_1) {
            r.decide("More than one panic in past week. Adding 2 to panic.");
            r.panic += 2;
        }
        break;

    case CQ::PANIC2_HOW_UNPLEASANT:
        if (v >= V_HOW_UNPLEASANT_UNPLEASANT) {
            r.decide("Panic 'unpleasant' or worse in past week. "
                     "Incrementing panic.");
            r.panic += 1;
        }
        break;

    case CQ::PANIC3_PANIC_GE_10_MIN:
        if (v == V_PANIC3_WORST_GE_10_MIN) {
            r.decide("Worst panic in past week lasted >=10 min. Incrementing panic.");
            r.panic += 1;
        }
        break;

    case CQ::PANIC4_RAPID_ONSET:
        if (answerIsYes(q, v)) {
            r.decide("Rapid onset of panic symptoms. "
                     "Setting panic_rapid_onset.");
            r.panic_rapid_onset = true;
        }
        break;

    case CQ::PANSYM:
        // Multi-way answer. All are scored 1=no, 2=yes.
        {
            int n_panic_symptoms = 0;
            for (const QString& fieldname : panicSymptomFieldnames()) {
                const int panic_symptom = valueInt(fieldname);
                const bool yes_present = panic_symptom == 2;
                if (yes_present) {
                    n_panic_symptoms += 1;
                }
            }
            r.decide(QString("%1 out of %2 specific panic symptoms endorsed.")
                     .arg(n_panic_symptoms, NUM_PANIC_SYMPTOMS));
        }
        // The next bit was coded in PANIC5, but lives more naturally here:
        if (answerIsNo(CQ::ANX_PHOBIA1_SPECIFIC_PAST_MONTH)) {
            jumpTo(CQ::PANIC_DUR);
        }
        break;

    case CQ::PANIC5_ALWAYS_SPECIFIC_TRIGGER:
    case CQ::PANIC_DUR:
    case CQ::ANX_OUTRO:
        break;

    // --------------------------------------------------------------------
    // Compulsions and obsessions
    // --------------------------------------------------------------------

    case CQ::COMP_MAND1_COMPULSIONS_PAST_MONTH:
        if (v == V_NSO_NO) {
            r.decide("No compulsions in past month. Moving to obsessions.");
            jumpTo(CQ::OBSESS_MAND1_OBSESSIONS_PAST_MONTH);
        }
        break;

    case CQ::COMP1_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("No compulsions in past week. Moving to obesssions.");
            jumpTo(CQ::OBSESS_MAND1_OBSESSIONS_PAST_MONTH);
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Obsessions on >=4 days in past week. "
                     "Incrementing compulsions.");
            r.compulsions += 1;
        }
        break;

    case CQ::COMP2_TRIED_TO_STOP:
        if (answerIsYes(q, v)) {
            r.decide("Attempts to stop compulsions in past week. "
                     "Setting compulsions_tried_to_stop. "
                     "Incrementing compulsions.");
            r.compulsions_tried_to_stop = true;
            r.compulsions += 1;
        }
        break;

    case CQ::COMP3_UPSETTING:
        if (answerIsYes(q, v)) {
            r.decide("Compulsions upsetting/annoying. Incrementing compulsions.");
            r.compulsions += 1;
        }
        break;

    case CQ::COMP4_MAX_N_REPETITIONS:
        if (v == V_COMP4_MAX_N_REPEATS_GE_3) {
            r.decide("At worst, >=3 repeats. Incrementing compulsions.");
            r.compulsions += 1;
        }
        break;

    case CQ::COMP_DUR:
        if (v >= V_DURATION_2W_6M) {
            r.decide("Compulsions for >=2 weeks. "
                     "Setting compulsions_at_least_2_weeks.");
            r.compulsions_at_least_2_weeks = true;
        }
        break;

    case CQ::OBSESS_MAND1_OBSESSIONS_PAST_MONTH:
        if (v == V_NSO_NO) {
            r.decide("No obsessions in past month. Moving on.");
            jumpTo(chooseFinalPage());
        }
        break;

    case CQ::OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL:
        if (v == V_OBSESS_MAND1_GENERAL_WORRIES) {
            r.decide("Worrying about something in general, not the same "
                     "thoughts over and over again. Moving on.");
            jumpTo(chooseFinalPage());
        }
        break;

    case CQ::OBSESS1_DAYS_PAST_WEEK:
        if (v == V_DAYS_IN_PAST_WEEK_0) {
            r.decide("No obsessions in past week. Moving on.");
            jumpTo(chooseFinalPage());
        } else if (v == V_DAYS_IN_PAST_WEEK_4_OR_MORE) {
            r.decide("Obsessions on >=4 days in past week. "
                     "Incrementing obsessions.");
            r.obsessions += 1;
        }
        break;

    case CQ::OBSESS2_TRIED_TO_STOP:
        if (answerIsYes(q, v)) {
            r.decide("Tried to stop obsessional thoughts in past week. "
                     "Setting obsessions_tried_to_stop. "
                     "Incrementing obsessions.");
            r.obsessions_tried_to_stop = true;
            r.obsessions += 1;
        }
        break;

    case CQ::OBSESS3_UPSETTING:
        if (answerIsYes(q, v)) {
            r.decide("Obsessions upsetting/annoying in past week. "
                     "Incrementing obsessions.");
            r.obsessions += 1;
        }
        break;

    case CQ::OBSESS4_MAX_DURATION:
        if (v == V_OBSESS4_GE_15_MIN) {
            r.decide("Obsessions lasting >=15 min in past week. "
                     "Incrementing obsessions.");
            r.obsessions += 1;
        }
        break;

    case CQ::OBSESS_DUR:
        if (v >= V_DURATION_2W_6M) {
            r.decide("Obsessions for >=2 weeks. "
                     "Setting obsessions_at_least_2_weeks.");
            r.obsessions_at_least_2_weeks = true;
        }
        break;

    // --------------------------------------------------------------------
    // End
    // --------------------------------------------------------------------

    case CQ::OVERALL1_INFO_ONLY:
        break;

    case CQ::OVERALL2_IMPACT_PAST_WEEK:
        if (answered(q, v)) {
            r.functional_impairment = v - 1;
            r.decide(QString("Setting functional_impairment to %1").arg(
                         r.functional_impairment));
        }
        break;

    case CQ::THANKS_FINISHED:
        break;

    case CQ::END_MARKER:  // this is not a page
        // we've reached the end; no point thinking further
        return CQ::END_MARKER;

    default:
        break;
    }
    if (next_q == -1) {
        // Nothing has expressed an overriding preference, so increment...
        next_q = enumToInt(q) + 1;
    }
    return intToEnum(next_q);
}


Cisr::CisrQuestion Cisr::getPageEnum(const int qnum_zero_based) const
{
    // This function embodies the logic about the question sequence.
    //
    // This is slightly tricky algorithmically, since the user can go backwards
    // and forwards (which the original CIS-R doesn't do). The answers so far
    // define a sequence of questions, and we offer the nth from that sequence.
    //
    // Since the CISR sequence is linear with answer-dependent skipping,
    // it's a little bit easier than it might be.
    // All we need to do is define the moments when we SKIP something.
    // (See nextQ().)
    //
    // We can't use the incoming current_qnum directly, though, since that is
    // the sequence WITHIN QUESTIONS BEING PRESENTED, not overall. Hence the
    // iteration.

    CisrQuestion internal_q = CQ::START_MARKER;
    CisrResult result;  // contents will be ignored!
    result.record_decisions = false;  // we don't care here

    for (int external_qnum = 0; external_qnum < qnum_zero_based; ++external_qnum) {
        internal_q = nextQ(internal_q, result);
    }  // loop until we have the right number of "external" pages
    return internal_q;
}


Cisr::CisrResult Cisr::getResult() const
{
    CisrQuestion internal_q = CQ::START_MARKER;
    CisrResult result;

    while (!result.incomplete && internal_q != CQ::END_MARKER) {
        internal_q = nextQ(internal_q, result);
    }  // loop until we reach the end or have incomplete data
    result.finalize();
    return result;
}


QuPagePtr Cisr::makePageFromEnum(CisrQuestion q)
{
    const int intq = enumToInt(q);

    // Internals
    auto makeOptions = [this]
            (const QString& xstring_name_stem, int last,
             int first = 1) -> NameValueOptions {
        Q_ASSERT(first <= last);
        NameValueOptions options;
        for (int i = first; i <= last; ++i) {
            const QString text = xstring(xstring_name_stem + QString::number(i));
            options.append(NameValuePair(text, i));
        }
        return options;
    };

    // Element makers
    auto title = [this, &q]() -> QString {
#ifdef DEBUG_SHOW_PAGE_TAGS
        return QString("CISR page %1 (%2)").arg(
                    QString::number(enumToInt(q)), tagForQuestion(q));
#else
        if (m_questionnaire && m_questionnaire->readOnly()) {
            // Show title tags on facsimile (read-only) version
            return QString("CISR page %1 (%2)").arg(
                        QString::number(enumToInt(q)), tagForQuestion(q));
        }
        return QString("CISR page %1").arg(enumToInt(q));
#endif
    };
    auto prompttext = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto question = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options) -> QuElement* {
        return new QuMcq(fieldRef(fieldname), options);
    };
#ifdef DEBUG_SHOW_PAGE_TAGS
    auto addTag = [this, &q](QuPage* p) {
        const QString tag = tagForQuestion(q);
        const QString text = QString("Debugging tags on: %1 (q%2)").arg(
                    tag, QString::number(enumToInt(q)));
        QuText* element = new QuText(text);
        element->setWarning(true);
        p->addElement(element);
    };
#endif

    // Page makers
    auto promptPage = [&title, &prompttext
#ifdef DEBUG_SHOW_PAGE_TAGS
                                          , &addTag
#endif
                                                   ]
            (const QString& prompt_xstringname) -> QuPagePtr {
        auto p = new QuPage();
        p->addElement(prompttext(prompt_xstringname));
        p->setTitle(title());
#ifdef DEBUG_SHOW_PAGE_TAGS
        addTag(p);
#endif
        return QuPagePtr(p);
    };
    auto qPage = [&title, &question
        #ifdef DEBUG_SHOW_PAGE_TAGS
                                   , &addTag
        #endif
                                           ]
            (const QString& question_xstringname, QuElement* element,
             QuElement* extra_between_q_and_element = nullptr)
            -> QuPagePtr {
        auto p = new QuPage();
        p->addElement(question(question_xstringname));
        if (extra_between_q_and_element) {
            p->addElement(extra_between_q_and_element);
        }
        p->addElement(element);
        p->setTitle(title());
#ifdef DEBUG_SHOW_PAGE_TAGS
        addTag(p);
#endif
        return QuPagePtr(p);
    };
    auto standardOptionsQPage = [this, &q, &qPage, &mcq]
            (const NameValueOptions& options) -> QuPagePtr {
        const QString fieldname = fieldnameForQuestion(q);
        const QString xstring_q = fieldname + XSTRING_QUESTION_SUFFIX;
        return qPage(xstring_q, mcq(fieldname, options));
    };
    auto overallDuration = [&standardOptionsQPage, &makeOptions]
            () -> QuPagePtr {
        return standardOptionsQPage(makeOptions("duration_a", N_DURATIONS));
    };
    auto daysPerWeek = [&standardOptionsQPage, &makeOptions]
            () -> QuPagePtr {
        return standardOptionsQPage(
                    makeOptions("dpw_a", N_OPTIONS_DAYS_PER_WEEK));
    };
    auto nightsPerWeek = [&standardOptionsQPage, &makeOptions]
            () -> QuPagePtr {
        return standardOptionsQPage(
                    makeOptions("npw_a", N_OPTIONS_NIGHTS_PER_WEEK));
    };
    auto howUnpleasantStandard = [&standardOptionsQPage, &makeOptions]
            () -> QuPagePtr {
        return standardOptionsQPage(
                    makeOptions("how_unpleasant_a", N_OPTIONS_HOW_UNPLEASANT));
    };
    auto fatigueCauses = [&standardOptionsQPage, &makeOptions]
            () -> QuPagePtr {
        return standardOptionsQPage(
                    makeOptions("fatigue_causes_a", N_OPTIONS_FATIGUE_CAUSES));
    };
    auto stressors = [&standardOptionsQPage, &makeOptions]
            () -> QuPagePtr {
        return standardOptionsQPage(
                    makeOptions("stressors_a", N_OPTIONS_STRESSORS));
    };
    auto noSometimesOften = [&standardOptionsQPage, &makeOptions]
            () -> QuPagePtr {
        return standardOptionsQPage(
                    makeOptions("nso_a", N_OPTIONS_NO_SOMETIMES_OFTEN));
    };
    auto notImplemented = [this, &title, &q, &intq
#ifdef DEBUG_SHOW_PAGE_TAGS
                                                  , &addTag
#endif
                                                           ]() -> QuPagePtr {
        qCritical() << "CISR question not implemented:"
                    << intq << tagForQuestion(q);
        auto p = new QuPage();
        p->addElement(new QuText(QString("Question %1 not implemented yet!")
                                 .arg(intq)));
        p->setTitle(title());
#ifdef DEBUG_SHOW_PAGE_TAGS
        addTag(p);
#endif
        return QuPagePtr(p);
    };
    auto ynQuestion = [this, &q, &intq, &notImplemented, &makeOptions,
                       &standardOptionsQPage]
            () -> QuPagePtr {
        NameValueOptions options;
        if (QUESTIONS_YN_SPECIFIC_TEXT.contains(q)) {
            const QString fieldname = fieldnameForQuestion(q);
            options = makeOptions(fieldname + "_a", 2);
        } else {
            const QString yes = CommonOptions::yes();
            const QString no = CommonOptions::no();
            if (QUESTIONS_1_NO_2_YES.contains(q)) {
                options.append(NameValuePair(no, 1));
                options.append(NameValuePair(yes, 2));
            } else if (QUESTIONS_1_YES_2_NO.contains(q)) {
                options.append(NameValuePair(yes, 1));
                options.append(NameValuePair(no, 2));
            } else {
                qCritical() << "Bad question to ynQuestion():" << intq;
                return notImplemented();
            }
        }
        return standardOptionsQPage(options);
    };
    auto multiwayQuestion = [this, &q, &intq, &qPage, &notImplemented,
                             &prompttext, &mcq, &makeOptions]
            (bool extra_stem, const QMap<CQ, QPair<int, int>>& first_last_map)
            -> QuPagePtr {
        if (!first_last_map.contains(q)) {
            qCritical() << "Bad question to multiwayQuestion():"
                        << intq << tagForQuestion(q);
            return notImplemented();
        }
        const QPair<int, int>& first_last = first_last_map[q];
        const int& first = first_last.first;
        int last = first_last.second;
        // Nasty hack...
        if (q == CQ::WEIGHT4_INCREASE_PAST_MONTH && isFemale()) {
            last = 3;  // adds pregnancy option
        }
        const QString fieldname = fieldnameForQuestion(q);
        const QString xstring_q = fieldname + XSTRING_QUESTION_SUFFIX;
        NameValueOptions options = makeOptions(fieldname + "_a", last, first);
        QuElement* extra_between_q_and_element = extra_stem
                ? prompttext(fieldname + XSTRING_EXTRA_STEM_SUFFIX)
                : nullptr;
        return qPage(xstring_q, mcq(fieldname, options),
                     extra_between_q_and_element);
    };
    auto panicSymptoms = [this, &qPage]() -> QuPagePtr {
        NameValueOptions options{
            {CommonOptions::no(), 1},
            {CommonOptions::yes(), 2},
        };
        QVector<QuestionWithOneField> q_field_pairs;
        for (const QString& fieldname : panicSymptomFieldnames()) {
            const QString xsymptom = fieldname + "_q";
            const QString stemtext = xstring(xsymptom);
            FieldRefPtr fr = fieldRef(fieldname);
            q_field_pairs.append(QuestionWithOneField(stemtext, fr));
        }
        auto grid = new QuMcqGrid(q_field_pairs, options);
        return qPage("pansym_q_prefix", grid);
    };

    // Now choose our question style.
    if (q == CQ::END_MARKER) {
        return nullptr;
    }
    if (QUESTIONS_MULTIWAY.contains(q)) {
        // this test must PRECEDE Y/N tests since WEIGHT4 is both multiway and Y/N
        return multiwayQuestion(false, QUESTIONS_MULTIWAY);
    }
    if (QUESTIONS_MULTIWAY_WITH_EXTRA_STEM.contains(q)) {
        // this test must PRECEDE Y/N tests since WEIGHT4 is both multiway and Y/N
        return multiwayQuestion(true, QUESTIONS_MULTIWAY_WITH_EXTRA_STEM);
    }
    if (QUESTIONS_PROMPT_ONLY.contains(q)) {
        return promptPage(QUESTIONS_PROMPT_ONLY[q]);
    }
    if (QUESTIONS_OVERALL_DURATION.contains(q)) {
        return overallDuration();
    }
    if (QUESTIONS_1_NO_2_YES.contains(q) || QUESTIONS_1_YES_2_NO.contains(q)) {
        return ynQuestion();
    }
    if (QUESTIONS_DAYS_PER_WEEK.contains(q)) {
        return daysPerWeek();
    }
    if (QUESTIONS_NIGHTS_PER_WEEK.contains(q)) {
        return nightsPerWeek();
    }
    if (QUESTIONS_HOW_UNPLEASANT_STANDARD.contains(q)) {
        return howUnpleasantStandard();
    }
    if (QUESTIONS_FATIGUE_CAUSES.contains(q)) {
        return fatigueCauses();
    }
    if (QUESTIONS_STRESSORS.contains(q)) {
        return stressors();
    }
    if (QUESTIONS_NO_SOMETIMES_OFTEN.contains(q)) {
        return noSometimesOften();
    }
    if (q == CQ::PANSYM) {  // special
        return panicSymptoms();
    }
    return notImplemented();
}


QVector<QString> Cisr::panicSymptomFieldnames() const
{
    QVector<QString> fieldnames;
    for (int i = 0; i < NUM_PANIC_SYMPTOMS; ++i) {
        const QString letter = QChar('a' + i);
        const QString fieldname = QString("pansym_%1").arg(letter);
        fieldnames.append(fieldname);
    }
    return fieldnames;
}


QString Cisr::diagnosisNameLong(int diagnosis_code) const
{
    QString xstring_name = QString("diag_%1_desc").arg(diagnosis_code);
    return xstring(xstring_name);
}


QString Cisr::diagnosisReason(int diagnosis_code) const
{
    QString xstring_name = QString("diag_%1_explan").arg(diagnosis_code);
    return xstring(xstring_name);
}


QString Cisr::suicideIntent(const Cisr::CisrResult& result,
                            bool with_warning) const
{
    QString intent;
    if (result.incomplete) {
        intent = "TASK INCOMPLETE. SO FAR: ";
    }
    intent += xstring(QString("suicid_%1").arg(result.suicidality));
    if (with_warning && result.suicidality >= SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING) {
        intent += QString(" <i>%1</i>").arg(xstring("suicid_instruction"));
    }
    if (result.suicidality != SUICIDE_INTENT_NONE) {
        intent = stringfunc::bold(intent);
    }
    return intent;
}



QString Cisr::functionalImpairment(const Cisr::CisrResult& result) const
{
    const QString xstringname = QString("impair_%1").arg(
                result.functional_impairment);
    return xstring(xstringname);
}



// ============================================================================
// CisrResult
// ============================================================================

void Cisr::CisrResult::decide(const QString& decision)
{
    if (record_decisions) {
        decisions.append(decision);
    }
}


int Cisr::CisrResult::getScore() const
{
    // The original used score-as-we-go. Here we just collect the results:
    return
            somatic_symptoms +
            fatigue +
            concentration_poor +
            sleep_problems +
            irritability +
            hypochondria +
            depression +
            depressive_thoughts +
            worry +
            anxiety +
            phobias_score +
            panic +
            compulsions +
            obsessions;
}


bool Cisr::CisrResult::needsImpairmentQuestion() const
{
    const int threshold = 2;  // for all symptoms
    return
            somatic_symptoms >= threshold ||
            hypochondria >= threshold ||
            fatigue >= threshold ||
            sleep_problems >= threshold ||
            irritability >= threshold ||
            concentration_poor >= threshold ||
            depression >= threshold ||
            depressive_thoughts >= threshold ||
            phobias_score >= threshold ||
            worry >= threshold ||
            anxiety >= threshold ||
            panic >= threshold ||
            compulsions >= threshold ||
            obsessions >= threshold;
}


void Cisr::CisrResult::finalize()
{
    const bool at_least_1_activity_impaired =
            functional_impairment >= OVERALL_IMPAIRMENT_STOP_1_ACTIVITY;
    const int score = getScore();

    // GAD
    if (anxiety >= 2 &&
            anxiety_physical_symptoms &&
            anxiety_at_least_2_weeks) {
        decide("Anxiety score >= 2 AND physical symptoms of anxiety AND "
               "anxiety for at least 2 weeks. "
               "Setting generalized_anxiety_disorder.");
        generalized_anxiety_disorder = true;
    }

    // Panic
    if (panic >= 3 && panic_rapid_onset) {
        decide("Panic score >= 3 AND panic_rapid_onset. "
               "Setting panic_disorder.");
        panic_disorder = true;
    }

    // Phobias
    if (phobias_type == PHOBIATYPES_AGORAPHOBIA &&
            phobic_avoidance &&
            phobias_score >= 2) {
        decide("Phobia type is agoraphobia AND phobic avoidance AND"
               "phobia score >= 2. Setting phobia_agoraphobia.");
        phobia_agoraphobia = true;
    }
    if (phobias_type == PHOBIATYPES_SOCIAL &&
            phobic_avoidance &&
            phobias_score >= 2) {
        decide("Phobia type is social AND phobic avoidance AND"
               "phobia score >= 2. Setting phobia_social.");
        phobia_social = true;
    }
    if (phobias_type == PHOBIATYPES_SOCIAL &&
            phobic_avoidance &&
            phobias_score >= 2) {
        decide("Phobia type is (animals/enclosed/heights OR other) AND "
               "phobic avoidance AND phobia score >= 2. "
               "Setting phobia_specific.");
        phobia_specific = true;
    }

    // OCD
    if (obsessions + compulsions >= 6 &&
            obsessions_tried_to_stop &&
            obsessions_at_least_2_weeks &&
            at_least_1_activity_impaired) {
        decide("obsessions + compulsions >= 6 AND "
               "tried to stop obsessions AND "
               "obsessions for at least 2 weeks AND "
               "at least 1 activity impaired. "
               "Setting obsessive_compulsive_disorder.");
        obsessive_compulsive_disorder = true;
    }
    if (obsessions + compulsions >= 6 &&
            compulsions_tried_to_stop &&
            compulsions_at_least_2_weeks &&
            at_least_1_activity_impaired) {
        decide("obsessions + compulsions >= 6 AND "
               "tried to stop compulsions AND "
               "compulsions for at least 2 weeks AND "
               "at least 1 activity impaired. "
               "Setting obsessive_compulsive_disorder.");
        obsessive_compulsive_disorder = true;
    }
    if (obsessions == 4 &&
            obsessions_tried_to_stop &&
            obsessions_at_least_2_weeks &&
            at_least_1_activity_impaired) {
        // NOTE: 4 is the maximum for obsessions
        decide("obsessions == 4 AND "
               "tried to stop obsessions AND "
               "obsessions for at least 2 weeks AND "
               "at least 1 activity impaired. "
               "Setting obsessive_compulsive_disorder.");
        obsessive_compulsive_disorder = true;
    }
    if (compulsions == 4 &&
            compulsions_tried_to_stop &&
            compulsions_at_least_2_weeks &&
            at_least_1_activity_impaired) {
        // NOTE: 4 is the maximum for compulsions
        decide("compulsions == 4 AND "
               "tried to stop compulsions AND "
               "compulsions for at least 2 weeks AND "
               "at least 1 activity impaired. "
               "Setting obsessive_compulsive_disorder.");
        obsessive_compulsive_disorder = true;
    }

    // Depression
    if (depression_at_least_2_weeks &&
            depr_crit_1_mood_anhedonia_energy > 1 &&
            depr_crit_1_mood_anhedonia_energy + depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 3) {
        decide("Depressive symptoms >=2 weeks AND "
               "depr_crit_1_mood_anhedonia_energy > 1 AND "
               "depr_crit_1_mood_anhedonia_energy + "
               "depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 3. "
               "Setting depression_mild.");
        depression_mild = true;
    }
    if (depression_at_least_2_weeks &&
            depr_crit_1_mood_anhedonia_energy > 1 &&
            (depr_crit_1_mood_anhedonia_energy + depr_crit_2_app_cnc_slp_mtr_glt_wth_sui) > 5) {
        decide("Depressive symptoms >=2 weeks AND "
               "depr_crit_1_mood_anhedonia_energy > 1 AND "
               "depr_crit_1_mood_anhedonia_energy + "
               "depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 5. "
               "Setting depression_moderate.");
        depression_moderate = true;
    }
    if (depression_at_least_2_weeks &&
            depr_crit_1_mood_anhedonia_energy == 3 &&
            depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 4) {
        decide("Depressive symptoms >=2 weeks AND "
               "depr_crit_1_mood_anhedonia_energy == 3 AND "
               "depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 4. "
               "Setting depression_severe.");
        depression_severe = true;
    }

    // CFS
    if (neurasthenia >= 2) {
        // The original had a pointless check for "DIAG1 == 0" too, but that
        // was always true.
        decide("neurasthenia >= 2. Setting chronic_fatigue_syndrome.");
        chronic_fatigue_syndrome = true;
    }

    // Final diagnostic hierarchy

    // ... primary diagnosis
    if (score >= 12) {
        decide("Total score >= 12. Setting diagnosis_1 to "
               "DIAG_1_MIXED_ANX_DEPR_DIS_MILD.");
        diagnosis_1 = DIAG_1_MIXED_ANX_DEPR_DIS_MILD;
    }
    if (generalized_anxiety_disorder) {
        decide("generalized_anxiety_disorder is true. Setting diagnosis_1 to "
               "DIAG_2_GENERALIZED_ANX_DIS_MILD.");
        diagnosis_1 = DIAG_2_GENERALIZED_ANX_DIS_MILD;
    }
    if (obsessive_compulsive_disorder) {
        decide("obsessive_compulsive_disorder is true. Setting diagnosis_1 to "
               "DIAG_3_OBSESSIVE_COMPULSIVE_DIS.");
        diagnosis_1 = DIAG_3_OBSESSIVE_COMPULSIVE_DIS;
    }
    if (score >= 20) {
        decide("Total score >= 20. Setting diagnosis_1 to "
               "DIAG_4_MIXED_ANX_DEPR_DIS.");
        diagnosis_1 = DIAG_4_MIXED_ANX_DEPR_DIS;
    }
    if (phobia_specific) {
        decide("phobia_specific is true. Setting diagnosis_1 to "
               "DIAG_5_SPECIFIC_PHOBIA.");
        diagnosis_1 = DIAG_5_SPECIFIC_PHOBIA;
    }
    if (phobia_social) {
        decide("phobia_social is true. Setting diagnosis_1 to "
               "DIAG_6_SOCIAL_PHOBIA.");
        diagnosis_1 = DIAG_6_SOCIAL_PHOBIA;
    }
    if (phobia_agoraphobia) {
        decide("phobia_agoraphobia is true. Setting diagnosis_1 to "
               "DIAG_7_AGORAPHOBIA.");
        diagnosis_1 = DIAG_7_AGORAPHOBIA;
    }
    if (generalized_anxiety_disorder && score >= 20) {
        decide("generalized_anxiety_disorder is true AND "
               "score >= 20. Setting diagnosis_1 to "
               "DIAG_8_GENERALIZED_ANX_DIS.");
        diagnosis_1 = DIAG_8_GENERALIZED_ANX_DIS;
    }
    if (panic_disorder) {
        decide("panic_disorder is true. Setting diagnosis_1 to "
               "DIAG_9_PANIC_DIS.");
        diagnosis_1 = DIAG_9_PANIC_DIS;
    }
    if (depression_mild) {
        decide("depression_mild is true. Setting diagnosis_1 to "
               "DIAG_10_MILD_DEPR_EPISODE.");
        diagnosis_1 = DIAG_10_MILD_DEPR_EPISODE;
    }
    if (depression_moderate) {
        decide("depression_moderate is true. Setting diagnosis_1 to "
               "DIAG_11_MOD_DEPR_EPISODE.");
        diagnosis_1 = DIAG_11_MOD_DEPR_EPISODE;
    }
    if (depression_severe) {
        decide("depression_severe is true. Setting diagnosis_1 to "
               "DIAG_12_SEVERE_DEPR_EPISODE.");
        diagnosis_1 = DIAG_12_SEVERE_DEPR_EPISODE;
    }

    // ... secondary diagnosis
    if (score >= 12 && diagnosis_1 >= 2) {
        decide("score >= 12 AND diagnosis_1 >= 2. "
               "Setting diagnosis_2 to DIAG_1_MIXED_ANX_DEPR_DIS_MILD.");
        diagnosis_2 = DIAG_1_MIXED_ANX_DEPR_DIS_MILD;
    }
    if (generalized_anxiety_disorder && diagnosis_1 >= 3) {
        decide("generalized_anxiety_disorder is true AND "
               "diagnosis_1 >= 3. "
               "Setting diagnosis_2 to DIAG_2_GENERALIZED_ANX_DIS_MILD.");
        diagnosis_2 = DIAG_2_GENERALIZED_ANX_DIS_MILD;
    }
    if (obsessive_compulsive_disorder && diagnosis_1 >= 4) {
        decide("obsessive_compulsive_disorder is true AND "
               "diagnosis_1 >= 4. "
               "Setting diagnosis_2 to DIAG_3_OBSESSIVE_COMPULSIVE_DIS.");
        diagnosis_2 = DIAG_3_OBSESSIVE_COMPULSIVE_DIS;
    }
    if (score >= 20 && diagnosis_1 >= 5) {
        decide("score >= 20 AND diagnosis_1 >= 5. "
               "Setting diagnosis_2 to DIAG_4_MIXED_ANX_DEPR_DIS.");
        diagnosis_2 = DIAG_4_MIXED_ANX_DEPR_DIS;
    }
    if (phobia_specific && diagnosis_1 >= 6) {
        decide("phobia_specific is true AND diagnosis_1 >= 6. "
               "Setting diagnosis_2 to DIAG_5_SPECIFIC_PHOBIA.");
        diagnosis_2 = DIAG_5_SPECIFIC_PHOBIA;
    }
    if (phobia_social && diagnosis_1 >= 7) {
        decide("phobia_social is true AND diagnosis_1 >= 7. "
               "Setting diagnosis_2 to DIAG_6_SOCIAL_PHOBIA.");
        diagnosis_2 = DIAG_6_SOCIAL_PHOBIA;
    }
    if (phobia_agoraphobia && diagnosis_1 >= 8) {
        decide("phobia_agoraphobia is true AND diagnosis_1 >= 8. "
               "Setting diagnosis_2 to DIAG_7_AGORAPHOBIA.");
        diagnosis_2 = DIAG_7_AGORAPHOBIA;
    }
    if (generalized_anxiety_disorder && score >= 20 && diagnosis_1 >= 9) {
        decide("generalized_anxiety_disorder is true AND "
               "score >= 20 AND "
               "diagnosis_1 >= 9. "
               "Setting diagnosis_2 to DIAG_8_GENERALIZED_ANX_DIS.");
        diagnosis_2 = DIAG_8_GENERALIZED_ANX_DIS;
    }
    if (panic_disorder && diagnosis_1 >= 9) {
        decide("panic_disorder is true AND diagnosis_1 >= 9. "
               "Setting diagnosis_2 to DIAG_9_PANIC_DIS.");
        diagnosis_2 = DIAG_9_PANIC_DIS;
    }

    // In summary:
    const QString scoreprefix = "... ";
    auto showint = [this, &scoreprefix](const QString& name, int value) {
        decide(QString("%1%2: %3").arg(
                   scoreprefix, name, QString::number(value)));
    };
    auto showbool = [this, &scoreprefix](const QString& name, bool value) {
        decide(QString("%1%2: %3").arg(
                   scoreprefix, name, value ? "true" : "false"));
    };
#define SHOWINT(x) showint(#x, x)
#define SHOWBOOL(x) showbool(#x, x)
    decide("FINISHED.");
    decide("--- Final scores:");
    SHOWINT(depression);
    SHOWINT(depr_crit_1_mood_anhedonia_energy);
    SHOWINT(depr_crit_2_app_cnc_slp_mtr_glt_wth_sui);
    SHOWINT(depr_crit_3_somatic_synd);
    SHOWINT(weight_change);
    SHOWINT(somatic_symptoms);
    SHOWINT(fatigue);
    SHOWINT(neurasthenia);
    SHOWINT(concentration_poor);
    SHOWINT(sleep_problems);
    SHOWINT(sleep_change);
    SHOWINT(depressive_thoughts);
    SHOWINT(irritability);
    SHOWINT(diurnal_mood_variation);
    SHOWBOOL(libido_decreased);
    SHOWINT(psychomotor_changes);
    SHOWINT(suicidality);
    SHOWBOOL(depression_at_least_2_weeks);

    SHOWINT(hypochondria);
    SHOWINT(worry);
    SHOWINT(anxiety);
    SHOWBOOL(anxiety_physical_symptoms);
    SHOWBOOL(anxiety_at_least_2_weeks);
    SHOWBOOL(phobias_flag);
    SHOWINT(phobias_score);
    SHOWINT(phobias_type);
    SHOWBOOL(phobic_avoidance);
    SHOWINT(panic);
    SHOWBOOL(panic_rapid_onset);
    SHOWINT(panic_symptoms_total);

    SHOWINT(compulsions);
    SHOWBOOL(compulsions_tried_to_stop);
    SHOWBOOL(compulsions_at_least_2_weeks);
    SHOWINT(obsessions);
    SHOWBOOL(obsessions_tried_to_stop);
    SHOWBOOL(obsessions_at_least_2_weeks);

    SHOWINT(functional_impairment);

    // Disorder flags
    SHOWBOOL(obsessive_compulsive_disorder);
    SHOWBOOL(depression_mild);
    SHOWBOOL(depression_moderate);
    SHOWBOOL(depression_severe);
    SHOWBOOL(chronic_fatigue_syndrome);
    SHOWBOOL(generalized_anxiety_disorder);
    SHOWBOOL(phobia_agoraphobia);
    SHOWBOOL(phobia_social);
    SHOWBOOL(phobia_specific);
    SHOWBOOL(panic_disorder);

    decide("--- Final diagnoses:");
    decide(QString("Probable primary diagnosis: %1").arg(
               diagnosisName(diagnosis_1)));
    decide(QString("Probable secondary diagnosis: %1").arg(
               diagnosisName(diagnosis_2)));
}


QString Cisr::CisrResult::diagnosisName(int diagnosis_code) const
{
    if (incomplete) {
        // Do NOT offer diagnostic information based on partial data.
        // Might be dangerous (e.g. say "mild depressive episode" when it's
        // severe + incomplete information).
        return "INFORMATION INCOMPLETE";
    }
    switch (diagnosis_code) {
    case DIAG_0_NO_DIAGNOSIS:
        return "No diagnosis identified";
    case DIAG_1_MIXED_ANX_DEPR_DIS_MILD:
        return "Mixed anxiety and depressive disorder (mild)";
    case DIAG_2_GENERALIZED_ANX_DIS_MILD:
        return "Generalized anxiety disorder (mild)";
    case DIAG_3_OBSESSIVE_COMPULSIVE_DIS:
        return "Obsessive–compulsive disorder";
    case DIAG_4_MIXED_ANX_DEPR_DIS:
        return "Mixed anxiety and depressive disorder";
    case DIAG_5_SPECIFIC_PHOBIA:
        return "Specific (isolated) phobia";
    case DIAG_6_SOCIAL_PHOBIA:
        return "Social phobia";
    case DIAG_7_AGORAPHOBIA:
        return "Agoraphobia";
    case DIAG_8_GENERALIZED_ANX_DIS:
        return "Generalized anxiety disorder";
    case DIAG_9_PANIC_DIS:
        return "Panic disorder";
    case DIAG_10_MILD_DEPR_EPISODE:
        return "Mild depressive episode";
    case DIAG_11_MOD_DEPR_EPISODE:
        return "Moderate depressive episode";
    case DIAG_12_SEVERE_DEPR_EPISODE:
        return "Severe depressive episode";
    default:
        return "[INTERNAL ERROR: BAD DIAGNOSIS CODE]";
    }
}
