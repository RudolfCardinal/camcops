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

#include "cardinalexpdettrialgroupspec.h"

// Tablename
const QString CardinalExpDetTrialGroupSpec::GROUPSPEC_TABLENAME(
        "cardinal_expdet_trialgroupspec");

// Fieldnames
const QString CardinalExpDetTrialGroupSpec::FN_FK_TO_TASK("cardinal_expdet_id");
const QString CardinalExpDetTrialGroupSpec::FN_GROUP_NUM("group_num");
const QString FN_CUE("cue");
const QString FN_TARGET_MODALITY("target_modality");
const QString FN_TARGET_NUMBER("target_number");
const QString FN_N_TARGET("n_target");
const QString FN_N_NO_TARGET("n_no_target");


CardinalExpDetTrialGroupSpec::CardinalExpDetTrialGroupSpec(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    DatabaseObject(app, db, GROUPSPEC_TABLENAME)
{
    // Keys
    addField(FN_FK_TO_TASK, QVariant::Int);
    addField(FN_GROUP_NUM, QVariant::Int);
    // Data
    addField(FN_CUE, QVariant::Int);
    addField(FN_TARGET_MODALITY, QVariant::Int);
    addField(FN_TARGET_NUMBER, QVariant::Int);
    addField(FN_N_TARGET, QVariant::Int);
    addField(FN_N_NO_TARGET, QVariant::Int);

    load(load_pk);
}


CardinalExpDetTrialGroupSpec::CardinalExpDetTrialGroupSpec(
        const int task_pk, const int group_num,
        const int cue, const int target_modality, const int target_number,
        const int n_target, const int n_no_target,
        CamcopsApp& app, DatabaseManager& db) :
    CardinalExpDetTrialGroupSpec::CardinalExpDetTrialGroupSpec(
        app, db, dbconst::NONEXISTENT_PK)  // delegating constructor
{
    setValue(FN_FK_TO_TASK, task_pk);
    setValue(FN_GROUP_NUM, group_num);
    setValue(FN_CUE, cue);
    setValue(FN_TARGET_MODALITY, target_modality);
    setValue(FN_TARGET_NUMBER, target_number);
    setValue(FN_N_TARGET, n_target);
    setValue(FN_N_NO_TARGET, n_no_target);

    save();
}


int CardinalExpDetTrialGroupSpec::cue() const
{
    return valueInt(FN_CUE);
}


int CardinalExpDetTrialGroupSpec::targetModality() const
{
    return valueInt(FN_TARGET_MODALITY);
}


int CardinalExpDetTrialGroupSpec::targetNumber() const
{
    return valueInt(FN_TARGET_NUMBER);
}


int CardinalExpDetTrialGroupSpec::nTarget() const
{
    return valueInt(FN_N_TARGET);
}


int CardinalExpDetTrialGroupSpec::nNoTarget() const
{
    return valueInt(FN_N_NO_TARGET);
}
