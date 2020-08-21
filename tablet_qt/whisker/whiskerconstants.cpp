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

#include "whiskerconstants.h"

namespace whiskerconstants {


// ============================================================================
// API constants
// ============================================================================

// ----------------------------------------------------------------------------
// Network config
// ----------------------------------------------------------------------------

const QString WHISKER_DEFAULT_HOST = "localhost";
const quint16 WHISKER_DEFAULT_PORT = 3233;  // IANA registered port number
const int WHISKER_DEFAULT_TIMEOUT_MS = 5000;

// ----------------------------------------------------------------------------
// Interface basics
// ----------------------------------------------------------------------------

const QString SPACE(" ");
const QString EOL("\n");  // Whisker sends (and accepts) LF between responses.

// Remember: C++ raw strings are R"(content)" for "content"
// https://en.cppreference.com/w/cpp/language/string_literal

// ----------------------------------------------------------------------------
// Server -> client
// ----------------------------------------------------------------------------

const QRegularExpression IMMPORT_REGEX(R"(^ImmPort: (\d+))");
const QRegularExpression CODE_REGEX(R"(^Code: (\w+))");
const QRegularExpression TIMESTAMP_REGEX(R"(^(.*)\s+\[(\d+)\]$)");

const QString RESPONSE_SUCCESS("Success");
const QString RESPONSE_FAILURE("Failure");
const QString PING("Ping");
const QString PING_ACK("PingAcknowledged");

const QRegularExpression EVENT_REGEX("^Event: (.*)$");
const QRegularExpression KEY_EVENT_REGEX(R"(^KeyEvent: (\d+) (\w+) (.*)$)");
const QRegularExpression CLIENT_MESSAGE_REGEX(R"(^ClientMessage: (\d+) (.*)$)");
const QRegularExpression INFO_REGEX("^Info: (.*)$");
const QRegularExpression WARNING_REGEX("Warning: (.*)$");
const QRegularExpression SYNTAX_ERROR_REGEX("^SyntaxError: (.*)$");
const QRegularExpression ERROR_REGEX("Error: (.*)$");

const QString EVENT_PREFIX("Event: ");
const QString KEY_EVENT_PREFIX("KeyEvent: ");
const QString CLIENT_MESSAGE_PREFIX("ClientMessage: ");
const QString INFO_PREFIX("Info: ");
const QString WARNING_PREFIX("Warning: ");
const QString SYNTAX_ERROR_PREFIX("SyntaxError: ");
const QString ERROR_PREFIX("Error: ");

const QString MSG_AUTHENTICATE_CHALLENGE("AuthenticateChallenge");
const QString MSG_DURATION("Duration");
const QString MSG_EXTENT("Extent");
const QString MSG_KEYEVENT_UP("up");
const QString MSG_KEYEVENT_DOWN("down");
const QString MSG_SIZE("Size");
const QString MSG_VIDEO_TIME("VideoTime");

// ----------------------------------------------------------------------------
// Client -> server
// ----------------------------------------------------------------------------

const QString CMD_AUDIO_CLAIM("AudioClaim");
const QString CMD_AUDIO_GET_SOUND_LENGTH("AudioGetSoundLength");
const QString CMD_AUDIO_LOAD_SOUND("AudioLoadSound");
const QString CMD_AUDIO_LOAD_TONE("AudioLoadTone");
const QString CMD_AUDIO_PLAY_FILE("AudioPlayFile");
const QString CMD_AUDIO_PLAY_SOUND("AudioPlaySound");
const QString CMD_AUDIO_RELINQUISH_ALL("AudioRelinquishAll");
const QString CMD_AUDIO_SET_ALIAS("AudioSetAlias");
const QString CMD_AUDIO_SET_SOUND_VOLUME("AudioSetSoundVolume");
const QString CMD_AUDIO_SILENCE_ALL_DEVICES("AudioSilenceAllDevices");
const QString CMD_AUDIO_SILENCE_DEVICE("AudioSilenceDevice");
const QString CMD_AUDIO_STOP_SOUND("AudioStopSound");
const QString CMD_AUDIO_UNLOAD_ALL("AudioUnloadAll");
const QString CMD_AUDIO_UNLOAD_SOUND("AudioUnloadSound");
const QString CMD_AUTHENTICATE("Authenticate");
const QString CMD_AUTHENTICATE_RESPONSE("AuthenticateResponse");
const QString CMD_CLAIM_GROUP("ClaimGroup");
const QString CMD_CLIENT_NUMBER("ClientNumber");
const QString CMD_DISPLAY_ADD_OBJECT("DisplayAddObject");
const QString CMD_DISPLAY_BLANK("DisplayBlank");
const QString CMD_DISPLAY_BRING_TO_FRONT("DisplayBringToFront");
const QString CMD_DISPLAY_CACHE_CHANGES("DisplayCacheChanges");
const QString CMD_DISPLAY_CLAIM("DisplayClaim");
const QString CMD_DISPLAY_CLEAR_BACKGROUND_EVENT("DisplayClearBackgroundEvent");
const QString CMD_DISPLAY_CLEAR_EVENT("DisplayClearEvent");
const QString CMD_DISPLAY_CREATE_DEVICE("DisplayCreateDevice");
const QString CMD_DISPLAY_CREATE_DOCUMENT("DisplayCreateDocument");
const QString CMD_DISPLAY_DELETE_DEVICE("DisplayDeleteDevice");
const QString CMD_DISPLAY_DELETE_DOCUMENT("DisplayDeleteDocument");
const QString CMD_DISPLAY_DELETE_OBJECT("DisplayDeleteObject");
const QString CMD_DISPLAY_EVENT_COORDS("DisplayEventCoords");
const QString CMD_DISPLAY_GET_DOCUMENT_SIZE("DisplayGetDocumentSize");
const QString CMD_DISPLAY_GET_OBJECT_EXTENT("DisplayGetObjectExtent");
const QString CMD_DISPLAY_GET_SIZE("DisplayGetSize");
const QString CMD_DISPLAY_KEYBOARD_EVENTS("DisplayKeyboardEvents");
const QString CMD_DISPLAY_RELINQUISH_ALL("DisplayRelinquishAll");
const QString CMD_DISPLAY_SCALE_DOCUMENTS("DisplayScaleDocuments");
const QString CMD_DISPLAY_SEND_TO_BACK("DisplaySendToBack");
const QString CMD_DISPLAY_SET_ALIAS("DisplaySetAlias");
const QString CMD_DISPLAY_SET_AUDIO_DEVICE("DisplaySetAudioDevice");
const QString CMD_DISPLAY_SET_BACKGROUND_COLOUR("DisplaySetBackgroundColour");
const QString CMD_DISPLAY_SET_BACKGROUND_EVENT("DisplaySetBackgroundEvent");
const QString CMD_DISPLAY_SET_DOCUMENT_SIZE("DisplaySetDocumentSize");
const QString CMD_DISPLAY_SET_EVENT("DisplaySetEvent");
const QString CMD_DISPLAY_SET_OBJ_EVENT_TRANSPARENCY("DisplaySetObjectEventTransparency");
const QString CMD_DISPLAY_SHOW_CHANGES("DisplayShowChanges");
const QString CMD_DISPLAY_SHOW_DOCUMENT("DisplayShowDocument");
const QString CMD_LINE_CLAIM("LineClaim");
const QString CMD_LINE_CLEAR_ALL_EVENTS("LineClearAllEvents");
const QString CMD_LINE_CLEAR_EVENT("LineClearEvent");
const QString CMD_LINE_CLEAR_EVENTS_BY_LINE("LineClearEventsByLine");
const QString CMD_LINE_CLEAR_SAFETY_TIMER("LineClearSafetyTimer");
const QString CMD_LINE_READ_STATE("LineReadState");
const QString CMD_LINE_RELINQUISH_ALL("LineRelinquishAll");
const QString CMD_LINE_SET_ALIAS("LineSetAlias");
const QString CMD_LINE_SET_EVENT("LineSetEvent");
const QString CMD_LINE_SET_SAFETY_TIMER("LineSetSafetyTimer");
const QString CMD_LINE_SET_STATE("LineSetState");
const QString CMD_LINK("Link");
const QString CMD_LOG_CLOSE("LogClose");
const QString CMD_LOG_OPEN("LogOpen");
const QString CMD_LOG_PAUSE("LogPause");
const QString CMD_LOG_RESUME("LogResume");
const QString CMD_LOG_SET_OPTIONS("LogSetOptions");
const QString CMD_LOG_WRITE("LogWrite");
const QString CMD_PERMIT_CLIENT_MESSAGES("PermitClientMessages");
const QString CMD_REPORT_COMMENT("ReportComment");
const QString CMD_REPORT_NAME("ReportName");
const QString CMD_REPORT_STATUS("ReportStatus");
const QString CMD_REQUEST_TIME("RequestTime");
const QString CMD_RESET_CLOCK("ResetClock");
const QString CMD_SEND_TO_CLIENT("SendToClient");
const QString CMD_SET_MEDIA_DIRECTORY("SetMediaDirectory");
const QString CMD_SHUTDOWN("Shutdown");
const QString CMD_TEST_NETWORK_LATENCY("TestNetworkLatency");
const QString CMD_TIMER_CLEAR_ALL_EVENTS("TimerClearAllEvents");
const QString CMD_TIMER_CLEAR_EVENT("TimerClearEvent");
const QString CMD_TIMER_SET_EVENT("TimerSetEvent");
const QString CMD_TIMESTAMPS("Timestamps");
const QString CMD_VERSION("Version");
const QString CMD_VIDEO_GET_DURATION("VideoGetDuration");
const QString CMD_VIDEO_GET_TIME("VideoGetTime");
const QString CMD_VIDEO_PAUSE("VideoPause");
const QString CMD_VIDEO_PLAY("VideoPlay");
const QString CMD_VIDEO_SEEK_ABSOLUTE("VideoSeekAbsolute");
const QString CMD_VIDEO_SEEK_RELATIVE("VideoSeekRelative");
const QString CMD_VIDEO_SET_VOLUME("VideoSetVolume");
const QString CMD_VIDEO_STOP("VideoStop");
const QString CMD_VIDEO_TIMESTAMPS("VideoTimestamps");
const QString CMD_WHISKER_STATUS("WhiskerStatus");

const QString FLAG_ALIAS("-alias");
const QString FLAG_BACKCOLOUR("-backcolour");
const QString FLAG_BASELINE("-baseline");
const QString FLAG_BITMAP_CLIP("-clip");
const QString FLAG_BITMAP_STRETCH("-stretch");
const QString FLAG_BOTTOM("-bottom");
const QString FLAG_BRUSH_BACKGROUND("-brushbackground");
const QString FLAG_BRUSH_OPAQUE("-brushopaque");
const QString FLAG_BRUSH_STYLE_HATCHED("-brushhatched");
const QString FLAG_BRUSH_STYLE_HOLLOW("-brushhollow");
const QString FLAG_BRUSH_STYLE_SOLID("-brushsolid");
const QString FLAG_BRUSH_TRANSPARENT("-brushtransparent");
const QString FLAG_CENTRE("-centre");
const QString FLAG_CLIENTCLIENT("-clientclient");
const QString FLAG_COMMS("-comms");
const QString FLAG_DEBUG_TOUCHES("-debugtouches");
const QString FLAG_DIRECTDRAW("-directdraw");
const QString FLAG_EVENTS("-events");
const QString FLAG_FONT("-font");
const QString FLAG_HEIGHT("-height");
const QString FLAG_INPUT("-input");
const QString FLAG_KEYEVENTS("-keyevents");
const QString FLAG_LEFT("-left");
const QString FLAG_LOOP("-loop");
const QString FLAG_MIDDLE("-middle");
const QString FLAG_OUTPUT("-output");
const QString FLAG_PEN_COLOUR("-pencolour");
const QString FLAG_PEN_STYLE("-penstyle");
const QString FLAG_PEN_WIDTH("-penwidth");
const QString FLAG_POLYGON_ALTERNATE("-alternate");
const QString FLAG_POLYGON_WINDING("-winding");
const QString FLAG_PREFIX("-prefix");
const QString FLAG_RESET_LEAVE("-leave");
const QString FLAG_RESET_OFF("-resetoff");
const QString FLAG_RESET_ON("-reseton");
const QString FLAG_RESIZE("-resize");
const QString FLAG_RIGHT("-right");
const QString FLAG_SIGNATURE("-signature");
const QString FLAG_SUFFIX("-suffix");
const QString FLAG_TEXT_COLOUR("-textcolour");
const QString FLAG_TEXT_ITALIC("-italic");
const QString FLAG_TEXT_OPAQUE("-opaque");
const QString FLAG_TEXT_UNDERLINE("-underline");
const QString FLAG_TEXT_WEIGHT("-weight");
const QString FLAG_TOP("-top");
const QString FLAG_VIDEO_AUDIO("-audio");
const QString FLAG_VIDEO_NOAUDIO("-noaudio");
const QString FLAG_VIDEO_NOLOOP("-noloop");
const QString FLAG_VIDEO_PLAYIMMEDIATE("-playimmediate");
const QString FLAG_VIDEO_PLAYWHENVISIBLE("-playwhenvisible");
const QString FLAG_VIDEO_WAIT("-wait");
const QString FLAG_WIDTH("-width");

const QString QUOTE("\"");

const QString VAL_ANALOGUE_EVENTTYPE_ABOVE("above");
const QString VAL_ANALOGUE_EVENTTYPE_ALL("all");
const QString VAL_ANALOGUE_EVENTTYPE_BELOW("below");
const QString VAL_ANALOGUE_EVENTTYPE_RANGE("range");
const QString VAL_BOTH("both");
const int VAL_BROADCAST_TO_ALL_CLIENTS = -1;
const QString VAL_BRUSH_HATCH_BDIAGONAL("bdiagonal");
const QString VAL_BRUSH_HATCH_CROSS("cross");
const QString VAL_BRUSH_HATCH_DIAGCROSS("diagcross");
const QString VAL_BRUSH_HATCH_FDIAGONAL("fdiagonal");
const QString VAL_BRUSH_HATCH_HORIZONTAL("horizontal");
const QString VAL_BRUSH_HATCH_VERTICAL("vertical");
const QString VAL_KEYEVENT_DOWN("down");
const QString VAL_KEYEVENT_NONE("none");
const QString VAL_KEYEVENT_UP("up");
const QString VAL_MOUSE_DBLCLICK("MouseDblClick");
const QString VAL_MOUSE_DOWN("MouseDown");
const QString VAL_MOUSE_MOVE("MouseMove");
const QString VAL_MOUSE_UP("MouseUp");
const QString VAL_OBJTYPE_ARC("arc");
const QString VAL_OBJTYPE_BEZIER("bezier");
const QString VAL_OBJTYPE_BITMAP("bitmap");
const QString VAL_OBJTYPE_CAMCOGQUADPATTERN("camcogquadpattern");
const QString VAL_OBJTYPE_CHORD("chord");
const QString VAL_OBJTYPE_ELLIPSE("ellipse");
const QString VAL_OBJTYPE_LINE("line");
const QString VAL_OBJTYPE_PIE("pie");
const QString VAL_OBJTYPE_POLYGON("polygon");
const QString VAL_OBJTYPE_RECTANGLE("rectangle");
const QString VAL_OBJTYPE_ROUNDRECT("roundrect");
const QString VAL_OBJTYPE_TEXT("text");
const QString VAL_OBJTYPE_VIDEO("video");
const QString VAL_OFF("off");
const QString VAL_ON("on");
const QString VAL_PEN_DASH("dash");
const QString VAL_PEN_DASH_DOT("dashdot");
const QString VAL_PEN_DASH_DOT_DOT("dashdotdot");
const QString VAL_PEN_DOT("dot");
const QString VAL_PEN_INSIDE_FRAME("insideframe");
const QString VAL_PEN_NULL("null");
const QString VAL_PEN_SOLID("solid");
const int VAL_TIMER_INFINITE_RELOADS = -1;
const QString VAL_TONE_SAWTOOTH("sawtooth");
const QString VAL_TONE_SINE("sine");
const QString VAL_TONE_SQUARE("square");
const QString VAL_TONE_TONE("tone");
const QString VAL_TOUCH_DOWN("TouchDown");
const QString VAL_TOUCH_MOVE("TouchMove");
const QString VAL_TOUCH_UP("TouchUp");

const QColor BLACK(0, 0, 0);
const QColor WHITE(255, 255, 255);

const QMap<VideoPlayMode, QString> VIDEO_PLAYMODE_FLAGS{
    {VideoPlayMode::Wait, FLAG_VIDEO_WAIT},
    {VideoPlayMode::Immediate, FLAG_VIDEO_PLAYIMMEDIATE},
    {VideoPlayMode::WhenVisible, FLAG_VIDEO_PLAYWHENVISIBLE},
};
const QMap<VerticalAlign, QString> VALIGN_FLAGS{
    {VerticalAlign::Top, FLAG_TOP},
    {VerticalAlign::Middle, FLAG_MIDDLE},
    {VerticalAlign::Bottom, FLAG_BOTTOM},
};
const QMap<HorizontalAlign, QString> HALIGN_FLAGS{
    {HorizontalAlign::Left, FLAG_LEFT},
    {HorizontalAlign::Centre, FLAG_CENTRE},
    {HorizontalAlign::Right, FLAG_RIGHT},
};
const QMap<TextVerticalAlign, QString> TEXT_VALIGN_FLAGS{
    {TextVerticalAlign::Top, FLAG_TOP},
    {TextVerticalAlign::Middle, FLAG_MIDDLE},
    {TextVerticalAlign::Bottom, FLAG_BOTTOM},
    {TextVerticalAlign::Baseline, FLAG_BASELINE},
};
const QMap<TextHorizontalAlign, QString> TEXT_HALIGN_FLAGS{
    {TextHorizontalAlign::Left, FLAG_LEFT},
    {TextHorizontalAlign::Centre, FLAG_CENTRE},
    {TextHorizontalAlign::Right, FLAG_RIGHT},
};
const QMap<ResetState, QString> LINE_RESET_FLAGS{
    {ResetState::Input, ""},
    {ResetState::On, FLAG_RESET_ON},
    {ResetState::Off, FLAG_RESET_OFF},
    {ResetState::Leave, FLAG_RESET_LEAVE},
};
const QMap<ToneType, QString> AUDIO_TONE_TYPES{
    {ToneType::Sine, VAL_TONE_SINE},
    {ToneType::Sawtooth, VAL_TONE_SAWTOOTH},
    {ToneType::Square, VAL_TONE_SQUARE},
    {ToneType::Tone, VAL_TONE_TONE},
};
const QMap<SafetyState, QString> LINE_SAFETY_STATES{
    {SafetyState::On, VAL_ON},
    {SafetyState::Off, VAL_OFF},
};
const QMap<LineEventType, QString> LINE_EVENT_TYPES{
    {LineEventType::On, VAL_ON},
    {LineEventType::Off, VAL_OFF},
    {LineEventType::Both, VAL_BOTH},
};
const QMap<DocEventType, QString> DOC_EVENT_TYPES{
    {DocEventType::MouseDown, VAL_MOUSE_DOWN},
    {DocEventType::MouseUp, VAL_MOUSE_UP},
    {DocEventType::MouseDoubleClick, VAL_MOUSE_DBLCLICK},
    {DocEventType::MouseMove, VAL_MOUSE_MOVE},
    {DocEventType::TouchDown, VAL_TOUCH_DOWN},
    {DocEventType::TouchUp, VAL_TOUCH_UP},
    {DocEventType::TouchMove, VAL_TOUCH_MOVE},
};
const QMap<KeyEventType, QString> KEY_EVENT_TYPES{
    {KeyEventType::None, VAL_KEYEVENT_NONE},
    {KeyEventType::Down, VAL_KEYEVENT_DOWN},
    {KeyEventType::Up, VAL_KEYEVENT_UP},
    {KeyEventType::Both, VAL_BOTH},
};
const QMap<PenStyle, QString> PEN_STYLE_FLAGS{
    {PenStyle::Solid, VAL_PEN_SOLID},
    {PenStyle::Dash, VAL_PEN_DASH},
    {PenStyle::Dot, VAL_PEN_DOT},
    {PenStyle::DashDot, VAL_PEN_DASH_DOT},
    {PenStyle::DashDotDot, VAL_PEN_DASH_DOT_DOT},
    {PenStyle::Null, VAL_PEN_NULL},
    {PenStyle::InsideFrame, VAL_PEN_INSIDE_FRAME},
};
const QMap<BrushStyle, QString> BRUSH_STYLE_FLAGS{
    {BrushStyle::Hollow, FLAG_BRUSH_STYLE_HOLLOW},
    {BrushStyle::Solid, FLAG_BRUSH_STYLE_SOLID},
    {BrushStyle::Hatched, FLAG_BRUSH_STYLE_HATCHED},
};
const QMap<BrushHatchStyle, QString> BRUSH_HATCH_VALUES{
    {BrushHatchStyle::Vertical, VAL_BRUSH_HATCH_VERTICAL},
    {BrushHatchStyle::FDiagonal, VAL_BRUSH_HATCH_FDIAGONAL},
    {BrushHatchStyle::Horizontal, VAL_BRUSH_HATCH_HORIZONTAL},
    {BrushHatchStyle::BDiagonal, VAL_BRUSH_HATCH_BDIAGONAL},
    {BrushHatchStyle::Cross, VAL_BRUSH_HATCH_CROSS},
    {BrushHatchStyle::DiagCross, VAL_BRUSH_HATCH_DIAGCROSS},
};

// ----------------------------------------------------------------------------
// Internal values
// ----------------------------------------------------------------------------

const int FAILURE_INT = -1;

const QString WHISKER_ALERT_TITLE("Whisker");
const QString NOT_CONNECTED("Not connected");
const QString WHISKER_SAYS("Whisker says:");

}  // namespace whiskerconstants
