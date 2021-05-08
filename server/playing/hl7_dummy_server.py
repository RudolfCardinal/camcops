#!/usr/bin/env python

"""
playing/hl7_dummy_server.py

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

**Pretend to be an HL7 server.**

"""

from __future__ import print_function
import hl7
import datetime
import dateutil
import dateutil.tz
import random
import time
from twisted.internet import protocol, reactor

# http://twistedmatrix.com/documents/current/core/howto/servers.html

# made by this code:
testmsg_1 = "MSH|^~\&|hl7_dummy_server.py||||20140619232037+0100||ACK|1|P" \
            "|2.3||||AL||UNICODE UTF-8|" \
            "\r" \
            "MSA|AE|20140619232037+0100|Failure|||"

# from http://hl7reference.com/HL7%20Specifications%20ORM-ORU.PDF
testmsg_2 = "MSH|^~\&|EMRDirect|Example Hospital|||20030226000000||ACK|" \
            "6162003124232500|P|2.3|||||||" \
            "\r" \
            "MSA|AA|20030226000000|Success||||"
testmsg_3 = "MSH|^~\&|EMRDirect|Example Hospital|||20030226000000||ACK|" \
            "6162003124232500|P|2.3|||||||" \
            "\r" \
            "MSA|AE|20030226000000|Failure||||"

SEGMENT_SEPARATOR = u"\r"
FIELD_SEPARATOR = u"|"
COMPONENT_SEPARATOR = u"^"
SUBCOMPONENT_SEPARATOR = u"&"
DATEFORMAT_HL7_DATETIME = "%Y%m%d%H%M%S%z"  # e.g. 20130724200407+0100
REPETITION_SEPARATOR = u"~"
ESCAPE_CHARACTER = u"\\"

SB = '\x0b'  # <SB>, vertical tab
EB = '\x1c'  # <EB>, file separator
CR = '\x0d'  # <CR>, \r
FF = '\x0c'  # <FF>, new page form feed

PROBABILITY_OF_TIMEOUT = 0.0
PROBABILITY_OF_SUCCESS = 1.0

DELAY_SEC = 0
# DELAY_SEC = 1


class Echo(protocol.Protocol):
    def dataReceived(self, data):
        self.transport.write(data)


class EchoFactory(protocol.Factory):
    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def buildProtocol(self, addr):
        return Echo()


class Hello(protocol.Protocol):
    def connectionMade(self):
        self.transport.write("Hello world")
        self.transport.loseConnection()


class HelloFactory(protocol.Factory):
    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def buildProtocol(self, addr):
        return Hello()


class HL7Server(protocol.Protocol):
    def connectionMade(self):
        print("Incoming connection: ", end="")
        if coin(PROBABILITY_OF_TIMEOUT):
            print("Deliberately failing to respond")
            return
        if DELAY_SEC:
            print("Delaying {} seconds... ".format(DELAY_SEC), end="")
            time.sleep(DELAY_SEC)
        success = coin(PROBABILITY_OF_SUCCESS)
        print("Sending HL7 ACK message with success = {}".format(success))

        # Our messages
        msg = make_hl7_ack(success)
        data = unicode(msg)

        # Testing other messages
        # data = testmsg_2 if success else testmsg_3
        # ... CamCOPS detects them correctly

        # wrap in MLLP gubbins?
        data = SB + data + EB + CR
        self.transport.write(data.encode('utf-8'))
        self.transport.loseConnection()


class HL7ServerFactory(protocol.Factory):
    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def buildProtocol(self, addr):
        return HL7Server()


def coin(p):
    return random.random() < p


def format_datetime(d, fmt, default=None):
    if d is None:
        return default
    return d.strftime(fmt)


def make_msh_segment(message_datetime):
    segment_id = u"MSH"
    encoding_characters = (COMPONENT_SEPARATOR + REPETITION_SEPARATOR +
                           ESCAPE_CHARACTER + SUBCOMPONENT_SEPARATOR)
    sending_application = u"hl7_dummy_server.py"
    sending_facility = ""
    receiving_application = ""
    receiving_facility = ""
    date_time_of_message = format_datetime(message_datetime,
                                           DATEFORMAT_HL7_DATETIME)
    security = ""
    message_type = u"ACK"
    message_control_id = "1"
    processing_id = "P"  # production (processing mode: current)
    version_id = "2.3"  # HL7 version
    sequence_number = ""
    continuation_pointer = ""
    accept_acknowledgement_type = ""
    application_acknowledgement_type = "AL"  # always
    country_code = ""
    character_set = "UNICODE UTF-8"
    principal_language_of_message = ""

    fields = [
        segment_id,
        # field separator inserted automatically; HL7 standard considers it a
        # field but the python-hl7 processor doesn't when it parses
        encoding_characters,
        sending_application,
        sending_facility,
        receiving_application,
        receiving_facility,
        date_time_of_message,
        security,
        message_type,
        message_control_id,
        processing_id,
        version_id,
        sequence_number,
        continuation_pointer,
        accept_acknowledgement_type,
        application_acknowledgement_type,
        country_code,
        character_set,
        principal_language_of_message,
    ]
    segment = hl7.Segment(FIELD_SEPARATOR, fields)
    return segment


def make_msa_segment(message_datetime, success):
    segment_id = u"MSA"
    acknowledgement_code = u"AA" if success else u"AE"
    message_control_id = format_datetime(message_datetime,
                                         DATEFORMAT_HL7_DATETIME)
    test_mesage = u"Success" if success else u"Failure"
    expected_sequence_number = ""
    delayed_acknowledgement_type = ""
    error_condition = ""

    fields = [
        segment_id,
        acknowledgement_code,
        message_control_id,
        test_mesage,
        expected_sequence_number,
        delayed_acknowledgement_type,
        error_condition,
    ]
    segment = hl7.Segment(FIELD_SEPARATOR, fields)
    return segment


def make_hl7_ack(success):
    message_datetime = datetime.datetime.now(dateutil.tz.tzlocal())
    segments = [
        make_msh_segment(message_datetime),
        make_msa_segment(message_datetime, success)
    ]
    msg = hl7.Message(SEGMENT_SEPARATOR, segments)
    return msg


def main(port=2575):
    print("Starting on port {}".format(port))
    print("P(timeout) = {}".format(PROBABILITY_OF_TIMEOUT))
    print("P(success) = {}".format(PROBABILITY_OF_SUCCESS))
    # reactor.listenTCP(port, EchoFactory())
    # reactor.listenTCP(port, HelloFactory())
    reactor.listenTCP(port, HL7ServerFactory())
    reactor.run()


if __name__ == '__main__':
    main()
