/*
    Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).

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
#include "questionnairelib/qugridcontainer.h"

class CamcopsApp;
class OpenableWidget;
class Questionnaire;
class TaskFactory;

void initializeDas28(TaskFactory& factory);


class Das28 : public Task
{
    Q_OBJECT
public:
    Das28(CamcopsApp& app, DatabaseManager& db,
         int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
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
    QVariant das28Crp() const;
    QVariant das28Esr() const;
public slots:
    void crpChanged();
    void esrChanged();
public:
    static const QString DAS28_TABLENAME;
protected:
    QPointer<Questionnaire> m_questionnaire;
private:
    QStringList fieldNames() const;
    QStringList getJointFieldNames() const;
    QStringList getSwollenFieldNames() const;
    QStringList getTenderFieldNames() const;
    QStringList getJointNames() const;
    int swollenJointCount() const;
    int tenderJointCount() const;

    QuGridContainer* getJointGrid();
    void addJointGridHeading(QuGridContainer* grid, int& row);

    QString activityStateCrp(const QVariant& measurement) const;
    QString activityStateEsr(const QVariant& measurement) const;
    FieldRefPtrList m_joint_fieldrefs;
    void markAllJointsOk();
};
