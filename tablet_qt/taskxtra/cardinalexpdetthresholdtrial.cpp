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

#include "cardinalexpdetthresholdtrial.h"
#include "lib/datetime.h"

// Tablename
const QString CardinalExpDetThresholdTrial::TRIAL_TABLENAME("cardinal_expdetthreshold_trials");

// Fieldnames
const QString CardinalExpDetThresholdTrial::FN_FK_TO_TASK("cardinal_expdetthreshold_id");
const QString CardinalExpDetThresholdTrial::FN_TRIAL("trial");
const QString FN_TRIAL_IGNORING_CATCH_TRIALS("trial_ignoring_catch_trials");
const QString FN_TARGET_PRESENTED("target_presented");
const QString FN_TARGET_TIME("target_time");
const QString FN_INTENSITY("intensity");
const QString FN_CHOICE_TIME("choice_time");
const QString FN_RESPONDED("responded");
const QString FN_RESPONSE_TIME("response_time");
const QString FN_RESPONSE_LATENCY_MS("response_latency_ms");
const QString FN_YES("yes");
const QString FN_NO("no");
const QString FN_CAUGHT_OUT_RESET("caught_out_reset");
const QString FN_TRIAL_NUM_IN_CALCULATION_SEQUENCE("trial_num_in_calculation_sequence");


CardinalExpDetThresholdTrial::CardinalExpDetThresholdTrial(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    DatabaseObject(app, db, TRIAL_TABLENAME)
{
    // Keys
    addField(FN_FK_TO_TASK, QVariant::Int);
    addField(FN_TRIAL, QVariant::Int, true);  // trial number within this session, 0-based
    // Results
    addField(FN_TRIAL_IGNORING_CATCH_TRIALS, QVariant::Int);
    addField(FN_TARGET_PRESENTED, QVariant::Bool);
    addField(FN_TARGET_TIME, QVariant::DateTime);
    addField(FN_INTENSITY, QVariant::Double);
    addField(FN_CHOICE_TIME, QVariant::DateTime);
    addField(FN_RESPONDED, QVariant::Bool);
    addField(FN_RESPONSE_TIME, QVariant::DateTime);
    addField(FN_RESPONSE_LATENCY_MS, QVariant::Int);
    addField(FN_YES, QVariant::Bool);
    addField(FN_NO, QVariant::Bool);
    addField(FN_CAUGHT_OUT_RESET, QVariant::Bool);
    addField(FN_TRIAL_NUM_IN_CALCULATION_SEQUENCE, QVariant::Int);  // 0 or NULL for trials not used

    load(load_pk);
}


CardinalExpDetThresholdTrial::CardinalExpDetThresholdTrial(
        int task_pk, int trial_num,
        const QVariant& trial_num_ignoring_catch_trials,
        bool target_presented,
        CamcopsApp& app, DatabaseManager& db) :
    CardinalExpDetThresholdTrial::CardinalExpDetThresholdTrial(
        app, db, dbconst::NONEXISTENT_PK)  // delegating constructor
{
    setValue(FN_FK_TO_TASK, task_pk);
    setValue(FN_TRIAL, trial_num);  // 0-based
    setValue(FN_TRIAL_IGNORING_CATCH_TRIALS, trial_num_ignoring_catch_trials);  // 0-based
    setValue(FN_TARGET_PRESENTED, target_presented);
    if (target_presented) {
        QDateTime now = datetime::now();
        setValue(FN_TARGET_TIME, now);
    }
    save();
}


bool CardinalExpDetThresholdTrial::wasCaughtOutReset() const
{
    return valueBool(FN_CAUGHT_OUT_RESET);
}


int CardinalExpDetThresholdTrial::trialNum() const
{
    return valueInt(FN_TRIAL);
}


int CardinalExpDetThresholdTrial::trialNumIgnoringCatchTrials() const
{
    return valueInt(FN_TRIAL_IGNORING_CATCH_TRIALS);
}


bool CardinalExpDetThresholdTrial::targetPresented() const
{
    return valueBool(FN_TARGET_PRESENTED);
}


qreal CardinalExpDetThresholdTrial::intensity() const
{
    return valueDouble(FN_INTENSITY);
}


void CardinalExpDetThresholdTrial::setIntensity(double intensity)
{
    setValue(FN_INTENSITY, intensity);
    save();
}


bool CardinalExpDetThresholdTrial::yes() const
{
    return valueBool(FN_YES);
}


void CardinalExpDetThresholdTrial::setCaughtOutReset()
{
    setValue(FN_CAUGHT_OUT_RESET, true);
    save();
}


void CardinalExpDetThresholdTrial::recordChoiceTime()
{
    const QDateTime now = datetime::now();
    setValue(FN_CHOICE_TIME, now);
    save();
}


void CardinalExpDetThresholdTrial::recordResponse(const bool yes)
{
    const QDateTime now = datetime::now();
    setValue(FN_RESPONDED, true);
    setValue(FN_RESPONSE_TIME, now);
    setValue(FN_RESPONSE_LATENCY_MS,
             valueDateTime(FN_CHOICE_TIME).msecsTo(now));
    setValue(FN_YES, yes);
    setValue(FN_NO, !yes);
    save();
}


QString CardinalExpDetThresholdTrial::summary() const
{
    if (!valueBool(FN_TARGET_PRESENTED)) {
        return "Catch trial";
    }
    return QString("Normal trial [#%1, w/o catch trials #%2], intensity %3")
            .arg(valueInt(FN_TRIAL))
            .arg(valueInt(FN_TRIAL_IGNORING_CATCH_TRIALS))
            .arg(valueDouble(FN_INTENSITY));
}


void CardinalExpDetThresholdTrial::setTrialNumInCalcSeq(const QVariant& value)
{
    setValue(FN_TRIAL_NUM_IN_CALCULATION_SEQUENCE, value);
    save();
}


bool CardinalExpDetThresholdTrial::isInCalculationSeq() const
{
    // See CardinalExpDetThreshold::labelTrialsForAnalysis()
    return valueInt(FN_TRIAL_NUM_IN_CALCULATION_SEQUENCE) >= 1;
}
