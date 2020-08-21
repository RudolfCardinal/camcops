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
#include "questionnairelib/namevalueoptions.h"
#include "tasklib/task.h"

class CamcopsApp;
class OpenableWidget;
class TaskFactory;

void initializeGmcPq(TaskFactory& factory);


class GmcPq : public Task
{
    Q_OBJECT
public:
    GmcPq(CamcopsApp& app, DatabaseManager& db,
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
    NameValueOptions optionsQ1() const;
    NameValueOptions optionsQ3() const;
    NameValueOptions optionsQ4() const;
    NameValueOptions optionsQ5() const;
    NameValueOptions optionsQ11() const;
    static NameValueOptions ethnicityOptions(CamcopsApp& app);  // used by others, hence static
    static bool ethnicityOther(int ethnicity_code);
    // ------------------------------------------------------------------------
    // Signal handlers
    // ------------------------------------------------------------------------
public slots:
    void updateMandatory();
    // ------------------------------------------------------------------------
    // Other
    // ------------------------------------------------------------------------
public:
    static const QString GMCPQ_TABLENAME;
};
