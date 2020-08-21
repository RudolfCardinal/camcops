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

// Consider: linear v. logarithmic volume; http://doc.qt.io/qt-5/qaudio.html#convertVolume

// #define DEBUG_STEP_DETAIL

#include "cardinalexpdetthreshold.h"
#include <QGraphicsScene>
#include <QPushButton>
#include <QTimer>
#include "common/uiconst.h"
#include "db/ancillaryfunc.h"
#include "db/dbnestabletransaction.h"
#include "graphics/graphicsfunc.h"
#include "lib/convert.h"
#include "lib/soundfunc.h"
#include "lib/timerfunc.h"
#include "lib/uifunc.h"
#include "maths/ccrandom.h"
#include "maths/mathfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskregistrar.h"
#include "taskxtra/cardinalexpdetcommon.h"
#include "taskxtra/cardinalexpdetthresholdtrial.h"
using namespace cardinalexpdetcommon;  // lots...
using ccrandom::coin;
using ccrandom::randomRealIncUpper;
using convert::msFromSec;
using graphicsfunc::ButtonAndProxy;
using graphicsfunc::makeImage;
using graphicsfunc::makeText;
using graphicsfunc::makeTextButton;
using mathfunc::mean;


// ============================================================================
// Constants
// ============================================================================

const QString CardinalExpDetThreshold::CARDINALEXPDETTHRESHOLD_TABLENAME(
        "cardinal_expdetthreshold");

// Fieldnames: config
const QString FN_MODALITY("modality");
const QString FN_TARGET_NUMBER("target_number");
const QString FN_BACKGROUND_FILENAME("background_filename");
const QString FN_TARGET_FILENAME("target_filename");
const QString FN_VISUAL_TARGET_DURATION_S("visual_target_duration_s");
const QString FN_BACKGROUND_INTENSITY("background_intensity");
const QString FN_START_INTENSITY_MIN("start_intensity_min");
const QString FN_START_INTENSITY_MAX("start_intensity_max");
const QString FN_INITIAL_LARGE_INTENSITY_STEP("initial_large_intensity_step");
const QString FN_MAIN_SMALL_INTENSITY_STEP("main_small_intensity_step");
const QString FN_NUM_TRIALS_IN_MAIN_SEQUENCE("num_trials_in_main_sequence");
const QString FN_P_CATCH_TRIAL("p_catch_trial");
const QString FN_PROMPT("prompt");
const QString FN_ITI_S("iti_s");
// Fieldnames: results
const QString FN_FINISHED("finished");
const QString FN_INTERCEPT("intercept");
const QString FN_SLOPE("slope");
const QString FN_K("k");
const QString FN_THETA("theta");

// Defaults
const qreal DEFAULT_VISUAL_TARGET_DURATION_S = 1.0;
const qreal DEFAULT_BACKGROUND_INTENSITY = 1.0;
const qreal DEFAULT_START_INTENSITY_MIN = 0.9;
const qreal DEFAULT_START_INTENSITY_MAX = 1.0;
const qreal DEFAULT_INITIAL_LARGE_INTENSITY_STEP = 0.1;
const qreal DEFAULT_MAIN_SMALL_INTENSITY_STEP = 0.01;
const int DEFAULT_NUM_TRIALS_IN_MAIN_SEQUENCE = 14;
const qreal DEFAULT_P_CATCH_TRIAL = 0.2;
const qreal DEFAULT_ITI_S = 0.2;

// Tags
const QString TAG_P2("p2");
const QString TAG_P3("p3");
const QString TAG_AUDITORY("a");
const QString TAG_VISUAL("v");
const QString TAG_WARNING_MIN_MAX("mm");

// Other
const int DP = 3;

// ============================================================================
// Factory method
// ============================================================================

void initializeCardinalExpDetThreshold(TaskFactory& factory)
{
    static TaskRegistrar<CardinalExpDetThreshold> registered(factory);
}


// ============================================================================
// CardinalExpectationDetection
// ============================================================================

CardinalExpDetThreshold::CardinalExpDetThreshold(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CARDINALEXPDETTHRESHOLD_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    // Config
    addField(FN_MODALITY, QVariant::Int);
    addField(FN_TARGET_NUMBER, QVariant::Int);
    addField(FN_BACKGROUND_FILENAME, QVariant::String);  // set automatically
    addField(FN_TARGET_FILENAME, QVariant::String);  // set automatically
    addField(FN_VISUAL_TARGET_DURATION_S, QVariant::Double);
    addField(FN_BACKGROUND_INTENSITY, QVariant::Double);
    addField(FN_START_INTENSITY_MIN, QVariant::Double);
    addField(FN_START_INTENSITY_MAX, QVariant::Double);
    addField(FN_INITIAL_LARGE_INTENSITY_STEP, QVariant::Double);
    addField(FN_MAIN_SMALL_INTENSITY_STEP, QVariant::Double);
    addField(FN_NUM_TRIALS_IN_MAIN_SEQUENCE, QVariant::Int);
    addField(FN_P_CATCH_TRIAL, QVariant::Double);
    addField(FN_PROMPT, QVariant::String);
    addField(FN_ITI_S, QVariant::Double);
    // Results
    addField(FN_FINISHED, QVariant::Bool);
    addField(FN_INTERCEPT, QVariant::Double);
    addField(FN_SLOPE, QVariant::Double);
    addField(FN_K, QVariant::Double);
    addField(FN_THETA, QVariant::Double);

    load(load_pk);

    if (load_pk == dbconst::NONEXISTENT_PK) {
        // Default values:
        setValue(FN_VISUAL_TARGET_DURATION_S, DEFAULT_VISUAL_TARGET_DURATION_S, false);
        setValue(FN_BACKGROUND_INTENSITY, DEFAULT_BACKGROUND_INTENSITY, false);
        setValue(FN_START_INTENSITY_MIN, DEFAULT_START_INTENSITY_MIN, false);
        setValue(FN_START_INTENSITY_MAX, DEFAULT_START_INTENSITY_MAX, false);
        setValue(FN_INITIAL_LARGE_INTENSITY_STEP, DEFAULT_INITIAL_LARGE_INTENSITY_STEP, false);
        setValue(FN_MAIN_SMALL_INTENSITY_STEP, DEFAULT_MAIN_SMALL_INTENSITY_STEP, false);
        setValue(FN_NUM_TRIALS_IN_MAIN_SEQUENCE, DEFAULT_NUM_TRIALS_IN_MAIN_SEQUENCE, false);
        setValue(FN_P_CATCH_TRIAL, DEFAULT_P_CATCH_TRIAL, false);
        setValue(FN_ITI_S, DEFAULT_ITI_S, false);
    }

    // Internal data
    m_current_trial = -1;
    m_current_trial_ignoring_catch_trials = -1;
    m_trial_last_y_b4_first_n = -1;
}


CardinalExpDetThreshold::~CardinalExpDetThreshold()
{
    // Necessary: for rationale, see QuAudioPlayer::~QuAudioPlayer()
    soundfunc::finishMediaPlayer(m_player_background);
    soundfunc::finishMediaPlayer(m_player_target);
}


// ============================================================================
// Class info
// ============================================================================

QString CardinalExpDetThreshold::shortname() const
{
    return "Cardinal_ExpDetThreshold";
}


QString CardinalExpDetThreshold::longname() const
{
    return tr("Cardinal RN — ExpDet-Threshold task");
}


QString CardinalExpDetThreshold::description() const
{
    return tr("Rapid assessment of auditory/visual thresholds "
              "(for expectation–detection task).");
}


// ============================================================================
// Ancillary management
// ============================================================================

QStringList CardinalExpDetThreshold::ancillaryTables() const
{
    return QStringList{CardinalExpDetThresholdTrial::TRIAL_TABLENAME};
}


QString CardinalExpDetThreshold::ancillaryTableFKToTaskFieldname() const
{
    return CardinalExpDetThresholdTrial::FN_FK_TO_TASK;
}


void CardinalExpDetThreshold::loadAllAncillary(const int pk)
{
    const OrderBy order_by{{CardinalExpDetThresholdTrial::FN_TRIAL, true}};
    ancillaryfunc::loadAncillary<CardinalExpDetThresholdTrial,
                                 CardinalExpDetThresholdTrialPtr>(
                m_trials, m_app, m_db,
                CardinalExpDetThresholdTrial::FN_FK_TO_TASK, order_by, pk);
}


QVector<DatabaseObjectPtr> CardinalExpDetThreshold::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>{
        CardinalExpDetThresholdTrialPtr(new CardinalExpDetThresholdTrial(m_app, m_db)),
    };
}


QVector<DatabaseObjectPtr> CardinalExpDetThreshold::getAllAncillary() const
{
    QVector<DatabaseObjectPtr> ancillaries;
    for (const CardinalExpDetThresholdTrialPtr& trial : m_trials) {
        ancillaries.append(trial);
    }
    return ancillaries;
}


// ============================================================================
// Instance info
// ============================================================================

bool CardinalExpDetThreshold::isComplete() const
{
    return valueBool(FN_FINISHED);
}


QStringList CardinalExpDetThreshold::summary() const
{
    return QStringList{
        QString("Target: <b>%1</b>.").arg(getTargetName()),
        QString("x75 [intensity for which p(detect) = 0.75]: <b>%1</b>").arg(
                    convert::prettyValue(x75(), DP)),
    };
}


QStringList CardinalExpDetThreshold::detail() const
{
    QStringList lines = completenessInfo() + summary();
    lines.append("\n");
    lines += recordSummaryLines();
    lines.append("\n");
    lines.append("Trials:");
    for (const CardinalExpDetThresholdTrialPtr& trial : m_trials) {
        lines.append(trial->recordSummaryCSVString());
    }
    lines.append("\n");
    LogisticDescriptives ld = calculateFit();
    lines.append(QString("Logistic parameters, recalculated now: intercept=%1,"
                         " slope=%2").arg(ld.intercept()).arg(ld.slope()));
    return lines;
}


OpenableWidget* CardinalExpDetThreshold::editor(const bool read_only)
{
    // ------------------------------------------------------------------------
    // OK to edit?
    // ------------------------------------------------------------------------
    if (read_only) {
        qWarning() << "Task not editable! Shouldn't have got here.";
        return nullptr;
    }

    // ------------------------------------------------------------------------
    // Configure the task using a Questionnaire
    // ------------------------------------------------------------------------

    const QString TX_CONFIG_TITLE(tr("Configure ExpDetThreshold task"));
    const NameValueOptions modality_options{
        {txtAuditory(), MODALITY_AUDITORY},
        {txtVisual(), MODALITY_VISUAL},
    };
    const NameValueOptions target_options_auditory{
        {ExpDetTextConst::auditoryTarget0(), 0},
        {ExpDetTextConst::auditoryTarget1(), 1},
    };
    const NameValueOptions target_options_visual{
        {ExpDetTextConst::visualTarget0(), 0},
        {ExpDetTextConst::visualTarget1(), 1},
    };

    // const int no_max = std::numeric_limits<int>::max();
    const QString warning_min_max(tr(
            "WARNING: cannot proceed: must satisfy "
            "min start intensity <= max start intensity"));

    auto text = [](const QString& t) -> QuElement* {
        return new QuText(t);
    };
    auto boldtext = [](const QString& t) -> QuElement* {
        return (new QuText(t))->setBold(true);
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options) -> QuElement* {
        return new QuMcq(fieldRef(fieldname), options);
    };

    QuPagePtr page1((new QuPage{
        boldtext(tr(
            "Set your device’s brightness and volume BEFORE running this "
            "task, and DO NOT ALTER THEM in between runs or before completing "
            "the main Expectation–Detection task. Also, try to keep the "
            "lighting and background noise constant throughout."
        )),
        text(tr(
            "Before you run the Expectation–Detection task for a given "
            "subject, please run this task FOUR times to determine the "
            "subject’s threshold for each of two auditory stimuli (tone, "
            "voice) and each of two auditory stimuli (circle, word)."
        )),
        text(tr(
            "Then, make a note of the 75% (“x75”) threshold intensities for "
            "each stimulus, and start the Expectation–Detection task (which "
            "only needs to be run once). It will ask you for these four "
            "intensities."
        )),
        boldtext(tr("Choose a modality:")),
        mcq(FN_MODALITY, modality_options),
    })->setTitle(TX_CONFIG_TITLE + " (1)"));

    QuPagePtr page2((new QuPage{
        boldtext(tr("Choose a target stimulus:")),
        mcq(FN_TARGET_NUMBER, target_options_auditory)->addTag(TAG_AUDITORY),
        mcq(FN_TARGET_NUMBER, target_options_visual)->addTag(TAG_VISUAL),
    })->setTitle(TX_CONFIG_TITLE + " (2)")->addTag(TAG_P2));

    const qreal zero = 0.0;
    const qreal one = 1.0;

    QuPagePtr page3((new QuPage{
        text(tr("Intensities and probabilities are in the range 0–1.")),
        questionnairefunc::defaultGridRawPointer({
            {ExpDetTextConst::configVisualTargetDurationS(),
             new QuLineEditDouble(fieldRef(FN_VISUAL_TARGET_DURATION_S), 0.1, 10.0)},
            {tr("Background intensity (usually 1.0)"),
             new QuLineEditDouble(fieldRef(FN_BACKGROUND_INTENSITY), zero, one)},
            {tr("Minimum starting intensity (e.g. 0.9)"),
             new QuLineEditDouble(fieldRef(FN_START_INTENSITY_MIN), zero, one)},
            {tr("Maximum starting intensity (e.g. 1.0)"),
             new QuLineEditDouble(fieldRef(FN_START_INTENSITY_MAX), zero, one)},
            {tr("Initial, large, intensity step (e.g. 0.1)"),
             new QuLineEditDouble(fieldRef(FN_INITIAL_LARGE_INTENSITY_STEP), zero, one)},
            {tr("Main, small, intensity step (e.g. 0.01)"),
             new QuLineEditDouble(fieldRef(FN_MAIN_SMALL_INTENSITY_STEP), zero, one)},
            {tr("Number of trials in the main test sequence (e.g. 14)"),
             new QuLineEditInteger(fieldRef(FN_NUM_TRIALS_IN_MAIN_SEQUENCE), 0, 100)},
            {tr("Probability of a catch trial (e.g. 0.2)"),
             new QuLineEditDouble(fieldRef(FN_P_CATCH_TRIAL), zero, one)},
            {tr("Intertrial interval (s) (e.g. 0.2)"),
             new QuLineEditDouble(fieldRef(FN_ITI_S), zero, 100.0)},
        }),
        (new QuText(warning_min_max))
                        ->setWarning(true)
                        ->addTag(TAG_WARNING_MIN_MAX),
    })->setTitle(TX_CONFIG_TITLE + " (3)")->addTag(TAG_P3));

    m_questionnaire = new Questionnaire(m_app, {page1, page2, page3});
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);
    m_questionnaire->setWithinChain(true);  // fast forward button, not stop

    connect(fieldRef(FN_MODALITY).data(), &FieldRef::valueChanged,
            this, &CardinalExpDetThreshold::validateQuestionnaire);
    connect(fieldRef(FN_START_INTENSITY_MIN).data(), &FieldRef::valueChanged,
            this, &CardinalExpDetThreshold::validateQuestionnaire);
    connect(fieldRef(FN_START_INTENSITY_MAX).data(), &FieldRef::valueChanged,
            this, &CardinalExpDetThreshold::validateQuestionnaire);

    connect(m_questionnaire.data(), &Questionnaire::cancelled,
            this, &CardinalExpDetThreshold::abort);
    connect(m_questionnaire.data(), &Questionnaire::completed,
            this, &CardinalExpDetThreshold::startTask);
    // Because our main m_widget isn't itself a questionnaire, we need to hook
    // up these, too:
    questionnairefunc::connectQuestionnaireToTask(m_questionnaire.data(), this);

    validateQuestionnaire();

    // ------------------------------------------------------------------------
    // If the config questionnaire is successful, we'll launch the main task;
    // prepare this too.
    // ------------------------------------------------------------------------

    m_scene = new QGraphicsScene(SCENE_RECT);
    m_scene->setBackgroundBrush(QBrush(SCENE_BACKGROUND));
    m_graphics_widget = makeGraphicsWidget(m_scene, SCENE_BACKGROUND,
                                           true, true);
    connect(m_graphics_widget.data(), &OpenableWidget::aborting,
            this, &CardinalExpDetThreshold::abort);

    m_widget = new OpenableWidget();

    // We start off by seeing the questionnaire:
    m_widget->setWidgetAsOnlyContents(m_questionnaire, 0, false, false);

    return m_widget;
}


// ============================================================================
// Config questionnaire internals
// ============================================================================

void CardinalExpDetThreshold::validateQuestionnaire()
{
    if (!m_questionnaire) {
        return;
    }

    // 1. Validation
    QVector<QuPage*> pages = m_questionnaire->getPages(false, TAG_P3);
    Q_ASSERT(pages.size() == 1);
    QuPage* page3 = pages.at(0);
    const bool duff_minmax = valueDouble(FN_START_INTENSITY_MAX) <
            valueDouble(FN_START_INTENSITY_MIN);
    m_questionnaire->setVisibleByTag(TAG_WARNING_MIN_MAX, duff_minmax,
                                     false, TAG_P3);
    page3->blockProgress(duff_minmax);

    // 2. Choice of target
    const bool auditory = isAuditory();
    m_questionnaire->setVisibleByTag(TAG_AUDITORY, auditory, false, TAG_P2);
    m_questionnaire->setVisibleByTag(TAG_VISUAL, !auditory, false, TAG_P2);
}


// ============================================================================
// Connection macros
// ============================================================================

// MUST USE Qt::QueuedConnection - see comments in clearScene()
#define CONNECT_BUTTON(b, funcname) \
    connect((b).button, &QPushButton::clicked, \
            this, &CardinalExpDetThreshold::funcname, \
            Qt::QueuedConnection)
// To use a Qt::ConnectionType parameter with a functor, we need a context
// See http://doc.qt.io/qt-5/qobject.html#connect-5
// That's the reason for the extra "this":
#define CONNECT_BUTTON_PARAM(b, funcname, param) \
    connect((b).button, &QPushButton::clicked, \
            this, std::bind(&CardinalExpDetThreshold::funcname, this, param), \
            Qt::QueuedConnection)


// ============================================================================
// Calculation/assistance functions for main task
// ============================================================================

QString CardinalExpDetThreshold::getDescriptiveModality() const
{
    const QVariant modality = value(FN_MODALITY);
    // can't use external constants in a switch statement
    if (modality.isNull()) {
        return TextConst::unknown();
    }
    if (modality.toInt() == MODALITY_AUDITORY) {
        return txtAuditory();
    }
    if (modality.toInt() == MODALITY_VISUAL) {
        return txtVisual();
    }
    return TextConst::unknown();
}


QString CardinalExpDetThreshold::getTargetName() const
{
    const QVariant modality = value(FN_MODALITY);
    const QVariant target_number = value(FN_TARGET_NUMBER);
    if (modality.isNull() || target_number.isNull()) {
        return TextConst::unknown();
    }
    if (modality.toInt() == MODALITY_AUDITORY) {
        switch (target_number.toInt()) {
        case 0:
            return ExpDetTextConst::auditoryTarget0();
        case 1:
            return ExpDetTextConst::auditoryTarget1();
        }
    } else if (modality.toInt() == MODALITY_VISUAL) {
        switch (target_number.toInt()) {
        case 0:
            return ExpDetTextConst::visualTarget0();
        case 1:
            return ExpDetTextConst::visualTarget1();
        }
    }
    return TextConst::unknown();
}


QVariant CardinalExpDetThreshold::x(const qreal p) const
{
    if (valueIsNull(FN_INTERCEPT) || valueIsNull(FN_SLOPE)) {
        return QVariant();
    }
    const qreal intercept = valueDouble(FN_INTERCEPT);
    const qreal slope = valueDouble(FN_SLOPE);
    LogisticDescriptives ld(intercept, slope);  // coefficients already known
    return ld.x(p);
}


QVariant CardinalExpDetThreshold::x75() const
{
    return x(0.75);
}


bool CardinalExpDetThreshold::haveWeJustReset() const
{
    const int last_trial = m_current_trial - 1;
    if (last_trial < 0 || last_trial >= m_trials.size()) {
        return false;
    }
    return m_trials.at(last_trial)->wasCaughtOutReset();
}


bool CardinalExpDetThreshold::inInitialStepPhase() const
{
    return m_trial_last_y_b4_first_n < 0;
}


bool CardinalExpDetThreshold::lastTrialWasFirstNo() const
{
    if (m_trial_last_y_b4_first_n < 0 || m_current_trial < 0) {
        return false;
    }
    return (
        m_trials.at(m_current_trial)->trialNumIgnoringCatchTrials() ==
            m_trials.at(m_trial_last_y_b4_first_n)->trialNumIgnoringCatchTrials()
                + 2
    );
}


int CardinalExpDetThreshold::getNBackNonCatchTrialIndex(
        const int n, const int start_index) const
{
    Q_ASSERT(start_index >= 0 && start_index < m_trials.size());
    const int target = m_trials.at(start_index)->trialNumIgnoringCatchTrials() - n;
    for (int i = 0; i < m_trials.size(); ++i) {
        const CardinalExpDetThresholdTrialPtr& t = m_trials.at(i);
        if (t->targetPresented() && t->trialNumIgnoringCatchTrials() == target) {
            return i;
        }
    }
    return -1;
}


qreal CardinalExpDetThreshold::getIntensity() const
{
    Q_ASSERT(m_current_trial >= 0 && m_current_trial < m_trials.size());
    const qreal fail = -1.0;
    const CardinalExpDetThresholdTrialPtr& t = m_trials.at(m_current_trial);
    if (!t->targetPresented()) {
        return fail;
    }
    if (t->trialNum() == 0 || haveWeJustReset()) {
        // First trial, or we've just reset
        return randomRealIncUpper(valueDouble(FN_START_INTENSITY_MIN),
                                  valueDouble(FN_START_INTENSITY_MAX));
    }
    const int one_back = getNBackNonCatchTrialIndex(1, m_current_trial);
    Q_ASSERT(one_back >= 0);
    const CardinalExpDetThresholdTrialPtr& prev = m_trials.at(one_back);
    if (inInitialStepPhase()) {
        return prev->intensity() - valueDouble(FN_INITIAL_LARGE_INTENSITY_STEP);
    }
    if (lastTrialWasFirstNo()) {
        const int two_back = getNBackNonCatchTrialIndex(2, m_current_trial);
        Q_ASSERT(two_back >= 0);
        const CardinalExpDetThresholdTrialPtr& tb = m_trials.at(two_back);
        return mean(prev->intensity(), tb->intensity());
    }
    if (prev->yes()) {
        // In main phase. Detected stimulus last time; make it harder
        return prev->intensity() - valueDouble(FN_MAIN_SMALL_INTENSITY_STEP);
    }
    // In main phase. Didn't detect stimulus last time; make it easier
    return prev->intensity() + valueDouble(FN_MAIN_SMALL_INTENSITY_STEP);
}


bool CardinalExpDetThreshold::wantCatchTrial(const int trial_num) const
{
    Q_ASSERT(trial_num - 1 < m_trials.size());
    if (trial_num <= 0) {
        return false;  // never on the first
    }
    if (m_trials.at(trial_num - 1)->wasCaughtOutReset()) {
        return false;  // never immediately after a reset
    }
    if (trial_num == 1) {
        return true;  // always on the second
    }
    if (m_trials.at(trial_num - 2)->wasCaughtOutReset()) {
        return true;  // always on the second of a fresh run
    }
    return coin(valueDouble(FN_P_CATCH_TRIAL));  // otherwise on e.g. 20% of trials
}


bool CardinalExpDetThreshold::isAuditory() const
{
    return valueInt(FN_MODALITY) == MODALITY_AUDITORY;
}


bool CardinalExpDetThreshold::timeToStop() const
{
    if (m_trial_last_y_b4_first_n < 0) {
        return false;
    }
    const int final_trial_ignoring_catch_trials =
            m_trials[m_trial_last_y_b4_first_n]->trialNumIgnoringCatchTrials()
            + valueInt(FN_NUM_TRIALS_IN_MAIN_SEQUENCE) - 1;
    return m_trials[m_current_trial]->trialNumIgnoringCatchTrials() >=
            final_trial_ignoring_catch_trials;
}


void CardinalExpDetThreshold::clearScene()
{
    m_scene->clear();
}


void CardinalExpDetThreshold::setTimeout(const int time_ms, FuncPtr callback)
{
    m_timer->stop();
    m_timer->disconnect();
    connect(m_timer.data(), &QTimer::timeout,
            this, callback,
            Qt::QueuedConnection);
    m_timer->start(time_ms);
}


void CardinalExpDetThreshold::showVisualStimulus(const QString& filename_stem,
                                                 const qreal intensity)
{
    QString filename = cardinalexpdetcommon::filenameFromStem(filename_stem);
    makeImage(m_scene, VISUAL_STIM_RECT, filename, intensity);
}


void CardinalExpDetThreshold::savingWait()
{
    clearScene();
    makeText(m_scene, SCENE_CENTRE, BASE_TEXT_CONFIG, TextConst::saving());
}


void CardinalExpDetThreshold::reset()
{
    Q_ASSERT(m_current_trial >= 0 && m_current_trial < m_trials.size());
    m_trials.at(m_current_trial)->setCaughtOutReset();
    m_trial_last_y_b4_first_n = -1;
}


void CardinalExpDetThreshold::labelTrialsForAnalysis()
{
    // Trial numbers in the calculation sequence start from 1.
    DbNestableTransaction trans(m_db);
    int tnum = 1;
    for (int i = 0; i < m_trials.size(); ++i) {
        const CardinalExpDetThresholdTrialPtr& t = m_trials.at(i);
        QVariant trial_num_in_seq;  // NULL
        if (i >= m_trial_last_y_b4_first_n && t->targetPresented()) {
            trial_num_in_seq = tnum++;
        }
        t->setTrialNumInCalcSeq(trial_num_in_seq);
    }
}


LogisticDescriptives CardinalExpDetThreshold::calculateFit() const
{
    QVector<double> intensity;  // predictor
    QVector<int> choice;  // dependent variable
    for (const CardinalExpDetThresholdTrialPtr& tp : m_trials) {
        if (tp->isInCalculationSeq()) {
            intensity.append(tp->intensity());
            choice.append(tp->yes() ? 1 : 0);
        }
    }
    qInfo() << "Calculating regression:";
    qInfo() << "Intensities:" << intensity;
    qInfo() << "Choices:" << choice;
    if (intensity.isEmpty()) {
        qWarning() << "No trials found for calculateFit()";
    }
    LogisticDescriptives ld(intensity, choice);  // fit the regression
    return ld;
}


void CardinalExpDetThreshold::calculateAndStoreFit()
{
    const LogisticDescriptives ld = calculateFit();
    qInfo().nospace() << "Coefficients: b0 (intercept) = " << ld.b0()
                      << ", b1 (slope) = " << ld.b1();
    setValue(FN_INTERCEPT, ld.intercept());
    setValue(FN_SLOPE, ld.slope());
    setValue(FN_K, ld.k());
    setValue(FN_THETA, ld.theta());
}


// ============================================================================
// Main task internals
// ============================================================================

void CardinalExpDetThreshold::startTask()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    m_widget->setWidgetAsOnlyContents(m_graphics_widget, 0, false, false);
    onEditStarted();  // will have been stopped by the end of the questionnaire?

    // Finalize the parameters
    const QString TX_DETECTION_Q_VISUAL(tr("Did you see a"));
    const QString TX_DETECTION_Q_AUDITORY(tr("Did you hear a"));
    const bool auditory = isAuditory();
    if (auditory) {
        setValue(FN_BACKGROUND_FILENAME, AUDITORY_BACKGROUND);
        if (valueInt(FN_TARGET_NUMBER) == 0) {
            setValue(FN_TARGET_FILENAME, AUDITORY_TARGETS.at(0));
            setValue(FN_PROMPT, TX_DETECTION_Q_AUDITORY + " " +
                     ExpDetTextConst::auditoryTarget0Short() + "?");
        } else {
            setValue(FN_TARGET_FILENAME, AUDITORY_TARGETS.at(1));
            setValue(FN_PROMPT, TX_DETECTION_Q_AUDITORY + " " +
                     ExpDetTextConst::auditoryTarget1Short() + "?");
        }
    } else {
        setValue(FN_BACKGROUND_FILENAME, VISUAL_BACKGROUND);
        if (valueInt(FN_TARGET_NUMBER) == 0) {
            setValue(FN_TARGET_FILENAME, VISUAL_TARGETS.at(0));
            setValue(FN_PROMPT, TX_DETECTION_Q_VISUAL + " " +
                     ExpDetTextConst::visualTarget0Short() + "?");
        } else {
            setValue(FN_TARGET_FILENAME, VISUAL_TARGETS.at(1));
            setValue(FN_PROMPT, TX_DETECTION_Q_VISUAL + " " +
                     ExpDetTextConst::visualTarget1Short() + "?");
        }
    }

    // Double-check we have a PK before we create trials
    save();

    // Set up players and timers
    soundfunc::makeMediaPlayer(m_player_background);
    soundfunc::makeMediaPlayer(m_player_target);
    if (!m_player_background || !m_player_target) {
        return;
    }
    connect(m_player_background.data(), &QMediaPlayer::mediaStatusChanged,
            this, &CardinalExpDetThreshold::mediaStatusChangedBackground);
    timerfunc::makeSingleShotTimer(m_timer);

    // Prep the sounds
    if (auditory) {
        m_player_background->setMedia(urlFromStem(
                                valueString(FN_BACKGROUND_FILENAME)));
        soundfunc::setVolume(m_player_background,
                             valueDouble(FN_BACKGROUND_INTENSITY));
        m_player_target->setMedia(urlFromStem(
                                valueString(FN_TARGET_FILENAME)));
        // Volume will be set later.
    }

    // Start
    ButtonAndProxy start = makeTextButton(
                m_scene, START_BUTTON_RECT, BASE_BUTTON_CONFIG,
                TextConst::touchToStart());
    CONNECT_BUTTON(start, nextTrial);
}


void CardinalExpDetThreshold::nextTrial()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    if (timeToStop()) {
        qDebug() << "Time to stop";
        savingWait();
        setValue(FN_FINISHED, true);  // will also be set by thanks() -> finish()
        labelTrialsForAnalysis();
        calculateAndStoreFit();
        save();
        thanks();
    } else {
        startTrial();
    }
}


void CardinalExpDetThreshold::startTrial()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif

    // Increment trial numbers; determine if it's a catch trial (on which no
    // stimulus is presented); create trial record
    ++m_current_trial;
    const bool want_catch = wantCatchTrial(m_current_trial);
    const bool present_target = !want_catch;
    if (!want_catch) {
        ++m_current_trial_ignoring_catch_trials;
    }
    const QVariant trial_ignoring_catch_trials = want_catch
            ? QVariant()  // NULL
            : m_current_trial_ignoring_catch_trials;
    CardinalExpDetThresholdTrialPtr tr(new CardinalExpDetThresholdTrial(
                                           pkvalueInt(),
                                           m_current_trial,  // zero-based trial number
                                           trial_ignoring_catch_trials,
                                           present_target,
                                           m_app,
                                           m_db));
    m_trials.append(tr);
    qDebug() << tr->summary();

    // Display stimulus
    const bool auditory = isAuditory();
    if (present_target) {
        // Now we've put the new trial in the vector, we can calculate intensity:
        const qreal intensity = qBound(0.0, getIntensity(), 1.0);
        // ... intensity is in the range [0, 1]
        tr->setIntensity(intensity);
        if (auditory) {
            soundfunc::setVolume(m_player_target, intensity);
            m_player_background->play();
            m_player_target->play();
        } else {
            showVisualStimulus(valueString(FN_BACKGROUND_FILENAME),
                               valueDouble(FN_BACKGROUND_INTENSITY));
            showVisualStimulus(valueString(FN_TARGET_FILENAME),
                               tr->intensity());
        }
    } else {
        // Catch trial
        if (auditory) {
            m_player_background->play();
        } else {
            showVisualStimulus(valueString(FN_BACKGROUND_FILENAME),
                               valueDouble(FN_BACKGROUND_INTENSITY));
        }
    }

    // If auditory, the event will be driven by the end of the sound, via
    // mediaStatusChangedBackground(). Otherwise:
    if (!auditory) {
        int stimulus_time_ms = msFromSec(
                    valueDouble(FN_VISUAL_TARGET_DURATION_S));
        setTimeout(stimulus_time_ms, &CardinalExpDetThreshold::offerChoice);
    }
}


void CardinalExpDetThreshold::mediaStatusChangedBackground(
        QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
#ifdef DEBUG_STEP_DETAIL
        qDebug() << "Background sound playback finished";
#endif
        m_player_target->stop();  // in case it's still playing
        offerChoice();
    }
}


void CardinalExpDetThreshold::offerChoice()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    Q_ASSERT(m_current_trial >= 0 && m_current_trial < m_trials.size());
    CardinalExpDetThresholdTrial& t = *m_trials.at(m_current_trial);
    clearScene();

    makeText(m_scene, PROMPT_CENTRE, BASE_TEXT_CONFIG, valueString(FN_PROMPT));
    ButtonAndProxy y = makeTextButton(m_scene, YES_BUTTON_RECT,
                                      BASE_BUTTON_CONFIG, TextConst::yes());
    ButtonAndProxy n = makeTextButton(m_scene, NO_BUTTON_RECT,
                                      BASE_BUTTON_CONFIG, TextConst::no());
    ButtonAndProxy a = makeTextButton(m_scene, ABORT_BUTTON_RECT,
                                      ABORT_BUTTON_CONFIG, TextConst::abort());
    CONNECT_BUTTON_PARAM(y, recordChoice, true);
    CONNECT_BUTTON_PARAM(n, recordChoice, false);
    CONNECT_BUTTON(a, abort);

    t.recordChoiceTime();
}


void CardinalExpDetThreshold::recordChoice(const bool yes)
{
    Q_ASSERT(m_current_trial >= 0 && m_current_trial < m_trials.size());
    CardinalExpDetThresholdTrial& t = *m_trials.at(m_current_trial);
    t.recordResponse(yes);
    if (!t.targetPresented() && yes) {
        // Caught out... reset.
        reset();
    } else if (m_current_trial == 0 && !yes) {
        // No on first trial -- treat as reset
        reset();
    } else if (t.targetPresented() && !yes && m_trial_last_y_b4_first_n < 0) {
        // First no
        m_trial_last_y_b4_first_n = getNBackNonCatchTrialIndex(1, m_current_trial);
        qDebug() << "First no response: m_trial_last_y_b4_first_n ="
                 << m_trial_last_y_b4_first_n;
    }
    clearScene();
    setTimeout(msFromSec(valueDouble(FN_ITI_S)),
               &CardinalExpDetThreshold::nextTrial);
}


void CardinalExpDetThreshold::thanks()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    ButtonAndProxy thx = makeTextButton(
                m_scene, THANKS_BUTTON_RECT, BASE_BUTTON_CONFIG,
                TextConst::thankYouTouchToExit());
    CONNECT_BUTTON(thx, finish);
}


void CardinalExpDetThreshold::abort()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    savingWait();
    setValue(FN_FINISHED, false);
    Q_ASSERT(m_widget);
    onEditFinishedAbort();  // will save
    emit m_widget->finished();
}


void CardinalExpDetThreshold::finish()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    setValue(FN_FINISHED, true);
    Q_ASSERT(m_widget);
    onEditFinishedProperly();  // will save
    emit m_widget->finished();
}


// ============================================================================
// Translatable text
// ============================================================================

QString CardinalExpDetThreshold::txtAuditory()
{
    return tr("Auditory");
}


QString CardinalExpDetThreshold::txtVisual()
{
    return tr("Visual");
}
