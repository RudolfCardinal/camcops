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
class Questionnaire;
class OpenableWidget;
class TaskFactory;

class KirbyRewardPair;
class KirbyTrial;
using KirbyTrialPtr = QSharedPointer<KirbyTrial>;


void initializeKirby(TaskFactory& factory);


class Kirby : public Task
{
    Q_OBJECT
public:
    Kirby(CamcopsApp& app, DatabaseManager& db,
          int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    virtual bool isCrippled() const override { return false; }
    virtual Version minimumServerVersion() const override;
    // ------------------------------------------------------------------------
    // Ancillary management
    // ------------------------------------------------------------------------
    virtual QStringList ancillaryTables() const override;
    virtual QString ancillaryTableFKToTaskFieldname() const override;
    virtual void loadAllAncillary(int pk) override;
    virtual QVector<DatabaseObjectPtr> getAncillarySpecimens() const override;
    virtual QVector<DatabaseObjectPtr> getAllAncillary() const override;
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
    // Return (or create/save/return) a trial given a 1-based trial number.
    KirbyTrialPtr getTrial(int trial_num);

    // Sort m_trials by trial number
    void sortTrials();

    // Return a representation of all trials (including unanswered ones)
    QVector<KirbyRewardPair> allTrialResults() const;

    // Return a representation of all questions/answers for analysis
    QVector<KirbyRewardPair> allChoiceResults() const;

    // Calculate k via Kirby (2000) method (see docs)
    static double kKirby(const QVector<KirbyRewardPair>& results);

    // How many choices in "results" are consistent with the given k value?
    static int nChoicesConsistent(double k,
                                  const QVector<KirbyRewardPair>& results);

    // Calculate k via Wileyto et al. (2004) method (see docs)
    static double kWileyto(const QVector<KirbyRewardPair>& results);

    // ------------------------------------------------------------------------
    // Questionnaire callbacks
    // ------------------------------------------------------------------------
    QVariant getChoice(int trial_num);
    bool choose(int trial_num, const QVariant& chose_ldr);

    // ------------------------------------------------------------------------
    // Data
    // ------------------------------------------------------------------------
protected:
    QVector<KirbyTrialPtr> m_trials;
    QPointer<Questionnaire> m_questionnaire;

    // ------------------------------------------------------------------------
    // Text constants
    // ------------------------------------------------------------------------
public:
    static QString textXtoday();
    static QString textXinYdays();
    static QString textWouldYouPreferXOrY();

    // ------------------------------------------------------------------------
    // Constants
    // ------------------------------------------------------------------------
public:
    static const QString KIRBY_TABLENAME;
};
