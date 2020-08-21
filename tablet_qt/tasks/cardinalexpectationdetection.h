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
#include "tasklib/task.h"
#include "taskxtra/cardinalexpdetrating.h"

class CamcopsApp;
class OpenableWidget;
class QGraphicsScene;
class QTimer;
class TaskFactory;

class CardinalExpDetTrial;
class CardinalExpDetTrialGroupSpec;
using CardinalExpDetTrialPtr = QSharedPointer<CardinalExpDetTrial>;
using CardinalExpDetTrialGroupSpecPtr = QSharedPointer<CardinalExpDetTrialGroupSpec>;

void initializeCardinalExpectationDetection(TaskFactory& factory);


class CardinalExpectationDetection : public Task
{
    Q_OBJECT
    using FuncPtr = void (CardinalExpectationDetection::*)();
    // ... a pointer to a member function of CardinalExpectationDetection that
    // takes no parameters and returns void

public:
    CardinalExpectationDetection(CamcopsApp& app, DatabaseManager& db,
                                 int load_pk = dbconst::NONEXISTENT_PK);
    ~CardinalExpectationDetection() override;
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
    // Calculation/assistance functions for main task
    // ------------------------------------------------------------------------
protected:
    void makeTrialGroupSpecs();
    QRectF getRatingButtonRect(int x, int n) const;
    void makeRatingButtonsAndPoints();
    void doCounterbalancing();
    int getRawCueIndex(int cue) const;
    QUrl getAuditoryCueUrl(int cue) const;
    QString getVisualCueFilenameStem(int cue) const;
    QUrl getAuditoryTargetUrl(int target_number) const;
    QString getVisualTargetFilenameStem(int target_number) const;
    QUrl getAuditoryBackgroundUrl() const;
    QString getVisualBackgroundFilename() const;
    QString getPromptText(int modality,  int target_number) const;
    void reportCounterbalancing() const;
    QVector<CardinalExpDetTrialPtr> makeTrialGroup(
            int block,  // zero-based block numbering
            int groupnum,  // zero-based group numbering
            const CardinalExpDetTrialGroupSpecPtr& groupspec) const;
    void createTrials();
    void estimateRemaining(int& n_trials_left, double& time_min) const;
    void clearScene();
    void setTimeout(int time_ms, FuncPtr callback);
    CardinalExpDetTrialPtr currentTrial() const;
    void showVisualStimulus(const QString& filename_stem, qreal intensity);

    // ------------------------------------------------------------------------
    // Main task proper
    // ------------------------------------------------------------------------
protected:
    void startTask();
protected slots:
    void nextTrial();
    void userPause();
    void startTrialProperWithCue();
    void isi();
    void target();
    void mediaStatusChangedBackground(QMediaPlayer::MediaStatus status);
    void detection();
    void processResponse(int rating);
    void displayScore();
    void iti();
    void endTrial();
    void thanks();
    void askAbort(FuncPtr nextfn);
    void abort();
    void finish();

    // ------------------------------------------------------------------------
    // Data
    // ------------------------------------------------------------------------
protected:
    QPointer<OpenableWidget> m_widget;
    QPointer<Questionnaire> m_questionnaire;
    QPointer<OpenableWidget> m_graphics_widget;
    QPointer<QGraphicsScene> m_scene;
    QSharedPointer<QTimer> m_timer;
    QSharedPointer<QMediaPlayer> m_player_cue;
    QSharedPointer<QMediaPlayer> m_player_background;
    QSharedPointer<QMediaPlayer> m_player_target_0;
    QSharedPointer<QMediaPlayer> m_player_target_1;
    QVector<CardinalExpDetTrialGroupSpecPtr> m_groups;
    QVector<CardinalExpDetTrialPtr> m_trials;
    int m_current_trial;  // zero-based trial number
    QVector<int> m_raw_cue_indices;  // Means of counterbalancing
    QVector<CardinalExpDetRating> m_ratings;

    // ------------------------------------------------------------------------
    // Constants
    // ------------------------------------------------------------------------
public:
    static const QString CARDINALEXPDET_TABLENAME;
};
