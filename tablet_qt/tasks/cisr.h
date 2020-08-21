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

/*

Should we ask about ethnicity, marital/employment/housing status?

    Note: demographics do not contribute in any way to the actual
    symptom-gathering or diagnostic approach. Some (e.g. age, sex) are already
    collected by CamCOPS.

    REASON TO INCLUDE THEM: the CIS-R is used in national morbidity surveys
    (e.g. PMID 22429544) and we want to keep compatibility.

    However, age is handled by CamCOPS (with greater precision than the CIS-R)
    as part of its patient details, and likewise sex.

    It's also helpful to add a "prefer not to say" option, for diagnosis-only
    contexts.
*/

#include <QPointer>
#include <QString>
#include "common/aliases_camcops.h"
#include "tasklib/task.h"

class CamcopsApp;
class DynamicQuestionnaire;
class OpenableWidget;
class TaskFactory;

void initializeCisr(TaskFactory& factory);


namespace cisrconst {

// Internal coding, NOT answer values:

// Magic numbers from the original:
const int WTCHANGE_NONE_OR_APPETITE_INCREASE = 0;
const int WTCHANGE_APPETITE_LOSS = 1;
const int WTCHANGE_NONDELIBERATE_WTLOSS_OR_WTGAIN = 2;
const int WTCHANGE_WTLOSS_GE_HALF_STONE = 3;
const int WTCHANGE_WTGAIN_GE_HALF_STONE = 4;
// ... I'm not entirely sure why this labelling system is used!

const int PHOBIATYPES_OTHER = 0;
const int PHOBIATYPES_AGORAPHOBIA = 1;
const int PHOBIATYPES_SOCIAL = 2;
const int PHOBIATYPES_BLOOD_INJURY = 3;
const int PHOBIATYPES_ANIMALS_ENCLOSED_HEIGHTS = 4;
// ... some of these are not really used, but I've followed the original CIS-R
// for clarity

// One smaller than the answer codes:
const int OVERALL_IMPAIRMENT_NONE = 0;
const int OVERALL_IMPAIRMENT_DIFFICULT = 1;
const int OVERALL_IMPAIRMENT_STOP_1_ACTIVITY = 2;
const int OVERALL_IMPAIRMENT_STOP_GT_1_ACTIVITY = 3;

// Again, we're following this coding structure primarily for compatibility:
const int DIAG_0_NO_DIAGNOSIS = 0;
const int DIAG_1_MIXED_ANX_DEPR_DIS_MILD = 1;
const int DIAG_2_GENERALIZED_ANX_DIS_MILD = 2;
const int DIAG_3_OBSESSIVE_COMPULSIVE_DIS = 3;
const int DIAG_4_MIXED_ANX_DEPR_DIS = 4;
const int DIAG_5_SPECIFIC_PHOBIA = 5;
const int DIAG_6_SOCIAL_PHOBIA = 6;
const int DIAG_7_AGORAPHOBIA = 7;
const int DIAG_8_GENERALIZED_ANX_DIS = 8;
const int DIAG_9_PANIC_DIS = 9;
const int DIAG_10_MILD_DEPR_EPISODE = 10;
const int DIAG_11_MOD_DEPR_EPISODE = 11;
const int DIAG_12_SEVERE_DEPR_EPISODE = 12;

const int SUICIDE_INTENT_NONE = 0;
const int SUICIDE_INTENT_HOPELESS_NO_SUICIDAL_THOUGHTS = 1;
const int SUICIDE_INTENT_LIFE_NOT_WORTH_LIVING = 2;
const int SUICIDE_INTENT_SUICIDAL_THOUGHTS = 3;
const int SUICIDE_INTENT_SUICIDAL_PLANS = 4;

const int SLEEPCHANGE_EMW = 1;
const int SLEEPCHANGE_INSOMNIA_NOT_EMW = 2;
const int SLEEPCHANGE_INCREASE = 3;

const int DIURNAL_MOOD_VAR_NONE = 0;
const int DIURNAL_MOOD_VAR_WORSE_MORNING = 1;
const int DIURNAL_MOOD_VAR_WORSE_EVENING = 2;

const int PSYCHOMOTOR_NONE = 0;
const int PSYCHOMOTOR_RETARDATION = 1;
const int PSYCHOMOTOR_AGITATION = 2;

// Answer values or pseudo-values:

const int V_MISSING = 0;  // Integer value of a missing answer
const int V_UNKNOWN = -1;  // Dummy value, never used for answers
}


class Cisr : public Task
{
    Q_OBJECT
public:
    Cisr(CamcopsApp& app, DatabaseManager& db,
         int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    // ------------------------------------------------------------------------
    // Instance overrides
    // ------------------------------------------------------------------------
    virtual bool isComplete() const override;
    virtual QStringList summary() const override;
    virtual QStringList detail() const override;
    virtual OpenableWidget* editor(bool read_only = false) override;
    // ------------------------------------------------------------------------
    // Task-specific calculations
    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    // DynamicQuestionnaire callbacks
    // ------------------------------------------------------------------------
    QuPagePtr makePage(int current_qnum);
    bool morePagesToGo(int current_qnum) const;
public:
    static const QString CISR_TABLENAME;
public:  // needs to be public to make non-class vectors of it
    enum class CisrQuestion {  // The sequence of all possible questions.
        START_MARKER = 1,  // start with 1

        INTRO_1 = START_MARKER,
        INTRO_2,

        INTRO_DEMOGRAPHICS,

        ETHNIC,
        MARRIED,
        EMPSTAT,
        EMPTYPE,
        HOME,

        HEALTH_WELLBEING,

        APPETITE1_LOSS_PAST_MONTH,
        WEIGHT1_LOSS_PAST_MONTH,
        WEIGHT2_TRYING_TO_LOSE,
        WEIGHT3_LOST_LOTS,
        APPETITE2_INCREASE_PAST_MONTH,
        WEIGHT4_INCREASE_PAST_MONTH,
        // WEIGHT4A = WEIGHT4 with pregnancy question; blended
        WEIGHT5_GAINED_LOTS,
        GP_YEAR,
        DISABLE,
        ILLNESS,

        SOMATIC_MAND1_PAIN_PAST_MONTH,
        SOMATIC_PAIN1_PSYCHOL_EXAC,
        SOMATIC_PAIN2_DAYS_PAST_WEEK,
        SOMATIC_PAIN3_GT_3H_ANY_DAY,
        SOMATIC_PAIN4_UNPLEASANT,
        SOMATIC_PAIN5_INTERRUPTED_INTERESTING,
        SOMATIC_MAND2_DISCOMFORT,
        SOMATIC_DIS1_PSYCHOL_EXAC,
        SOMATIC_DIS2_DAYS_PAST_WEEK,
        SOMATIC_DIS3_GT_3H_ANY_DAY,
        SOMATIC_DIS4_UNPLEASANT,
        SOMATIC_DIS5_INTERRUPTED_INTERESTING,
        SOMATIC_DUR,

        FATIGUE_MAND1_TIRED_PAST_MONTH,
        FATIGUE_CAUSE1_TIRED,
        FATIGUE_TIRED1_DAYS_PAST_WEEK,
        FATIGUE_TIRED2_GT_3H_ANY_DAY,
        FATIGUE_TIRED3_HAD_TO_PUSH,
        FATIGUE_TIRED4_DURING_ENJOYABLE,
        FATIGUE_MAND2_LACK_ENERGY_PAST_MONTH,
        FATIGUE_CAUSE2_LACK_ENERGY,
        FATIGUE_ENERGY1_DAYS_PAST_WEEK,
        FATIGUE_ENERGY2_GT_3H_ANY_DAY,
        FATIGUE_ENERGY3_HAD_TO_PUSH,
        FATIGUE_ENERGY4_DURING_ENJOYABLE,
        FATIGUE_DUR,

        CONC_MAND1_POOR_CONC_PAST_MONTH,
        CONC_MAND2_FORGETFUL_PAST_MONTH,
        CONC1_CONC_DAYS_PAST_WEEK,
        CONC2_CONC_FOR_TV_READING_CONVERSATION,
        CONC3_CONC_PREVENTED_ACTIVITIES,
        CONC_DUR,
        CONC4_FORGOTTEN_IMPORTANT,
        FORGET_DUR,

        SLEEP_MAND1_LOSS_PAST_MONTH,
        SLEEP_LOSE1_NIGHTS_PAST_WEEK,
        SLEEP_LOSE2_DIS_WORST_DURATION,  // DIS = delayed initiation of sleep
        SLEEP_LOSE3_NIGHTS_GT_3H_DIS_PAST_WEEK,
        SLEEP_EMW_PAST_WEEK,  // EMW = early-morning waking
        SLEEP_CAUSE,
        SLEEP_MAND2_GAIN_PAST_MONTH,
        SLEEP_GAIN1_NIGHTS_PAST_WEEK,
        SLEEP_GAIN2_EXTRA_ON_LONGEST_NIGHT,
        SLEEP_GAIN3_NIGHTS_GT_3H_EXTRA_PAST_WEEK,
        SLEEP_DUR,

        IRRIT_MAND1_PEOPLE_PAST_MONTH,
        IRRIT_MAND2_THINGS_PAST_MONTH,
        IRRIT1_DAYS_PER_WEEK,
        IRRIT2_GT_1H_ANY_DAY,
        IRRIT3_WANTED_TO_SHOUT,
        IRRIT4_ARGUMENTS,
        IRRIT_DUR,

        HYPO_MAND1_WORRIED_RE_HEALTH_PAST_MONTH,
        HYPO_MAND2_WORRIED_RE_SERIOUS_ILLNESS,
        HYPO1_DAYS_PAST_WEEK,
        HYPO2_WORRY_TOO_MUCH,
        HYPO3_HOW_UNPLEASANT,
        HYPO4_CAN_DISTRACT,
        HYPO_DUR,

        DEPR_MAND1_LOW_MOOD_PAST_MONTH,
        DEPR1_LOW_MOOD_PAST_WEEK,
        DEPR_MAND2_ENJOYMENT_PAST_MONTH,
        DEPR2_ENJOYMENT_PAST_WEEK,
        DEPR3_DAYS_PAST_WEEK,
        DEPR4_GT_3H_ANY_DAY,
        DEPR_CONTENT,
        DEPR5_COULD_CHEER_UP,
        DEPR_DUR,
        DEPTH1_DIURNAL_VARIATION,  // "depth" = depressive thoughts?
        DEPTH2_LIBIDO,
        DEPTH3_RESTLESS,
        DEPTH4_SLOWED,
        DEPTH5_GUILT,
        DEPTH6_WORSE_THAN_OTHERS,
        DEPTH7_HOPELESS,
        DEPTH8_LNWL,  // life not worth living
        DEPTH9_SUICIDE_THOUGHTS,
        DEPTH10_SUICIDE_METHOD,
        DOCTOR,
        DOCTOR2_PLEASE_TALK_TO,
        DEPR_OUTRO,

        WORRY_MAND1_MORE_THAN_NEEDED_PAST_MONTH,
        WORRY_MAND2_ANY_WORRIES_PAST_MONTH,
        WORRY_CONT1,
        WORRY1_INFO_ONLY,
        WORRY2_DAYS_PAST_WEEK,
        WORRY3_TOO_MUCH,
        WORRY4_HOW_UNPLEASANT,
        WORRY5_GT_3H_ANY_DAY,
        WORRY_DUR,

        ANX_MAND1_ANXIETY_PAST_MONTH,
        ANX_MAND2_TENSION_PAST_MONTH,
        ANX_PHOBIA1_SPECIFIC_PAST_MONTH,
        ANX_PHOBIA2_SPECIFIC_OR_GENERAL,
        ANX1_INFO_ONLY,
        ANX2_GENERAL_DAYS_PAST_WEEK,
        ANX3_GENERAL_HOW_UNPLEASANT,
        ANX4_GENERAL_PHYSICAL_SYMPTOMS,
        ANX5_GENERAL_GT_3H_ANY_DAY,
        ANX_DUR_GENERAL,

        PHOBIAS_MAND_AVOIDANCE_PAST_MONTH,
        PHOBIAS_TYPE1,
        PHOBIAS1_DAYS_PAST_WEEK,
        PHOBIAS2_PHYSICAL_SYMPTOMS,
        PHOBIAS3_AVOIDANCE,
        PHOBIAS4_AVOIDANCE_DAYS_PAST_WEEK,
        PHOBIAS_DUR,

        PANIC_MAND_PAST_MONTH,
        PANIC1_NUM_PAST_WEEK,
        PANIC2_HOW_UNPLEASANT,
        PANIC3_PANIC_GE_10_MIN,
        PANIC4_RAPID_ONSET,
        PANSYM,  // questions about each of several symptoms
        PANIC5_ALWAYS_SPECIFIC_TRIGGER,
        PANIC_DUR,

        ANX_OUTRO,

        COMP_MAND1_COMPULSIONS_PAST_MONTH,
        COMP1_DAYS_PAST_WEEK,
        COMP2_TRIED_TO_STOP,
        COMP3_UPSETTING,
        COMP4_MAX_N_REPETITIONS,
        COMP_DUR,

        OBSESS_MAND1_OBSESSIONS_PAST_MONTH,
        OBSESS_MAND2_SAME_THOUGHTS_OR_GENERAL,
        OBSESS1_DAYS_PAST_WEEK,
        OBSESS2_TRIED_TO_STOP,
        OBSESS3_UPSETTING,
        OBSESS4_MAX_DURATION,
        OBSESS_DUR,

        OVERALL1_INFO_ONLY,
        OVERALL2_IMPACT_PAST_WEEK,
        THANKS_FINISHED,
        END_MARKER,  // not a real page
    };
protected:
    struct CisrResult {
        // Internal
        bool incomplete = false;  // Missing information? DO NOT use results if so.
        bool record_decisions = true;  // should we keep a record of decisions?
        QStringList decisions;  // Human-readable record of decisions made

        // Overall scoring
        int getScore() const;  // SCORE in original
        bool needsImpairmentQuestion() const;  // code in OVERALL1 in original
        void finalize();
        void decide(const QString& decision);
        QString diagnosisName(int diagnosis_code) const;

        // Symptom scoring
        int depression = 0;  // DEPR in original
        int depr_crit_1_mood_anhedonia_energy = 0;  // DEPCRIT1
        int depr_crit_2_app_cnc_slp_mtr_glt_wth_sui = 0;  // DEPCRIT2
        int depr_crit_3_somatic_synd = 0;  // DEPCRIT3
            // ... looks to me like the ICD-10 criteria for somatic syndrome
            // (e.g. F32.01, F32.11, F33.01, F33.11), with the "do you cheer up
            // when..." question (DEPR5) being the one for "lack of emotional
            // reactions to events or activities that normally produce an
            // emotional response".
        int weight_change = cisrconst::WTCHANGE_NONE_OR_APPETITE_INCREASE;  // WTCHANGE IN original
        int somatic_symptoms = 0;  // SOMATIC in original
        int fatigue = 0;  // FATIGUE in original
        int neurasthenia = 0;  // NEURAS in original
        int concentration_poor = 0;  // CONC in original
        int sleep_problems = 0;  // SLEEP in original
        int sleep_change = 0;  // SLEEPCH in original
        int depressive_thoughts = 0;  // DEPTHTS in original
        int irritability = 0;  // IRRIT in original
        int diurnal_mood_variation = cisrconst::DIURNAL_MOOD_VAR_NONE;  // DVM in original
        bool libido_decreased = false;  // LIBID in original
        int psychomotor_changes = cisrconst::PSYCHOMOTOR_NONE;  // PSYCHMOT in original
        int suicidality = 0;  // SUICID in original
        bool depression_at_least_2_weeks = false;  // DEPR_DUR >= 2 in original

        int hypochondria = 0;  // HYPO in original
        int worry = 0;  // WORRY in original
        int anxiety = 0;  // ANX in original
        bool anxiety_physical_symptoms = false;  // AN4 == 2 in original
        bool anxiety_at_least_2_weeks = false;  // ANX_DUR >= 2 in original
        bool phobias_flag = false;  // PHOBIAS_FLAG in original
        int phobias_score = 0;  // PHOBIAS in original
        int phobias_type = 0;  // PHOBIAS_TYPE in original
        bool phobic_avoidance = false;  // PHOBIAS3 == 2 in original
        int panic = 0;  // PANIC in original
        bool panic_rapid_onset = false;  // PANIC4 == 2 in original
        int panic_symptoms_total = 0;  // PANSYTOT in original

        int compulsions = 0;  // COMP in original
        bool compulsions_tried_to_stop = false;  // COMP2 == 2 in original
        bool compulsions_at_least_2_weeks = false;  // COMP_DUR >= 2 in original
        int obsessions = 0;  // OBSESS in original
        bool obsessions_tried_to_stop = false;  // OBSESS2 == 2 in original
        bool obsessions_at_least_2_weeks = false;  // OBSESS_DUR >= 2 in original

        int functional_impairment = 0;  // IMPAIR in original

        // Disorder flags
        bool obsessive_compulsive_disorder = false;  // OBCOMP in original
        bool depression_mild = false;  // DEPRMILD in original
        bool depression_moderate = false;  // DEPRMOD in original
        bool depression_severe = false;  // DEPRSEV in original
        bool chronic_fatigue_syndrome = false;  // CFS in original
        bool generalized_anxiety_disorder = false;  // GAD in original
        bool phobia_agoraphobia = false;  // PHOBAG in original
        bool phobia_social = false;  // PHOBSOC in original
        bool phobia_specific = false;  // PHOBSPEC in original
        bool panic_disorder = false;  // PANICD in original

        // Final diagnoses
        int diagnosis_1 = cisrconst::DIAG_0_NO_DIAGNOSIS;  // DIAG1 in original
        int diagnosis_2 = cisrconst::DIAG_0_NO_DIAGNOSIS;  // DIAG2 in original
    };
protected:
    QStringList summaryForResult(const CisrResult& result) const;
    CisrQuestion intToEnum(int qi) const;
    int enumToInt(CisrQuestion qe) const;
    CisrQuestion getPageEnum(int qnum_zero_based) const;
    QuPagePtr makePageFromEnum(CisrQuestion q);
    QString fieldnameForQuestion(CisrQuestion q) const;
    QString tagForQuestion(CisrQuestion q) const;
    QVariant valueForQuestion(CisrQuestion q) const;
    int intValueForQuestion(CisrQuestion q) const;
    bool answerIsNo(CisrQuestion q, int value = cisrconst::V_UNKNOWN) const;
    bool answerIsYes(CisrQuestion q, int value = cisrconst::V_UNKNOWN) const;
    bool answered(CisrQuestion q, int value = cisrconst::V_UNKNOWN) const;
    QVector<QString> panicSymptomFieldnames() const;
    CisrQuestion nextQ(CisrQuestion q, CisrResult& getResult) const;
    CisrResult getResult() const;
    QString diagnosisNameLong(int diagnosis_code) const;
    QString diagnosisReason(int diagnosis_code) const;
    QString suicideIntent(const Cisr::CisrResult& result,
                          bool with_warning = true) const;
    QString functionalImpairment(const Cisr::CisrResult& result) const;

protected:
    QPointer<DynamicQuestionnaire> m_questionnaire;
};
