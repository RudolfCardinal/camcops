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

#include "cardinalexpdettrial.h"
#include "lib/datetime.h"
#include "taskxtra/cardinalexpdetcommon.h"
#include "taskxtra/cardinalexpdetrating.h"


// Tablename
const QString CardinalExpDetTrial::TRIAL_TABLENAME("cardinal_expdet_trials");

// Fieldnames
const QString CardinalExpDetTrial::FN_FK_TO_TASK("cardinal_expdet_id");
const QString CardinalExpDetTrial::FN_TRIAL("trial");
const QString FN_BLOCK("block");
const QString FN_GROUP_NUM("group_num");
const QString FN_CUE("cue");
const QString FN_RAW_CUE_NUMBER("raw_cue_number");
const QString FN_TARGET_MODALITY("target_modality");
const QString FN_TARGET_NUMBER("target_number");
const QString FN_TARGET_PRESENT("target_present");
const QString FN_ITI_LENGTH_S("iti_length_s");
const QString FN_PAUSE_GIVEN_BEFORE_TRIAL("pause_given_before_trial");
const QString FN_PAUSE_END_TIME("pause_end_time");
const QString FN_PAUSE_START_TIME("pause_start_time");
const QString FN_TRIAL_START_TIME("trial_start_time");
const QString FN_CUE_START_TIME("cue_start_time");
const QString FN_TARGET_START_TIME("target_start_time");
const QString FN_DETECTION_START_TIME("detection_start_time");
const QString FN_ITI_START_TIME("iti_start_time");
const QString FN_ITI_END_TIME("iti_end_time");
const QString FN_TRIAL_END_TIME("trial_end_time");
const QString FN_RESPONDED("responded");
const QString FN_RESPONSE_TIME("response_time");
const QString FN_RESPONSE_LATENCY_MS("response_latency_ms");
const QString FN_RATING("rating");
const QString FN_CORRECT("correct");
const QString FN_POINTS("points");
const QString FN_CUMULATIVE_POINTS("cumulative_points");


CardinalExpDetTrial::CardinalExpDetTrial(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    DatabaseObject(app, db, TRIAL_TABLENAME)
{
    // Keys
    addField(FN_FK_TO_TASK, QVariant::Int);
    addField(FN_TRIAL, QVariant::Int);
    // Task determines these (via an autogeneration process from the config):
    addField(FN_BLOCK, QVariant::Int);
    addField(FN_GROUP_NUM, QVariant::Int);
    addField(FN_CUE, QVariant::Int);
    addField(FN_RAW_CUE_NUMBER, QVariant::Int); // following counterbalancing
    addField(FN_TARGET_MODALITY, QVariant::Int);
    addField(FN_TARGET_NUMBER, QVariant::Int);
    addField(FN_TARGET_PRESENT, QVariant::Bool);
    addField(FN_ITI_LENGTH_S, QVariant::Double);
    // Task determines these (on the fly):
    addField(FN_PAUSE_GIVEN_BEFORE_TRIAL, QVariant::Bool);
    addField(FN_PAUSE_START_TIME, QVariant::DateTime);
    addField(FN_PAUSE_END_TIME, QVariant::DateTime);
    addField(FN_TRIAL_START_TIME, QVariant::DateTime);
    addField(FN_CUE_START_TIME, QVariant::DateTime);
    addField(FN_TARGET_START_TIME, QVariant::DateTime);
    addField(FN_DETECTION_START_TIME, QVariant::DateTime);
    addField(FN_ITI_START_TIME, QVariant::DateTime);
    addField(FN_ITI_END_TIME, QVariant::DateTime);
    addField(FN_TRIAL_END_TIME, QVariant::DateTime);
    // Subject decides these:
    addField(FN_RESPONDED, QVariant::Bool);
    addField(FN_RESPONSE_TIME, QVariant::DateTime);
    addField(FN_RESPONSE_LATENCY_MS, QVariant::Int);
    addField(FN_RATING, QVariant::Int);
    addField(FN_CORRECT, QVariant::Bool);
    addField(FN_POINTS, QVariant::Int);
    addField(FN_CUMULATIVE_POINTS, QVariant::Int);

    load(load_pk);
}


CardinalExpDetTrial::CardinalExpDetTrial(
        const int task_pk,
        const int block, const int group, const int cue, const int raw_cue,
        const int target_modality, const int target_number, const bool target_present,
        const double iti_s,
        CamcopsApp& app, DatabaseManager& db) :
    CardinalExpDetTrial(app, db, dbconst::NONEXISTENT_PK)  // delegating constructor
{
    setValue(FN_FK_TO_TASK, task_pk);
    setValue(FN_BLOCK, block);
    setValue(FN_GROUP_NUM, group);
    setValue(FN_CUE, cue);
    setValue(FN_RAW_CUE_NUMBER, raw_cue);
    setValue(FN_TARGET_MODALITY, target_modality);
    setValue(FN_TARGET_NUMBER, target_number);
    setValue(FN_TARGET_PRESENT, target_present);
    setValue(FN_ITI_LENGTH_S, iti_s);
    // Doesn't yet save; see setTrialNum()
}


void CardinalExpDetTrial::setTrialNum(const int trial_num)
{
    setValue(FN_TRIAL, trial_num);
    save();
}


// ============================================================================
// Info
// ============================================================================

int CardinalExpDetTrial::cue() const
{
    return valueInt(FN_CUE);
}


bool CardinalExpDetTrial::targetPresent() const
{
    return valueBool(FN_TARGET_PRESENT);
}


int CardinalExpDetTrial::targetNumber() const
{
    return valueInt(FN_TARGET_NUMBER);
}


int CardinalExpDetTrial::targetModality() const
{
    return valueInt(FN_TARGET_MODALITY);
}


bool CardinalExpDetTrial::isTargetAuditory() const
{
    return targetModality() == cardinalexpdetcommon::MODALITY_AUDITORY;
}


int CardinalExpDetTrial::points() const
{
    return valueInt(FN_POINTS);
}


int CardinalExpDetTrial::cumulativePoints() const
{
    return valueInt(FN_CUMULATIVE_POINTS);
}


int CardinalExpDetTrial::itiLengthMs() const
{
    return datetime::secToIntMs(valueDouble(FN_ITI_LENGTH_S));
}


bool CardinalExpDetTrial::responded() const
{
    return valueBool(FN_RESPONDED);
}


// ============================================================================
// Recording
// ============================================================================

void CardinalExpDetTrial::startPauseBeforeTrial(const bool pause)
{
    setValue(FN_PAUSE_GIVEN_BEFORE_TRIAL, pause);
    if (pause) {
        setValue(FN_PAUSE_START_TIME, datetime::now());
    }
    save();
}


void CardinalExpDetTrial::startTrialWithCue()
{
    const QDateTime now = datetime::now();
    if (valueBool(FN_PAUSE_GIVEN_BEFORE_TRIAL)) {
        setValue(FN_PAUSE_END_TIME, now);
    }
    setValue(FN_TRIAL_START_TIME, now);
    setValue(FN_CUE_START_TIME, now);
    save();
}


void CardinalExpDetTrial::startTarget()
{
    setValue(FN_TARGET_START_TIME, datetime::now());
    save();
}


void CardinalExpDetTrial::startDetection()
{
    setValue(FN_DETECTION_START_TIME, datetime::now());
    save();
}


void CardinalExpDetTrial::recordResponse(const CardinalExpDetRating& rating,
                                         const int previous_points)
{
    const QDateTime now = datetime::now();
    const bool correct = rating.means_dont_know
            ? false
            : (rating.means_yes == targetPresent());
    const int points = (correct ? 1 : -1) * rating.points_multiplier;
    setValue(FN_RESPONDED, true);
    setValue(FN_RESPONSE_TIME, now);
    setValue(FN_RESPONSE_LATENCY_MS,
             valueDateTime(FN_DETECTION_START_TIME).msecsTo(now));
    setValue(FN_RATING, rating.rating);
    setValue(FN_CORRECT, correct);
    setValue(FN_POINTS, points);
    setValue(FN_CUMULATIVE_POINTS, previous_points + points);
    save();
}


void CardinalExpDetTrial::startIti()
{
    setValue(FN_ITI_START_TIME, datetime::now());
    save();
}


void CardinalExpDetTrial::endTrial()
{
    const QDateTime now = datetime::now();
    setValue(FN_ITI_END_TIME, now);
    setValue(FN_TRIAL_END_TIME, now);
    save();
}
