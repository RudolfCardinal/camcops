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

void initializeIcd10SpecPD(TaskFactory& factory);


class Icd10SpecPD : public Task
{
    Q_OBJECT
public:
    Icd10SpecPD(CamcopsApp& app, DatabaseManager& db,
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
    bool isPDExcluded() const;
    bool isCompleteGeneral() const;
    bool isCompleteParanoid() const;
    bool isCompleteSchizoid() const;
    bool isCompleteDissocial() const;
    bool isCompleteEU() const;
    bool isCompleteHistrionic() const;
    bool isCompleteAnankastic() const;
    bool isCompleteAnxious() const;
    bool isCompleteDependent() const;
    QVariant hasPD() const;
    QVariant hasParanoidPD() const;
    QVariant hasSchizoidPD() const;
    QVariant hasDissocialPD() const;
    QVariant hasEUPD_I() const;
    QVariant hasEUPD_B() const;
    QVariant hasHistrionicPD() const;
    QVariant hasAnankasticPD() const;
    QVariant hasAnxiousPD() const;
    QVariant hasDependentPD() const;
    // ------------------------------------------------------------------------
    // Signal handlers
    // ------------------------------------------------------------------------
protected:
    void updateMandatory();
    QVariant getHasPDYesNoUnknown() const;
    bool ignoreValue(const QVariant& value) const;
protected:
    FieldRefPtr m_fr_has_pd;
public:
    static const QString ICD10SPECPD_TABLENAME;
};
