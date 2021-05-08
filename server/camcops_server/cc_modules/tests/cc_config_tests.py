#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_config_tests.py

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
"""

import configparser
from unittest import TestCase

from cardinal_pythonlib.sqlalchemy.logs import pre_disable_sqlalchemy_extra_echo_log  # noqa

from camcops_server.cc_modules.cc_config import CamcopsConfig, get_demo_config


# =============================================================================
# Unit tests
# =============================================================================

class EmailConfigTests(TestCase):

    def setUp(self):
        super().setUp()

        from io import StringIO

        # Start with a working config and just set the things we want to test
        config_text = get_demo_config()
        self.parser = configparser.ConfigParser()
        self.parser.read_string(config_text)

        self.parser.set("export", "RECIPIENTS", "recipient_A")
        self.parser.set("recipient:recipient_A", "TRANSMISSION_METHOD", "email")

        self.parser.set("site", "EMAIL_HOST", "smtp.example.com")
        self.parser.set("site", "EMAIL_PORT", "587")
        self.parser.set("site", "EMAIL_USE_TLS", "true")
        self.parser.set("site", "EMAIL_HOST_USERNAME", "username")
        self.parser.set("site", "EMAIL_HOST_PASSWORD", "mypassword")
        self.parser.set("site", "EMAIL_FROM",
                        "CamCOPS computer <from@example.com>")
        self.parser.set("site", "EMAIL_SENDER",
                        "CamCOPS computer <sender@example.com>")
        self.parser.set("site", "EMAIL_REPLY_TO",
                        "CamCOPS clinical administrator <admin@example.com>")

        with StringIO() as buffer:
            self.parser.write(buffer)
            self.config = CamcopsConfig(config_filename="",
                                        config_text=buffer.getvalue())

    def test_export_recipients_use_site_email_config(self) -> None:
        recipient = self.config._export_recipients[0]
        self.assertEqual(recipient.recipient_name, "recipient_A")

        self.assertEqual(recipient.email_host, "smtp.example.com")
        self.assertEqual(recipient.email_port, 587)
        self.assertTrue(recipient.email_use_tls)
        self.assertEqual(recipient.email_host_username, "username")
        self.assertEqual(recipient.email_host_password, "mypassword")
        self.assertEqual(recipient.email_from,
                         "CamCOPS computer <from@example.com>")
        self.assertEqual(recipient.email_sender,
                         "CamCOPS computer <sender@example.com>")
        self.assertEqual(recipient.email_reply_to,
                         "CamCOPS clinical administrator <admin@example.com>")
