#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_hl7_tests.py

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


import hl7
from pendulum import Date, DateTime as Pendulum

from camcops_server.cc_modules.cc_constants import FileType
from camcops_server.cc_modules.cc_hl7 import (
    escape_hl7_text,
    get_mod11_checkdigit,
    make_msh_segment,
    make_obr_segment,
    make_obx_segment,
    make_pid_segment,
)
from camcops_server.cc_modules.cc_simpleobjects import (
    HL7PatientIdentifier,
    TaskExportOptions,
)
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase
from camcops_server.tasks.phq9 import Phq9


# =============================================================================
# Unit tests
# =============================================================================

class HL7CoreTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_hl7core_func(self) -> None:
        self.announce("test_hl7core_func")

        pitlist = [
            HL7PatientIdentifier(pid="1", id_type="TT",
                                 assigning_authority="AA")
        ]
        # noinspection PyTypeChecker
        dob = Date.today()  # type: Date
        now = Pendulum.now()
        task = self.dbsession.query(Phq9).first()
        assert task, "Missing Phq9 in demo database!"

        self.assertIsInstance(get_mod11_checkdigit("12345"), str)
        self.assertIsInstance(get_mod11_checkdigit("badnumber"), str)
        self.assertIsInstance(get_mod11_checkdigit("None"), str)
        self.assertIsInstance(make_msh_segment(now, "control_id"), hl7.Segment)
        self.assertIsInstance(make_pid_segment(
            forename="fname",
            surname="sname",
            dob=dob,
            sex="M",
            address="Somewhere",
            patient_id_list=pitlist
        ), hl7.Segment)
        self.assertIsInstance(make_obr_segment(task), hl7.Segment)
        for task_format in [FileType.PDF, FileType.HTML, FileType.XML]:
            for comments in [True, False]:
                export_options = TaskExportOptions(
                    xml_include_comments=comments,
                    xml_with_header_comments=comments,
                )
                self.assertIsInstance(make_obx_segment(
                    req=self.req,
                    task=task,
                    task_format=task_format,
                    observation_identifier="obs_id",
                    observation_datetime=now,
                    responsible_observer="responsible_observer",
                    export_options=export_options,
                ), hl7.Segment)
        self.assertIsInstance(escape_hl7_text("blahblah"), str)
