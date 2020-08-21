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
#include "db/databaseobject.h"
#include "kirbyrewardpair.h"


class KirbyTrial : public DatabaseObject
{
    Q_OBJECT
public:
    // Load, or create blank
    KirbyTrial(CamcopsApp& app, DatabaseManager& db,
               int load_pk = dbconst::NONEXISTENT_PK);

    // Create and save
    KirbyTrial(int task_pk, int trial_num, const KirbyRewardPair& info,
               CamcopsApp& app, DatabaseManager& db);

    // Trial number (1-based)
    int trialNum() const;

    // Return the original question.
    KirbyRewardPair info() const;

    // Record a subject's response: did they choose the large delayed reward?
    void recordChoice(bool chose_ldr);

    // Return the choice
    QVariant getChoice() const;

    // Answered?
    bool answered() const;

public:
    static const QString KIRBY_TRIAL_TABLENAME;
    static const QString FN_FK_TO_TASK;
    static const QString FN_TRIAL;
protected:
};
