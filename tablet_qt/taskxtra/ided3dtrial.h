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
class IDED3DStage;


class IDED3DTrial : public DatabaseObject
{
    Q_OBJECT
public:
    IDED3DTrial(CamcopsApp& app, DatabaseManager& db,
                int load_pk = dbconst::NONEXISTENT_PK);
    IDED3DTrial(const IDED3DStage& stage, int trial_num_zero_based,
                CamcopsApp& app, DatabaseManager& db);
    void recordTrialStart();
    void recordResponse(bool correct);
    int stageZeroBased() const;
    bool wasCorrect() const;
    int correctLocation() const;
    int correctShape() const;
    int correctColour() const;
    int correctNumber() const;
    int incorrectLocation() const;
    int incorrectShape() const;
    int incorrectColour() const;
    int incorrectNumber() const;
    QString summary() const;
public:
    static const QString TRIAL_TABLENAME;
    static const QString FN_FK_TO_TASK;
    static const QString FN_STAGE;
    static const QString FN_TRIAL;
protected:
    int m_stage_num_zero_based;
    int m_trial_num_zero_based;
};
