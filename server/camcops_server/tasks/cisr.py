#!/usr/bin/env python

"""
camcops_server/tasks/cisr.py

===============================================================================

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
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

"""

from enum import Enum
import logging
from typing import List, Optional

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.logs import BraceStyleAdapter
import cardinal_pythonlib.rnc_web as ws
from semantic_version import Version
from sqlalchemy.sql.sqltypes import Boolean, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    answer,
    bold,
    get_yes_no,
    get_yes_no_none,
    italic,
    subheading_spanning_two_columns,
    td,
    tr,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ZERO_TO_FOUR_CHECKER,
    ONE_TO_TWO_CHECKER,
    ONE_TO_THREE_CHECKER,
    ONE_TO_FOUR_CHECKER,
    ONE_TO_FIVE_CHECKER,
    ONE_TO_SIX_CHECKER,
    ONE_TO_SEVEN_CHECKER,
    ONE_TO_EIGHT_CHECKER,
    ONE_TO_NINE_CHECKER,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
)

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

DEBUG_SHOW_QUESTIONS_CONSIDERED = True

NOT_APPLICABLE_TEXT = "—"

# -----------------------------------------------------------------------------
# Comments for fields
# -----------------------------------------------------------------------------

CMT_DEMOGRAPHICS = "(Demographics) "
CMT_1_NO_2_YES = " (1 no, 2 yes)"
CMT_1_YES_2_NO = " (1 yes, 2 no)"
CMT_DURATION = (" (1: <2 weeks; 2: 2 weeks–6 months; 3: 6 months–1 year; "
                "4: 1–2 years; 5: >2 years)")
CMT_NEVER_SOMETIMES_ALWAYS = " (1 never, 2 sometimes, 3 always)"
CMT_DAYS_PER_WEEK = " (1: none, 2: one to three, 3: four or more)"
CMT_NIGHTS_PER_WEEK = CMT_DAYS_PER_WEEK
CMT_UNPLEASANT = (" (1 not at all, 2 a little unpleasant, 3 unpleasant, "
                  "4 very unpleasant)")
CMT_NO_SOMETIMES_OFTEN = " (1 no, 2 sometimes, 3 often)"
CMT_BOTHERSOME_INTERESTING = (" (1 no, 2 yes, 3 haven't done anything "
                              "interesting)")
CMT_DURING_ENJOYABLE = " (1 no, 2 yes, 3 haven't done anything enjoyable)"
CMT_FATIGUE_CAUSE = (
    " (1 problems with sleep; 2 medication; 3 physical illness; 4 working too "
    "hard inc. childcare; 5 stress/worry/other psychological; 6 physical "
    "exercise; 7 other; 8 don't know)"
)
CMT_STRESSORS = (
    " (1 family members; 2 relationships with friends/colleagues; 3 housing; "
    "4 money/bills; 5 own physical health; 6 own mental health; 7 work/lack "
    "of work; 8 legal; 9 political/news)"
)
CMT_SLEEP_CHANGE = " (1: <15min, 2: 15–60min, 3: 1–3h, 4: >=3h)"
CMT_ANHEDONIA = (" (1 yes; 2 no, less enjoyment than usual; "
                 "3 no, don't enjoy anything)")
CMT_PANIC_SYMPTOM = "Panic symptom in past week: "

# ... and results:
DESC_DEPCRIT1 = "Depressive criterion 1 (mood, anhedonia, energy; max. 3)"
DESC_DEPCRIT2 = ("Depressive criterion 2 (appetite/weight, concentration, "
                 "sleep, motor, guilt, self-worth, suicidality; max. 7)")
DESC_DEPCRIT3 = (
    "Depressive criterion 3: somatic syndrome (anhedonia, lack of emotional "
    "reactivity, early-morning waking, depression worse in the morning, "
    "psychomotor retardation/agitation, loss of appetite, weight loss, loss "
    "of libido; max. 8)"
)
DESC_DEPCRIT3_MET = "Somatic syndrome criterion met (≥4)?"
DESC_NEURASTHENIA_SCORE = "Neurasthenia score (max. 3)"

DISORDER_OCD = "Obsessive–compulsive disorder"
DISORDER_DEPR_MILD = "Depressive episode: at least mild"
DISORDER_DEPR_MOD = "Depressive episode: at least moderate"
DISORDER_DEPR_SEV = "Depressive episode: severe"
DISORDER_CFS = "Chronic fatigue syndrome"
DISORDER_GAD = "Generalized anxiety disorder"
DISORDER_AGORAPHOBIA = "Agoraphobia"
DISORDER_SOCIAL_PHOBIA = "Social phobia"
DISORDER_SPECIFIC_PHOBIA = "Specific phobia"
DISORDER_PANIC = "Panic disorder"

# -----------------------------------------------------------------------------
# Number of response values (numbered from 1 to N)
# -----------------------------------------------------------------------------

N_DURATIONS = 5
N_OPTIONS_DAYS_PER_WEEK = 3
N_OPTIONS_NIGHTS_PER_WEEK = 3
N_OPTIONS_HOW_UNPLEASANT = 4
N_OPTIONS_FATIGUE_CAUSES = 8
N_OPTIONS_STRESSORS = 9
N_OPTIONS_NO_SOMETIMES_OFTEN = 3
NUM_PANIC_SYMPTOMS = 13  # from a to m

# -----------------------------------------------------------------------------
# Actual response values
# -----------------------------------------------------------------------------

# For e.g. SOMATIC_DUR, etc.: "How long have you..."
V_DURATION_LT_2W = 1
V_DURATION_2W_6M = 2
V_DURATION_6M_1Y = 3
V_DURATION_1Y_2Y = 4
V_DURATION_GE_2Y = 5

# For quite a few: "on how many days in the past week...?"
V_DAYS_IN_PAST_WEEK_0 = 1
V_DAYS_IN_PAST_WEEK_1_TO_3 = 2
V_DAYS_IN_PAST_WEEK_4_OR_MORE = 3

V_NIGHTS_IN_PAST_WEEK_0 = 1
V_NIGHTS_IN_PAST_WEEK_1_TO_3 = 2
V_NIGHTS_IN_PAST_WEEK_4_OR_MORE = 3

V_HOW_UNPLEASANT_NOT_AT_ALL = 1
V_HOW_UNPLEASANT_A_LITTLE = 2
V_HOW_UNPLEASANT_UNPLEASANT = 3
V_HOW_UNPLEASANT_VERY = 4

V_FATIGUE_CAUSE_SLEEP = 1
V_FATIGUE_CAUSE_MEDICATION = 2
V_FATIGUE_CAUSE_PHYSICAL_ILLNESS = 3
V_FATIGUE_CAUSE_OVERWORK = 4
V_FATIGUE_CAUSE_PSYCHOLOGICAL = 5
V_FATIGUE_CAUSE_EXERCISE = 6
V_FATIGUE_CAUSE_OTHER = 7
V_FATIGUE_CAUSE_DONT_KNOW = 8

V_STRESSOR_FAMILY = 1
V_STRESSOR_FRIENDS_COLLEAGUES = 2
V_STRESSOR_HOUSING = 3
V_STRESSOR_MONEY = 4
V_STRESSOR_PHYSICAL_HEALTH = 5
V_STRESSOR_MENTAL_HEALTH = 6
V_STRESSOR_WORK = 7
V_STRESSOR_LEGAL = 8
V_STRESSOR_POLITICAL_NEWS = 9

V_NSO_NO = 1
V_NSO_SOMETIMES = 2
V_NSO_OFTEN = 3

V_SLEEP_CHANGE_LT_15_MIN = 1
V_SLEEP_CHANGE_15_MIN_TO_1_H = 2
V_SLEEP_CHANGE_1_TO_3_H = 3
V_SLEEP_CHANGE_GT_3_H = 4

V_ANHEDONIA_ENJOYING_NORMALLY = 1
V_ANHEDONIA_ENJOYING_LESS = 2
V_ANHEDONIA_NOT_ENJOYING = 3

# Specific other question values:

V_EMPSTAT_FT = 1  # unused
V_EMPSTAT_PT = 2  # unused
V_EMPSTAT_STUDENT = 3  # unused
V_EMPSTAT_RETIRED = 4  # unused
V_EMPSTAT_HOUSEPERSON = 5  # unused
V_EMPSTAT_UNEMPJOBSEEKER = 6  # unused
V_EMPSTAT_UNEMPILLHEALTH = 7  # unused

V_EMPTYPE_SELFEMPWITHEMPLOYEES = 1  # unused
V_EMPTYPE_SELFEMPNOEMPLOYEES = 2  # unused
V_EMPTYPE_EMPLOYEE = 3  # unused
V_EMPTYPE_SUPERVISOR = 4  # unused
V_EMPTYPE_MANAGER = 5  # unused
V_EMPTYPE_NOT_APPLICABLE = 6  # unused
# ... the last one: added by RNC, in case pt never employed. (Mentioned to
# Glyn Lewis 2017-12-04. Not, in any case, part of the important bits of the
# CIS-R.)

V_HOME_OWNER = 1  # unused
V_HOME_TENANT = 2  # unused
V_HOME_RELATIVEFRIEND = 3  # unused
V_HOME_HOSTELCAREHOME = 4  # unused
V_HOME_HOMELESS = 5  # unused
V_HOME_OTHER = 6  # unused

V_WEIGHT2_WTLOSS_NOTTRYING = 1
V_WEIGHT2_WTLOSS_TRYING = 2

V_WEIGHT3_WTLOSS_GE_HALF_STONE = 1
V_WEIGHT3_WTLOSS_LT_HALF_STONE = 2

V_WEIGHT4_WTGAIN_YES_PREGNANT = 3

V_WEIGHT5_WTGAIN_GE_HALF_STONE = 1
V_WEIGHT5_WTGAIN_LT_HALF_STONE = 2

V_GPYEAR_NONE = 0
V_GPYEAR_1_2 = 1
V_GPYEAR_3_5 = 2
V_GPYEAR_6_10 = 3
V_GPYEAR_GT_10 = 4

V_ILLNESS_DIABETES = 1
V_ILLNESS_ASTHMA = 2
V_ILLNESS_ARTHRITIS = 3
V_ILLNESS_HEART_DISEASE = 4
V_ILLNESS_HYPERTENSION = 5
V_ILLNESS_LUNG_DISEASE = 6
V_ILLNESS_MORE_THAN_ONE = 7
V_ILLNESS_NONE = 8

V_SOMATIC_PAIN1_NEVER = 1
V_SOMATIC_PAIN1_SOMETIMES = 2
V_SOMATIC_PAIN1_ALWAYS = 3

V_SOMATIC_PAIN3_LT_3H = 1
V_SOMATIC_PAIN3_GT_3H = 2

V_SOMATIC_PAIN4_NOT_AT_ALL = 1
V_SOMATIC_PAIN4_LITTLE_UNPLEASANT = 2
V_SOMATIC_PAIN4_UNPLEASANT = 3
V_SOMATIC_PAIN4_VERY_UNPLEASANT = 4

V_SOMATIC_PAIN5_NO = 1
V_SOMATIC_PAIN5_YES = 2
V_SOMATIC_PAIN5_NOT_DONE_ANYTHING_INTERESTING = 3

V_SOMATIC_MAND2_NO = 1
V_SOMATIC_MAND2_YES = 2

V_SOMATIC_DIS1_NEVER = 1
V_SOMATIC_DIS1_SOMETIMES = 2
V_SOMATIC_DIS1_ALWAYS = 3

V_SOMATIC_DIS2_NONE = 1
V_SOMATIC_DIS2_1_TO_3_DAYS = 2
V_SOMATIC_DIS2_4_OR_MORE_DAYS = 3

V_SOMATIC_DIS3_LT_3H = 1
V_SOMATIC_DIS3_GT_3H = 2

V_SOMATIC_DIS4_NOT_AT_ALL = 1
V_SOMATIC_DIS4_LITTLE_UNPLEASANT = 2
V_SOMATIC_DIS4_UNPLEASANT = 3
V_SOMATIC_DIS4_VERY_UNPLEASANT = 4

V_SOMATIC_DIS5_NO = 1
V_SOMATIC_DIS5_YES = 2
V_SOMATIC_DIS5_NOT_DONE_ANYTHING_INTERESTING = 3

V_SLEEP_MAND2_NO = 1
V_SLEEP_MAND2_YES_BUT_NOT_A_PROBLEM = 2
V_SLEEP_MAND2_YES = 3

V_IRRIT_MAND2_NO = 1
V_IRRIT_MAND2_SOMETIMES = 2
V_IRRIT_MAND2_YES = 3

V_IRRIT3_SHOUTING_NO = 1
V_IRRIT3_SHOUTING_WANTED_TO = 2
V_IRRIT3_SHOUTING_DID = 3

V_IRRIT4_ARGUMENTS_NO = 1
V_IRRIT4_ARGUMENTS_YES_JUSTIFIED = 2
V_IRRIT4_ARGUMENTS_YES_UNJUSTIFIED = 3

V_DEPR5_COULD_CHEER_UP_YES = 1
V_DEPR5_COULD_CHEER_UP_SOMETIMES = 2
V_DEPR5_COULD_CHEER_UP_NO = 3

V_DEPTH1_DMV_WORSE_MORNING = 1
V_DEPTH1_DMV_WORSE_EVENING = 2
V_DEPTH1_DMV_VARIES = 3
V_DEPTH1_DMV_NONE = 4

V_DEPTH2_LIBIDO_NA = 1
V_DEPTH2_LIBIDO_NO_CHANGE = 2
V_DEPTH2_LIBIDO_INCREASED = 3
V_DEPTH2_LIBIDO_DECREASED = 4

V_DEPTH5_GUILT_NEVER = 1
V_DEPTH5_GUILT_WHEN_AT_FAULT = 2
V_DEPTH5_GUILT_SOMETIMES = 3
V_DEPTH5_GUILT_OFTEN = 4

V_DEPTH8_LNWL_NO = 1
V_DEPTH8_LNWL_SOMETIMES = 2
V_DEPTH8_LNWL_ALWAYS = 3

V_DEPTH9_SUICIDAL_THOUGHTS_NO = 1
V_DEPTH9_SUICIDAL_THOUGHTS_YES_BUT_NEVER_WOULD = 2
V_DEPTH9_SUICIDAL_THOUGHTS_YES = 3

V_DOCTOR_YES = 1
V_DOCTOR_NO_BUT_OTHERS = 2
V_DOCTOR_NO = 3

V_ANX_PHOBIA2_ALWAYS_SPECIFIC = 1
V_ANX_PHOBIA2_SOMETIMES_GENERAL = 2

V_PHOBIAS_TYPE1_ALONE_PUBLIC_TRANSPORT = 1
V_PHOBIAS_TYPE1_FAR_FROM_HOME = 2
V_PHOBIAS_TYPE1_PUBLIC_SPEAKING_EATING = 3
V_PHOBIAS_TYPE1_BLOOD = 4
V_PHOBIAS_TYPE1_CROWDED_SHOPS = 5
V_PHOBIAS_TYPE1_ANIMALS = 6
V_PHOBIAS_TYPE1_BEING_WATCHED = 7
V_PHOBIAS_TYPE1_ENCLOSED_SPACES_HEIGHTS = 8
V_PHOBIAS_TYPE1_OTHER = 9

V_PANIC1_N_PANICS_PAST_WEEK_0 = 1
V_PANIC1_N_PANICS_PAST_WEEK_1 = 2
V_PANIC1_N_PANICS_PAST_WEEK_GT_1 = 3

V_PANIC3_WORST_LT_10_MIN = 1
V_PANIC3_WORST_GE_10_MIN = 2

V_COMP4_MAX_N_REPEATS_1 = 1
V_COMP4_MAX_N_REPEATS_2 = 2
V_COMP4_MAX_N_REPEATS_GE_3 = 3

V_OBSESS_MAND1_SAME_THOUGHTS_REPEATED = 1
V_OBSESS_MAND1_GENERAL_WORRIES = 2

V_OBSESS4_LT_15_MIN = 1
V_OBSESS4_GE_15_MIN = 2

V_OVERALL_IMPAIRMENT_NONE = 1
V_OVERALL_IMPAIRMENT_DIFFICULT = 2
V_OVERALL_IMPAIRMENT_STOP_1_ACTIVITY = 3
V_OVERALL_IMPAIRMENT_STOP_GT_1_ACTIVITY = 4

# -----------------------------------------------------------------------------
# Internal coding, NOT answer values:
# -----------------------------------------------------------------------------

# Magic numbers from the original:
WTCHANGE_NONE_OR_APPETITE_INCREASE = 0
WTCHANGE_APPETITE_LOSS = 1
WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN = 2
WTCHANGE_WTLOSS_GE_HALF_STONE = 3
WTCHANGE_WTGAIN_GE_HALF_STONE = 4
# ... I'm not entirely sure why this labelling system is used!

DESC_WEIGHT_CHANGE = (
    "Weight change "
    f"({WTCHANGE_NONE_OR_APPETITE_INCREASE}: none or appetite increase; "
    f"{WTCHANGE_APPETITE_LOSS}: appetite loss without weight loss; "
    f"{WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN}: "
    "non-deliberate weight loss or gain <0.5 st; "
    f"{WTCHANGE_WTLOSS_GE_HALF_STONE}: weight loss ≥0.5 st; "
    f"{WTCHANGE_WTGAIN_GE_HALF_STONE}: weight gain ≥0.5 st)"
)

PHOBIATYPES_OTHER = 0
PHOBIATYPES_AGORAPHOBIA = 1
PHOBIATYPES_SOCIAL = 2
PHOBIATYPES_BLOOD_INJURY = 3
PHOBIATYPES_ANIMALS_ENCLOSED_HEIGHTS = 4
# ... some of these are not really used, but I've followed the original CIS-R
# for clarity

# One smaller than the answer codes:
OVERALL_IMPAIRMENT_NONE = 0
OVERALL_IMPAIRMENT_DIFFICULT = 1
OVERALL_IMPAIRMENT_STOP_1_ACTIVITY = 2
OVERALL_IMPAIRMENT_STOP_GT_1_ACTIVITY = 3

# Again, we're following this coding structure primarily for compatibility:
DIAG_0_NO_DIAGNOSIS = 0
DIAG_1_MIXED_ANX_DEPR_DIS_MILD = 1
DIAG_2_GENERALIZED_ANX_DIS_MILD = 2
DIAG_3_OBSESSIVE_COMPULSIVE_DIS = 3
DIAG_4_MIXED_ANX_DEPR_DIS = 4
DIAG_5_SPECIFIC_PHOBIA = 5
DIAG_6_SOCIAL_PHOBIA = 6
DIAG_7_AGORAPHOBIA = 7
DIAG_8_GENERALIZED_ANX_DIS = 8
DIAG_9_PANIC_DIS = 9
DIAG_10_MILD_DEPR_EPISODE = 10
DIAG_11_MOD_DEPR_EPISODE = 11
DIAG_12_SEVERE_DEPR_EPISODE = 12

SUICIDE_INTENT_NONE = 0
SUICIDE_INTENT_HOPELESS_NO_SUICIDAL_THOUGHTS = 1
SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING = 2
SUICIDE_INTENT_SUICIDAL_THOUGHTS = 3
SUICIDE_INTENT_SUICIDAL_PLANS = 4

SLEEPCHANGE_NONE = 0  # added
SLEEPCHANGE_EMW = 1
SLEEPCHANGE_INSOMNIA_NOT_EMW = 2
SLEEPCHANGE_INCREASE = 3

DESC_SLEEP_CHANGE = (
    f"Sleep change ({SLEEPCHANGE_NONE}: none; "
    f"{SLEEPCHANGE_EMW}: early-morning waking; "
    f"{SLEEPCHANGE_INSOMNIA_NOT_EMW}: insomnia without early-morning waking; "
    f"{SLEEPCHANGE_INCREASE}: sleep increase)"
)

DIURNAL_MOOD_VAR_NONE = 0
DIURNAL_MOOD_VAR_WORSE_MORNING = 1
DIURNAL_MOOD_VAR_WORSE_EVENING = 2

PSYCHOMOTOR_NONE = 0
PSYCHOMOTOR_RETARDATION = 1
PSYCHOMOTOR_AGITATION = 2

# Answer values or pseudo-values:

V_MISSING = 0  # Integer value of a missing answer
V_UNKNOWN = -1  # Dummy value, never used for answers

SCORE_PREFIX = "... "

# -----------------------------------------------------------------------------
# Scoring constants:
# -----------------------------------------------------------------------------

MAX_TOTAL = 57

MAX_SOMATIC = 4
MAX_HYPO = 4
MAX_IRRIT = 4
MAX_CONC = 4
MAX_FATIGUE = 4
MAX_SLEEP = 4
MAX_DEPR = 4
MAX_DEPTHTS = 5
MAX_PHOBIAS = 4
MAX_WORRY = 4
MAX_ANX = 4
MAX_PANIC = 4
MAX_COMP = 4
MAX_OBSESS = 4
MAX_DEPCRIT1 = 3
MAX_DEPCRIT2 = 7
MAX_DEPCRIT3 = 8

SOMATIC_SYNDROME_CRITERION = 4  # number of symptoms

# -----------------------------------------------------------------------------
# Question numbers
# -----------------------------------------------------------------------------

# Not quite sure to do an autonumbering enum that also can have synonyms, like
# C++. The AutoNumberEnum (q.v.) is close, but won't do the synonyms. So:

_nasty_hack_next_enum = 1  # start with 1


def next_enum() -> int:
    global _nasty_hack_next_enum
    v = _nasty_hack_next_enum
    _nasty_hack_next_enum += 1
    return v


class CisrQuestion(Enum):
    # The values below look like integers, but they aren't; they are of type
    # CisrQuestion, and have attributes like ".value".
    START_MARKER = next_enum()

    INTRO_1 = START_MARKER
    INTRO_2 = next_enum()
    INTRO_DEMOGRAPHICS = next_enum()

    ETHNIC = next_enum()
    MARRIED = next_enum()
    EMPSTAT = next_enum()
    EMPTYPE = next_enum()
    HOME = next_enum()
    HEALTH_WELLBEING = next_enum()

    APPETITE1_LOSS_PAST_MONTH = next_enum()

    WEIGHT1_LOSS_PAST_MONTH = next_enum()
    WEIGHT2_TRYING_TO_LOSE = next_enum()
    WEIGHT3_LOST_LOTS = next_enum()
    APPETITE2_INCREASE_PAST_MONTH = next_enum()
    WEIGHT4_INCREASE_PAST_MONTH = next_enum()
    # WEIGHT4A = WEIGHT4 with pregnancy question; blended
    WEIGHT5_GAINED_LOTS = next_enum()
    GP_YEAR = next_enum()
    DISABLE = next_enum()
    ILLNESS = next_enum()

    SOMATIC_MAND1_PAIN_PAST_MONTH = next_enum()
    SOMATIC_PAIN1_PSYCHOL_EXAC = next_enum()
    SOMATIC_PAIN2_DAYS_PAST_WEEK = next_enum()
    SOMATIC_PAIN3_GT_3H_ANY_DAY = next_enum()
    SOMATIC_PAIN4_UNPLEASANT = next_enum()
    SOMATIC_PAIN5_INTERRUPTED_INTERESTING = next_enum()
    SOMATIC_MAND2_DISCOMFORT = next_enum()
    SOMATIC_DIS1_PSYCHOL_EXAC = next_enum()
    SOMATIC_DIS2_DAYS_PAST_WEEK = next_enum()
    SOMATIC_DIS3_GT_3H_ANY_DAY = next_enum()
    SOMATIC_DIS4_UNPLEASANT = next_enum()
    SOMATIC_DIS5_INTERRUPTED_INTERESTING = next_enum()
    SOMATIC_DUR = next_enum()

    FATIGUE_MAND1_TIRED_PAST_MONTH = next_enum()
    FATIGUE_CAUSE1_TIRED = next_enum()
    FATIGUE_TIRED1_DAYS_PAST_WEEK = next_enum()
    FATIGUE_TIRED2_GT_3H_ANY_DAY = next_enum()
    FATIGUE_TIRED3_HAD_TO_PUSH = next_enum()
    FATIGUE_TIRED4_DURING_ENJOYABLE = next_enum()
    FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH = next_enum()
    FATIGUE_CAUSE2_LACK_ENERGY = next_enum()
    FATIGUE_ENERGY1_DAYS_PAST_WEEK = next_enum()
    FATIGUE_ENERGY2_GT_3H_ANY_DAY = next_enum()
    FATIGUE_ENERGY3_HAD_TO_PUSH = next_enum()
    FATIGUE_ENERGY4_DURING_ENJOYABLE = next_enum()
    FATIGUE_DUR = next_enum()

    CONC_MAND1_POOR_CONC_PAST_MONTH = next_enum()
    CONC_MAND2_FORGETFUL_PAST_MONTH = next_enum()
    CONC1_CONC_DAYS_PAST_WEEK = next_enum()
    CONC2_CONC_FOR_TV_READING_CONVERSATION = next_enum()
    CONC3_CONC_PREVENTED_ACTIVITIES = next_enum()
    CONC_DUR = next_enum()
    CONC4_FORGOTTEN_IMPORTANT = next_enum()
    FORGET_DUR = next_enum()

    SLEEP_MAND1_LOSS_PAST_MONTH = next_enum()
    SLEEP_LOSE1_NIGHTS_PAST_WEEK = next_enum()
    SLEEP_LOSE2_DIS_WORST_DURATION = next_enum()  # DIS = delayed initiation of sleep  # noqa
    SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK = next_enum()
    SLEEP_EMW_PAST_WEEK = next_enum()  # EMW = early-morning waking
    SLEEP_CAUSE = next_enum()
    SLEEP_MAND2_GAIN_PAST_MONTH = next_enum()
    SLEEP_GAIN1_NIGHTS_PAST_WEEK = next_enum()
    SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT = next_enum()
    SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK = next_enum()
    SLEEP_DUR = next_enum()

    IRRIT_MAND1_PEOPLE_PAST_MONTH = next_enum()
    IRRIT_MAND2_THINGS_PAST_MONTH = next_enum()
    IRRIT1_DAYS_PER_WEEK = next_enum()
    IRRIT2_GT_1H_ANY_DAY = next_enum()
    IRRIT3_WANTED_TO_SHOUT = next_enum()
    IRRIT4_ARGUMENTS = next_enum()
    IRRIT_DUR = next_enum()

    HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH = next_enum()
    HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS = next_enum()
    HYPO1_DAYS_PAST_WEEK = next_enum()
    HYPO2_WORRY_TOO_MUCH = next_enum()
    HYPO3_HOW_UNPLEASANT = next_enum()
    HYPO4_CAN_DISTRACT = next_enum()
    HYPO_DUR = next_enum()

    DEPR_MAND1_LOW_MOOD_PAST_MONTH = next_enum()
    DEPR1_LOW_MOOD_PAST_WEEK = next_enum()
    DEPR_MAND2_ENJOYMENT_PAST_MONTH = next_enum()
    DEPR2_ENJOYMENT_PAST_WEEK = next_enum()
    DEPR3_DAYS_PAST_WEEK = next_enum()
    DEPR4_GT_3H_ANY_DAY = next_enum()
    DEPR_CONTENT = next_enum()
    DEPR5_COULD_CHEER_UP = next_enum()
    DEPR_DUR = next_enum()
    DEPTH1_DIURNAL_VARIATION = next_enum()  # "depth" = depressive thoughts?
    DEPTH2_LIBIDO = next_enum()
    DEPTH3_RESTLESS = next_enum()
    DEPTH4_SLOWED = next_enum()
    DEPTH5_GUILT = next_enum()
    DEPTH6_WORSE_THAN_OTHERS = next_enum()
    DEPTH7_HOPELESS = next_enum()
    DEPTH8_LNWL = next_enum()  # life not worth living
    DEPTH9_SUICIDE_THOUGHTS = next_enum()
    DEPTH10_SUICIDE_METHOD = next_enum()
    DOCTOR = next_enum()
    DOCTOR2_PLEASE_TALK_TO = next_enum()
    DEPR_OUTRO = next_enum()

    WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH = next_enum()
    WORRY_MAND2_ANY_WORRIES_PAST_MONTH = next_enum()
    WORRY_CONT1 = next_enum()
    WORRY1_INFO_ONLY = next_enum()
    WORRY2_DAYS_PAST_WEEK = next_enum()
    WORRY3_TOO_MUCH = next_enum()
    WORRY4_HOW_UNPLEASANT = next_enum()
    WORRY5_GT_3H_ANY_DAY = next_enum()
    WORRY_DUR = next_enum()

    ANX_MAND1_ANXIETY_PAST_MONTH = next_enum()
    ANX_MAND2_TENSION_PAST_MONTH = next_enum()
    ANX_PHOBIA1_SPECIFIC_PAST_MONTH = next_enum()
    ANX_PHOBIA2_SPECIFIC_OR_GENERAL = next_enum()
    ANX1_INFO_ONLY = next_enum()
    ANX2_GENERAL_DAYS_PAST_WEEK = next_enum()
    ANX3_GENERAL_HOW_UNPLEASANT = next_enum()
    ANX4_GENERAL_PHYSICAL_SYMPTOMS = next_enum()
    ANX5_GENERAL_GT_3H_ANY_DAY = next_enum()
    ANX_DUR_GENERAL = next_enum()

    PHOBIAS_MAND_AVOIDANCE_PAST_MONTH = next_enum()
    PHOBIAS_TYPE1 = next_enum()
    PHOBIAS1_DAYS_PAST_WEEK = next_enum()
    PHOBIAS2_PHYSICAL_SYMPTOMS = next_enum()
    PHOBIAS3_AVOIDANCE = next_enum()
    PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK = next_enum()
    PHOBIAS_DUR = next_enum()

    PANIC_MAND_PAST_MONTH = next_enum()
    PANIC1_NUM_PAST_WEEK = next_enum()
    PANIC2_HOW_UNPLEASANT = next_enum()
    PANIC3_PANIC_GE_10_MIN = next_enum()
    PANIC4_RAPID_ONSET = next_enum()
    PANSYM = next_enum()  # questions about each of several symptoms
    PANIC5_ALWAYS_SPECIFIC_TRIGGER = next_enum()
    PANIC_DUR = next_enum()

    ANX_OUTRO = next_enum()

    COMP_MAND1_COMPULSIONS_PAST_MONTH = next_enum()
    COMP1_DAYS_PAST_WEEK = next_enum()
    COMP2_TRIED_TO_STOP = next_enum()
    COMP3_UPSETTING = next_enum()
    COMP4_MAX_N_REPETITIONS = next_enum()
    COMP_DUR = next_enum()

    OBSESS_MAND1_OBSESSIONS_PAST_MONTH = next_enum()
    OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL = next_enum()
    OBSESS1_DAYS_PAST_WEEK = next_enum()
    OBSESS2_TRIED_TO_STOP = next_enum()
    OBSESS3_UPSETTING = next_enum()
    OBSESS4_MAX_DURATION = next_enum()
    OBSESS_DUR = next_enum()

    OVERALL1_INFO_ONLY = next_enum()
    OVERALL2_IMPACT_PAST_WEEK = next_enum()
    THANKS_FINISHED = next_enum()
    END_MARKER = next_enum()  # not a real page


CQ = CisrQuestion  # shorthand

# Demographics section
FN_ETHNIC = "ethnic"
FN_MARRIED = "married"
FN_EMPSTAT = "empstat"
FN_EMPTYPE = "emptype"
FN_HOME = "home"

FN_APPETITE1 = "appetite1"
FN_WEIGHT1 = "weight1"
FN_WEIGHT2 = "weight2"
FN_WEIGHT3 = "weight3"
FN_APPETITE2 = "appetite2"
FN_WEIGHT4 = "weight4"  # male/female responses unified (no "weight4a"
FN_WEIGHT5 = "weight5"

FN_GP_YEAR = "gp_year"
FN_DISABLE = "disable"
FN_ILLNESS = "illness"

FN_SOMATIC_MAND1 = "somatic_mand1"
FN_SOMATIC_PAIN1 = "somatic_pain1"
FN_SOMATIC_PAIN2 = "somatic_pain2"
FN_SOMATIC_PAIN3 = "somatic_pain3"
FN_SOMATIC_PAIN4 = "somatic_pain4"
FN_SOMATIC_PAIN5 = "somatic_pain5"
FN_SOMATIC_MAND2 = "somatic_mand2"
FN_SOMATIC_DIS1 = "somatic_dis1"
FN_SOMATIC_DIS2 = "somatic_dis2"
FN_SOMATIC_DIS3 = "somatic_dis3"
FN_SOMATIC_DIS4 = "somatic_dis4"
FN_SOMATIC_DIS5 = "somatic_dis5"
FN_SOMATIC_DUR = "somatic_dur"

FN_FATIGUE_MAND1 = "fatigue_mand1"
FN_FATIGUE_CAUSE1 = "fatigue_cause1"
FN_FATIGUE_TIRED1 = "fatigue_tired1"
FN_FATIGUE_TIRED2 = "fatigue_tired2"
FN_FATIGUE_TIRED3 = "fatigue_tired3"
FN_FATIGUE_TIRED4 = "fatigue_tired4"
FN_FATIGUE_MAND2 = "fatigue_mand2"
FN_FATIGUE_CAUSE2 = "fatigue_cause2"
FN_FATIGUE_ENERGY1 = "fatigue_energy1"
FN_FATIGUE_ENERGY2 = "fatigue_energy2"
FN_FATIGUE_ENERGY3 = "fatigue_energy3"
FN_FATIGUE_ENERGY4 = "fatigue_energy4"
FN_FATIGUE_DUR = "fatigue_dur"

FN_CONC_MAND1 = "conc_mand1"
FN_CONC_MAND2 = "conc_mand2"
FN_CONC1 = "conc1"
FN_CONC2 = "conc2"
FN_CONC3 = "conc3"
FN_CONC_DUR = "conc_dur"
FN_CONC4 = "conc4"
FN_FORGET_DUR = "forget_dur"

FN_SLEEP_MAND1 = "sleep_mand1"
FN_SLEEP_LOSE1 = "sleep_lose1"
FN_SLEEP_LOSE2 = "sleep_lose2"
FN_SLEEP_LOSE3 = "sleep_lose3"
FN_SLEEP_EMW = "sleep_emw"
FN_SLEEP_CAUSE = "sleep_cause"
FN_SLEEP_MAND2 = "sleep_mand2"
FN_SLEEP_GAIN1 = "sleep_gain1"
FN_SLEEP_GAIN2 = "sleep_gain2"
FN_SLEEP_GAIN3 = "sleep_gain3"
FN_SLEEP_DUR = "sleep_dur"

FN_IRRIT_MAND1 = "irrit_mand1"
FN_IRRIT_MAND2 = "irrit_mand2"
FN_IRRIT1 = "irrit1"
FN_IRRIT2 = "irrit2"
FN_IRRIT3 = "irrit3"
FN_IRRIT4 = "irrit4"
FN_IRRIT_DUR = "irrit_dur"

FN_HYPO_MAND1 = "hypo_mand1"
FN_HYPO_MAND2 = "hypo_mand2"
FN_HYPO1 = "hypo1"
FN_HYPO2 = "hypo2"
FN_HYPO3 = "hypo3"
FN_HYPO4 = "hypo4"
FN_HYPO_DUR = "hypo_dur"

FN_DEPR_MAND1 = "depr_mand1"
FN_DEPR1 = "depr1"
FN_DEPR_MAND2 = "depr_mand2"
FN_DEPR2 = "depr2"
FN_DEPR3 = "depr3"
FN_DEPR4 = "depr4"
FN_DEPR_CONTENT = "depr_content"
FN_DEPR5 = "depr5"
FN_DEPR_DUR = "depr_dur"
FN_DEPTH1 = "depth1"
FN_DEPTH2 = "depth2"
FN_DEPTH3 = "depth3"
FN_DEPTH4 = "depth4"
FN_DEPTH5 = "depth5"
FN_DEPTH6 = "depth6"
FN_DEPTH7 = "depth7"
FN_DEPTH8 = "depth8"
FN_DEPTH9 = "depth9"
FN_DEPTH10 = "depth10"
FN_DOCTOR = "doctor"

FN_WORRY_MAND1 = "worry_mand1"
FN_WORRY_MAND2 = "worry_mand2"
FN_WORRY_CONT1 = "worry_cont1"
FN_WORRY2 = "worry2"
FN_WORRY3 = "worry3"
FN_WORRY4 = "worry4"
FN_WORRY5 = "worry5"
FN_WORRY_DUR = "worry_dur"

FN_ANX_MAND1 = "anx_mand1"
FN_ANX_MAND2 = "anx_mand2"
FN_ANX_PHOBIA1 = "anx_phobia1"
FN_ANX_PHOBIA2 = "anx_phobia2"
FN_ANX2 = "anx2"
FN_ANX3 = "anx3"
FN_ANX4 = "anx4"
FN_ANX5 = "anx5"
FN_ANX_DUR = "anx_dur"

FN_PHOBIAS_MAND = "phobias_mand"
FN_PHOBIAS_TYPE1 = "phobias_type1"
FN_PHOBIAS1 = "phobias1"
FN_PHOBIAS2 = "phobias2"
FN_PHOBIAS3 = "phobias3"
FN_PHOBIAS4 = "phobias4"
FN_PHOBIAS_DUR = "phobias_dur"

FN_PANIC_MAND = "panic_mand"
FN_PANIC1 = "panic1"
FN_PANIC2 = "panic2"
FN_PANIC3 = "panic3"
FN_PANIC4 = "panic4"
FN_PANSYM_A = "pansym_a"
FN_PANSYM_B = "pansym_b"
FN_PANSYM_C = "pansym_c"
FN_PANSYM_D = "pansym_d"
FN_PANSYM_E = "pansym_e"
FN_PANSYM_F = "pansym_f"
FN_PANSYM_G = "pansym_g"
FN_PANSYM_H = "pansym_h"
FN_PANSYM_I = "pansym_i"
FN_PANSYM_J = "pansym_j"
FN_PANSYM_K = "pansym_k"
FN_PANSYM_L = "pansym_l"
FN_PANSYM_M = "pansym_m"
FN_PANIC5 = "panic5"
FN_PANIC_DUR = "panic_dur"

FN_COMP_MAND1 = "comp_mand1"
FN_COMP1 = "comp1"
FN_COMP2 = "comp2"
FN_COMP3 = "comp3"
FN_COMP4 = "comp4"
FN_COMP_DUR = "comp_dur"

FN_OBSESS_MAND1 = "obsess_mand1"
FN_OBSESS_MAND2 = "obsess_mand2"
FN_OBSESS1 = "obsess1"
FN_OBSESS2 = "obsess2"
FN_OBSESS3 = "obsess3"
FN_OBSESS4 = "obsess4"
FN_OBSESS_DUR = "obsess_dur"

FN_OVERALL2 = "overall2"

PANIC_SYMPTOM_FIELDNAMES = [
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
]

FIELDNAME_FOR_QUESTION = {
    # CQ.INTRO_1:  # information only
    # CQ.INTRO_2:  # information only
    # CQ.INTRO_DEMOGRAPHICS:  # information only

    CQ.ETHNIC: FN_ETHNIC,
    CQ.MARRIED: FN_MARRIED,
    CQ.EMPSTAT: FN_EMPSTAT,
    CQ.EMPTYPE: FN_EMPTYPE,
    CQ.HOME: FN_HOME,

    # CQ.HEALTH_WELLBEING: # information only

    CQ.APPETITE1_LOSS_PAST_MONTH: FN_APPETITE1,
    CQ.WEIGHT1_LOSS_PAST_MONTH: FN_WEIGHT1,
    CQ.WEIGHT2_TRYING_TO_LOSE: FN_WEIGHT2,
    CQ.WEIGHT3_LOST_LOTS: FN_WEIGHT3,
    CQ.APPETITE2_INCREASE_PAST_MONTH: FN_APPETITE2,
    CQ.WEIGHT4_INCREASE_PAST_MONTH: FN_WEIGHT4,
    # CQ.WEIGHT4A: not used (= WEIGHT4 + pregnancy option)
    CQ.WEIGHT5_GAINED_LOTS: FN_WEIGHT5,
    CQ.GP_YEAR: FN_GP_YEAR,
    CQ.DISABLE: FN_DISABLE,
    CQ.ILLNESS: FN_ILLNESS,

    CQ.SOMATIC_MAND1_PAIN_PAST_MONTH: FN_SOMATIC_MAND1,
    CQ.SOMATIC_PAIN1_PSYCHOL_EXAC: FN_SOMATIC_PAIN1,
    CQ.SOMATIC_PAIN2_DAYS_PAST_WEEK: FN_SOMATIC_PAIN2,
    CQ.SOMATIC_PAIN3_GT_3H_ANY_DAY: FN_SOMATIC_PAIN3,
    CQ.SOMATIC_PAIN4_UNPLEASANT: FN_SOMATIC_PAIN4,
    CQ.SOMATIC_PAIN5_INTERRUPTED_INTERESTING: FN_SOMATIC_PAIN5,
    CQ.SOMATIC_MAND2_DISCOMFORT: FN_SOMATIC_MAND2,
    CQ.SOMATIC_DIS1_PSYCHOL_EXAC: FN_SOMATIC_DIS1,
    CQ.SOMATIC_DIS2_DAYS_PAST_WEEK: FN_SOMATIC_DIS2,
    CQ.SOMATIC_DIS3_GT_3H_ANY_DAY: FN_SOMATIC_DIS3,
    CQ.SOMATIC_DIS4_UNPLEASANT: FN_SOMATIC_DIS4,
    CQ.SOMATIC_DIS5_INTERRUPTED_INTERESTING: FN_SOMATIC_DIS5,
    CQ.SOMATIC_DUR: FN_SOMATIC_DUR,

    CQ.FATIGUE_MAND1_TIRED_PAST_MONTH: FN_FATIGUE_MAND1,
    CQ.FATIGUE_CAUSE1_TIRED: FN_FATIGUE_CAUSE1,
    CQ.FATIGUE_TIRED1_DAYS_PAST_WEEK: FN_FATIGUE_TIRED1,
    CQ.FATIGUE_TIRED2_GT_3H_ANY_DAY: FN_FATIGUE_TIRED2,
    CQ.FATIGUE_TIRED3_HAD_TO_PUSH: FN_FATIGUE_TIRED3,
    CQ.FATIGUE_TIRED4_DURING_ENJOYABLE: FN_FATIGUE_TIRED4,
    CQ.FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH: FN_FATIGUE_MAND2,
    CQ.FATIGUE_CAUSE2_LACK_ENERGY: FN_FATIGUE_CAUSE2,
    CQ.FATIGUE_ENERGY1_DAYS_PAST_WEEK: FN_FATIGUE_ENERGY1,
    CQ.FATIGUE_ENERGY2_GT_3H_ANY_DAY: FN_FATIGUE_ENERGY2,
    CQ.FATIGUE_ENERGY3_HAD_TO_PUSH: FN_FATIGUE_ENERGY3,
    CQ.FATIGUE_ENERGY4_DURING_ENJOYABLE: FN_FATIGUE_ENERGY4,
    CQ.FATIGUE_DUR: FN_FATIGUE_DUR,

    CQ.CONC_MAND1_POOR_CONC_PAST_MONTH: FN_CONC_MAND1,
    CQ.CONC_MAND2_FORGETFUL_PAST_MONTH: FN_CONC_MAND2,
    CQ.CONC1_CONC_DAYS_PAST_WEEK: FN_CONC1,
    CQ.CONC2_CONC_FOR_TV_READING_CONVERSATION: FN_CONC2,
    CQ.CONC3_CONC_PREVENTED_ACTIVITIES: FN_CONC3,
    CQ.CONC_DUR: FN_CONC_DUR,
    CQ.CONC4_FORGOTTEN_IMPORTANT: FN_CONC4,
    CQ.FORGET_DUR: FN_FORGET_DUR,

    CQ.SLEEP_MAND1_LOSS_PAST_MONTH: FN_SLEEP_MAND1,
    CQ.SLEEP_LOSE1_NIGHTS_PAST_WEEK: FN_SLEEP_LOSE1,
    CQ.SLEEP_LOSE2_DIS_WORST_DURATION: FN_SLEEP_LOSE2,
    CQ.SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK: FN_SLEEP_LOSE3,
    CQ.SLEEP_EMW_PAST_WEEK: FN_SLEEP_EMW,
    CQ.SLEEP_CAUSE: FN_SLEEP_CAUSE,
    CQ.SLEEP_MAND2_GAIN_PAST_MONTH: FN_SLEEP_MAND2,
    CQ.SLEEP_GAIN1_NIGHTS_PAST_WEEK: FN_SLEEP_GAIN1,
    CQ.SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT: FN_SLEEP_GAIN2,
    CQ.SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK: FN_SLEEP_GAIN3,
    CQ.SLEEP_DUR: FN_SLEEP_DUR,

    CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH: FN_IRRIT_MAND1,
    CQ.IRRIT_MAND2_THINGS_PAST_MONTH: FN_IRRIT_MAND2,
    CQ.IRRIT1_DAYS_PER_WEEK: FN_IRRIT1,
    CQ.IRRIT2_GT_1H_ANY_DAY: FN_IRRIT2,
    CQ.IRRIT3_WANTED_TO_SHOUT: FN_IRRIT3,
    CQ.IRRIT4_ARGUMENTS: FN_IRRIT4,
    CQ.IRRIT_DUR: FN_IRRIT_DUR,

    CQ.HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH: FN_HYPO_MAND1,
    CQ.HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS: FN_HYPO_MAND2,
    CQ.HYPO1_DAYS_PAST_WEEK: FN_HYPO1,
    CQ.HYPO2_WORRY_TOO_MUCH: FN_HYPO2,
    CQ.HYPO3_HOW_UNPLEASANT: FN_HYPO3,
    CQ.HYPO4_CAN_DISTRACT: FN_HYPO4,
    CQ.HYPO_DUR: FN_HYPO_DUR,

    CQ.DEPR_MAND1_LOW_MOOD_PAST_MONTH: FN_DEPR_MAND1,
    CQ.DEPR1_LOW_MOOD_PAST_WEEK: FN_DEPR1,
    CQ.DEPR_MAND2_ENJOYMENT_PAST_MONTH: FN_DEPR_MAND2,
    CQ.DEPR2_ENJOYMENT_PAST_WEEK: FN_DEPR2,
    CQ.DEPR3_DAYS_PAST_WEEK: FN_DEPR3,
    CQ.DEPR4_GT_3H_ANY_DAY: FN_DEPR4,
    CQ.DEPR_CONTENT: FN_DEPR_CONTENT,
    CQ.DEPR5_COULD_CHEER_UP: FN_DEPR5,
    CQ.DEPR_DUR: FN_DEPR_DUR,
    CQ.DEPTH1_DIURNAL_VARIATION: FN_DEPTH1,
    CQ.DEPTH2_LIBIDO: FN_DEPTH2,
    CQ.DEPTH3_RESTLESS: FN_DEPTH3,
    CQ.DEPTH4_SLOWED: FN_DEPTH4,
    CQ.DEPTH5_GUILT: FN_DEPTH5,
    CQ.DEPTH6_WORSE_THAN_OTHERS: FN_DEPTH6,
    CQ.DEPTH7_HOPELESS: FN_DEPTH7,
    CQ.DEPTH8_LNWL: FN_DEPTH8,
    CQ.DEPTH9_SUICIDE_THOUGHTS: FN_DEPTH9,
    CQ.DEPTH10_SUICIDE_METHOD: FN_DEPTH10,
    CQ.DOCTOR: FN_DOCTOR,
    # CQ.DOCTOR2_PLEASE_TALK_TO: # info only
    # CQ.DEPR_OUTRO: # info only

    CQ.WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH: FN_WORRY_MAND1,
    CQ.WORRY_MAND2_ANY_WORRIES_PAST_MONTH: FN_WORRY_MAND2,
    CQ.WORRY_CONT1: FN_WORRY_CONT1,
    # CQ.WORRY1_INFO_ONLY: # info only
    CQ.WORRY2_DAYS_PAST_WEEK: FN_WORRY2,
    CQ.WORRY3_TOO_MUCH: FN_WORRY3,
    CQ.WORRY4_HOW_UNPLEASANT: FN_WORRY4,
    CQ.WORRY5_GT_3H_ANY_DAY: FN_WORRY5,
    CQ.WORRY_DUR: FN_WORRY_DUR,

    CQ.ANX_MAND1_ANXIETY_PAST_MONTH: FN_ANX_MAND1,
    CQ.ANX_MAND2_TENSION_PAST_MONTH: FN_ANX_MAND2,
    CQ.ANX_PHOBIA1_SPECIFIC_PAST_MONTH: FN_ANX_PHOBIA1,
    CQ.ANX_PHOBIA2_SPECIFIC_OR_GENERAL: FN_ANX_PHOBIA2,
    # CQ.ANX1_INFO_ONLY: # info only
    CQ.ANX2_GENERAL_DAYS_PAST_WEEK: FN_ANX2,
    CQ.ANX3_GENERAL_HOW_UNPLEASANT: FN_ANX3,
    CQ.ANX4_GENERAL_PHYSICAL_SYMPTOMS: FN_ANX4,
    CQ.ANX5_GENERAL_GT_3H_ANY_DAY: FN_ANX5,
    CQ.ANX_DUR_GENERAL: FN_ANX_DUR,

    CQ.PHOBIAS_MAND_AVOIDANCE_PAST_MONTH: FN_PHOBIAS_MAND,
    CQ.PHOBIAS_TYPE1: FN_PHOBIAS_TYPE1,
    CQ.PHOBIAS1_DAYS_PAST_WEEK: FN_PHOBIAS1,
    CQ.PHOBIAS2_PHYSICAL_SYMPTOMS: FN_PHOBIAS2,
    CQ.PHOBIAS3_AVOIDANCE: FN_PHOBIAS3,
    CQ.PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK: FN_PHOBIAS4,
    CQ.PHOBIAS_DUR: FN_PHOBIAS_DUR,

    CQ.PANIC_MAND_PAST_MONTH: FN_PANIC_MAND,
    CQ.PANIC1_NUM_PAST_WEEK: FN_PANIC1,
    CQ.PANIC2_HOW_UNPLEASANT: FN_PANIC2,
    CQ.PANIC3_PANIC_GE_10_MIN: FN_PANIC3,
    CQ.PANIC4_RAPID_ONSET: FN_PANIC4,
    # CQ.PANSYM: # multiple stems
    CQ.PANIC5_ALWAYS_SPECIFIC_TRIGGER: FN_PANIC5,
    CQ.PANIC_DUR: FN_PANIC_DUR,

    # CQ.ANX_OUTRO: # info only

    CQ.COMP_MAND1_COMPULSIONS_PAST_MONTH: FN_COMP_MAND1,
    CQ.COMP1_DAYS_PAST_WEEK: FN_COMP1,
    CQ.COMP2_TRIED_TO_STOP: FN_COMP2,
    CQ.COMP3_UPSETTING: FN_COMP3,
    CQ.COMP4_MAX_N_REPETITIONS: FN_COMP4,
    CQ.COMP_DUR: FN_COMP_DUR,

    CQ.OBSESS_MAND1_OBSESSIONS_PAST_MONTH: FN_OBSESS_MAND1,
    CQ.OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL: FN_OBSESS_MAND2,
    CQ.OBSESS1_DAYS_PAST_WEEK: FN_OBSESS1,
    CQ.OBSESS2_TRIED_TO_STOP: FN_OBSESS2,
    CQ.OBSESS3_UPSETTING: FN_OBSESS3,
    CQ.OBSESS4_MAX_DURATION: FN_OBSESS4,
    CQ.OBSESS_DUR: FN_OBSESS_DUR,

    # CQ.OVERALL1: # info only
    CQ.OVERALL2_IMPACT_PAST_WEEK: FN_OVERALL2,
}

# Questions for which 1 = no, 2 = yes (+/- other options)
QUESTIONS_1_NO_2_YES = [
    CQ.APPETITE1_LOSS_PAST_MONTH,
    CQ.WEIGHT1_LOSS_PAST_MONTH,
    CQ.WEIGHT2_TRYING_TO_LOSE,
    CQ.APPETITE2_INCREASE_PAST_MONTH,
    CQ.WEIGHT4_INCREASE_PAST_MONTH,  # may also offer "yes but pregnant"  # noqa
    CQ.SOMATIC_MAND1_PAIN_PAST_MONTH,
    CQ.SOMATIC_MAND2_DISCOMFORT,
    CQ.SOMATIC_PAIN3_GT_3H_ANY_DAY,
    CQ.SOMATIC_PAIN5_INTERRUPTED_INTERESTING,  # also has other options  # noqa
    CQ.SOMATIC_DIS3_GT_3H_ANY_DAY,
    CQ.SOMATIC_DIS5_INTERRUPTED_INTERESTING,  # also has other options  # noqa
    CQ.FATIGUE_MAND1_TIRED_PAST_MONTH,
    CQ.FATIGUE_TIRED2_GT_3H_ANY_DAY,
    CQ.FATIGUE_TIRED3_HAD_TO_PUSH,
    CQ.FATIGUE_TIRED4_DURING_ENJOYABLE,  # also has other options
    CQ.FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH,
    CQ.FATIGUE_ENERGY2_GT_3H_ANY_DAY,
    CQ.FATIGUE_ENERGY3_HAD_TO_PUSH,
    CQ.FATIGUE_ENERGY4_DURING_ENJOYABLE,  # also has other options
    CQ.CONC_MAND1_POOR_CONC_PAST_MONTH,
    CQ.CONC_MAND2_FORGETFUL_PAST_MONTH,
    CQ.CONC3_CONC_PREVENTED_ACTIVITIES,
    CQ.CONC4_FORGOTTEN_IMPORTANT,
    CQ.SLEEP_MAND1_LOSS_PAST_MONTH,
    CQ.SLEEP_EMW_PAST_WEEK,
    CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH,
    CQ.IRRIT2_GT_1H_ANY_DAY,
    CQ.HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH,
    CQ.HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS,
    CQ.HYPO2_WORRY_TOO_MUCH,
    CQ.DEPR_MAND1_LOW_MOOD_PAST_MONTH,
    CQ.DEPR1_LOW_MOOD_PAST_WEEK,
    CQ.DEPR4_GT_3H_ANY_DAY,
    CQ.DEPTH3_RESTLESS,
    CQ.DEPTH4_SLOWED,
    CQ.DEPTH6_WORSE_THAN_OTHERS,
    CQ.DEPTH7_HOPELESS,
    CQ.DEPTH10_SUICIDE_METHOD,
    CQ.WORRY_MAND2_ANY_WORRIES_PAST_MONTH,
    CQ.WORRY3_TOO_MUCH,
    CQ.WORRY5_GT_3H_ANY_DAY,
    CQ.ANX_MAND1_ANXIETY_PAST_MONTH,
    CQ.ANX_PHOBIA1_SPECIFIC_PAST_MONTH,
    CQ.ANX4_GENERAL_PHYSICAL_SYMPTOMS,
    CQ.ANX5_GENERAL_GT_3H_ANY_DAY,
    CQ.PHOBIAS_MAND_AVOIDANCE_PAST_MONTH,
    CQ.PHOBIAS2_PHYSICAL_SYMPTOMS,
    CQ.PHOBIAS3_AVOIDANCE,
    CQ.PANIC4_RAPID_ONSET,
    CQ.PANIC5_ALWAYS_SPECIFIC_TRIGGER,
    CQ.COMP2_TRIED_TO_STOP,
    CQ.COMP3_UPSETTING,
    CQ.OBSESS2_TRIED_TO_STOP,
    CQ.OBSESS3_UPSETTING,
]
# Questions for which 1 = yes, 2 = no (+/- other options)
QUESTIONS_1_YES_2_NO = [
    CQ.DISABLE,
    CQ.CONC2_CONC_FOR_TV_READING_CONVERSATION,
    CQ.HYPO4_CAN_DISTRACT,
]
# Yes-no (or no-yes) questions but with specific text
QUESTIONS_YN_SPECIFIC_TEXT = [
    CQ.WEIGHT2_TRYING_TO_LOSE,
    CQ.SOMATIC_PAIN3_GT_3H_ANY_DAY,
    CQ.SOMATIC_DIS3_GT_3H_ANY_DAY,
    CQ.FATIGUE_TIRED2_GT_3H_ANY_DAY,
    CQ.FATIGUE_TIRED3_HAD_TO_PUSH,
    CQ.FATIGUE_ENERGY2_GT_3H_ANY_DAY,
    CQ.FATIGUE_ENERGY3_HAD_TO_PUSH,
    CQ.CONC_MAND1_POOR_CONC_PAST_MONTH,
    CQ.CONC2_CONC_FOR_TV_READING_CONVERSATION,
    CQ.CONC4_FORGOTTEN_IMPORTANT,
    CQ.SLEEP_EMW_PAST_WEEK,
    CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH,
    CQ.IRRIT2_GT_1H_ANY_DAY,
    CQ.HYPO2_WORRY_TOO_MUCH,
    CQ.HYPO4_CAN_DISTRACT,
    CQ.DEPR1_LOW_MOOD_PAST_WEEK,
    CQ.DEPR4_GT_3H_ANY_DAY,
    CQ.DEPTH6_WORSE_THAN_OTHERS,
    CQ.DEPTH7_HOPELESS,
    CQ.WORRY3_TOO_MUCH,
    CQ.WORRY5_GT_3H_ANY_DAY,
    CQ.ANX4_GENERAL_PHYSICAL_SYMPTOMS,
    CQ.ANX5_GENERAL_GT_3H_ANY_DAY,
    CQ.PHOBIAS2_PHYSICAL_SYMPTOMS,
    CQ.PHOBIAS3_AVOIDANCE,
    CQ.COMP2_TRIED_TO_STOP,
    CQ.COMP3_UPSETTING,
    CQ.OBSESS2_TRIED_TO_STOP,
    CQ.OBSESS3_UPSETTING,
]
# Demographics questions (optional for diagnosis)
QUESTIONS_DEMOGRAPHICS = [
    CQ.ETHNIC,
    CQ.MARRIED,
    CQ.EMPSTAT,
    CQ.EMPTYPE,
    CQ.HOME,
]
# "Questions" that are just a prompt screen
QUESTIONS_PROMPT_ONLY = {
    # Maps questions to their prompt's xstring name
    CQ.INTRO_1: "intro_1",
    CQ.INTRO_2: "intro_2",
    CQ.INTRO_DEMOGRAPHICS: "intro_demographics_statement",

    CQ.HEALTH_WELLBEING: "health_wellbeing_statement",
    CQ.DOCTOR2_PLEASE_TALK_TO: "doctor2",
    CQ.DEPR_OUTRO: "depr_outro",
    CQ.WORRY1_INFO_ONLY: "worry1",
    CQ.ANX1_INFO_ONLY: "anx1",
    CQ.ANX_OUTRO: "anx_outro",
    CQ.OVERALL1_INFO_ONLY: "overall1",
    CQ.THANKS_FINISHED: "end",
}
# "How many days per week" questions
# "Overall duration" questions
QUESTIONS_OVERALL_DURATION = [
    CQ.SOMATIC_DUR,
    CQ.FATIGUE_DUR,
    CQ.CONC_DUR,
    CQ.FORGET_DUR,
    CQ.SLEEP_DUR,
    CQ.IRRIT_DUR,
    CQ.HYPO_DUR,
    CQ.DEPR_DUR,
    CQ.WORRY_DUR,
    CQ.ANX_DUR_GENERAL,
    CQ.PHOBIAS_DUR,
    CQ.PANIC_DUR,
    CQ.COMP_DUR,
    CQ.OBSESS_DUR,
]
# Multi-way questions, other than yes/no ones.
QUESTIONS_MULTIWAY = {
    # Maps questions to first and last number of answers.
    CQ.WEIGHT3_LOST_LOTS: (1, 2),
    CQ.WEIGHT4_INCREASE_PAST_MONTH: (1, 2),  # may be modified to 3 if female
    CQ.WEIGHT5_GAINED_LOTS: (1, 2),
    CQ.GP_YEAR: (0, 4),  # unusual; starts at 0
    CQ.ILLNESS: (1, 8),
    CQ.SOMATIC_PAIN1_PSYCHOL_EXAC: (1, 3),
    CQ.SOMATIC_PAIN5_INTERRUPTED_INTERESTING: (1, 3),
    CQ.SOMATIC_DIS1_PSYCHOL_EXAC: (1, 3),
    CQ.SOMATIC_DIS5_INTERRUPTED_INTERESTING: (1, 3),
    CQ.FATIGUE_TIRED4_DURING_ENJOYABLE: (1, 3),
    CQ.FATIGUE_ENERGY4_DURING_ENJOYABLE: (1, 3),
    CQ.SLEEP_LOSE2_DIS_WORST_DURATION: (1, 4),
    CQ.SLEEP_CAUSE: (1, 6),
    CQ.SLEEP_MAND2_GAIN_PAST_MONTH: (1, 3),
    CQ.SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT: (1, 4),
    CQ.IRRIT_MAND2_THINGS_PAST_MONTH: (1, 3),
    CQ.IRRIT3_WANTED_TO_SHOUT: (1, 3),
    CQ.IRRIT4_ARGUMENTS: (1, 3),
    CQ.DEPR_MAND2_ENJOYMENT_PAST_MONTH: (1, 3),
    CQ.DEPR2_ENJOYMENT_PAST_WEEK: (1, 3),
    CQ.DEPR5_COULD_CHEER_UP: (1, 3),
    CQ.DEPTH1_DIURNAL_VARIATION: (1, 4),
    CQ.DEPTH2_LIBIDO: (1, 4),
    CQ.DEPTH5_GUILT: (1, 4),
    CQ.DEPTH8_LNWL: (1, 3),
    CQ.DEPTH9_SUICIDE_THOUGHTS: (1, 3),
    CQ.DOCTOR: (1, 3),
    CQ.ANX_PHOBIA2_SPECIFIC_OR_GENERAL: (1, 2),
    CQ.PHOBIAS_TYPE1: (1, 9),
    CQ.PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK: (1, 3),
    CQ.PANIC_MAND_PAST_MONTH: (1, 3),
    CQ.PANIC1_NUM_PAST_WEEK: (1, 3),
    CQ.PANIC2_HOW_UNPLEASANT: (1, 3),
    CQ.PANIC3_PANIC_GE_10_MIN: (1, 2),
    CQ.COMP4_MAX_N_REPETITIONS: (1, 3),
    CQ.OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL: (1, 2),
    CQ.OBSESS4_MAX_DURATION: (1, 2),
    CQ.OVERALL2_IMPACT_PAST_WEEK: (1, 4),
}
QUESTIONS_MULTIWAY_WITH_EXTRA_STEM = {
    # Maps questions to first and last number of answers.
    CQ.ETHNIC: (1, 7),  # 7 includes our additional "prefer not to say"
    CQ.MARRIED: (1, 6),  # 6 includes our additional "prefer not to say"
    CQ.EMPSTAT: (1, 8),  # 8 includes our additional "prefer not to say"
    CQ.EMPTYPE: (1, 7),  # 7 includes our additional "not applicable" + "prefer not to say"  # noqa
    CQ.HOME: (1, 7),  # 7 includes our additional "prefer not to say"
}
QUESTIONS_DAYS_PER_WEEK = [
    CQ.SOMATIC_PAIN2_DAYS_PAST_WEEK,
    CQ.SOMATIC_DIS2_DAYS_PAST_WEEK,
    CQ.FATIGUE_TIRED1_DAYS_PAST_WEEK,
    CQ.FATIGUE_ENERGY1_DAYS_PAST_WEEK,
    CQ.CONC1_CONC_DAYS_PAST_WEEK,
    CQ.IRRIT1_DAYS_PER_WEEK,
    CQ.HYPO1_DAYS_PAST_WEEK,
    CQ.DEPR3_DAYS_PAST_WEEK,
    CQ.WORRY2_DAYS_PAST_WEEK,
    CQ.ANX2_GENERAL_DAYS_PAST_WEEK,
    CQ.PHOBIAS1_DAYS_PAST_WEEK,
    # not this: CQ.PHOBIAS4_AVOIDANCE_FREQUENCY -- different phrasing
    # not this: CQ.PANIC1_FREQUENCY
    CQ.COMP1_DAYS_PAST_WEEK,
    CQ.OBSESS1_DAYS_PAST_WEEK,
]
QUESTIONS_NIGHTS_PER_WEEK = [
    CQ.SLEEP_LOSE1_NIGHTS_PAST_WEEK,
    CQ.SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK,
    CQ.SLEEP_GAIN1_NIGHTS_PAST_WEEK,   # (*) see below
    # (*) Probably an error in the original:
    # "On how many nights in the PAST SEVEN NIGHTS did you have problems
    # with your sleep? (1) None. (2) Between one and three days. (3) Four
    # days or more." Note day/night confusion. Altered to "nights".
    CQ.SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK,
]
QUESTIONS_HOW_UNPLEASANT_STANDARD = [
    CQ.SOMATIC_PAIN4_UNPLEASANT,
    CQ.SOMATIC_DIS4_UNPLEASANT,
    CQ.HYPO3_HOW_UNPLEASANT,
    CQ.WORRY4_HOW_UNPLEASANT,
    CQ.ANX3_GENERAL_HOW_UNPLEASANT,
]
QUESTIONS_FATIGUE_CAUSES = [
    CQ.FATIGUE_CAUSE1_TIRED,
    CQ.FATIGUE_CAUSE2_LACK_ENERGY,
]
QUESTIONS_STRESSORS = [
    CQ.DEPR_CONTENT,
    CQ.WORRY_CONT1,
]
QUESTIONS_NO_SOMETIMES_OFTEN = [
    CQ.WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH,
    CQ.ANX_MAND2_TENSION_PAST_MONTH,
    CQ.COMP_MAND1_COMPULSIONS_PAST_MONTH,
    CQ.OBSESS_MAND1_OBSESSIONS_PAST_MONTH,
    # and no-sometimes-often values also used by:
    #      CQ.PANIC_MAND_PAST_MONTH
    # ... but with variations on the text.
]


# =============================================================================
# Ancillary functions
# =============================================================================

def fieldname_for_q(q: CisrQuestion) -> str:
    return FIELDNAME_FOR_QUESTION.get(q, "")


def enum_to_int(qe: CisrQuestion) -> int:
    return qe.value


def int_to_enum(qi: int) -> CisrQuestion:
    # https://stackoverflow.com/questions/23951641/how-to-convert-int-to-enum-in-python  # noqa
    return CisrQuestion(qi)


# =============================================================================
# CisrResult
# =============================================================================

class CisrResult(object):
    def __init__(self, record_decisions: bool = False) -> None:
        self.incomplete = False
        self.record_decisions = record_decisions
        self.decisions = []  # type: List[str]

        # Symptom scoring
        self.depression = 0  # DEPR in original
        self.depr_crit_1_mood_anhedonia_energy = 0  # DEPCRIT1
        self.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui = 0  # DEPCRIT2
        self.depr_crit_3_somatic_synd = 0  # DEPCRIT3
        # ... looks to me like the ICD-10 criteria for somatic syndrome
        # (e.g. F32.01, F32.11, F33.01, F33.11), with the "do you cheer up
        # when..." question (DEPR5) being the one for "lack of emotional
        # reactions to events or activities that normally produce an
        # emotional response".
        self.weight_change = WTCHANGE_NONE_OR_APPETITE_INCREASE  # WTCHANGE IN original  # noqa
        self.somatic_symptoms = 0  # SOMATIC in original
        self.fatigue = 0  # FATIGUE in original
        self.neurasthenia = 0  # NEURAS in original
        self.concentration_poor = 0  # CONC in original
        self.sleep_problems = 0  # SLEEP in original
        self.sleep_change = SLEEPCHANGE_NONE  # SLEEPCH in original
        self.depressive_thoughts = 0  # DEPTHTS in original
        self.irritability = 0  # IRRIT in original
        self.diurnal_mood_variation = DIURNAL_MOOD_VAR_NONE  # DVM in original
        self.libido_decreased = False  # LIBID in original
        self.psychomotor_changes = PSYCHOMOTOR_NONE  # PSYCHMOT in original
        self.suicidality = SUICIDE_INTENT_NONE  # type: int  # SUICID in original  # noqa
        self.depression_at_least_2_weeks = False  # DEPR_DUR >= 2 in original

        self.hypochondria = 0  # HYPO in original
        self.worry = 0  # WORRY in original
        self.anxiety = 0  # ANX in original
        self.anxiety_physical_symptoms = False  # AN4 == 2 in original
        self.anxiety_at_least_2_weeks = False  # ANX_DUR >= 2 in original
        self.phobias_flag = False  # PHOBIAS_FLAG in original
        self.phobias_score = 0  # PHOBIAS in original
        self.phobias_type = 0  # PHOBIAS_TYPE in original
        self.phobic_avoidance = False  # PHOBIAS3 == 2 in original
        self.panic = 0  # PANIC in original
        self.panic_rapid_onset = False  # PANIC4 == 2 in original
        self.panic_symptoms_total = 0  # PANSYTOT in original

        self.compulsions = 0  # COMP in original
        self.compulsions_tried_to_stop = False  # COMP2 == 2 in original
        self.compulsions_at_least_2_weeks = False  # COMP_DUR >= 2 in original
        self.obsessions = 0  # OBSESS in original
        self.obsessions_tried_to_stop = False  # OBSESS2 == 2 in original
        self.obsessions_at_least_2_weeks = False  # OBSESS_DUR >= 2 in original

        self.functional_impairment = 0  # IMPAIR in original

        # Disorder flags
        self.obsessive_compulsive_disorder = False  # OBCOMP in original
        self.depression_mild = False  # DEPRMILD in original
        self.depression_moderate = False  # DEPRMOD in original
        self.depression_severe = False  # DEPRSEV in original
        self.chronic_fatigue_syndrome = False  # CFS in original
        self.generalized_anxiety_disorder = False  # GAD in original
        self.phobia_agoraphobia = False  # PHOBAG in original
        self.phobia_social = False  # PHOBSOC in original
        self.phobia_specific = False  # PHOBSPEC in original
        self.panic_disorder = False  # PANICD in original

        # Final diagnoses
        self.diagnosis_1 = DIAG_0_NO_DIAGNOSIS  # DIAG1 in original
        self.diagnosis_2 = DIAG_0_NO_DIAGNOSIS  # DIAG2 in original

    # -------------------------------------------------------------------------
    # Overall scoring
    # -------------------------------------------------------------------------

    def get_score(self) -> int:  # SCORE in original
        return (
            self.somatic_symptoms +
            self.fatigue +
            self.concentration_poor +
            self.sleep_problems +
            self.irritability +
            self.hypochondria +
            self.depression +
            self.depressive_thoughts +
            self.worry +
            self.anxiety +
            self.phobias_score +
            self.panic +
            self.compulsions +
            self.obsessions
        )

    def needs_impairment_question(self) -> bool:
        # code in OVERALL1 in original
        threshold = 2  # for all symptoms
        return (
            self.somatic_symptoms >= threshold or
            self.hypochondria >= threshold or
            self.fatigue >= threshold or
            self.sleep_problems >= threshold or
            self.irritability >= threshold or
            self.concentration_poor >= threshold or
            self.depression >= threshold or
            self.depressive_thoughts >= threshold or
            self.phobias_score >= threshold or
            self.worry >= threshold or
            self.anxiety >= threshold or
            self.panic >= threshold or
            self.compulsions >= threshold or
            self.obsessions >= threshold
        )

    def has_somatic_syndrome(self) -> bool:
        return self.depr_crit_3_somatic_synd >= SOMATIC_SYNDROME_CRITERION

    def get_final_page(self) -> CisrQuestion:
        # see chooseFinalPage() in the C++ version
        return (
            CQ.OVERALL1_INFO_ONLY if self.needs_impairment_question()
            else CQ.THANKS_FINISHED
        )

    def decide(self, decision: str) -> None:
        if self.record_decisions:
            self.decisions.append(decision)

    def _showint(self, name: str, value: int) -> None:
        self.decide(f"{SCORE_PREFIX}{name}: {value}")

    def _showbool(self, name: str, value: bool) -> None:
        self.decide(f"{SCORE_PREFIX}{name}: {'true' if value else 'false'}")

    def diagnosis_name(self, diagnosis_code: int) -> str:
        if self.incomplete:
            # Do NOT offer diagnostic information based on partial data.
            # Might be dangerous (e.g. say "mild depressive episode" when it's
            # severe + incomplete information).
            return "INFORMATION INCOMPLETE"

        if diagnosis_code == DIAG_0_NO_DIAGNOSIS:
            return "No diagnosis identified"
        elif diagnosis_code == DIAG_1_MIXED_ANX_DEPR_DIS_MILD:
            return "Mixed anxiety and depressive disorder (mild)"
        elif diagnosis_code == DIAG_2_GENERALIZED_ANX_DIS_MILD:
            return "Generalized anxiety disorder (mild)"
        elif diagnosis_code == DIAG_3_OBSESSIVE_COMPULSIVE_DIS:
            return "Obsessive–compulsive disorder"
        elif diagnosis_code == DIAG_4_MIXED_ANX_DEPR_DIS:
            return "Mixed anxiety and depressive disorder"
        elif diagnosis_code == DIAG_5_SPECIFIC_PHOBIA:
            return "Specific (isolated) phobia"
        elif diagnosis_code == DIAG_6_SOCIAL_PHOBIA:
            return "Social phobia"
        elif diagnosis_code == DIAG_7_AGORAPHOBIA:
            return "Agoraphobia"
        elif diagnosis_code == DIAG_8_GENERALIZED_ANX_DIS:
            return "Generalized anxiety disorder"
        elif diagnosis_code == DIAG_9_PANIC_DIS:
            return "Panic disorder"
        elif diagnosis_code == DIAG_10_MILD_DEPR_EPISODE:
            return "Mild depressive episode"
        elif diagnosis_code == DIAG_11_MOD_DEPR_EPISODE:
            return "Moderate depressive episode"
        elif diagnosis_code == DIAG_12_SEVERE_DEPR_EPISODE:
            return "Severe depressive episode"
        else:
            return "[INTERNAL ERROR: BAD DIAGNOSIS CODE]"

    def diagnosis_icd10_code(self, diagnosis_code: int) -> str:
        if self.incomplete:
            return ""

        if diagnosis_code == DIAG_0_NO_DIAGNOSIS:
            return ""
        elif diagnosis_code == DIAG_1_MIXED_ANX_DEPR_DIS_MILD:
            return "F41.2"  # no sub-code for "mild"
        elif diagnosis_code == DIAG_2_GENERALIZED_ANX_DIS_MILD:
            return "F41.1"  # no sub-code for "mild"
        elif diagnosis_code == DIAG_3_OBSESSIVE_COMPULSIVE_DIS:
            return "Obsessive–compulsive disorder"
        elif diagnosis_code == DIAG_4_MIXED_ANX_DEPR_DIS:
            return "F41.2"
        elif diagnosis_code == DIAG_5_SPECIFIC_PHOBIA:
            return "F40.2"
        elif diagnosis_code == DIAG_6_SOCIAL_PHOBIA:
            return "F40.1"
        elif diagnosis_code == DIAG_7_AGORAPHOBIA:
            return "F40.0"  # not clear whether F40.00/F40.01 are distinguished
        elif diagnosis_code == DIAG_8_GENERALIZED_ANX_DIS:
            return "F41.1"
        elif diagnosis_code == DIAG_9_PANIC_DIS:
            return "F41.0"
        elif diagnosis_code == DIAG_10_MILD_DEPR_EPISODE:
            if self.has_somatic_syndrome():
                return "F32.01"
            else:
                return "F32.00"
        elif diagnosis_code == DIAG_11_MOD_DEPR_EPISODE:
            if self.has_somatic_syndrome():
                return "F32.11"
            else:
                return "F32.10"
        elif diagnosis_code == DIAG_12_SEVERE_DEPR_EPISODE:
            return "F32.2 or F32.3"
        else:
            return "[INTERNAL ERROR: BAD DIAGNOSIS CODE]"

    def has_diagnosis(self, diagnosis_code: int) -> bool:
        return not self.incomplete and diagnosis_code != DIAG_0_NO_DIAGNOSIS

    def has_diagnosis_1(self) -> bool:
        return self.has_diagnosis(self.diagnosis_1)

    def has_diagnosis_2(self) -> bool:
        return self.has_diagnosis(self.diagnosis_1)

    def diagnosis_1_name(self) -> str:
        return self.diagnosis_name(self.diagnosis_1)

    def diagnosis_1_icd10_code(self) -> str:
        return self.diagnosis_icd10_code(self.diagnosis_1)

    def diagnosis_2_name(self) -> str:
        return self.diagnosis_name(self.diagnosis_2)

    def diagnosis_2_icd10_code(self) -> str:
        return self.diagnosis_icd10_code(self.diagnosis_2)

    def finalize(self) -> None:
        at_least_1_activity_impaired = (self.functional_impairment >=
                                        OVERALL_IMPAIRMENT_STOP_1_ACTIVITY)
        score = self.get_score()

        # GAD
        if (self.anxiety >= 2 and
                self.anxiety_physical_symptoms and
                self.anxiety_at_least_2_weeks):
            self.decide(
                "Anxiety score >= 2 AND physical symptoms of anxiety AND "
                "anxiety for at least 2 weeks. "
                "Setting generalized_anxiety_disorder.")
            self.generalized_anxiety_disorder = True

        # Panic
        if self.panic >= 3 and self.panic_rapid_onset:
            self.decide("Panic score >= 3 AND panic_rapid_onset. "
                        "Setting panic_disorder.")
            self.panic_disorder = True

        # Phobias
        if (self.phobias_type == PHOBIATYPES_AGORAPHOBIA and
                self.phobic_avoidance and
                self.phobias_score >= 2):
            self.decide("Phobia type is agoraphobia AND phobic avoidance AND"
                        "phobia score >= 2. Setting phobia_agoraphobia.")
            self.phobia_agoraphobia = True
        if (self.phobias_type == PHOBIATYPES_SOCIAL and
                self.phobic_avoidance and
                self.phobias_score >= 2):
            self.decide("Phobia type is social AND phobic avoidance AND"
                        "phobia score >= 2. Setting phobia_social.")
            self.phobia_social = True
        if (self.phobias_type == PHOBIATYPES_SOCIAL and
                self.phobic_avoidance and
                self.phobias_score >= 2):
            self.decide(
                "Phobia type is (animals/enclosed/heights OR other) AND "
                "phobic avoidance AND phobia score >= 2. "
                "Setting phobia_specific.")
            self.phobia_specific = True

        # OCD
        if (self.obsessions + self.compulsions >= 6 and
                self.obsessions_tried_to_stop and
                self.obsessions_at_least_2_weeks and
                at_least_1_activity_impaired):
            self.decide("obsessions + compulsions >= 6 AND "
                        "tried to stop obsessions AND "
                        "obsessions for at least 2 weeks AND "
                        "at least 1 activity impaired. "
                        "Setting obsessive_compulsive_disorder.")
            self.obsessive_compulsive_disorder = True
        if (self.obsessions + self.compulsions >= 6 and
                self.compulsions_tried_to_stop and
                self.compulsions_at_least_2_weeks and
                at_least_1_activity_impaired):
            self.decide("obsessions + compulsions >= 6 AND "
                        "tried to stop compulsions AND "
                        "compulsions for at least 2 weeks AND "
                        "at least 1 activity impaired. "
                        "Setting obsessive_compulsive_disorder.")
            self.obsessive_compulsive_disorder = True
        if (self.obsessions == 4 and
                self.obsessions_tried_to_stop and
                self.obsessions_at_least_2_weeks and
                at_least_1_activity_impaired):
            # NOTE: 4 is the maximum for obsessions
            self.decide("obsessions == 4 AND "
                        "tried to stop obsessions AND "
                        "obsessions for at least 2 weeks AND "
                        "at least 1 activity impaired. "
                        "Setting obsessive_compulsive_disorder.")
            self.obsessive_compulsive_disorder = True
        if (self.compulsions == 4 and
                self.compulsions_tried_to_stop and
                self.compulsions_at_least_2_weeks and
                at_least_1_activity_impaired):
            # NOTE: 4 is the maximum for compulsions
            self.decide("compulsions == 4 AND "
                        "tried to stop compulsions AND "
                        "compulsions for at least 2 weeks AND "
                        "at least 1 activity impaired. "
                        "Setting obsessive_compulsive_disorder.")
            self.obsessive_compulsive_disorder = True

        # Depression
        if (self.depression_at_least_2_weeks and
                self.depr_crit_1_mood_anhedonia_energy > 1 and
                self.depr_crit_1_mood_anhedonia_energy +
                self.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 3):
            self.decide("Depressive symptoms >=2 weeks AND "
                        "depr_crit_1_mood_anhedonia_energy > 1 AND "
                        "depr_crit_1_mood_anhedonia_energy + "
                        "depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 3. "
                        "Setting depression_mild.")
            self.depression_mild = True
        if (self.depression_at_least_2_weeks and
                self.depr_crit_1_mood_anhedonia_energy > 1 and
                (self.depr_crit_1_mood_anhedonia_energy +
                 self.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui) > 5):
            self.decide("Depressive symptoms >=2 weeks AND "
                        "depr_crit_1_mood_anhedonia_energy > 1 AND "
                        "depr_crit_1_mood_anhedonia_energy + "
                        "depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 5. "
                        "Setting depression_moderate.")
            self.depression_moderate = True
        if (self.depression_at_least_2_weeks and
                self.depr_crit_1_mood_anhedonia_energy == 3 and
                self.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 4):
            self.decide("Depressive symptoms >=2 weeks AND "
                        "depr_crit_1_mood_anhedonia_energy == 3 AND "
                        "depr_crit_2_app_cnc_slp_mtr_glt_wth_sui > 4. "
                        "Setting depression_severe.")
            self.depression_severe = True

        # CFS
        if self.neurasthenia >= 2:
            # The original had a pointless check for "DIAG1 == 0" too, but that
            # was always true.
            self.decide("neurasthenia >= 2. Setting chronic_fatigue_syndrome.")
            self.chronic_fatigue_syndrome = True

        # Final diagnostic hierarchy

        # ... primary diagnosis
        if score >= 12:
            self.decide("Total score >= 12. Setting diagnosis_1 to "
                        "DIAG_1_MIXED_ANX_DEPR_DIS_MILD.")
            self.diagnosis_1 = DIAG_1_MIXED_ANX_DEPR_DIS_MILD
        if self.generalized_anxiety_disorder:
            self.decide("generalized_anxiety_disorder is true. Setting "
                        "diagnosis_1 to DIAG_2_GENERALIZED_ANX_DIS_MILD.")
            self.diagnosis_1 = DIAG_2_GENERALIZED_ANX_DIS_MILD
        if self.obsessive_compulsive_disorder:
            self.decide("obsessive_compulsive_disorder is true. Setting "
                        "diagnosis_1 to DIAG_3_OBSESSIVE_COMPULSIVE_DIS.")
            self.diagnosis_1 = DIAG_3_OBSESSIVE_COMPULSIVE_DIS
        if score >= 20:
            self.decide("Total score >= 20. Setting diagnosis_1 to "
                        "DIAG_4_MIXED_ANX_DEPR_DIS.")
            self.diagnosis_1 = DIAG_4_MIXED_ANX_DEPR_DIS
        if self.phobia_specific:
            self.decide("phobia_specific is true. Setting diagnosis_1 to "
                        "DIAG_5_SPECIFIC_PHOBIA.")
            self.diagnosis_1 = DIAG_5_SPECIFIC_PHOBIA
        if self.phobia_social:
            self.decide("phobia_social is true. Setting diagnosis_1 to "
                        "DIAG_6_SOCIAL_PHOBIA.")
            self.diagnosis_1 = DIAG_6_SOCIAL_PHOBIA
        if self.phobia_agoraphobia:
            self.decide("phobia_agoraphobia is true. Setting diagnosis_1 to "
                        "DIAG_7_AGORAPHOBIA.")
            self.diagnosis_1 = DIAG_7_AGORAPHOBIA
        if self.generalized_anxiety_disorder and score >= 20:
            self.decide("generalized_anxiety_disorder is true AND "
                        "score >= 20. Setting diagnosis_1 to "
                        "DIAG_8_GENERALIZED_ANX_DIS.")
            self.diagnosis_1 = DIAG_8_GENERALIZED_ANX_DIS
        if self.panic_disorder:
            self.decide("panic_disorder is true. Setting diagnosis_1 to "
                        "DIAG_9_PANIC_DIS.")
            self.diagnosis_1 = DIAG_9_PANIC_DIS
        if self.depression_mild:
            self.decide("depression_mild is true. Setting diagnosis_1 to "
                        "DIAG_10_MILD_DEPR_EPISODE.")
            self.diagnosis_1 = DIAG_10_MILD_DEPR_EPISODE
        if self.depression_moderate:
            self.decide("depression_moderate is true. Setting diagnosis_1 to "
                        "DIAG_11_MOD_DEPR_EPISODE.")
            self.diagnosis_1 = DIAG_11_MOD_DEPR_EPISODE
        if self.depression_severe:
            self.decide("depression_severe is true. Setting diagnosis_1 to "
                        "DIAG_12_SEVERE_DEPR_EPISODE.")
            self.diagnosis_1 = DIAG_12_SEVERE_DEPR_EPISODE

        # ... secondary diagnosis
        if score >= 12 and self.diagnosis_1 >= 2:
            self.decide(
                "score >= 12 AND diagnosis_1 >= 2. "
                "Setting diagnosis_2 to DIAG_1_MIXED_ANX_DEPR_DIS_MILD.")
            self.diagnosis_2 = DIAG_1_MIXED_ANX_DEPR_DIS_MILD
        if self.generalized_anxiety_disorder and self.diagnosis_1 >= 3:
            self.decide(
                "generalized_anxiety_disorder is true AND "
                "diagnosis_1 >= 3. "
                "Setting diagnosis_2 to DIAG_2_GENERALIZED_ANX_DIS_MILD.")
            self.diagnosis_2 = DIAG_2_GENERALIZED_ANX_DIS_MILD
        if self.obsessive_compulsive_disorder and self.diagnosis_1 >= 4:
            self.decide(
                "obsessive_compulsive_disorder is true AND "
                "diagnosis_1 >= 4. "
                "Setting diagnosis_2 to DIAG_3_OBSESSIVE_COMPULSIVE_DIS.")
            self.diagnosis_2 = DIAG_3_OBSESSIVE_COMPULSIVE_DIS
        if score >= 20 and self.diagnosis_1 >= 5:
            self.decide("score >= 20 AND diagnosis_1 >= 5. "
                        "Setting diagnosis_2 to DIAG_4_MIXED_ANX_DEPR_DIS.")
            self.diagnosis_2 = DIAG_4_MIXED_ANX_DEPR_DIS
        if self.phobia_specific and self.diagnosis_1 >= 6:
            self.decide("phobia_specific is true AND diagnosis_1 >= 6. "
                        "Setting diagnosis_2 to DIAG_5_SPECIFIC_PHOBIA.")
            self.diagnosis_2 = DIAG_5_SPECIFIC_PHOBIA
        if self.phobia_social and self.diagnosis_1 >= 7:
            self.decide("phobia_social is true AND diagnosis_1 >= 7. "
                        "Setting diagnosis_2 to DIAG_6_SOCIAL_PHOBIA.")
            self.diagnosis_2 = DIAG_6_SOCIAL_PHOBIA
        if self.phobia_agoraphobia and self.diagnosis_1 >= 8:
            self.decide("phobia_agoraphobia is true AND diagnosis_1 >= 8. "
                        "Setting diagnosis_2 to DIAG_7_AGORAPHOBIA.")
            self.diagnosis_2 = DIAG_7_AGORAPHOBIA
        if (self.generalized_anxiety_disorder and score >= 20 and
                self.diagnosis_1 >= 9):
            self.decide("generalized_anxiety_disorder is true AND "
                        "score >= 20 AND "
                        "diagnosis_1 >= 9. "
                        "Setting diagnosis_2 to DIAG_8_GENERALIZED_ANX_DIS.")
            self.diagnosis_2 = DIAG_8_GENERALIZED_ANX_DIS
        if self.panic_disorder and self.diagnosis_1 >= 9:
            self.decide("panic_disorder is true AND diagnosis_1 >= 9. "
                        "Setting diagnosis_2 to DIAG_9_PANIC_DIS.")
            self.diagnosis_2 = DIAG_9_PANIC_DIS

        # In summary:
        self.decide("FINISHED.")
        self.decide("--- Final scores:")
        self._showint("depression", self.depression)
        self._showint("depr_crit_1_mood_anhedonia_energy",
                      self.depr_crit_1_mood_anhedonia_energy)
        self._showint("depr_crit_2_app_cnc_slp_mtr_glt_wth_sui",
                      self.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui)
        self._showint("depr_crit_3_somatic_synd",
                      self.depr_crit_3_somatic_synd)
        self._showint("weight_change", self.weight_change)
        self._showint("somatic_symptoms", self.somatic_symptoms)
        self._showint("fatigue", self.fatigue)
        self._showint("neurasthenia", self.neurasthenia)
        self._showint("concentration_poor", self.concentration_poor)
        self._showint("sleep_problems", self.sleep_problems)
        self._showint("sleep_change", self.sleep_change)
        self._showint("depressive_thoughts", self.depressive_thoughts)
        self._showint("irritability", self.irritability)
        self._showint("diurnal_mood_variation", self.diurnal_mood_variation)
        self._showbool("libido_decreased", self.libido_decreased)
        self._showint("psychomotor_changes", self.psychomotor_changes)
        self._showint("suicidality", self.suicidality)
        self._showbool("depression_at_least_2_weeks",
                       self.depression_at_least_2_weeks)

        self._showint("hypochondria", self.hypochondria)
        self._showint("worry", self.worry)
        self._showint("anxiety", self.anxiety)
        self._showbool("anxiety_physical_symptoms",
                       self.anxiety_physical_symptoms)
        self._showbool("anxiety_at_least_2_weeks",
                       self.anxiety_at_least_2_weeks)
        self._showbool("phobias_flag", self.phobias_flag)
        self._showint("phobias_score", self.phobias_score)
        self._showint("phobias_type", self.phobias_type)
        self._showbool("phobic_avoidance", self.phobic_avoidance)
        self._showint("panic", self.panic)
        self._showbool("panic_rapid_onset", self.panic_rapid_onset)
        self._showint("panic_symptoms_total", self.panic_symptoms_total)

        self._showint("compulsions", self.compulsions)
        self._showbool("compulsions_tried_to_stop",
                       self.compulsions_tried_to_stop)
        self._showbool("compulsions_at_least_2_weeks",
                       self.compulsions_at_least_2_weeks)
        self._showint("obsessions", self.obsessions)
        self._showbool("obsessions_tried_to_stop",
                       self.obsessions_tried_to_stop)
        self._showbool("obsessions_at_least_2_weeks",
                       self.obsessions_at_least_2_weeks)

        self._showint("functional_impairment", self.functional_impairment)

        # Disorder flags
        self._showbool("obsessive_compulsive_disorder",
                       self.obsessive_compulsive_disorder)
        self._showbool("depression_mild", self.depression_mild)
        self._showbool("depression_moderate", self.depression_moderate)
        self._showbool("depression_severe", self.depression_severe)
        self._showbool("chronic_fatigue_syndrome",
                       self.chronic_fatigue_syndrome)
        self._showbool("generalized_anxiety_disorder",
                       self.generalized_anxiety_disorder)
        self._showbool("phobia_agoraphobia", self.phobia_agoraphobia)
        self._showbool("phobia_social", self.phobia_social)
        self._showbool("phobia_specific", self.phobia_specific)
        self._showbool("panic_disorder", self.panic_disorder)

        self.decide("--- Final diagnoses:")
        self.decide("Probable primary diagnosis: " +
                    self.diagnosis_name(self.diagnosis_1))
        self.decide("Probable secondary diagnosis: " +
                    self.diagnosis_name(self.diagnosis_2))


# =============================================================================
# CISR
# =============================================================================

class Cisr(TaskHasPatientMixin, Task):
    """
    Server implementation of the CIS-R task.
    """
    __tablename__ = "cisr"
    shortname = "CIS-R"
    provides_trackers = False

    # Demographics

    ethnic = CamcopsColumn(
        FN_ETHNIC, Integer,
        comment=(
            CMT_DEMOGRAPHICS +
            "Ethnicity (1 white, 2 mixed, 3 Asian/British Asian, "
            "4 Black/Black British, 5 Chinese, 6 other, 7 prefer not to say)"
        ),
        permitted_value_checker=ONE_TO_SEVEN_CHECKER,
    )
    married = CamcopsColumn(
        FN_MARRIED, Integer,
        comment=(
            CMT_DEMOGRAPHICS +
            "Marital status (1 married/living as married, 2 single, "
            "3 separated, 4 divorced, 5 widowed, 6 prefer not to say)"
        ),
        permitted_value_checker=ONE_TO_SIX_CHECKER,
    )
    empstat = CamcopsColumn(
        FN_EMPSTAT, Integer,
        comment=(
            CMT_DEMOGRAPHICS +
            "Current employment status (1 working full time, "
            "2 working part time, 3 student, 4 retired, 5 houseperson, "
            "6 unemployed job seeker, 7 unemployed due to ill health,"
            "8 prefer not to say)"
        ),
        permitted_value_checker=ONE_TO_EIGHT_CHECKER,
    )
    emptype = CamcopsColumn(
        FN_EMPTYPE, Integer,
        comment=(
            CMT_DEMOGRAPHICS +
            "Current/last paid employment "
            "(1 self-employed with paid employees, "
            "2 self-employed with no paid employees, 3 employee, "
            "4 foreman/supervisor, 5 manager, 6 not applicable,"
            "7 prefer not to say)"
        ),
        permitted_value_checker=ONE_TO_SEVEN_CHECKER,
    )
    home = CamcopsColumn(
        FN_HOME, Integer,
        comment=(
            CMT_DEMOGRAPHICS +
            "Housing situation (1 home owner, 2 tenant, 3 living with "
            "relative/friend, 4 hostel/care home, 5 homeless, 6 other,"
            "7 prefer not to say)"
        ),
        permitted_value_checker=ONE_TO_SEVEN_CHECKER,
    )

    # Appetite/weight

    appetite1 = CamcopsColumn(
        FN_APPETITE1, Integer,
        comment="Marked appetite loss in past month" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    weight1 = CamcopsColumn(
        FN_WEIGHT1, Integer,
        comment="Weight loss in past month" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    weight2 = CamcopsColumn(
        FN_WEIGHT2, Integer,
        comment="Weight loss: trying to lose weight?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    weight3 = CamcopsColumn(
        FN_WEIGHT3, Integer,
        comment="Weight loss amount (1: ≥0.5 stones; 2: <0.5 stones)",
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    appetite2 = CamcopsColumn(
        FN_APPETITE2, Integer,
        comment="Marked increase in appetite in past month" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    weight4 = CamcopsColumn(
        # male/female responses unified (no "weight4a")
        FN_WEIGHT4, Integer,
        comment="Weight gain in past month (1 yes, 2 no, 3 yes but pregnant)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    weight5 = CamcopsColumn(
        FN_WEIGHT5, Integer,
        comment="Weight gain amount (1: ≥0.5 stones; 2: <0.5 stones)",
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )

    # Somatic problems

    gp_year = CamcopsColumn(
        FN_GP_YEAR, Integer,
        comment="Consultations with GP in past year (0: none, 1: 1–2, 2: 3–4, "
                "3: 6–10; 4: >10",
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
    )
    disable = CamcopsColumn(
        FN_DISABLE, Integer,
        comment="Longstanding illness/disability/infirmity" + CMT_1_YES_2_NO,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    illness = CamcopsColumn(
        FN_ILLNESS, Integer,
        comment="Conditions (1 diabetes, 2 asthma, 3 arthritis, 4 heart "
                "disease, 5 high blood pressure, 6 lung disease, 7 more than "
                "one of the above, 8 none of the above)",
        permitted_value_checker=ONE_TO_EIGHT_CHECKER,
    )

    somatic_mand1 = CamcopsColumn(
        FN_SOMATIC_MAND1, Integer,
        comment="Any aches/pains in past month?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    somatic_pain1 = CamcopsColumn(
        FN_SOMATIC_PAIN1, Integer,
        comment="Pain/ache brought on or made worse because low/anxious/"
                "stressed" + CMT_NEVER_SOMETIMES_ALWAYS,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    somatic_pain2 = CamcopsColumn(
        FN_SOMATIC_PAIN2, Integer,
        comment="Pain: days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    somatic_pain3 = CamcopsColumn(
        FN_SOMATIC_PAIN3, Integer,
        comment="Pain: lasted >3h on any day in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    somatic_pain4 = CamcopsColumn(
        FN_SOMATIC_PAIN4, Integer,
        comment="Pain: unpleasant in past week?" + CMT_UNPLEASANT,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    somatic_pain5 = CamcopsColumn(
        FN_SOMATIC_PAIN5, Integer,
        comment="Pain: bothersome whilst doing something interesting in past "
                "week?" + CMT_BOTHERSOME_INTERESTING,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    somatic_mand2 = CamcopsColumn(
        FN_SOMATIC_MAND2, Integer,
        comment="Bodily discomfort in past month?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    somatic_dis1 = CamcopsColumn(
        FN_SOMATIC_DIS1, Integer,
        comment="Discomfort brought on or made worse because low/anxious/"
                "stressed" + CMT_NEVER_SOMETIMES_ALWAYS,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    somatic_dis2 = CamcopsColumn(
        FN_SOMATIC_DIS2, Integer,
        comment="Discomfort: days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    somatic_dis3 = CamcopsColumn(
        FN_SOMATIC_DIS3, Integer,
        comment="Discomfort: lasted >3h on any day in past week"
                + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    somatic_dis4 = CamcopsColumn(
        FN_SOMATIC_DIS4, Integer,
        comment="Discomfort: unpleasant in past week?" + CMT_UNPLEASANT,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    somatic_dis5 = CamcopsColumn(
        FN_SOMATIC_DIS5, Integer,
        comment="Discomfort: bothersome whilst doing something interesting in "
                "past week?" + CMT_BOTHERSOME_INTERESTING,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    somatic_dur = CamcopsColumn(
        FN_SOMATIC_DUR, Integer,
        comment="Duration of ache/pain/discomfort" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Fatigue/lacking energy

    fatigue_mand1 = CamcopsColumn(
        FN_FATIGUE_MAND1, Integer,
        comment="Tired in past month" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    fatigue_cause1 = CamcopsColumn(
        FN_FATIGUE_CAUSE1, Integer,
        comment="Main reason for feeling tired" + CMT_FATIGUE_CAUSE,
        permitted_value_checker=ONE_TO_EIGHT_CHECKER,
    )
    fatigue_tired1 = CamcopsColumn(
        FN_FATIGUE_TIRED1, Integer,
        comment="Tired: days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    fatigue_tired2 = CamcopsColumn(
        FN_FATIGUE_TIRED2, Integer,
        comment="Tired: >3h on any one day in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    fatigue_tired3 = CamcopsColumn(
        FN_FATIGUE_TIRED3, Integer,
        comment="So tired you've had to push yourself to get things done in "
                "past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    fatigue_tired4 = CamcopsColumn(
        FN_FATIGUE_TIRED4, Integer,
        comment="Tired during an enjoyable activity" + CMT_DURING_ENJOYABLE,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    fatigue_mand2 = CamcopsColumn(
        FN_FATIGUE_MAND2, Integer,
        comment="Lacking in energy in past month" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    fatigue_cause2 = CamcopsColumn(
        FN_FATIGUE_CAUSE2, Integer,
        comment="Main reason for lacking energy" + CMT_FATIGUE_CAUSE,
        permitted_value_checker=ONE_TO_EIGHT_CHECKER,
    )
    fatigue_energy1 = CamcopsColumn(
        FN_FATIGUE_ENERGY1, Integer,
        comment="Lacking energy: days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    fatigue_energy2 = CamcopsColumn(
        FN_FATIGUE_ENERGY2, Integer,
        comment="Lacking energy: for >3h on any one day in past week" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    fatigue_energy3 = CamcopsColumn(
        FN_FATIGUE_ENERGY3, Integer,
        comment="So lacking in energy you've had to push yourself to get "
                "things done in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    fatigue_energy4 = CamcopsColumn(
        FN_FATIGUE_ENERGY4, Integer,
        comment="Lacking energy during an enjoyable activity" +
                CMT_DURING_ENJOYABLE,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    fatigue_dur = CamcopsColumn(
        FN_FATIGUE_DUR, Integer,
        comment="Feeling tired/lacking energy for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Concentration/memory

    conc_mand1 = CamcopsColumn(
        FN_CONC_MAND1, Integer,
        comment="Problems in concentrating during past monnth?" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    conc_mand2 = CamcopsColumn(
        FN_CONC_MAND2, Integer,
        comment="Problems with forgetting things during past month?" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    conc1 = CamcopsColumn(
        FN_CONC1, Integer,
        comment="Concentration/memory problems: days in past week" +
                CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    conc2 = CamcopsColumn(
        FN_CONC2, Integer,
        comment="In past week, could concentrate on all of: TV, newspaper, "
                "conversation" + CMT_1_YES_2_NO,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    conc3 = CamcopsColumn(
        FN_CONC3, Integer,
        comment="Problems with concentration have stopped you from getting on "
                "with things in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    conc_dur = CamcopsColumn(
        FN_CONC_DUR, Integer,
        comment="Problems with concentration: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )
    conc4 = CamcopsColumn(
        FN_CONC4, Integer,
        comment="Forgotten anything important in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    forget_dur = CamcopsColumn(
        FN_FORGET_DUR, Integer,
        comment="Problems with memory: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Sleep

    sleep_mand1 = CamcopsColumn(
        FN_SLEEP_MAND1, Integer,
        comment="Problems with sleep loss in past month" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    sleep_lose1 = CamcopsColumn(
        FN_SLEEP_LOSE1, Integer,
        comment="Sleep loss: nights in past week with problems" +
                CMT_NIGHTS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    sleep_lose2 = CamcopsColumn(
        FN_SLEEP_LOSE2, Integer,
        comment="On night with least sleep in past week, how long trying to "
                "get to sleep?" + CMT_SLEEP_CHANGE,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    sleep_lose3 = CamcopsColumn(
        FN_SLEEP_LOSE3, Integer,
        comment="On how many nights in past week did you spend >=3h trying to "
                "get to sleep?" + CMT_NIGHTS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    sleep_emw = CamcopsColumn(
        FN_SLEEP_EMW, Integer,
        comment="Woken >2h earlier (and couldn't return to sleep) in past "
                "week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    sleep_cause = CamcopsColumn(
        FN_SLEEP_CAUSE, Integer,
        comment="What are your sleep difficulties caused by? (1 noise, "
                "2 shift work, 3 pain/illness, 4 worries, 5 unknown, 6 other",
        permitted_value_checker=ONE_TO_SIX_CHECKER,
    )
    sleep_mand2 = CamcopsColumn(
        FN_SLEEP_MAND2, Integer,
        comment="Problems with excess sleep in past month (1 no, 2 slept more "
                "than usual but not a problem, 3 yes)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    sleep_gain1 = CamcopsColumn(
        FN_SLEEP_GAIN1, Integer,
        comment="Sleep gain: how many nights in past week" +
                CMT_NIGHTS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    sleep_gain2 = CamcopsColumn(
        FN_SLEEP_GAIN2, Integer,
        comment="On night with most sleep in past week, how much more than "
                "usual?" + CMT_SLEEP_CHANGE,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    sleep_gain3 = CamcopsColumn(
        FN_SLEEP_GAIN3, Integer,
        comment="On how many nights in past week did you sleep >3h longer "
                "than usual?" + CMT_NIGHTS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    sleep_dur = CamcopsColumn(
        FN_SLEEP_DUR, Integer,
        comment="How long have you had these problems with sleep?" +
                CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Irritability

    irrit_mand1 = CamcopsColumn(
        FN_IRRIT_MAND1, Integer,
        comment="Irritable with those around you in past month?" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    irrit_mand2 = CamcopsColumn(
        FN_IRRIT_MAND2, Integer,
        comment="Short-tempered/angry over trivial things in past month? "
                "(1 no, 2 sometimes, 3 yes)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    irrit1 = CamcopsColumn(
        FN_IRRIT1, Integer,
        comment="Irritable/short-tempered/angry: days in past week" +
                CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    irrit2 = CamcopsColumn(
        FN_IRRIT2, Integer,
        comment="Irritable/short-tempered/angry: for >1h on any day in past "
                "week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    irrit3 = CamcopsColumn(
        FN_IRRIT3, Integer,
        comment="Irritable/short-tempered/angry: wanted to shout at someone? "
                "(1 no; yes but didn't shout; 3 yes and did shout)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    irrit4 = CamcopsColumn(
        FN_IRRIT4, Integer,
        comment="In past week, have you had arguments/rows/lost temper? "
                "(1 no; 2 yes but justified; 3 yes)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    irrit_dur = CamcopsColumn(
        FN_IRRIT_DUR, Integer,
        comment="Irritable/short-tempered/angry: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Hypochondriasis

    hypo_mand1 = CamcopsColumn(
        FN_HYPO_MAND1, Integer,
        comment="Worried about physical health in past month?" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    hypo_mand2 = CamcopsColumn(
        FN_HYPO_MAND2, Integer,
        comment="Do you worry you have a serious illness?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    hypo1 = CamcopsColumn(
        FN_HYPO1, Integer,
        comment="Worrying about health/having a serious illness: how many "
                "days in past week?" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    hypo2 = CamcopsColumn(
        FN_HYPO2, Integer,
        comment="Worrying too much about physical health?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    hypo3 = CamcopsColumn(
        FN_HYPO3, Integer,
        comment="Worrying about health: how unpleasant?" + CMT_UNPLEASANT,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    hypo4 = CamcopsColumn(
        FN_HYPO4, Integer,
        comment="Able to take mind off health worries in past week?" +
                CMT_1_YES_2_NO,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    hypo_dur = CamcopsColumn(
        FN_HYPO_DUR, Integer,
        comment="Worrying about physical health: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Depression

    depr_mand1 = CamcopsColumn(
        FN_DEPR_MAND1, Integer,
        comment="Sad/miserable/depressed in past month?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    depr1 = CamcopsColumn(
        FN_DEPR1, Integer,
        comment="Sad/miserable/depressed in past week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    depr_mand2 = CamcopsColumn(
        FN_DEPR_MAND2, Integer,
        comment="In the past month, able to enjoy/take an interest in things "
                "as much as usual?" + CMT_ANHEDONIA,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    depr2 = CamcopsColumn(
        FN_DEPR2, Integer,
        comment="In the past week, able to enjoy/take an interest in things "
                "as much as usual?" + CMT_ANHEDONIA,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    depr3 = CamcopsColumn(
        FN_DEPR3, Integer,
        comment="[Depressed mood] or [anhedonia] on how many days in past "
                "week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    depr4 = CamcopsColumn(
        FN_DEPR4, Integer,
        comment="[Depressed mood] or [anhedonia] for >3h on any day in past "
                "week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    depr_content = CamcopsColumn(
        FN_DEPR_CONTENT, Integer,
        comment="Main reason for [depressed mood] or [anhedonia]?" +
                CMT_STRESSORS,
        permitted_value_checker=ONE_TO_NINE_CHECKER,
    )
    depr5 = CamcopsColumn(
        FN_DEPR5, Integer,
        comment="In past week, during [depressed mood] or [anhedonia], did "
                "nice things/company make you happier? "
                "(1 always, 2 sometimes, 3 no)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    depr_dur = CamcopsColumn(
        FN_DEPR_DUR, Integer,
        comment="Depressed mood/anhedonia: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )
    depth1 = CamcopsColumn(
        FN_DEPTH1, Integer,
        comment="Diurnal mood variation in past week (1 worse in the morning, "
                "2 worse in the evening, 3 varies, 4 no difference)",
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    depth2 = CamcopsColumn(
        FN_DEPTH2, Integer,
        comment="Libido in past month (1 not applicable, 2 no change, "
                "3 increased, 4 decreased)",
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    depth3 = CamcopsColumn(
        FN_DEPTH3, Integer,
        comment="Restlessness in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    depth4 = CamcopsColumn(
        FN_DEPTH4, Integer,
        comment="Psychomotor retardation in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    depth5 = CamcopsColumn(
        FN_DEPTH5, Integer,
        comment="Guilt/blamed self in past week (1 never, 2 only when it was "
                "my fault, 3 sometimes, 4 often)",
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    depth6 = CamcopsColumn(
        FN_DEPTH6, Integer,
        comment="Feeling not as good as other people in past week" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    depth7 = CamcopsColumn(
        FN_DEPTH7, Integer,
        comment="Hopeless in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    depth8 = CamcopsColumn(
        FN_DEPTH8, Integer,
        comment="Life not worth living in past week (1 no, 2 sometimes, "
                "3 always)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    depth9 = CamcopsColumn(
        FN_DEPTH9, Integer,
        comment="Thoughts of suicide in past week (1 no; 2 yes, but would "
                "never commit suicide; 3 yes)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    depth10 = CamcopsColumn(
        FN_DEPTH10, Integer,
        comment="Thoughts of way to kill self in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    doctor = CamcopsColumn(
        FN_DOCTOR, Integer,
        comment="Have you spoken to your doctor about these thoughts of "
                "killing yourself (1 yes; 2 no, but have talked to other "
                "people; 3 no)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )

    # Worry/generalized anxiety

    worry_mand1 = CamcopsColumn(
        FN_WORRY_MAND1, Integer,
        comment="Excessive worry in past month?" + CMT_NO_SOMETIMES_OFTEN,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    worry_mand2 = CamcopsColumn(
        FN_WORRY_MAND2, Integer,
        comment="Any worries at all in past month?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    worry_cont1 = CamcopsColumn(
        FN_WORRY_CONT1, Integer,
        comment="Main source of worry in past week?" + CMT_STRESSORS,
        permitted_value_checker=ONE_TO_NINE_CHECKER,
    )
    worry2 = CamcopsColumn(
        FN_WORRY2, Integer,
        comment="Worries (about things other than physical health) on how "
                "many days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    worry3 = CamcopsColumn(
        FN_WORRY3, Integer,
        comment="Worrying too much?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    worry4 = CamcopsColumn(
        FN_WORRY4, Integer,
        comment="How unpleasant is worry (about things other than physical "
                "health)" + CMT_UNPLEASANT,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    worry5 = CamcopsColumn(
        FN_WORRY5, Integer,
        comment="Worry (about things other than physical health) for >3h on "
                "any day in past week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    worry_dur = CamcopsColumn(
        FN_WORRY_DUR, Integer,
        comment="Worry (about things other than physical health): for how "
                "long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    anx_mand1 = CamcopsColumn(
        FN_ANX_MAND1, Integer,
        comment="Anxious/nervous in past month?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    anx_mand2 = CamcopsColumn(
        FN_ANX_MAND2, Integer,
        comment="Muscle tension/couldn't relax in past month?" +
                CMT_NO_SOMETIMES_OFTEN,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    anx_phobia1 = CamcopsColumn(
        FN_ANX_PHOBIA1, Integer,
        comment="Phobic anxiety in past month?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    anx_phobia2 = CamcopsColumn(
        FN_ANX_PHOBIA2, Integer,
        comment="Phobic anxiety: always specific? (1 always specific, "
                "2 sometimes general)",
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    anx2 = CamcopsColumn(
        FN_ANX2, Integer,
        comment="Anxiety/nervousness/tension: how many days in past week" +
                CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    anx3 = CamcopsColumn(
        FN_ANX3, Integer,
        comment="Anxiety/nervousness/tension: how unpleasant in past week" +
                CMT_UNPLEASANT,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )
    anx4 = CamcopsColumn(
        FN_ANX4, Integer,
        comment="Anxiety/nervousness/tension: physical symptoms in past "
                "week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    anx5 = CamcopsColumn(
        FN_ANX5, Integer,
        comment="Anxiety/nervousness/tension: for >3h on any day in past "
                "week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    anx_dur = CamcopsColumn(
        FN_ANX_DUR, Integer,
        comment="Anxiety/nervousness/tension: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Specific phobias

    phobias_mand = CamcopsColumn(
        FN_PHOBIAS_MAND, Integer,
        comment="Phobic avoidance in past month?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    phobias_type1 = CamcopsColumn(
        FN_PHOBIAS_TYPE1, Integer,
        comment="Which phobia? (1 travelling alone by bus/train; 2 being far "
                "from home; 3 public eating/speaking; 4 sight of blood; "
                "5 crowded shops; 6 insects/spiders/animals; 7 being watched; "
                "8 enclosed spaces or heights; 9 something else)",
        permitted_value_checker=ONE_TO_NINE_CHECKER,
    )
    phobias1 = CamcopsColumn(
        FN_PHOBIAS1, Integer,
        comment="Phobic anxiety: days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    phobias2 = CamcopsColumn(
        FN_PHOBIAS2, Integer,
        comment="Phobic anxiety: physical symptoms in past week?" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    phobias3 = CamcopsColumn(
        FN_PHOBIAS3, Integer,
        comment="Phobic avoidance in past week?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    phobias4 = CamcopsColumn(
        FN_PHOBIAS4, Integer,
        comment="Phobic avoidance: how many times in past week? (1: none, "
                "2: 1–3, 3: >=4)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    phobias_dur = CamcopsColumn(
        FN_PHOBIAS_DUR, Integer,
        comment="Phobic anxiety: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Panic

    panic_mand = CamcopsColumn(
        FN_PANIC_MAND, Integer,
        comment="Panic in past month (1: no, my anxiety never got that bad; "
                "2: yes, sometimes; 3: yes, often)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    panic1 = CamcopsColumn(
        FN_PANIC1, Integer,
        comment="Panic: how often in past week (1 not in past seven days, "
                "2 once, 3 more than once)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    panic2 = CamcopsColumn(
        FN_PANIC2, Integer,
        comment="Panic: how unpleasant in past week (1 a little "
                "uncomfortable; 2 unpleasant; 3 unbearable, or very "
                "unpleasant)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    panic3 = CamcopsColumn(
        FN_PANIC3, Integer,
        comment="Panic: in the past week, did the worst panic last >10min "
                "(1: <10min; 2 >=10min)",
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    panic4 = CamcopsColumn(
        FN_PANIC4, Integer,
        comment="Do panics start suddenly?" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_a = CamcopsColumn(
        FN_PANSYM_A, Integer,
        comment=CMT_PANIC_SYMPTOM + "heart racing" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_b = CamcopsColumn(
        FN_PANSYM_B, Integer,
        comment=CMT_PANIC_SYMPTOM + "hands sweaty/clammy" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_c = CamcopsColumn(
        FN_PANSYM_C, Integer,
        comment=CMT_PANIC_SYMPTOM + "trembling/shaking" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_d = CamcopsColumn(
        FN_PANSYM_D, Integer,
        comment=CMT_PANIC_SYMPTOM + "short of breath" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_e = CamcopsColumn(
        FN_PANSYM_E, Integer,
        comment=CMT_PANIC_SYMPTOM + "choking sensation" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_f = CamcopsColumn(
        FN_PANSYM_F, Integer,
        comment=(CMT_PANIC_SYMPTOM + "chest pain/pressure/discomfort" +
                 CMT_1_NO_2_YES),
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_g = CamcopsColumn(
        FN_PANSYM_G, Integer,
        comment=CMT_PANIC_SYMPTOM + "nausea" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_h = CamcopsColumn(
        FN_PANSYM_H, Integer,
        comment=(CMT_PANIC_SYMPTOM + "dizzy/unsteady/lightheaded/faint" +
                 CMT_1_NO_2_YES),
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_i = CamcopsColumn(
        FN_PANSYM_I, Integer,
        comment=(CMT_PANIC_SYMPTOM + "derealization/depersonalization" +
                 CMT_1_NO_2_YES),
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_j = CamcopsColumn(
        FN_PANSYM_J, Integer,
        comment=(CMT_PANIC_SYMPTOM + "losing control/going crazy" +
                 CMT_1_NO_2_YES),
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_k = CamcopsColumn(
        FN_PANSYM_K, Integer,
        comment=CMT_PANIC_SYMPTOM + "fear were dying" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_l = CamcopsColumn(
        FN_PANSYM_L, Integer,
        comment=CMT_PANIC_SYMPTOM + "tingling/numbness" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    pansym_m = CamcopsColumn(
        FN_PANSYM_M, Integer,
        comment=CMT_PANIC_SYMPTOM + "hot flushes/chills" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    panic5 = CamcopsColumn(
        FN_PANIC5, Integer,
        comment="Is panic always brought on by specific things?" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    panic_dur = CamcopsColumn(
        FN_PANIC_DUR, Integer,
        comment="Panic: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Compulsions

    comp_mand1 = CamcopsColumn(
        FN_COMP_MAND1, Integer,
        comment="Compulsions in past month" + CMT_NO_SOMETIMES_OFTEN,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    comp1 = CamcopsColumn(
        FN_COMP1, Integer,
        comment="Compulsions: how many days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    comp2 = CamcopsColumn(
        FN_COMP2, Integer,
        comment="Compulsions: tried to stop in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    comp3 = CamcopsColumn(
        FN_COMP3, Integer,
        comment="Compulsions: upsetting/annoying in past week" +
                CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    comp4 = CamcopsColumn(
        FN_COMP4, Integer,
        comment="Compulsions: greatest number of repeats in past week "
                "(1: once, i.e. two times altogether; 2: two repeats; "
                "3: three or more repeats)",
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    comp_dur = CamcopsColumn(
        FN_COMP_DUR, Integer,
        comment="Compulsions: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Obsessions

    obsess_mand1 = CamcopsColumn(
        FN_OBSESS_MAND1, Integer,
        comment="Obsessions in past month" + CMT_NO_SOMETIMES_OFTEN,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    obsess_mand2 = CamcopsColumn(
        FN_OBSESS_MAND2, Integer,
        comment="Obsessions: same thoughts repeating or general worries (1 "
                "same thoughts over and over, 2 worrying about something in "
                "general)",
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    obsess1 = CamcopsColumn(
        FN_OBSESS1, Integer,
        comment="Obsessions: how many days in past week" + CMT_DAYS_PER_WEEK,
        permitted_value_checker=ONE_TO_THREE_CHECKER,
    )
    obsess2 = CamcopsColumn(
        FN_OBSESS2, Integer,
        comment="Obsessions: tried to stop in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    obsess3 = CamcopsColumn(
        FN_OBSESS3, Integer,
        comment="Obsessions: upsetting/annoying in past week" + CMT_1_NO_2_YES,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    obsess4 = CamcopsColumn(
        FN_OBSESS4, Integer,
        comment="Obsessions: longest time spent thinking these thoughts, in "
                "past week (1: <15min; 2: >=15min)",
        permitted_value_checker=ONE_TO_TWO_CHECKER,
    )
    obsess_dur = CamcopsColumn(
        FN_OBSESS_DUR, Integer,
        comment="Obsessions: for how long?" + CMT_DURATION,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
    )

    # Overall impact

    overall2 = CamcopsColumn(
        FN_OVERALL2, Integer,
        comment="Overall impact on normal activities in past week (1 not at "
                "all; 2 they have made things more difficult but I get "
                "everything done; 3 they have stopped one activity; 4 they "
                "have stopped >1 activity)",
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
    )

    # -------------------------------------------------------------------------
    # Functions
    # -------------------------------------------------------------------------

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Clinical Interview Schedule, Revised")

    # noinspection PyMethodParameters
    @classproperty
    def minimum_client_version(cls) -> Version:
        return Version("2.2.0")

    def value_for_question(self, q: CisrQuestion) -> Optional[int]:
        fieldname = fieldname_for_q(q)
        assert fieldname, f"Blank fieldname for question {q}"
        return getattr(self, fieldname)

    def int_value_for_question(self, q: CisrQuestion) -> int:
        value = self.value_for_question(q)
        return int(value) if value is not None else 0

    def answer_is_no(self, q: CisrQuestion, value: int = V_UNKNOWN) -> bool:
        if value == V_UNKNOWN:  # "Please look it up for me"
            value = self.int_value_for_question(q)
        if q in QUESTIONS_1_NO_2_YES:
            return value == 1
        elif q in QUESTIONS_1_YES_2_NO:
            return value == 2
        else:
            raise ValueError("answer_is_no() called for inappropriate "
                             f"question {q}")

    def answer_is_yes(self, q: CisrQuestion, value: int = V_UNKNOWN) -> bool:
        if value == V_UNKNOWN:  # "Please look it up for me"
            value = self.int_value_for_question(q)
        if q in QUESTIONS_1_NO_2_YES:
            return value == 2
        elif q in QUESTIONS_1_YES_2_NO:
            return value == 1
        else:
            raise ValueError("answer_is_yes() called for inappropriate "
                             f"question {q}")

    def answered(self, q: CisrQuestion, value: int = V_UNKNOWN) -> bool:
        if value == V_UNKNOWN:  # "Please look it up for me"
            value = self.int_value_for_question(q)
        return value != V_MISSING

    def get_textual_answer(self, req: CamcopsRequest,
                           q: CisrQuestion) -> Optional[str]:
        value = self.value_for_question(q)
        if value is None or value == V_MISSING:
            return None
        if q in QUESTIONS_1_NO_2_YES:
            return get_yes_no(req, value == 2)
        elif q in QUESTIONS_1_YES_2_NO:
            return get_yes_no(req, value == 1)
        elif q in QUESTIONS_PROMPT_ONLY:
            return NOT_APPLICABLE_TEXT
        fieldname = fieldname_for_q(q)
        if (q in QUESTIONS_YN_SPECIFIC_TEXT or
                q in QUESTIONS_MULTIWAY or
                q in QUESTIONS_MULTIWAY_WITH_EXTRA_STEM):
            return self.wxstring(req, fieldname + f"_a{value}")
        elif q in QUESTIONS_OVERALL_DURATION:
            return self.wxstring(req, f"duration_a{value}")
        elif q in QUESTIONS_DAYS_PER_WEEK:
            return self.wxstring(req, f"dpw_a{value}")
        elif q in QUESTIONS_NIGHTS_PER_WEEK:
            return self.wxstring(req, f"npw_a{value}")
        elif q in QUESTIONS_HOW_UNPLEASANT_STANDARD:
            return self.wxstring(req, f"how_unpleasant_a{value}")
        elif q in QUESTIONS_FATIGUE_CAUSES:
            return self.wxstring(req, f"fatigue_causes_a{value}")
        elif q in QUESTIONS_STRESSORS:
            return self.wxstring(req, f"stressors_a{value}")
        elif q in QUESTIONS_NO_SOMETIMES_OFTEN:
            return self.wxstring(req, f"nso_a{value}")
        return f"? [value: {value}]"

    def next_q(self, q: CisrQuestion, r: CisrResult) -> CisrQuestion:
        # See equivalent in the C++ code.
        # ANY CHANGES HERE MUST BE REFLECTED IN THE C++ CODE AND VICE VERSA.

        v = V_MISSING  # integer value
        if DEBUG_SHOW_QUESTIONS_CONSIDERED:
            r.decide(f"Considering question {q.value}: {q.name}")
        fieldname = fieldname_for_q(q)
        if fieldname:  # eliminates prompt-only questions
            var_q = getattr(self, fieldname)  # integer-or-NULL value
            if var_q is None:
                if q not in QUESTIONS_DEMOGRAPHICS:
                    # From a diagnostic point of view, OK to have missing
                    # demographic information. Otherwise:
                    r.decide("INCOMPLETE INFORMATION. STOPPING.")
                    r.incomplete = True
            else:
                v = int(var_q)

        next_q = -1

        def jump_to(qe: CisrQuestion) -> None:
            nonlocal next_q
            next_q = enum_to_int(qe)

        # If there is no special handling for a question, then after the
        # switch() statement we will move to the next question in sequence.
        # So only special "skip" situations are handled here.

        # FOLLOW THE EXACT SEQUENCE of the CIS-R. Don't agglomerate case
        # statements just because it's shorter (except empty ones when they are
        # in sequence). Clarity is key.

        # ---------------------------------------------------------------------
        # Demographics/preamble
        # ---------------------------------------------------------------------

        if q in QUESTIONS_DEMOGRAPHICS or q in QUESTIONS_PROMPT_ONLY:
            # Nothing special
            pass
            # Note that this makes some of the other prompt-only checks
            # below redundant! Still, it's quicker. The C++ version uses
            # switch() instead.

        # --------------------------------------------------------------------
        # Appetite/weight
        # --------------------------------------------------------------------

        elif q == CQ.APPETITE1_LOSS_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("No loss of appetite in past month.")
                jump_to(CQ.APPETITE2_INCREASE_PAST_MONTH)
            elif self.answer_is_yes(q, v):
                r.decide("Loss of appetite in past month. "
                         "Incrementing depr_crit_3_somatic_synd.")
                r.depr_crit_3_somatic_synd += 1
                r.weight_change = WTCHANGE_APPETITE_LOSS

        elif q == CQ.WEIGHT1_LOSS_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("No weight loss.")
                jump_to(CQ.GP_YEAR)

        elif q == CQ.WEIGHT2_TRYING_TO_LOSE:
            if v == V_WEIGHT2_WTLOSS_TRYING:
                # Trying to lose weight. Move on.
                r.decide("Weight loss but it was deliberate.")
            elif v == V_WEIGHT2_WTLOSS_NOTTRYING:
                r.decide("Non-deliberate weight loss.")
                r.weight_change = WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN

        elif q == CQ.WEIGHT3_LOST_LOTS:
            if v == V_WEIGHT3_WTLOSS_GE_HALF_STONE:
                r.decide("Weight loss ≥0.5st in past month. "
                         "Incrementing depr_crit_3_somatic_synd.")
                r.weight_change = WTCHANGE_WTLOSS_GE_HALF_STONE
                r.depr_crit_3_somatic_synd += 1
            r.decide("Loss of weight, so skipping appetite/weight gain "
                     "questions.")
            jump_to(CQ.GP_YEAR)

        elif q == CQ.APPETITE2_INCREASE_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("No increase in appetite in past month.")
                jump_to(CQ.GP_YEAR)

        elif q == CQ.WEIGHT4_INCREASE_PAST_MONTH:
            if self.answer_is_yes(q, v):
                r.decide("Weight gain.")
                r.weight_change = WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN
            elif self.answered(q, v):
                r.decide("No weight gain, or weight gain but pregnant.")
                jump_to(CQ.GP_YEAR)

        elif q == CQ.WEIGHT5_GAINED_LOTS:
            if (v == V_WEIGHT5_WTGAIN_GE_HALF_STONE and
                    r.weight_change == WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN):  # noqa
                # ... redundant check on weight_change, I think!
                r.decide("Weight gain ≥0.5 st in past month.")
                r.weight_change = WTCHANGE_WTGAIN_GE_HALF_STONE

        # --------------------------------------------------------------------
        # Somatic symptoms
        # --------------------------------------------------------------------

        elif q == CQ.GP_YEAR:
            # Score the preceding block:
            if (r.weight_change == WTCHANGE_WTLOSS_GE_HALF_STONE and
                    self.answer_is_yes(CQ.APPETITE1_LOSS_PAST_MONTH)):
                r.decide(
                    "Appetite loss and weight loss ≥0.5st in past month. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.")
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1
            if (r.weight_change == WTCHANGE_WTGAIN_GE_HALF_STONE and
                    self.answer_is_yes(CQ.APPETITE2_INCREASE_PAST_MONTH)):
                r.decide(
                    "Appetite gain and weight gain ≥0.5st in past month. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.")
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1

        elif q == CQ.DISABLE:
            if self.answer_is_no(q):
                r.decide("No longstanding illness/disability/infirmity.")
                jump_to(CQ.SOMATIC_MAND1_PAIN_PAST_MONTH)

        elif q == CQ.ILLNESS:
            pass

        elif q == CQ.SOMATIC_MAND1_PAIN_PAST_MONTH:
            if self.answer_is_no(q):
                r.decide("No aches/pains in past month.")
                jump_to(CQ.SOMATIC_MAND2_DISCOMFORT)

        elif q == CQ.SOMATIC_PAIN1_PSYCHOL_EXAC:
            if v == V_SOMATIC_PAIN1_NEVER:
                r.decide("Pains never exacerbated by low mood/anxiety/stress.")
                jump_to(CQ.SOMATIC_MAND2_DISCOMFORT)

        elif q == CQ.SOMATIC_PAIN2_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("No pain in last 7 days.")
                jump_to(CQ.SOMATIC_MAND2_DISCOMFORT)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Pain on >=4 of last 7 days. "
                         "Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1

        elif q == CQ.SOMATIC_PAIN3_GT_3H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Pain for >3h on any day in past week. "
                         "Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1

        elif q == CQ.SOMATIC_PAIN4_UNPLEASANT:
            if v >= V_HOW_UNPLEASANT_UNPLEASANT:
                r.decide("Pain 'unpleasant' or worse in past week. "
                         "Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1

        elif q == CQ.SOMATIC_PAIN5_INTERRUPTED_INTERESTING:
            if self.answer_is_yes(q, v):
                r.decide("Pain interrupted an interesting activity in past "
                         "week. "
                         "Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1
            r.decide("There was pain, so skip 'discomfort' section.")
            jump_to(CQ.SOMATIC_DUR)  # skip SOMATIC_MAND2

        elif q == CQ.SOMATIC_MAND2_DISCOMFORT:
            if self.answer_is_no(q, v):
                r.decide("No discomfort.")
                jump_to(CQ.FATIGUE_MAND1_TIRED_PAST_MONTH)

        elif q == CQ.SOMATIC_DIS1_PSYCHOL_EXAC:
            if v == V_SOMATIC_DIS1_NEVER:
                r.decide("Discomfort never exacerbated by being "
                         "low/anxious/stressed.")
                jump_to(CQ.FATIGUE_MAND1_TIRED_PAST_MONTH)

        elif q == CQ.SOMATIC_DIS2_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("No discomfort in last 7 days.")
                jump_to(CQ.FATIGUE_MAND1_TIRED_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Discomfort on >=4 days in past week. "
                         "Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1

        elif q == CQ.SOMATIC_DIS3_GT_3H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Discomfort for >3h on any day in past week. "
                         "Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1

        elif q == CQ.SOMATIC_DIS4_UNPLEASANT:
            if v >= V_HOW_UNPLEASANT_UNPLEASANT:
                r.decide("Discomfort 'unpleasant' or worse in past week. "
                         "Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1

        elif q == CQ.SOMATIC_DIS5_INTERRUPTED_INTERESTING:
            if self.answer_is_yes(q, v):
                r.decide("Discomfort interrupted an interesting activity in "
                         "past "
                         "week. Incrementing somatic_symptoms.")
                r.somatic_symptoms += 1

        # --------------------------------------------------------------------
        # Fatigue/energy
        # --------------------------------------------------------------------

        elif q == CQ.FATIGUE_MAND1_TIRED_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("Not tired.")
                jump_to(CQ.FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH)

        elif q == CQ.FATIGUE_CAUSE1_TIRED:
            if v == V_FATIGUE_CAUSE_EXERCISE:
                r.decide("Tired due to exercise. Move on.")
                jump_to(CQ.CONC_MAND1_POOR_CONC_PAST_MONTH)

        elif q == CQ.FATIGUE_TIRED1_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("Not tired in past week.")
                jump_to(CQ.FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Tired on >=4 days in past week. "
                         "Incrementing fatigue.")
                r.fatigue += 1

        elif q == CQ.FATIGUE_TIRED2_GT_3H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Tired for >3h on any day in past week. "
                         "Incrementing fatigue.")
                r.fatigue += 1

        elif q == CQ.FATIGUE_TIRED3_HAD_TO_PUSH:
            if self.answer_is_yes(q, v):
                r.decide("Tired enough to have to push self during past week. "
                         "Incrementing fatigue.")
                r.fatigue += 1

        elif q == CQ.FATIGUE_TIRED4_DURING_ENJOYABLE:
            if self.answer_is_yes(q, v):
                r.decide("Tired during an enjoyable activity during past "
                         "week. "
                         "Incrementing fatigue.")
                r.fatigue += 1
            r.decide("There was tiredness, so skip 'lack of energy' section.")
            jump_to(CQ.FATIGUE_DUR)  # skip FATIGUE_MAND2

        elif q == CQ.FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("Not lacking in energy.")
                jump_to(CQ.CONC_MAND1_POOR_CONC_PAST_MONTH)

        elif q == CQ.FATIGUE_CAUSE2_LACK_ENERGY:
            if v == V_FATIGUE_CAUSE_EXERCISE:
                r.decide("Lacking in energy due to exercise. Move on.")
                jump_to(CQ.CONC_MAND1_POOR_CONC_PAST_MONTH)

        elif q == CQ.FATIGUE_ENERGY1_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("Not lacking in energy during last week.")
                jump_to(CQ.CONC_MAND1_POOR_CONC_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Lacking in energy on >=4 days in past week. "
                         "Incrementing fatigue.")
                r.fatigue += 1

        elif q == CQ.FATIGUE_ENERGY2_GT_3H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Lacking in energy for >3h on any day in past week. "
                         "Incrementing fatigue.")
                r.fatigue += 1

        elif q == CQ.FATIGUE_ENERGY3_HAD_TO_PUSH:
            if self.answer_is_yes(q, v):
                r.decide("Lacking in energy enough to have to push self during "
                         "past week. Incrementing fatigue.")
                r.fatigue += 1

        elif q == CQ.FATIGUE_ENERGY4_DURING_ENJOYABLE:
            if self.answer_is_yes(q, v):
                r.decide("Lacking in energy during an enjoyable activity "
                         "during "
                         "past week. Incrementing fatigue.")
                r.fatigue += 1

        elif q == CQ.FATIGUE_DUR:
            # Score preceding:
            if r.somatic_symptoms >= 2 and r.fatigue >= 2:
                r.decide("somatic >= 2 and fatigue >= 2. "
                         "Incrementing neurasthenia.")
                r.neurasthenia += 1

        # --------------------------------------------------------------------
        # Concentration/memory
        # --------------------------------------------------------------------

        elif q == CQ.CONC_MAND1_POOR_CONC_PAST_MONTH:
            # Score preceding:
            if r.fatigue >= 2:
                r.decide("fatigue >= 2. "
                         "Incrementing depr_crit_1_mood_anhedonia_energy.")
                r.depr_crit_1_mood_anhedonia_energy += 1

        elif q == CQ.CONC_MAND2_FORGETFUL_PAST_MONTH:
            if (self.answer_is_no(CQ.CONC_MAND1_POOR_CONC_PAST_MONTH)
                    and self.answer_is_no(q, v)):
                r.decide("No problems with concentration or forgetfulness.")
                jump_to(CQ.SLEEP_MAND1_LOSS_PAST_MONTH)

        elif q == CQ.CONC1_CONC_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("No concentration/memory problems in past week.")
                jump_to(CQ.SLEEP_MAND1_LOSS_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Problems with concentration/memory problems on >=4 "
                         "days in past week. Incrementing concentration_poor.")
                r.concentration_poor += 1
            if (self.answer_is_no(CQ.CONC_MAND1_POOR_CONC_PAST_MONTH) and
                    self.answer_is_yes(CQ.CONC_MAND2_FORGETFUL_PAST_MONTH)):
                r.decide("Forgetfulness, not concentration, problems; skip "
                         "over more detailed concentration questions.")
                jump_to(CQ.CONC4_FORGOTTEN_IMPORTANT)  # skip CONC2, CONC3, CONC_DUR  # noqa

        elif q == CQ.CONC2_CONC_FOR_TV_READING_CONVERSATION:
            if self.answer_is_no(q, v):
                r.decide("Couldn't concentrate on at least one of {TV, "
                         "newspaper, "
                         "conversation}. Incrementing concentration_poor.")
                r.concentration_poor += 1

        elif q == CQ.CONC3_CONC_PREVENTED_ACTIVITIES:
            if self.answer_is_yes(q, v):
                r.decide("Problems with concentration stopped usual/desired "
                         "activity. Incrementing concentration_poor.")
                r.concentration_poor += 1

        elif q == CQ.CONC_DUR:
            if self.answer_is_no(CQ.CONC_MAND2_FORGETFUL_PAST_MONTH):
                jump_to(CQ.SLEEP_MAND1_LOSS_PAST_MONTH)

        elif q == CQ.CONC4_FORGOTTEN_IMPORTANT:
            if self.answer_is_yes(q, v):
                r.decide("Forgotten something important in past week. "
                         "Incrementing concentration_poor.")
                r.concentration_poor += 1

        elif q == CQ.FORGET_DUR:
            pass

        # --------------------------------------------------------------------
        # Sleep
        # --------------------------------------------------------------------

        elif q == CQ.SLEEP_MAND1_LOSS_PAST_MONTH:
            # Score previous block:
            if r.concentration_poor >= 2:
                r.decide(
                    "concentration >= 2. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.")
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1
            # This question:
            if self.answer_is_no(q, v):
                r.decide("No problems with sleep loss in past month. "
                         "Moving on.")
                jump_to(CQ.SLEEP_MAND2_GAIN_PAST_MONTH)

        elif q == CQ.SLEEP_LOSE1_NIGHTS_PAST_WEEK:
            if v == V_NIGHTS_IN_PAST_WEEK_0:
                r.decide("No problems with sleep in past week. Moving on.")
                jump_to(CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH)
            elif v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Problems with sleep on >=4 nights in past week. "
                         "Incrementing sleep_problems.")
                r.sleep_problems += 1

        elif q == CQ.SLEEP_LOSE2_DIS_WORST_DURATION:
            if v == V_SLEEP_CHANGE_LT_15_MIN:
                r.decide("Less than 15min maximum delayed initiation of sleep "
                         "in past week. Moving on.")
                jump_to(CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH)
            elif v == V_SLEEP_CHANGE_15_MIN_TO_1_H:
                r.decide("15min-1h maximum delayed initiation of sleep in past "
                         "week. Incrementing sleep_problems.")
                r.sleep_problems += 1
            elif v == V_SLEEP_CHANGE_1_TO_3_H or v == V_SLEEP_CHANGE_GT_3_H:
                r.decide(">=1h maximum delayed initiation of sleep in past "
                         "week. Adding 2 to sleep_problems.")
                r.sleep_problems += 2

        elif q == CQ.SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK:
            if v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE:
                r.decide(">=4 nights in past week with >=3h delayed "
                         "initiation of "
                         "sleep. Incrementing sleep_problems.")
                r.sleep_problems += 1

        elif q == CQ.SLEEP_EMW_PAST_WEEK:
            if self.answer_is_yes(q, v):
                r.decide("EMW of >2h in past week. "
                         "Setting sleep_change to SLEEPCHANGE_EMW. "
                         "Incrementing depr_crit_3_somatic_synd.")
                # Was: SLEEPCH += answer - 1 (which only does anything for a
                # "yes" (2) answer).
                # ... but at this point, SLEEPCH is always 0.
                r.sleep_change = SLEEPCHANGE_EMW  # LIKELY REDUNDANT.
                r.depr_crit_3_somatic_synd += 1
                if r.sleep_problems >= 1:
                    r.decide("EMW of >2h in past week and sleep_problems >= 1; "
                             "setting sleep_change to SLEEPCHANGE_EMW.")
                    r.sleep_change = SLEEPCHANGE_EMW
            elif self.answer_is_no(q, v):
                r.decide("No EMW of >2h in past week.")
                if r.sleep_problems >= 1:
                    r.decide("No EMW of >2h in past week, and sleep_problems "
                             ">= 1. Setting sleep_change to "
                             "SLEEPCHANGE_INSOMNIA_NOT_EMW.")
                    r.sleep_change = SLEEPCHANGE_INSOMNIA_NOT_EMW

        elif q == CQ.SLEEP_CAUSE:
            r.decide("Problems with sleep loss; skipping over sleep gain.")
            jump_to(CQ.SLEEP_DUR)

        elif q == CQ.SLEEP_MAND2_GAIN_PAST_MONTH:
            if (v == V_SLEEP_MAND2_NO or
                    v == V_SLEEP_MAND2_YES_BUT_NOT_A_PROBLEM):
                r.decide("No problematic sleep gain. Moving on.")
                jump_to(CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH)

        elif q == CQ.SLEEP_GAIN1_NIGHTS_PAST_WEEK:
            if v == V_NIGHTS_IN_PAST_WEEK_0:
                r.decide("No nights with sleep problems [gain] in past week.")
                jump_to(CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH)
            elif v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Problems with sleep [gain] on >=4 nights in past "
                         "week. Incrementing sleep_problems.")
                r.sleep_problems += 1

        elif q == CQ.SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT:
            if v == V_SLEEP_CHANGE_LT_15_MIN:
                r.decide("Sleep gain <15min. Moving on.")
                jump_to(CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH)
            elif v == V_SLEEP_CHANGE_15_MIN_TO_1_H:
                r.decide("Sleep gain 15min-1h. Incrementing sleep_problems.")
                r.sleep_problems += 1
            elif v >= V_SLEEP_CHANGE_1_TO_3_H:
                r.decide("Sleep gain >=1h. "
                         "Adding 2 to sleep_problems. "
                         "Setting sleep_change to SLEEPCHANGE_INCREASE.")
                r.sleep_problems += 2
                r.sleep_change = SLEEPCHANGE_INCREASE
                # Note that in the original, if the answer was 3
                # (V_SLEEP_CHANGE_1_TO_3_H) or greater, first 2 was added to
                # sleep, and then if sleep was >=1, sleepch [sleep_change] was set  # noqa
                # to 3. However, sleep is never decremented/set below 0, so that  # noqa
                # was a redundant test (always true).

        elif q == CQ.SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK:
            if v == V_NIGHTS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Sleep gain of >3h on >=4 nights in past week. "
                         "Incrementing sleep_problems.")
                r.sleep_problems += 1

        elif q == CQ.SLEEP_DUR:
            pass

        # --------------------------------------------------------------------
        # Irritability
        # --------------------------------------------------------------------

        elif q == CQ.IRRIT_MAND1_PEOPLE_PAST_MONTH:
            # Score previous block:
            if r.sleep_problems >= 2:
                r.decide(
                    "sleep_problems >= 2. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.")
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1
            # This bit erroneously lived under IRRIT_DUR in the original; see
            # discussion there:
            if r.sleep_problems >= 2 and r.fatigue >= 2:
                r.decide("sleep_problems >=2 and fatigue >=2. "
                         "Incrementing neurasthenia.")
                r.neurasthenia += 1
            # This question:
            if self.answer_is_yes(q, v):
                r.decide("Irritability (people) in past month; exploring "
                         "further.")
                jump_to(CQ.IRRIT1_DAYS_PER_WEEK)

        elif q == CQ.IRRIT_MAND2_THINGS_PAST_MONTH:
            if v == V_IRRIT_MAND2_NO:
                r.decide("No irritability. Moving on.")
                jump_to(CQ.HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH)
            elif self.answered(q, v):
                r.decide("Irritability (things) in past month; exploring "
                         "further.")

        elif q == CQ.IRRIT1_DAYS_PER_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("No irritability in past week. Moving on.")
                jump_to(CQ.HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Irritable on >=4 days in past week. "
                         "Incrementing irritability.")
                r.irritability += 1

        elif q == CQ.IRRIT2_GT_1H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Irritable for >1h on any day in past week. "
                         "Incrementing irritability.")
                r.irritability += 1

        elif q == CQ.IRRIT3_WANTED_TO_SHOUT:
            if v >= V_IRRIT3_SHOUTING_WANTED_TO:
                r.decide("Wanted to or did shout. Incrementing irritability.")
                r.irritability += 1

        elif q == CQ.IRRIT4_ARGUMENTS:
            if v == V_IRRIT4_ARGUMENTS_YES_UNJUSTIFIED:
                r.decide("Arguments without justification. "
                         "Incrementing irritability.")
                r.irritability += 1

        elif q == CQ.IRRIT_DUR:
            # Score recent things:
            if r.irritability >= 2 and r.fatigue >= 2:
                r.decide("irritability >=2 and fatigue >=2. "
                         "Incrementing neurasthenia.")
                r.neurasthenia += 1
            # In the original, we had the rule "sleep_problems >=2 and
            # fatigue >=2 -> incrementing neurasthenia" here, but that would mean  # noqa
            # we would fail to score sleep if the patient didn't report
            # irritability (because if you say no at IRRIT_MAND2, you jump beyond  # noqa
            # this point to HYPO_MAND1). Checked with Glyn Lewis 2017-12-04, who  # noqa
            # agreed on 2017-12-05. Therefore, moved to IRRIT_MAND1 as above.
            # Note that the only implication would have been potential small
            # mis-scoring of the CFS criterion (not any of the diagnoses that
            # the CIS-R reports as its primary/secondary diagnoses).

        # --------------------------------------------------------------------
        # Hypochondriasis
        # --------------------------------------------------------------------

        elif q == CQ.HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH:
            if self.answer_is_yes(q, v):
                r.decide("No worries about physical health in past month. "
                         "Moving on.")
                jump_to(CQ.HYPO1_DAYS_PAST_WEEK)

        elif q == CQ.HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS:
            if self.answer_is_no(q, v):
                r.decide("No worries about having a serious illness. "
                         "Moving on.")
                jump_to(CQ.DEPR_MAND1_LOW_MOOD_PAST_MONTH)

        elif q == CQ.HYPO1_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("No days in past week worrying about health. "
                         "Moving on.")
                jump_to(CQ.DEPR_MAND1_LOW_MOOD_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Worries about health on >=4 days in past week. "
                         "Incrementing hypochondria.")
                r.hypochondria += 1

        elif q == CQ.HYPO2_WORRY_TOO_MUCH:
            if self.answer_is_yes(q, v):
                r.decide("Worrying too much about health. "
                         "Incrementing hypochondria.")
                r.hypochondria += 1

        elif q == CQ.HYPO3_HOW_UNPLEASANT:
            if v >= V_HOW_UNPLEASANT_UNPLEASANT:
                r.decide("Worrying re health 'unpleasant' or worse in past "
                         "week. Incrementing hypochondria.")
                r.hypochondria += 1

        elif q == CQ.HYPO4_CAN_DISTRACT:
            if self.answer_is_no(q, v):
                r.decide("Cannot take mind off health worries by doing "
                         "something else. Incrementing hypochondria.")
                r.hypochondria += 1

        elif q == CQ.HYPO_DUR:
            pass

        # --------------------------------------------------------------------
        # Depression
        # --------------------------------------------------------------------

        elif q == CQ.DEPR_MAND1_LOW_MOOD_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("Mood not low in past month. Moving to anhedonia.")
                jump_to(CQ.DEPR_MAND2_ENJOYMENT_PAST_MONTH)

        elif q == CQ.DEPR1_LOW_MOOD_PAST_WEEK:
            pass

        elif q == CQ.DEPR_MAND2_ENJOYMENT_PAST_MONTH:
            if (v == V_ANHEDONIA_ENJOYING_NORMALLY and
                    self.answer_is_no(CQ.DEPR1_LOW_MOOD_PAST_WEEK)):
                r.decide("Neither low mood nor anhedonia in past month. "
                         "Moving on.")
                jump_to(CQ.WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH)

        elif q == CQ.DEPR2_ENJOYMENT_PAST_WEEK:
            if (v == V_ANHEDONIA_ENJOYING_NORMALLY and
                    self.answer_is_no(CQ.DEPR_MAND1_LOW_MOOD_PAST_MONTH)):
                r.decide("No anhedonia in past week and no low mood in past "
                         "month. Moving on.")
                jump_to(CQ.WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH)
            elif v >= V_ANHEDONIA_ENJOYING_LESS:
                r.decide("Partial or complete anhedonia in past week. "
                         "Incrementing depression. "
                         "Incrementing depr_crit_1_mood_anhedonia_energy. "
                         "Incrementing depr_crit_3_somatic_synd.")
                r.depression += 1
                r.depr_crit_1_mood_anhedonia_energy += 1
                r.depr_crit_3_somatic_synd += 1

        elif q == CQ.DEPR3_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Low mood or anhedonia on >=4 days in past week. "
                         "Incrementing depression.")
                r.depression += 1

        elif q == CQ.DEPR4_GT_3H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Low mood or anhedonia for >3h/day on at least one "
                         "day in past week. Incrementing depression.")
                r.depression += 1
                if (self.int_value_for_question(CQ.DEPR3_DAYS_PAST_WEEK) and
                        self.answer_is_yes(CQ.DEPR1_LOW_MOOD_PAST_WEEK)):
                    r.decide("(A) Low mood in past week, and "
                             "(B) low mood or anhedonia for >3h/day on at "
                             "least one day in past week, and "
                             "(C) low mood or anhedonia on >=4 days in past "
                             "week. "
                             "Incrementing depr_crit_1_mood_anhedonia_energy.")
                    r.depr_crit_1_mood_anhedonia_energy += 1

        elif q == CQ.DEPR_CONTENT:
            pass

        elif q == CQ.DEPR5_COULD_CHEER_UP:
            if v >= V_DEPR5_COULD_CHEER_UP_SOMETIMES:
                r.decide("'Sometimes' or 'never' cheered up by nice things. "
                         "Incrementing depression. "
                         "Incrementing depr_crit_3_somatic_synd.")
                r.depression += 1
                r.depr_crit_3_somatic_synd += 1

        elif q == CQ.DEPR_DUR:
            if v >= V_DURATION_2W_6M:
                r.decide("Depressive symptoms for >=2 weeks. "
                         "Setting depression_at_least_2_weeks.")
                r.depression_at_least_2_weeks = True
            # This code was at the start of DEPTH1, but involves skipping over
            # DEPTH1; since we never get to DEPTH1 without coming here, we can
            # move it here:
            if r.depression == 0:
                r.decide("Score for 'depression' is 0; skipping over "
                         "depressive thought content questions.")
                jump_to(CQ.WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH)

        elif q == CQ.DEPTH1_DIURNAL_VARIATION:
            if (v == V_DEPTH1_DMV_WORSE_MORNING or
                    v == V_DEPTH1_DMV_WORSE_EVENING):
                r.decide("Diurnal mood variation present.")
                r.diurnal_mood_variation = (
                    DIURNAL_MOOD_VAR_WORSE_MORNING
                    if v == V_DEPTH1_DMV_WORSE_MORNING
                    else DIURNAL_MOOD_VAR_WORSE_EVENING
                )
                if v == V_DEPTH1_DMV_WORSE_MORNING:
                    r.decide("Diurnal mood variation, worse in the mornings. "
                             "Incrementing depr_crit_3_somatic_synd.")
                    r.depr_crit_3_somatic_synd += 1

        elif q == CQ.DEPTH2_LIBIDO:
            if v == V_DEPTH2_LIBIDO_DECREASED:
                r.decide("Libido decreased over past month. "
                         "Setting libido_decreased. "
                         "Incrementing depr_crit_3_somatic_synd.")
                r.libido_decreased = True
                r.depr_crit_3_somatic_synd += 1

        elif q == CQ.DEPTH3_RESTLESS:
            if self.answer_is_yes(q):
                r.decide("Psychomotor agitation.")
                r.psychomotor_changes = PSYCHOMOTOR_AGITATION

        elif q == CQ.DEPTH4_SLOWED:
            if self.answer_is_yes(q):
                r.decide("Psychomotor retardation.")
                r.psychomotor_changes = PSYCHOMOTOR_RETARDATION
            if r.psychomotor_changes > PSYCHOMOTOR_NONE:
                r.decide(
                    "Psychomotor agitation or retardation. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui. "
                    "Incrementing depr_crit_3_somatic_synd.")
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1
                r.depr_crit_3_somatic_synd += 1

        elif q == CQ.DEPTH5_GUILT:
            if v >= V_DEPTH5_GUILT_SOMETIMES:
                r.decide(
                    "Feel guilty when not at fault sometimes or often. "
                    "Incrementing depressive_thoughts. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.")
                r.depressive_thoughts += 1
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1

        elif q == CQ.DEPTH6_WORSE_THAN_OTHERS:
            if self.answer_is_yes(q, v):
                r.decide(
                    "Feeling not as good as other people. "
                    "Incrementing depressive_thoughts. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.")
                r.depressive_thoughts += 1
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1

        elif q == CQ.DEPTH7_HOPELESS:
            if self.answer_is_yes(q, v):
                r.decide("Hopelessness. "
                         "Incrementing depressive_thoughts. "
                         "Setting suicidality to "
                         "SUICIDE_INTENT_HOPELESS_NO_SUICIDAL_THOUGHTS.")
                r.depressive_thoughts += 1
                r.suicidality = SUICIDE_INTENT_HOPELESS_NO_SUICIDAL_THOUGHTS

        elif q == CQ.DEPTH8_LNWL:
            if v == V_DEPTH8_LNWL_NO:
                r.decide("No thoughts of life not being worth living. "
                         "Skipping to end of depression section.")
                jump_to(CQ.DEPR_OUTRO)
            elif v >= V_DEPTH8_LNWL_SOMETIMES:
                r.decide("Sometimes or always feeling life isn't worth living. "
                         "Incrementing depressive_thoughts. "
                         "Setting suicidality to "
                         "SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING.")
                r.depressive_thoughts += 1
                r.suicidality = SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING

        elif q == CQ.DEPTH9_SUICIDE_THOUGHTS:
            if v == V_DEPTH9_SUICIDAL_THOUGHTS_NO:
                r.decide("No thoughts of suicide. Skipping to end of "
                         "depression section.")
                jump_to(CQ.DEPR_OUTRO)
            if v >= V_DEPTH9_SUICIDAL_THOUGHTS_YES_BUT_NEVER_WOULD:
                r.decide("Suicidal thoughts present. "
                         "Setting suicidality to "
                         "SUICIDE_INTENT_SUICIDAL_THOUGHTS.")
                r.suicidality = SUICIDE_INTENT_SUICIDAL_THOUGHTS
            if v == V_DEPTH9_SUICIDAL_THOUGHTS_YES_BUT_NEVER_WOULD:
                r.decide("Suicidal thoughts present but denies would ever act. "
                         "Skipping to talk-to-doctor section.")
                jump_to(CQ.DOCTOR)
            if v == V_DEPTH9_SUICIDAL_THOUGHTS_YES:
                r.decide(
                    "Thoughts of suicide in past week. "
                    "Incrementing depressive_thoughts. "
                    "Incrementing depr_crit_2_app_cnc_slp_mtr_glt_wth_sui.")
                r.depressive_thoughts += 1
                r.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui += 1

        elif q == CQ.DEPTH10_SUICIDE_METHOD:
            if self.answer_is_yes(q, v):
                r.decide("Suicidal thoughts without denying might ever act. "
                         "Setting suicidality to "
                         "SUICIDE_INTENT_SUICIDAL_PLANS.")
                r.suicidality = SUICIDE_INTENT_SUICIDAL_PLANS

        elif q == CQ.DOCTOR:
            if v == V_DOCTOR_YES:
                r.decide("Has spoken to doctor about suicidality. Skipping "
                         "exhortation to do so.")
                jump_to(CQ.DEPR_OUTRO)

        elif q == CQ.DOCTOR2_PLEASE_TALK_TO:
            pass

        elif q == CQ.DEPR_OUTRO:
            pass

        # --------------------------------------------------------------------
        # Worry/anxiety
        # --------------------------------------------------------------------

        elif q == CQ.WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH:
            if v >= V_NSO_SOMETIMES:
                r.decide("Worrying excessively 'sometimes' or 'often'. "
                         "Exploring further.")
                jump_to(CQ.WORRY_CONT1)

        elif q == CQ.WORRY_MAND2_ANY_WORRIES_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("No worries at all in the past month. Moving on.")
                jump_to(CQ.ANX_MAND1_ANXIETY_PAST_MONTH)

        elif q == CQ.WORRY_CONT1:
            pass

        elif q == CQ.WORRY1_INFO_ONLY:
            pass

        elif q == CQ.WORRY2_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("Worry [other than re physical health] on 0 days in "
                         "past week. Moving on.")
                jump_to(CQ.ANX_MAND1_ANXIETY_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Worry [other than re physical health] on >=4 days in "
                         "past week. Incrementing worry.")
                r.worry += 1

        elif q == CQ.WORRY3_TOO_MUCH:
            if self.answer_is_yes(q, v):
                r.decide("Worrying too much. Incrementing worry.")
                r.worry += 1

        elif q == CQ.WORRY4_HOW_UNPLEASANT:
            if v >= V_HOW_UNPLEASANT_UNPLEASANT:
                r.decide("Worry [other than re physical health] 'unpleasant' "
                         "or worse in past week. Incrementing worry.")
                r.worry += 1

        elif q == CQ.WORRY5_GT_3H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Worry [other than re physical health] for >3h on any "
                         "day in past week. Incrementing worry.")
                r.worry += 1

        elif q == CQ.WORRY_DUR:
            pass

        elif q == CQ.ANX_MAND1_ANXIETY_PAST_MONTH:
            if self.answer_is_yes(q, v):
                r.decide("Anxious/nervous in past month. "
                         "Skipping tension question.")
                jump_to(CQ.ANX_PHOBIA1_SPECIFIC_PAST_MONTH)

        elif q == CQ.ANX_MAND2_TENSION_PAST_MONTH:
            if v == V_NSO_NO:
                r.decide("No tension in past month (and no anxiety, from "
                         "previous question). Moving on.")
                jump_to(CQ.PHOBIAS_MAND_AVOIDANCE_PAST_MONTH)

        elif q == CQ.ANX_PHOBIA1_SPECIFIC_PAST_MONTH:
            if self.answer_is_no(q, v):
                r.decide("No phobias. Moving on to general anxiety.")
                jump_to(CQ.ANX2_GENERAL_DAYS_PAST_WEEK)
            elif self.answer_is_yes(q, v):
                # This was in ANX_PHOBIA2; PHOBIAS_FLAG was set by arriving
                # there (but that only happens when we get a 'yes' answer here).
                r.decide("Phobias. Exploring further. Setting phobias flag.")
                r.phobias_flag = True

        elif q == CQ.ANX_PHOBIA2_SPECIFIC_OR_GENERAL:
            if v == V_ANX_PHOBIA2_ALWAYS_SPECIFIC:
                r.decide("Anxiety always specific. "
                         "Skipping generalized anxiety.")
                jump_to(CQ.PHOBIAS_TYPE1)

        elif q == CQ.ANX1_INFO_ONLY:
            pass

        elif q == CQ.ANX2_GENERAL_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                if r.phobias_flag:
                    r.decide("No generalized anxiety in past week. "
                             "Skipping further generalized anxiety questions.")
                    jump_to(CQ.PHOBIAS1_DAYS_PAST_WEEK)
                else:
                    r.decide("No generalized anxiety in past week. Moving on.")
                    jump_to(CQ.COMP_MAND1_COMPULSIONS_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Generalized anxiety on >=4 days in past week. "
                         "Incrementing anxiety.")
                r.anxiety += 1

        elif q == CQ.ANX3_GENERAL_HOW_UNPLEASANT:
            if v >= V_HOW_UNPLEASANT_UNPLEASANT:
                r.decide("Anxiety 'unpleasant' or worse in past week. "
                         "Incrementing anxiety.")
                r.anxiety += 1

        elif q == CQ.ANX4_GENERAL_PHYSICAL_SYMPTOMS:
            if self.answer_is_yes(q, v):
                r.decide("Physical symptoms of anxiety. "
                         "Setting anxiety_physical_symptoms. "
                         "Incrementing anxiety.")
                r.anxiety_physical_symptoms = True
                r.anxiety += 1

        elif q == CQ.ANX5_GENERAL_GT_3H_ANY_DAY:
            if self.answer_is_yes(q, v):
                r.decide("Anxiety for >3h on any day in past week. "
                         "Incrementing anxiety.")
                r.anxiety += 1

        elif q == CQ.ANX_DUR_GENERAL:
            if v >= V_DURATION_2W_6M:
                r.decide("Anxiety for >=2 weeks. "
                         "Setting anxiety_at_least_2_weeks.")
                r.anxiety_at_least_2_weeks = True
            if r.phobias_flag:
                r.decide("Phobias flag set. Exploring further.")
                jump_to(CQ.PHOBIAS_TYPE1)
            else:
                if r.anxiety <= 1:
                    r.decide("Anxiety score <=1. Moving on to compulsions.")
                    jump_to(CQ.COMP_MAND1_COMPULSIONS_PAST_MONTH)
                else:
                    r.decide("Anxiety score >=2. Exploring panic.")
                    jump_to(CQ.PANIC_MAND_PAST_MONTH)

        elif q == CQ.PHOBIAS_MAND_AVOIDANCE_PAST_MONTH:
            if self.answer_is_no(q, v):
                if r.anxiety <= 1:
                    r.decide("Anxiety score <=1. Moving on to compulsions.")
                    jump_to(CQ.COMP_MAND1_COMPULSIONS_PAST_MONTH)
                else:
                    r.decide("Anxiety score >=2. Exploring panic.")
                    jump_to(CQ.PANIC_MAND_PAST_MONTH)

        elif q == CQ.PHOBIAS_TYPE1:
            if v in [V_PHOBIAS_TYPE1_ALONE_PUBLIC_TRANSPORT,
                     V_PHOBIAS_TYPE1_FAR_FROM_HOME,
                     V_PHOBIAS_TYPE1_CROWDED_SHOPS]:
                r.decide("Phobia type category: agoraphobia.")
                r.phobias_type = PHOBIATYPES_AGORAPHOBIA

            elif v in [V_PHOBIAS_TYPE1_PUBLIC_SPEAKING_EATING,
                       V_PHOBIAS_TYPE1_BEING_WATCHED]:
                r.decide("Phobia type category: social.")
                r.phobias_type = PHOBIATYPES_SOCIAL

            elif v == V_PHOBIAS_TYPE1_BLOOD:
                r.decide("Phobia type category: blood/injury.")
                r.phobias_type = PHOBIATYPES_BLOOD_INJURY

            elif v in [V_PHOBIAS_TYPE1_ANIMALS,
                       V_PHOBIAS_TYPE1_ENCLOSED_SPACES_HEIGHTS]:
                r.decide("Phobia type category: animals/enclosed spaces/"
                         "heights.")
                r.phobias_type = PHOBIATYPES_ANIMALS_ENCLOSED_HEIGHTS

            elif v == V_PHOBIAS_TYPE1_OTHER:
                r.decide("Phobia type category: other.")
                r.phobias_type = PHOBIATYPES_OTHER

            else:
                pass

        elif q == CQ.PHOBIAS1_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Phobic anxiety on >=4 days in past week. "
                         "Incrementing phobias_score.")
                r.phobias_score += 1

        elif q == CQ.PHOBIAS2_PHYSICAL_SYMPTOMS:
            if self.answer_is_yes(q, v):
                r.decide("Physical symptoms during phobic anxiety in past "
                         "week. Incrementing phobias_score.")
                r.phobias_score += 1

        elif q == CQ.PHOBIAS3_AVOIDANCE:
            if self.answer_is_no(q, v):  # no avoidance in past week
                if r.anxiety <= 1 and r.phobias_score == 0:
                    r.decide("No avoidance in past week; "
                             "anxiety <= 1 and phobias_score == 0. "
                             "Finishing anxiety section.")
                    jump_to(CQ.ANX_OUTRO)
                else:
                    r.decide("No avoidance in past week; "
                             "anxiety >= 2 or phobias_score >= 1. "
                             "Moving to panic section.")
                    jump_to(CQ.PANIC_MAND_PAST_MONTH)
            elif self.answer_is_yes(q, v):
                r.decide("Setting phobic_avoidance.")
                r.phobic_avoidance = True

        elif q == CQ.PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_1_TO_3:
                r.decide("Phobic avoidance on 1-3 days in past week. "
                         "Incrementing phobias_score.")
                r.phobias_score += 1
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Phobic avoidance on >=4 days in past week. "
                         "Adding 2 to phobias_score.")
                r.phobias_score += 2
            if (r.anxiety <= 1 and
                    self.int_value_for_question(CQ.PHOBIAS1_DAYS_PAST_WEEK) ==
                    V_DAYS_IN_PAST_WEEK_0):
                r.decide("anxiety <= 1 and no phobic anxiety in past week. "
                         "Finishing anxiety section.")
                jump_to(CQ.ANX_OUTRO)

        elif q == CQ.PHOBIAS_DUR:
            pass

        elif q == CQ.PANIC_MAND_PAST_MONTH:
            if v == V_NSO_NO:
                r.decide("No panic in the past month. Finishing anxiety "
                         "section.")
                jump_to(CQ.ANX_OUTRO)

        elif q == CQ.PANIC1_NUM_PAST_WEEK:
            if v == V_PANIC1_N_PANICS_PAST_WEEK_0:
                r.decide("No panic in past week. Finishing anxiety section.")
                jump_to(CQ.ANX_OUTRO)
            elif v == V_PANIC1_N_PANICS_PAST_WEEK_1:
                r.decide("One panic in past week. Incrementing panic.")
                r.panic += 1
            elif v == V_PANIC1_N_PANICS_PAST_WEEK_GT_1:
                r.decide("More than one panic in past week. Adding 2 to panic.")
                r.panic += 2

        elif q == CQ.PANIC2_HOW_UNPLEASANT:
            if v >= V_HOW_UNPLEASANT_UNPLEASANT:
                r.decide("Panic 'unpleasant' or worse in past week. "
                         "Incrementing panic.")
                r.panic += 1

        elif q == CQ.PANIC3_PANIC_GE_10_MIN:
            if v == V_PANIC3_WORST_GE_10_MIN:
                r.decide("Worst panic in past week lasted >=10 min. "
                         "Incrementing panic.")
                r.panic += 1

        elif q == CQ.PANIC4_RAPID_ONSET:
            if self.answer_is_yes(q, v):
                r.decide("Rapid onset of panic symptoms. "
                         "Setting panic_rapid_onset.")
                r.panic_rapid_onset = True

        elif q == CQ.PANSYM:
            # Multi-way answer. All are scored 1=no, 2=yes.
            n_panic_symptoms = 0
            for panic_fn in PANIC_SYMPTOM_FIELDNAMES:
                panic_symptom = getattr(self, panic_fn) or 0  # force to int
                yes_present = panic_symptom == 2
                if yes_present:
                    n_panic_symptoms += 1
            r.decide(
                f"{n_panic_symptoms} out of "
                f"{NUM_PANIC_SYMPTOMS} specific panic symptoms endorsed.")
            # The next bit was coded in PANIC5, but lives more naturally here:
            if self.answer_is_no(CQ.ANX_PHOBIA1_SPECIFIC_PAST_MONTH):
                jump_to(CQ.PANIC_DUR)

        elif q == CQ.PANIC5_ALWAYS_SPECIFIC_TRIGGER:
            pass

        elif q == CQ.PANIC_DUR:
            pass

        elif q == CQ.ANX_OUTRO:
            pass

        # --------------------------------------------------------------------
        # Compulsions and obsessions
        # --------------------------------------------------------------------

        elif q == CQ.COMP_MAND1_COMPULSIONS_PAST_MONTH:
            if v == V_NSO_NO:
                r.decide("No compulsions in past month. Moving to obsessions.")
                jump_to(CQ.OBSESS_MAND1_OBSESSIONS_PAST_MONTH)

        elif q == CQ.COMP1_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("No compulsions in past week. Moving to obesssions.")
                jump_to(CQ.OBSESS_MAND1_OBSESSIONS_PAST_MONTH)
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Obsessions on >=4 days in past week. "
                         "Incrementing compulsions.")
                r.compulsions += 1

        elif q == CQ.COMP2_TRIED_TO_STOP:
            if self.answer_is_yes(q, v):
                r.decide("Attempts to stop compulsions in past week. "
                         "Setting compulsions_tried_to_stop. "
                         "Incrementing compulsions.")
                r.compulsions_tried_to_stop = True
                r.compulsions += 1

        elif q == CQ.COMP3_UPSETTING:
            if self.answer_is_yes(q, v):
                r.decide("Compulsions upsetting/annoying. "
                         "Incrementing compulsions.")
                r.compulsions += 1

        elif q == CQ.COMP4_MAX_N_REPETITIONS:
            if v == V_COMP4_MAX_N_REPEATS_GE_3:
                r.decide("At worst, >=3 repeats. Incrementing compulsions.")
                r.compulsions += 1

        elif q == CQ.COMP_DUR:
            if v >= V_DURATION_2W_6M:
                r.decide("Compulsions for >=2 weeks. "
                         "Setting compulsions_at_least_2_weeks.")
                r.compulsions_at_least_2_weeks = True

        elif q == CQ.OBSESS_MAND1_OBSESSIONS_PAST_MONTH:
            if v == V_NSO_NO:
                r.decide("No obsessions in past month. Moving on.")
                jump_to(r.get_final_page())

        elif q == CQ.OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL:
            if v == V_OBSESS_MAND1_GENERAL_WORRIES:
                r.decide("Worrying about something in general, not the same "
                         "thoughts over and over again. Moving on.")
                jump_to(r.get_final_page())

        elif q == CQ.OBSESS1_DAYS_PAST_WEEK:
            if v == V_DAYS_IN_PAST_WEEK_0:
                r.decide("No obsessions in past week. Moving on.")
                jump_to(r.get_final_page())
            elif v == V_DAYS_IN_PAST_WEEK_4_OR_MORE:
                r.decide("Obsessions on >=4 days in past week. "
                         "Incrementing obsessions.")
                r.obsessions += 1

        elif q == CQ.OBSESS2_TRIED_TO_STOP:
            if self.answer_is_yes(q, v):
                r.decide("Tried to stop obsessional thoughts in past week. "
                         "Setting obsessions_tried_to_stop. "
                         "Incrementing obsessions.")
                r.obsessions_tried_to_stop = True
                r.obsessions += 1

        elif q == CQ.OBSESS3_UPSETTING:
            if self.answer_is_yes(q, v):
                r.decide("Obsessions upsetting/annoying in past week. "
                         "Incrementing obsessions.")
                r.obsessions += 1

        elif q == CQ.OBSESS4_MAX_DURATION:
            if v == V_OBSESS4_GE_15_MIN:
                r.decide("Obsessions lasting >=15 min in past week. "
                         "Incrementing obsessions.")
                r.obsessions += 1

        elif q == CQ.OBSESS_DUR:
            if v >= V_DURATION_2W_6M:
                r.decide("Obsessions for >=2 weeks. "
                         "Setting obsessions_at_least_2_weeks.")
                r.obsessions_at_least_2_weeks = True

        # --------------------------------------------------------------------
        # End
        # --------------------------------------------------------------------

        elif q == CQ.OVERALL1_INFO_ONLY:
            pass

        elif q == CQ.OVERALL2_IMPACT_PAST_WEEK:
            if self.answered(q, v):
                r.functional_impairment = v - 1
                r.decide(
                    f"Setting functional_impairment to "
                    f"{r.functional_impairment}")

        elif q == CQ.THANKS_FINISHED:
            pass

        elif q == CQ.END_MARKER:  # this is not a page
            # we've reached the end; no point thinking further
            return CQ.END_MARKER

        else:
            pass

        if next_q == -1:
            # Nothing has expressed an overriding preference, so increment...
            next_q = enum_to_int(q) + 1

        return int_to_enum(next_q)

    def get_result(self, record_decisions: bool = False) -> CisrResult:
        # internal_q = CQ.START_MARKER
        internal_q = CQ.APPETITE1_LOSS_PAST_MONTH  # skip the preamble etc.
        result = CisrResult(record_decisions)
        while (not result.incomplete) and internal_q != CQ.END_MARKER:
            internal_q = self.next_q(internal_q, result)
            # loop until we reach the end or have incomplete data
        result.finalize()
        return result

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        result = self.get_result()
        if result.incomplete:
            return CTV_INCOMPLETE
        return [
            CtvInfo(content=(
                f"Probable primary diagnosis: "
                f"{bold(result.diagnosis_1_name())} "
                f"({result.diagnosis_1_icd10_code()})")),
            CtvInfo(content=(
                f"Probable secondary diagnosis: "
                f"{bold(result.diagnosis_2_name())} "
                f"({result.diagnosis_2_icd10_code()})")),
            CtvInfo(content=(
                f"CIS-R suicide intent: "
                f"{self.get_suicide_intent(req, result, with_warning=False)}")),
        ]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        result = self.get_result()
        return self.standard_task_summary_fields() + [

            # Diagnoses

            SummaryElement(
                name="diagnosis_1_code",
                coltype=Integer(),
                value=result.diagnosis_1,
                comment="Probable primary diagnosis (CIS-R code)"),
            SummaryElement(
                name="diagnosis_1_text",
                coltype=UnicodeText(),
                value=result.diagnosis_1_name(),
                comment="Probable primary diagnosis (text)"),
            SummaryElement(
                name="diagnosis_1_icd10",
                coltype=UnicodeText(),
                value=result.diagnosis_1_icd10_code(),
                comment="Probable primary diagnosis (ICD-10 code/codes)"),
            SummaryElement(
                name="diagnosis_2_code",
                coltype=Integer(),
                value=result.diagnosis_2,
                comment="Probable secondary diagnosis (CIS-R code)"),
            SummaryElement(
                name="diagnosis_2_text",
                coltype=UnicodeText(),
                value=result.diagnosis_2_icd10_code(),
                comment="Probable secondary diagnosis (text)"),
            SummaryElement(
                name="diagnosis_2_icd10",
                coltype=UnicodeText(),
                value=result.diagnosis_2_icd10_code(),
                comment="Probable secondary diagnosis (ICD-10 code/codes)"),

            # Suicidality/doctell: directly encoded in data

            # Total score

            SummaryElement(
                name="score_total",
                coltype=Integer(),
                value=result.get_score(),
                comment=f"CIS-R total score (max. {MAX_TOTAL})"),
            # Functional impairment: directly encoded in data

            # Subscores

            SummaryElement(
                name="score_somatic_symptoms",
                coltype=Integer(),
                value=result.somatic_symptoms,
                comment="Score: somatic symptoms (max. 4)"),
            SummaryElement(
                name="score_hypochondria",
                coltype=Integer(),
                value=result.hypochondria,
                comment="Score: worry over physical health (max. 4)"),
            SummaryElement(
                name="score_irritability",
                coltype=Integer(),
                value=result.irritability,
                comment="Score: irritability (max. 4)"),
            SummaryElement(
                name="score_concentration_poor",
                coltype=Integer(),
                value=result.concentration_poor,
                comment="Score: poor concentration (max. 4)"),
            SummaryElement(
                name="score_fatigue",
                coltype=Integer(),
                value=result.fatigue,
                comment="Score: fatigue (max. 4)"),
            SummaryElement(
                name="score_sleep_problems",
                coltype=Integer(),
                value=result.sleep_problems,
                comment="Score: sleep problems (max. 4)"),
            SummaryElement(
                name="score_depression",
                coltype=Integer(),
                value=result.depression,
                comment="Score: depression (max. 4)"),
            SummaryElement(
                name="score_depressive_thoughts",
                coltype=Integer(),
                value=result.depressive_thoughts,
                comment="Score: depressive ideas (max. 5)"),
            SummaryElement(
                name="score_phobias",
                coltype=Integer(),
                value=result.phobias_score,
                comment="Score: phobias (max. 4)"),
            SummaryElement(
                name="score_worry",
                coltype=Integer(),
                value=result.worry,
                comment="Score: worry (max. 4)"),
            SummaryElement(
                name="score_anxiety",
                coltype=Integer(),
                value=result.anxiety,
                comment="Score: anxiety (max. 4)"),
            SummaryElement(
                name="score_panic",
                coltype=Integer(),
                value=result.panic,
                comment="Score: panic (max. 4)"),
            SummaryElement(
                name="score_compulsions",
                coltype=Integer(),
                value=result.compulsions,
                comment="Score: compulsions (max. 4)"),
            SummaryElement(
                name="score_obsessions",
                coltype=Integer(),
                value=result.obsessions,
                comment="Score: obsessions (max. 4)"),

            # Other

            SummaryElement(
                name="sleep_change",
                coltype=Integer(),
                value=result.sleep_change,
                comment=DESC_SLEEP_CHANGE),
            SummaryElement(
                name="weight_change",
                coltype=Integer(),
                value=result.weight_change,
                comment=DESC_WEIGHT_CHANGE),
            SummaryElement(
                name="depcrit1_score",
                coltype=Integer(),
                value=result.depr_crit_1_mood_anhedonia_energy,
                comment=DESC_DEPCRIT1),
            SummaryElement(
                name="depcrit2_score",
                coltype=Integer(),
                value=result.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui,
                comment=DESC_DEPCRIT2),
            SummaryElement(
                name="depcrit3_score",
                coltype=Integer(),
                value=result.depr_crit_3_somatic_synd,
                comment=DESC_DEPCRIT3),
            SummaryElement(
                name="depcrit3_met_somatic_syndrome",
                coltype=Boolean(),
                value=result.has_somatic_syndrome(),
                comment=DESC_DEPCRIT3_MET),
            SummaryElement(
                name="neurasthenia_score",
                coltype=Integer(),
                value=result.neurasthenia,
                comment=DESC_NEURASTHENIA_SCORE),

            # Disorder flags

            SummaryElement(
                name="disorder_ocd",
                coltype=Boolean(),
                value=result.obsessive_compulsive_disorder,
                comment=DISORDER_OCD),
            SummaryElement(
                name="disorder_depression_mild",
                coltype=Boolean(),
                value=result.depression_mild,
                comment=DISORDER_DEPR_MILD),
            SummaryElement(
                name="disorder_depression_moderate",
                coltype=Boolean(),
                value=result.depression_moderate,
                comment=DISORDER_DEPR_MOD),
            SummaryElement(
                name="disorder_depression_severe",
                coltype=Boolean(),
                value=result.depression_severe,
                comment=DISORDER_DEPR_SEV),
            SummaryElement(
                name="disorder_cfs",
                coltype=Boolean(),
                value=result.chronic_fatigue_syndrome,
                comment=DISORDER_CFS),
            SummaryElement(
                name="disorder_gad",
                coltype=Boolean(),
                value=result.generalized_anxiety_disorder,
                comment=DISORDER_GAD),
            SummaryElement(
                name="disorder_agoraphobia",
                coltype=Boolean(),
                value=result.phobia_agoraphobia,
                comment=DISORDER_AGORAPHOBIA),
            SummaryElement(
                name="disorder_social_phobia",
                coltype=Boolean(),
                value=result.phobia_social,
                comment=DISORDER_SOCIAL_PHOBIA),
            SummaryElement(
                name="disorder_specific_phobia",
                coltype=Boolean(),
                value=result.phobia_specific,
                comment=DISORDER_SPECIFIC_PHOBIA),
            SummaryElement(
                name="disorder_panic_disorder",
                coltype=Boolean(),
                value=result.panic_disorder,
                comment=DISORDER_PANIC),
        ]

    def is_complete(self) -> bool:
        result = self.get_result()
        return not result.incomplete

    def diagnosis_name(self, req: CamcopsRequest, diagnosis_code: int) -> str:
        xstring_name = f"diag_{diagnosis_code}_desc"
        return self.wxstring(req, xstring_name)

    def diagnosis_reason(self, req: CamcopsRequest,
                         diagnosis_code: int) -> str:
        xstring_name = f"diag_{diagnosis_code}_explan"
        return self.wxstring(req, xstring_name)

    def get_suicide_intent(self, req: CamcopsRequest,
                           result: CisrResult,
                           with_warning: bool = True) -> str:
        if result.incomplete:
            html = "TASK INCOMPLETE. SO FAR: "
        else:
            html = ""
        html += self.wxstring(req, f"suicid_{result.suicidality}")
        if (with_warning and
                result.suicidality >= SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING):
            html += f" <i>{self.wxstring(req, 'suicid_instruction')}</i>"
        if result.suicidality != SUICIDE_INTENT_NONE:
            html = bold(html)
        return html

    def get_doctell(self, req: CamcopsRequest) -> str:
        if self.doctor is None:
            return ""
        return self.xstring(req, f"doctell_{self.doctor}")
        # ... xstring() as may use HTML

    def get_sleep_change(self, req: CamcopsRequest, result: CisrResult) -> str:
        if result.sleep_change == SLEEPCHANGE_NONE:
            return ""
        return self.wxstring(req, f"sleepch_{result.sleep_change}")

    def get_weight_change(self, req: CamcopsRequest, result: CisrResult) -> str:
        if result.weight_change in [WTCHANGE_NONE_OR_APPETITE_INCREASE,
                                    WTCHANGE_APPETITE_LOSS]:
            return ""
        return self.wxstring(req, f"wtchange_{result.weight_change}")

    def get_impairment(self, req: CamcopsRequest, result: CisrResult) -> str:
        return self.wxstring(
            req, f"impair_{result.functional_impairment}")

    def get_task_html(self, req: CamcopsRequest) -> str:
        # Iterate only once, for efficiency, so don't use get_result().

        def qa_row(q_: CisrQuestion, qtext: str,
                   a_: Optional[str]) -> str:
            return tr(f"{q_.value}. {qtext}", answer(a_))

        def max_text(maxval: int) -> str:
            return f" (max. {maxval})"

        demographics_html_list = []  # type: List[str]
        question_html_list = []  # type: List[str]
        q = CQ.ETHNIC  # type: CisrQuestion
        result = CisrResult(record_decisions=True)
        while (not result.incomplete) and q != CQ.END_MARKER:
            # Iterate until we get to the end or the result declares itself
            # incomplete.
            # noinspection PyTypeChecker
            target_list = (
                demographics_html_list if q.value < CQ.HEALTH_WELLBEING.value
                else question_html_list)
            if q in QUESTIONS_PROMPT_ONLY:
                question = self.wxstring(req, QUESTIONS_PROMPT_ONLY[q])
                target_list.append(
                    qa_row(q, question, NOT_APPLICABLE_TEXT))
            elif q == CQ.PANSYM:  # special!
                target_list.append(qa_row(
                    q,
                    self.wxstring(req, "pansym_q_prefix"),
                    NOT_APPLICABLE_TEXT))
                for fieldname in PANIC_SYMPTOM_FIELDNAMES:
                    question = self.wxstring(req, fieldname + "_q")
                    value = getattr(self, fieldname)
                    a = get_yes_no_none(
                        req, value == 2 if value is not None else None)
                    target_list.append(qa_row(q, question, a))
            else:
                fieldname = fieldname_for_q(q)
                assert fieldname, f"No fieldname for question {q}"
                question = self.wxstring(req, fieldname + "_q")
                a = self.get_textual_answer(req, q)
                target_list.append(qa_row(q, question, a))

            q = self.next_q(q, result)
            # loop until we reach the end or have incomplete data
        result.finalize()

        is_complete = not result.incomplete
        is_complete_html_td = """{}<b>{}</b></td>""".format(
            "<td>" if is_complete
            else f"""<td class="{CssClass.INCOMPLETE}">""",
            get_yes_no(req, is_complete)
        )

        summary_rows = [

            subheading_spanning_two_columns("Diagnoses"),

            tr(
                "Probable primary diagnosis",
                (
                    bold(self.diagnosis_name(req, result.diagnosis_1)) +
                    (f" ({result.diagnosis_1_icd10_code()})"
                     if result.has_diagnosis_1() else "")
                )
            ),
            tr(italic("... summary of reasons/description"),
               italic(self.diagnosis_reason(req, result.diagnosis_1))),
            tr(
                "Probable secondary diagnosis",
                (
                    bold(self.diagnosis_name(req, result.diagnosis_2)) +
                    (f" ({result.diagnosis_2_icd10_code()})"
                     if result.has_diagnosis_2() else "")
                )
            ),
            tr(italic("... summary of reasons/description"),
               italic(self.diagnosis_reason(req, result.diagnosis_2))),

            subheading_spanning_two_columns("Suicidality"),

            tr(td(self.wxstring(req, "suicid_heading")),
               td(self.get_suicide_intent(req, result)),
               literal=True),
            tr("... spoken to doctor?",
               self.get_doctell(req)),

            subheading_spanning_two_columns("Total score/overall impairment"),

            tr(
                f"CIS-R total score (max. {MAX_TOTAL}) <sup>[1]</sup>",
                result.get_score()
            ),
            tr(self.wxstring(req, "impair_label"),
               self.get_impairment(req, result)),

            subheading_spanning_two_columns("Subscores contributing to total "
                                            "<sup>[2]</sup>"),

            tr(self.wxstring(req, "somatic_label") + max_text(MAX_SOMATIC),
               result.somatic_symptoms),
            tr(self.wxstring(req, "hypo_label") + max_text(MAX_HYPO),
               result.hypochondria),
            tr(self.wxstring(req, "irrit_label") + max_text(MAX_IRRIT),
               result.irritability),
            tr(self.wxstring(req, "conc_label") + max_text(MAX_CONC),
               result.concentration_poor),
            tr(self.wxstring(req, "fatigue_label") + max_text(MAX_FATIGUE),
               result.fatigue),
            tr(self.wxstring(req, "sleep_label") + max_text(MAX_SLEEP),
               result.sleep_problems),
            tr(self.wxstring(req, "depr_label") + max_text(MAX_DEPR),
               result.depression),
            tr(self.wxstring(req, "depthts_label") + max_text(MAX_DEPTHTS),
               result.depressive_thoughts),
            tr(self.wxstring(req, "phobias_label") + max_text(MAX_PHOBIAS),
               result.phobias_score),
            tr(self.wxstring(req, "worry_label") + max_text(MAX_WORRY),
               result.worry),
            tr(self.wxstring(req, "anx_label") + max_text(MAX_ANX),
               result.anxiety),
            tr(self.wxstring(req, "panic_label") + max_text(MAX_PANIC),
               result.panic),
            tr(self.wxstring(req, "comp_label") + max_text(MAX_COMP),
               result.compulsions),
            tr(self.wxstring(req, "obsess_label") + max_text(MAX_OBSESS),
               result.obsessions),

            subheading_spanning_two_columns("Other"),

            tr("Sleep change", self.get_sleep_change(req, result)),
            tr("Weight change", self.get_weight_change(req, result)),
            tr(DESC_DEPCRIT1, result.depr_crit_1_mood_anhedonia_energy),
            tr(DESC_DEPCRIT2, result.depr_crit_2_app_cnc_slp_mtr_glt_wth_sui),
            tr(DESC_DEPCRIT3, result.depr_crit_3_somatic_synd),
            tr(DESC_DEPCRIT3_MET, result.has_somatic_syndrome()),  # RNC
            tr(DESC_NEURASTHENIA_SCORE, result.neurasthenia),

            subheading_spanning_two_columns("Disorder flags"),

            tr(DISORDER_OCD, result.obsessive_compulsive_disorder),
            tr(DISORDER_DEPR_MILD, result.depression_mild),
            tr(DISORDER_DEPR_MOD, result.depression_moderate),
            tr(DISORDER_DEPR_SEV, result.depression_severe),
            tr(DISORDER_CFS, result.chronic_fatigue_syndrome),
            tr(DISORDER_GAD, result.generalized_anxiety_disorder),
            tr(DISORDER_AGORAPHOBIA, result.phobia_agoraphobia),
            tr(DISORDER_SOCIAL_PHOBIA, result.phobia_social),
            tr(DISORDER_SPECIFIC_PHOBIA, result.phobia_specific),
            tr(DISORDER_PANIC, result.panic_disorder),
        ]

        return f"""
            <div class="{CssClass.HEADING}">{self.wxstring(req, "results_1")}</div>
            <div>{self.wxstring(req, "results_2")}</div>
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    <tr>
                        <td width="50%">Completed?</td>
                        {is_complete_html_td}
                    </tr>
                    {"".join(summary_rows)}
                </table>
            </div>

            <div class="{CssClass.FOOTNOTES}">
                [1] {self.wxstring(req, "score_note")}
                [2] {self.wxstring(req, "symptom_score_note")}
            </div>

            <div class="{CssClass.HEADING}">
                Preamble/demographics (not contributing to diagnosis)
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="75%">Page</th>
                    <th width="25%">Answer</td>
                </tr>
                {"".join(demographics_html_list)}
            </table>

            <div class="{CssClass.HEADING}">
                Data considered by algorithm (may be a subset of all data if
                subject revised answers)
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="75%">Page</th>
                    <th width="25%">Answer</td>
                </tr>
                {"".join(question_html_list)}
            </table>

            <div class="{CssClass.HEADING}">Decisions</div>
            <pre>{"<br>".join(ws.webify("‣ " + x) for x in result.decisions)}</pre>

            <div class="{CssClass.COPYRIGHT}">
                • Original papers:

                ▶ Lewis G, Pelosi AJ, Aray R, Dunn G (1992).
                Measuring psychiatric disorder in the community: a standardized
                assessment for use by lay interviewers.
                Psychological Medicine 22: 465-486. PubMed ID 
                <a href="https://www.ncbi.nlm.nih.gov/pubmed/1615114">1615114</a>.

                ▶ Lewis G (1994).
                Assessing psychiatric disorder with a human interviewer or a
                computer.
                J Epidemiol Community Health 48: 207-210. PubMed ID 
                <a href="https://www.ncbi.nlm.nih.gov/pubmed/8189180">8189180</a>.

                • Source/copyright: Glyn Lewis.

                ▶ The task itself is not in the reference publications, so
                copyright presumed to rest with the authors (not the journals).

                ▶ “There are no copyright issues with the CISR so please adapt
                it for use.” — Prof. Glyn Lewis, personal communication to
                Rudolf Cardinal, 27 Oct 2017.
            </div>
        """  # noqa
