#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/client_api_tests.py

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

import json
# from pprint import pformat
import string

from cardinal_pythonlib.convert import (
    base64_64format_encode,
    hex_xformat_encode,
)
from cardinal_pythonlib.sql.literals import sql_quote_string
from cardinal_pythonlib.text import escape_newlines, unescape_newlines

from camcops_server.cc_modules.cc_client_api_core import (
    fail_server_error,
    fail_unsupported_operation,
    fail_user_error,
    ServerErrorException,
    TabletParam,
    UserErrorException,
)
from camcops_server.cc_modules.cc_convert import (
    decode_values,
)
from camcops_server.cc_modules.cc_ipuse import IpUse
from camcops_server.cc_modules.cc_proquint import (
    uuid_from_proquint,
)
from camcops_server.cc_modules.cc_taskindex import update_indexes_and_push_exports  # noqa
from camcops_server.cc_modules.cc_testhelpers import class_attribute_names
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase
from camcops_server.cc_modules.cc_user import User
from camcops_server.cc_modules.cc_version import (
    MINIMUM_TABLET_VERSION,
)
from camcops_server.cc_modules.cc_validators import (
    validate_alphanum_underscore,
)
from camcops_server.cc_modules.client_api import (
    client_api,
    DEVICE_STORED_VAR_TABLENAME_DEFUNCT,
    FAILURE_CODE,
    get_reply_dict_from_response,
    make_single_user_mode_username,
    Operations,
    SUCCESS_CODE,
    TEST_NHS_NUMBER,
)


class ClientApiTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_client_api_basics(self) -> None:
        self.announce("test_client_api_basics")

        with self.assertRaises(UserErrorException):
            fail_user_error("testmsg")
        with self.assertRaises(ServerErrorException):
            fail_server_error("testmsg")
        with self.assertRaises(UserErrorException):
            fail_unsupported_operation("duffop")

        # Encoding/decoding tests
        # data = bytearray("hello")
        data = b"hello"
        enc_b64data = base64_64format_encode(data)
        enc_hexdata = hex_xformat_encode(data)
        not_enc_1 = "X'012345'"
        not_enc_2 = "64'aGVsbG8='"
        teststring = """one, two, 3, 4.5, NULL, 'hello "hi
            with linebreak"', 'NULL', 'quote''s here', {b}, {h}, {s1}, {s2}"""
        sql_csv_testdict = {
            teststring.format(
                b=enc_b64data,
                h=enc_hexdata,
                s1=sql_quote_string(not_enc_1),
                s2=sql_quote_string(not_enc_2),
            ): [
                "one",
                "two",
                3,
                4.5,
                None,
                'hello "hi\n            with linebreak"',
                "NULL",
                "quote's here",
                data,
                data,
                not_enc_1,
                not_enc_2,
            ],
            "": [],
        }
        for k, v in sql_csv_testdict.items():
            r = decode_values(k)
            self.assertEqual(r, v, "Mismatch! Result: {r!s}\n"
                                   "Should have been: {v!s}\n"
                                   "Key was: {k!s}".format(r=r, v=v, k=k))

        # Newline encoding/decodine
        ts2 = "slash \\ newline \n ctrl_r \r special \\n other special \\r " \
              "quote ' doublequote \" "
        self.assertEqual(unescape_newlines(escape_newlines(ts2)), ts2,
                         "Bug in escape_newlines() or unescape_newlines()")

        # TODO: client_api.ClientApiTests: more tests here... ?

    def test_client_api_antique_support_1(self) -> None:
        self.announce("test_client_api_antique_support_1")
        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.WHICH_KEYS_TO_SEND,
            TabletParam.TABLE: DEVICE_STORED_VAR_TABLENAME_DEFUNCT,
        })
        response = client_api(self.req)
        d = get_reply_dict_from_response(response)
        self.assertEqual(d[TabletParam.SUCCESS], SUCCESS_CODE)

    def test_client_api_antique_support_2(self) -> None:
        self.announce("test_client_api_antique_support_2")
        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.WHICH_KEYS_TO_SEND,
            TabletParam.TABLE: "nonexistent_table",
        })
        response = client_api(self.req)
        d = get_reply_dict_from_response(response)
        self.assertEqual(d[TabletParam.SUCCESS], FAILURE_CODE)

    def test_client_api_antique_support_3(self) -> None:
        self.announce("test_client_api_antique_support_3")
        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.UPLOAD_TABLE,
            TabletParam.TABLE: DEVICE_STORED_VAR_TABLENAME_DEFUNCT,
        })
        response = client_api(self.req)
        d = get_reply_dict_from_response(response)
        self.assertEqual(d[TabletParam.SUCCESS], SUCCESS_CODE)

    def test_client_api_validators(self) -> None:
        self.announce("test_client_api_validators")
        for x in class_attribute_names(Operations):
            try:
                validate_alphanum_underscore(x, self.req)
            except ValueError:
                self.fail(f"Operations.{x} fails validate_alphanum_underscore")


class PatientRegistrationTests(DemoDatabaseTestCase):
    def create_tasks(self) -> None:
        # Speed things up a bit
        pass

    def test_returns_patient_info(self) -> None:
        import datetime
        patient = self.create_patient(
            forename="JO", surname="PATIENT", dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", gp="GP", other="Other",
            as_server_patient=True
        )

        self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER,
            as_server_patient=True
        )

        proquint = patient.uuid_as_proquint

        # For type checker
        assert proquint is not None
        assert self.other_device.name is not None

        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: proquint,
        })
        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)

        self.assertEqual(reply_dict[TabletParam.SUCCESS], SUCCESS_CODE,
                         msg=reply_dict)

        patient_dict = json.loads(reply_dict[TabletParam.PATIENT_INFO])[0]

        self.assertEqual(patient_dict[TabletParam.SURNAME], "PATIENT")
        self.assertEqual(patient_dict[TabletParam.FORENAME], "JO")
        self.assertEqual(patient_dict[TabletParam.SEX], "F")
        self.assertEqual(patient_dict[TabletParam.DOB], "1958-04-19")
        self.assertEqual(patient_dict[TabletParam.ADDRESS], "Address")
        self.assertEqual(patient_dict[TabletParam.GP], "GP")
        self.assertEqual(patient_dict[TabletParam.OTHER], "Other")
        self.assertEqual(patient_dict[f"idnum{self.nhs_iddef.which_idnum}"],
                         TEST_NHS_NUMBER)

    def test_creates_user(self) -> None:
        from camcops_server.cc_modules.cc_taskindex import (
            PatientIdNumIndexEntry,
        )
        patient = self.create_patient(_group_id=self.group.id,
                                      as_server_patient=True)
        idnum = self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER,
            as_server_patient=True
        )
        PatientIdNumIndexEntry.index_idnum(idnum, self.dbsession)

        proquint = patient.uuid_as_proquint

        # For type checker
        assert proquint is not None
        assert self.other_device.name is not None

        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: proquint,
        })
        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)

        self.assertEqual(reply_dict[TabletParam.SUCCESS], SUCCESS_CODE,
                         msg=reply_dict)

        username = reply_dict[TabletParam.USER]
        self.assertEqual(
            username,
            make_single_user_mode_username(self.other_device.name, patient._pk)
        )
        password = reply_dict[TabletParam.PASSWORD]
        self.assertEqual(len(password), 32)

        valid_chars = string.ascii_letters + string.digits + string.punctuation
        self.assertTrue(all(c in valid_chars for c in password))

        user = self.req.dbsession.query(User).filter(
            User.username == username).one_or_none()
        self.assertIsNotNone(user)
        self.assertEqual(user.upload_group, patient.group)
        self.assertTrue(user.auto_generated)
        self.assertTrue(user.may_register_devices)
        self.assertTrue(user.may_upload)

    def test_does_not_create_user_when_name_exists(self) -> None:
        from camcops_server.cc_modules.cc_taskindex import (
            PatientIdNumIndexEntry,
        )
        patient = self.create_patient(_group_id=self.group.id,
                                      as_server_patient=True)
        idnum = self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER,
            as_server_patient=True
        )
        PatientIdNumIndexEntry.index_idnum(idnum, self.dbsession)

        proquint = patient.uuid_as_proquint

        user = User(
            username=make_single_user_mode_username(
                self.other_device.name, patient._pk
            )
        )
        user.set_password(self.req, "old password")
        self.dbsession.add(user)
        self.dbsession.commit()

        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: proquint,
        })
        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)

        self.assertEqual(reply_dict[TabletParam.SUCCESS], SUCCESS_CODE,
                         msg=reply_dict)

        username = reply_dict[TabletParam.USER]
        self.assertEqual(
            username,
            make_single_user_mode_username(self.other_device.name, patient._pk)
        )
        password = reply_dict[TabletParam.PASSWORD]
        self.assertEqual(len(password), 32)

        valid_chars = string.ascii_letters + string.digits + string.punctuation
        self.assertTrue(all(c in valid_chars for c in password))

        user = self.req.dbsession.query(User).filter(
            User.username == username).one_or_none()
        self.assertIsNotNone(user)
        self.assertEqual(user.upload_group, patient.group)
        self.assertTrue(user.auto_generated)
        self.assertTrue(user.may_register_devices)
        self.assertTrue(user.may_upload)

    def test_raises_for_invalid_proquint(self) -> None:
        # For type checker
        assert self.other_device.name is not None

        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: "invalid",
        })
        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)

        self.assertEqual(reply_dict[TabletParam.SUCCESS], FAILURE_CODE,
                         msg=reply_dict)
        self.assertIn("no patient with access key 'invalid'",
                      reply_dict[TabletParam.ERROR])

    def test_raises_for_missing_valid_proquint(self) -> None:
        valid_proquint = "sazom-diliv-navol-hubot-mufur-mamuv-kojus-loluv-v"

        # Error message is same as for invalid proquint so make sure our
        # test proquint really is valid (should not raise)
        uuid_from_proquint(valid_proquint)

        assert self.other_device.name is not None

        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: valid_proquint,
        })
        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)

        self.assertEqual(reply_dict[TabletParam.SUCCESS], FAILURE_CODE,
                         msg=reply_dict)
        self.assertIn(f"no patient with access key '{valid_proquint}'",
                      reply_dict[TabletParam.ERROR])

    def test_raises_when_no_patient_idnums(self) -> None:
        # In theory this shouldn't be possible in normal operation as the
        # patient cannot be created without any idnums
        patient = self.create_patient(as_server_patient=True)

        proquint = patient.uuid_as_proquint
        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: proquint,
        })

        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)
        self.assertEqual(reply_dict[TabletParam.SUCCESS], FAILURE_CODE,
                         msg=reply_dict)
        self.assertIn("Patient has no ID numbers",
                      reply_dict[TabletParam.ERROR])

    def test_raises_when_patient_not_created_on_server(self) -> None:
        patient = self.create_patient(_device_id=self.other_device.id,
                                      as_server_patient=True)

        proquint = patient.uuid_as_proquint
        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: proquint,
        })

        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)
        self.assertEqual(reply_dict[TabletParam.SUCCESS], FAILURE_CODE,
                         msg=reply_dict)
        self.assertIn(f"no patient with access key '{proquint}'",
                      reply_dict[TabletParam.ERROR])

    def test_returns_ip_use_flags(self) -> None:
        import datetime
        from camcops_server.cc_modules.cc_taskindex import (
            PatientIdNumIndexEntry,
        )

        patient = self.create_patient(
            forename="JO", surname="PATIENT", dob=datetime.date(1958, 4, 19),
            sex="F", address="Address", gp="GP", other="Other",
            as_server_patient=True
        )
        idnum = self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER,
            as_server_patient=True
        )
        PatientIdNumIndexEntry.index_idnum(idnum, self.dbsession)

        patient.group.ip_use = IpUse()

        patient.group.ip_use.commercial = True
        patient.group.ip_use.clinical = True
        patient.group.ip_use.educational = False
        patient.group.ip_use.research = False

        self.dbsession.add(patient.group)
        self.dbsession.commit()

        proquint = patient.uuid_as_proquint

        # For type checker
        assert proquint is not None
        assert self.other_device.name is not None

        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.REGISTER_PATIENT,
            TabletParam.PATIENT_PROQUINT: proquint,
        })
        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)

        self.assertEqual(reply_dict[TabletParam.SUCCESS], SUCCESS_CODE,
                         msg=reply_dict)

        ip_use_info = json.loads(reply_dict[TabletParam.IP_USE_INFO])

        self.assertEqual(ip_use_info[TabletParam.IP_USE_COMMERCIAL], 1)
        self.assertEqual(ip_use_info[TabletParam.IP_USE_CLINICAL], 1)
        self.assertEqual(ip_use_info[TabletParam.IP_USE_EDUCATIONAL], 0)
        self.assertEqual(ip_use_info[TabletParam.IP_USE_RESEARCH], 0)


class GetTaskSchedulesTests(DemoDatabaseTestCase):
    def create_tasks(self) -> None:
        # Speed things up a bit
        pass

    def test_returns_task_schedules(self) -> None:
        from pendulum import DateTime as Pendulum, Duration, local, parse

        from camcops_server.cc_modules.cc_taskindex import (
            PatientIdNumIndexEntry,
            TaskIndexEntry,
        )
        from camcops_server.cc_modules.cc_taskschedule import (
            PatientTaskSchedule,
            TaskSchedule,
            TaskScheduleItem
        )
        from camcops_server.tasks.bmi import Bmi

        schedule1 = TaskSchedule()
        schedule1.group_id = self.group.id
        schedule1.name = "Test 1"
        self.dbsession.add(schedule1)

        schedule2 = TaskSchedule()
        schedule2.group_id = self.group.id
        self.dbsession.add(schedule2)
        self.dbsession.commit()

        item1 = TaskScheduleItem()
        item1.schedule_id = schedule1.id
        item1.task_table_name = "phq9"
        item1.due_from = Duration(days=0)
        item1.due_by = Duration(days=7)
        self.dbsession.add(item1)

        item2 = TaskScheduleItem()
        item2.schedule_id = schedule1.id
        item2.task_table_name = "bmi"
        item2.due_from = Duration(days=0)
        item2.due_by = Duration(days=8)
        self.dbsession.add(item2)

        item3 = TaskScheduleItem()
        item3.schedule_id = schedule1.id
        item3.task_table_name = "phq9"
        item3.due_from = Duration(days=30)
        item3.due_by = Duration(days=37)
        self.dbsession.add(item3)

        item4 = TaskScheduleItem()
        item4.schedule_id = schedule1.id
        item4.task_table_name = "gmcpq"
        item4.due_from = Duration(days=30)
        item4.due_by = Duration(days=38)
        self.dbsession.add(item4)
        self.dbsession.commit()

        patient = self.create_patient()
        idnum = self.create_patient_idnum(
            patient_id=patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER
        )
        PatientIdNumIndexEntry.index_idnum(idnum, self.dbsession)

        server_patient = self.create_patient(as_server_patient=True)
        _ = self.create_patient_idnum(
            patient_id=server_patient.id,
            which_idnum=self.nhs_iddef.which_idnum,
            idnum_value=TEST_NHS_NUMBER,
            as_server_patient=True
        )

        schedule_1 = PatientTaskSchedule()
        schedule_1.patient_pk = server_patient.pk
        schedule_1.schedule_id = schedule1.id
        schedule_1.settings = {
            "bmi": {
                "bmi_key": "bmi_value",
            },
            "phq9": {
                "phq9_key": "phq9_value",
            }
        }
        schedule_1.start_datetime = local(2020, 7, 31)
        self.dbsession.add(schedule_1)

        schedule_2 = PatientTaskSchedule()
        schedule_2.patient_pk = server_patient.pk
        schedule_2.schedule_id = schedule2.id
        self.dbsession.add(schedule_2)

        bmi = Bmi()
        self.apply_standard_task_fields(bmi)
        bmi.id = 1
        bmi.height_m = 1.83
        bmi.mass_kg = 67.57
        bmi.patient_id = patient.id
        bmi.when_created = local(2020, 8, 1)
        self.dbsession.add(bmi)
        self.dbsession.commit()
        self.assertTrue(bmi.is_complete())

        TaskIndexEntry.index_task(
            bmi,
            self.dbsession,
            indexed_at_utc=Pendulum.utcnow()
        )
        self.dbsession.commit()

        proquint = server_patient.uuid_as_proquint

        # For type checker
        assert proquint is not None
        assert self.other_device.name is not None

        self.req.fake_request_post_from_dict({
            TabletParam.CAMCOPS_VERSION: MINIMUM_TABLET_VERSION,
            TabletParam.DEVICE: self.other_device.name,
            TabletParam.OPERATION: Operations.GET_TASK_SCHEDULES,
            TabletParam.PATIENT_PROQUINT: proquint,
        })
        response = client_api(self.req)
        reply_dict = get_reply_dict_from_response(response)

        self.assertEqual(reply_dict[TabletParam.SUCCESS], SUCCESS_CODE,
                         msg=reply_dict)

        task_schedules = json.loads(reply_dict[TabletParam.TASK_SCHEDULES])

        self.assertEqual(len(task_schedules), 2)

        s = task_schedules[0]
        self.assertEqual(s[TabletParam.TASK_SCHEDULE_NAME], "Test 1")

        schedule_items = s[TabletParam.TASK_SCHEDULE_ITEMS]
        self.assertEqual(len(schedule_items), 4)

        phq9_1_sched = schedule_items[0]
        self.assertEqual(phq9_1_sched[TabletParam.TABLE], "phq9")
        self.assertEqual(phq9_1_sched[TabletParam.SETTINGS], {
            "phq9_key": "phq9_value"
        })
        self.assertEqual(parse(phq9_1_sched[TabletParam.DUE_FROM]),
                         local(2020, 7, 31))
        self.assertEqual(parse(phq9_1_sched[TabletParam.DUE_BY]),
                         local(2020, 8, 7))
        self.assertFalse(phq9_1_sched[TabletParam.COMPLETE])
        self.assertFalse(phq9_1_sched[TabletParam.ANONYMOUS])

        bmi_sched = schedule_items[1]
        self.assertEqual(bmi_sched[TabletParam.TABLE], "bmi")
        self.assertEqual(bmi_sched[TabletParam.SETTINGS], {
            "bmi_key": "bmi_value",
        })
        self.assertEqual(parse(bmi_sched[TabletParam.DUE_FROM]),
                         local(2020, 7, 31))
        self.assertEqual(parse(bmi_sched[TabletParam.DUE_BY]),
                         local(2020, 8, 8))
        self.assertTrue(bmi_sched[TabletParam.COMPLETE])
        self.assertFalse(bmi_sched[TabletParam.ANONYMOUS])

        phq9_2_sched = schedule_items[2]
        self.assertEqual(phq9_2_sched[TabletParam.TABLE], "phq9")
        self.assertEqual(phq9_2_sched[TabletParam.SETTINGS], {
            "phq9_key": "phq9_value"
        })
        self.assertEqual(parse(phq9_2_sched[TabletParam.DUE_FROM]),
                         local(2020, 8, 30))
        self.assertEqual(parse(phq9_2_sched[TabletParam.DUE_BY]),
                         local(2020, 9, 6))
        self.assertFalse(phq9_2_sched[TabletParam.COMPLETE])
        self.assertFalse(phq9_2_sched[TabletParam.ANONYMOUS])

        # GMCPQ
        gmcpq_sched = schedule_items[3]
        self.assertTrue(gmcpq_sched[TabletParam.ANONYMOUS])
