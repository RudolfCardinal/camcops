/*
    Copyright (C) 2012-2018 Rudolf Cardinal (rudolf@pobox.com).

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

// ============================================================================
// debugging #defines
// ============================================================================

#define DEBUG_WHISKER_MESSAGES

// ============================================================================
// #includes
// ============================================================================

#include "whiskermanager.h"
#include <QRegularExpression>
#include "common/varconst.h"
#include "core/camcopsapp.h"
#include "lib/uifunc.h"
#include "whisker/whiskerapi.h"
#include "whisker/whiskerconstants.h"
#include "whisker/whiskerworker.h"
using namespace whiskerapi;
using namespace whiskerconstants;


// ============================================================================
// WhiskerManager
// ============================================================================

WhiskerManager::WhiskerManager(CamcopsApp& app, const QString& sysevent_prefix) :
    m_app(app),
    m_worker(new WhiskerWorker(this)),
    m_sysevent_prefix(sysevent_prefix),
    m_sysevent_counter(0)
{
    // As per http://doc.qt.io/qt-5/qthread.html:
    m_worker->moveToThread(&m_worker_thread);  // changes thread affinity
    connect(&m_worker_thread, &QThread::finished,
            m_worker, &QObject::deleteLater);  // this is how we ensure deletion of m_worker

    // Our additional signal/slot connections:
    connect(this, &WhiskerManager::internalConnectToServer,
            m_worker, &WhiskerWorker::connectToServer);
    connect(this, &WhiskerManager::disconnectFromServer,
            m_worker, &WhiskerWorker::disconnectFromServer);
    connect(this, &WhiskerManager::internalSend,
            m_worker, &WhiskerWorker::sendToServer);

    connect(m_worker, &WhiskerWorker::connectionStateChanged,
            this, &WhiskerManager::internalConnectionStateChanged);
    connect(m_worker, &WhiskerWorker::receivedFromServerMainSocket,
            this, &WhiskerManager::internalReceiveFromMainSocket);

    m_worker_thread.start();
}


WhiskerManager::~WhiskerManager()
{
    m_worker_thread.quit();
    m_worker_thread.wait();
}


void WhiskerManager::connectToServer()
{
    const QString host = m_app.varString(varconst::WHISKER_HOST);
    const quint16 port = m_app.varInt(varconst::WHISKER_PORT);
    const int timeout_ms = m_app.varInt(varconst::WHISKER_TIMEOUT_MS);
    emit internalConnectToServer(host, port, timeout_ms);
}


bool WhiskerManager::isConnected() const
{
    return m_worker->isImmediateConnected();
}


void WhiskerManager::sendMain(const QString& command)
{
    WhiskerOutboundCommand cmd(command, false);
    emit internalSend(cmd);
}


void WhiskerManager::sendMain(const QStringList& args)
{
    sendMain(msgFromArgs(args));
}


void WhiskerManager::sendMain(std::initializer_list<QString> args)
{
    sendMain(msgFromArgs(QStringList(args)));
}


void WhiskerManager::sendImmediateIgnoreReply(const QString& command)
{
#ifdef DEBUG_SOCKETS
    qDebug() << "Sending immediate-socket command (for no reply):" << command;
#endif
    WhiskerOutboundCommand cmd(command, true, true);
    emit internalSend(cmd);  // transfer send command to our worker on its socket thread
}


WhiskerInboundMessage WhiskerManager::sendImmediateGetReply(
        const QString& command)
{
#ifdef DEBUG_SOCKETS
    qDebug() << "Sending immediate-socket command:" << command;
#endif
    WhiskerOutboundCommand cmd(command, true, false);
    emit internalSend(cmd);  // transfer send command to our worker on its socket thread
    WhiskerInboundMessage msg = m_worker->getPendingImmediateReply();
#ifdef DEBUG_SOCKETS
        qDebug()
                << "Immediate-socket command" << msg.m_causal_command
                << "-> reply" << msg.m_msg;
#endif
    return msg;
}


QString WhiskerManager::immResp(const QString& command)
{
    const WhiskerInboundMessage reply = sendImmediateGetReply(command);
    return reply.message();
}


QString WhiskerManager::immResp(const QStringList& args)
{
    return immResp(msgFromArgs(args));
}


QString WhiskerManager::immResp(std::initializer_list<QString> args)
{
    return immResp(msgFromArgs(QStringList(args)));
}


bool WhiskerManager::immBool(const QString& command, bool ignore_reply)
{
    if (ignore_reply) {
        sendImmediateIgnoreReply(command);
        return true;
    } else {
        const WhiskerInboundMessage msg = sendImmediateGetReply(command);
        return msg.immediateReplySucceeded();
    }
}


bool WhiskerManager::immBool(const QStringList& args, bool ignore_reply)
{
    return immBool(msgFromArgs(args), ignore_reply);
}


bool WhiskerManager::immBool(std::initializer_list<QString> args,
                             bool ignore_reply)
{
    return immBool(msgFromArgs(QStringList(args)), ignore_reply);
}


void WhiskerManager::internalReceiveFromMainSocket(
        const WhiskerInboundMessage& msg)
{
#ifdef DEBUG_WHISKER_MESSAGES
    qDebug() << "Received Whisker main-socket message:" << msg;
#endif

    // Send the message via the general-purpose signal
    emit messageReceived(msg);

    // Send the message to specific-purpose receivers
    if (msg.isEvent()) {
        // *** check for system events +/- bin out
        emit eventReceived(msg);
    } else if (msg.isKeyEvent()) {
        emit keyEventReceived(msg);
    } else if (msg.isClientMessage()) {
        emit clientMessageReceived(msg);
    } else if (msg.isWarning()) {
        emit warningReceived(msg);
    } else if (msg.isSyntaxError()) {
        emit syntaxErrorReceived(msg);
    } else if (msg.isError()) {
        emit errorReceived(msg);
    } else if (msg.isPingAck()) {
        emit pingAckReceived(msg);
    }
}


void WhiskerManager::internalConnectionStateChanged(
        WhiskerConnectionState state)
{
    emit connectionStateChanged(state == WhiskerConnectionState::G_FullyConnected);
}


void WhiskerManager::onSocketError(const QString& msg)
{
    uifunc::alert("Whisker socket error:\n\n" + msg, WHISKER_ALERT_TITLE);
}


// ============================================================================
// API
// ============================================================================

/*


***

    # -------------------------------------------------------------------------
    # Custom event handling, e.g. for line flashing
    # -------------------------------------------------------------------------

    def get_new_sysevent(self, *args) -> str:
        self.sysevent_counter += 1
        return self.sysevent_prefix + "_".join(
            str(x) for x in [self.sysevent_counter] + list(args)
        ).replace(" ", "")

    def process_backend_event(self, event: str) -> bool:
        """Returns True if the backend API has dealt with the event and it
        doesn't need to go to the main behavioural task."""
        n_called, swallow_event = self.callback_handler.process_event(event)
        return (
            (n_called > 0 and swallow_event) or
            event.startswith(self.sysevent_prefix)
        )

    def send_after_delay(self, delay_ms: int, msg: str,
                         event: str = '') -> None:
        event = event or self.get_new_sysevent("send", msg)
        self.timer_set_event(event, delay_ms)
        self.callback_handler.add_single(event, self._immsend_get_reply, [msg])

    def call_after_delay(self,
                         delay_ms: int,
                         callback: Callable[..., None],
                         args: List[Any] = None,
                         kwargs: List[Any] = None,
                         event: str = '') -> None:
        args = args or []
        kwargs = kwargs or {}
        event = event or self.get_new_sysevent("call")
        self.timer_set_event(event, delay_ms)
        self.callback_handler.add_single(event, callback, args, kwargs)

    def call_on_event(self,
                      event: str,
                      callback: Callable[..., None],
                      args: List[Any] = None,
                      kwargs: List[Any] = None,
                      swallow_event: bool = False) -> None:
        args = args or []
        kwargs = kwargs or {}
        self.callback_handler.add_persistent(event, callback, args, kwargs,
                                             swallow_event=swallow_event)

    def clear_event_callback(self,
                             event: str,
                             callback: Callable[..., None] = None) -> None:
        self.callback_handler.remove(event, callback=callback)

    def clear_all_callbacks(self) -> None:
        self.callback_handler.clear()

    def debug_callbacks(self) -> None:
        self.callback_handler.debug()

    # -------------------------------------------------------------------------
    # Line flashing
    # -------------------------------------------------------------------------

    def flash_line_pulses(self,
                          line: str,
                          count: int,
                          on_ms: int,
                          off_ms: int,
                          on_at_rest: bool = False) -> int:
        assert count > 0
        # Generally better to ping-pong the events, rather than line them up
        # in advance, in case the user specifies very rapid oscillation that
        # exceeds the network bandwidth, or something; better to be slow than
        # to garbage up the sequence.
        if on_at_rest:
            # Currently at rest = on.
            # For 4 flashes:
            # OFF .. ON .... OFF .. ON .... OFF .. ON .... OFF .. ON
            on_now = False
            timing_sequence = [off_ms] + (count - 1) * [on_ms, off_ms]
        else:
            # Currently at rest = off.
            # For 4 flashes:
            # ON .... OFF .. ON .... OFF .. ON .... OFF .. ON .... OFF
            on_now = True
            timing_sequence = [on_ms] + (count - 1) * [off_ms, on_ms]
        total_duration_ms = sum(timing_sequence)
        self.flash_line_ping_pong(line, on_now, timing_sequence)
        return total_duration_ms

    def flash_line_ping_pong(self,
                             line: str,
                             on_now: bool,
                             timing_sequence: List[int]) -> None:
        """
        line: line number/name
        on_now: switch it on or off now?
        timing_sequence: array of times (in ms) for the next things
        """
        self.line_on(line) if on_now else self.line_off(line)
        if not timing_sequence:
            return
        delay_ms = timing_sequence[0]
        timing_sequence = timing_sequence[1:]
        event = self.get_new_sysevent(line, "off" if on_now else "on")
        self.call_after_delay(delay_ms, self.flash_line_ping_pong,
                              args=[line, not on_now, timing_sequence],
                              event=event)


                           */


// ----------------------------------------------------------------------------
// Whisker command set: comms, misc
// ----------------------------------------------------------------------------

bool WhiskerManager::setTimestamps(bool on, bool ignore_reply)
{
    return immBool({CMD_TIMESTAMPS, onVal(on)}, ignore_reply);
}


bool WhiskerManager::resetClock(bool ignore_reply)
{
    return immBool(CMD_RESET_CLOCK, ignore_reply);
}


QString WhiskerManager::getServerVersion()
{
    return immResp(CMD_VERSION);
}


float WhiskerManager::getServerVersionNumeric()
{
    const QString version_str = getServerVersion();
    return version_str.toFloat();
}


unsigned int WhiskerManager::getServerTimeMs()
{
    const QString time_str = immResp(CMD_REQUEST_TIME);
    return time_str.toUInt();
}


int WhiskerManager::getClientNumber()
{
    const QString clientnum_str = immResp(CMD_CLIENT_NUMBER);
    return clientnum_str.toInt();
}


bool WhiskerManager::permitClientMessages(bool permit, bool ignore_reply)
{
    return immBool({CMD_PERMIT_CLIENT_MESSAGES, onVal(permit)}, ignore_reply);
}


bool WhiskerManager::sendToClient(int clientNum, const QString& message,
                                  bool ignore_reply)
{
    return immBool({CMD_SEND_TO_CLIENT, QString::number(clientNum), message},
                   ignore_reply);
}


bool WhiskerManager::setMediaDirectory(const QString& directory,
                                       bool ignore_reply)
{
    return immBool({CMD_SET_MEDIA_DIRECTORY, quote(directory)}, ignore_reply);
}


bool WhiskerManager::reportName(const QString& name, bool ignore_reply)
{
    return immBool({CMD_REPORT_NAME, name}, ignore_reply);
    // quotes not necessary
}


bool WhiskerManager::reportStatus(const QString& status, bool ignore_reply)
{
    return immBool({CMD_REPORT_STATUS, status}, ignore_reply);
    // quotes not necessary
}


bool WhiskerManager::reportComment(const QString& comment, bool ignore_reply)
{
    return immBool({CMD_REPORT_COMMENT, comment}, ignore_reply);
    // quotes not necessary
}


int WhiskerManager::getNetworkLatencyMs()
{
    WhiskerInboundMessage reply_ping = sendImmediateGetReply(
                CMD_TEST_NETWORK_LATENCY);
    if (reply_ping.message() != PING) {
        return FAILURE_INT;
    }
    WhiskerInboundMessage reply_latency = sendImmediateGetReply(PING_ACK);
    bool ok;
    const int latency_ms = reply_latency.message().toInt(&ok);
    if (!ok) {
        return FAILURE_INT;
    }
    return latency_ms;
}


bool WhiskerManager::ping()
{
    return immResp(PING) == PING_ACK;
}


bool WhiskerManager::shutdown(bool ignore_reply)
{
    return immBool(CMD_SHUTDOWN, ignore_reply);
}


QString WhiskerManager::authenticateGetChallenge(const QString& package,
                                                 const QString& client_name)
{
    const QString reply = immResp({CMD_AUTHENTICATE, package, client_name});
    const QStringList parts = reply.split(SPACE);
    if (parts.size() != 2 || parts.at(0) != MSG_AUTHENTICATE_CHALLENGE) {
        return "";
    }
    return parts.at(1);
}


bool WhiskerManager::authenticateProvideResponse(const QString& response,
                                                 bool ignore_reply)
{
    return immBool({CMD_AUTHENTICATE_RESPONSE, response}, ignore_reply);
}


// ----------------------------------------------------------------------------
// Whisker command set: logs
// ----------------------------------------------------------------------------

bool WhiskerManager::logOpen(const QString& filename, bool ignore_reply)
{
    return immBool({CMD_LOG_OPEN, quote(filename)}, ignore_reply);
}


bool WhiskerManager::logSetOptions(const LogOptions& options, bool ignore_reply)
{
    return immBool({
        CMD_LOG_SET_OPTIONS,
        FLAG_EVENTS, onVal(options.events),
        FLAG_KEYEVENTS, onVal(options.key_events),
        FLAG_CLIENTCLIENT, onVal(options.client_client),
        FLAG_COMMS, onVal(options.comms),
        FLAG_SIGNATURE, onVal(options.signature),
    }, ignore_reply);
}


bool WhiskerManager::logPause(bool ignore_reply)
{
    return immBool(CMD_LOG_PAUSE, ignore_reply);
}


bool WhiskerManager::logResume(bool ignore_reply)
{
    return immBool(CMD_LOG_RESUME, ignore_reply);
}


bool WhiskerManager::logWrite(const QString& msg, bool ignore_reply)
{
    return immBool({CMD_LOG_WRITE, msg}, ignore_reply);
}


bool WhiskerManager::logClose(bool ignore_reply)
{
    return immBool(CMD_LOG_CLOSE, ignore_reply);
}


// ----------------------------------------------------------------------------
// Whisker command set: timers
// ----------------------------------------------------------------------------

bool WhiskerManager::timerSetEvent(const QString& event,
                                   unsigned int duration_ms,
                                   int reload_count,
                                   bool ignore_reply)
{
    return immBool({
        CMD_TIMER_SET_EVENT,
        event,
        QString::number(duration_ms),
        QString::number(reload_count),
    }, ignore_reply);
}


bool WhiskerManager::timerClearEvent(const QString& event, bool ignore_reply)
{
    return immBool({CMD_TIMER_CLEAR_EVENT, event}, ignore_reply);
}


bool WhiskerManager::timerClearAllEvents(bool ignore_reply)
{
    return immBool(CMD_TIMER_CLEAR_ALL_EVENTS, ignore_reply);
}

// ----------------------------------------------------------------------------
// Whisker command set: claiming, relinquishing
// ----------------------------------------------------------------------------

bool WhiskerManager::claimGroup(const QString& group,
                                const QString& prefix,
                                const QString& suffix)
{
    QStringList args{CMD_CLAIM_GROUP, group};
    if (!prefix.isEmpty()) {
        args.append({FLAG_PREFIX, prefix});
    }
    if (!suffix.isEmpty()) {
        args.append({FLAG_SUFFIX, suffix});
    }
    return immBool(args);
}


bool WhiskerManager::claimLine(unsigned int line_number, bool output,
                               const QString& alias, ResetState reset_state)
{
    QStringList args{
        CMD_LINE_CLAIM,
        QString::number(line_number),
        output ? FLAG_OUTPUT : FLAG_INPUT,
        LINE_RESET_FLAGS[output ? reset_state : ResetState::Input],
    };
    if (!alias.isEmpty()) {
        args.append({FLAG_ALIAS, alias});
    }
    return immBool(args);
}


bool WhiskerManager::claimLine(const QString& group, const QString& device,
                               bool output, const QString& alias,
                               ResetState reset_state)
{
    QStringList args{
        CMD_LINE_CLAIM,
        group,
        device,
        output ? FLAG_OUTPUT : FLAG_INPUT,
        LINE_RESET_FLAGS[output ? reset_state : ResetState::Input],
    };
    if (!alias.isEmpty()) {
        args.append({FLAG_ALIAS, alias});
    }
    return immBool(args);
}


bool WhiskerManager::relinquishAllLines(bool ignore_reply)
{
    return immBool(CMD_LINE_RELINQUISH_ALL, ignore_reply);
}


bool WhiskerManager::lineSetAlias(unsigned int line_number,
                                  const QString& alias, bool ignore_reply)
{
    return immBool({CMD_LINE_SET_ALIAS, QString::number(line_number), alias},
                   ignore_reply);
}


bool WhiskerManager::lineSetAlias(const QString& existing_alias,
                                  const QString& new_alias, bool ignore_reply)
{
    return immBool({CMD_LINE_SET_ALIAS, existing_alias, new_alias},
                   ignore_reply);
}



bool WhiskerManager::claimAudio(unsigned int device_number,
                                const QString& alias)
{
    QStringList args{
        CMD_AUDIO_CLAIM,
        QString::number(device_number),
    };
    if (!alias.isEmpty()) {
        args.append({FLAG_ALIAS, alias});
    }
    return immBool(args);
}


bool WhiskerManager::claimAudio(const QString& group, const QString& device,
                                const QString& alias)
{
    QStringList args{
        CMD_AUDIO_CLAIM,
        group,
        device,
    };
    if (!alias.isEmpty()) {
        args.append({FLAG_ALIAS, alias});
    }
    return immBool(args);
}


bool WhiskerManager::audioSetAlias(unsigned int device_number,
                                   const QString& alias,
                                   bool ignore_reply)
{
    return immBool({CMD_AUDIO_SET_ALIAS, QString::number(device_number), alias},
                   ignore_reply);
}


bool WhiskerManager::audioSetAlias(const QString& existing_alias,
                                   const QString& new_alias,
                                   bool ignore_reply)
{
    return immBool({CMD_AUDIO_SET_ALIAS, existing_alias, new_alias},
                   ignore_reply);
}


bool WhiskerManager::relinquishAllAudio(bool ignore_reply)
{
    return immBool(CMD_AUDIO_RELINQUISH_ALL, ignore_reply);
}


bool WhiskerManager::claimDisplay(unsigned int display_number,
                                  const QString& alias)
{
    // Autocreating debug views not supported (see C++ WhiskerClientLib).
    QStringList args{
        CMD_DISPLAY_CLAIM,
        QString::number(display_number),
    };
    if (!alias.isEmpty()) {
        args.append({FLAG_ALIAS, alias});
    }
    return immBool(args);
}


bool WhiskerManager::claimDisplay(const QString& group, const QString& device,
                                  const QString& alias)
{
    // Autocreating debug views not supported (see C++ WhiskerClientLib).
    QStringList args{
        CMD_DISPLAY_CLAIM,
        group,
        device,
    };
    if (!alias.isEmpty()) {
        args.append({FLAG_ALIAS, alias});
    }
    return immBool(args);
}


bool WhiskerManager::displaySetAlias(unsigned int display_number,
                                     const QString& alias,
                                     bool ignore_reply)
{
    return immBool({CMD_DISPLAY_SET_ALIAS, QString::number(display_number), alias},
                   ignore_reply);
}


bool WhiskerManager::displaySetAlias(const QString& existing_alias,
                                     const QString& new_alias,
                                     bool ignore_reply)
{
    return immBool({CMD_DISPLAY_SET_ALIAS, existing_alias, new_alias},
                   ignore_reply);
}


bool WhiskerManager::relinquishAllDisplays(bool ignore_reply)
{
    return immBool(CMD_DISPLAY_RELINQUISH_ALL, ignore_reply);
}


bool WhiskerManager::displayCreateDevice(const QString& name,
                                         DisplayCreationOptions options)
{
    QStringList args{
        CMD_DISPLAY_CREATE_DEVICE,
        name,
        FLAG_RESIZE, onVal(options.resize),
        FLAG_DIRECTDRAW, onVal(options.directdraw),
    };
    if (!options.rectangle.isEmpty()) {
        args.append({
            QString::number(options.rectangle.left()),
            QString::number(options.rectangle.top()),
            QString::number(options.rectangle.width()),
            QString::number(options.rectangle.height()),
        });
    }
    if (options.debug_touches) {
        args.append(FLAG_DEBUG_TOUCHES);
    }
    return immBool(args);
}


bool WhiskerManager::displayDeleteDevice(const QString& device,
                                         bool ignore_reply)
{
    return immBool({CMD_DISPLAY_DELETE_DEVICE, device}, ignore_reply);
}


// ----------------------------------------------------------------------------
// Whisker command set: lines
// ----------------------------------------------------------------------------

bool WhiskerManager::lineSetState(const QString& line, bool on,
                                  bool ignore_reply)
{
    return immBool({CMD_LINE_SET_STATE, line, onVal(on)}, ignore_reply);
}


bool WhiskerManager::lineReadState(const QString& line, bool* ok)
{
    WhiskerInboundMessage reply = immResp({CMD_LINE_READ_STATE, line});
    const QString msg = reply.message();
    if (msg == VAL_ON) {
        // Line is on
        if (ok) {
            *ok = true;
        }
        return true;
    } else if (msg == VAL_OFF) {
        // Line is off
        if (ok) {
            *ok = true;
        }
        return false;
    } else {
        // Something went wrong
        if (ok) {
            *ok = false;
        }
        return false;
    }
}


bool WhiskerManager::lineSetEvent(const QString& line, const QString& event,
                                  LineEventType event_type, bool ignore_reply)
{
    return immBool({
        CMD_LINE_SET_EVENT, line, LINE_EVENT_TYPES[event_type], event
    }, ignore_reply);
}


bool WhiskerManager::lineClearEvent(const QString& event, bool ignore_reply)
{
    return immBool({CMD_LINE_CLEAR_EVENT, event}, ignore_reply);
}


bool WhiskerManager::lineClearEventByLine(const QString& line,
                                          LineEventType event_type,
                                          bool ignore_reply)
{
    return immBool({
        CMD_LINE_CLEAR_EVENTS_BY_LINE, line, LINE_EVENT_TYPES[event_type]
    }, ignore_reply);
}


bool WhiskerManager::lineClearAllEvents(bool ignore_reply)
{
    return immBool(CMD_LINE_CLEAR_ALL_EVENTS, ignore_reply);
}


bool WhiskerManager::lineSetSafetyTimer(const QString& line,
                                        unsigned int time_ms,
                                        SafetyState safety_state,
                                        bool ignore_reply)
{
    return immBool({
        CMD_LINE_SET_SAFETY_TIMER, line, QString::number(time_ms),
        LINE_SAFETY_STATES[safety_state],
    }, ignore_reply);
}


bool WhiskerManager::lineClearSafetyTimer(const QString& line,
                                          bool ignore_reply)
{
    return immBool({CMD_LINE_CLEAR_SAFETY_TIMER, line}, ignore_reply);
}


// ----------------------------------------------------------------------------
// Whisker command set: audio
// ----------------------------------------------------------------------------

bool WhiskerManager::audioPlayWav(const QString& device,
                                  const QString& filename,
                                  bool ignore_reply)
{
    return immBool({CMD_AUDIO_PLAY_FILE, device, quote(filename)},
                   ignore_reply);
}


bool WhiskerManager::audioLoadTone(const QString& device,
                                   const QString& sound_name,
                                   unsigned int frequency_hz,
                                   whiskerconstants::ToneType tone_type,
                                   unsigned int duration_ms,
                                   bool ignore_reply)
{
    return immBool({
        CMD_AUDIO_LOAD_TONE,
        device,
        sound_name,
        QString::number(frequency_hz),
        AUDIO_TONE_TYPES[tone_type],
        QString::number(duration_ms),  // *** add to Whisker docs about the duration_ms option; see whiskerserver/client.cpp
    }, ignore_reply);
}


bool WhiskerManager::audioLoadWav(const QString& device,
                                  const QString& sound_name,
                                  const QString& filename,
                                  bool ignore_reply)
{
    return immBool({CMD_AUDIO_LOAD_SOUND, device, sound_name, quote(filename)},
                   ignore_reply);
}


bool WhiskerManager::audioPlaySound(const QString& device,
                                    const QString& sound_name,
                                    bool loop, bool ignore_reply)
{
    QStringList args{CMD_AUDIO_PLAY_SOUND, device, sound_name};
    if (loop) {
        args.append(FLAG_LOOP);
    }
    return immBool(args, ignore_reply);
}


bool WhiskerManager::audioUnloadSound(const QString& device,
                                      const QString& sound_name,
                                      bool ignore_reply)
{
    return immBool({CMD_AUDIO_UNLOAD_SOUND, device, sound_name}, ignore_reply);
}


bool WhiskerManager::audioStopSound(const QString& device,
                                    const QString& sound_name,
                                    bool ignore_reply)
{
    return immBool({CMD_AUDIO_STOP_SOUND, device, sound_name}, ignore_reply);
}


bool WhiskerManager::audioSilenceDevice(const QString& device,
                                        bool ignore_reply)
{
    return immBool({CMD_AUDIO_SILENCE_DEVICE, device}, ignore_reply);
}


bool WhiskerManager::audioUnloadAll(const QString& device, bool ignore_reply)
{
    return immBool({CMD_AUDIO_UNLOAD_ALL, device}, ignore_reply);
}


bool WhiskerManager::audioSetSoundVolume(const QString& device,
                                         const QString& sound_name,
                                         unsigned int volume,
                                         bool ignore_reply)
{
    return immBool({
        CMD_AUDIO_SET_SOUND_VOLUME, device, sound_name, QString::number(volume)
    }, ignore_reply);
}


bool WhiskerManager::audioSilenceAllDevices(bool ignore_reply)
{
    return immBool(CMD_AUDIO_SILENCE_ALL_DEVICES, ignore_reply);
}


unsigned int WhiskerManager::audioGetSoundDurationMs(const QString& device,
                                                     const QString& sound_name,
                                                     bool* ok)
{
    const QString reply = immResp(
        {CMD_AUDIO_GET_SOUND_LENGTH, device, sound_name});
    return reply.toUInt(ok);
}


// ----------------------------------------------------------------------------
// Whisker command set: display: display operations
// ----------------------------------------------------------------------------

QSize WhiskerManager::displayGetSize(const QString& device)
{
    const QString reply = immResp({CMD_DISPLAY_GET_SIZE, device});
    const QStringList parts = reply.split(SPACE);
    if (parts.size() != 3 || parts.at(0) != MSG_SIZE) {
        return QSize();
    }
    bool ok;
    const int width = parts.at(1).toInt(&ok);
    if (!ok) {
        return QSize();
    }
    const int height = parts.at(2).toInt(&ok);
    if (!ok) {
        return QSize();
    }
    return QSize(width, height);
}


bool WhiskerManager::displayScaleDocuments(const QString& device,
                                           bool scale, bool ignore_reply)
{
    return immBool({CMD_DISPLAY_SCALE_DOCUMENTS, device, onVal(scale)},
                   ignore_reply);
}


bool WhiskerManager::displayShowDocument(const QString& device,
                                         const QString& doc,
                                         bool ignore_reply)
{
    return immBool({CMD_DISPLAY_SHOW_DOCUMENT, device, doc}, ignore_reply);
}


bool WhiskerManager::displayBlank(const QString& device, bool ignore_reply)
{
    return immBool({CMD_DISPLAY_BLANK, device}, ignore_reply);
}


// ----------------------------------------------------------------------------
// Whisker command set: display: document operations
// ----------------------------------------------------------------------------

bool WhiskerManager::displayCreateDocument(const QString& doc,
                                           bool ignore_reply)
{
    return immBool({CMD_DISPLAY_CREATE_DOCUMENT, doc}, ignore_reply);
}


bool WhiskerManager::displayDeleteDocument(const QString& doc,
                                           bool ignore_reply)
{
    return immBool({CMD_DISPLAY_DELETE_DOCUMENT, doc}, ignore_reply);
}


bool WhiskerManager::displaySetDocumentSize(const QString& doc,
                                            const QSize& size,
                                            bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_SET_DOCUMENT_SIZE,
        doc,
        QString::number(size.width()),
        QString::number(size.height()),
    }, ignore_reply);
}


bool WhiskerManager::displaySetBackgroundColour(const QString& doc,
                                                const QColor& colour,
                                                bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_SET_BACKGROUND_COLOUR,
        doc,
        rgbFromColour(colour),
    }, ignore_reply);
}


bool WhiskerManager::displayDeleteObject(const QString& doc,
                                         const QString& obj,
                                         bool ignore_reply)
{
    return immBool({CMD_DISPLAY_DELETE_OBJECT, doc, obj}, ignore_reply);
}


bool WhiskerManager::displayAddObject(
        const QString& doc, const QString& obj,
        const DisplayObject& object_definition, bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_ADD_OBJECT,
        doc, obj,
        object_definition.optionString(),
    }, ignore_reply);
}


bool WhiskerManager::displaySetEvent(const QString& doc,
                                     const QString& obj,
                                     const QString& event,
                                     DocEventType event_type,
                                     bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_SET_EVENT,
        doc,
        obj,
        DOC_EVENT_TYPES[event_type],
        quote(event),
    }, ignore_reply);
}


bool WhiskerManager::displayClearEvent(const QString& doc,
                                       const QString& obj,
                                       DocEventType event_type,
                                       bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_CLEAR_EVENT,
        doc,
        obj,
        DOC_EVENT_TYPES[event_type],
    }, ignore_reply);
}


bool WhiskerManager::displaySetObjectEventTransparency(
        const QString& doc, const QString& obj,
        bool transparent, bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_SET_OBJ_EVENT_TRANSPARENCY,
        doc,
        obj,
        onVal(transparent),
    }, ignore_reply);
}


bool WhiskerManager::displayEventCoords(bool on, bool ignore_reply)
{
    return immBool({CMD_DISPLAY_EVENT_COORDS, onVal(on)}, ignore_reply);
}


bool WhiskerManager::displayBringToFront(const QString& doc,
                                         const QString& obj,
                                         bool ignore_reply)
{
    return immBool({CMD_DISPLAY_BRING_TO_FRONT, doc, obj}, ignore_reply);
}


bool WhiskerManager::displaySendToBack(const QString& doc,
                                       const QString& obj,
                                       bool ignore_reply)
{
    return immBool({CMD_DISPLAY_BRING_TO_FRONT, doc, obj}, ignore_reply);
}


bool WhiskerManager::displayKeyboardEvents(const QString& doc,
                                           KeyEventType key_event_type,
                                           bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_KEYBOARD_EVENTS,
        doc,
        KEY_EVENT_TYPES[key_event_type],
    }, ignore_reply);
}


bool WhiskerManager::displayCacheChanges(const QString& doc,
                                         bool ignore_reply)
{
    return immBool({CMD_DISPLAY_CACHE_CHANGES, doc}, ignore_reply);
}


bool WhiskerManager::displayShowChanges(const QString& doc,
                                        bool ignore_reply)
{
    return immBool({CMD_DISPLAY_SHOW_CHANGES, doc}, ignore_reply);
}


QSize WhiskerManager::displayGetDocumentSize(const QString& doc)
{
    const QString reply = immResp({CMD_DISPLAY_GET_DOCUMENT_SIZE, doc});
    const QStringList parts = reply.split(SPACE);
    if (parts.size() != 3 || parts.at(0) != MSG_SIZE) {
        return QSize();
    }
    bool ok;
    const int width = parts.at(1).toInt(&ok);
    if (!ok) {
        return QSize();
    }
    const int height = parts.at(2).toInt(&ok);
    if (!ok) {
        return QSize();
    }
    return QSize(width, height);
}


QRect WhiskerManager::displayGetObjectExtent(const QString& doc,
                                             const QString& obj)
{
    const QString reply = immResp({CMD_DISPLAY_GET_OBJECT_EXTENT, doc, obj});
    const QStringList parts = reply.split(SPACE);
    if (parts.size() != 5 || parts.at(0) != MSG_EXTENT) {
        return QRect();
    }
    bool ok;
    const int left = parts.at(1).toInt(&ok);
    if (!ok) {
        return QRect();
    }
    const int right = parts.at(2).toInt(&ok);
    if (!ok) {
        return QRect();
    }
    const int top = parts.at(3).toInt(&ok);
    if (!ok) {
        return QRect();
    }
    const int bottom = parts.at(4).toInt(&ok);
    if (!ok) {
        return QRect();
    }
    const int width = right - left;
    const int height = bottom - top;
    return QRect(left, top, width, height);
    // The Whisker coordinate system has its origin at the TOP LEFT, with
    // positive x to the right, and positive y down.
    // This is the same as the default Qt coordinate system.
}


bool WhiskerManager::displaySetBackgroundEvent(const QString& doc,
                                               const QString& event,
                                               DocEventType event_type,
                                               bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_SET_BACKGROUND_EVENT,
        doc,
        DOC_EVENT_TYPES[event_type],
        quote(event),
    }, ignore_reply);
}


bool WhiskerManager::displayClearBackgroundEvent(const QString& doc,
                                                 DocEventType event_type,
                                                 bool ignore_reply)
{
    return immBool({
        CMD_DISPLAY_CLEAR_BACKGROUND_EVENT,
        doc,
        DOC_EVENT_TYPES[event_type],
    }, ignore_reply);
}

// ----------------------------------------------------------------------------
// Whisker command set: display: specific object creation
// ----------------------------------------------------------------------------
// ... all superseded by calls to displayAddObject().


// ----------------------------------------------------------------------------
// Whisker command set: display: video extras
// ----------------------------------------------------------------------------

bool WhiskerManager::displaySetAudioDevice(const QString& display_device,
                                           const QString& audio_device,
                                           bool ignore_reply)
{
    // Devices may be specified as numbers or names.
    return immBool(
        {CMD_DISPLAY_SET_AUDIO_DEVICE, display_device, audio_device},
        ignore_reply
    );
}


bool WhiskerManager::videoPlay(const QString& doc, const QString& video,
                               bool ignore_reply)
{
    return immBool({CMD_VIDEO_PLAY, doc, video}, ignore_reply);
}


bool WhiskerManager::videoPause(const QString& doc, const QString& video,
                                bool ignore_reply)
{
    return immBool({CMD_VIDEO_PAUSE, doc, video}, ignore_reply);
}


bool WhiskerManager::videoStop(const QString& doc, const QString& video,
                               bool ignore_reply)
{
    return immBool({CMD_VIDEO_STOP, doc, video}, ignore_reply);
}


bool WhiskerManager::videoTimestamps(bool on, bool ignore_reply)
{
    return immBool({CMD_VIDEO_TIMESTAMPS, onVal(on)}, ignore_reply);
}


unsigned int WhiskerManager::videoGetTimeMs(const QString& doc,
                                            const QString& video,
                                            bool* ok)
{
    const QString reply = immResp({CMD_VIDEO_GET_TIME, doc, video});
    const QStringList parts = reply.split(SPACE);
    const unsigned int failure = 0;
    if (parts.size() != 2 || parts.at(0) != MSG_VIDEO_TIME) {
        if (ok) {
            *ok = false;
        }
        return failure;
    }
    return parts.at(1).toUInt(ok);
}


unsigned int WhiskerManager::videoGetDurationMs(const QString& doc,
                                                const QString& video,
                                                bool* ok)
{
    const QString reply = immResp({CMD_VIDEO_GET_DURATION, doc, video});
    const QStringList parts = reply.split(SPACE);
    const unsigned int failure = 0;
    if (parts.size() != 2 || parts.at(0) != MSG_DURATION) {
        if (ok) {
            *ok = false;
        }
        return failure;
    }
    return parts.at(1).toUInt(ok);
}


bool WhiskerManager::videoSeekRelative(const QString& doc,
                                       const QString& video,
                                       int relative_time_ms,
                                       bool ignore_reply)
{
    return immBool({
        CMD_VIDEO_SEEK_RELATIVE, doc, video, QString::number(relative_time_ms)
    }, ignore_reply);
}


bool WhiskerManager::videoSeekAbsolute(const QString& doc,
                                       const QString& video,
                                       unsigned int absolute_time_ms,
                                       bool ignore_reply)
{
    return immBool({
        CMD_VIDEO_SEEK_ABSOLUTE, doc, video, QString::number(absolute_time_ms)
    }, ignore_reply);
}


bool WhiskerManager::videoSetVolume(const QString& doc, const QString& video,
                                    unsigned int volume, bool ignore_reply)
{
    return immBool({
        CMD_VIDEO_SET_VOLUME, doc, video, QString::number(volume)
    }, ignore_reply);
}


// ----------------------------------------------------------------------------
// Shortcuts to Whisker commands
// ----------------------------------------------------------------------------

bool WhiskerManager::lineOn(const QString& line, bool ignore_reply)
{
    return lineSetState(line, true, ignore_reply);
}


bool WhiskerManager::lineOff(const QString& line, bool ignore_reply)
{
    return lineSetState(line, false, ignore_reply);
}


bool WhiskerManager::broadcast(const QString& message, bool ignore_reply)
{
    return sendToClient(VAL_BROADCAST_TO_ALL_CLIENTS, message, ignore_reply);
}
