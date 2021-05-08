#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_validator_tests.py

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

from typing import List
import unittest

from camcops_server.cc_modules.cc_validators import (
    anchor,
    min_max_copies,
    one_or_more,
    STRING_VALIDATOR_TYPE,
    validate_alphanum,
    validate_alphanum_underscore,
    validate_email,
    validate_human_name,
    validate_ip_address,
    validate_new_password,
    validate_restricted_sql_search_literal,
    validate_task_tablename,
    zero_or_more,
)


# =============================================================================
# Unit tests
# =============================================================================

class ValidatorTests(unittest.TestCase):
    """
    Test our validators.
    """

    def good_bad(self, validator: STRING_VALIDATOR_TYPE,
                 good: List[str], bad: List[str]) -> None:
        """
        Test a validator with a bunch of known-good and known-bad strings.
        """
        for g in good:
            # print(f"Testing good: {g!r}")
            try:
                validator(g, None)
            except ValueError as e:
                print(f"Validator failed for good value {g!r}: {e}")
                raise
        for b in bad:
            # print(f"Testing bad: {b!r}")
            self.assertRaises(ValueError, validator, b)

    def test_regex_manipulation(self) -> None:
        self.assertEqual(anchor("x"), "^x$")
        self.assertEqual(anchor("x", anchor_start=False), "x$")
        self.assertEqual(anchor("x", anchor_end=False), "^x")
        self.assertEqual(anchor("x", anchor_start=False, anchor_end=False),
                         "x")

        self.assertEqual(zero_or_more("x"), "x*")
        self.assertEqual(one_or_more("x"), "x+")
        self.assertEqual(min_max_copies("x", max_count=5), "x{1,5}")
        self.assertEqual(min_max_copies("x", min_count=0, max_count=5),
                         "x{0,5}")

    def test_generic_validators(self) -> None:
        self.good_bad(
            validate_alphanum,
            good=["hello123"],
            bad=["hello!"]
        )
        self.good_bad(
            validate_alphanum_underscore,
            good=["hello123_blah"],
            bad=["hello!"]
        )
        self.good_bad(
            validate_human_name,
            good=[
                "Al-Assad",
                "Al Assad",
                "John",
                "João",
                "タロウ",
                "やまだ",
                "山田",
                "先生",
                "мыхаыл",
                "Θεοκλεια",
                # NOT WORKING: "आकाङ्क्षा",
                "علاء الدين",
                # NOT WORKING: "אַבְרָהָם",
                # NOT WORKING: "മലയാളം",
                "상",
                "D'Addario",
                "John-Doe",
                "P.A.M.",
            ],
            bad=[
                "hello!",
                "' --",
                # "<xss>",
                # "\"",
                # "Robert'); DROP TABLE students;--",
            ]
        )
        self.good_bad(
            validate_restricted_sql_search_literal,
            good=[
                "F20%",
                "F2_0",
                "F200",
            ],
            bad=[
                "F200!",
            ]
        )

    def test_email_validator(self) -> None:
        self.good_bad(
            validate_email,
            good=[
                "blah@somewhere.com",
                "r&d@sillydomain.co.uk",
            ],
            bad=[
                "plaintext",
                "plain.domain.com",
                "two@at@symbols.com",
            ]
        )

    def test_ip_address_validator(self) -> None:
        self.good_bad(
            validate_ip_address,
            good=[
                "127.0.0.1",
                "131.141.8.42",
                "0.0.0.0",
            ],
            bad=[
                "plaintext",
                "plain.domain.com",
                "two@at@symbols.com",
                "999.999.999.999",
            ]
        )

    def test_password_validator(self) -> None:
        self.good_bad(
            validate_new_password,
            good=[
                "gibberishfly93",
                "myotherarmadilloisintheworkshop",
            ],
            bad=[
                "",
                "                  ",
                "aork",
                "hastalavista",
            ]
        )

    def test_task_tablename_validator(self) -> None:
        self.good_bad(
            validate_task_tablename,
            good=[
                "phq9",
                "gad7_with_extra_bits",
            ],
            bad=[
                "7hah",
                "thing space",
                "table!",
                # ... and of course:
                "Robert'); DROP TABLE students;--",
            ]
        )
