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

void initializeIcd10Depressive(TaskFactory& factory);


class Icd10Depressive : public Task
{
    Q_OBJECT
public:
    Icd10Depressive(CamcopsApp& app, DatabaseManager& db,
                    int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    virtual QString infoFilenameStem() const override;
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
    int nCore() const;
    int nAdditional() const;
    int nTotal() const;
    int nSomatic() const;
    bool mainComplete() const;
    // The QVariant ones return true, false, or NULL (for unknown):
    QVariant meetsCriteriaSeverePsychoticSchizophrenic() const;
    QVariant meetsCriteriaSeverePsychoticIcd() const;
    QVariant meetsCriteriaSevereNonpsychotic() const;
    QVariant meetsCriteriaSevereIgnoringPsychosis() const;
    QVariant meetsCriteriaModerate() const;
    QVariant meetsCriteriaMild() const;
    QVariant meetsCriteriaNone() const;
    QVariant meetsCriteriaSomatic() const;
    QString getSomaticDescription() const;
    QString getMainDescription() const;
    QString getFullDescription() const;
    QStringList detailGroup(const QStringList& fieldnames) const;
    // ------------------------------------------------------------------------
    // Signal handlers
    // ------------------------------------------------------------------------
protected:
    void updateMandatory();
public:
    static const QString ICD10DEP_TABLENAME;
};
