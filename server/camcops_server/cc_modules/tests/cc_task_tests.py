#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_task_tests.py

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

import logging

from cardinal_pythonlib.logs import BraceStyleAdapter
from pendulum import Date, DateTime as Pendulum

from camcops_server.cc_modules.cc_task import Task
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase
from camcops_server.cc_modules.cc_validators import (
    validate_task_tablename,
)

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Unit testing
# =============================================================================

class TaskTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_query_phq9(self) -> None:
        self.announce("test_query_phq9")
        from camcops_server.tasks import Phq9
        phq9_query = self.dbsession.query(Phq9)
        results = phq9_query.all()
        log.info("{}", results)

    def test_all_tasks(self) -> None:
        self.announce("test_all_tasks")
        from datetime import date
        import hl7
        from sqlalchemy.sql.schema import Column
        from camcops_server.cc_modules.cc_ctvinfo import CtvInfo  # noqa: F811
        from camcops_server.cc_modules.cc_patient import Patient  # noqa: F811
        from camcops_server.cc_modules.cc_simpleobjects import IdNumReference
        from camcops_server.cc_modules.cc_snomed import SnomedExpression  # noqa: E501,F811
        from camcops_server.cc_modules.cc_string import APPSTRING_TASKNAME
        from camcops_server.cc_modules.cc_summaryelement import SummaryElement
        from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo  # noqa: E501,F811
        from camcops_server.cc_modules.cc_tsv import TsvPage  # noqa: F811
        from camcops_server.cc_modules.cc_xml import XmlElement

        subclasses = Task.all_subclasses_by_tablename()
        tables = [cls.tablename for cls in subclasses]
        log.info("Actual task table names: {!r} (n={})", tables, len(tables))
        req = self.req
        recipdef = self.recipdef
        for cls in subclasses:
            log.info("Testing {}", cls)
            assert cls.extrastring_taskname != APPSTRING_TASKNAME
            q = self.dbsession.query(cls)
            t = q.first()  # type: Task

            self.assertIsNotNone(t, "Missing task!")

            # Name validity
            validate_task_tablename(t.tablename)

            # Core functions
            self.assertIsInstance(t.is_complete(), bool)
            self.assertIsInstance(t.get_task_html(req), str)
            for trackerinfo in t.get_trackers(req):
                self.assertIsInstance(trackerinfo, TrackerInfo)
            ctvlist = t.get_clinical_text(req)
            if ctvlist is not None:
                for ctvinfo in ctvlist:
                    self.assertIsInstance(ctvinfo, CtvInfo)
            for est in t.get_all_summary_tables(req):
                self.assertIsInstance(est.get_tsv_page(), TsvPage)
                self.assertIsInstance(est.get_xml_element(), XmlElement)

            self.assertIsInstance(t.has_patient, bool)
            self.assertIsInstance(t.is_anonymous, bool)
            self.assertIsInstance(t.has_clinician, bool)
            self.assertIsInstance(t.has_respondent, bool)
            self.assertIsInstance(t.tablename, str)
            for fn in t.get_fieldnames():
                self.assertIsInstance(fn, str)
            self.assertIsInstance(t.field_contents_valid(), bool)
            for msg in t.field_contents_invalid_because():
                self.assertIsInstance(msg, str)
            for fn in t.get_blob_fields():
                self.assertIsInstance(fn, str)

            self.assertIsInstance(t.pk, int)  # all our examples do have PKs  # noqa
            self.assertIsInstance(t.is_preserved(), bool)
            self.assertIsInstance(t.was_forcibly_preserved(), bool)
            self.assertIsInstanceOrNone(t.get_creation_datetime(), Pendulum)
            self.assertIsInstanceOrNone(
                t.get_creation_datetime_utc(), Pendulum)
            self.assertIsInstanceOrNone(
                t.get_seconds_from_creation_to_first_finish(), float)

            self.assertIsInstance(t.get_adding_user_id(), int)
            self.assertIsInstance(t.get_adding_user_username(), str)
            self.assertIsInstance(t.get_removing_user_username(), str)
            self.assertIsInstance(t.get_preserving_user_username(), str)
            self.assertIsInstance(t.get_manually_erasing_user_username(), str)

            # Summaries
            for se in t.standard_task_summary_fields():
                self.assertIsInstance(se, SummaryElement)

            # SNOMED-CT
            if req.snomed_supported:
                for snomed_code in t.get_snomed_codes(req):
                    self.assertIsInstance(snomed_code, SnomedExpression)

            # Clinician
            self.assertIsInstance(t.get_clinician_name(), str)

            # Respondent
            self.assertIsInstance(t.is_respondent_complete(), bool)

            # Patient
            self.assertIsInstanceOrNone(t.patient, Patient)
            self.assertIsInstance(t.is_female(), bool)
            self.assertIsInstance(t.is_male(), bool)
            self.assertIsInstanceOrNone(t.get_patient_server_pk(), int)
            self.assertIsInstance(t.get_patient_forename(), str)
            self.assertIsInstance(t.get_patient_surname(), str)
            dob = t.get_patient_dob()
            assert (
                dob is None or
                isinstance(dob, date) or
                isinstance(dob, Date)
            )
            self.assertIsInstanceOrNone(t.get_patient_dob_first11chars(), str)
            self.assertIsInstance(t.get_patient_sex(), str)
            self.assertIsInstance(t.get_patient_address(), str)
            for idnum in t.get_patient_idnum_objects():
                self.assertIsInstance(idnum.get_idnum_reference(),
                                      IdNumReference)
                self.assertIsInstance(idnum.is_superficially_valid(), bool)
                self.assertIsInstance(idnum.description(req), str)
                self.assertIsInstance(idnum.short_description(req), str)
                self.assertIsInstance(idnum.get_filename_component(req), str)

            # HL7
            pidseg = t.get_patient_hl7_pid_segment(req, recipdef)
            assert isinstance(pidseg, str) or isinstance(pidseg, hl7.Segment)
            for dataseg in t.get_hl7_data_segments(req, recipdef):
                self.assertIsInstance(dataseg, hl7.Segment)
            for dataseg in t.get_hl7_extra_data_segments(recipdef):
                self.assertIsInstance(dataseg, hl7.Segment)

            # Other properties
            self.assertIsInstance(t.is_erased(), bool)
            self.assertIsInstance(t.is_live_on_tablet(), bool)
            for attrname, col in t.gen_text_filter_columns():
                self.assertIsInstance(attrname, str)
                self.assertIsInstance(col, Column)

            # Views
            for page in t.get_tsv_pages(req):
                self.assertIsInstance(page.get_tsv(), str)
            self.assertIsInstance(t.get_xml(req), str)
            self.assertIsInstance(t.get_html(req), str)
            self.assertIsInstance(t.get_pdf(req), bytes)
            self.assertIsInstance(t.get_pdf_html(req), str)
            self.assertIsInstance(t.suggested_pdf_filename(req), str)
            self.assertIsInstance(
                t.get_rio_metadata(req,
                                   which_idnum=1,
                                   uploading_user_id=self.user.id,
                                   document_type="some_doc_type"),
                str
            )

            # Special operations
            t.apply_special_note(req, "Debug: Special note! (1)",
                                 from_console=True)
            t.apply_special_note(req, "Debug: Special note! (2)",
                                 from_console=False)
            self.assertIsInstance(t.special_notes, list)
            t.cancel_from_export_log(req, from_console=True)
            t.cancel_from_export_log(req, from_console=False)

            # Destructive special operations
            self.assertFalse(t.is_erased())
            t.manually_erase(req)
            self.assertTrue(t.is_erased())
            t.delete_entirely(req)
