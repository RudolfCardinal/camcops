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

void initializeCecaQ3(TaskFactory& factory);


class CecaQ3 : public Task
{
    Q_OBJECT
public:
    CecaQ3(CamcopsApp& app, DatabaseManager& db,
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
protected:
    bool complete1A() const;
    bool complete1ASomebodySelected() const;
    bool complete1B() const;
    bool complete1C() const;
    bool complete2A() const;
    bool complete2B() const;
    bool complete3A() const;
    bool complete3B() const;
    bool complete3C() const;
    bool complete4A() const;
    bool complete4B() const;
    bool complete4C() const;
    bool complete5() const;
    bool complete6() const;
    // ------------------------------------------------------------------------
    // Signal handlers
    // ------------------------------------------------------------------------
public slots:
    void dataChanged1A();
    void dataChanged1B();
    void dataChanged1C();
    void dataChanged2A();
    void dataChanged2B();
    void dataChanged3A();
    void dataChanged3B();
    void dataChanged3C();
    void dataChanged4A();
    void dataChanged4B();
    void dataChanged4C();
    void dataChanged5();
    void dataChanged6();
    void dataChangedDummy();
protected:
    void setMandatory(bool mandatory, const QStringList& fieldnames);
    void setMultipleResponseMinAnswers(const QString& tag, int min_answers);
protected:
    QPointer<Questionnaire> m_questionnaire;
public:
    static const QString CECAQ3_TABLENAME;
};
