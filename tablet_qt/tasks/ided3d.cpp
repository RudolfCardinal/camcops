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

/*

===============================================================================
Comments
===============================================================================

- Sounds: originals (Rogers 1999 Psychopharm 146:482) were:
    correct = 1200 Hz, 164 ms, volume not given
    incorrect = 200 Hz, 550 ms, volume not given
  However, on the tablet, using the Audacity tone generator with the same
  parameters (in other respects) for the stimuli, makes the incorrect one
  nearly inaudible. So let's use different notes.
- Actual source in my chord.py. All are sine waves.
    correct = E5 + G5 + C6 (Cmaj), 164 ms
    incorrect = A4 + C5 + Eb5 + F#5 (Adim7), 550 ms

- Any further control required over over exact values used for shape/colour/
  number?

*/

// #define DEBUG_SVG
// #define DEBUG_STEP_DETAIL

#include "ided3d.h"
#include <functional>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QPushButton>
#include <QTimer>
#include <QtMath>
#include "common/colourdefs.h"
#include "common/textconst.h"
#include "db/ancillaryfunc.h"
#include "lib/containers.h"
#include "lib/datetime.h"
#include "lib/soundfunc.h"
#include "lib/timerfunc.h"
#include "lib/uifunc.h"
#include "maths/ccrandom.h"
#include "maths/mathfunc.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
#include "taskxtra/ided3dexemplars.h"
#include "taskxtra/ided3dstage.h"
#include "taskxtra/ided3dtrial.h"
#include "widgets/adjustablepie.h"
#include "widgets/svgwidgetclickable.h"
#include "widgets/openablewidget.h"
using ccrandom::dwor;
using graphicsfunc::ButtonAndProxy;
using graphicsfunc::centredRect;
using graphicsfunc::makeSvg;
using graphicsfunc::makeText;
using graphicsfunc::makeTextButton;
using graphicsfunc::makeObscuringRect;
using graphicsfunc::SvgTransform;
using graphicsfunc::SvgWidgetAndProxy;
using mathfunc::distribute;
using mathfunc::gridDimensions;
using mathfunc::rep;

// ============================================================================
// Constants
// ============================================================================

// Table names
const QString IDED3D::IDED3D_TABLENAME("ided3d");

namespace ided3dconst {
// We use this namespace because otherwise we can't create BORDER_PEN with
// the same name as the one in QolSG (which, now, is also in a namespace).
// Not sure why, except it's probably something to do with QObject objects
// being mushed into one conceptual space by the Qt MOC. There is no C++
// reason why variables in independent compilation units can't have the same
// name.

// Fieldnames: IDED3D
const QString FN_LAST_STAGE("last_stage");
const QString FN_MAX_TRIALS_PER_STAGE("max_trials_per_stage");
const QString FN_PROGRESS_CRITERION_X("progress_criterion_x");
const QString FN_PROGRESS_CRITERION_Y("progress_criterion_y");
const QString FN_MIN_NUMBER("min_number");
const QString FN_MAX_NUMBER("max_number");
const QString FN_PAUSE_AFTER_BEEP_MS("pause_after_beep_ms");
const QString FN_ITI_MS("iti_ms");
const QString FN_COUNTERBALANCE_DIMENSIONS("counterbalance_dimensions");
const QString FN_VOLUME("volume");
const QString FN_OFFER_ABORT("offer_abort");
const QString FN_DEBUG_DISPLAY_STIMULI_ONLY("debug_display_stimuli_only");
const QString FN_SHAPE_DEFINITIONS_SVG("shape_definitions_svg");
const QString FN_COLOUR_DEFINITIONS_RGB("colour_definitions_rgb");  // new in v2.0.0
const QString FN_ABORTED("aborted");
const QString FN_FINISHED("finished");
const QString FN_LAST_TRIAL_COMPLETED("last_trial_completed");

// Questionnaire bit
const QString TAG_WARNING_PROGRESS_CRITERION("pc");
const QString TAG_WARNING_MIN_MAX("mm");

// Graphics
const qreal SCENE_WIDTH = 1000;
const qreal SCENE_HEIGHT = 750;  // 4:3 aspect ratio
const QColor SCENE_BACKGROUND(QCOLOR_BLACK);  // try QCOLOR_LIGHTSALMON
const int BORDER_WIDTH_PX = 3;
const QColor BUTTON_BACKGROUND(QCOLOR_BLUE);
const QColor TEXT_COLOUR(QCOLOR_WHITE);
const QColor BUTTON_PRESSED_BACKGROUND(QCOLOR_OLIVE);
const QColor ABORT_BUTTON_BACKGROUND(QCOLOR_DARKRED);
const int TEXT_SIZE_PX = 20;  // will be scaled
const int BUTTON_RADIUS = 5;
const int PADDING = 5;
const Qt::Alignment BUTTON_TEXT_ALIGN = Qt::AlignCenter;
const Qt::Alignment TEXT_ALIGN = Qt::AlignCenter;
const int STIMSIZE = 120;  // max width/height
const int STIM_STROKE_WIDTH = 3;
const QColor STIM_PRESSED_BG_COLOUR(QCOLOR_ORANGE);
const QColor EDGE_COLOUR(QCOLOR_WHITE);
const QColor CORRECT_BG_COLOUR(QCOLOR_GREEN);
const QColor INCORRECT_BG_COLOUR(QCOLOR_RED);
const qreal FEEDBACK_OPACITY = 0.75;

// Colours
const QColor TEST_BACKGROUND(QCOLOR_GREEN);
const QColor TEST_COLOUR(QCOLOR_PURPLE);

// Sound
const QString SOUND_COUNTDOWN_FINISHED("qrc:///resources/camcops/sounds/countdown_finished.wav");

const QString SOUND_FILE_CORRECT("ided3d/correct.wav");
const QString SOUND_FILE_INCORRECT("ided3d/incorrect.wav");
const qreal MIN_VOLUME = 0.0;
const qreal MAX_VOLUME = 1.0;  // NOTE: for Qt, need to scale volume to 0-100
const int VOLUME_DP = 2;

// Task constants
const int MAX_STAGES = 8;
const int MAX_NUMBER = 9;
const int MAX_COUNTERBALANCE_DIMENSIONS = 5;
const int DEFAULT_MAX_TRIALS_PER_STAGE = 50;
const int DEFAULT_PROGRESS_CRITERION_X = 6;  // as per Rogers et al. 1999
const int DEFAULT_PROGRESS_CRITERION_Y = 6;  // as per Rogers et al. 1999
const int DEFAULT_PAUSE_AFTER_BEEP_MS = 500;
const int DEFAULT_ITI_MS = 500;
const qreal DEFAULT_VOLUME = MAX_VOLUME / 2.0;
const bool DEFAULT_OFFER_ABORT = false;

// Derived constants
const QRectF SCENE_RECT(0, 0, SCENE_WIDTH, SCENE_HEIGHT);
const QPen BORDER_PEN(QBrush(EDGE_COLOUR), BORDER_WIDTH_PX);
const ButtonConfig BASE_BUTTON_CONFIG(
        PADDING, TEXT_SIZE_PX, TEXT_COLOUR, BUTTON_TEXT_ALIGN,
        BUTTON_BACKGROUND, BUTTON_PRESSED_BACKGROUND,
        BORDER_PEN, BUTTON_RADIUS);
const ButtonConfig STIM_BUTTON_CONFIG(
        PADDING, TEXT_SIZE_PX, TEXT_COLOUR, BUTTON_TEXT_ALIGN,
        QCOLOR_TRANSPARENT, BUTTON_PRESSED_BACKGROUND,
        BORDER_PEN, BUTTON_RADIUS);
const ButtonConfig EMPTYBOX_BUTTON_CONFIG(
        PADDING, TEXT_SIZE_PX, TEXT_COLOUR, BUTTON_TEXT_ALIGN,
        QCOLOR_TRANSPARENT, QCOLOR_TRANSPARENT,
        BORDER_PEN, BUTTON_RADIUS);
const TextConfig BASE_TEXT_CONFIG(TEXT_SIZE_PX, TEXT_COLOUR,
                                  static_cast<int>(SCENE_WIDTH), TEXT_ALIGN);

const qreal BOXWIDTH = SCENE_WIDTH * 0.45;  // use 90%
const qreal BOXHEIGHT = SCENE_HEIGHT * 0.3;  // use 90%
const QBrush BOXBRUSH;
const QVector<qreal> VDIST(distribute(3, 0, SCENE_HEIGHT));
const QVector<qreal> HDIST{
    SCENE_WIDTH * 0.25,
    SCENE_WIDTH * 0.5,
    SCENE_WIDTH * 0.75
};
const QVector<QPointF> LOCATIONS{  // centre points
    QPointF(HDIST.at(1), VDIST.at(0)),  // top
    QPointF(HDIST.at(2), VDIST.at(1)),  // right
    QPointF(HDIST.at(1), VDIST.at(2)),  // bottom
    QPointF(HDIST.at(0), VDIST.at(1)),  // left
};
const QPointF SCENE_CENTRE(SCENE_WIDTH * 0.5, SCENE_HEIGHT * 0.5);
const QRectF ANSWER_BACKDROP_RECT(centredRect(SCENE_CENTRE,
                                              0.3 * SCENE_WIDTH,
                                              0.1 * SCENE_HEIGHT));


}  // namespace ided3dconst
using namespace ided3dconst;


// ============================================================================
// Factory method
// ============================================================================

void initializeIDED3D(TaskFactory& factory)
{
    static TaskRegistrar<IDED3D> registered(factory);
}


// ============================================================================
// IDED3D
// ============================================================================

IDED3D::IDED3D(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, IDED3D_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    // Config
    addField(FN_LAST_STAGE, QVariant::Int);
    addField(FN_MAX_TRIALS_PER_STAGE, QVariant::Int);
    addField(FN_PROGRESS_CRITERION_X, QVariant::Int);
    addField(FN_PROGRESS_CRITERION_Y, QVariant::Int);
    addField(FN_MIN_NUMBER, QVariant::Int);
    addField(FN_MAX_NUMBER, QVariant::Int);
    addField(FN_PAUSE_AFTER_BEEP_MS, QVariant::Int);
    addField(FN_ITI_MS, QVariant::Int);
    addField(FN_COUNTERBALANCE_DIMENSIONS, QVariant::Int);
    addField(FN_VOLUME, QVariant::Double);
    addField(FN_OFFER_ABORT, QVariant::Bool);
    addField(FN_DEBUG_DISPLAY_STIMULI_ONLY, QVariant::Bool);
    addField(FN_SHAPE_DEFINITIONS_SVG, QVariant::String);
    addField(FN_COLOUR_DEFINITIONS_RGB, QVariant::String);
    // Results
    addField(FN_ABORTED, QVariant::Bool);
    getField(FN_ABORTED).setDefaultValue(false);
    addField(FN_FINISHED, QVariant::Bool);
    getField(FN_FINISHED).setDefaultValue(false);
    addField(FN_LAST_TRIAL_COMPLETED, QVariant::Int);

    load(load_pk);

    if (load_pk == dbconst::NONEXISTENT_PK) {
        // Default values:
        setValue(FN_LAST_STAGE, MAX_STAGES, false);
        setValue(FN_MAX_TRIALS_PER_STAGE, DEFAULT_MAX_TRIALS_PER_STAGE, false);
        setValue(FN_PROGRESS_CRITERION_X, DEFAULT_PROGRESS_CRITERION_X, false);
        setValue(FN_PROGRESS_CRITERION_Y, DEFAULT_PROGRESS_CRITERION_Y, false);
        setValue(FN_MIN_NUMBER, 1, false);
        setValue(FN_MAX_NUMBER, MAX_NUMBER, false);
        setValue(FN_PAUSE_AFTER_BEEP_MS, DEFAULT_PAUSE_AFTER_BEEP_MS, false);
        setValue(FN_ITI_MS, DEFAULT_ITI_MS, false);
        setValue(FN_VOLUME, DEFAULT_VOLUME, false);
        setValue(FN_OFFER_ABORT, DEFAULT_OFFER_ABORT, false);
        setValue(FN_DEBUG_DISPLAY_STIMULI_ONLY, false, false);
    }

    // Internal data
    m_current_stage = 0;
    m_current_trial = -1;
    timerfunc::makeSingleShotTimer(m_timer);
}


IDED3D::~IDED3D()
{
    // Necessary: for rationale, see QuAudioPlayer::~QuAudioPlayer()
    soundfunc::finishMediaPlayer(m_player_correct);
    soundfunc::finishMediaPlayer(m_player_incorrect);
}


// ============================================================================
// Class info
// ============================================================================

QString IDED3D::shortname() const
{
    return "ID/ED-3D";
}


QString IDED3D::longname() const
{
    return tr("Three-dimensional intradimensional/extradimensional "
              "set-shifting task");
}


QString IDED3D::description() const
{
    return tr("Simple discrimination, reversal, compound discrimination, "
              "reversal, ID set shift, reversal, ED set shift, reversal. "
              "Dimensions of shape/colour/number.");
}


// ============================================================================
// Ancillary management
// ============================================================================

QStringList IDED3D::ancillaryTables() const
{
    return QStringList{IDED3DStage::STAGE_TABLENAME,
                       IDED3DTrial::TRIAL_TABLENAME};
}


QString IDED3D::ancillaryTableFKToTaskFieldname() const
{
    Q_ASSERT(IDED3DStage::FN_FK_TO_TASK == IDED3DTrial::FN_FK_TO_TASK);
    return IDED3DStage::FN_FK_TO_TASK;
}


void IDED3D::loadAllAncillary(int pk)
{
    const OrderBy stage_order_by{{IDED3DStage::FN_STAGE, true}};
    ancillaryfunc::loadAncillary<IDED3DStage, IDED3DStagePtr>(
                m_stages, m_app, m_db,
                IDED3DStage::FN_FK_TO_TASK, stage_order_by, pk);
    const OrderBy trial_order_by{{IDED3DTrial::FN_TRIAL, true}};
    ancillaryfunc::loadAncillary<IDED3DTrial, IDED3DTrialPtr>(
                m_trials, m_app, m_db,
                IDED3DTrial::FN_FK_TO_TASK, trial_order_by, pk);
}


QVector<DatabaseObjectPtr> IDED3D::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>{
        IDED3DStagePtr(new IDED3DStage(m_app, m_db)),
        IDED3DTrialPtr(new IDED3DTrial(m_app, m_db)),
    };
}


QVector<DatabaseObjectPtr> IDED3D::getAllAncillary() const
{
    QVector<DatabaseObjectPtr> ancillaries;
    for (const IDED3DStagePtr& stage : m_stages) {
        ancillaries.append(stage);
    }
    for (const IDED3DTrialPtr& trial : m_trials) {
        ancillaries.append(trial);
    }
    return ancillaries;
}


// ============================================================================
// Instance info
// ============================================================================

bool IDED3D::isComplete() const
{
    return valueBool(FN_DEBUG_DISPLAY_STIMULI_ONLY) || valueBool(FN_FINISHED);
}


QStringList IDED3D::summary() const
{
    if (valueBool(FN_DEBUG_DISPLAY_STIMULI_ONLY)) {
        return QStringList{tr("Debug mode for displaying test stimuli only")};
    }
    QStringList lines;
    const int n_trials = m_trials.length();
    lines.append(tr("Performed %1 trial(s).").arg(n_trials));
    if (n_trials > 0) {
        IDED3DTrialPtr last_trial = m_trials.at(n_trials - 1);
        lines.append(tr("Last trial was at stage %1.")
                     .arg(last_trial->valueInt(IDED3DTrial::FN_STAGE)));
    }
    return lines;
}


QStringList IDED3D::detail() const
{
    QStringList lines = completenessInfo() + recordSummaryLines();
    lines.append("\n");
    lines.append(tr("Stages:"));
    for (const IDED3DStagePtr& stage : m_stages) {
        lines.append(stage->recordSummaryCSVString());
    }
    lines.append("\n");
    lines.append(tr("Trials:"));
    for (const IDED3DTrialPtr& trial : m_trials) {
        lines.append(trial->recordSummaryCSVString());
    }
    return lines;
}


OpenableWidget* IDED3D::editor(const bool read_only)
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

    const int no_max = std::numeric_limits<int>::max();
    const QString warning_progress_criterion(tr(
            "WARNING: cannot proceed: must satisfy "
            "progress_criterion_x <= progress_criterion_y"));
    const QString warning_min_max(tr(
            "WARNING: cannot proceed: must satisfy "
            "min_number <= max_number"));

    QuPagePtr page((new QuPage{
        questionnairefunc::defaultGridRawPointer({
            {xstring("last_stage"),
             new QuLineEditInteger(fieldRef(FN_LAST_STAGE), 1, MAX_STAGES)},
            {xstring("max_trials_per_stage"),
             new QuLineEditInteger(fieldRef(FN_MAX_TRIALS_PER_STAGE), 1, no_max)},
            {xstring("progress_criterion_x"),
             new QuLineEditInteger(fieldRef(FN_PROGRESS_CRITERION_X), 1, no_max)},
            {xstring("progress_criterion_y"),
             new QuLineEditInteger(fieldRef(FN_PROGRESS_CRITERION_Y), 1, no_max)},
            {xstring("min_number"),
             new QuLineEditInteger(fieldRef(FN_MIN_NUMBER), 1, MAX_NUMBER)},
            {xstring("max_number"),
             new QuLineEditInteger(fieldRef(FN_MAX_NUMBER), 1, MAX_NUMBER)},
            {xstring("pause_after_beep_ms"),
             new QuLineEditInteger(fieldRef(FN_PAUSE_AFTER_BEEP_MS), 0, no_max)},
            {xstring("iti_ms"),
             new QuLineEditInteger(fieldRef(FN_ITI_MS), 0, no_max)},
            {xstring("counterbalance_dimensions"),
             new QuLineEditInteger(fieldRef(FN_COUNTERBALANCE_DIMENSIONS), 0, MAX_COUNTERBALANCE_DIMENSIONS)},
            {xstring("volume"),
             new QuLineEditDouble(fieldRef(FN_VOLUME), MIN_VOLUME, MAX_VOLUME, VOLUME_DP)},
            {xstring("offer_abort"),
             (new QuBoolean(xstring("offer_abort"),
                            fieldRef(FN_OFFER_ABORT)))->setAsTextButton(true)},
            {xstring("debug_display_stimuli_only"),
             (new QuBoolean(xstring("debug_display_stimuli_only"),
                            fieldRef(FN_DEBUG_DISPLAY_STIMULI_ONLY)))->setAsTextButton(true)},
        }),
        (new QuText(warning_progress_criterion))
                        ->setWarning(true)
                        ->addTag(TAG_WARNING_PROGRESS_CRITERION),
        (new QuText(warning_min_max))
                        ->setWarning(true)
                        ->addTag(TAG_WARNING_MIN_MAX),
    })->setTitle(longname()));

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);
    m_questionnaire->setWithinChain(true);  // fast forward button, not stop

    connect(fieldRef(FN_PROGRESS_CRITERION_X).data(), &FieldRef::valueChanged,
            this, &IDED3D::validateQuestionnaire);
    connect(fieldRef(FN_PROGRESS_CRITERION_Y).data(), &FieldRef::valueChanged,
            this, &IDED3D::validateQuestionnaire);
    connect(fieldRef(FN_MIN_NUMBER).data(), &FieldRef::valueChanged,
            this, &IDED3D::validateQuestionnaire);
    connect(fieldRef(FN_MAX_NUMBER).data(), &FieldRef::valueChanged,
            this, &IDED3D::validateQuestionnaire);

    connect(m_questionnaire.data(), &Questionnaire::cancelled,
            this, &IDED3D::abort);
    connect(m_questionnaire.data(), &Questionnaire::completed,
            this, &IDED3D::startTask);
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
            this, &IDED3D::abort);

    m_widget = new OpenableWidget();

    // We start off by seeing the questionnaire:
    m_widget->setWidgetAsOnlyContents(m_questionnaire, 0, false, false);

    return m_widget;
}


// ============================================================================
// Config questionnaire internals
// ============================================================================

void IDED3D::validateQuestionnaire()
{
    if (!m_questionnaire) {
        return;
    }
    QVector<QuPage*> pages = m_questionnaire->getPages(true);
    Q_ASSERT(pages.size() == 1);
    QuPage* page = pages.at(0);

    const bool duff_pc = valueInt(FN_PROGRESS_CRITERION_Y) < valueInt(FN_PROGRESS_CRITERION_X);
    const bool duff_minmax = valueInt(FN_MAX_NUMBER) < valueInt(FN_MIN_NUMBER);

    m_questionnaire->setVisibleByTag(TAG_WARNING_PROGRESS_CRITERION, duff_pc);
    m_questionnaire->setVisibleByTag(TAG_WARNING_MIN_MAX, duff_minmax);
    page->blockProgress(duff_pc || duff_minmax);
}


// ============================================================================
// Connection macros
// ============================================================================

// MUST USE Qt::QueuedConnection - see comments in clearScene()
#define CONNECT_BUTTON(b, funcname) \
    connect((b).button, &QPushButton::clicked, \
            this, &IDED3D::funcname, \
            Qt::QueuedConnection)
// To use a Qt::ConnectionType parameter with a functor, we need a context
// See http://doc.qt.io/qt-5/qobject.html#connect-5
// That's the reason for the extra "this":
#define CONNECT_BUTTON_PARAM(b, funcname, param) \
    connect((b).button, &QPushButton::clicked, \
            this, std::bind(&IDED3D::funcname, this, param), \
            Qt::QueuedConnection)
// For debugging:
#define CONNECT_SVG_CLICKED(svg, funcname) \
    connect((svg).widget, &SvgWidgetClickable::clicked, \
            this, &IDED3D::funcname, \
            Qt::QueuedConnection)
    // ... svg is an SvgItemAndRenderer
    // ... use "pressed" not "clicked" for rapid response detection.


// ============================================================================
// Calculation/assistance functions for main task
// ============================================================================

void IDED3D::makeStages()
{
    using Exemplars = IDED3DExemplars;
    const QStringList poss_dimensions = Exemplars::possibleDimensions();
    const int n_dimensions = poss_dimensions.length();
    const QVector<QVector<int>> possibilities = Exemplars::possibilities(
                valueInt(FN_MIN_NUMBER),
                valueInt(FN_MAX_NUMBER));

    // Counterbalancing
    const int cb_dim = valueInt(FN_COUNTERBALANCE_DIMENSIONS);
    const int cb1max = n_dimensions;
    const int cb2max = n_dimensions - 1;
    const int cb1 = cb_dim % cb1max;
    const int cb2 = static_cast<int>(floor(static_cast<double>(cb_dim) /
                                           static_cast<double>(cb1max))) % cb2max;
    // Dimensions
    const int first_dim_index = cb1;
    const int second_dim_index = (first_dim_index + 1 + cb2) % n_dimensions;
    const int third_dim_index = (first_dim_index + 1 + (cb2max - 1 - cb2)) % n_dimensions;

    // Exemplars ("poss" = possibilities)
    QVector<int> poss_first_dim = possibilities.at(first_dim_index);
    QVector<int> poss_second_dim = possibilities.at(second_dim_index);
    QVector<int> poss_third_dim = possibilities.at(third_dim_index);

    // Relevant exemplars:
    const int sd_correct_exemplar = dwor(poss_first_dim);
    const int sd_incorrect_exemplar = dwor(poss_first_dim);
    const int id_correct_exemplar = dwor(poss_first_dim);
    const int id_incorrect_exemplar = dwor(poss_first_dim);
    const int ed_correct_exemplar = dwor(poss_second_dim);
    const int ed_incorrect_exemplar = dwor(poss_second_dim);

    // Irrelevant exemplars:
    const int sd_irrelevant_exemplar_second_dim = dwor(poss_second_dim);
    const int sd_irrelevant_exemplar_third_dim = dwor(poss_third_dim);
    const QVector<int> cd_irrelevant_exemplars_second_dim{
        // Only two distracting exemplars in each irrelevant dimension.
        dwor(poss_second_dim),
        dwor(poss_second_dim),
    };
    const QVector<int> cd_irrelevant_exemplars_third_dim{
        dwor(poss_third_dim),
        dwor(poss_third_dim),
    };
    const QVector<int> id_irrelevant_exemplars_second_dim{
        // Only two distracting exemplars in each irrelevant dimension.
        dwor(poss_second_dim),
        dwor(poss_second_dim),
    };
    const QVector<int> id_irrelevant_exemplars_third_dim{
        dwor(poss_third_dim),
        dwor(poss_third_dim),
    };
    const QVector<int> ed_irrelevant_exemplars_first_dim{
        // Only two distracting exemplars in each irrelevant dimension.
        dwor(poss_first_dim),
        dwor(poss_first_dim),
    };
    const QVector<int> ed_irrelevant_exemplars_third_dim{
        dwor(poss_third_dim),
        dwor(poss_third_dim),
    };

    // Final stimulus collections
    const QStringList dimensions{
        poss_dimensions.at(first_dim_index),
        poss_dimensions.at(second_dim_index),
        poss_dimensions.at(third_dim_index),
    };
    // SD: simple discrimination
    const Exemplars sd_correct(dimensions, {{sd_correct_exemplar},
                                            {sd_irrelevant_exemplar_second_dim},
                                            {sd_irrelevant_exemplar_third_dim}});
    const Exemplars sd_incorrect(dimensions, {{sd_incorrect_exemplar},
                                              {sd_irrelevant_exemplar_second_dim},
                                              {sd_irrelevant_exemplar_third_dim}});
    // SDR: SD reversal
    const Exemplars sdr_correct(dimensions, {{sd_incorrect_exemplar},
                                             {sd_irrelevant_exemplar_second_dim},
                                             {sd_irrelevant_exemplar_third_dim}});
    const Exemplars sdr_incorrect(dimensions, {{sd_correct_exemplar},
                                               {sd_irrelevant_exemplar_second_dim},
                                               {sd_irrelevant_exemplar_third_dim}});
    // CD: concurrent discrimination
    const Exemplars cd_correct(dimensions, {{sd_incorrect_exemplar},
                                            cd_irrelevant_exemplars_second_dim,
                                            cd_irrelevant_exemplars_third_dim});
    const Exemplars cd_incorrect(dimensions, {{sd_correct_exemplar},
                                              cd_irrelevant_exemplars_second_dim,
                                              cd_irrelevant_exemplars_third_dim});
    // CDR: CD reversal
    const Exemplars cdr_correct(dimensions, {{sd_correct_exemplar},
                                             cd_irrelevant_exemplars_second_dim,
                                             cd_irrelevant_exemplars_third_dim});
    const Exemplars cdr_incorrect(dimensions, {{sd_incorrect_exemplar},
                                               cd_irrelevant_exemplars_second_dim,
                                               cd_irrelevant_exemplars_third_dim});
    // ID: intradimensional set shift
    const Exemplars id_correct(dimensions, {{id_correct_exemplar},
                                            id_irrelevant_exemplars_second_dim,
                                            id_irrelevant_exemplars_third_dim});
    const Exemplars id_incorrect(dimensions, {{id_incorrect_exemplar},
                                              id_irrelevant_exemplars_second_dim,
                                              id_irrelevant_exemplars_third_dim});
    // IDR: ID reversal
    const Exemplars idr_correct(dimensions, {{id_incorrect_exemplar},
                                             id_irrelevant_exemplars_second_dim,
                                             id_irrelevant_exemplars_third_dim});
    const Exemplars idr_incorrect(dimensions, {{id_correct_exemplar},
                                               id_irrelevant_exemplars_second_dim,
                                               id_irrelevant_exemplars_third_dim});
    // ED: extradimensional set shift
    const Exemplars ed_correct(dimensions, {ed_irrelevant_exemplars_first_dim,
                                            {ed_correct_exemplar},
                                            ed_irrelevant_exemplars_third_dim});
    const Exemplars ed_incorrect(dimensions, {ed_irrelevant_exemplars_first_dim,
                                              {ed_incorrect_exemplar},
                                              ed_irrelevant_exemplars_third_dim});
    // EDR: ED reversal
    const Exemplars edr_correct(dimensions, {ed_irrelevant_exemplars_first_dim,
                                             {ed_incorrect_exemplar},
                                             ed_irrelevant_exemplars_third_dim});
    const Exemplars edr_incorrect(dimensions, {ed_irrelevant_exemplars_first_dim,
                                               {ed_correct_exemplar},
                                               ed_irrelevant_exemplars_third_dim});

    // Stages
    const QString& first_dim_name = poss_dimensions.at(first_dim_index);
    const QString& second_dim_name = poss_dimensions.at(second_dim_index);
    int stage = 0;  // zero-based stage number
    m_stages.clear();
    // ... use QVector<IDED3DStagePtr>; don't use QVector<Stage>;
    // no default constructor ("implicitly deleted... ill-formed...")
    const int n_locations = LOCATIONS.length();
    const int pk = pkvalueInt();
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "SD",  // Only a single dimension varies
            first_dim_name, sd_correct, sd_incorrect, n_locations,
            true)));  // incorrect_stimulus_can_overlap
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "SDr",  // Reversal of SD
            first_dim_name, sdr_correct, sdr_incorrect, n_locations,
            true)));  // incorrect_stimulus_can_overlap
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "CD",  // "Compound discrimination"
            first_dim_name, cd_correct, cd_incorrect, n_locations,
            false)));  // incorrect_stimulus_can_overlap
    /*
    The phrase "compound discrimination" is ambiguous.
    The discrimination is not that a compound stimulus is correct
    (e.g. blue square), but that a particular unidimensional exemplar
    (e.g. blue) is correct, while the stimuli also vary along
    irrelevant dimensions (e.g. two/four, square/circle).
    */
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "CDr",  // Reversal of CD
            first_dim_name, cdr_correct, cdr_incorrect, n_locations,
            false)));  // incorrect_stimulus_can_overlap
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "ID",  // Intradimensional shift
            first_dim_name, id_correct, id_incorrect, n_locations,
            false)));  // incorrect_stimulus_can_overlap
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "IDr",  // ID reversal
            first_dim_name, idr_correct, idr_incorrect, n_locations,
            false)));  // incorrect_stimulus_can_overlap
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "ED",  // Extradimensional shift
            second_dim_name, ed_correct, ed_incorrect, n_locations,
            false)));  // incorrect_stimulus_can_overlap
    m_stages.append(IDED3DStagePtr(new IDED3DStage(
            pk, m_app, m_db, stage++, "EDr",  // ED reversal
            second_dim_name, edr_correct, edr_incorrect, n_locations,
            false)));  // incorrect_stimulus_can_overlap
}


void IDED3D::debugDisplayStimuli()
{
    const int n_stimuli = IDED3DExemplars::nShapes();
    m_scene->addRect(SCENE_RECT, QPen(), QBrush(TEST_BACKGROUND));
    const qreal aspect = SCENE_WIDTH / SCENE_HEIGHT;
    const QPair<int, int> x_y = gridDimensions(n_stimuli, aspect);
    const int nx = x_y.first;
    const int ny = x_y.second;
    const QVector<qreal> x_centres = distribute(nx, 0, SCENE_WIDTH);
    const QVector<qreal> y_centres = distribute(ny, 0, SCENE_HEIGHT);
    const qreal scale = 0.8 * qMin(SCENE_WIDTH / nx, SCENE_HEIGHT / ny) / STIMSIZE;
    int n = 0;
    for (int y = 0; y < ny && n < n_stimuli; ++y) {
        for (int x = 0; x < nx && n < n_stimuli; ++x) {
            QPointF centre(x_centres[x], y_centres[y]);
            SvgWidgetAndProxy stim = showIndividualStimulus(
                        n, TEST_COLOUR, centre, scale, true);
            QString label = QString::number(n);
            makeText(m_scene, centre, BASE_TEXT_CONFIG, label);
            CONNECT_SVG_CLICKED(stim, finish);
            n += 1;
        }
    }
}


SvgWidgetAndProxy IDED3D::showIndividualStimulus(
        const int stimulus_num, const QColor& colour,
        const QPointF& centre, const qreal scale,
        const bool debug)
{
    Q_ASSERT(stimulus_num >= 0 && stimulus_num < IDED3DExemplars::nShapes());
    const QString& path_contents = IDED3DExemplars::shapeSvg(stimulus_num);
    SvgTransform transform;
    transform.scale(scale);
    const QString svg = graphicsfunc::svgFromPathContents(
                path_contents, colour, STIM_STROKE_WIDTH, colour, transform);
#ifdef DEBUG_SVG
    qDebug().noquote() << "showIndividualStimulus: svg:" << svg;
#endif
    bool transparent_for_mouse = !debug;
    return makeSvg(m_scene, centre, svg,
                   debug ? STIM_PRESSED_BG_COLOUR : QCOLOR_TRANSPARENT,
                   QCOLOR_TRANSPARENT,
                   transparent_for_mouse);
}


QVector<QPointF> IDED3D::stimCentres(const int n) const
{
    // Centre-of-stimulus positions within box.
    // Distribute stimuli about (0, 0) in an imaginary box that's 1 x 1,
    // i.e. from -0.5 to +0.5 in each direction.
    const qreal left = -0.5;
    const qreal right = +0.5;
    const qreal top = -0.5;
    const qreal bottom = +0.5;

    QVector<qreal> x, y;

    switch (n) {

    // horizontal row:
    case 1:
    case 2:
        x = distribute(n, left, right);
        y = rep(0.0, n);
        break;

    // two rows:
    case 4:
    case 6:  // Rogers 1999 gives this example
    case 8:
        x = rep(distribute(n / 2, left, right), 1, 2);
        y = rep(distribute(2, top, bottom), n / 2, 1);
        break;

    // one fewer on bottom than top:
    case 3:  // Rogers 1999 gives this example
    case 5:
    case 7:
    case 9:
        {
            int ntop = qFloor(n / 2.0);
            int nbottom = qCeil(n / 2.0);
            QVector<qreal> topx = distribute(ntop, left, right);
            QVector<qreal> bottomx = distribute(nbottom, left, right);
            QVector<qreal> tempy = distribute(2, top, bottom);
            QVector<qreal> topy = rep(tempy.at(0), ntop);
            QVector<qreal> bottomy = rep(tempy.at(1), nbottom);
            x = topx + bottomx;
            y = topy + bottomy;
        }
        break;

    // something wrong:
    default:
        Q_ASSERT(false);
    }

    Q_ASSERT(x.size() == y.size());
    QVector<QPointF> points;
    for (int i = 0; i < x.size(); ++i) {
        points.append(QPointF(x.at(i), y.at(i)));
    }
    Q_ASSERT(points.size() == n);
    return points;
}


QRectF IDED3D::locationRect(const int location) const
{
    Q_ASSERT(location >= 0 && location < LOCATIONS.size());
    const QPointF centre = LOCATIONS.at(location);
    return QRectF(centre.x() - BOXWIDTH / 2,
                  centre.y() - BOXHEIGHT / 2,
                  BOXWIDTH,
                  BOXHEIGHT);
}


void IDED3D::showEmptyBox(const int location,
                          const bool touchable,
                          const bool correct)
{
    const QRectF rect = locationRect(location);
    ButtonAndProxy box = makeTextButton(
                m_scene,
                rect,
                touchable ? STIM_BUTTON_CONFIG : EMPTYBOX_BUTTON_CONFIG,
                "");
    if (touchable) {
        CONNECT_BUTTON_PARAM(box, recordResponse, correct);
    }
}


void IDED3D::showCompositeStimulus(const int shape,
                                   const int colour_number,
                                   const int number,
                                   const int location,
                                   bool correct)
{
    Q_ASSERT(location >= 0 && location < LOCATIONS.size());
    const QPointF overall_centre = LOCATIONS.at(location);
    const QColor colour = IDED3DExemplars::colour(colour_number);
    const qreal scale = (0.75 * 0.95 * BOXHEIGHT / 2) / STIMSIZE;
    // ... without the 0.75, you can fit 4 but not 5 wide.
    QVector<QPointF> centres = stimCentres(number);

    // We make the background box touchable, not the SVG. This handles line-
    // like stimuli better, and is visually preferable.
    showEmptyBox(location, true, correct);
    for (int i = 0; i < number; ++i) {
        // Scale up
        centres[i].rx() *= BOXWIDTH;
        centres[i].ry() *= BOXHEIGHT;
        // Reposition (from coordinates relative to box centre at 0,0)
        centres[i].rx() += overall_centre.x();
        centres[i].ry() += overall_centre.y();
        showIndividualStimulus(shape, colour, centres.at(i), scale);
    }
}


void IDED3D::clearScene()
{
    m_scene->clear();  // be careful not to do m_scene.clear() instead!
}


void IDED3D::setTimeout(int time_ms, FuncPtr callback)
{
    m_timer->stop();
    m_timer->disconnect();
    connect(m_timer.data(), &QTimer::timeout,
            this, callback,
            Qt::QueuedConnection);
    m_timer->start(time_ms);
}


bool IDED3D::stagePassed() const
{
    // X of the last Y correct?
    int n_correct = 0;
    const int first = m_trials.size() - valueInt(FN_PROGRESS_CRITERION_Y);
    for (int i = m_current_trial;
            i >= 0 && i >= first && m_trials.at(i)->stageZeroBased() == m_current_stage;
            --i) {
        if (m_trials.at(i)->wasCorrect()) {
            n_correct += 1;
        }
    }
    bool passed = n_correct >= valueInt(FN_PROGRESS_CRITERION_X);
    qDebug().nospace()
            << n_correct << " correct (need X="
            << valueInt(FN_PROGRESS_CRITERION_X) << ") of last Y="
            << valueInt(FN_PROGRESS_CRITERION_Y)
            << " trials this stage => stage passed = "
            << passed;
    return passed;
}


int IDED3D::getNumTrialsThisStage() const
{
    int n = 0;
    for (int i = m_current_trial; i >= 0; --i) {
        IDED3DTrialPtr trial = m_trials.at(i);
        if (trial->stageZeroBased() != m_current_stage) {
            return n;
        }
        n += 1;
    }
    return n;
}


bool IDED3D::stageFailed() const
{
    const int n_this_stage = getNumTrialsThisStage();
    const bool failed = n_this_stage >= valueInt(FN_MAX_TRIALS_PER_STAGE);
    qDebug().nospace()
            << n_this_stage
            << " trials performed this stage (max="
            << valueInt(FN_MAX_TRIALS_PER_STAGE) << ") => stage failed = "
            << failed;
    return failed;
}


// ============================================================================
// Main task internals
// ============================================================================

void IDED3D::startTask()
{
    qDebug() << Q_FUNC_INFO;
    m_widget->setWidgetAsOnlyContents(m_graphics_widget, 0, false, false);
    if (valueBool(FN_DEBUG_DISPLAY_STIMULI_ONLY)) {
        debugDisplayStimuli();
        return;
    }

    // Store a version of the shape definitions, in JSON format
    setValue(FN_SHAPE_DEFINITIONS_SVG, IDED3DExemplars::allShapesAsJson());
    // Similarly for colours
    setValue(FN_COLOUR_DEFINITIONS_RGB, IDED3DExemplars::allColoursAsJson());
    onEditStarted();  // will have been stopped by the end of the questionnaire?

    // Double-check we have a PK before we create stages/trials
    save();

    // Make the stages
    makeStages();

    // Prep the sounds
    soundfunc::makeMediaPlayer(m_player_correct);
    soundfunc::makeMediaPlayer(m_player_incorrect);
    if (!m_player_correct || !m_player_incorrect) {
        return;
    }
    // ... for rationale, see QuAudioPlayer::makeWidget()
    m_player_correct->setMedia(uifunc::resourceUrl(SOUND_FILE_CORRECT));
    m_player_incorrect->setMedia(uifunc::resourceUrl(SOUND_FILE_INCORRECT));
    soundfunc::setVolume(m_player_correct, valueDouble(FN_VOLUME));
    soundfunc::setVolume(m_player_incorrect, valueDouble(FN_VOLUME));
    connect(m_player_correct.data(), &QMediaPlayer::mediaStatusChanged,
            this, &IDED3D::mediaStatusChanged);
    connect(m_player_incorrect.data(), &QMediaPlayer::mediaStatusChanged,
            this, &IDED3D::mediaStatusChanged);

    // Start
    ButtonAndProxy start = makeTextButton(
                m_scene,
                QRectF(0.2 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                       0.6 * SCENE_WIDTH, 0.1 * SCENE_HEIGHT),
                BASE_BUTTON_CONFIG,
                TextConst::touchToStart());
    CONNECT_BUTTON(start, nextTrial);
}


void IDED3D::nextTrial()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    Q_ASSERT(m_current_stage >= 0 && m_current_stage < m_stages.size());
    IDED3DStagePtr stage = m_stages.at(m_current_stage);
    clearScene();

    if (m_current_trial >= 0) {
        stage->recordTrialCompleted();
    }
    if (stagePassed()) {
        qDebug() << "Passed stage";
        stage->recordStageEnded(true);
        m_current_stage += 1;
    } else if (stageFailed()) {
        qDebug() << "Failed stage";
        stage->recordStageEnded(false);
        thanks();
        return;
    }
    // Finished last stage?
    if (m_current_stage >= m_stages.size() ||
            m_current_stage >= valueInt(FN_LAST_STAGE)) {
        qDebug() << "Completed task";
        thanks();
        return;
    }

    stage = m_stages.at(m_current_stage);  // a different one, perhaps
    qDebug().noquote() << stage->summary();
    m_current_trial += 1;
    IDED3DTrialPtr tr = IDED3DTrialPtr(new IDED3DTrial(
                                *stage, m_current_trial, m_app, m_db));
    m_trials.append(tr);
    Q_ASSERT(m_current_trial == m_trials.size() - 1);
    stage->setFirstTrialIfBlank(m_current_trial);
    startTrial();
}


void IDED3D::startTrial()
{
    qDebug() << Q_FUNC_INFO
             << "m_current_stage" << m_current_stage
             << "m_current_trial" << m_current_trial;
    Q_ASSERT(0 <= m_current_trial && m_current_trial < m_trials.size());
    IDED3DTrialPtr trial = m_trials.at(m_current_trial);
    qDebug().noquote() << trial->summary();

    // Two stimuli are shown for every trial. (So no need to record explicitly
    // the location that is chosen; that information is available from the fact
    // of having responded correctly or incorrectly.) Empty boxes are shown at
    // the other locations

    for (int l = 0; l < LOCATIONS.size(); ++l) {
        if (l == trial->correctLocation()) {
            showCompositeStimulus(
                        trial->correctShape(),
                        trial->correctColour(),
                        trial->correctNumber(),
                        trial->correctLocation(),
                        true);
        } else if (l == trial->incorrectLocation()) {
            showCompositeStimulus(
                        trial->incorrectShape(),
                        trial->incorrectColour(),
                        trial->incorrectNumber(),
                        trial->incorrectLocation(),
                        false);
        } else {
            showEmptyBox(l);
        }
    }
    if (valueBool(FN_OFFER_ABORT)) {
        ButtonConfig abort_cfg = BASE_BUTTON_CONFIG;
        abort_cfg.background_colour = ABORT_BUTTON_BACKGROUND;
        ButtonAndProxy abort_button = makeTextButton(
                    m_scene,
                    QRectF(0.01 * SCENE_WIDTH, 0.94 * SCENE_HEIGHT,
                           0.07 * SCENE_WIDTH, 0.05 * SCENE_HEIGHT),
                    abort_cfg,
                    TextConst::abort());
        CONNECT_BUTTON(abort_button, abort);
    }
    trial->recordTrialStart();
}


void IDED3D::recordResponse(const bool correct)
{
    qDebug() << Q_FUNC_INFO << "correct" << correct;
    Q_ASSERT(0 <= m_current_stage && m_current_stage < m_stages.size());
    IDED3DStagePtr stage = m_stages.at(m_current_stage);
    Q_ASSERT(0 <= m_current_trial && m_current_trial < m_trials.size());
    IDED3DTrialPtr trial = m_trials.at(m_current_trial);

    trial->recordResponse(correct);
    stage->recordResponse(correct);
    setValue(FN_LAST_TRIAL_COMPLETED, m_current_trial + 1);  // one-based
    showAnswer(correct);
}


void IDED3D::showAnswer(bool correct)
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO << "correct" << correct;
#endif
    const QString& text = correct ? TextConst::correct() : TextConst::wrong();
    const QColor& colour = correct ? CORRECT_BG_COLOUR : INCORRECT_BG_COLOUR;
    makeObscuringRect(m_scene, SCENE_RECT, FEEDBACK_OPACITY, colour);
    m_scene->addRect(ANSWER_BACKDROP_RECT, QPen(Qt::NoPen), QBrush(colour));
    makeText(m_scene, SCENE_CENTRE, BASE_TEXT_CONFIG, text);
    if (correct) {
        m_player_correct->play();  // on completion will go to mediaStatusChanged()
    } else {
        m_player_incorrect->play();  // on completion will go to mediaStatusChanged()
    }
}


void IDED3D::mediaStatusChanged(const QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
#ifdef DEBUG_STEP_DETAIL
        qDebug() << "Sound playback finished";
#endif
        waitAfterBeep();
    }
}


void IDED3D::waitAfterBeep()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    setTimeout(valueInt(FN_PAUSE_AFTER_BEEP_MS), &IDED3D::iti);
}


void IDED3D::iti()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    setTimeout(valueInt(FN_ITI_MS), &IDED3D::nextTrial);
}


void IDED3D::thanks()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    clearScene();
    ButtonAndProxy thx = makeTextButton(
                m_scene,
                QRectF(0.3 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                       0.4 * SCENE_WIDTH, 0.1 * SCENE_HEIGHT),
                BASE_BUTTON_CONFIG,
                TextConst::thankYouTouchToExit());
    CONNECT_BUTTON(thx, finish);
}


void IDED3D::abort()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    setValue(FN_ABORTED, true);
    Q_ASSERT(m_widget);
    onEditFinishedAbort();
    emit m_widget->finished();
}


void IDED3D::finish()
{
#ifdef DEBUG_STEP_DETAIL
    qDebug() << Q_FUNC_INFO;
#endif
    setValue(FN_FINISHED, true);
    Q_ASSERT(m_widget);
    onEditFinishedProperly();
    emit m_widget->finished();
}
