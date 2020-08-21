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

/*

GENERAL THREADING APPROACH FOR WHISKER CLIENT
===============================================================================

- QTcpSocket can run via an event-driven system using the readyRead() signal,
  or a blocking system using waitForReadyRead(). The docs warn that
  waitForReadyRead() can fail randomly under Windows, so that means we must
  use readyRead() [http://doc.qt.io/qt-5/qabstractsocket.html#waitForReadyRead].

- We must presume that the end user will run the task on the GUI thread (which
  is the worst-case scenario; if a separate thread is used, it can do what it
  likes, but if it uses the GUI thread, it mustn't sit there and spin-wait).

- The Whisker side of things mustn't care which thread the user decides to run
  the task on, though. That means that if the task calls a function to send
  data, that data must cross to a socket-owning thread.
  For the main socket, the simplest thing is to use Qt signals/slots, since
  they handle the thread boundary. So that means something like:

    class WhiskerController {
    signals:
        void eventReceived(const QString& event);
    slots:
        void sendToServer(const QString& command);
    };

- Then the tricky bit is the blocking call:

    QString sendImmediateGetReply(const QString& command);

- Remember that there are several ways to use a QThread:
  - let the QThread do its default run(), which runs a Qt event loop, and
    give it (via QObject::moveToThread()) objects that do useful things via
    signals; or
  - override run().
  See http://doc.qt.io/qt-5/qthread.html.

- So the simplest way will be to have a WhiskerWorker that's derived from
  QObject, and for WhiskerManager to put that into a new thread.

*/

#include <QObject>
#include <QPointer>
#include <QSize>
#include <QThread>
#include "whisker/whiskerapi.h"
#include "whisker/whiskercallbackhandler.h"
#include "whisker/whiskerconstants.h"
#include "whisker/whiskerconnectionstate.h"

class CamcopsApp;
class WhiskerInboundMessage;
class WhiskerOutboundCommand;
class WhiskerWorker;


class WhiskerManager : public QObject
{
    // High-level object to communicate with a Whisker server, and provide its
    // API. Owned by the GUI thread. (Uses a worker thread for socket
    // communications.)

    Q_OBJECT
public:

    // Constructor.
    WhiskerManager(QObject* parent = nullptr,
                   const QString& sysevent_prefix = "sys");

    // Destructor.
    ~WhiskerManager();

    // Send a message via the main socket.
    void sendMain(const QString& command);
    void sendMain(const QStringList& args);
    void sendMain(std::initializer_list<QString> args);

    // Send a message via the immediate socket, ignoring the reply.
    void sendImmediateIgnoreReply(const QString& command);

    // Send a message via the immediate socket, returning the reply.
    WhiskerInboundMessage sendImmediateGetReply(const QString& command);
    QString immResp(const QString& command);
    QString immResp(const QStringList& args);
    QString immResp(std::initializer_list<QString> args);

    // Send a message via the immediate socket, returning "did the reply
    // indicate success?"
    bool immBool(const QString& command, bool ignore_reply = false);
    bool immBool(const QStringList& args, bool ignore_reply = false);
    bool immBool(std::initializer_list<QString> args, bool ignore_reply = false);

    // Connect to a Whisker server.
    void connectToServer(const QString& host, quint16 main_port);

    // Are we fully connected.
    bool isConnected() const;

    // Are we fully disconnected?
    bool isFullyDisconnected() const;

    // Provide a user alert that we are not connected.
    void alertNotConnected() const;

    // Calls disconnectAllWhiskerSignals(), then emits disconnectFromServer().
    void disconnectServerAndSignals(QObject* receiver);

signals:
    // this -> worker: "please disconnect from the Whisker server".
    void disconnectFromServer();

    // Worker -> this -> world: "Whisker connection state has changed".
    void connectionStateChanged(WhiskerConnectionState state);

    // Worker -> this -> world: "Fully connected to Whisker server."
    void onFullyConnected();

    // "Whisker message received."
    void messageReceived(const WhiskerInboundMessage& msg);

    // "Whisker event received."
    void eventReceived(const WhiskerInboundMessage& msg);

    // "Whisker key event received."
    void keyEventReceived(const WhiskerInboundMessage& msg);

    // "Whisker client-to-client message received."
    void clientMessageReceived(const WhiskerInboundMessage& msg);

    // "Warning received from Whisker."
    void warningReceived(const WhiskerInboundMessage& msg);

    // "Syntax error received from Whisker."
    void syntaxErrorReceived(const WhiskerInboundMessage& msg);

    // "Error received from Whisker."
    void errorReceived(const WhiskerInboundMessage& msg);

    // "Ping acknowledgement received from Whisker."
    void pingAckReceived(const WhiskerInboundMessage& msg);

    // this -> worker: "please connect to the Whisker server".
    void internalConnectToServer(const QString& host, quint16 main_port);

    // this -> worker: "send message to the Whisker server".
    void internalSend(const WhiskerOutboundCommand& cmd);

public slots:
    // worker -> this: "Message received from server main socket."
    void internalReceiveFromMainSocket(const WhiskerInboundMessage& msg);

    // Worker -> this -> world: "Whisker socket error has occurred."
    void onSocketError(const QString& msg);

protected:

    // Disconnect all signals from "this" to "receiver".
    void disconnectAllWhiskerSignals(QObject* receiver);

    // Return a new event name for a system event.
    // The name is of the format <m_sysevent_prefix><m_sysevent_counter><suffix>.
    QString getNewSysEvent(const QString& suffix = "");

    // Clear all user-defined Whisker event callbacks.
    void clearAllCallbacks();

    // Send a message to the Whisker server after a delay (using a Whisker
    // timer for that delay).
    // If the event name is not specified, a new system event name is created.
    void sendAfterDelay(unsigned int delay_ms, const QString& msg,
                        QString event = "");

    // Call a user function after a delay, via a Whisker timer event.
    // If the event name is not specified, a new system event name is created.
    void callAfterDelay(
            unsigned int delay_ms,
            const WhiskerCallbackDefinition::CallbackFunction& callback,
            QString event = "");

protected:
    QThread m_worker_thread;  // worker thread to talk to sockets
    QPointer<WhiskerWorker> m_worker;  // worker object; lives in worker thread
    QString m_sysevent_prefix;  // prefix for all "system" events
    quint64 m_sysevent_counter;  // counter to make system events unique
    WhiskerCallbackHandler m_internal_callback_handler;  // manages callbacks

    // ========================================================================
    // Whisker API: see http://www.whiskercontrol.com/
    // ========================================================================
public:

    // ------------------------------------------------------------------------
    // Whisker command set: comms, misc
    // ------------------------------------------------------------------------
    bool setTimestamps(bool on, bool ignore_reply = false);
    bool resetClock(bool ignore_reply = false);
    QString getServerVersion();
    float getServerVersionNumeric();
    unsigned int getServerTimeMs();
    int getClientNumber();
    bool permitClientMessages(bool permit, bool ignore_reply = false);
    bool sendToClient(int clientNum, const QString& message, bool ignore_reply = false);
    bool setMediaDirectory(const QString& directory, bool ignore_reply = false);
    bool reportName(const QString& name, bool ignore_reply = false);
    bool reportStatus(const QString& status, bool ignore_reply = false);
    bool reportComment(const QString& comment, bool ignore_reply = false);
    int getNetworkLatencyMs();  // whiskerconstants::FAILURE_INT for failure
    bool ping();
    bool shutdown(bool ignore_reply = false);
    QString authenticateGetChallenge(const QString& package, const QString& client_name);
    bool authenticateProvideResponse(const QString& response, bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Whisker command set: logs
    // ------------------------------------------------------------------------
    bool logOpen(const QString& filename, bool ignore_reply = false);
    bool logSetOptions(const whiskerapi::LogOptions& options, bool ignore_reply = false);
    bool logPause(bool ignore_reply = false);
    bool logResume(bool ignore_reply = false);
    bool logWrite(const QString& msg, bool ignore_reply = false);
    bool logClose(bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Whisker command set: timers
    // ------------------------------------------------------------------------
    bool timerSetEvent(const QString& event, unsigned int duration_ms,
                       int reload_count = 0, bool ignore_reply = false);
    bool timerClearEvent(const QString& event, bool ignore_reply = false);
    bool timerClearAllEvents(bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Whisker command set: claiming, relinquishing
    // ------------------------------------------------------------------------
    bool claimGroup(const QString& group, const QString& prefix = "",
                    const QString& suffix = "");
    bool lineClaim(
            unsigned int line_number,
            bool output,
            const QString& alias = "",
            whiskerconstants::ResetState reset_state = whiskerconstants::ResetState::Leave);
    bool lineClaim(
            const QString& group,
            const QString& device,
            bool output,
            const QString& alias = "",
            whiskerconstants::ResetState reset_state = whiskerconstants::ResetState::Leave);
    bool lineRelinquishAll(bool ignore_reply = false);
    bool lineSetAlias(unsigned int line_number, const QString& alias,
                      bool ignore_reply = false);
    bool lineSetAlias(const QString& existing_alias, const QString& new_alias,
                      bool ignore_reply = false);
    bool audioClaim(unsigned int device_number,  const QString& alias = "");
    bool audioClaim(const QString& group, const QString& device,
                    const QString& alias = "");
    bool audioSetAlias(unsigned int device_number, const QString& alias,
                       bool ignore_reply = false);
    bool audioSetAlias(const QString& existing_alias, const QString& new_alias,
                       bool ignore_reply = false);
    bool audioRelinquishAll(bool ignore_reply = false);
    bool displayClaim(unsigned int display_number, const QString& alias = "");
    bool displayClaim(const QString& group, const QString& device,
                      const QString& alias = "");
    bool displaySetAlias(unsigned int display_number, const QString& alias,
                         bool ignore_reply = false);
    bool displaySetAlias(const QString& existing_alias,
                         const QString& new_alias, bool ignore_reply = false);
    bool displayRelinquishAll(bool ignore_reply = false);
    bool displayCreateDevice(const QString& name,
                             whiskerapi::DisplayCreationOptions options);
    bool displayDeleteDevice(const QString& device, bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Whisker command set: lines
    // ------------------------------------------------------------------------
    bool lineSetState(const QString& line, bool on, bool ignore_reply = false);
    bool lineReadState(const QString& line, bool* ok = nullptr);
    bool lineSetEvent(
            const QString& line, const QString& event,
            whiskerconstants::LineEventType event_type = whiskerconstants::LineEventType::On,
            bool ignore_reply = false);
    bool lineClearEvent(const QString& event, bool ignore_reply);
    bool lineClearEventByLine(const QString& line,
                              whiskerconstants::LineEventType event_type,
                              bool ignore_reply = false);
    bool lineClearAllEvents(bool ignore_reply = false);
    bool lineSetSafetyTimer(const QString& line,
                            unsigned int time_ms,
                            whiskerconstants::SafetyState safety_state,
                            bool ignore_reply = false);
    bool lineClearSafetyTimer(const QString& line, bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Whisker command set: audio
    // ------------------------------------------------------------------------
    bool audioPlayWav(const QString& device, const QString& filename,
                      bool ignore_reply = false);
    bool audioLoadTone(const QString& device, const QString& sound_name,
                       unsigned int frequency_hz,
                       whiskerconstants::ToneType tone_type,
                       unsigned int duration_ms,
                       bool ignore_reply = false);
    bool audioLoadWav(const QString& device, const QString& sound_name,
                      const QString& filename, bool ignore_reply = false);
    bool audioPlaySound(const QString& device, const QString& sound_name,
                        bool loop = false, bool ignore_reply = false);
    bool audioUnloadSound(const QString& device, const QString& sound_name,
                          bool ignore_reply = false);
    bool audioStopSound(const QString& device, const QString& sound_name,
                        bool ignore_reply = false);
    bool audioSilenceDevice(const QString& device, bool ignore_reply = false);
    bool audioUnloadAll(const QString& device, bool ignore_reply = false);
    bool audioSetSoundVolume(const QString& device, const QString& sound_name,
                             unsigned int volume, bool ignore_reply = false);
    bool audioSilenceAllDevices(bool ignore_reply = false);
    unsigned int audioGetSoundDurationMs(const QString& device,
                                         const QString& sound_name,
                                         bool* ok = nullptr);

    // ------------------------------------------------------------------------
    // Whisker command set: display: display operations
    // ------------------------------------------------------------------------
    QSize displayGetSize(const QString& device);
    bool displayScaleDocuments(const QString& device, bool scale = true,
                               bool ignore_reply = false);
    bool displayShowDocument(const QString& device, const QString& doc,
                             bool ignore_reply = false);
    bool displayBlank(const QString& device, bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Whisker command set: display: document operations
    // ------------------------------------------------------------------------
    bool displayCreateDocument(const QString& doc, bool ignore_reply = false);
    bool displayDeleteDocument(const QString& doc, bool ignore_reply = false);
    bool displaySetDocumentSize(const QString& doc, const QSize& size,
                                bool ignore_reply = false);
    bool displaySetBackgroundColour(const QString& doc, const QColor& colour,
                                    bool ignore_reply = false);
    bool displayDeleteObject(const QString& doc, const QString& obj,
                             bool ignore_reply = false);
    bool displayAddObject(
            const QString& doc, const QString& obj,
            const whiskerapi::DisplayObject& object_definition,
            bool ignore_reply = false);
    // ... can be used with any derived class too, e.g. TextObject
    bool displaySetEvent(
            const QString& doc, const QString& obj,
            whiskerconstants::DocEventType event_type,
            const QString& event,
            bool ignore_reply = false);
    bool displayClearEvent(
            const QString& doc, const QString& obj,
            whiskerconstants::DocEventType event_type,
            bool ignore_reply = false);
    bool displaySetObjectEventTransparency(
            const QString& doc, const QString& obj,
            bool transparent, bool ignore_reply = false);
    bool displayEventCoords(bool on, bool ignore_reply = false);
    bool displayBringToFront(const QString& doc, const QString& obj,
                             bool ignore_reply = false);
    bool displaySendToBack(const QString& doc, const QString& obj,
                           bool ignore_reply = false);
    bool displayKeyboardEvents(
            const QString& doc,
            whiskerconstants::KeyEventType key_event_type = whiskerconstants::KeyEventType::Down,
            bool ignore_reply = false);
    bool displayCacheChanges(const QString& doc, bool ignore_reply = false);
    bool displayShowChanges(const QString& doc, bool ignore_reply = false);
    QSize displayGetDocumentSize(const QString& doc);
    QRect displayGetObjectExtent(const QString& doc, const QString& obj);
    bool displaySetBackgroundEvent(
            const QString& doc,
            whiskerconstants::DocEventType event_type,
            const QString& event,
            bool ignore_reply = false);
    bool displayClearBackgroundEvent(
            const QString& doc,
            whiskerconstants::DocEventType event_type,
            bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Whisker command set: display: specific object creation
    // ------------------------------------------------------------------------
    // ... all superseded by calls to displayAddObject().

    // ------------------------------------------------------------------------
    // Whisker command set: display: video extras
    // ------------------------------------------------------------------------
    bool displaySetAudioDevice(const QString& display_device,
                               const QString& audio_device,
                               bool ignore_reply = false);
    bool videoPlay(const QString& doc, const QString& video,
                   bool ignore_reply = false);
    bool videoPause(const QString& doc, const QString& video,
                    bool ignore_reply = false);
    bool videoStop(const QString& doc, const QString& video,
                   bool ignore_reply = false);
    bool videoTimestamps(bool on, bool ignore_reply = false);
    unsigned int videoGetTimeMs(const QString& doc, const QString& video,
                                bool* ok = nullptr);
    unsigned int videoGetDurationMs(const QString& doc, const QString& video,
                                    bool* ok = nullptr);
    bool videoSeekRelative(const QString& doc, const QString& video,
                           int relative_time_ms, bool ignore_reply = false);
    bool videoSeekAbsolute(const QString& doc, const QString& video,
                           unsigned int absolute_time_ms,
                           bool ignore_reply = false);
    bool videoSetVolume(const QString& doc, const QString& video,
                        unsigned int volume, bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Shortcuts to Whisker commands
    // ------------------------------------------------------------------------

    // Shorthand for lineSetState(line, true, ignore_reply).
    bool lineOn(const QString& line, bool ignore_reply = false);

    // Shorthand for lineSetState(line, false, ignore_reply).
    bool lineOff(const QString& line, bool ignore_reply = false);

    // Broadcast to all other Whisker clients. Shorthand for
    // sendToClient(VAL_BROADCAST_TO_ALL_CLIENTS, message, ignore_reply).
    bool broadcast(const QString& message, bool ignore_reply = false);

    // ------------------------------------------------------------------------
    // Line flashing
    // ------------------------------------------------------------------------

    // "Flash" a digital output line "count" times, where the "on" phase lasts
    // on_ms and the "off" phase lasts off_ms.
    // - Flip on_at_rest for a line that is reversed (on by default and you are
    //   flashing it "off").
    // - Returns the total estimated time, in ms.
    unsigned int flashLinePulses(const QString& line,
                                 unsigned int count,
                                 unsigned int on_ms,
                                 unsigned int off_ms,
                                 bool on_at_rest = false);

protected:
    // Worker function for flashLinePulses().
    void flashLinePulsesOn(const QString& line,
                           unsigned int count,
                           unsigned int on_ms,
                           unsigned int off_ms,
                           bool on_at_rest);

    // Worker function for flashLinePulses().
    void flashLinePulsesOff(const QString& line,
                            unsigned int count,
                            unsigned int on_ms,
                            unsigned int off_ms,
                            bool on_at_rest);
};
