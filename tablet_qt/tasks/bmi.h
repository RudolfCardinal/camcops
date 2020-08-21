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
#include <QPointer>
#include <QString>
#include "tasklib/task.h"

class CamcopsApp;
class OpenableWidget;
class Questionnaire;
class TaskFactory;

void initializeBmi(TaskFactory& factory);


class Bmi : public Task
{
    Q_OBJECT
public:
    Bmi(CamcopsApp& app, DatabaseManager& db,
        int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
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
    QVariant bmiVariant() const;
    QString bmiString(int dp = 2) const;  // to specified d.p.
    QString category() const;
    // ------------------------------------------------------------------------
    // Signal handlers
    // ------------------------------------------------------------------------
public slots:
    void massUnitsChanged();
    void heightUnitsChanged();
    void waistUnitsChanged();
protected:
    QPointer<Questionnaire> m_questionnaire;
    // ------------------------------------------------------------------------
    // Getters/setters
    // ------------------------------------------------------------------------
public:
    QVariant getHeightUnits() const;
    QVariant getHeightM() const;
    QVariant getHeightFt() const;
    QVariant getHeightIn() const;
    bool setHeightUnits(const QVariant& value);  // returns: changed?
    bool setHeightM(const QVariant& value);
    bool setHeightFt(const QVariant& value);
    bool setHeightIn(const QVariant& value);
    void updateMetricHeight();
    void updateImperialHeight();

    QVariant getMassUnits() const;
    QVariant getMassKg() const;
    QVariant getMassSt() const;
    QVariant getMassLb() const;
    QVariant getMassOz() const;
    bool setMassUnits(const QVariant& value);  // returns: changed?
    bool setMassKg(const QVariant& value);
    bool setMassSt(const QVariant& value);
    bool setMassLb(const QVariant& value);
    bool setMassOz(const QVariant& value);
    void updateMetricMass();
    void updateImperialMass();

    QVariant getWaistUnits() const;
    QVariant getWaistCm() const;
    QVariant getWaistIn() const;
    bool setWaistUnits(const QVariant& value);  // returns: changed?
    bool setWaistCm(const QVariant& value);
    bool setWaistIn(const QVariant& value);
    void updateMetricWaist();
    void updateImperialWaist();


protected:
    int m_height_units;
    QVariant m_height_ft;
    QVariant m_height_in;

    int m_mass_units;
    QVariant m_mass_st;
    QVariant m_mass_lb;
    QVariant m_mass_oz;

    int m_waist_units;
    QVariant m_waist_cm;
    QVariant m_waist_in;

    FieldRefPtr m_fr_height_units;
    FieldRefPtr m_fr_height_m;
    FieldRefPtr m_fr_height_ft;
    FieldRefPtr m_fr_height_in;

    FieldRefPtr m_fr_mass_units;
    FieldRefPtr m_fr_mass_kg;
    FieldRefPtr m_fr_mass_st;
    FieldRefPtr m_fr_mass_lb;
    FieldRefPtr m_fr_mass_oz;

    FieldRefPtr m_fr_waist_units;
    FieldRefPtr m_fr_waist_cm;
    FieldRefPtr m_fr_waist_in;
private:
    void setUpHeightFields();
    void setUpMassFields();
    void setUpWaistFields();
public:
    static const QString BMI_TABLENAME;
};
