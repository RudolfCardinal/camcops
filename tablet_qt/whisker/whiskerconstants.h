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

#include <QColor>
#include <QMap>
#include <QRegularExpression>
#include <QString>

namespace whiskerconstants {

// Constants for talking to a Whisker server.

// ============================================================================
// Enums
// ============================================================================

enum class ResetState {
    // When Whisker quits, what state should a digital line be left in?
    Input,  // it's an input line; not applicable
    On,  // turn it on
    Off,  // turn it off
    Leave,  // leave it in the state it is at the time
};

enum class LineEventType {
    // Should line events trigger upon "on" transitions, "off" transitions, or
    // both?
    On,
    Off,
    Both,
};

enum class SafetyState {
    // If a safety timer is set on an output line, what state should it be put
    // into when the timer elapses (due to inactivity)?
    Off,
    On,
};

enum class DocEventType {
    // What mouse/touch events should a display object respond to?
    MouseDown,
    MouseUp,
    MouseDoubleClick,
    MouseMove,
    TouchDown,
    TouchUp,
    TouchMove,
};

enum class KeyEventType {
    // What keystroke actions should a key event respond to?
    None,
    Down,
    Up,
    Both,
};

enum class ToneType {
    // Sound wave types for tone generation.
    Sine,
    Sawtooth,
    Square,
    Tone,
};

enum class VerticalAlign {
    // Vertical alignment options for graphical objects.
    Top,
    Middle,
    Bottom,
};

enum class TextVerticalAlign {
    // Vertical alignment options for text objects.
    Top,
    Middle,
    Bottom,
    Baseline,
};

enum class HorizontalAlign {
    // Horizontal alignment options for graphical objects.
    Left,
    Centre,
    Right,
};

enum class TextHorizontalAlign {
    // Horizontal alignment options for text objects.
    Left,
    Centre,
    Right,
};

enum class VideoPlayMode {
    // When videos are created, should they start playing straight away (etc.)?
    Wait,
    Immediate,
    WhenVisible,
};

enum class PenStyle {
    // Styles for pens.
    Solid,
    Dash,
    Dot,
    DashDot,
    DashDotDot,
    Null,
    InsideFrame,
};

enum class BrushStyle {
    // Styles for brushes.
    Hollow,
    Solid,
    Hatched,
};

enum class BrushHatchStyle {
    // Styles for hatched brushes.
    Horizontal,  // -----
    Vertical,  //   |||||
    FDiagonal,  //  \\\\\ (see WinGDI.h)
    BDiagonal,  //  ///// (see WinGDI.h)
    Cross,  //      +++++
    DiagCross,  //  xxxxx
};


// ============================================================================
// API constants
// ============================================================================

// ----------------------------------------------------------------------------
// Network config
// ----------------------------------------------------------------------------

extern const QString WHISKER_DEFAULT_HOST;
extern const quint16 WHISKER_DEFAULT_PORT;  // IANA registered port number
extern const int WHISKER_DEFAULT_TIMEOUT_MS;

// ----------------------------------------------------------------------------
// Interface basics
// ----------------------------------------------------------------------------

extern const QString SPACE;
extern const QString EOL;  // Whisker sends (and accepts) LF between responses.

// ----------------------------------------------------------------------------
// Server -> client
// ----------------------------------------------------------------------------

extern const QRegularExpression IMMPORT_REGEX;
extern const QRegularExpression CODE_REGEX;
extern const QRegularExpression TIMESTAMP_REGEX;

extern const QString RESPONSE_SUCCESS;
extern const QString RESPONSE_FAILURE;
extern const QString PING;
extern const QString PING_ACK;

extern const QRegularExpression EVENT_REGEX;
extern const QRegularExpression KEY_EVENT_REGEX;
extern const QRegularExpression CLIENT_MESSAGE_REGEX;
extern const QRegularExpression INFO_REGEX;
extern const QRegularExpression WARNING_REGEX;
extern const QRegularExpression SYNTAX_ERROR_REGEX;
extern const QRegularExpression ERROR_REGEX;

extern const QString EVENT_PREFIX;
extern const QString KEY_EVENT_PREFIX;
extern const QString CLIENT_MESSAGE_PREFIX;
extern const QString INFO_PREFIX;
extern const QString WARNING_PREFIX;
extern const QString SYNTAX_ERROR_PREFIX;
extern const QString ERROR_PREFIX;

extern const QString MSG_AUTHENTICATE_CHALLENGE;
extern const QString MSG_DURATION;
extern const QString MSG_EXTENT;
extern const QString MSG_KEYEVENT_UP;
extern const QString MSG_KEYEVENT_DOWN;
extern const QString MSG_SIZE;
extern const QString MSG_VIDEO_TIME;

// ----------------------------------------------------------------------------
// Client -> server
// ----------------------------------------------------------------------------

// Commands
extern const QString CMD_AUDIO_CLAIM;
extern const QString CMD_AUDIO_GET_SOUND_LENGTH;
extern const QString CMD_AUDIO_LOAD_SOUND;
extern const QString CMD_AUDIO_LOAD_TONE;
extern const QString CMD_AUDIO_PLAY_FILE;
extern const QString CMD_AUDIO_PLAY_SOUND;
extern const QString CMD_AUDIO_RELINQUISH_ALL;
extern const QString CMD_AUDIO_SET_ALIAS;
extern const QString CMD_AUDIO_SET_SOUND_VOLUME;
extern const QString CMD_AUDIO_SILENCE_ALL_DEVICES;
extern const QString CMD_AUDIO_SILENCE_DEVICE;
extern const QString CMD_AUDIO_STOP_SOUND;
extern const QString CMD_AUDIO_UNLOAD_ALL;
extern const QString CMD_AUDIO_UNLOAD_SOUND;
extern const QString CMD_AUTHENTICATE;
extern const QString CMD_AUTHENTICATE_RESPONSE;
extern const QString CMD_CLAIM_GROUP;
extern const QString CMD_CLIENT_NUMBER;
extern const QString CMD_DISPLAY_ADD_OBJECT;
extern const QString CMD_DISPLAY_BLANK;
extern const QString CMD_DISPLAY_BRING_TO_FRONT;
extern const QString CMD_DISPLAY_CACHE_CHANGES;
extern const QString CMD_DISPLAY_CLAIM;
extern const QString CMD_DISPLAY_CLEAR_BACKGROUND_EVENT;
extern const QString CMD_DISPLAY_CLEAR_EVENT;
extern const QString CMD_DISPLAY_CREATE_DEVICE;
extern const QString CMD_DISPLAY_CREATE_DOCUMENT;
extern const QString CMD_DISPLAY_DELETE_DEVICE;
extern const QString CMD_DISPLAY_DELETE_DOCUMENT;
extern const QString CMD_DISPLAY_DELETE_OBJECT;
extern const QString CMD_DISPLAY_EVENT_COORDS;
extern const QString CMD_DISPLAY_GET_DOCUMENT_SIZE;
extern const QString CMD_DISPLAY_GET_OBJECT_EXTENT;
extern const QString CMD_DISPLAY_GET_SIZE;
extern const QString CMD_DISPLAY_KEYBOARD_EVENTS;
extern const QString CMD_DISPLAY_RELINQUISH_ALL;
extern const QString CMD_DISPLAY_SCALE_DOCUMENTS;
extern const QString CMD_DISPLAY_SEND_TO_BACK;
extern const QString CMD_DISPLAY_SET_ALIAS;
extern const QString CMD_DISPLAY_SET_AUDIO_DEVICE;
extern const QString CMD_DISPLAY_SET_BACKGROUND_COLOUR;
extern const QString CMD_DISPLAY_SET_BACKGROUND_EVENT;
extern const QString CMD_DISPLAY_SET_DOCUMENT_SIZE;
extern const QString CMD_DISPLAY_SET_EVENT;
extern const QString CMD_DISPLAY_SET_OBJ_EVENT_TRANSPARENCY;
extern const QString CMD_DISPLAY_SHOW_CHANGES;
extern const QString CMD_DISPLAY_SHOW_DOCUMENT;
extern const QString CMD_LINE_CLAIM;
extern const QString CMD_LINE_CLEAR_ALL_EVENTS;
extern const QString CMD_LINE_CLEAR_EVENT;
extern const QString CMD_LINE_CLEAR_EVENTS_BY_LINE;
extern const QString CMD_LINE_CLEAR_SAFETY_TIMER;
extern const QString CMD_LINE_READ_STATE;
extern const QString CMD_LINE_RELINQUISH_ALL;
extern const QString CMD_LINE_SET_ALIAS;
extern const QString CMD_LINE_SET_EVENT;
extern const QString CMD_LINE_SET_SAFETY_TIMER;
extern const QString CMD_LINE_SET_STATE;
extern const QString CMD_LINK;
extern const QString CMD_LOG_CLOSE;
extern const QString CMD_LOG_OPEN;
extern const QString CMD_LOG_PAUSE;
extern const QString CMD_LOG_RESUME;
extern const QString CMD_LOG_SET_OPTIONS;
extern const QString CMD_LOG_WRITE;
extern const QString CMD_PERMIT_CLIENT_MESSAGES;
extern const QString CMD_REPORT_COMMENT;
extern const QString CMD_REPORT_NAME;
extern const QString CMD_REPORT_STATUS;
extern const QString CMD_REQUEST_TIME;
extern const QString CMD_RESET_CLOCK;
extern const QString CMD_SEND_TO_CLIENT;
extern const QString CMD_SET_MEDIA_DIRECTORY;
extern const QString CMD_SHUTDOWN;
extern const QString CMD_TEST_NETWORK_LATENCY;
extern const QString CMD_TIMER_CLEAR_ALL_EVENTS;
extern const QString CMD_TIMER_CLEAR_EVENT;
extern const QString CMD_TIMER_SET_EVENT;
extern const QString CMD_TIMESTAMPS;
extern const QString CMD_VERSION;
extern const QString CMD_VIDEO_GET_DURATION;
extern const QString CMD_VIDEO_GET_TIME;
extern const QString CMD_VIDEO_PAUSE;
extern const QString CMD_VIDEO_PLAY;
extern const QString CMD_VIDEO_SEEK_ABSOLUTE;
extern const QString CMD_VIDEO_SEEK_RELATIVE;
extern const QString CMD_VIDEO_SET_VOLUME;
extern const QString CMD_VIDEO_STOP;
extern const QString CMD_VIDEO_TIMESTAMPS;
extern const QString CMD_WHISKER_STATUS;

// Command parameter flags
extern const QString FLAG_ALIAS;
extern const QString FLAG_BACKCOLOUR;
extern const QString FLAG_BASELINE;
extern const QString FLAG_BITMAP_CLIP;
extern const QString FLAG_BITMAP_STRETCH;
extern const QString FLAG_BOTTOM;
extern const QString FLAG_BRUSH_BACKGROUND;
extern const QString FLAG_BRUSH_OPAQUE;
extern const QString FLAG_BRUSH_STYLE_HATCHED;
extern const QString FLAG_BRUSH_STYLE_HOLLOW;
extern const QString FLAG_BRUSH_STYLE_SOLID;
extern const QString FLAG_BRUSH_TRANSPARENT;
extern const QString FLAG_CENTRE;
extern const QString FLAG_CLIENTCLIENT;
extern const QString FLAG_COMMS;
extern const QString FLAG_DEBUG_TOUCHES;
extern const QString FLAG_DIRECTDRAW;
extern const QString FLAG_EVENTS;
extern const QString FLAG_FONT;
extern const QString FLAG_HEIGHT;
extern const QString FLAG_INPUT;
extern const QString FLAG_KEYEVENTS;
extern const QString FLAG_LEFT;
extern const QString FLAG_LOOP;
extern const QString FLAG_MIDDLE;
extern const QString FLAG_OUTPUT;
extern const QString FLAG_PEN_COLOUR;
extern const QString FLAG_PEN_STYLE;
extern const QString FLAG_PEN_WIDTH;
extern const QString FLAG_POLYGON_ALTERNATE;
extern const QString FLAG_POLYGON_WINDING;
extern const QString FLAG_PREFIX;
extern const QString FLAG_RESET_LEAVE;
extern const QString FLAG_RESET_OFF;
extern const QString FLAG_RESET_ON;
extern const QString FLAG_RESIZE;
extern const QString FLAG_RIGHT;
extern const QString FLAG_SIGNATURE;
extern const QString FLAG_SUFFIX;
extern const QString FLAG_TEXT_COLOUR;
extern const QString FLAG_TEXT_ITALIC;
extern const QString FLAG_TEXT_OPAQUE;
extern const QString FLAG_TEXT_UNDERLINE;
extern const QString FLAG_TEXT_WEIGHT;
extern const QString FLAG_TOP;
extern const QString FLAG_VIDEO_AUDIO;
extern const QString FLAG_VIDEO_NOAUDIO;
extern const QString FLAG_VIDEO_NOLOOP;
extern const QString FLAG_VIDEO_PLAYIMMEDIATE;
extern const QString FLAG_VIDEO_PLAYWHENVISIBLE;
extern const QString FLAG_VIDEO_WAIT;
extern const QString FLAG_WIDTH;

// Quoting strings
extern const QString QUOTE;

// Specific parameter values
extern const QString VAL_ANALOGUE_EVENTTYPE_ABOVE;
extern const QString VAL_ANALOGUE_EVENTTYPE_ALL;
extern const QString VAL_ANALOGUE_EVENTTYPE_BELOW;
extern const QString VAL_ANALOGUE_EVENTTYPE_RANGE;
extern const QString VAL_BOTH;
extern const int VAL_BROADCAST_TO_ALL_CLIENTS;
extern const QString VAL_BRUSH_HATCH_BDIAGONAL;
extern const QString VAL_BRUSH_HATCH_CROSS;
extern const QString VAL_BRUSH_HATCH_DIAGCROSS;
extern const QString VAL_BRUSH_HATCH_FDIAGONAL;
extern const QString VAL_BRUSH_HATCH_HORIZONTAL;
extern const QString VAL_BRUSH_HATCH_VERTICAL;
extern const QString VAL_KEYEVENT_DOWN;
extern const QString VAL_KEYEVENT_NONE;
extern const QString VAL_KEYEVENT_UP;
extern const QString VAL_MOUSE_DBLCLICK;
extern const QString VAL_MOUSE_DOWN;
extern const QString VAL_MOUSE_MOVE;
extern const QString VAL_MOUSE_UP;
extern const QString VAL_OBJTYPE_ARC;
extern const QString VAL_OBJTYPE_BEZIER;
extern const QString VAL_OBJTYPE_BITMAP;
extern const QString VAL_OBJTYPE_CAMCOGQUADPATTERN;
extern const QString VAL_OBJTYPE_CHORD;
extern const QString VAL_OBJTYPE_ELLIPSE;
extern const QString VAL_OBJTYPE_LINE;
extern const QString VAL_OBJTYPE_PIE;
extern const QString VAL_OBJTYPE_POLYGON;
extern const QString VAL_OBJTYPE_RECTANGLE;
extern const QString VAL_OBJTYPE_ROUNDRECT;
extern const QString VAL_OBJTYPE_TEXT;
extern const QString VAL_OBJTYPE_VIDEO;
extern const QString VAL_OFF;
extern const QString VAL_ON;
extern const QString VAL_PEN_DASH;
extern const QString VAL_PEN_DASH_DOT;
extern const QString VAL_PEN_DASH_DOT_DOT;
extern const QString VAL_PEN_DOT;
extern const QString VAL_PEN_INSIDE_FRAME;
extern const QString VAL_PEN_NULL;
extern const QString VAL_PEN_SOLID;
extern const int VAL_TIMER_INFINITE_RELOADS;
extern const QString VAL_TONE_SAWTOOTH;
extern const QString VAL_TONE_SINE;
extern const QString VAL_TONE_SQUARE;
extern const QString VAL_TONE_TONE;
extern const QString VAL_TOUCH_DOWN;
extern const QString VAL_TOUCH_MOVE;
extern const QString VAL_TOUCH_UP;

// Colours
extern const QColor BLACK;
extern const QColor WHITE;

// Mapping C++ enum values to parameter string values
extern const QMap<VideoPlayMode, QString> VIDEO_PLAYMODE_FLAGS;
extern const QMap<VerticalAlign, QString> VALIGN_FLAGS;
extern const QMap<HorizontalAlign, QString> HALIGN_FLAGS;
extern const QMap<TextVerticalAlign, QString> TEXT_VALIGN_FLAGS;
extern const QMap<TextHorizontalAlign, QString> TEXT_HALIGN_FLAGS;
extern const QMap<ResetState, QString> LINE_RESET_FLAGS;
extern const QMap<ToneType, QString> AUDIO_TONE_TYPES;
extern const QMap<SafetyState, QString> LINE_SAFETY_STATES;
extern const QMap<LineEventType, QString> LINE_EVENT_TYPES;
extern const QMap<DocEventType, QString> DOC_EVENT_TYPES;
extern const QMap<KeyEventType, QString> KEY_EVENT_TYPES;
extern const QMap<PenStyle, QString> PEN_STYLE_FLAGS;
extern const QMap<BrushStyle, QString> BRUSH_STYLE_FLAGS;
extern const QMap<BrushHatchStyle, QString> BRUSH_HATCH_VALUES;

// ----------------------------------------------------------------------------
// Internal values
// ----------------------------------------------------------------------------

extern const int FAILURE_INT;

extern const QString WHISKER_ALERT_TITLE;
extern const QString NOT_CONNECTED;
extern const QString WHISKER_SAYS;

}  // namespace whiskerconstants
