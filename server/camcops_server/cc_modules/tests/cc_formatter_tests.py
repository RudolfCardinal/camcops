#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_formatter_tests.py

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

from unittest import TestCase

from camcops_server.cc_modules.cc_formatter import SafeFormatter


class SafeFormatterTests(TestCase):
    def setUp(self) -> None:
        super().setUp()

        self.formatter = SafeFormatter(["forename", "surname", "email"])

    def test_formats_with_allowed_keys(self) -> None:
        output = self.formatter.format("{forename} {surname} <{email}>",
                                       forename="Erin", surname="Byrne",
                                       email="erin.byrne@example.com")
        self.assertEqual(output, "Erin Byrne <erin.byrne@example.com>")

    def test_format_raises_with_disallowed_keys(self) -> None:
        with self.assertRaises(KeyError):
            self.formatter.format("{email.__class__}")

    def test_returns_valid_parameters_string(self) -> None:
        self.assertEqual(self.formatter.get_valid_parameters_string(),
                         "{forename}, {surname}, {email}")

    def test_validate_raises_key_error_for_unknown_key(self) -> None:
        with self.assertRaises(KeyError):
            self.formatter.validate("{phone}")

    def test_validate_raises_value_error_for_mismatched_brackets(self) -> None:
        with self.assertRaises(ValueError):
            self.formatter.validate("{forename")
