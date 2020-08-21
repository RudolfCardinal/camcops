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

#include "cardinalexpdetcommon.h"
#include <QObject>
#include "common/colourdefs.h"
#include "lib/uifunc.h"


namespace cardinalexpdetcommon
{

const int N_CUES_PER_MODALITY = 8;
const qreal MIN_INTENSITY = 0.0;
const qreal MAX_INTENSITY = 1.0;

const QString AUDITORY_BACKGROUND("A_background.wav");
const QStringList AUDITORY_CUES{
    "A_cue_00_pluck.wav",
    "A_cue_01_river.wav",
    "A_cue_02_bird.wav",
    "A_cue_03_morse.wav",
    "A_cue_04_cymbal.wav",
    "A_cue_05_match.wav",
    "A_cue_06_metal.wav",
    "A_cue_07_bach.wav",
};
const QStringList AUDITORY_TARGETS{
    "A_target_0_tone.wav",
    "A_target_1_voice.wav",
};
const QString VISUAL_BACKGROUND("V_background.png");
const QStringList VISUAL_CUES{
    "V_cue_00.png",
    "V_cue_01.png",
    "V_cue_02.png",
    "V_cue_03.png",
    "V_cue_04.png",
    "V_cue_05.png",
    "V_cue_06.png",
    "V_cue_07.png",
};
const QStringList VISUAL_TARGETS{
    "V_target_0_circle.png",
    "V_target_1_word.png",
};

const int MODALITY_AUDITORY = 0;
const int MODALITY_VISUAL = 1;

const int SOUNDTEST_VOLUME = 100;

// Graphics: positioning
const QRectF SCENE_RECT(0, 0, SCENE_WIDTH, SCENE_HEIGHT);
const QPointF SCENE_CENTRE(SCENE_WIDTH * 0.5, SCENE_HEIGHT * 0.5);
const qreal STIM_SIDE = 400;
// Keep stimuli above all buttons, to avoid screen smudging
const QRectF VISUAL_STIM_RECT(0.5 * SCENE_WIDTH - STIM_SIDE / 2.0,  // left
                              0.05 * SCENE_HEIGHT,  // top
                              STIM_SIDE,  // width
                              STIM_SIDE);  // height
const QRectF START_BUTTON_RECT(0.2 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                               0.6 * SCENE_WIDTH, 0.1 * SCENE_HEIGHT);
const QPointF PROMPT_CENTRE(0.5 * SCENE_WIDTH, 0.65 * SCENE_HEIGHT);
const qreal RESPONSE_BUTTON_TOP = 0.75 * SCENE_HEIGHT;
const qreal RESPONSE_BUTTON_HEIGHT = 0.2 * SCENE_HEIGHT;
const qreal RESPONSE_BUTTON_WIDTH = 0.2 * SCENE_WIDTH;
const QRectF NO_BUTTON_RECT(0.2 * SCENE_WIDTH, RESPONSE_BUTTON_TOP,
                            RESPONSE_BUTTON_WIDTH, RESPONSE_BUTTON_HEIGHT);
const QRectF YES_BUTTON_RECT(0.6 * SCENE_WIDTH, RESPONSE_BUTTON_TOP,
                             RESPONSE_BUTTON_WIDTH, RESPONSE_BUTTON_HEIGHT);
const QRectF ABORT_BUTTON_RECT(0.01 * SCENE_WIDTH, 0.94 * SCENE_HEIGHT,
                               0.07 * SCENE_WIDTH, 0.05 * SCENE_HEIGHT);
const QRectF THANKS_BUTTON_RECT(0.3 * SCENE_WIDTH, 0.6 * SCENE_HEIGHT,
                                0.4 * SCENE_WIDTH, 0.1 * SCENE_HEIGHT);

// Graphics: other
const QColor SCENE_BACKGROUND(QCOLOR_BLACK);  // try QCOLOR_LIGHTSALMON
const int BORDER_WIDTH_PX = 3;
const QColor BUTTON_BACKGROUND(QCOLOR_MEDIUMBLUE);
const QColor TEXT_COLOUR(QCOLOR_WHITE);
const QColor BUTTON_PRESSED_BACKGROUND(QCOLOR_OLIVE);
const QColor ABORT_BUTTON_BACKGROUND(QCOLOR_DARKRED);
const int TEXT_SIZE_PX = 20;  // will be scaled
const int BUTTON_RADIUS = 5;
const int PADDING = 5;
const Qt::Alignment BUTTON_TEXT_ALIGN = Qt::AlignCenter;
const Qt::Alignment TEXT_ALIGN = Qt::AlignCenter;
const QColor EDGE_COLOUR(QCOLOR_WHITE);
const QPen BORDER_PEN(QBrush(EDGE_COLOUR), BORDER_WIDTH_PX);
const ButtonConfig BASE_BUTTON_CONFIG(
        PADDING, TEXT_SIZE_PX, TEXT_COLOUR, BUTTON_TEXT_ALIGN,
        BUTTON_BACKGROUND, BUTTON_PRESSED_BACKGROUND,
        BORDER_PEN, BUTTON_RADIUS);

// WATCH OUT: anything using the clone() method must be in THIS FILE, not
// another .cpp file; see qt_notes.txt,
// "Problems with .cpp files containing const QObjects"
const ButtonConfig ABORT_BUTTON_CONFIG = BASE_BUTTON_CONFIG.clone()
        .setBackgroundColour(ABORT_BUTTON_BACKGROUND);
const TextConfig BASE_TEXT_CONFIG(TEXT_SIZE_PX, TEXT_COLOUR,
                                  static_cast<int>(SCENE_WIDTH), TEXT_ALIGN);
const QColor CONTINUE_BUTTON_BACKGROUND(QCOLOR_DARKGREEN);
const ButtonConfig CONTINUE_BUTTON_CONFIG = BASE_BUTTON_CONFIG.clone()
        .setBackgroundColour(CONTINUE_BUTTON_BACKGROUND);


QUrl urlFromStem(const QString& stem)
{
    return uifunc::resourceUrl(QString("/expdet/%1").arg(stem));
}


QString filenameFromStem(const QString& stem)
{
    return uifunc::resourceFilename(QString("/expdet/%1").arg(stem));
}


// ============================================================================
// Translatable text (ugly coding for Qt lupdate tool)
// ============================================================================

QString ExpDetTextConst::soundtestTitle()
{
    return tr("Sound test for Cardinal RN / Expectation–Detection task");
}


QString ExpDetTextConst::soundtestSubtitle()
{
    return tr(
        "Plays the auditory background sound. [Use maximum device volume. "
        "The sound should be 60.0 dB(A).]"
    );
}


QString ExpDetTextConst::configVisualTargetDurationS()
{
    return tr("Visual target duration (s) (e.g. 1.0):");
}


QString ExpDetTextConst::auditoryTarget0()
{
    return tr("tone (auditory target 0)");
}


QString ExpDetTextConst::auditoryTarget0Short()
{
    return tr("tone");
}


QString ExpDetTextConst::auditoryTarget1()
{
    return tr("voice (auditory target 1)");
}


QString ExpDetTextConst::auditoryTarget1Short()
{
    return tr("voice");
}


QString ExpDetTextConst::visualTarget0()
{
    return tr("circle (visual target 0)");
}


QString ExpDetTextConst::visualTarget0Short()
{
    return tr("circle");
}


QString ExpDetTextConst::visualTarget1()
{
    return tr("word (visual target 1)");
}


QString ExpDetTextConst::visualTarget1Short()
{
    return tr("word");
}


}  // namespace cardinalexpdetcommon
