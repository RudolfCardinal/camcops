#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_patient_tests.py

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
import pendulum

from camcops_server.cc_modules.cc_simpleobjects import BarePatientInfo
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_simpleobjects import IdNumReference
from camcops_server.cc_modules.cc_tsv import TsvPage
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase
from camcops_server.cc_modules.cc_xml import XmlElement


# =============================================================================
# Unit tests
# =============================================================================

class PatientTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_patient(self) -> None:
        self.announce("test_patient")
        from camcops_server.cc_modules.cc_group import Group

        req = self.req
        q = self.dbsession.query(Patient)
        p = q.first()  # type: Patient
        assert p, "Missing Patient in demo database!"

        for pidnum in p.get_idnum_objects():
            self.assertIsInstance(pidnum, PatientIdNum)
        for idref in p.get_idnum_references():
            self.assertIsInstance(idref, IdNumReference)
        for idnum in p.get_idnum_raw_values_only():
            self.assertIsInstance(idnum, int)
        self.assertIsInstance(p.get_xml_root(req), XmlElement)
        self.assertIsInstance(p.get_tsv_page(req), TsvPage)
        self.assertIsInstance(p.get_bare_ptinfo(), BarePatientInfo)
        self.assertIsInstanceOrNone(p.group, Group)
        self.assertIsInstance(p.satisfies_upload_id_policy(), bool)
        self.assertIsInstance(p.satisfies_finalize_id_policy(), bool)
        self.assertIsInstance(p.get_surname(), str)
        self.assertIsInstance(p.get_forename(), str)
        self.assertIsInstance(p.get_surname_forename_upper(), str)
        for longform in [True, False]:
            self.assertIsInstance(p.get_dob_html(req, longform), str)
        age_str_int = p.get_age(req)
        assert isinstance(age_str_int, str) or isinstance(age_str_int, int)
        self.assertIsInstanceOrNone(p.get_dob(), pendulum.Date)
        self.assertIsInstanceOrNone(p.get_dob_str(), str)
        age_at_str_int = p.get_age_at(req.now)
        assert isinstance(age_at_str_int, str) or isinstance(age_at_str_int, int)  # noqa
        self.assertIsInstance(p.is_female(), bool)
        self.assertIsInstance(p.is_male(), bool)
        self.assertIsInstance(p.get_sex(), str)
        self.assertIsInstance(p.get_sex_verbose(), str)
        self.assertIsInstance(p.get_address(), str)
        self.assertIsInstance(p.get_email(), str)
        self.assertIsInstance(p.get_hl7_pid_segment(req, self.recipdef),
                              hl7.Segment)
        self.assertIsInstanceOrNone(p.get_idnum_object(which_idnum=1),
                                    PatientIdNum)
        self.assertIsInstanceOrNone(p.get_idnum_value(which_idnum=1), int)
        self.assertIsInstance(p.get_iddesc(req, which_idnum=1), str)
        self.assertIsInstance(p.get_idshortdesc(req, which_idnum=1), str)
        self.assertIsInstance(p.is_preserved(), bool)
        self.assertIsInstance(p.is_finalized(), bool)
        self.assertIsInstance(p.user_may_edit(req), bool)

    def test_surname_forename_upper(self) -> None:
        patient = Patient()
        patient.forename = "Forename"
        patient.surname = "Surname"

        self.assertEqual(patient.get_surname_forename_upper(),
                         "SURNAME, FORENAME")

    def test_surname_forename_upper_no_forename(self) -> None:
        patient = Patient()
        patient.surname = "Surname"

        self.assertEqual(patient.get_surname_forename_upper(),
                         "SURNAME, (UNKNOWN)")

    def test_surname_forename_upper_no_surname(self) -> None:
        patient = Patient()
        patient.forename = "Forename"

        self.assertEqual(patient.get_surname_forename_upper(),
                         "(UNKNOWN), FORENAME")


class LineageTests(DemoDatabaseTestCase):
    def create_tasks(self) -> None:
        # Actually not creating any tasks but we don't want the patients
        # created by default in the baseclass

        # First record for patient 1
        self.set_era("2020-01-01")

        self.patient_1 = Patient()
        self.patient_1.id = 1
        self._apply_standard_db_fields(self.patient_1)
        self.dbsession.add(self.patient_1)

        # First ID number record for patient 1
        self.patient_idnum_1_1 = PatientIdNum()
        self.patient_idnum_1_1.id = 3
        self._apply_standard_db_fields(self.patient_idnum_1_1)
        self.patient_idnum_1_1.patient_id = 1
        self.patient_idnum_1_1.which_idnum = self.nhs_iddef.which_idnum
        self.patient_idnum_1_1.idnum_value = 555
        self.dbsession.add(self.patient_idnum_1_1)

        # Second ID number record for patient 1
        self.patient_idnum_1_2 = PatientIdNum()
        self.patient_idnum_1_2.id = 3
        self._apply_standard_db_fields(self.patient_idnum_1_2)
        # This one is not current
        self.patient_idnum_1_2._current = False
        self.patient_idnum_1_2.patient_id = 1
        self.patient_idnum_1_2.which_idnum = self.nhs_iddef.which_idnum
        self.patient_idnum_1_2.idnum_value = 555
        self.dbsession.add(self.patient_idnum_1_2)

        self.dbsession.commit()

    def test_gen_patient_idnums_even_noncurrent(self) -> None:
        idnums = list(self.patient_1.gen_patient_idnums_even_noncurrent())

        self.assertEqual(len(idnums), 2)
