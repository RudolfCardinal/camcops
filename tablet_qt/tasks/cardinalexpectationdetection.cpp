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

// #define DEBUG_STEP_DETAIL

#include "cardinalexpectationdetection.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPushButton>
#include <QTimer>
#include "common/textconst.h"
#include "db/ancillaryfunc.h"
#include "db/dbnestabletransaction.h"
#include "lib/containers.h"
#include "lib/datetime.h"
#include "lib/soundfunc.h"
#include "lib/timerfunc.h"
#include "lib/uifunc.h"
#include "graphics/graphicsfunc.h"
#include "maths/ccrandom.h"
#include "maths/mathfunc.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskregistrar.h"
#include "taskxtra/cardinalexpdetcommon.h"
#include "taskxtra/cardinalexpdettrial.h"
#include "taskxtra/cardinalexpdettrialgroupspec.h"
using namespace cardinalexpdetcommon;  // lots...
using containers::rotateSequenceInPlace;
using datetime::msToSec;
using datetime::secToIntMs;
using datetime::secToMin;
using mathfunc::mean;
using graphicsfunc::ButtonAndProxy;
using graphicsfunc::makeImage;
using graphicsfunc::makeText;
using graphicsfunc::makeTextButton;


// ============================================================================
// Constants
// ============================================================================

const QString CardinalExpectationDetection::CARDINALEXPDET_TABLENAME(
        "cardinal_expdet");

// Fieldnames
const QString FN_NUM_BLOCKS("num_blocks");
const QString FN_STIMULUS_COUNTERBALANCING("stimulus_counterbalancing");
const QString FN_IS_DETECTION_RESPONSE_ON_RIGHT("is_detection_response_on_right");
const QString FN_PAUSE_EVERY_N_TRIALS("pause_every_n_trials");
const QString FN_CUE_DURATION_S("cue_duration_s");
const QString FN_VISUAL_CUE_INTENSITY("visual_cue_intensity");
const QString FN_AUDITORY_CUE_INTENSITY("auditory_cue_intensity");
const QString FN_ISI_DURATION_S("isi_duration_s");
const QString FN_VISUAL_TARGET_DURATION_S("visual_target_duration_s");
const QString FN_VISUAL_BACKGROUND_INTENSITY("visual_background_intensity");
const QString FN_VISUAL_TARGET_0_INTENSITY("visual_target_0_intensity");
const QString FN_VISUAL_TARGET_1_INTENSITY("visual_target_1_intensity");
const QString FN_AUDITORY_BACKGROUND_INTENSITY("auditory_background_intensity");
const QString FN_AUDITORY_TARGET_0_INTENSITY("auditory_target_0_intensity");
const QString FN_AUDITORY_TARGET_1_INTENSITY("auditory_target_1_intensity");
const QString FN_ITI_MIN_S("iti_min_s");
const QString FN_ITI_MAX_S("iti_max_s");
const QString FN_ABORTED("aborted");
const QString FN_FINISHED("finished");
const QString FN_LAST_TRIAL_COMPLETED("last_trial_completed");

// Text for user
const QString TX_CONFIG_TITLE("Configure Expectation–Detection task");
const QString TX_CONFIG_INSTRUCTIONS_1("You’ll need to set these parameters:");
const QString TX_CONFIG_INSTRUCTIONS_2(
        "Configure these based on the results of the ExpDetThreshold task. "
        "(DO NOT alter your tablet device’s brightness or volume, or the "
        "environmental lighting/noise conditions.)");
const QString TX_CONFIG_INSTRUCTIONS_3(
        "These parameters are less likely to need changing:");
const QString TX_CONFIG_STIMULUS_COUNTERBALANCING(
        "Stimulus counterbalancing (0–7):");
const QString TX_CONFIG_NUM_BLOCKS(
        "Number of trial blocks (24 trials/block):");
const QString TX_CONFIG_PAUSE_EVERY_N_TRIALS(
        "Pause every n trials (0 for no pausing):");
const QString TX_CONFIG_IS_DETECTION_RESPONSE_ON_RIGHT(
        "“Detection” responses are towards the right");
const QString TX_CONFIG_CUE_DURATION_S(
        "Cue duration (s) (cue is multimodal; auditory+visual):");
const QString TX_CONFIG_VISUAL_CUE_INTENSITY(
        "Visual cue intensity (0.0–1.0, usually 1.0):");
const QString TX_CONFIG_AUDITORY_CUE_INTENSITY(
        "Auditory cue intensity (0.0–1.0, usually 1.0):");
const QString TX_CONFIG_ISI_DURATION_S(
        "Interstimulus interval (ISI) (s) (e.g. 0.2):");
const QString TX_CONFIG_VISUAL_BACKGROUND_INTENSITY(
        "Visual background intensity (0.0–1.0, usually 1.0):");
const QString TX_CONFIG_INTENSITY_PREFIX("Intensity (0.0–1.0) for:");
const QString TX_CONFIG_AUDITORY_BACKGROUND_INTENSITY(
        "Auditory background intensity (0.0–1.0, usually 1.0):");
const QString TX_CONFIG_ITI_MIN_S(
        "Intertrial interval (ITI) minimum duration (s):");
const QString TX_CONFIG_ITI_MAX_S(
        "Intertrial interval (ITI) maximum duration (s):");
const QString TX_INSTRUCTIONS_1(
        "Please ensure you can see and hear this tablet/computer clearly.");
const QString TX_INSTRUCTIONS_2(
        "The experimenter will assist you with any headphones required.");
const QString TX_INSTRUCTIONS_3(
        "Once you have started the task, please follow the instructions that "
        "appear on the screen.");
const QString TX_DETECTION_Q_PREFIX("Did you");
const QString TX_DETECTION_Q_VISUAL("see a");
const QString TX_DETECTION_Q_AUDITORY("hear a");
const QString TX_CONTINUE_WHEN_READY(
        "When you’re ready, touch here to continue.");
const QString TX_NUM_TRIALS_LEFT("Number of trials to go:");
const QString TX_TIME_LEFT("Estimated time left (minutes):");
const QString TX_POINTS("Your score on this trial was:");
const QString TX_CUMULATIVE_POINTS("Your total score so far is:");

// Default values:
const int DEFAULT_NUM_BLOCKS = 8;
const bool DEFAULT_IS_DETECTION_RESPONSE_ON_RIGHT = true;
const int DEFAULT_PAUSE_EVERY_N_TRIALS = 20;
// ... cue
const qreal DEFAULT_CUE_DURATION_S = 1.0;
const qreal DEFAULT_VISUAL_CUE_INTENSITY = 1.0;
const qreal DEFAULT_AUDITORY_CUE_INTENSITY = 1.0;
// ... ISI
const qreal DEFAULT_ISI_DURATION_S = 0.2;
// ... target
const qreal DEFAULT_VISUAL_TARGET_DURATION_S = 1.0; // to match auditory
const qreal DEFAULT_VISUAL_BACKGROUND_INTENSITY = 1.0;
const qreal DEFAULT_AUDITORY_BACKGROUND_INTENSITY = 1.0;
// ... ITI
const qreal DEFAULT_ITI_MIN_S = 0.2;
const qreal DEFAULT_ITI_MAX_S = 0.8;

// Other task constants
const int N_TRIAL_GROUPS = 8;

// Graphics
const qreal PROMPT_X(0.5 * SCENE_WIDTH);
const QPointF PROMPT_1(PROMPT_X, 0.20 * SCENE_HEIGHT);
const QPointF PROMPT_2(PROMPT_X, 0.25 * SCENE_HEIGHT);
// const QPointF PROMPT_3(PROMPT_X, 0.30 * SCENE_HEIGHT);
const QRectF START_BTN_RECT(0.2 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                            0.6 * SCENE_WIDTH, 0.1 * SCENE_HEIGHT);
const QRectF CONTINUE_BTN_RECT(0.3 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                               0.4 * SCENE_WIDTH, 0.2 * SCENE_HEIGHT);
const QRectF CANCEL_ABORT_RECT(0.2 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                               0.2 * SCENE_WIDTH, 0.2 * SCENE_HEIGHT);
const QRectF REALLY_ABORT_RECT(0.6 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                               0.2 * SCENE_WIDTH, 0.2 * SCENE_HEIGHT);


// ============================================================================
// Factory method
// ============================================================================

void initializeCardinalExpectationDetection(TaskFactory& factory)
{
    static TaskRegistrar<CardinalExpectationDetection> registered(factory);
}


// ============================================================================
// CardinalExpectationDetection
// ============================================================================

CardinalExpectationDetection::CardinalExpectationDetection(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CARDINALEXPDET_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    // Config
    addField(FN_NUM_BLOCKS, QVariant::Int);
    addField(FN_STIMULUS_COUNTERBALANCING, QVariant::Int);
    addField(FN_IS_DETECTION_RESPONSE_ON_RIGHT, QVariant::Bool);
    addField(FN_PAUSE_EVERY_N_TRIALS, QVariant::Int);
    // ... cue
    addField(FN_CUE_DURATION_S, QVariant::Double);
    addField(FN_VISUAL_CUE_INTENSITY, QVariant::Double);
    addField(FN_AUDITORY_CUE_INTENSITY, QVariant::Double);
    // ... ISI
    addField(FN_ISI_DURATION_S, QVariant::Double);
    // ... target
    addField(FN_VISUAL_TARGET_DURATION_S, QVariant::Double);
    addField(FN_VISUAL_BACKGROUND_INTENSITY, QVariant::Double);  // 0 to 1
    addField(FN_VISUAL_TARGET_0_INTENSITY, QVariant::Double);  // 0 to 1
    addField(FN_VISUAL_TARGET_1_INTENSITY, QVariant::Double);  // 0 to 1
    addField(FN_AUDITORY_BACKGROUND_INTENSITY, QVariant::Double);  // 0 to 1
    addField(FN_AUDITORY_TARGET_0_INTENSITY, QVariant::Double);  // 0 to 1
    addField(FN_AUDITORY_TARGET_1_INTENSITY, QVariant::Double);  // 0 to 1
    // ... ITI
    addField(FN_ITI_MIN_S, QVariant::Double);
    addField(FN_ITI_MAX_S, QVariant::Double);
    // Results:
    addField(FN_ABORTED, QVariant::Bool, false, false, false, false);
    addField(FN_FINISHED, QVariant::Bool, false, false, false, false);
    addField(FN_LAST_TRIAL_COMPLETED, QVariant::Int);

    load(load_pk);

    if (load_pk == dbconst::NONEXISTENT_PK) {
        // Default values:
        setValue(FN_NUM_BLOCKS, DEFAULT_NUM_BLOCKS, false);
        setValue(FN_IS_DETECTION_RESPONSE_ON_RIGHT, DEFAULT_IS_DETECTION_RESPONSE_ON_RIGHT, false);
        setValue(FN_PAUSE_EVERY_N_TRIALS, DEFAULT_PAUSE_EVERY_N_TRIALS, false);
        setValue(FN_CUE_DURATION_S, DEFAULT_CUE_DURATION_S, false);
        setValue(FN_VISUAL_CUE_INTENSITY, DEFAULT_VISUAL_CUE_INTENSITY, false);
        setValue(FN_AUDITORY_CUE_INTENSITY, DEFAULT_AUDITORY_CUE_INTENSITY, false);
        setValue(FN_ISI_DURATION_S, DEFAULT_ISI_DURATION_S, false);
        setValue(FN_VISUAL_TARGET_DURATION_S, DEFAULT_VISUAL_TARGET_DURATION_S, false);
        setValue(FN_VISUAL_BACKGROUND_INTENSITY, DEFAULT_VISUAL_BACKGROUND_INTENSITY, false);
        setValue(FN_AUDITORY_BACKGROUND_INTENSITY, DEFAULT_AUDITORY_BACKGROUND_INTENSITY, false);
        setValue(FN_ITI_MIN_S, DEFAULT_ITI_MIN_S, false);
        setValue(FN_ITI_MAX_S, DEFAULT_ITI_MAX_S, false);
    }

    // Internal data
    m_current_trial = -1;
}


CardinalExpectationDetection::~CardinalExpectationDetection()
{
    // Necessary: for rationale, see QuAudioPlayer::~QuAudioPlayer()
    soundfunc::finishMediaPlayer(m_player_cue);
    soundfunc::finishMediaPlayer(m_player_background);
    soundfunc::finishMediaPlayer(m_player_target_0);
    soundfunc::finishMediaPlayer(m_player_target_1);
}


// ============================================================================
// Class info
// ============================================================================

QString CardinalExpectationDetection::shortname() const
{
    return "Cardinal_ExpDet";
}


QString CardinalExpectationDetection::longname() const
{
    return tr("Cardinal RN — Expectation–Detection task");
}


QString CardinalExpectationDetection::description() const
{
    return tr("Putative assay of proneness to hallucinations.");
}


// ============================================================================
// Ancillary management
// ============================================================================

QStringList CardinalExpectationDetection::ancillaryTables() const
{
    return QStringList{CardinalExpDetTrialGroupSpec::GROUPSPEC_TABLENAME,
                       CardinalExpDetTrial::TRIAL_TABLENAME};
}


QString CardinalExpectationDetection::ancillaryTableFKToTaskFieldname() const
{
    Q_ASSERT(CardinalExpDetTrialGroupSpec::FN_FK_TO_TASK == CardinalExpDetTrialGroupSpec::FN_FK_TO_TASK);
    return CardinalExpDetTrial::FN_FK_TO_TASK;
}


void CardinalExpectationDetection::loadAllAncillary(const int pk)
{
    const OrderBy group_order_by{{CardinalExpDetTrialGroupSpec::FN_GROUP_NUM, true}};
    ancillaryfunc::loadAncillary<CardinalExpDetTrialGroupSpec, CardinalExpDetTrialGroupSpecPtr>(
                m_groups, m_app, m_db,
                CardinalExpDetTrialGroupSpec::FN_FK_TO_TASK, group_order_by, pk);
    const OrderBy trial_order_by{{CardinalExpDetTrial::FN_TRIAL, true}};
    ancillaryfunc::loadAncillary<CardinalExpDetTrial, CardinalExpDetTrialPtr>(
                m_trials, m_app, m_db,
                CardinalExpDetTrial::FN_FK_TO_TASK, trial_order_by, pk);
}


QVector<DatabaseObjectPtr> CardinalExpectationDetection::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>{
        CardinalExpDetTrialGroupSpecPtr(new CardinalExpDetTrialGroupSpec(m_app, m_db)),
        CardinalExpDetTrialPtr(new CardinalExpDetTrial(m_app, m_db)),
    };
}


QVector<DatabaseObjectPtr> CardinalExpectationDetection::getAllAncillary() const
{
    QVector<DatabaseObjectPtr> ancillaries;
    for (const CardinalExpDetTrialGroupSpecPtr& group : m_groups) {
        ancillaries.append(group);
    }
    for (const CardinalExpDetTrialPtr& trial : m_trials) {
        ancillaries.append(trial);
    }
    return ancillaries;
}


// ============================================================================
// Instance info
// ============================================================================

bool CardinalExpectationDetection::isComplete() const
{
    return valueBool(FN_FINISHED);
}


QStringList CardinalExpectationDetection::summary() const
{
    QStringList lines;
    const int n_trials = m_trials.length();
    int completed_trials = 0;
    for (int i = 0; i < n_trials; ++i) {
        if (m_trials.at(i)->responded()) {
            ++completed_trials;
        }
    }
    lines.append(QString("Performed %1 trial(s).").arg(completed_trials));
    return lines;
}


QStringList CardinalExpectationDetection::detail() const
{
    QStringList lines = completenessInfo() + recordSummaryLines();
    lines.append("\n");
    lines.append("Group specifications:");
    for (const CardinalExpDetTrialGroupSpecPtr& group : m_groups) {
        lines.append(group->recordSummaryCSVString());
    }
    lines.append("\n");
    lines.append("Trials:");
    for (const CardinalExpDetTrialPtr& trial : m_trials) {
        lines.append(trial->recordSummaryCSVString());
    }
    return lines;
}


OpenableWidget* CardinalExpectationDetection::editor(const bool read_only)
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
    auto boldtext = [](const QString& text) -> QuElement* {
        return new QuText(text);
    };

    const int INTENSITY_DP = 3;
    const int TIME_DP = 1;

    QuPagePtr page((new QuPage{
        boldtext(TX_CONFIG_INSTRUCTIONS_1),
        questionnairefunc::defaultGridRawPointer({
            {TX_CONFIG_STIMULUS_COUNTERBALANCING,
             new QuLineEditInteger(fieldRef(FN_STIMULUS_COUNTERBALANCING),
                                   0, N_CUES_PER_MODALITY - 1)},
        }),
        boldtext(TX_CONFIG_INSTRUCTIONS_2),
        questionnairefunc::defaultGridRawPointer({
            {TX_CONFIG_INTENSITY_PREFIX + ExpDetTextConst::auditoryTarget0(),
             new QuLineEditDouble(fieldRef(FN_AUDITORY_TARGET_0_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY)},
            {TX_CONFIG_INTENSITY_PREFIX + ExpDetTextConst::auditoryTarget1(),
             new QuLineEditDouble(fieldRef(FN_AUDITORY_TARGET_1_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY)},
            {TX_CONFIG_INTENSITY_PREFIX + ExpDetTextConst::visualTarget0(),
             new QuLineEditDouble(fieldRef(FN_VISUAL_TARGET_0_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY)},
            {TX_CONFIG_INTENSITY_PREFIX + ExpDetTextConst::visualTarget1(),
             new QuLineEditDouble(fieldRef(FN_VISUAL_TARGET_1_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY)},
        }),
        boldtext(TX_CONFIG_INSTRUCTIONS_3),
        questionnairefunc::defaultGridRawPointer({
            {TX_CONFIG_NUM_BLOCKS,
             new QuLineEditInteger(fieldRef(FN_NUM_BLOCKS), 1, 100)},
            {TX_CONFIG_ITI_MIN_S,
             new QuLineEditDouble(fieldRef(FN_ITI_MIN_S),
                                  0.1, 100.0, TIME_DP)},
            {TX_CONFIG_ITI_MAX_S,
             new QuLineEditDouble(fieldRef(FN_ITI_MAX_S),
                                  0.1, 100.0, TIME_DP)},
            {TX_CONFIG_PAUSE_EVERY_N_TRIALS,
             new QuLineEditInteger(fieldRef(FN_PAUSE_EVERY_N_TRIALS), 0, 100)},
            {TX_CONFIG_CUE_DURATION_S,
             new QuLineEditDouble(fieldRef(FN_CUE_DURATION_S),
                                  0.1, 10.0, TIME_DP)},
            {TX_CONFIG_VISUAL_CUE_INTENSITY,
             new QuLineEditDouble(fieldRef(FN_VISUAL_CUE_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY, INTENSITY_DP)},
            {TX_CONFIG_AUDITORY_CUE_INTENSITY,
             new QuLineEditDouble(fieldRef(FN_AUDITORY_CUE_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY, INTENSITY_DP)},
            {ExpDetTextConst::configVisualTargetDurationS(),
             new QuLineEditDouble(fieldRef(FN_VISUAL_TARGET_DURATION_S),
                                  0.1, 10.0, TIME_DP)},
            {TX_CONFIG_VISUAL_BACKGROUND_INTENSITY,
             new QuLineEditDouble(fieldRef(FN_VISUAL_BACKGROUND_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY, INTENSITY_DP)},
            {TX_CONFIG_AUDITORY_BACKGROUND_INTENSITY,
             new QuLineEditDouble(fieldRef(FN_AUDITORY_BACKGROUND_INTENSITY),
                                  MIN_INTENSITY, MAX_INTENSITY, INTENSITY_DP)},
            {TX_CONFIG_ISI_DURATION_S,
             new QuLineEditDouble(fieldRef(FN_ISI_DURATION_S),
                                  0.0, 100.0, TIME_DP)},
        }),
        new QuBoolean(TX_CONFIG_IS_DETECTION_RESPONSE_ON_RIGHT,
                      fieldRef(FN_IS_DETECTION_RESPONSE_ON_RIGHT)),
    })->setTitle(TX_CONFIG_TITLE));

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);
    m_questionnaire->setWithinChain(true);  // fast forward button, not stop

    connect(m_questionnaire.data(), &Questionnaire::cancelled,
            this, &CardinalExpectationDetection::abort);
    connect(m_questionnaire.data(), &Questionnaire::completed,
            this, &CardinalExpectationDetection::startTask);
    // Because our main m_widget isn't itself a questionnaire, we need to hook
    // up these, too:
    questionnairefunc::connectQuestionnaireToTask(m_questionnaire.data(), this);

    // ------------------------------------------------------------------------
    // If the config questionnaire is successful, we'll launch the main task;
    // prepare this too.
    // ------------------------------------------------------------------------

    m_scene = new QGraphicsScene(SCENE_RECT);
    m_scene->setBackgroundBrush(QBrush(SCENE_BACKGROUND));
    m_graphics_widget = makeGraphicsWidget(m_scene, SCENE_BACKGROUND,
                                           true, true);
    connect(m_graphics_widget.data(), &OpenableWidget::aborting,
            this, &CardinalExpectationDetection::abort);

    m_widget = new OpenableWidget();

    // We start off by seeing the questionnaire:
    m_widget->setWidgetAsOnlyContents(m_questionnaire, 0, false, false);

    return m_widget;
}


// ============================================================================
// Connection macros
// ============================================================================

// MUST USE Qt::QueuedConnection - see comments in clearScene()
#define CONNECT_BUTTON(b, funcname) \
    connect((b).button, &QPushButton::clicked, \
            this, &CardinalExpectationDetection::funcname, \
            Qt::QueuedConnection)
// To use a Qt::ConnectionType parameter with a functor, we need a context
// See http://doc.qt.io/qt-5/qobject.html#connect-5
// That's the reason for the extra "this":
#define CONNECT_BUTTON_PARAM(b, funcname, param) \
    connect((b).button, &QPushButton::clicked, \
            this, std::bind(&CardinalExpectationDetection::funcname, this, param), \
            Qt::QueuedConnection)


// ============================================================================
// Calculation/assistance functions for main task
// ============================================================================

void CardinalExpectationDetection::makeTrialGroupSpecs()
{
    DbNestableTransaction trans(m_db);
    m_groups.clear();  // should be clear anyway
    for (int i = 0; i < N_TRIAL_GROUPS; ++i) {
        const int group_num = i;

        // CUE             00 01 02 03 04 05 06 07
        // TARGET_MODALITY  0  0  0  0  1  1  1  1  } define the four target types
        // TARGET_NUMBER    0  0  1  1  0  0  1  1  }
        // N_TARGET         2  1  2  1  2  1  2  1    } define the high-/low-probability cues
        // N_NO_TARGET      1  2  1  2  1  2  1  2    }

        const int cue = i;
        const int target_modality = i / 4;
        const int target_number = (i / 2) % 2;
        const int n_target = (i % 2 == 0) ? 2 : 1;
        const int n_no_target = (i % 2 == 0) ? 1 : 2;

        CardinalExpDetTrialGroupSpecPtr g(new CardinalExpDetTrialGroupSpec(
                    pkvalueInt(), group_num,
                    cue, target_modality, target_number,
                    n_target, n_no_target,
                    m_app, m_db));
        m_groups.append(g);
    }
}


void CardinalExpectationDetection::makeRatingButtonsAndPoints()
{
    const bool detection_response_on_right = valueBool(
                FN_IS_DETECTION_RESPONSE_ON_RIGHT);
    m_ratings.clear();
    for (int i = 0; i < CardinalExpDetRating::N_RATINGS; ++i) {
        m_ratings.append(CardinalExpDetRating(i, detection_response_on_right));
    }
}


void CardinalExpectationDetection::doCounterbalancing()
{
    m_raw_cue_indices.clear();
    for (int i = 0; i < N_CUES_PER_MODALITY; ++i) {
        m_raw_cue_indices.append(i);
    }
    // Then rotate it by the counterbalancing number:
    rotateSequenceInPlace(m_raw_cue_indices, valueInt(FN_STIMULUS_COUNTERBALANCING));
}


int CardinalExpectationDetection::getRawCueIndex(const int cue) const
{
    Q_ASSERT(cue >= 0 && cue < m_raw_cue_indices.size());
    return m_raw_cue_indices.at(cue);
}


QUrl CardinalExpectationDetection::getAuditoryCueUrl(const int cue) const
{
    return urlFromStem(AUDITORY_CUES.at(getRawCueIndex(cue)));
}


QString CardinalExpectationDetection::getVisualCueFilenameStem(
        const int cue) const
{
    return VISUAL_CUES.at(getRawCueIndex(cue));
}


QUrl CardinalExpectationDetection::getAuditoryTargetUrl(
        const int target_number) const
{
    Q_ASSERT(target_number >= 0 && target_number < AUDITORY_TARGETS.size());
    return urlFromStem(AUDITORY_TARGETS.at(target_number));
}


QString CardinalExpectationDetection::getVisualTargetFilenameStem(
        const int target_number) const
{
    Q_ASSERT(target_number >= 0 && target_number < VISUAL_TARGETS.size());
    return VISUAL_TARGETS.at(target_number);
}


QUrl CardinalExpectationDetection::getAuditoryBackgroundUrl() const
{
    return urlFromStem(AUDITORY_BACKGROUND);
}


QString CardinalExpectationDetection::getVisualBackgroundFilename() const
{
    return VISUAL_BACKGROUND;
}


QString CardinalExpectationDetection::getPromptText(
        const int modality,
        const int target_number) const
{
    const bool auditory = modality == MODALITY_AUDITORY;
    const bool first = target_number == 0;
    const QString sense = auditory ? TX_DETECTION_Q_AUDITORY : TX_DETECTION_Q_VISUAL;
    const QString target = auditory
            ? (first ? ExpDetTextConst::auditoryTarget0Short()
                     : ExpDetTextConst::auditoryTarget1Short())
            : (first ? ExpDetTextConst::visualTarget0Short()
                     : ExpDetTextConst::visualTarget1Short());
    return QString("%1 %2 %3?").arg(TX_DETECTION_Q_PREFIX, sense, target);
}


void CardinalExpectationDetection::reportCounterbalancing() const
{
    const QString SPACER("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    qDebug() << SPACER;
    qDebug() << "COUNTERBALANCING =" << valueInt(FN_STIMULUS_COUNTERBALANCING);
    qDebug() << "m_raw_cue_indices:" << m_raw_cue_indices;
    for (int i = 0; i < m_raw_cue_indices.size(); ++i) {
        qDebug() << "Cue" << i << "maps to raw cue" << m_raw_cue_indices.at(i);
    }
    qDebug() << SPACER;
}


QVector<CardinalExpDetTrialPtr> CardinalExpectationDetection::makeTrialGroup(
        const int block, const int groupnum,
        const CardinalExpDetTrialGroupSpecPtr& groupspec) const
{
    QVector<CardinalExpDetTrialPtr> trials;
    const int cue = groupspec->cue();
    const int raw_cue_number = m_raw_cue_indices.at(cue);
    const int target_modality = groupspec->targetModality();
    const int target_number = groupspec->targetNumber();
    const double iti_min_s = valueDouble(FN_ITI_MIN_S);
    const double iti_max_s = valueDouble(FN_ITI_MAX_S);
    const int task_pk = pkvalueInt();

    // Note: trial number is assigned later, by createTrials()

    for (bool target_present : {true, false}) {
        for (int i = 0; i < groupspec->nTarget(); ++i) {
            trials.append(CardinalExpDetTrialPtr(new CardinalExpDetTrial(
                    task_pk,
                    block,
                    groupnum,
                    cue,
                    raw_cue_number,
                    target_modality,
                    target_number,
                    target_present,
                    ccrandom::randomRealIncUpper(iti_min_s, iti_max_s),
                    m_app, m_db)));
        }
    }
    return trials;
}


void CardinalExpectationDetection::createTrials()
{
    DbNestableTransaction trans(m_db);
    m_trials.clear();  // should be clear anyway
    const int num_blocks = valueInt(FN_NUM_BLOCKS);
    for (int b = 0; b < num_blocks; ++b) {
        QVector<CardinalExpDetTrialPtr> block_of_trials;
        for (int g = 0; g < m_groups.length(); ++g) {
            // zero-based block and group numbering
            QVector<CardinalExpDetTrialPtr> group_of_trials = makeTrialGroup(
                        b, g, m_groups.at(g));
            block_of_trials += group_of_trials;
        }
        ccrandom::shuffle(block_of_trials);  // Randomize in blocks
        m_trials += block_of_trials;
    }
    // Write trial numbers
    for (int i = 0; i < m_trials.size(); ++i) {
        // zero-based trial numbering
        m_trials.at(i)->setTrialNum(i);  // will save
    }
}


void CardinalExpectationDetection::estimateRemaining(int& n_trials_left,
                                                     double& time_min) const
{
    const qint64 auditory_bg_ms = m_player_background->duration();
    const double auditory_bg_s = msToSec(auditory_bg_ms);
    const double visual_target_s = valueDouble(FN_VISUAL_TARGET_DURATION_S);
    const double min_iti_s = valueDouble(FN_ITI_MIN_S);
    const double max_iti_s = valueDouble(FN_ITI_MAX_S);
    const double avg_trial_s =
            mean(visual_target_s, auditory_bg_s) +
            1.0 +  // rough guess for user response time
            2.0 + // rough guess for user confirmation time
            mean(min_iti_s, max_iti_s);
    // Results:
    n_trials_left = m_trials.size() - m_current_trial;
    time_min = secToMin(n_trials_left * avg_trial_s);
}


void CardinalExpectationDetection::clearScene()
{
    m_scene->clear();
}


void CardinalExpectationDetection::setTimeout(const int time_ms,
                                              FuncPtr callback)
{
    m_timer->stop();
    m_timer->disconnect();
    connect(m_timer.data(), &QTimer::timeout,
            this, callback,
            Qt::QueuedConnection);
    m_timer->start(time_ms);
}


CardinalExpDetTrialPtr CardinalExpectationDetection::currentTrial() const
{
    return m_trials.at(m_current_trial);
}


void CardinalExpectationDetection::showVisualStimulus(
        const QString& filename_stem, qreal intensity)
{
    const QString filename = cardinalexpdetcommon::filenameFromStem(filename_stem);
    qDebug() << Q_FUNC_INFO << "Filename:" << filename;
    makeImage(m_scene, VISUAL_STIM_RECT, filename, intensity);
}


// ============================================================================
// Main task internals
// ============================================================================

void CardinalExpectationDetection::startTask()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    m_widget->setWidgetAsOnlyContents(m_graphics_widget, 0, false, false);

    onEditStarted();  // will have been stopped by the end of the questionnaire?

    // Set up players and timers
    soundfunc::makeMediaPlayer(m_player_cue);
    soundfunc::makeMediaPlayer(m_player_background);
    soundfunc::makeMediaPlayer(m_player_target_0);
    soundfunc::makeMediaPlayer(m_player_target_1);
    if (!m_player_cue || !m_player_background ||
            !m_player_target_0 || !m_player_target_1) {
        return;
    }
    connect(m_player_background.data(), &QMediaPlayer::mediaStatusChanged,
            this, &CardinalExpectationDetection::mediaStatusChangedBackground);

    timerfunc::makeSingleShotTimer(m_timer);

    // Prep the background sound, and the targets (just to avoid any subtle
    // loading time information)
    soundfunc::setVolume(m_player_cue,
                         valueDouble(FN_AUDITORY_CUE_INTENSITY));
    soundfunc::setVolume(m_player_background,
                         valueDouble(FN_AUDITORY_BACKGROUND_INTENSITY));
    soundfunc::setVolume(m_player_target_0,
                         valueDouble(FN_AUDITORY_TARGET_0_INTENSITY));
    soundfunc::setVolume(m_player_target_1,
                         valueDouble(FN_AUDITORY_TARGET_1_INTENSITY));
    m_player_background->setMedia(getAuditoryBackgroundUrl());
    m_player_target_0->setMedia(getAuditoryTargetUrl(0));
    m_player_target_1->setMedia(getAuditoryTargetUrl(1));

    // Double-check we have a PK before we create stages/trials
    save();

    // Make everything
    makeRatingButtonsAndPoints();
    doCounterbalancing();
    reportCounterbalancing();
    makeTrialGroupSpecs();
    createTrials();

    // Start
    ButtonAndProxy start = makeTextButton(
                m_scene, START_BTN_RECT, BASE_BUTTON_CONFIG,
                TextConst::touchToStart());
    CONNECT_BUTTON(start, nextTrial);
}


void CardinalExpectationDetection::nextTrial()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    ++m_current_trial;
    if (m_current_trial >= m_trials.length()) {
        thanks();
        return;
    }
    const int pause_every_n = valueInt(FN_PAUSE_EVERY_N_TRIALS);
    const bool pause = pause_every_n > 0 && m_current_trial % pause_every_n == 0;
    currentTrial()->startPauseBeforeTrial(pause);
    if (pause) {
        // we allow a pause at the start of trial 0
        userPause();
    } else {
        startTrialProperWithCue();
    }
}


void CardinalExpectationDetection::userPause()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    int n_trials_left;
    double time_min;
    estimateRemaining(n_trials_left, time_min);
    const QString msg_trials = TX_NUM_TRIALS_LEFT + " " + QString::number(n_trials_left);
    const QString msg_time = TX_TIME_LEFT + " " + QString::number(qRound(time_min));
    makeText(m_scene, PROMPT_1, BASE_TEXT_CONFIG, msg_trials);
    makeText(m_scene, PROMPT_2, BASE_TEXT_CONFIG, msg_time);
    ButtonAndProxy a = makeTextButton(
                m_scene, ABORT_BUTTON_RECT,
                ABORT_BUTTON_CONFIG, TextConst::abort());
    ButtonAndProxy s = makeTextButton(
                m_scene, CONTINUE_BTN_RECT,
                CONTINUE_BUTTON_CONFIG, TX_CONTINUE_WHEN_READY);
    CONNECT_BUTTON_PARAM(a, askAbort, &CardinalExpectationDetection::userPause);
    CONNECT_BUTTON(s, startTrialProperWithCue);
}


void CardinalExpectationDetection::startTrialProperWithCue()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    CardinalExpDetTrialPtr t = currentTrial();
    t->startTrialWithCue();
    // Cues are multimodal.
    const int cue = t->cue();
    // (a) sound
    m_player_cue->setMedia(getAuditoryCueUrl(cue));
    m_player_cue->play();
    // (b) image
    showVisualStimulus(getVisualCueFilenameStem(cue),
                       valueDouble(FN_VISUAL_CUE_INTENSITY));
    // Timer:
    setTimeout(secToIntMs(valueDouble(FN_CUE_DURATION_S)),
               &CardinalExpectationDetection::isi);
}


void CardinalExpectationDetection::isi()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    m_player_cue->stop();  // in case it hasn't already; also resets it to the start
    setTimeout(secToIntMs(valueDouble(FN_ISI_DURATION_S)),
               &CardinalExpectationDetection::target);
}


void CardinalExpectationDetection::target()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    CardinalExpDetTrialPtr t = currentTrial();
    qDebug().nospace() << "Target present: " << t->targetPresent()
                       << ", target number: " << t->targetNumber();
    t->startTarget();
    const int target_number = t->targetNumber();

    if (t->isTargetAuditory()) {
        // AUDITORY
        m_player_background->play();
        if (t->targetPresent()) {
            // volume was preset above
            if (target_number == 0) {
                m_player_target_0->play();
            } else {
                m_player_target_1->play();
            }
        }
        // We will get to detection() via the background player's timeout
    } else {
        // VISUAL
        showVisualStimulus(getVisualBackgroundFilename(),
                           valueDouble(FN_VISUAL_BACKGROUND_INTENSITY));
        if (t->targetPresent()) {
            double intensity = target_number == 0
                    ? valueDouble(FN_VISUAL_TARGET_0_INTENSITY)
                    : valueDouble(FN_VISUAL_TARGET_1_INTENSITY);
            showVisualStimulus(getVisualTargetFilenameStem(target_number), intensity);
        }
        setTimeout(secToIntMs(valueDouble(FN_VISUAL_TARGET_DURATION_S)),
                   &CardinalExpectationDetection::detection);
    }
}


void CardinalExpectationDetection::mediaStatusChangedBackground(
        const QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
#ifdef DEBUG_STEP_DETAIL
        qDebug() << "Background sound playback finished";
#endif
        m_player_target_0->stop();  // in case it's still playing
        m_player_target_1->stop();  // in case it's still playing
        detection();
    }
}


void CardinalExpectationDetection::detection()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    CardinalExpDetTrialPtr t = currentTrial();
    makeText(m_scene, PROMPT_1, BASE_TEXT_CONFIG,
             getPromptText(t->targetModality(), t->targetNumber()));
    for (int i = 0; i < m_ratings.size(); ++i) {
        const CardinalExpDetRating& r = m_ratings.at(i);
        ButtonAndProxy b = makeTextButton(m_scene, r.rect,
                                          BASE_BUTTON_CONFIG, r.label);
        CONNECT_BUTTON_PARAM(b, processResponse, i);
    }
    currentTrial()->startDetection();
}


void CardinalExpectationDetection::processResponse(const int rating)
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    qDebug() << "Response: rating =" << rating;
    CardinalExpDetTrialPtr t = currentTrial();
    const CardinalExpDetRating& r = m_ratings.at(rating);
    int previous_points = 0;
    if (m_current_trial > 0) {
        previous_points = m_trials.at(m_current_trial - 1)->cumulativePoints();
    }
    t->recordResponse(r, previous_points);
    setValue(FN_LAST_TRIAL_COMPLETED, m_current_trial);
    save();
    displayScore();
}


void CardinalExpectationDetection::displayScore()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    CardinalExpDetTrialPtr t = currentTrial();
    const int points = t->points();
    const int cum_points = t->cumulativePoints();
    const QString points_msg = TX_POINTS + " " +
            (points > 0 ? "+" : "") +  QString::number(points);
    const QString cumpoints_msg = TX_CUMULATIVE_POINTS + " " +
            (cum_points > 0 ? "+" : "") + QString::number(cum_points);
    makeText(m_scene, PROMPT_1, BASE_TEXT_CONFIG, points_msg);
    makeText(m_scene, PROMPT_2, BASE_TEXT_CONFIG, cumpoints_msg);
    ButtonAndProxy a = makeTextButton(
                m_scene, ABORT_BUTTON_RECT,
                ABORT_BUTTON_CONFIG, TextConst::abort());
    ButtonAndProxy cont = makeTextButton(
                m_scene, CONTINUE_BTN_RECT,
                CONTINUE_BUTTON_CONFIG, TX_CONTINUE_WHEN_READY);
    CONNECT_BUTTON_PARAM(a, askAbort,
                         &CardinalExpectationDetection::displayScore);
    CONNECT_BUTTON(cont, iti);
}


void CardinalExpectationDetection::iti()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    CardinalExpDetTrialPtr t = currentTrial();
    t->startIti();
    setTimeout(t->itiLengthMs(), &CardinalExpectationDetection::endTrial);
}


void CardinalExpectationDetection::endTrial()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    currentTrial()->endTrial();
    nextTrial();
}


void CardinalExpectationDetection::thanks()
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


void CardinalExpectationDetection::askAbort(FuncPtr nextfn)
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    makeText(m_scene, PROMPT_1, BASE_TEXT_CONFIG, TextConst::reallyAbort());
    ButtonAndProxy a = makeTextButton(
                m_scene, REALLY_ABORT_RECT,
                ABORT_BUTTON_CONFIG, TextConst::abort());
    ButtonAndProxy c = makeTextButton(
                m_scene, CANCEL_ABORT_RECT,
                CONTINUE_BUTTON_CONFIG, TextConst::cancel());
    CONNECT_BUTTON(a, abort);
    connect(c.button, &QPushButton::clicked,
            this, nextfn, Qt::QueuedConnection);
}


void CardinalExpectationDetection::abort()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    setValue(FN_ABORTED, true);
    Q_ASSERT(m_widget);
    onEditFinishedAbort();
    emit m_widget->finished();
}


void CardinalExpectationDetection::finish()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    setValue(FN_FINISHED, true);
    Q_ASSERT(m_widget);
    onEditFinishedProperly();
    emit m_widget->finished();
}
