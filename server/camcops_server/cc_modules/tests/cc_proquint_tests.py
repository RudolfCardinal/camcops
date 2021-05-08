"""
camcops_server/cc_modules/tests/cc_proquint_tests.py

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

import random
import uuid
from unittest import TestCase

from camcops_server.cc_modules.cc_proquint import (
    int_from_proquint,
    InvalidProquintException,
    proquint_from_int,
    proquint_from_uuid,
    uuid_from_proquint,
)


# =============================================================================
# Unit tests
# =============================================================================

class ProquintTest(TestCase):
    def test_int_encoded_as_proquint(self) -> None:
        self.assertEqual(proquint_from_int(0x493b05ee, 32), "hohur-bilov-j")

    def test_uuid_encoded_as_proquint(self) -> None:
        self.assertEqual(
            proquint_from_uuid(
                uuid.UUID("6457cb90-1ca0-47a7-9f40-767567819bee")
            ),
            "kidil-sovib-dufob-hivol-nutab-linuj-kivad-nozov-t"
        )

    def test_proquint_decoded_as_int(self) -> None:
        self.assertEqual(int_from_proquint("hohur-bilov-j"), 0x493b05ee)

    def test_proquint_decoded_as_uuid(self) -> None:
        self.assertEqual(
            uuid_from_proquint(
                "kidil-sovib-dufob-hivol-nutab-linuj-kivad-nozov-t"
            ),
            uuid.UUID("6457cb90-1ca0-47a7-9f40-767567819bee")
        )

    def test_ints_converted_to_proquints_and_back(self) -> None:
        for bits in [16, 32, 48, 64, 80, 96, 128, 256]:
            for i in range(1000):
                random_int = random.getrandbits(bits)

                encoded = proquint_from_int(random_int, bits)

                num_expected_words = bits // 16
                num_expected_dashes = num_expected_words
                check_character_length = 1
                expected_proquint_length = (5 * num_expected_words
                                            + num_expected_dashes
                                            + check_character_length)
                self.assertEqual(len(encoded), expected_proquint_length)

                decoded = int_from_proquint(encoded)

                self.assertEqual(
                    decoded,
                    random_int,
                    msg=(f"Conversion failed for {random_int}, "
                         f"encoded={encoded}, decoded={decoded} ")
                )

    def test_raises_when_bits_not_multiple_of_16(self) -> None:
        with self.assertRaises(ValueError) as cm:
            proquint_from_int(0, 5)

        self.assertEqual(str(cm.exception),
                         "size_in_bits (5) must be a multiple of 16")

    def test_raises_when_proquint_has_invalid_chars(self) -> None:
        with self.assertRaises(InvalidProquintException) as cm:
            int_from_proquint("lusab-rrrrr-s")

        self.assertEqual(
            str(cm.exception),
            "'lusab-rrrrr-s' contains invalid or transposed characters"
        )

    def test_raises_when_proquint_has_chars_in_wrong_order(self) -> None:
        with self.assertRaises(InvalidProquintException) as cm:
            int_from_proquint("lusab-abadu-b")

        self.assertEqual(
            str(cm.exception),
            "'lusab-abadu-b' contains invalid or transposed characters"
        )

    def test_raises_when_check_character_doesnt_match(self) -> None:
        with self.assertRaises(InvalidProquintException) as cm:
            int_from_proquint("hohur-dilov-j")

        self.assertEqual(
            str(cm.exception),
            "'hohur-dilov-j' is not valid (check character mismatch)"
        )
