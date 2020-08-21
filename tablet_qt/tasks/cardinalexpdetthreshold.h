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
#include <QMediaPlayer>
#include <QPointer>
#include <QSharedPointer>
#include "maths/logisticdescriptives.h"
#include "tasklib/task.h"

class CamcopsApp;
class OpenableWidget;
class QGraphicsScene;
class QTimer;
class TaskFactory;

class CardinalExpDetThresholdTrial;

void initializeCardinalExpDetThreshold(TaskFactory& factory);


class CardinalExpDetThreshold : public Task
{
    Q_OBJECT
    using FuncPtr = void (CardinalExpDetThreshold::*)();
    // ... a pointer to a member function of CardinalExpDetThreshold that takes
    // no parameters and returns void
    using CardinalExpDetThresholdTrialPtr = QSharedPointer<CardinalExpDetThresholdTrial>;

public:
    CardinalExpDetThreshold(CamcopsApp& app, DatabaseManager& db,
                            int load_pk = dbconst::NONEXISTENT_PK);
    ~CardinalExpDetThreshold() override;
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    virtual bool isEditable() const override { return false; }
    virtual bool isCrippled() const override { return false; }
    virtual bool isExperimental() const override { return true; }
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
    // Validation for questionnaire
    // ------------------------------------------------------------------------
protected slots:
    void validateQuestionnaire();

    // ------------------------------------------------------------------------
    // Calculation/assistance functions for main task
    // ------------------------------------------------------------------------
protected:
    QString getDescriptiveModality() const;
    QString getTargetName() const;
    QVariant x(qreal p) const;
    QVariant x75() const;
    bool haveWeJustReset() const;
    bool inInitialStepPhase() const;
    bool lastTrialWasFirstNo() const;
    int getNBackNonCatchTrialIndex(int n, int start_index) const;
    qreal getIntensity() const;
    bool wantCatchTrial(int trial_num) const;
    bool isAuditory() const;
    bool timeToStop() const;

    void clearScene();
    void setTimeout(int time_ms, FuncPtr callback);
    void showVisualStimulus(const QString& filename_stem, qreal intensity);
    void savingWait();
    void reset();
    void labelTrialsForAnalysis();
    LogisticDescriptives calculateFit() const;
    void calculateAndStoreFit();

    // ------------------------------------------------------------------------
    // Main task proper
    // ------------------------------------------------------------------------
protected:
    void startTask();
protected slots:
    void nextTrial();
    void startTrial();
    void mediaStatusChangedBackground(QMediaPlayer::MediaStatus status);
    void offerChoice();
    void recordChoice(bool yes);
    void thanks();
    void abort();
    void finish();

    // ------------------------------------------------------------------------
    // Translatable text
    // ------------------------------------------------------------------------
private:
    static QString txtAuditory();
    static QString txtVisual();

    // ------------------------------------------------------------------------
    // Data
    // ------------------------------------------------------------------------
protected:
    QPointer<OpenableWidget> m_widget;
    QPointer<Questionnaire> m_questionnaire;
    QPointer<OpenableWidget> m_graphics_widget;
    QPointer<QGraphicsScene> m_scene;
    QSharedPointer<QTimer> m_timer;
    QSharedPointer<QMediaPlayer> m_player_background;
    QSharedPointer<QMediaPlayer> m_player_target;
    QVector<CardinalExpDetThresholdTrialPtr> m_trials;
    int m_current_trial;  // zero-based trial number
    int m_current_trial_ignoring_catch_trials;  // zero-based
    int m_trial_last_y_b4_first_n;  // zero-based

    // ------------------------------------------------------------------------
    // Constants
    // ------------------------------------------------------------------------
public:
    static const QString CARDINALEXPDETTHRESHOLD_TABLENAME;
};
