/*
    Copyright (C) 2012-2018 Rudolf Cardinal (rudolf@pobox.com).

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
#include <QPointer>
#include <QString>
#include "tasklib/task.h"

class CamcopsApp;
class OpenableWidget;
class Questionnaire;
class TaskFactory;

void initializeCesdr(TaskFactory& factory);

class Cesdr : public Task
{
    Q_OBJECT
public:
    Cesdr(CamcopsApp& app, DatabaseManager& db,
        int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString menusubtitle() const override;
    virtual Version minimumServerVersion() const override;
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
    int totalScore() const;
    int numNull(int first, int last) const;
    int depressionCategory() const;
    bool fulfilsGroupCriteria(const QVector<int>& qnums,
                              bool nearly_every_day_2w = false) const;
public:
    static const QString CESDR_TABLENAME;
protected:
    QPointer<Questionnaire> m_questionnaire;
};