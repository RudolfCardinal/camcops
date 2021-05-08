#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_tracker_tests.py

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

from camcops_server.cc_modules.cc_simpleobjects import IdNumReference
from camcops_server.cc_modules.cc_taskcollection import TaskFilter
from camcops_server.cc_modules.cc_tracker import ClinicalTextView, Tracker
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase


# =============================================================================
# Unit tests
# =============================================================================

class TrackerCtvTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def setUp(self) -> None:
        super().setUp()

        self.taskfilter = TaskFilter()

        idnum_ref = IdNumReference(which_idnum=0, idnum_value=0)

        self.taskfilter.idnum_criteria = [idnum_ref]
        self.taskfilter.tasks_with_patient_only = True

    def test_tracker(self) -> None:
        self.announce("test_tracker")
        req = self.req
        t = Tracker(req, self.taskfilter)

        self.assertIsInstance(t.get_html(), str)
        self.assertIsInstance(t.get_pdf(), bytes)
        self.assertIsInstance(t.get_pdf_html(), str)
        self.assertIsInstance(t.get_xml(), str)
        self.assertIsInstance(t.suggested_pdf_filename(), str)

    def test_ctv(self) -> None:
        self.announce("test_ctv")
        req = self.req
        c = ClinicalTextView(req, self.taskfilter)

        self.assertIsInstance(c.get_html(), str)
        self.assertIsInstance(c.get_pdf(), bytes)
        self.assertIsInstance(c.get_pdf_html(), str)
        self.assertIsInstance(c.get_xml(), str)
        self.assertIsInstance(c.suggested_pdf_filename(), str)
