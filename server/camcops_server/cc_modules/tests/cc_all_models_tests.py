#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_all_models_tests.py

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

from camcops_server.cc_modules.cc_sqlalchemy import log_all_ddl
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase


class ModelTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    # noinspection PyMethodMayBeStatic
    def test_show_ddl(self) -> None:
        log_all_ddl()
