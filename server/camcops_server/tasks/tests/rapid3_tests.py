#!/usr/bin/env python

"""
camcops_server/tasks/tests/rapid3_tests.py

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

from unittest import mock
import unittest

from camcops_server.tasks.rapid3 import Rapid3


# =============================================================================
# Unit tests
# =============================================================================

class Rapid3Tests(unittest.TestCase):
    def setUp(self) -> None:
        super().setUp()

        self.request = mock.Mock()

    def test_rapid3_calculation(self) -> None:
        rapid3 = Rapid3()

        # a-j total 13
        # expected FN = 13/3 = 4.3 (1 dp)
        rapid3.q1a = 1
        rapid3.q1b = 2
        rapid3.q1c = 3
        rapid3.q1d = 0
        rapid3.q1e = 1
        rapid3.q1f = 0
        rapid3.q1g = 3
        rapid3.q1h = 0
        rapid3.q1i = 1
        rapid3.q1j = 2

        # k-m not scored formally
        rapid3.q1k = 3
        rapid3.q1l = 0
        rapid3.q1m = 1

        rapid3.q2 = 0.5
        rapid3.q3 = 2.0

        # cumulative = 4.3 + 0.5 + 2.0 = 6.8

        self.assertEqual(rapid3.rapid3(), 6.8)

    def test_rapid3_none_when_field_none(self) -> None:
        rapid3 = Rapid3()

        self.assertIsNone(rapid3.rapid3())

    def test_complete_when_all_answers_valid(self) -> None:
        rapid3 = Rapid3()

        rapid3.q1a = 0
        rapid3.q1b = 0
        rapid3.q1c = 0
        rapid3.q1d = 0
        rapid3.q1e = 0
        rapid3.q1f = 0
        rapid3.q1g = 0
        rapid3.q1h = 0
        rapid3.q1i = 0
        rapid3.q1j = 0

        rapid3.q1k = 0
        rapid3.q1l = 0
        rapid3.q1m = 0

        rapid3.q2 = 0.0
        rapid3.q3 = 0.0

        self.assertTrue(rapid3.is_complete())

    def test_incomplete_when_any_field_none(self) -> None:
        all_fields = [
            "q1a",
            "q1b",
            "q1c",
            "q1d",
            "q1e",
            "q1f",
            "q1g",
            "q1h",
            "q1i",
            "q1j",
            "q1k",
            "q1l",
            "q1m",
            "q2",
            "q3",
        ]

        for none_field in all_fields:
            rapid3 = Rapid3()

            for field in all_fields:
                setattr(rapid3, field, 0.0)

            setattr(rapid3, none_field, None)
            self.assertFalse(rapid3.is_complete(),
                             msg=f"Failed when setting {none_field} to None")

    def test_incomplete_when_any_field_invalid(self) -> None:
        all_fields = [
            "q1a",
            "q1b",
            "q1c",
            "q1d",
            "q1e",
            "q1f",
            "q1g",
            "q1h",
            "q1i",
            "q1j",
            "q1k",
            "q1l",
            "q1m",
            "q2",
            "q3",
        ]

        for invalid_field in all_fields:
            rapid3 = Rapid3()

            for field in all_fields:
                setattr(rapid3, field, 0.0)

            setattr(rapid3, invalid_field, 10.5)
            self.assertFalse(
                rapid3.is_complete(),
                msg=f"Failed when setting {invalid_field} invalid")

    def test_disease_severity_n_a_for_none(self) -> None:
        rapid3 = Rapid3()

        with mock.patch.object(rapid3, "rapid3") as mock_rapid3:
            mock_rapid3.return_value = None
            with mock.patch.object(rapid3, "wxstring") as mock_wxstring:
                rapid3.disease_severity(self.request)

        mock_wxstring.assert_called_once_with(self.request, "n_a")

    def test_disease_severity_near_remission_for_3(self) -> None:
        rapid3 = Rapid3()

        with mock.patch.object(rapid3, "rapid3") as mock_rapid3:
            mock_rapid3.return_value = 3.0
            with mock.patch.object(rapid3, "wxstring") as mock_wxstring:
                rapid3.disease_severity(self.request)

        mock_wxstring.assert_called_once_with(self.request, "near_remission")

    def test_disease_severity_low_for_6(self) -> None:
        rapid3 = Rapid3()

        with mock.patch.object(rapid3, "rapid3") as mock_rapid3:
            mock_rapid3.return_value = 6
            with mock.patch.object(rapid3, "wxstring") as mock_wxstring:
                rapid3.disease_severity(self.request)

        mock_wxstring.assert_called_once_with(self.request, "low_severity")

    def test_disease_severity_moderate_for_12(self) -> None:
        rapid3 = Rapid3()

        with mock.patch.object(rapid3, "rapid3") as mock_rapid3:
            mock_rapid3.return_value = 12
            with mock.patch.object(rapid3, "wxstring") as mock_wxstring:
                rapid3.disease_severity(self.request)

        mock_wxstring.assert_called_once_with(self.request, "moderate_severity")

    def test_disease_severity_high_for_12point1(self) -> None:
        rapid3 = Rapid3()

        with mock.patch.object(rapid3, "rapid3") as mock_rapid3:
            mock_rapid3.return_value = 12.1
            with mock.patch.object(rapid3, "wxstring") as mock_wxstring:
                rapid3.disease_severity(self.request)

        mock_wxstring.assert_called_once_with(self.request, "high_severity")
