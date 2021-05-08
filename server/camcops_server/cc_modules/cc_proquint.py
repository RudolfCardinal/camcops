"""
camcops_server/cc_modules/cc_proquint.py

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

Convert integers into Pronounceable Quintuplets (proquints)
https://arxiv.org/html/0901.4016

Based on https://github.com/dsw/proquint, which has the following licence:

--8<---------------------------------------------------------------------------

Copyright (c) 2009 Daniel S. Wilkerson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

    Neither the name of Daniel S. Wilkerson nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

--8<---------------------------------------------------------------------------


"""
import uuid

CONSONANTS = "bdfghjklmnprstvz"
VOWELS = "aiou"

SIZE_OF_CONSONANT = 4
SIZE_OF_VOWEL = 2

LOOKUP_CONSONANTS = {
    'b': 0x0, 'd': 0x1, 'f': 0x2, 'g': 0x3,
    'h': 0x4, 'j': 0x5, 'k': 0x6, 'l': 0x7,
    'm': 0x8, 'n': 0x9, 'p': 0xa, 'r': 0xb,
    's': 0xc, 't': 0xd, 'v': 0xe, 'z': 0xf,
}
LOOKUP_VOWELS = {
    'a': 0x0, 'i': 0x1, 'o': 0x2, 'u': 0x3,
}
LOOKUP_TABLE = {
    **LOOKUP_CONSONANTS, **LOOKUP_VOWELS,
}


class InvalidProquintException(Exception):
    pass


def proquint_from_uuid(uuid_obj: uuid.UUID) -> str:
    """
    Convert UUID to proquint (via the UUID's 128-bit integer representation).
    """
    return proquint_from_int(uuid_obj.int, 128)


def proquint_from_int(int_value: int,
                      size_in_bits: int) -> str:
    """Convert integer value into proquint

    .. code-block:: none

        >>> proquint_from_int(0x493b05ee, 32)
        hohur-bilov

        0x493b05ee in binary is:
        0100 1001 0011 1011 - 0000 0101 1110 1110

        grouped into alternating 4 and 2 bit values:

        cons vo cons vo cons - cons vo cons vo cons
        0100 10 0100 11 1011 - 0000 01 0111 10 1110

           h  o    h  u    r -    b  i    l  o    v

    Args:
        int_value:
             integer value to encode
        size_in_bits:
             size of integer in bits (must be a multiple of 16)

    Returns:
        proquint string identifier
    """
    proquint = []

    if size_in_bits % 16 != 0:
        raise ValueError(
            f"size_in_bits ({size_in_bits}) must be a multiple of 16"
        )

    for i in range(size_in_bits // 16):
        proquint.insert(0, _proquint_from_int16(int_value & 0xffff))

        int_value >>= 16

    check_character = _generate_check_character("".join(proquint))

    proquint.append(check_character)

    return "-".join(proquint)


def _generate_check_character(proquint: str) -> str:
    """
    Luhn mod 16 check digit

    https://en.wikipedia.org/wiki/Luhn_mod_N_algorithm

    .. code-block:: none
        consonant_values = {
            'b': 0x0, 'd': 0x1, 'f': 0x2, 'g': 0x3,
            'h': 0x4, 'j': 0x5, 'k': 0x6, 'l': 0x7,
            'm': 0x8, 'n': 0x9, 'p': 0xa, 'r': 0xb,
            's': 0xc, 't': 0xd, 'v': 0xe, 'z': 0xf,
        }

        vowel_values = {
            'a': 0x0, 'i': 0x1, 'o': 0x2, 'u': 0x3,
        }

        To generate the check character, start with the last character in the
        string and move left doubling every other code-point. The "digits" of
        the code-points as written in hex (since there are 16 valid input
        characters) should then be summed up:

        Example (all in hex):

        hohur-bilov

        Character      h     o     h     u     r     b     i     l     o     v
        Code point     4     2     4     3     b     0     1     7     2     e
        Double               4           6           0           e          1c
        Reduce         4     4     4     6     b     0     1     e     2   1+c
        Sum            4     4     4     6     b     0     1     e     2     d

        Total sum = 4 + 4 + 4 + 6 + b + 0 + 1 + e + 2 + d = 0x3b
        Next multiple of 0x10 is 0x40

        Check character code = 0x40 - 0x3b = 0x5
        So check character is 'j'

    """

    remainder = _generate_luhn_mod_16_remainder(proquint, 2)

    check_code_point = (16 - remainder) % 16

    return CONSONANTS[check_code_point]


def _proquint_from_int16(int16_value: int) -> str:
    """
    Convert 16-bit integer into proquint.
    """
    proquint = []
    for i in range(5):
        if i & 1:
            letters = VOWELS
            mask = 0x3
            shift = SIZE_OF_VOWEL
        else:
            letters = CONSONANTS
            mask = 0xf
            shift = SIZE_OF_CONSONANT

        index = int16_value & mask
        proquint.insert(0, letters[index])
        int16_value >>= shift

    return ''.join(proquint)


def uuid_from_proquint(proquint: str) -> uuid.UUID:
    """
    Convert proquint to UUID.
    """
    int_value = int_from_proquint(proquint)

    return uuid.UUID(int=int_value)


def int_from_proquint(proquint: str) -> int:
    """
    Convert proquint string into integer.

    .. code-block:; none

        >>> hex(int_from_proquint('hohur-bilov-j'))
        0x493b05ee

           h    o    h    u    r -    b    i    l    o    v
         0x4  0x2  0x4  0x3  0xb -  0x0  0x1  0x7  0x2  0xe

        0100   10 0100   11 1011 - 0000   01 0111   10 1110
        0100    1001   0011 1011 - 0000    0101   1110 1110
         0x4     0x9    0x3  0xb -  0x0     0x5    0xe  0xe

    Args:
        proquint:
            string to decode
    Returns:
        converted integer value
    """

    int_value = 0

    words = proquint.split("-")

    if not _is_valid_proquint("".join(words)):
        raise InvalidProquintException(
            f"'{proquint}' is not valid (check character mismatch)"
        )

    # Remove check character
    words.pop()

    for word in words:
        for (i, c) in enumerate(word):
            if i & 1:
                lookup_table = LOOKUP_VOWELS
                shift = SIZE_OF_VOWEL
            else:
                lookup_table = LOOKUP_CONSONANTS
                shift = SIZE_OF_CONSONANT

            value = lookup_table.get(c)

            if value is None:
                raise InvalidProquintException(
                    f"'{proquint}' contains invalid or transposed characters"
                )

            int_value <<= shift
            int_value += value

    return int_value


def _is_valid_proquint(proquint: str) -> bool:
    """
    Does the proquint validate?
    """
    return _generate_luhn_mod_16_remainder(proquint, 1) == 0


def _generate_luhn_mod_16_remainder(proquint: str, start_factor: int) -> int:
    """
    Part of the checksum calculations; see :func:`_generate_check_character`.
    For a valid sequence, the overall remainder should be 0.
    See https://en.wikipedia.org/wiki/Luhn_mod_N_algorithm.
    """
    factor = start_factor
    sum_ = 0

    for char in reversed(proquint):
        value = LOOKUP_TABLE[char] * factor
        sum_ = sum_ + value // 16 + value % 16

        if factor == 2:
            factor = 1
        else:
            factor = 2

    return sum_ % 16
