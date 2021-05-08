#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_taskschedule_tests.py

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

from urllib.parse import urlencode

from pendulum import Duration

from camcops_server.cc_modules.cc_pyramid import Routes
from camcops_server.cc_modules.cc_taskschedule import (
    PatientTaskSchedule,
    TaskSchedule,
    TaskScheduleItem,
)
from camcops_server.cc_modules.cc_unittest import (
    DemoDatabaseTestCase,
    DemoRequestTestCase,
)


# =============================================================================
# Unit tests
# =============================================================================

class TaskScheduleItemTests(DemoRequestTestCase):
    def test_description_shows_shortname_and_number_of_days(self) -> None:
        item = TaskScheduleItem()
        item.task_table_name = "bmi"
        item.due_from = Duration(days=30)

        self.assertEqual(item.description(self.req), "BMI @ 30 days")

    def test_description_with_no_durations(self) -> None:
        item = TaskScheduleItem()
        item.task_table_name = "bmi"

        self.assertEqual(item.description(self.req), "BMI @ ? days")

    def test_due_within_calculated_from_due_by_and_due_from(self) -> None:
        item = TaskScheduleItem()
        item.due_from = Duration(days=30)
        item.due_by = Duration(days=50)

        self.assertEqual(item.due_within.in_days(), 20)

    def test_due_within_is_none_when_missing_due_by(self) -> None:
        item = TaskScheduleItem()
        item.due_from = Duration(days=30)

        self.assertIsNone(item.due_within)

    def test_due_within_calculated_when_missing_due_from(self) -> None:
        item = TaskScheduleItem()
        item.due_by = Duration(days=30)

        self.assertEqual(item.due_within.in_days(), 30)


class PatientTaskScheduleTests(DemoDatabaseTestCase):
    def setUp(self) -> None:
        super().setUp()

        import datetime

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.dbsession.add(self.schedule)

        self.patient = self.create_patient(
            id=1, forename="Jo", surname="Patient",
            dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", gp="GP", other="Other"
        )

        self.pts = PatientTaskSchedule()
        self.pts.schedule_id = self.schedule.id
        self.pts.patient_pk = self.patient.pk
        self.dbsession.add(self.pts)
        self.dbsession.flush()

    def test_mailto_url_contains_patient_email(self) -> None:
        self.assertIn(f"mailto:{self.patient.email}",
                      self.pts.mailto_url(self.req))

    def test_mailto_url_contains_subject(self) -> None:
        self.schedule.email_subject = "CamCOPS access key"
        self.dbsession.add(self.schedule)
        self.dbsession.flush()

        self.assertIn("subject=CamCOPS%20access%20key",
                      self.pts.mailto_url(self.req))

    def test_mailto_url_contains_access_key(self) -> None:
        self.schedule.email_template = "{access_key}"
        self.dbsession.add(self.schedule)
        self.dbsession.flush()

        self.assertIn(f"body={self.patient.uuid_as_proquint}",
                      self.pts.mailto_url(self.req))

    def test_mailto_url_contains_server_url(self) -> None:
        self.schedule.email_template = "{server_url}"
        self.dbsession.add(self.schedule)
        self.dbsession.flush()

        expected_url = urlencode({"body":
                                  self.req.route_url(Routes.CLIENT_API)})

        self.assertIn(f"{expected_url}", self.pts.mailto_url(self.req))

    def test_mailto_url_disallows_invalid_template(self) -> None:
        self.schedule.email_template = "{foobar}"
        self.dbsession.add(self.schedule)
        self.dbsession.flush()

        with self.assertRaises(KeyError):
            self.pts.mailto_url(self.req)

    def test_mailto_url_disallows_accessing_properties(self) -> None:
        self.schedule.email_template = "{server_url.__class__}"
        self.dbsession.add(self.schedule)
        self.dbsession.flush()

        with self.assertRaises(KeyError):
            self.pts.mailto_url(self.req)
