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

#include "qolsg.h"
#include <functional>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QPushButton>
#include "common/colourdefs.h"
#include "common/textconst.h"
#include "lib/datetime.h"
#include "lib/stringfunc.h"
#include "maths/ccrandom.h"
#include "tasklib/taskfactory.h"
#include "widgets/adjustablepie.h"
#include "widgets/openablewidget.h"
using datetime::now;
using graphicsfunc::AdjustablePieAndProxy;
using graphicsfunc::ButtonAndProxy;
using graphicsfunc::makeAdjustablePie;
using graphicsfunc::makeText;
using graphicsfunc::makeTextButton;
using stringfunc::replaceFirst;


// ============================================================================
// Constants
// ============================================================================

// Table name
const QString QolSG::QOLSG_TABLENAME("qolsg");

namespace qolsgconst {
// See namespace rationale in ided3d.cpp

// Fieldnames
const QString FN_CATEGORY_START_TIME("category_start_time");
const QString FN_CATEGORY_RESPONDED("category_responded");
const QString FN_CATEGORY_RESPONSE_TIME("category_response_time");
const QString FN_CATEGORY_CHOSEN("category_chosen");
const QString FN_GAMBLE_FIXED_OPTION("gamble_fixed_option");
const QString FN_GAMBLE_LOTTERY_OPTION_P("gamble_lottery_option_p");
const QString FN_GAMBLE_LOTTERY_OPTION_Q("gamble_lottery_option_q");
const QString FN_GAMBLE_LOTTERY_ON_LEFT("gamble_lottery_on_left");
const QString FN_GAMBLE_STARTING_P("gamble_starting_p");
const QString FN_GAMBLE_START_TIME("gamble_start_time");
const QString FN_GAMBLE_RESPONDED("gamble_responded");
const QString FN_GAMBLE_RESPONSE_TIME("gamble_response_time");
const QString FN_GAMBLE_P("gamble_p");
const QString FN_UTILITY("utility");

// Strings
const QString TX_UTILITY("Utility");
const QString TX_INITIAL_INSTRUCTION(
        "Quality of Life Standard Gamble<br><br><br>"
        "<b>Please choose the statement that best describes your current health "
        "state:</b>");
const QString TX_CURRENT_STATE("Current state");
const QString TX_DEAD("Dead");
const QString TX_HEALTHY("Healthy");
const QString TX_INDIFFERENT("Both wheels seem about equal to me now");
const QString TX_H_ABOVE_1("I am better than 100% healthy");
const QString TX_H_0_TO_1("I am somewhere from 0% to 100% healthy");
const QString TX_H_BELOW_0("My current state is worse than being dead");
const QString TX_LEFT("left");
const QString TX_RIGHT("right");
const QString TX_INSTRUCTION_PREFIX(
        "<b>Suppose you are offered two alternatives, represented by the two "
        "wheels below.</b>");
const QString TX_INSTRUCTION_MEDIUM(
        "The FIXEDSIDE wheel represents you remaining in your current state "
        "of health for the rest of your life.\n"
        "The LOTTERYSIDE wheel represents an experimental treatment. There is "
        "a chance that it will return you to full health for the rest of "
        "your life. However, there is also a chance that it will kill you "
        "instantly.");
const QString TX_INSTRUCTION_LOW(
        "The FIXEDSIDE wheel represents a poison that would kill you "
        "instantly.\n"
        "The LOTTERYSIDE wheel represents an experimental treatment. There is "
        "a chance that it will return you to full health for the rest of your "
        "life. However, there is also a chance that you will remain in your "
        "current state of health for the rest of your life.");
const QString TX_INSTRUCTION_HIGH(
        "The FIXEDSIDE wheel represents a medicine that would give you normal "
        "full health for the rest of your life.\n"
        "The LOTTERYSIDE wheel represents an experimental treatment. There is "
        "a chance that it will keep you in your current state of health for "
        "the rest of your life. However, there is also a chance that it will "
        "kill you instantly.");
const QString TX_INSTRUCTION_SUFFIX(
        "<b>Please drag the red pointer to adjust the chances on the "
        "LOTTERYSIDE wheel, until the two wheels seem EQUAL IN VALUE to you. "
        "Then press the green button.</b>");
const QString TX_THANKS("Thank you! Please touch here to exit.");

// Bits to replace in the string above:
const QString FIXEDSIDE("FIXEDSIDE");
const QString LOTTERYSIDE("LOTTERYSIDE");

// Parameters/result values
const QString CHOICE_HIGH("high");
const QString CHOICE_MEDIUM("medium");
const QString CHOICE_LOW("low");
const QString LOTTERY_OPTION_CURRENT("current");
const QString LOTTERY_OPTION_HEALTHY("healthy");
const QString LOTTERY_OPTION_DEAD("dead");

// Graphics

const qreal SCENE_WIDTH = 1000;
const qreal SCENE_HEIGHT = 750;  // 4:3 aspect ratio
const int BORDER_WIDTH_PX = 3;
const QColor EDGE_COLOUR(QCOLOR_WHITE);
const QColor SCENE_BACKGROUND(QCOLOR_BLACK);  // try also QCOLOR_LIGHTSALMON
const QColor BUTTON_BACKGROUND(QCOLOR_BLUE);
const QColor TEXT_COLOUR(QCOLOR_WHITE);
const QColor BUTTON_PRESSED_BACKGROUND(QCOLOR_OLIVE);
const QColor BACK_BUTTON_BACKGROUND(QCOLOR_DARKRED);
const int TEXT_SIZE_PX = 20;  // will be scaled
const int BUTTON_RADIUS = 5;
const int PADDING = 5;
const Qt::Alignment BUTTON_TEXT_ALIGN = Qt::AlignCenter;
const Qt::Alignment TEXT_ALIGN = Qt::AlignCenter;  // Qt::AlignLeft | Qt::AlignTop;

const qreal EDGESPACE_FRAC = 0.01; // left, right
const qreal EDGESPACE_AT_STIM = 0.05;
const qreal CENTRESPACE_FRAC = 0.10;
const qreal STIMDIAMETER_FRAC = 0.5 - EDGESPACE_AT_STIM - (0.5 * CENTRESPACE_FRAC);
const qreal STIMDIAMETER = SCENE_WIDTH * STIMDIAMETER_FRAC;
const qreal STIM_VCENTRE = 0.60 * SCENE_HEIGHT;
const qreal LEFT_STIM_CENTRE = SCENE_WIDTH * (0.5 - (0.5 * CENTRESPACE_FRAC +
                                                     0.5 * STIMDIAMETER_FRAC));
const qreal RIGHT_STIM_CENTRE = SCENE_WIDTH * (0.5 + (0.5 * CENTRESPACE_FRAC +
                                                      0.5 * STIMDIAMETER_FRAC));

const QRectF SCENE_RECT(0, 0, SCENE_WIDTH, SCENE_HEIGHT);
const QPen BORDER_PEN(QBrush(EDGE_COLOUR), BORDER_WIDTH_PX);
const ButtonConfig BASE_BUTTON_CONFIG(PADDING,
                                      TEXT_SIZE_PX,
                                      TEXT_COLOUR,
                                      BUTTON_TEXT_ALIGN,
                                      BUTTON_BACKGROUND,
                                      BUTTON_PRESSED_BACKGROUND,
                                      BORDER_PEN,
                                      BUTTON_RADIUS);
const TextConfig BASE_TEXT_CONFIG(TEXT_SIZE_PX, TEXT_COLOUR,
                                  static_cast<int>(SCENE_WIDTH), TEXT_ALIGN);
// YOU CANNOT INSTANTIATE A STATIC QFont() OBJECT BEFORE QT IS FULLY
// FIRED UP; QFont::QFont() calls QFontPrivate::QFontPrivate()) calls
// QGuiApplication::primaryScreen() which causes a segmentation fault.
// We could deal with this by
// (a) only ever dynamically creating a TextConfig etc., or
// (b) taking the QFont out of those objects and moving them into
//     the makeText() call.
// For safety, went with (b).

const QColor CURRENT_STATE_TEXT_COLOUR(QCOLOR_YELLOW);
const QolSG::LotteryOption TESTSTATE(
        TX_CURRENT_STATE, QCOLOR_GREY, CURRENT_STATE_TEXT_COLOUR);
const QolSG::LotteryOption DEAD(
        TX_DEAD, QCOLOR_BLACK, QCOLOR_RED);
const QolSG::LotteryOption HEALTHY(
        TX_HEALTHY, QCOLOR_BLUE, QCOLOR_WHITE);

// AdjustablePie settings:
const qreal PIE_FRAC = 0.5;
const qreal CURSOR_FRAC = 0.25;
const qreal LABEL_CURSOR_GAP_FRAC = 0.05;
const int PIE_CURSOR_ANGLE = 60;
const int PIE_REPORTING_DELAY_MS = 10;
const int PIE_BASE_HEADING = 180;
const PenBrush CURSOR_PENBRUSH(QPen(Qt::NoPen), QBrush(QCOLOR_RED));
const PenBrush CURSOR_ACTIVE_PENBRUSH(QPen(QBrush(QCOLOR_ORANGE), 3.0),
                                      QBrush(QCOLOR_RED));
const QPen SECTOR_PEN(QBrush(QCOLOR_WHITE), 3.0);

}  // namespace qolsgconst
using namespace qolsgconst;


// ============================================================================
// Initialization
// ============================================================================

void initializeQolSG(TaskFactory& factory)
{
    static TaskRegistrar<QolSG> registered(factory);
}


QolSG::QolSG(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, QOLSG_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_pie_touched_at_least_once(false),
    m_last_p(0)
{
    addField(FN_CATEGORY_START_TIME, QVariant::DateTime);
    addField(FN_CATEGORY_RESPONDED, QVariant::Bool);
    addField(FN_CATEGORY_RESPONSE_TIME, QVariant::DateTime);
    addField(FN_CATEGORY_CHOSEN, QVariant::String);
    addField(FN_GAMBLE_FIXED_OPTION, QVariant::String);
    addField(FN_GAMBLE_LOTTERY_OPTION_P, QVariant::String);
    addField(FN_GAMBLE_LOTTERY_OPTION_Q, QVariant::String);
    addField(FN_GAMBLE_LOTTERY_ON_LEFT, QVariant::Bool);
    addField(FN_GAMBLE_STARTING_P, QVariant::Double);
    addField(FN_GAMBLE_START_TIME, QVariant::DateTime);
    addField(FN_GAMBLE_RESPONDED, QVariant::Bool);
    addField(FN_GAMBLE_RESPONSE_TIME, QVariant::DateTime);
    addField(FN_GAMBLE_P, QVariant::Double);
    addField(FN_UTILITY, QVariant::Double);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString QolSG::shortname() const
{
    return "QoL-SG";
}


QString QolSG::longname() const
{
    return tr("Quality of Life: Standard Gamble");
}


QString QolSG::description() const
{
    return tr("Standard-gamble measure of quality of life.");
}


QString QolSG::infoFilenameStem() const
{
    return "qol";
}


// ============================================================================
// Instance info
// ============================================================================

bool QolSG::isComplete() const
{
    return !valueIsNull(FN_UTILITY);
}


QStringList QolSG::summary() const
{
    return QStringList{stringfunc::standardResult(TX_UTILITY,
                                                  prettyValue(FN_UTILITY, 3))};
}


QStringList QolSG::detail() const
{
    return completenessInfo() + recordSummaryLines();
}


OpenableWidget* QolSG::editor(const bool read_only)
{
    if (read_only) {
        qWarning() << "Task not editable! Shouldn't have got here.";
        return nullptr;
    }

    m_scene = new QGraphicsScene(SCENE_RECT);
    m_scene->setBackgroundBrush(QBrush(SCENE_BACKGROUND));
    m_widget = makeGraphicsWidgetForImmediateEditing(m_scene, SCENE_BACKGROUND);

    startTask();

    return m_widget;
}


// ============================================================================
// Internals
// ============================================================================

// MUST USE Qt::QueuedConnection - see comments in clearScene()
#define CONNECT_BUTTON(b, funcname) \
    connect((b).button, &QPushButton::clicked, \
            this, &QolSG::funcname, \
            Qt::QueuedConnection)
// To use a Qt::ConnectionType parameter with a functor, we need a context
// See http://doc.qt.io/qt-5/qobject.html#connect-5
// That's the reason for the extra "this":
#define CONNECT_BUTTON_PARAM(b, funcname, param) \
    connect((b).button, &QPushButton::clicked, this, \
            std::bind(&QolSG::funcname, this, param), \
            Qt::QueuedConnection)


void QolSG::startTask()
{
    askCategory();
}


void QolSG::askCategory()
{
    Q_ASSERT(m_scene);
    clearScene();
    makeText(m_scene,
             QPointF(0.5 * SCENE_WIDTH, 0.15 * SCENE_HEIGHT),
             BASE_TEXT_CONFIG,
             TX_INITIAL_INSTRUCTION);
    const qreal button_left = 0.2 * SCENE_WIDTH;
    const qreal button_width = 0.6 * SCENE_WIDTH;
    const qreal button_height = 0.1  * SCENE_HEIGHT;
    ButtonAndProxy h = makeTextButton(
                m_scene,
                QRectF(button_left, 0.35 * SCENE_HEIGHT,
                       button_width, button_height),
                BASE_BUTTON_CONFIG,
                TX_H_ABOVE_1);
    ButtonAndProxy m = makeTextButton(
                m_scene,
                QRectF(button_left, 0.55 * SCENE_HEIGHT,
                       button_width, button_height),
                BASE_BUTTON_CONFIG,
                TX_H_0_TO_1);
    ButtonAndProxy l = makeTextButton(
                m_scene,
                QRectF(button_left, 0.75 * SCENE_HEIGHT,
                       button_width, button_height),
                BASE_BUTTON_CONFIG,
                TX_H_BELOW_0);
    CONNECT_BUTTON_PARAM(h, giveChoice, CHOICE_HIGH);
    CONNECT_BUTTON_PARAM(m, giveChoice, CHOICE_MEDIUM);
    CONNECT_BUTTON_PARAM(l, giveChoice, CHOICE_LOW);

    setValue(FN_CATEGORY_START_TIME, now());
    save();
}


void QolSG::thanks()
{
    Q_ASSERT(m_scene);
    clearScene();
    ButtonAndProxy t = makeTextButton(
                m_scene,
                QRectF(0.3 * SCENE_WIDTH, 0.4 * SCENE_HEIGHT,
                       0.4 * SCENE_WIDTH, 0.2 * SCENE_HEIGHT),
                BASE_BUTTON_CONFIG,
                TX_THANKS);
    CONNECT_BUTTON(t, finished);
}


void QolSG::clearScene()
{
    // CAUTION REQUIRED HERE.
    // (1) m_scene is a QPointer, so be careful to use ->clear() not .clear()!
    // (2) If you call this from within a QGraphicsScene event, you will get
    //     a segfault if you have a standard Qt signal/slot connection. You
    //     need to use a QueuedConnection.
    //     http://stackoverflow.com/questions/20387679/clear-widget-in-a-qgraphicsscene-crash
    m_scene->clear();  // be careful not to do m_scene.clear() instead!
}


// ============================================================================
// Signal handlers
// ============================================================================

void QolSG::giveChoice(const QString& category_chosen)
{
    qDebug() << Q_FUNC_INFO << category_chosen;
    setValue(FN_CATEGORY_RESPONSE_TIME, now());
    setValue(FN_CATEGORY_RESPONDED, true);
    setValue(FN_CATEGORY_CHOSEN, category_chosen);
    const bool lottery_on_left = false;  // coin();
    // task is more confusing with lots of left/right references. Fix the lottery on the right.
    setValue(FN_GAMBLE_LOTTERY_ON_LEFT, lottery_on_left);
    clearScene();

    qreal h = 0;
    qreal p = 0;
    LotteryOption option1;
    LotteryOption option2;
    LotteryOption option_fixed;

    if (category_chosen == CHOICE_HIGH) {

        h = 1.5;
        // RNC: h > 1, since we should consider mania...
        // If indifferent, p * h + (1 - p) * 0 = 1 * 1  =>  h = 1/p  =>  p = 1/h
        p = 1 / h;
        setValue(FN_GAMBLE_LOTTERY_OPTION_P, LOTTERY_OPTION_CURRENT);
        setValue(FN_GAMBLE_LOTTERY_OPTION_Q, LOTTERY_OPTION_DEAD);
        setValue(FN_GAMBLE_FIXED_OPTION, LOTTERY_OPTION_HEALTHY);
        option1 = TESTSTATE;
        option2 = DEAD;
        option_fixed = HEALTHY;
        // If the subject chooses A, their utility is HIGHER than h.
        // However, we'll ask them to aim for indifference directly -- simpler.

    } else if (category_chosen == CHOICE_MEDIUM) {

        h = 0.5;
        // NORMAL STATE! 0 <= h <= 1
        // If indifferent, h = p
        // Obvious derivation: p * 1 + (1 - p) * 0 = 1 * h
        p = h;
        setValue(FN_GAMBLE_LOTTERY_OPTION_P, LOTTERY_OPTION_HEALTHY);
        setValue(FN_GAMBLE_LOTTERY_OPTION_Q, LOTTERY_OPTION_DEAD);
        setValue(FN_GAMBLE_FIXED_OPTION, LOTTERY_OPTION_CURRENT);
        option1 = HEALTHY;
        option2 = DEAD;
        option_fixed = TESTSTATE;
        // If the subject chooses A, their utility is LOWER than h.
        // However, we'll ask them to aim for indifference directly -- simpler.

    } else if (category_chosen == CHOICE_LOW) {

        h = -0.5;
        // h < 0: if indifferent here, current state is worse than death
        // If indifferent, Torrance gives h = -p / (1 - p) = p / (p - 1)  =>  p = h / (h - 1)
        // Derivation: p * 1 + (1 - p) * h = 1 * 0  =>  h = -p / (1-p)  => etc.
        p = h / (h - 1);
        setValue(FN_GAMBLE_LOTTERY_OPTION_P, LOTTERY_OPTION_HEALTHY);
        setValue(FN_GAMBLE_LOTTERY_OPTION_Q, LOTTERY_OPTION_CURRENT);
        setValue(FN_GAMBLE_FIXED_OPTION, LOTTERY_OPTION_DEAD);
        option1 = HEALTHY;
        option2 = TESTSTATE;
        option_fixed = DEAD;
        // If the subject chooses A, their utility is HIGHER than h.
        // Example: h = -1, so p = 0.5: will be indifferent between {0.5 health, 0.5 current} versus {1 death}
        // Example: h = -0.1, so p = 0.0909: will be approx. indifferent between {0.9 health, 0.1 current} versus {1 death}
        // However, we'll ask them to aim for indifference directly -- simpler.

    } else {
        qWarning() << "Bad category_chosen:" << category_chosen;
    }

    showGambleInstruction(lottery_on_left, category_chosen);
    showFixed(!lottery_on_left, option_fixed);
    showLottery(lottery_on_left, option1, option2, p);
    setValue(FN_GAMBLE_STARTING_P, p);

    // Back button
    ButtonConfig back_button_cfg = BASE_BUTTON_CONFIG;
    back_button_cfg.background_colour = BACK_BUTTON_BACKGROUND;
    ButtonAndProxy b = makeTextButton(
                m_scene,
                QRectF(0.05 * SCENE_WIDTH, 0.94 * SCENE_HEIGHT,
                       0.1 * SCENE_WIDTH, 0.05 * SCENE_HEIGHT),
                back_button_cfg,
                TextConst::back());
    CONNECT_BUTTON(b, askCategory);

    // Off we go
    setValue(FN_GAMBLE_START_TIME, now());
    save();
}


AdjustablePieAndProxy QolSG::makePie(const QPointF& centre,
                                     const int n_sectors)
{
    const qreal diameter = STIMDIAMETER;
    const qreal radius = diameter / 2;
    AdjustablePieAndProxy pp = makeAdjustablePie(m_scene, centre,
                                                 n_sectors, diameter);
    AdjustablePie* pie = pp.pie;
    pie->setBackgroundBrush(QBrush(SCENE_BACKGROUND));
    pie->setBaseCompassHeading(PIE_BASE_HEADING);
    pie->setSectorRadius(radius * PIE_FRAC);
    pie->setCursorRadius(radius * PIE_FRAC,
                         radius * (PIE_FRAC + CURSOR_FRAC));
    pie->setCursorAngle(PIE_CURSOR_ANGLE);
    pie->setLabelStartRadius(radius * (PIE_FRAC + CURSOR_FRAC +
                                       LABEL_CURSOR_GAP_FRAC));
    pie->setLabelRotation(true);
    pie->setReportingDelay(PIE_REPORTING_DELAY_MS);

    QFont font;
    font.setBold(true);
    font.setPixelSize(TEXT_SIZE_PX);
    pie->setOuterLabelFont(font);
    pie->setCentreLabelFont(font);

    if (n_sectors > 1) {
        pie->setCursorPenBrushes({CURSOR_PENBRUSH});
        pie->setCursorActivePenBrushes({CURSOR_ACTIVE_PENBRUSH});
    }

    return pp;
}


void QolSG::showFixed(const bool left, const LotteryOption& option)
{
    const QPointF lottery_centre(left ? LEFT_STIM_CENTRE : RIGHT_STIM_CENTRE,
                                 STIM_VCENTRE);
    AdjustablePieAndProxy pp = makePie(lottery_centre, 1);
    pp.pie->setProportions({1.0});
    pp.pie->setSectorPenBrushes({{SECTOR_PEN, QBrush(option.fill_colour)}});
    pp.pie->setCentreLabel(option.label);
    pp.pie->setCentreLabelColour(option.text_colour);
}


void QolSG::showLottery(const bool left, const LotteryOption& option1,
                        const LotteryOption& option2, const qreal starting_p)
{
    const QPointF lottery_centre(left ? LEFT_STIM_CENTRE : RIGHT_STIM_CENTRE,
                                 STIM_VCENTRE);
    AdjustablePieAndProxy pp = makePie(lottery_centre, 2);
    m_pie = pp.pie;
    m_pie->setProportions({starting_p, 1.0 - starting_p});
    m_pie->setSectorPenBrushes({{SECTOR_PEN, QBrush(option1.fill_colour)},
                                {SECTOR_PEN, QBrush(option2.fill_colour)}});
    m_pie->setLabels({option1.label, option2.label});
    m_pie->setLabelColours({option1.text_colour, option2.text_colour});
    m_pie_touched_at_least_once = false;
    connect(m_pie.data(), &AdjustablePie::proportionsChanged,
            this, &QolSG::pieAdjusted);
}


void QolSG::showGambleInstruction(const bool lottery_on_left,
                                  const QString& category_chosen)
{
    qDebug() << Q_FUNC_INFO << lottery_on_left << category_chosen;

    QString instruction;

    if (category_chosen == CHOICE_HIGH) {
        instruction = TX_INSTRUCTION_HIGH;
    } else if (category_chosen == CHOICE_MEDIUM) {
        instruction = TX_INSTRUCTION_MEDIUM;
    } else if (category_chosen == CHOICE_LOW) {
        instruction = TX_INSTRUCTION_LOW;
    } else {
        qWarning() << Q_FUNC_INFO
                   << "- duff category_chosen:" << category_chosen;
        return;
    }

    const QString fixed_side = lottery_on_left ? TX_RIGHT : TX_LEFT;
    const QString lottery_side = lottery_on_left ? TX_LEFT : TX_RIGHT;

    replaceFirst(instruction, FIXEDSIDE, fixed_side);
    replaceFirst(instruction, LOTTERYSIDE, lottery_side);
    QString suffix = TX_INSTRUCTION_SUFFIX;
    replaceFirst(suffix, FIXEDSIDE, fixed_side);
    replaceFirst(suffix, LOTTERYSIDE, lottery_side);

    TextConfig tc = BASE_TEXT_CONFIG;
    tc.width = (1 - 2 * EDGESPACE_FRAC) * SCENE_WIDTH;
    tc.alignment = Qt::AlignLeft | Qt::AlignTop;
    const qreal left = EDGESPACE_FRAC * SCENE_WIDTH;
    QString sep("<br><br>");

    makeText(m_scene,
             QPointF(left, left),
             tc,
             TX_INSTRUCTION_PREFIX + sep + instruction + sep + suffix);
}


void QolSG::pieAdjusted(const QVector<qreal>& proportions)
{
    lotteryTouched(proportions.at(0));
}


void QolSG::lotteryTouched(const qreal p)
{
    if (!m_pie_touched_at_least_once) {
        // Make the "indifference" button appear only after the twirler has been set.
        m_pie_touched_at_least_once = true;
        ButtonConfig indiff_button_cfg = BASE_BUTTON_CONFIG;
        indiff_button_cfg.background_colour = QCOLOR_DARKGREEN;
        ButtonAndProxy c = makeTextButton(
                    m_scene,
                    QRectF(0.3 * SCENE_WIDTH, 0.90 * SCENE_HEIGHT,
                           0.4 * SCENE_WIDTH, 0.09 * SCENE_HEIGHT),
                    indiff_button_cfg,
                    TX_INDIFFERENT);
        CONNECT_BUTTON(c, recordChoice);
    }
    m_last_p = p;
}


void QolSG::recordChoice()
{
    const qreal p = m_last_p;
    qDebug() << Q_FUNC_INFO << "p =" << p;
    setValue(FN_GAMBLE_RESPONSE_TIME, now());
    setValue(FN_GAMBLE_RESPONDED, true);
    setValue(FN_GAMBLE_P, p);
    QString category_chosen = valueString(FN_CATEGORY_CHOSEN);
    qreal utility = 0;
    if (category_chosen == CHOICE_HIGH) {
        utility = 1 / p;
    } else if (category_chosen == CHOICE_MEDIUM) {
        utility = p;
    } else if (category_chosen == CHOICE_LOW) {
        utility = -p / (1 - p);
    } else {
        qWarning() << "Bad category_chosen:" << category_chosen;
    }
    qDebug() << Q_FUNC_INFO << "utility =" << p;
    setValue(FN_UTILITY, utility);
    save();
    thanks();
}


void QolSG::finished()
{
    Q_ASSERT(m_widget);
    onEditFinishedProperly();
    emit m_widget->finished();
}
