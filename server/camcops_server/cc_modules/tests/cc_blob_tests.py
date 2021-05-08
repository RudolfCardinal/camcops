#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_blob_tests.py

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

from camcops_server.cc_modules.cc_blob import Blob
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase
from camcops_server.cc_modules.cc_xml import XmlElement


# =============================================================================
# Unit tests
# =============================================================================

class BlobTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_blob(self) -> None:
        self.announce("test_blob")
        q = self.dbsession.query(Blob)
        b = q.first()  # type: Blob
        assert b, "Missing BLOB in demo database!"
        self.assertIsInstanceOrNone(b.get_rotated_image(), bytes)
        self.assertIsInstance(b.get_img_html(), str)
        self.assertIsInstance(b.get_xml_element(self.req), XmlElement)
        self.assertIsInstance(b.get_data_url(), str)
