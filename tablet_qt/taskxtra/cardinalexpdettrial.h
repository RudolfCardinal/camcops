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
class CardinalExpDetRating;


class CardinalExpDetTrial : public DatabaseObject
{
    Q_OBJECT
public:
    CardinalExpDetTrial(CamcopsApp& app, DatabaseManager& db,
                        int load_pk = dbconst::NONEXISTENT_PK);
    CardinalExpDetTrial(
            int task_pk,
            int block, int group, int cue, int raw_cue,
            int target_modality, int target_number, bool target_present,
            double iti_s,
            CamcopsApp& app, DatabaseManager& db);
    // Setup
    void setTrialNum(int trial_num);
    // Info
    int cue() const;
    bool targetPresent() const;
    int targetNumber() const;
    int targetModality() const;
    bool isTargetAuditory() const;
    int points() const;
    int cumulativePoints() const;
    int itiLengthMs() const;
    bool responded() const;
    // Recording
    void startPauseBeforeTrial(bool pause);
    void startTrialWithCue();
    void startTarget();
    void startDetection();
    void recordResponse(const CardinalExpDetRating& rating,
                        int previous_points);
    void startIti();
    void endTrial();
public:
    static const QString TRIAL_TABLENAME;
    static const QString FN_FK_TO_TASK;
    static const QString FN_TRIAL;
protected:
};
