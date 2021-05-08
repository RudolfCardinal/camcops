#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_text_tests.py

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

from camcops_server.cc_modules.cc_text import server_string, SS
from camcops_server.cc_modules.cc_unittest import DemoRequestTestCase


class TextTestCase(DemoRequestTestCase):
    """
    Unit tests.
    """
    def test_server_string(self) -> None:
        for k in SS.__dict__.keys():
            if k.startswith("_"):
                continue
            w = SS[k]
            assert isinstance(server_string(self.req, w), str)
