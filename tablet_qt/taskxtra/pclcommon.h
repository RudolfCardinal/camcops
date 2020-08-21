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


class PclCommon : public Task
{
    // abstract base class
    Q_OBJECT  // for tr()
public:
    PclCommon(CamcopsApp& app, DatabaseManager& db,
              const QString& tablename,
              const QString& xstring_prefix,
              bool specific_event,
              int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override = 0;
    virtual QString longname() const override = 0;
    virtual QString description() const override;
    virtual QString infoFilenameStem() const override;
    virtual QString xstringTaskname() const override;
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
    int totalScore() const;
    int numSymptomatic(int first, int last) const;
    int numNull(int first, int last) const;
    QVariant hasPtsd() const;
protected:
    QString m_xstring_prefix;
    bool m_specific_event;
};
