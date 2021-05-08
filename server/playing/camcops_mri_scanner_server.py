#!/usr/bin/env python

"""
playing/camcops_mri_scanner_server.py

===============================================================================

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
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

**Pretend to be an MRI scanner.**

"""

import argparse
import datetime
import dateutil.tz
import logging
import random
from typing import Any, Tuple

from twisted.internet import reactor
# from twisted.internet.stdio import StandardIO
from twisted.internet.protocol import connectionDone, Factory, Protocol
from twisted.internet.serialport import (
    SerialPort, EIGHTBITS, PARITY_NONE, STOPBITS_ONE
)
from twisted.internet.task import LoopingCall
from twisted.protocols.basic import LineReceiver
from twisted.python.failure import Failure
# http://twistedmatrix.com/documents/current/core/howto/servers.html

"""
ADDITIONAL REQUIREMENTS:
- twisted
    sudo pip install twisted
- pyserial (install with pip)
- pygame
    On OS X:
        brew install sdl sdl_image sdl_mixer sdl_ttf portmidi
        sudo pip install hg+http://bitbucket.org/pygame/pygame
"""

logging.basicConfig()
logger = logging.getLogger("camcops_mr")
logger.setLevel(logging.DEBUG)

# =============================================================================
# Messages to/from scanner (TS, FS)
# =============================================================================
# https://cni.stanford.edu/wiki/MR_Hardware

TS_TRIGGER = "[t]"
TS_START_PULSES = "[p]"

FS_PULSE = "p"

# =============================================================================
# Messages to/from response button box (TB, FB)
# =============================================================================

# TODO: MRI scanner server

# =============================================================================
# Messages to/from tablet (TT, FT)
# =============================================================================

TT_PULSE = "pulse"
TT_SYNTAX_ERROR = "syntax_error"
TT_TRIGGER = "trigger_sent"
TT_START_TIMING_PULSES = "start_timing_pulses_sent"
TT_BB = "buttonbox"
TT_KEY = "key"

FT_START_TIMING_PULSES = "start_timing_pulses"
FT_TRIGGER = "trigger"
FT_FAKE_PULSE = "fake_pulse"  # for debugging

# =============================================================================
# Other constants
# =============================================================================

ENCODING = "utf-8"
LOCALTZ = dateutil.tz.tzlocal()  # constant for a given run
ISO8601_FMT = "%Y-%m-%dT%H:%M:%S.%f%z"  # e.g. 2013-07-24T20:04:07.000000+0100
KEYBOARD_TICK_S = 0.001


# =============================================================================
# Support functions
# =============================================================================

def get_now() -> datetime.datetime:
    return datetime.datetime.now(LOCALTZ)


def coin(p: float) -> bool:
    return random.random() < p


# =============================================================================
# Communication with scanner
# =============================================================================
# http://stackoverflow.com/questions/4715340

class MRIProtocol(Protocol):
    delimiter = '\n'

    def __init__(self, network: TabletServerProtocolFactory) -> None:
        self.network = network
        network.scanner = self

    # noinspection PyPep8Naming
    def dataReceived(self, data: bytes) -> None:
        """
        Data received from scanner.
        Timestamp as early as possible.
        """
        now = get_now()
        data = data.decode(ENCODING)
        logger.info("Received from scanner: {}".format(data))
        self.network.from_scanner(data, now)

    # noinspection PyMethodMayBeStatic
    def tell_scanner(self, data: str) -> None:
        """
        Send information to scanner.
        """
        logger.info("Sending to scanner: {}".format(data))
        # WOULD DO SOMETHING HERE

    def fake_pulse(self):
        logger.debug("FAKING A PULSE")
        self.dataReceived(FS_PULSE.encode(ENCODING))


# =============================================================================
# Communication with button box
# =============================================================================
# http://stackoverflow.com/questions/4715340

class ButtonBoxProtocol(Protocol):

    def __init__(self, network: TabletServerProtocolFactory) -> None:
        self.network = network
        network.buttonbox = self

    # noinspection PyPep8Naming
    def dataReceived(self, data: bytes) -> None:
        """
        Data received from button box.
        Timestamp as early as possible.
        """
        now = get_now()
        data = data.decode(ENCODING)
        logger.info("Received from button box: {}".format(data))
        self.network.from_buttonbox(data, now)

    # noinspection PyMethodMayBeStatic
    def tell_buttonbox(self, data: str) -> None:
        """
        Send information to button box.
        """
        logger.info("Sending to button box: {}".format(data))
        # WOULD DO SOMETHING HERE


# =============================================================================
# Communication with keyboard (or button box emulating keyboard)
# =============================================================================

class StubbornlyLineBasedKeyboardProtocol(LineReceiver):

    def __init__(self, network: TabletServerProtocolFactory) -> None:
        self.network = network

    # noinspection PyPep8Naming
    def connectionMade(self) -> None:
        self.setRawMode()

    # noinspection PyPep8Naming
    def rawDataReceived(self, data: bytes) -> None:
        """
        Data received from keyboard.
        Timestamp as early as possible.
        """
        now = get_now()
        data = data.decode(ENCODING)
        logger.info("Received from keyboard: {}".format(data))
        self.network.from_keyboard(data, now)

    # noinspection PyPep8Naming
    def lineReceived(self, line: bytes) -> None:
        pass


# may need pygame
# see also http://stackoverflow.com/questions/12469827
# and http://stackoverflow.com/questions/510357


class KeyboardPoller(object):

    def __init__(self, network: TabletServerProtocolFactory) -> None:
        self.network = network
        # self.nticks = 0

    def send_key(self, key: str) -> None:
        now = get_now()
        logger.info("Received from keyboard: {}".format(key))
        self.network.from_keyboard(key, now)

    def tick(self) -> None:
        # self.nticks += 1
        # if self.nticks % 1000 == 0:
        #     logger.info("tock: {} ticks".format(self.nticks))
        if coin(0.001):
            self.send_key("X")


# =============================================================================
# Tablet server, to communicate with tablet
# =============================================================================

class TabletServerProtocol(LineReceiver):
    # alter "delimiter" if necessary

    def __init__(self) -> None:
        self.start_time = get_now()
        self.peer = None
        self.npulses = 0

    # -------------------------------------------------------------------------
    # Connection handling
    # -------------------------------------------------------------------------

    # noinspection PyPep8Naming
    def connectionMade(self) -> None:
        """
        Overrides Twisted function.
        """
        self.factory.client_list.append(self)
        peer = self.transport.getPeer()
        logger.info("Incoming connection from: {}.".format(str(peer)))
        if hasattr(peer, "host") and hasattr(peer, "port"):
            self.peer = "{h}:{p}".format(h=peer.host, p=peer.port)

    # noinspection PyUnusedLocal,PyPep8Naming
    def connectionLost(self, reason: Failure = connectionDone) -> None:
        """
        Overrides Twisted function.
        """
        if self in self.factory.client_list:
            self.factory.client_list.remove(self)

    # -------------------------------------------------------------------------
    # Outbound information
    # -------------------------------------------------------------------------

    def tell_tablet(self,
                    data: str,
                    now: datetime.datetime = None,
                    *args: Any) -> None:
        """
        Send information to tablet, with timestamps.
        Comma-separed columns, containing:
            relative time in seconds (relative to command to start timing)
            ISO-8601 absolute time
            message
            [any additional parameters]
                ... for "pulse": pulse number (1-based)
        """
        abs_time, rel_time = self.get_abs_rel_time(now)
        # msg = "{d},{r},{a}".format(d=data, r=rel_time, a=abs_time)
        msg = ",".join(str(x) for x in [
            rel_time,
            abs_time,
            data
        ] + list(args))
        logger.info("Sending to tablet<{p}>: {m}".format(
            p=self.peer, m=msg))
        self.sendLine(msg.encode(ENCODING))

    def tell_scanner(self, data: str) -> None:
        """
        Send information to scanner.
        """
        self.factory.tell_scanner(data)

    def tell_buttonbox(self, data: str) -> None:
        """
        Send information to buttonbox.
        """
        self.factory.tell_buttonbox(data)

    # -------------------------------------------------------------------------
    # Support functions
    # -------------------------------------------------------------------------

    def get_abs_rel_time(self, now: datetime.datetime = None) \
            -> Tuple[str, float]:
        """
        Returns tuple:
            absolute time in ISO-8601 format
            relative time in seconds
        """
        if now is None:
            now = get_now()
        nowstr = now.strftime(ISO8601_FMT)
        timediff = now - self.start_time
        return nowstr, timediff.total_seconds()

    # -------------------------------------------------------------------------
    # From tablet
    # -------------------------------------------------------------------------

    # noinspection PyPep8Naming
    def rawDataReceived(self, data: bytes) -> None:
        pass

    # noinspection PyPep8Naming
    def lineReceived(self, data: bytes) -> None:
        """
        Override of Twisted function. Data received from tablet via TCP.
        """
        data = data.decode(ENCODING)
        logger.debug("Line received from tablet<{p}>: {d}".format(
            p=self.peer, d=data))
        if data == FT_START_TIMING_PULSES:
            self.ft_want_pulses()
        elif data == FT_TRIGGER:
            self.ft_trigger()
        elif data == FT_FAKE_PULSE:
            self.factory.fake_pulse()
        else:
            logger.warning("Unknown command received from tablet<{p}>: "
                           "{d}".format(p=self.peer, d=data))
            self.tell_tablet(TT_SYNTAX_ERROR)

    def ft_trigger(self) -> None:
        """
        Tablet has sent us a trigger. Tell the scanner.
        """
        now = get_now()
        self.tell_scanner(TS_TRIGGER)
        self.tell_tablet(TT_TRIGGER, now)

    def ft_want_pulses(self) -> None:
        """
        Tablet wants timing pulses from the scanner.
        Tell the scanner. Reset local timing.
        """
        self.start_time = get_now()
        self.npulses = 0
        self.tell_scanner(TS_START_PULSES)
        self.tell_tablet(TT_START_TIMING_PULSES, self.start_time)

    # -------------------------------------------------------------------------
    # From scanner
    # -------------------------------------------------------------------------

    def from_scanner(self, data: str, now: datetime.datetime) -> None:
        """
        Information received from scanner, via factory. Parse.
        """
        if data == FS_PULSE:
            self.fs_pulse(now)
        else:
            logger.warning("Unknown command received from scanner: "
                           "{}".format(data))

    def fs_pulse(self, now: datetime.datetime) -> None:
        """
        Scanner has sent us a pulse. Tell the tablet.
        """
        self.npulses += 1
        self.tell_tablet(TT_PULSE, now, self.npulses)

    # -------------------------------------------------------------------------
    # From button box
    # -------------------------------------------------------------------------

    def from_buttonbox(self, data: str, now: datetime.datetime) -> None:
        """
        Information received from button box. Parse.
        """
        self.tell_tablet(TT_BB, now, data)

    # -------------------------------------------------------------------------
    # From keyboard
    # -------------------------------------------------------------------------

    def from_keyboard(self, data: str, now: datetime.datetime) -> None:
        self.tell_tablet(TT_KEY, now, data)


# =============================================================================
# Tablet server factory, to create and manage instances of tablet servers
# =============================================================================

class TabletServerProtocolFactory(Factory):
    protocol = TabletServerProtocol
    # ... registers the protocol to be manufactured

    def __init__(self):
        """
        The factory's client list is edited when clients connect/disconnect, by
        the clients themselves. The factory also knows about a single scanner
        and button box.
        """
        self.client_list = []
        self.scanner = None
        self.buttonbox = None

    def from_scanner(self, data: str, now: datetime.datetime) -> None:
        """
        Pass information from scanner to tablet(s).
        """
        for client in self.client_list:
            client.from_scanner(data, now)

    def tell_scanner(self, data: str) -> None:
        """
        Pass command to scanner.
        """
        if not self.scanner:
            return
        self.scanner.tell_scanner(data)

    def fake_pulse(self) -> None:
        """
        Ask the scanner handler to fake a pulse."""
        if not self.scanner:
            return
        self.scanner.fake_pulse()

    def from_buttonbox(self, data: str, now: datetime.datetime) -> None:
        """
        Pass information from buttonbox to tablet(s).
        """
        for client in self.client_list:
            client.from_buttonbox(data, now)

    def tell_buttonbox(self, data: str) -> None:
        """
        Pass command to button box.
        """
        if not self.buttonbox:
            return
        self.buttonbox.tell_buttonbox(data)

    def from_keyboard(self, data: str, now: datetime.datetime) -> None:
        """
        Pass information from keyboard to tablet(s).
        """
        for client in self.client_list:
            client.from_keyboard(data, now)


# =============================================================================
# Main
# =============================================================================

def main() -> None:
    """
    Connect to MRI scanner and button box. Start network comms.
    """
    # Fetch command-line options.
    parser = argparse.ArgumentParser(
        prog="camcops_mri_scanner_server",  # name the user will use to call it
        description="CamCOPS MRI scanner/button box interface server."
    )
    parser.add_argument("--port", type=int, default=3233,
                        help="TCP port for network communications")
    # ... Suboptimal, but this is the Whisker port number

    parser.add_argument("--mri_serialdev", type=str,
                        default="/dev/cu.Bluetooth-Serial-1",
                        help=("Device to talk to MRI scanner "
                              "(e.g. Linux /dev/XXX; Windows COM4)"))
    parser.add_argument("--mri_baudrate", type=int, default=19200,
                        help="MRI scanner: baud rate (e.g. 19200)")
    parser.add_argument("--mri_bytesize", type=int, default=EIGHTBITS,
                        help="MRI scanner: number of bits (e.g. 8)")
    parser.add_argument("--mri_parity", type=str, default=PARITY_NONE,
                        help="MRI scanner: parity (e.g. N)")
    parser.add_argument("--mri_stopbits", type=int, default=STOPBITS_ONE,
                        help="MRI scanner: stop bits (e.g. 1)")
    parser.add_argument("--mri_xonxoff", type=int, default=0,
                        help="MRI scanner: use XON/XOFF (0 or 1)")
    parser.add_argument("--mri_rtscts", type=int, default=0,
                        help="MRI scanner: use RTS/CTS (0 or 1)")

    parser.add_argument("--bb_serialdev", type=str,
                        default="/dev/cu.Bluetooth-Serial-2",
                        help=("Device to talk to button box "
                              "(e.g. Linux /dev/YYY; Windows COM5)"))
    parser.add_argument("--bb_baudrate", type=int, default=19200,
                        help="Button box: baud rate (e.g. 19200)")
    parser.add_argument("--bb_bytesize", type=int, default=EIGHTBITS,
                        help="Button box: number of bits (e.g. 8)")
    parser.add_argument("--bb_parity", type=str, default=PARITY_NONE,
                        help="Button box: parity (e.g. N)")
    parser.add_argument("--bb_stopbits", type=int, default=STOPBITS_ONE,
                        help="Button box: stop bits (e.g. 1)")
    parser.add_argument("--bb_xonxoff", type=int, default=0,
                        help="Button box: use XON/XOFF (0 or 1)")
    parser.add_argument("--bb_rtscts", type=int, default=0,
                        help="Button box: use RTS/CTS (0 or 1)")

    args = parser.parse_args()

    tcpfactory = TabletServerProtocolFactory()

    logger.info(
        "MRI scanner: starting serial comms on device {dev} "
        "({baud} bps, {b}{p}{s})".format(
            dev=args.mri_serialdev,
            baud=args.mri_baudrate,
            b=args.mri_bytesize,
            p=args.mri_parity,
            s=args.mri_stopbits))
    SerialPort(protocol=MRIProtocol(tcpfactory),
               deviceNameOrPortNumber=args.mri_serialdev,
               reactor=reactor,
               baudrate=args.mri_baudrate,
               bytesize=args.mri_bytesize,
               parity=args.mri_parity,
               stopbits=args.mri_stopbits,
               xonxoff=args.mri_xonxoff,
               rtscts=args.mri_rtscts)

    logger.info(
        "Response button box: starting serial comms on device {dev} "
        "({baud} bps, {b}{p}{s})".format(
            dev=args.bb_serialdev,
            baud=args.bb_baudrate,
            b=args.bb_bytesize,
            p=args.bb_parity,
            s=args.bb_stopbits))
    SerialPort(protocol=ButtonBoxProtocol(tcpfactory),
               deviceNameOrPortNumber=args.bb_serialdev,
               reactor=reactor,
               baudrate=args.bb_baudrate,
               bytesize=args.bb_bytesize,
               parity=args.bb_parity,
               stopbits=args.bb_stopbits,
               xonxoff=args.bb_xonxoff,
               rtscts=args.bb_rtscts)

    logger.info("Starting keyboard input")

    # Keyboard: method 1
    # StandardIO(StubbornlyLineBasedKeyboardProtocol(tcpfactory))

    # Keyboard: method 2
    k = KeyboardPoller(tcpfactory)
    lc = LoopingCall(k.tick)
    lc.start(KEYBOARD_TICK_S)

    logger.info("Starting network comms on port {}".format(args.port))
    logger.debug('To debug, try: "telnet localhost {}"'.format(args.port))
    reactor.listenTCP(args.port, tcpfactory)
    reactor.run()


if __name__ == '__main__':
    main()
