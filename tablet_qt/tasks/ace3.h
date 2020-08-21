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

void initializeAce3(TaskFactory& factory);


class Ace3 : public Task
{
    Q_OBJECT
public:
    Ace3(CamcopsApp& app, DatabaseManager& db,
         int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    virtual bool hasClinician() const override { return true; }
    virtual bool prohibitsCommercial() const override { return true; }
    // ------------------------------------------------------------------------
    // Instance overrides
    // ------------------------------------------------------------------------
    virtual bool isComplete() const override;
    virtual QStringList summary() const override;
    virtual OpenableWidget* editor(bool read_only = false) override;
    // ------------------------------------------------------------------------
    // Task-specific calculations
    // ------------------------------------------------------------------------
    int getAttnScore() const;
    int getMemRecognitionScore() const;
    int getMemScore() const;
    int getFluencyScore() const;
    int getFollowCommandScore() const;
    int getRepeatWordScore() const;
    int getLangScore() const;
    int getVisuospatialScore() const;
    int totalScore() const;
    bool isRecognitionComplete() const;
    // ------------------------------------------------------------------------
    // Signal handlers
    // ------------------------------------------------------------------------
public slots:
    void langPracticeChanged(const FieldRef* fieldref);
    void updateAddressRecognition();
protected:
    QPointer<Questionnaire> m_questionnaire;
public:
    static const QString ACE3_TABLENAME;
};
