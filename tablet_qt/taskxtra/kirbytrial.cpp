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

#include "kirbytrial.h"
#include "kirbyrewardpair.h"
#include "lib/datetime.h"


const QString KirbyTrial::KIRBY_TRIAL_TABLENAME("kirby_mcq_trials");
const QString KirbyTrial::FN_FK_TO_TASK("kirby_mcq_id");
const QString KirbyTrial::FN_TRIAL("trial");

const QString FN_SIR("sir");
const QString FN_LDR("ldr");
const QString FN_DELAY_DAYS("delay_days");
const QString FN_CURRENCY("currency");
const QString FN_CURRENCY_SYMBOL_FIRST("currency_symbol_first");
const QString FN_CHOSE_LDR("chose_ldr");


KirbyTrial::KirbyTrial(CamcopsApp& app, DatabaseManager& db, int load_pk) :
    DatabaseObject (app, db, KIRBY_TRIAL_TABLENAME)
{
    // Keys
    addField(FN_FK_TO_TASK, QVariant::Int);
    addField(FN_TRIAL, QVariant::Int, true);  // trial number within this session, 1-based
    // Choice
    addField(FN_SIR, QVariant::Int);  // int for now
    addField(FN_LDR, QVariant::Int);  // int for now
    addField(FN_DELAY_DAYS, QVariant::Int);  // int for now
    addField(FN_CURRENCY, QVariant::String);
    addField(FN_CURRENCY_SYMBOL_FIRST, QVariant::Bool);
    // Response
    addField(FN_CHOSE_LDR, QVariant::Bool);

    load(load_pk);
}


KirbyTrial::KirbyTrial(const int task_pk, const int trial_num,
                       const KirbyRewardPair& choice,
                       CamcopsApp& app, DatabaseManager& db) :
    KirbyTrial(app, db, dbconst::NONEXISTENT_PK)  // delegating constructor
{
    setValue(FN_FK_TO_TASK, task_pk);
    setValue(FN_TRIAL, trial_num);  // 1-based

    setValue(FN_SIR, choice.sir);
    setValue(FN_LDR, choice.ldr);
    setValue(FN_DELAY_DAYS, choice.delay_days);
    setValue(FN_CURRENCY, choice.currency);
    setValue(FN_CURRENCY_SYMBOL_FIRST, choice.currency_symbol_first);

    save();
}


int KirbyTrial::trialNum() const
{
    return valueInt(FN_TRIAL);
}


KirbyRewardPair KirbyTrial::info() const
{
    return KirbyRewardPair(
                valueInt(FN_SIR),
                valueInt(FN_LDR),
                valueInt(FN_DELAY_DAYS),
                value(FN_CHOSE_LDR),
                valueString(FN_CURRENCY),
                valueBool(FN_CURRENCY_SYMBOL_FIRST)
    );
}


void KirbyTrial::recordChoice(const bool chose_ldr)
{
    setValue(FN_CHOSE_LDR, chose_ldr);
    save();
}


QVariant KirbyTrial::getChoice() const
{
    return value(FN_CHOSE_LDR);
}


bool KirbyTrial::answered() const
{
    return !valueIsNull(FN_CHOSE_LDR);
}
