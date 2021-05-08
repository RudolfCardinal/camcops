#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_device_tests.py

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

from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase


# =============================================================================
# Unit tests
# =============================================================================

class DeviceTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_device(self) -> None:
        self.announce("test_device")
        q = self.dbsession.query(Device)
        d = q.first()  # type: Device
        assert d, "Missing Device in demo database!"
        self.assertIsInstance(d.get_friendly_name(), str)
        self.assertIsInstance(d.get_id(), int)
        self.assertIsInstance(d.is_valid(), bool)
