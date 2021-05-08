#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/webview_tests.py

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

from collections import OrderedDict
import datetime
import json
from typing import cast
import unittest
from unittest import mock

from pendulum import local
from pyramid.httpexceptions import HTTPBadRequest, HTTPFound
from webob.multidict import MultiDict

from camcops_server.cc_modules.cc_constants import ERA_NOW
from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_group import Group
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_pyramid import (
    FormAction,
    ViewArg,
    ViewParam,
)
from camcops_server.cc_modules.cc_taskindex import PatientIdNumIndexEntry
from camcops_server.cc_modules.cc_taskschedule import (
    PatientTaskSchedule,
    TaskSchedule,
    TaskScheduleItem,
)
from camcops_server.cc_modules.cc_testhelpers import class_attribute_names
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase
from camcops_server.cc_modules.cc_user import User
from camcops_server.cc_modules.cc_validators import (
    validate_alphanum_underscore,
)
from camcops_server.cc_modules.webview import (
    AddPatientView,
    AddTaskScheduleItemView,
    AddTaskScheduleView,
    DeleteServerCreatedPatientView,
    DeleteTaskScheduleItemView,
    DeleteTaskScheduleView,
    EditTaskScheduleItemView,
    EditTaskScheduleView,
    EditFinalizedPatientView,
    EditGroupView,
    EditServerCreatedPatientView,
    EraseTaskEntirelyView,
    EraseTaskLeavingPlaceholderView,
    FLASH_INFO,
    FLASH_SUCCESS,
    any_records_use_group,
    edit_group,
    edit_finalized_patient,
    edit_server_created_patient,
)


# =============================================================================
# Unit testing
# =============================================================================

TEST_NHS_NUMBER_1 = 4887211163  # generated at random
TEST_NHS_NUMBER_2 = 1381277373


class WebviewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_any_records_use_group_true(self) -> None:
        # All tasks created in DemoDatabaseTestCase will be in this group
        self.announce("test_any_records_use_group_true")
        self.assertTrue(any_records_use_group(self.req, self.group))

    def test_any_records_use_group_false(self) -> None:
        """
        If this fails with:
        sqlalchemy.exc.InvalidRequestError: SQL expression, column, or mapped
        entity expected - got <name of task base class>
        then the base class probably needs to be declared __abstract__. See
        DiagnosisItemBase as an example.
        """
        self.announce("test_any_records_use_group_false")
        group = Group()
        self.dbsession.add(self.group)
        self.dbsession.commit()

        self.assertFalse(any_records_use_group(self.req, group))

    def test_webview_constant_validators(self) -> None:
        self.announce("test_webview_constant_validators")
        for x in class_attribute_names(ViewArg):
            try:
                validate_alphanum_underscore(x, self.req)
            except ValueError:
                self.fail(f"Operations.{x} fails validate_alphanum_underscore")


class AddTaskScheduleViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_schedule_form_displayed(self) -> None:
        view = AddTaskScheduleView(self.req)

        response = view.dispatch()
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.body.decode("utf-8").count("<form"), 1)

    def test_schedule_is_created(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.NAME, "MOJO"),
            (ViewParam.GROUP_ID, self.group.id),
            (ViewParam.EMAIL_SUBJECT, "Subject"),
            (ViewParam.EMAIL_TEMPLATE, "Email template"),
            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        view = AddTaskScheduleView(self.req)

        with self.assertRaises(HTTPFound) as e:
            view.dispatch()

        schedule = self.dbsession.query(TaskSchedule).one()

        self.assertEqual(schedule.name, "MOJO")
        self.assertEqual(schedule.email_subject, "Subject")
        self.assertEqual(schedule.email_template, "Email template")

        self.assertEqual(e.exception.status_code, 302)
        self.assertIn(
            "view_task_schedules",
            e.exception.headers["Location"]
        )


class EditTaskScheduleViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def setUp(self) -> None:
        super().setUp()

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.schedule.name = "Test"
        self.dbsession.add(self.schedule)
        self.dbsession.commit()

    def test_schedule_name_can_be_updated(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.NAME, "MOJO"),
            (ViewParam.GROUP_ID, self.group.id),
            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)
        self.req.add_get_params({
            ViewParam.SCHEDULE_ID: str(self.schedule.id)
        }, set_method_get=False)

        view = EditTaskScheduleView(self.req)

        with self.assertRaises(HTTPFound) as e:
            view.dispatch()

        schedule = self.dbsession.query(TaskSchedule).one()

        self.assertEqual(schedule.name, "MOJO")

        self.assertEqual(e.exception.status_code, 302)
        self.assertIn(
            "view_task_schedules",
            e.exception.headers["Location"]
        )

    def test_group_a_schedule_cannot_be_edited_by_group_b_admin(self) -> None:
        group_a = Group()
        group_a.name = "Group A"
        self.dbsession.add(group_a)

        group_b = Group()
        group_b.name = "Group B"
        self.dbsession.add(group_b)
        self.dbsession.commit()

        group_a_schedule = TaskSchedule()
        group_a_schedule.group_id = group_a.id
        group_a_schedule.name = "Group A schedule"
        self.dbsession.add(group_a_schedule)
        self.dbsession.commit()

        self.user = User()
        self.user.upload_group_id = group_b.id
        self.user.username = "group b admin"
        self.user.set_password(self.req, "secret123")
        self.dbsession.add(self.user)
        self.dbsession.commit()
        self.req._debugging_user = self.user

        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.NAME, "Something else"),
            (ViewParam.GROUP_ID, self.group.id),
            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)
        self.req.add_get_params({
            ViewParam.SCHEDULE_ID: str(self.schedule.id)
        }, set_method_get=False)

        view = EditTaskScheduleView(self.req)

        with self.assertRaises(HTTPBadRequest) as cm:
            view.dispatch()

        self.assertIn(
            "not a group administrator",
            cm.exception.message
        )


class DeleteTaskScheduleViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def setUp(self) -> None:
        super().setUp()

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.schedule.name = "Test"
        self.dbsession.add(self.schedule)
        self.dbsession.commit()

    def test_schedule_item_is_deleted(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            ("confirm_1_t", "true"),
            ("confirm_2_t", "true"),
            ("confirm_4_t", "true"),
            ("__start__", "danger:mapping"),
            ("target", "7176"),
            ("user_entry", "7176"),
            ("__end__", "danger:mapping"),
            ("delete", "delete"),
            (FormAction.DELETE, "delete"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        self.req.add_get_params({
            ViewParam.SCHEDULE_ID: str(self.schedule.id)
        }, set_method_get=False)
        view = DeleteTaskScheduleView(self.req)

        with self.assertRaises(HTTPFound) as e:
            view.dispatch()

        self.assertEqual(e.exception.status_code, 302)
        self.assertIn(
            "view_task_schedules",
            e.exception.headers["Location"]
        )

        item = self.dbsession.query(TaskScheduleItem).one_or_none()

        self.assertIsNone(item)


class AddTaskScheduleItemViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def setUp(self) -> None:
        super().setUp()

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.schedule.name = "Test"

        self.dbsession.add(self.schedule)
        self.dbsession.commit()

    def test_schedule_item_form_displayed(self) -> None:
        view = AddTaskScheduleItemView(self.req)

        self.req.add_get_params({ViewParam.SCHEDULE_ID: str(self.schedule.id)})

        response = view.dispatch()
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.body.decode("utf-8").count("<form"), 1)

    def test_schedule_item_is_created(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SCHEDULE_ID, self.schedule.id),
            (ViewParam.TABLE_NAME, "ace3"),
            (ViewParam.CLINICIAN_CONFIRMATION, "true"),
            ("__start__", "due_from:mapping"),
            ("months", "1"),
            ("weeks", "2"),
            ("days", "3"),
            ("__end__", "due_from:mapping"),
            ("__start__", "due_within:mapping"),
            ("months", "2"),  # 60 days
            ("weeks", "3"),   # 21 days
            ("days", "15"),   # 15 days
            ("__end__", "due_within:mapping"),
            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        view = AddTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPFound) as e:
            view.dispatch()

        item = self.dbsession.query(TaskScheduleItem).one()

        self.assertEqual(item.schedule_id, self.schedule.id)
        self.assertEqual(item.task_table_name, "ace3")
        self.assertEqual(item.due_from.in_days(), 47)
        self.assertEqual(item.due_by.in_days(), 143)

        self.assertEqual(e.exception.status_code, 302)
        self.assertIn(
            f"view_task_schedule_items?schedule_id={self.schedule.id}",
            e.exception.headers["Location"]
        )

    def test_schedule_item_is_not_created_on_cancel(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SCHEDULE_ID, self.schedule.id),
            (ViewParam.TABLE_NAME, "ace3"),
            ("__start__", "due_from:mapping"),
            ("months", "1"),
            ("weeks", "2"),
            ("days", "3"),
            ("__end__", "due_from:mapping"),
            ("__start__", "due_within:mapping"),
            ("months", "4"),
            ("weeks", "3"),
            ("days", "2"),
            ("__end__", "due_within:mapping"),
            (FormAction.CANCEL, "cancel"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        view = AddTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPFound):
            view.dispatch()

        item = self.dbsession.query(TaskScheduleItem).one_or_none()

        self.assertIsNone(item)

    def test_non_existent_schedule_handled(self) -> None:
        self.req.add_get_params({ViewParam.SCHEDULE_ID: "99999"})

        view = AddTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPBadRequest):
            view.dispatch()


class EditTaskScheduleItemViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def setUp(self) -> None:
        from pendulum import Duration
        super().setUp()

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.schedule.name = "Test"
        self.dbsession.add(self.schedule)
        self.dbsession.commit()

        self.item = TaskScheduleItem()
        self.item.schedule_id = self.schedule.id
        self.item.task_table_name = "ace3"
        self.item.due_from = Duration(days=30)
        self.item.due_by = Duration(days=60)
        self.dbsession.add(self.item)
        self.dbsession.commit()

    def test_schedule_item_is_updated(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SCHEDULE_ID, self.schedule.id),
            (ViewParam.TABLE_NAME, "bmi"),
            ("__start__", "due_from:mapping"),
            ("months", "0"),
            ("weeks", "0"),
            ("days", "30"),
            ("__end__", "due_from:mapping"),
            ("__start__", "due_within:mapping"),
            ("months", "0"),
            ("weeks", "0"),
            ("days", "60"),
            ("__end__", "due_within:mapping"),
            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        self.req.add_get_params({
            ViewParam.SCHEDULE_ITEM_ID: str(self.item.id)
        }, set_method_get=False)
        view = EditTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPFound) as cm:
            view.dispatch()

        self.assertEqual(self.item.task_table_name, "bmi")
        self.assertEqual(cm.exception.status_code, 302)
        self.assertIn(
            f"view_task_schedule_items?schedule_id={self.item.schedule_id}",
            cm.exception.headers["Location"]
        )

    def test_schedule_item_is_not_updated_on_cancel(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SCHEDULE_ID, self.schedule.id),
            (ViewParam.TABLE_NAME, "bmi"),
            ("__start__", "due_from:mapping"),
            ("months", "0"),
            ("weeks", "0"),
            ("days", "30"),
            ("__end__", "due_from:mapping"),
            ("__start__", "due_within:mapping"),
            ("months", "0"),
            ("weeks", "0"),
            ("days", "60"),
            ("__end__", "due_within:mapping"),
            (FormAction.CANCEL, "cancel"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        self.req.add_get_params({
            ViewParam.SCHEDULE_ITEM_ID: str(self.item.id)
        }, set_method_get=False)
        view = EditTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPFound):
            view.dispatch()

        self.assertEqual(self.item.task_table_name, "ace3")

    def test_non_existent_item_handled(self) -> None:
        self.req.add_get_params({ViewParam.SCHEDULE_ITEM_ID: "99999"})

        view = EditTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPBadRequest):
            view.dispatch()

    def test_null_item_handled(self) -> None:
        view = EditTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPBadRequest):
            view.dispatch()

    def test_get_form_values(self) -> None:
        view = EditTaskScheduleItemView(self.req)
        view.object = self.item

        form_values = view.get_form_values()

        self.assertEqual(form_values[ViewParam.SCHEDULE_ID], self.schedule.id)
        self.assertEqual(form_values[ViewParam.TABLE_NAME],
                         self.item.task_table_name)
        self.assertEqual(form_values[ViewParam.DUE_FROM], self.item.due_from)

        due_within = self.item.due_by - self.item.due_from
        self.assertEqual(form_values[ViewParam.DUE_WITHIN], due_within)

    def test_group_a_item_cannot_be_edited_by_group_b_admin(self) -> None:
        from pendulum import Duration

        group_a = Group()
        group_a.name = "Group A"
        self.dbsession.add(group_a)

        group_b = Group()
        group_b.name = "Group B"
        self.dbsession.add(group_b)
        self.dbsession.commit()

        group_a_schedule = TaskSchedule()
        group_a_schedule.group_id = group_a.id
        group_a_schedule.name = "Group A schedule"
        self.dbsession.add(group_a_schedule)
        self.dbsession.commit()

        group_a_item = TaskScheduleItem()
        group_a_item.schedule_id = group_a_schedule.id
        group_a_item.task_table_name = "ace3"
        group_a_item.due_from = Duration(days=30)
        group_a_item.due_by = Duration(days=60)
        self.dbsession.add(group_a_item)
        self.dbsession.commit()

        self.user = User()
        self.user.upload_group_id = group_b.id
        self.user.username = "group b admin"
        self.user.set_password(self.req, "secret123")
        self.dbsession.add(self.user)
        self.dbsession.commit()
        self.req._debugging_user = self.user

        view = EditTaskScheduleItemView(self.req)
        view.object = group_a_item

        with self.assertRaises(HTTPBadRequest) as cm:
            view.get_schedule()

        self.assertIn(
            "not a group administrator",
            cm.exception.message
        )


class DeleteTaskScheduleItemViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def setUp(self) -> None:
        super().setUp()

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.schedule.name = "Test"
        self.dbsession.add(self.schedule)
        self.dbsession.commit()

        self.item = TaskScheduleItem()
        self.item.schedule_id = self.schedule.id
        self.item.task_table_name = "ace3"
        self.dbsession.add(self.item)
        self.dbsession.commit()

    def test_delete_form_displayed(self) -> None:
        view = DeleteTaskScheduleItemView(self.req)

        self.req.add_get_params({ViewParam.SCHEDULE_ITEM_ID: str(self.item.id)})

        response = view.dispatch()
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.body.decode("utf-8").count("<form"), 1)

    def test_errors_displayed_when_deletion_validation_fails(self) -> None:
        self.req.fake_request_post_from_dict({
            FormAction.DELETE: "delete"
        })

        self.req.add_get_params({
            ViewParam.SCHEDULE_ITEM_ID: str(self.item.id)
        }, set_method_get=False)
        view = DeleteTaskScheduleItemView(self.req)

        response = view.dispatch()
        self.assertIn("Errors have been highlighted",
                      response.body.decode("utf-8"))

    def test_schedule_item_is_deleted(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            ("confirm_1_t", "true"),
            ("confirm_2_t", "true"),
            ("confirm_4_t", "true"),
            ("__start__", "danger:mapping"),
            ("target", "7176"),
            ("user_entry", "7176"),
            ("__end__", "danger:mapping"),
            ("delete", "delete"),
            (FormAction.DELETE, "delete"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        self.req.add_get_params({
            ViewParam.SCHEDULE_ITEM_ID: str(self.item.id)
        }, set_method_get=False)
        view = DeleteTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPFound) as e:
            view.dispatch()

        self.assertEqual(e.exception.status_code, 302)
        self.assertIn(
            f"view_task_schedule_items?schedule_id={self.item.schedule_id}",
            e.exception.headers["Location"]
        )

        item = self.dbsession.query(TaskScheduleItem).one_or_none()

        self.assertIsNone(item)

    def test_schedule_item_not_deleted_on_cancel(self) -> None:
        self.req.fake_request_post_from_dict({
            FormAction.CANCEL: "cancel"
        })

        self.req.add_get_params({
            ViewParam.SCHEDULE_ITEM_ID: str(self.item.id)
        }, set_method_get=False)
        view = DeleteTaskScheduleItemView(self.req)

        with self.assertRaises(HTTPFound):
            view.dispatch()

        item = self.dbsession.query(TaskScheduleItem).one_or_none()

        self.assertIsNotNone(item)


class EditFinalizedPatientViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def create_tasks(self) -> None:
        # speed things up a bit
        pass

    def test_raises_when_patient_does_not_exists(self) -> None:
        with self.assertRaises(HTTPBadRequest) as cm:
            edit_finalized_patient(self.req)

        self.assertEqual(str(cm.exception), "Cannot find Patient with _pk:None")

    @unittest.skip("Can't save patient in database without group")
    def test_raises_when_patient_not_in_a_group(self) -> None:
        patient = self.create_patient(_group_id=None)

        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        })

        with self.assertRaises(HTTPBadRequest) as cm:
            edit_finalized_patient(self.req)

        self.assertEqual(str(cm.exception), "Bad patient: not in a group")

    def test_raises_when_not_authorized(self) -> None:
        patient = self.create_patient()

        self.req._debugging_user = User()

        with mock.patch.object(
                self.req._debugging_user,
                "may_administer_group",
                return_value=False
        ):
            self.req.add_get_params({
                ViewParam.SERVER_PK: patient.pk
            })

            with self.assertRaises(HTTPBadRequest) as cm:
                edit_finalized_patient(self.req)

        self.assertEqual(str(cm.exception),
                         "Not authorized to edit this patient")

    def test_raises_when_patient_not_finalized(self) -> None:
        device = Device(name="Not the server device")
        self.req.dbsession.add(device)
        self.req.dbsession.commit()

        patient = self.create_patient(
            id=1, _device_id=device.id, _era=ERA_NOW
        )

        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        })

        with self.assertRaises(HTTPBadRequest) as cm:
            edit_finalized_patient(self.req)

        self.assertIn("Patient is not editable", str(cm.exception))

    def test_patient_updated(self) -> None:
        patient = self.create_patient()

        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        }, set_method_get=False)

        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SERVER_PK, patient.pk),
            (ViewParam.GROUP_ID, patient.group.id),
            (ViewParam.FORENAME, "Jo"),
            (ViewParam.SURNAME, "Patient"),
            ("__start__", "dob:mapping"),
            ("date", "1958-04-19"),
            ("__end__", "dob:mapping"),
            ("__start__", "sex:rename"),
            ("deformField7", "X"),
            ("__end__", "sex:rename"),
            (ViewParam.ADDRESS, "New address"),
            (ViewParam.EMAIL, "newjopatient@example.com"),
            (ViewParam.GP, "New GP"),
            (ViewParam.OTHER, "New other"),
            ("__start__", "id_references:sequence"),
            ("__start__", "idnum_sequence:mapping"),
            (ViewParam.WHICH_IDNUM, self.nhs_iddef.which_idnum),
            (ViewParam.IDNUM_VALUE, str(TEST_NHS_NUMBER_1)),
            ("__end__", "idnum_sequence:mapping"),
            ("__end__", "id_references:sequence"),
            ("__start__", "danger:mapping"),
            ("target", "7836"),
            ("user_entry", "7836"),
            ("__end__", "danger:mapping"),
            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        with self.assertRaises(HTTPFound):
            edit_finalized_patient(self.req)

        self.dbsession.commit()

        self.assertEqual(patient.forename, "Jo")
        self.assertEqual(patient.surname, "Patient")
        self.assertEqual(patient.dob.isoformat(), "1958-04-19")
        self.assertEqual(patient.sex, "X")
        self.assertEqual(patient.address, "New address")
        self.assertEqual(patient.email, "newjopatient@example.com")
        self.assertEqual(patient.gp, "New GP")
        self.assertEqual(patient.other, "New other")

        idnum = patient.get_idnum_objects()[0]
        self.assertEqual(idnum.patient_id, patient.id)
        self.assertEqual(idnum.which_idnum, self.nhs_iddef.which_idnum)
        self.assertEqual(idnum.idnum_value, TEST_NHS_NUMBER_1)

        self.assertEqual(len(patient.special_notes), 1)
        note = patient.special_notes[0].note

        self.assertIn("Patient details edited", note)
        self.assertIn("forename", note)
        self.assertIn("Jo", note)

        self.assertIn("surname", note)
        self.assertIn("Patient", note)

        self.assertIn("idnum1", note)
        self.assertIn(str(TEST_NHS_NUMBER_1), note)

        messages = self.req.session.peek_flash(FLASH_SUCCESS)

        self.assertIn(f"Amended patient record with server PK {patient.pk}",
                      messages[0])
        self.assertIn("forename", messages[0])
        self.assertIn("Jo", messages[0])

        self.assertIn("surname", messages[0])
        self.assertIn("Patient", messages[0])

        self.assertIn("idnum1", messages[0])
        self.assertIn(str(TEST_NHS_NUMBER_1), messages[0])

    def test_message_when_no_changes(self) -> None:
        patient = self.create_patient(
            forename="Jo", surname="Patient", dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", gp="GP", other="Other"
        )
        patient_idnum = self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER_1
        )
        schedule1 = TaskSchedule()
        schedule1.group_id = self.group.id
        schedule1.name = "Test 1"
        self.dbsession.add(schedule1)
        self.dbsession.commit()

        patient_task_schedule = PatientTaskSchedule()
        patient_task_schedule.patient_pk = patient.pk
        patient_task_schedule.schedule_id = schedule1.id
        patient_task_schedule.start_datetime = local(2020, 6, 12, 9)
        patient_task_schedule.settings = {
            "name 1": "value 1",
            "name 2": "value 2",
            "name 3": "value 3",
        }

        self.dbsession.add(patient_task_schedule)
        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        }, set_method_get=False)

        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SERVER_PK, patient.pk),
            (ViewParam.GROUP_ID, patient.group.id),
            (ViewParam.FORENAME, patient.forename),
            (ViewParam.SURNAME, patient.surname),

            ("__start__", "dob:mapping"),
            ("date", patient.dob.isoformat()),
            ("__end__", "dob:mapping"),

            ("__start__", "sex:rename"),
            ("deformField7", patient.sex),
            ("__end__", "sex:rename"),

            (ViewParam.ADDRESS, patient.address),
            (ViewParam.GP, patient.gp),
            (ViewParam.OTHER, patient.other),

            ("__start__", "id_references:sequence"),
            ("__start__", "idnum_sequence:mapping"),
            (ViewParam.WHICH_IDNUM, patient_idnum.which_idnum),
            (ViewParam.IDNUM_VALUE, patient_idnum.idnum_value),
            ("__end__", "idnum_sequence:mapping"),
            ("__end__", "id_references:sequence"),

            ("__start__", "danger:mapping"),
            ("target", "7836"),
            ("user_entry", "7836"),
            ("__end__", "danger:mapping"),

            ("__start__", "task_schedules:sequence"),
            ("__start__", "task_schedule_sequence:mapping"),
            ("schedule_id", schedule1.id),
            ("__start__", "start_datetime:mapping"),
            ("date", "2020-06-12"),
            ("time", "09:00:00"),
            ("__end__", "start_datetime:mapping"),
            ("settings", json.dumps({
                "name 1": "value 1",
                "name 2": "value 2",
                "name 3": "value 3",
            })),
            ("__end__", "task_schedule_sequence:mapping"),
            ("__end__", "task_schedules:sequence"),


            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        with self.assertRaises(HTTPFound):
            edit_finalized_patient(self.req)

        messages = self.req.session.peek_flash(FLASH_INFO)

        self.assertIn("No changes required", messages[0])

    def test_template_rendered_with_values(self) -> None:
        patient = self.create_patient(
            id=1, forename="Jo", surname="Patient",
            dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", gp="GP", other="Other"
        )
        self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER_1
        )

        from camcops_server.tasks import Bmi

        task1 = Bmi()
        task1.id = 1
        task1._device_id = patient.device_id
        task1._group_id = patient.group_id
        task1._era = patient.era
        task1.patient_id = patient.id
        task1.when_created = self.era_time
        task1._current = False
        self.dbsession.add(task1)

        task2 = Bmi()
        task2.id = 2
        task2._device_id = patient.device_id
        task2._group_id = patient.group_id
        task2._era = patient.era
        task2.patient_id = patient.id
        task2.when_created = self.era_time
        task2._current = False
        self.dbsession.add(task2)
        self.dbsession.commit()

        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        })

        view = EditFinalizedPatientView(self.req)
        with mock.patch.object(view, "render_to_response") as mock_render:
            view.dispatch()

        args, kwargs = mock_render.call_args

        context = args[0]

        self.assertIn("form", context)
        self.assertIn(task1, context["tasks"])
        self.assertIn(task2, context["tasks"])

    def test_form_values_for_existing_patient(self) -> None:
        patient = self.create_patient(
            id=1, forename="Jo", surname="Patient",
            dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", email="jopatient@example.com",
            gp="GP", other="Other"
        )

        schedule1 = TaskSchedule()
        schedule1.group_id = self.group.id
        schedule1.name = "Test 1"
        self.dbsession.add(schedule1)
        self.dbsession.commit()

        patient_task_schedule = PatientTaskSchedule()
        patient_task_schedule.patient_pk = patient.pk
        patient_task_schedule.schedule_id = schedule1.id
        patient_task_schedule.start_datetime = local(2020, 6, 12)
        patient_task_schedule.settings = {
            "name 1": "value 1",
            "name 2": "value 2",
            "name 3": "value 3",
        }

        self.dbsession.add(patient_task_schedule)
        self.dbsession.commit()

        self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER_1
        )

        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        })

        view = EditFinalizedPatientView(self.req)
        view.object = patient

        form_values = view.get_form_values()

        self.assertEqual(form_values[ViewParam.FORENAME], "Jo")
        self.assertEqual(form_values[ViewParam.SURNAME], "Patient")
        self.assertEqual(form_values[ViewParam.DOB], datetime.date(1958, 4, 19))
        self.assertEqual(form_values[ViewParam.SEX], "F")
        self.assertEqual(form_values[ViewParam.ADDRESS], "Address")
        self.assertEqual(form_values[ViewParam.EMAIL], "jopatient@example.com")
        self.assertEqual(form_values[ViewParam.GP], "GP")
        self.assertEqual(form_values[ViewParam.OTHER], "Other")

        self.assertEqual(form_values[ViewParam.SERVER_PK], patient.pk)
        self.assertEqual(form_values[ViewParam.GROUP_ID], patient.group.id)

        idnum = form_values[ViewParam.ID_REFERENCES][0]
        self.assertEqual(idnum[ViewParam.WHICH_IDNUM],
                         self.nhs_iddef.which_idnum)
        self.assertEqual(idnum[ViewParam.IDNUM_VALUE], TEST_NHS_NUMBER_1)

        task_schedule = form_values[ViewParam.TASK_SCHEDULES][0]
        self.assertEqual(task_schedule[ViewParam.SCHEDULE_ID],
                         patient_task_schedule.schedule_id)
        self.assertEqual(task_schedule[ViewParam.START_DATETIME],
                         patient_task_schedule.start_datetime)
        self.assertEqual(task_schedule[ViewParam.SETTINGS],
                         patient_task_schedule.settings)

    def test_changes_to_simple_params(self) -> None:
        view = EditFinalizedPatientView(self.req)
        patient = self.create_patient(
            id=1, forename="Jo", surname="Patient",
            dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", email="jopatient@example.com",
            gp="GP", other=None,
        )
        view.object = patient

        changes = OrderedDict()  # type: OrderedDict

        appstruct = {
            ViewParam.FORENAME: "Joanna",
            ViewParam.SURNAME: "Patient-Patient",
            ViewParam.DOB: datetime.date(1958, 4, 19),
            ViewParam.ADDRESS: "New address",
            ViewParam.OTHER: "",
        }

        view._save_simple_params(appstruct, changes)

        self.assertEqual(changes[ViewParam.FORENAME], ("Jo", "Joanna"))
        self.assertEqual(changes[ViewParam.SURNAME],
                         ("Patient", "Patient-Patient"))
        self.assertNotIn(ViewParam.DOB, changes)
        self.assertEqual(changes[ViewParam.ADDRESS], ("Address", "New address"))
        self.assertNotIn(ViewParam.OTHER, changes)

    def test_changes_to_idrefs(self) -> None:
        view = EditFinalizedPatientView(self.req)
        patient = self.create_patient(id=1)
        self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER_1
        )
        self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.study_iddef.which_idnum,
            idnum_value=123
        )

        view.object = patient

        changes = OrderedDict()  # type: OrderedDict

        appstruct = {
            ViewParam.ID_REFERENCES: [
                {
                    ViewParam.WHICH_IDNUM: self.nhs_iddef.which_idnum,
                    ViewParam.IDNUM_VALUE: TEST_NHS_NUMBER_2,
                },
                {
                    ViewParam.WHICH_IDNUM: self.rio_iddef.which_idnum,
                    ViewParam.IDNUM_VALUE: 456,
                }
            ]
        }

        view._save_idrefs(appstruct, changes)

        self.assertEqual(changes["idnum1 (NHS number)"],
                         (TEST_NHS_NUMBER_1, TEST_NHS_NUMBER_2))
        self.assertEqual(changes["idnum3 (Study number)"],
                         (123, None))
        self.assertEqual(changes["idnum2 (RiO number)"],
                         (None, 456))


class EditServerCreatedPatientViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def create_tasks(self) -> None:
        # speed things up a bit
        pass

    def test_group_updated(self) -> None:
        patient = self.create_patient(sex="F", as_server_patient=True)
        new_group = Group()
        new_group.name = "newgroup"
        new_group.description = "New group"
        new_group.upload_policy = "sex AND anyidnum"
        new_group.finalize_policy = "sex AND idnum1"
        self.dbsession.add(new_group)
        self.dbsession.commit()

        view = EditServerCreatedPatientView(self.req)
        view.object = patient

        appstruct = {
            ViewParam.GROUP_ID: new_group.id,
        }

        view.save_object(appstruct)

        self.assertEqual(patient.group_id, new_group.id)

        messages = self.req.session.peek_flash(FLASH_SUCCESS)

        self.assertIn("testgroup", messages[0])
        self.assertIn("newgroup", messages[0])
        self.assertIn("group:", messages[0])

    def test_raises_when_not_created_on_the_server(self) -> None:
        patient = self.create_patient(
            id=1, _device_id=self.other_device.id,
        )

        view = EditServerCreatedPatientView(self.req)

        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        })

        with self.assertRaises(HTTPBadRequest) as cm:
            view.get_object()

        self.assertIn("Patient is not editable", str(cm.exception))

    def test_patient_task_schedules_updated(self) -> None:
        patient = self.create_patient(sex="F", as_server_patient=True)

        schedule1 = TaskSchedule()
        schedule1.group_id = self.group.id
        schedule1.name = "Test 1"
        self.dbsession.add(schedule1)
        schedule2 = TaskSchedule()
        schedule2.group_id = self.group.id
        schedule2.name = "Test 2"
        self.dbsession.add(schedule2)
        schedule3 = TaskSchedule()
        schedule3.group_id = self.group.id
        schedule3.name = "Test 3"
        self.dbsession.add(schedule3)
        self.dbsession.commit()

        patient_task_schedule = PatientTaskSchedule()
        patient_task_schedule.patient_pk = patient.pk
        patient_task_schedule.schedule_id = schedule1.id
        patient_task_schedule.start_datetime = local(2020, 6, 12, 9)
        patient_task_schedule.settings = {
            "name 1": "value 1",
            "name 2": "value 2",
            "name 3": "value 3",
        }

        self.dbsession.add(patient_task_schedule)

        patient_task_schedule = PatientTaskSchedule()
        patient_task_schedule.patient_pk = patient.pk
        patient_task_schedule.schedule_id = schedule3.id

        self.dbsession.add(patient_task_schedule)
        self.dbsession.commit()

        self.req.add_get_params({
            ViewParam.SERVER_PK: patient.pk
        }, set_method_get=False)

        changed_schedule_1_settings = {
            "name 1": "new value 1",
            "name 2": "new value 2",
            "name 3": "new value 3",
        }
        new_schedule_2_settings = {
            "name 4": "value 4",
            "name 5": "value 5",
            "name 6": "value 6",
        }
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SERVER_PK, patient.pk),
            (ViewParam.GROUP_ID, patient.group.id),
            (ViewParam.FORENAME, patient.forename),
            (ViewParam.SURNAME, patient.surname),
            ("__start__", "dob:mapping"),
            ("date", ""),
            ("__end__", "dob:mapping"),
            ("__start__", "sex:rename"),
            ("deformField7", patient.sex),
            ("__end__", "sex:rename"),
            (ViewParam.ADDRESS, patient.address),
            (ViewParam.GP, patient.gp),
            (ViewParam.OTHER, patient.other),
            ("__start__", "id_references:sequence"),
            ("__start__", "idnum_sequence:mapping"),
            (ViewParam.WHICH_IDNUM, self.nhs_iddef.which_idnum),
            (ViewParam.IDNUM_VALUE, str(TEST_NHS_NUMBER_1)),
            ("__end__", "idnum_sequence:mapping"),
            ("__end__", "id_references:sequence"),
            ("__start__", "danger:mapping"),
            ("target", "7836"),
            ("user_entry", "7836"),
            ("__end__", "danger:mapping"),
            ("__start__", "task_schedules:sequence"),
            ("__start__", "task_schedule_sequence:mapping"),
            ("schedule_id", schedule1.id),
            ("__start__", "start_datetime:mapping"),
            ("date", "2020-06-19"),
            ("time", "08:00:00"),
            ("__end__", "start_datetime:mapping"),
            ("settings", json.dumps(changed_schedule_1_settings)),
            ("__end__", "task_schedule_sequence:mapping"),
            ("__start__", "task_schedule_sequence:mapping"),
            ("schedule_id", schedule2.id),
            ("__start__", "start_datetime:mapping"),
            ("date", "2020-07-01"),
            ("time", "13:45:00"),
            ("__end__", "start_datetime:mapping"),
            ("settings", json.dumps(new_schedule_2_settings)),
            ("__end__", "task_schedule_sequence:mapping"),
            ("__end__", "task_schedules:sequence"),

            (FormAction.SUBMIT, "submit"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        with self.assertRaises(HTTPFound):
            edit_server_created_patient(self.req)

        self.dbsession.commit()

        schedules = {pts.task_schedule.name: pts
                     for pts in patient.task_schedules}
        self.assertIn("Test 1", schedules)
        self.assertIn("Test 2", schedules)
        self.assertNotIn("Test 3", schedules)

        self.assertEqual(
            schedules["Test 1"].start_datetime, local(2020, 6, 19, 8)
        )
        self.assertEqual(
            schedules["Test 1"].settings, changed_schedule_1_settings,
        )
        self.assertEqual(
            schedules["Test 2"].start_datetime,
            local(2020, 7, 1, 13, 45)
        )
        self.assertEqual(
            schedules["Test 2"].settings, new_schedule_2_settings,
        )

        messages = self.req.session.peek_flash(FLASH_SUCCESS)

        self.assertIn(f"Amended patient record with server PK {patient.pk}",
                      messages[0])
        self.assertIn("Test 2", messages[0])

    def test_changes_to_task_schedules(self) -> None:
        patient = self.create_patient(sex="F", as_server_patient=True)

        schedule1 = TaskSchedule()
        schedule1.group_id = self.group.id
        schedule1.name = "Test 1"
        self.dbsession.add(schedule1)
        schedule2 = TaskSchedule()
        schedule2.group_id = self.group.id
        schedule2.name = "Test 2"
        self.dbsession.add(schedule2)
        schedule3 = TaskSchedule()
        schedule3.group_id = self.group.id
        schedule3.name = "Test 3"
        self.dbsession.add(schedule3)
        self.dbsession.commit()

        patient_task_schedule = PatientTaskSchedule()
        patient_task_schedule.patient_pk = patient.pk
        patient_task_schedule.schedule_id = schedule1.id
        patient_task_schedule.start_datetime = local(2020, 6, 12, 12, 34)

        schedule_1_settings = {
            "name 1": "value 1",
            "name 2": "value 2",
            "name 3": "value 3",
        }

        patient_task_schedule.settings = schedule_1_settings

        self.dbsession.add(patient_task_schedule)

        patient_task_schedule = PatientTaskSchedule()
        patient_task_schedule.patient_pk = patient.pk
        schedule_3_settings = {
            "name 1": "value 1",
        }
        patient_task_schedule.schedule_id = schedule3.id
        patient_task_schedule.settings = schedule_3_settings
        patient_task_schedule.start_datetime = local(2020, 7, 31, 13, 45)

        self.dbsession.add(patient_task_schedule)
        self.dbsession.commit()

        # The patient starts on schedule 1 and schedule 3
        view = EditServerCreatedPatientView(self.req)
        view.object = patient

        changes = OrderedDict()  # type: OrderedDict

        changed_schedule_1_settings = {
            "name 1": "new value 1",
            "name 2": "new value 2",
            "name 3": "new value 3",
        }
        new_schedule_2_settings = {
            "name 4": "value 4",
            "name 5": "value 5",
            "name 6": "value 6",
        }

        # We update schedule 1, add schedule 2 and (by its absence) delete
        # schedule 3
        appstruct = {
            ViewParam.TASK_SCHEDULES: [
                {
                    ViewParam.SCHEDULE_ID: schedule1.id,
                    ViewParam.START_DATETIME: local(
                        2020, 6, 19, 0, 1
                    ),
                    ViewParam.SETTINGS: changed_schedule_1_settings,
                },
                {
                    ViewParam.SCHEDULE_ID: schedule2.id,
                    ViewParam.START_DATETIME: local(
                        2020, 7, 1, 19, 2),
                    ViewParam.SETTINGS: new_schedule_2_settings,
                }
            ]
        }

        view._save_task_schedules(appstruct, changes)

        expected_old_1 = (local(2020, 6, 12, 12, 34),
                          schedule_1_settings)
        expected_new_1 = (local(2020, 6, 19, 0, 1),
                          changed_schedule_1_settings)

        expected_old_2 = (None, None)
        expected_new_2 = (local(2020, 7, 1, 19, 2),
                          new_schedule_2_settings)

        expected_old_3 = (local(2020, 7, 31, 13, 45),
                          schedule_3_settings)
        expected_new_3 = (None, None)

        self.assertEqual(changes[f"schedule{schedule1.id} (Test 1)"],
                         (expected_old_1, expected_new_1))
        self.assertEqual(changes[f"schedule{schedule2.id} (Test 2)"],
                         (expected_old_2, expected_new_2))
        self.assertEqual(changes[f"schedule{schedule3.id} (Test 3)"],
                         (expected_old_3, expected_new_3))


class AddPatientViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_patient_created(self) -> None:
        view = AddPatientView(self.req)

        schedule1 = TaskSchedule()
        schedule1.group_id = self.group.id
        schedule1.name = "Test 1"
        self.dbsession.add(schedule1)

        schedule2 = TaskSchedule()
        schedule2.group_id = self.group.id
        schedule2.name = "Test 2"
        self.dbsession.add(schedule2)
        self.dbsession.commit()

        start_datetime1 = local(2020, 6, 12)
        start_datetime2 = local(2020, 7, 1)

        settings1 = json.dumps({
            "name 1": "value 1",
            "name 2": "value 2",
            "name 3": "value 3",
        })

        appstruct = {
            ViewParam.GROUP_ID: self.group.id,
            ViewParam.FORENAME: "Jo",
            ViewParam.SURNAME: "Patient",
            ViewParam.DOB: datetime.date(1958, 4, 19),
            ViewParam.SEX: "F",
            ViewParam.ADDRESS: "Address",
            ViewParam.EMAIL: "jopatient@example.com",
            ViewParam.GP: "GP",
            ViewParam.OTHER: "Other",
            ViewParam.ID_REFERENCES: [{
                ViewParam.WHICH_IDNUM: self.nhs_iddef.which_idnum,
                ViewParam.IDNUM_VALUE: 1192220552,
            }],
            ViewParam.TASK_SCHEDULES: [
                {
                    ViewParam.SCHEDULE_ID: schedule1.id,
                    ViewParam.START_DATETIME: start_datetime1,
                    ViewParam.SETTINGS: settings1,
                },
                {
                    ViewParam.SCHEDULE_ID: schedule2.id,
                    ViewParam.START_DATETIME: start_datetime2,
                    ViewParam.SETTINGS: {},
                },
            ],
        }

        view.save_object(appstruct)

        patient = cast(Patient, view.object)

        server_device = Device.get_server_device(
            self.req.dbsession
        )

        self.assertEqual(patient.id, 1)
        self.assertEqual(patient.device_id, server_device.id)
        self.assertEqual(patient.era, ERA_NOW)
        self.assertEqual(patient.group.id, self.group.id)

        self.assertEqual(patient.forename, "Jo")
        self.assertEqual(patient.surname, "Patient")
        self.assertEqual(patient.dob.isoformat(), "1958-04-19")
        self.assertEqual(patient.sex, "F")
        self.assertEqual(patient.address, "Address")
        self.assertEqual(patient.email, "jopatient@example.com")
        self.assertEqual(patient.gp, "GP")
        self.assertEqual(patient.other, "Other")

        idnum = patient.get_idnum_objects()[0]
        self.assertEqual(idnum.patient_id, 1)
        self.assertEqual(idnum.which_idnum, self.nhs_iddef.which_idnum)
        self.assertEqual(idnum.idnum_value, 1192220552)

        patient_task_schedules = {
            pts.task_schedule.name: pts for pts in patient.task_schedules
        }

        self.assertIn("Test 1", patient_task_schedules)
        self.assertIn("Test 2", patient_task_schedules)

        self.assertEqual(
            patient_task_schedules["Test 1"].start_datetime,
            start_datetime1
        )
        self.assertEqual(
            patient_task_schedules["Test 1"].settings,
            settings1
        )
        self.assertEqual(
            patient_task_schedules["Test 2"].start_datetime,
            start_datetime2
        )

    def test_patient_takes_next_available_id(self) -> None:
        self.create_patient(id=1234, as_server_patient=True)

        view = AddPatientView(self.req)

        appstruct = {
            ViewParam.GROUP_ID: self.group.id,
            ViewParam.FORENAME: "Jo",
            ViewParam.SURNAME: "Patient",
            ViewParam.DOB: datetime.date(1958, 4, 19),
            ViewParam.SEX: "F",
            ViewParam.ADDRESS: "Address",
            ViewParam.GP: "GP",
            ViewParam.OTHER: "Other",
            ViewParam.ID_REFERENCES: [{
                ViewParam.WHICH_IDNUM: self.nhs_iddef.which_idnum,
                ViewParam.IDNUM_VALUE: 1192220552,
            }],
            ViewParam.TASK_SCHEDULES: [
            ],
        }

        view.save_object(appstruct)

        patient = cast(Patient, view.object)

        self.assertEqual(patient.id, 1235)

    def test_form_rendered_with_values(self) -> None:
        view = AddPatientView(self.req)

        with mock.patch.object(view, "render_to_response") as mock_render:
            view.dispatch()

        args, kwargs = mock_render.call_args

        context = args[0]

        self.assertIn("form", context)


class DeleteServerCreatedPatientViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def setUp(self) -> None:
        super().setUp()

        self.patient = self.create_patient(
            as_server_patient=True,
            forename="Jo", surname="Patient",
            dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", gp="GP", other="Other"
        )

        patient_pk = self.patient.pk

        idnum = self.create_patient_idnum(
            as_server_patient=True,
            patient_id=self.patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER_1
        )

        PatientIdNumIndexEntry.index_idnum(idnum, self.dbsession)

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.schedule.name = "Test 1"
        self.dbsession.add(self.schedule)
        self.dbsession.commit()

        pts = PatientTaskSchedule()
        pts.patient_pk = patient_pk
        pts.schedule_id = self.schedule.id
        self.dbsession.add(pts)
        self.dbsession.commit()

        self.multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            ("confirm_1_t", "true"),
            ("confirm_2_t", "true"),
            ("confirm_4_t", "true"),
            ("__start__", "danger:mapping"),
            ("target", "7176"),
            ("user_entry", "7176"),
            ("__end__", "danger:mapping"),
            ("delete", "delete"),
            (FormAction.DELETE, "delete"),
        ])

    def create_tasks(self) -> None:
        # speed things up a bit
        pass

    def test_patient_schedule_and_idnums_deleted(self) -> None:
        self.req.fake_request_post_from_dict(self.multidict)

        patient_pk = self.patient.pk
        self.req.add_get_params({
            ViewParam.SERVER_PK: patient_pk
        }, set_method_get=False)
        view = DeleteServerCreatedPatientView(self.req)

        with self.assertRaises(HTTPFound) as e:
            view.dispatch()

        self.assertEqual(e.exception.status_code, 302)
        self.assertIn(
            "view_patient_task_schedules",
            e.exception.headers["Location"]
        )

        deleted_patient = self.dbsession.query(Patient).filter(
            Patient._pk == patient_pk).one_or_none()

        self.assertIsNone(deleted_patient)

        pts = self.dbsession.query(PatientTaskSchedule).filter(
            PatientTaskSchedule.patient_pk == patient_pk).one_or_none()

        self.assertIsNone(pts)

        idnum = self.dbsession.query(PatientIdNum).filter(
            PatientIdNum.patient_id == self.patient.id,
            PatientIdNum._device_id == self.patient.device_id,
            PatientIdNum._era == self.patient.era,
            PatientIdNum._current == True  # noqa: E712
        ).one_or_none()

        self.assertIsNone(idnum)

    def test_registered_patient_deleted(self) -> None:
        from camcops_server.cc_modules.client_api import (
            get_or_create_single_user,
        )
        user1, _ = get_or_create_single_user(self.req, "test", self.patient)
        self.assertEqual(user1.single_patient, self.patient)

        user2, _ = get_or_create_single_user(self.req, "test", self.patient)
        self.assertEqual(user2.single_patient, self.patient)

        self.req.fake_request_post_from_dict(self.multidict)

        patient_pk = self.patient.pk
        self.req.add_get_params({
            ViewParam.SERVER_PK: patient_pk
        }, set_method_get=False)
        view = DeleteServerCreatedPatientView(self.req)

        with self.assertRaises(HTTPFound):
            view.dispatch()

        self.dbsession.commit()

        deleted_patient = self.dbsession.query(Patient).filter(
            Patient._pk == patient_pk).one_or_none()

        self.assertIsNone(deleted_patient)

        # TODO: We get weird behaviour when all the tests are run together
        # (fine for --test_class=DeleteServerCreatedPatientViewTests)
        # the assertion below fails with sqlite in spite of the commit()
        # above.

        # user = self.dbsession.query(User).filter(
        #     User.id == user1.id).one_or_none()
        # self.assertIsNone(user.single_patient_pk)

        # user = self.dbsession.query(User).filter(
        #     User.id == user2.id).one_or_none()
        # self.assertIsNone(user.single_patient_pk)

    def test_unrelated_patient_unaffected(self) -> None:
        other_patient = self.create_patient(
            as_server_patient=True,
            forename="Mo", surname="Patient",
            dob=datetime.date(1968, 11, 30),
            sex="M", address="Address", gp="GP", other="Other"
        )
        patient_pk = other_patient._pk

        saved_patient = self.dbsession.query(Patient).filter(
            Patient._pk == patient_pk).one_or_none()

        self.assertIsNotNone(saved_patient)

        idnum = self.create_patient_idnum(
            as_server_patient=True,
            patient_id=other_patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER_2
        )

        PatientIdNumIndexEntry.index_idnum(idnum, self.dbsession)

        saved_idnum = self.dbsession.query(PatientIdNum).filter(
            PatientIdNum.patient_id == other_patient.id,
            PatientIdNum._device_id == other_patient.device_id,
            PatientIdNum._era == other_patient.era,
            PatientIdNum._current == True  # noqa: E712
        ).one_or_none()

        self.assertIsNotNone(saved_idnum)

        pts = PatientTaskSchedule()
        pts.patient_pk = patient_pk
        pts.schedule_id = self.schedule.id
        self.dbsession.add(pts)
        self.dbsession.commit()

        self.req.fake_request_post_from_dict(self.multidict)

        self.req.add_get_params({
            ViewParam.SERVER_PK: self.patient._pk
        }, set_method_get=False)
        view = DeleteServerCreatedPatientView(self.req)

        with self.assertRaises(HTTPFound):
            view.dispatch()

        saved_patient = self.dbsession.query(Patient).filter(
            Patient._pk == patient_pk).one_or_none()

        self.assertIsNotNone(saved_patient)

        saved_pts = self.dbsession.query(PatientTaskSchedule).filter(
            PatientTaskSchedule.patient_pk == patient_pk).one_or_none()

        self.assertIsNotNone(saved_pts)

        saved_idnum = self.dbsession.query(PatientIdNum).filter(
            PatientIdNum.patient_id == other_patient.id,
            PatientIdNum._device_id == other_patient.device_id,
            PatientIdNum._era == other_patient.era,
            PatientIdNum._current == True  # noqa: E712
        ).one_or_none()

        self.assertIsNotNone(saved_idnum)


class EraseTaskTestCase(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def create_tasks(self) -> None:
        from camcops_server.tasks.bmi import Bmi

        self.task = Bmi()
        self.task.id = 1
        self.apply_standard_task_fields(self.task)
        patient = self.create_patient_with_one_idnum()
        self.task.patient_id = patient.id

        self.dbsession.add(self.task)
        self.dbsession.commit()


class EraseTaskLeavingPlaceholderViewTests(EraseTaskTestCase):
    """
    Unit tests.
    """
    def test_displays_form(self) -> None:
        self.req.add_get_params({
            ViewParam.SERVER_PK: self.task.pk,
            ViewParam.TABLE_NAME: self.task.tablename,
        }, set_method_get=False)
        view = EraseTaskLeavingPlaceholderView(self.req)

        with mock.patch.object(view, "render_to_response") as mock_render:
            view.dispatch()

        args, kwargs = mock_render.call_args
        context = args[0]

        self.assertIn("form", context)

    def test_deletes_task_leaving_placeholder(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SERVER_PK, self.task.pk),
            (ViewParam.TABLE_NAME, self.task.tablename),
            ("confirm_1_t", "true"),
            ("confirm_2_t", "true"),
            ("confirm_4_t", "true"),
            ("__start__", "danger:mapping"),
            ("target", "7176"),
            ("user_entry", "7176"),
            ("__end__", "danger:mapping"),
            ("delete", "delete"),
            (FormAction.DELETE, "delete"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        view = EraseTaskLeavingPlaceholderView(self.req)
        with mock.patch.object(self.task,
                               "manually_erase") as mock_manually_erase:

            with self.assertRaises(HTTPFound):
                view.dispatch()

        mock_manually_erase.assert_called_once()
        args, kwargs = mock_manually_erase.call_args
        request = args[0]

        self.assertEqual(request, self.req)

    def test_task_not_deleted_on_cancel(self) -> None:
        self.req.fake_request_post_from_dict({
            FormAction.CANCEL: "cancel"
        })

        self.req.add_get_params({
            ViewParam.SERVER_PK: self.task.pk,
            ViewParam.TABLE_NAME: self.task.tablename,
        }, set_method_get=False)
        view = EraseTaskLeavingPlaceholderView(self.req)

        with self.assertRaises(HTTPFound):
            view.dispatch()

        task = self.dbsession.query(self.task.__class__).one_or_none()

        self.assertIsNotNone(task)

    def test_redirect_on_cancel(self) -> None:
        self.req.fake_request_post_from_dict({
            FormAction.CANCEL: "cancel"
        })

        self.req.add_get_params({
            ViewParam.SERVER_PK: self.task.pk,
            ViewParam.TABLE_NAME: self.task.tablename,
        }, set_method_get=False)
        view = EraseTaskLeavingPlaceholderView(self.req)

        with self.assertRaises(HTTPFound) as cm:
            view.dispatch()

        self.assertEqual(cm.exception.status_code, 302)
        self.assertIn(
            "/task", cm.exception.headers["Location"]
        )
        self.assertIn(
            "table_name={}".format(self.task.tablename),
            cm.exception.headers["Location"]
        )
        self.assertIn(
            "server_pk={}".format(self.task.pk),
            cm.exception.headers["Location"]
        )
        self.assertIn("viewtype=html", cm.exception.headers["Location"])

    def test_raises_when_task_does_not_exist(self) -> None:
        self.req.add_get_params({
            ViewParam.SERVER_PK: "123",
            ViewParam.TABLE_NAME: "phq9",
        }, set_method_get=False)
        view = EraseTaskLeavingPlaceholderView(self.req)

        with self.assertRaises(HTTPBadRequest) as cm:
            view.dispatch()

        self.assertEqual(
            cm.exception.message,
            "No such task: phq9, PK=123"
        )

    def test_raises_when_task_is_live_on_tablet(self) -> None:
        self.task._era = ERA_NOW
        self.dbsession.add(self.task)
        self.dbsession.commit()

        self.req.add_get_params({
            ViewParam.SERVER_PK: self.task.pk,
            ViewParam.TABLE_NAME: self.task.tablename,
        }, set_method_get=False)
        view = EraseTaskLeavingPlaceholderView(self.req)

        with self.assertRaises(HTTPBadRequest) as cm:
            view.dispatch()

        self.assertIn(
            "Task is live on tablet",
            cm.exception.message
        )

    def test_raises_when_user_not_authorized_to_erase(self) -> None:
        with mock.patch.object(self.user, "authorized_to_erase_tasks",
                               return_value=False):

            self.req.add_get_params({
                ViewParam.SERVER_PK: self.task.pk,
                ViewParam.TABLE_NAME: self.task.tablename,
            }, set_method_get=False)
            view = EraseTaskLeavingPlaceholderView(self.req)

            with self.assertRaises(HTTPBadRequest) as cm:
                view.dispatch()

        self.assertIn(
            "Not authorized to erase tasks",
            cm.exception.message
        )

    def test_raises_when_task_already_erased(self) -> None:
        self.task._manually_erased = True
        self.dbsession.add(self.task)
        self.dbsession.commit()

        self.req.add_get_params({
            ViewParam.SERVER_PK: self.task.pk,
            ViewParam.TABLE_NAME: self.task.tablename,
        }, set_method_get=False)
        view = EraseTaskLeavingPlaceholderView(self.req)

        with self.assertRaises(HTTPBadRequest) as cm:
            view.dispatch()

        self.assertIn(
            "already erased",
            cm.exception.message
        )


class EraseTaskEntirelyViewTests(EraseTaskTestCase):
    """
    Unit tests.
    """
    def test_deletes_task_entirely(self) -> None:
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.SERVER_PK, self.task.pk),
            (ViewParam.TABLE_NAME, self.task.tablename),
            ("confirm_1_t", "true"),
            ("confirm_2_t", "true"),
            ("confirm_4_t", "true"),
            ("__start__", "danger:mapping"),
            ("target", "7176"),
            ("user_entry", "7176"),
            ("__end__", "danger:mapping"),
            ("delete", "delete"),
            (FormAction.DELETE, "delete"),
        ])

        self.req.fake_request_post_from_dict(multidict)

        view = EraseTaskEntirelyView(self.req)

        with mock.patch.object(self.task,
                               "delete_entirely") as mock_delete_entirely:

            with self.assertRaises(HTTPFound):
                view.dispatch()

        mock_delete_entirely.assert_called_once()
        args, kwargs = mock_delete_entirely.call_args
        request = args[0]

        self.assertEqual(request, self.req)

        messages = self.req.session.peek_flash(FLASH_SUCCESS)
        self.assertTrue(len(messages) > 0)

        self.assertIn("Task erased", messages[0])
        self.assertIn(self.task.tablename, messages[0])
        self.assertIn("server PK {}".format(self.task.pk), messages[0])


class EditGroupViewTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_group_updated(self) -> None:
        other_group_1 = Group()
        other_group_1.name = "other-group-1"
        self.dbsession.add(other_group_1)

        other_group_2 = Group()
        other_group_2.name = "other-group-2"
        self.dbsession.add(other_group_2)

        self.dbsession.commit()

        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.GROUP_ID, self.group.id),
            (ViewParam.NAME, "new-name"),
            (ViewParam.DESCRIPTION, "new description"),
            (ViewParam.UPLOAD_POLICY, "anyidnum AND sex"),  # reversed
            (ViewParam.FINALIZE_POLICY, "idnum1 AND sex"),  # reversed
            ("__start__", "group_ids:sequence"),
            ("group_id_sequence", str(other_group_1.id)),
            ("group_id_sequence", str(other_group_2.id)),
            ("__end__", "group_ids:sequence"),
            (FormAction.SUBMIT, "submit"),
        ])
        self.req.fake_request_post_from_dict(multidict)

        with self.assertRaises(HTTPFound):
            edit_group(self.req)

        self.assertEqual(self.group.name, "new-name")
        self.assertEqual(self.group.description, "new description")
        self.assertEqual(self.group.upload_policy, "anyidnum AND sex")
        self.assertEqual(self.group.finalize_policy, "idnum1 AND sex")
        self.assertIn(other_group_1, self.group.can_see_other_groups)
        self.assertIn(other_group_2, self.group.can_see_other_groups)

    def test_ip_use_added(self) -> None:
        from camcops_server.cc_modules.cc_ipuse import IpContexts
        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.GROUP_ID, self.group.id),
            (ViewParam.NAME, "new-name"),
            (ViewParam.DESCRIPTION, "new description"),
            (ViewParam.UPLOAD_POLICY, "anyidnum AND sex"),
            (ViewParam.FINALIZE_POLICY, "idnum1 AND sex"),
            ("__start__", "ip_use:mapping"),
            (IpContexts.CLINICAL, "true"),
            (IpContexts.COMMERCIAL, "true"),
            ("__end__", "ip_use:mapping"),
            (FormAction.SUBMIT, "submit"),
        ])
        self.req.fake_request_post_from_dict(multidict)

        with self.assertRaises(HTTPFound):
            edit_group(self.req)

        self.assertTrue(self.group.ip_use.clinical)
        self.assertTrue(self.group.ip_use.commercial)
        self.assertFalse(self.group.ip_use.educational)
        self.assertFalse(self.group.ip_use.research)

    def test_ip_use_updated(self) -> None:
        from camcops_server.cc_modules.cc_ipuse import IpContexts
        self.group.ip_use.educational = True
        self.group.ip_use.research = True
        self.dbsession.add(self.group.ip_use)
        self.dbsession.commit()

        old_id = self.group.ip_use.id

        multidict = MultiDict([
            ("_charset_", "UTF-8"),
            ("__formid__", "deform"),
            (ViewParam.CSRF_TOKEN, self.req.session.get_csrf_token()),
            (ViewParam.GROUP_ID, self.group.id),
            (ViewParam.NAME, "new-name"),
            (ViewParam.DESCRIPTION, "new description"),
            (ViewParam.UPLOAD_POLICY, "anyidnum AND sex"),
            (ViewParam.FINALIZE_POLICY, "idnum1 AND sex"),
            ("__start__", "ip_use:mapping"),
            (IpContexts.CLINICAL, "true"),
            (IpContexts.COMMERCIAL, "true"),
            ("__end__", "ip_use:mapping"),
            (FormAction.SUBMIT, "submit"),
        ])
        self.req.fake_request_post_from_dict(multidict)

        with self.assertRaises(HTTPFound):
            edit_group(self.req)

        self.assertTrue(self.group.ip_use.clinical)
        self.assertTrue(self.group.ip_use.commercial)
        self.assertFalse(self.group.ip_use.educational)
        self.assertFalse(self.group.ip_use.research)
        self.assertEqual(self.group.ip_use.id, old_id)

    def test_other_groups_displayed_in_form(self) -> None:
        z_group = Group()
        z_group.name = "z-group"
        self.dbsession.add(z_group)

        a_group = Group()
        a_group.name = "a-group"
        self.dbsession.add(a_group)
        self.dbsession.commit()

        other_groups = Group.get_groups_from_id_list(
            self.dbsession, [z_group.id, a_group.id]
        )
        self.group.can_see_other_groups = other_groups

        self.dbsession.add(self.group)
        self.dbsession.commit()

        view = EditGroupView(self.req)
        view.object = self.group

        form_values = view.get_form_values()

        self.assertEqual(
            form_values[ViewParam.GROUP_IDS], [a_group.id, z_group.id]
        )

    def test_group_id_displayed_in_form(self) -> None:
        view = EditGroupView(self.req)
        view.object = self.group

        form_values = view.get_form_values()

        self.assertEqual(
            form_values[ViewParam.GROUP_ID], self.group.id
        )

    def test_ip_use_displayed_in_form(self) -> None:
        view = EditGroupView(self.req)
        view.object = self.group

        form_values = view.get_form_values()

        self.assertEqual(
            form_values[ViewParam.IP_USE], self.group.ip_use
        )
