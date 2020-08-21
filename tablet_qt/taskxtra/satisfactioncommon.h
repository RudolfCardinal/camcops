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


class SatisfactionCommon : public Task
{
    // abstract base class
    // not a Q_OBJECT
public:
    SatisfactionCommon(CamcopsApp& app, DatabaseManager& db,
                       const QString& tablename, bool anonymous,
                       int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override = 0;
    virtual QString longname() const override = 0;
    virtual QString description() const override = 0;
    virtual bool isCrippled() const override { return false; }
    // ------------------------------------------------------------------------
    // Instance overrides
    // ------------------------------------------------------------------------
    virtual bool isComplete() const override;
    virtual QStringList summary() const override;
    virtual QStringList detail() const override;
    virtual void setDefaultsAtFirstUse() override;
    virtual OpenableWidget* editor(bool read_only = false) override = 0;
    OpenableWidget* satisfactionEditor(const QString& rating_q,
                                       bool read_only);
    // ------------------------------------------------------------------------
    // Task-specific calculations
    // ------------------------------------------------------------------------
protected:
    QString getRatingText() const;
};
