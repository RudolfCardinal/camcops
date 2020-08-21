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

class IDED3DExemplars;


class IDED3DStage : public DatabaseObject
{
    Q_OBJECT
public:
    IDED3DStage(CamcopsApp& app, DatabaseManager& db,
                int load_pk = dbconst::NONEXISTENT_PK);
    IDED3DStage(int task_id, CamcopsApp& app, DatabaseManager& db,
                int stage_num_zero_based,
                const QString& stage_name,
                const QString& relevant_dimension,
                const IDED3DExemplars& correct_exemplars,
                const IDED3DExemplars& incorrect_exemplars,
                int n_possible_locations,
                bool incorrect_stimulus_can_overlap);
    int taskId() const;
    int stageNumZeroBased() const;
    int nPossibleLocations() const;
    QVector<int> correctStimulusShapes() const;
    QVector<int> correctStimulusColours() const;
    QVector<int> correctStimulusNumbers() const;
    QVector<int> incorrectStimulusShapes() const;
    QVector<int> incorrectStimulusColours() const;
    QVector<int> incorrectStimulusNumbers() const;
    bool incorrectStimulusCanOverlap() const;

    void setFirstTrialIfBlank(int trial_num_zero_based);
    void recordResponse(bool correct);
    void recordTrialCompleted();
    void recordStageEnded(bool passed);
    QString summary() const;
public:
    static const QString STAGE_TABLENAME;
    static const QString FN_FK_TO_TASK;
    static const QString FN_STAGE;
protected:
    bool m_incorrect_stimulus_can_overlap;
    int m_n_possible_locations;
};
