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

void initializeDeakinS1HealthReview(TaskFactory& factory);


class DeakinS1HealthReview : public Task
{
    Q_OBJECT
public:
    DeakinS1HealthReview(CamcopsApp& app, DatabaseManager& db,
                         int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    QString infoFilenameStem() const override;
    virtual bool isCrippled() const override { return false; }
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
    // Signal handlers
    // ------------------------------------------------------------------------
signals:
    void setAllergyMinimum(int minimum_answers);
    void setRecentInfectionsMinimum(int minimum_answers);
    void setChronicInfectionsMinimum(int minimum_answers);
    void setImmuneMinimum(int minimum_answers);
    void setFHImmuneMinimum(int minimum_answers);
public slots:
    void updateMandatory();
public:
    static const QString DEAKIN_S1_HEALTHREVIEW_TABLENAME;
};
