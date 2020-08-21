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
#include <QString>
#include "tasklib/task.h"

class CamcopsApp;
class OpenableWidget;
class TaskFactory;

void initializeHads(TaskFactory& factory);


class Hads : public Task
{
    // We also make this task suitable for use for respondents who are not the
    // primary patient (e.g. carers). This involves no modification to the HADS
    // structure, so we have two options:
    //
    // (1) one task + boolean patient-or-not flag + respondent info applicable
    //     if respondent is not the patient
    // (2) two tasks, one for primary patient, one for other respondent
    //
    // Both would be reasonable. The risk with (1) is that someone charts
    // numerical progress on the HADS thinking it's from the patient, and it
    // isn't. That alone warrants a strong "patient task versus respondent
    // task" distinction. So we'll have an additional task, HadsRespondent,
    // which inherits from this.

    Q_OBJECT
public:
    Hads(CamcopsApp& app, DatabaseManager& db,
         int load_pk = dbconst::NONEXISTENT_PK);
protected:
    Hads(CamcopsApp& app, DatabaseManager& db,
         const QString& tablename, bool has_respondent,
         int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
public:
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    virtual TaskImplementationType implementationType() const override {
        return TaskImplementationType::UpgradableSkeleton;
    }
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
protected:
    int getScore(const QVector<int>& questions) const;
public:
    static const QString HADS_TABLENAME;
};
