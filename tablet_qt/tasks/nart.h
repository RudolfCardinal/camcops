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

void initializeNart(TaskFactory& factory);


class Nart : public Task
{
    Q_OBJECT
public:
    struct NartIQ
    {
        NartIQ(const QString& quantity, const QString& reference,
               const QString& formula, const QVariant& iq) :
            quantity(quantity),
            reference(reference),
            formula(formula),
            iq(iq)
        {}
        QString quantity;
        QString reference;
        QString formula;
        QVariant iq;
    };

    Nart(CamcopsApp& app, DatabaseManager& db,
         int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
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
protected:
    int numErrors() const;
    NartIQ nelsonFullScaleIQ(bool complete, int errors) const;
    NartIQ nelsonVerbalIQ(bool complete, int errors) const;
    NartIQ nelsonPerformanceIQ(bool complete, int errors) const;
    NartIQ nelsonWillisonFullScaleIQ(bool complete, int errors) const;
    NartIQ brightFullScaleIQ(bool complete, int errors) const;
    NartIQ brightGeneralAbility(bool complete, int errors) const;
    NartIQ brightVerbalComprehension(bool complete, int errors) const;
    NartIQ brightPerceptualReasoning(bool complete, int errors) const;
    NartIQ brightWorkingMemory(bool complete, int errors) const;
    NartIQ brightPerceptualSpeed(bool complete, int errors) const;
    QString result(const NartIQ& iq, bool full = true) const;
public:
    static const QString NART_TABLENAME;
};
