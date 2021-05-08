#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_forms_tests.py

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
from pprint import pformat
from typing import Any, Dict
from unittest import mock, TestCase

# noinspection PyProtectedMember
from colander import Invalid, null, Schema
from pendulum import Duration

from camcops_server.cc_modules.cc_baseconstants import TEMPLATE_DIR
from camcops_server.cc_modules.cc_forms import (
    DurationType,
    DurationWidget,
    GroupIpUseWidget,
    IpUseType,
    JsonType,
    JsonWidget,
    LoginSchema,
    TaskScheduleItemSchema,
    TaskScheduleNode,
    TaskScheduleSchema,
)
from camcops_server.cc_modules.cc_ipuse import IpContexts
from camcops_server.cc_modules.cc_pyramid import ViewParam
from camcops_server.cc_modules.cc_taskschedule import TaskSchedule
from camcops_server.cc_modules.cc_unittest import (
    DemoDatabaseTestCase,
    DemoRequestTestCase,
)


# =============================================================================
# Unit tests
# =============================================================================

class SchemaTestCase(DemoRequestTestCase):
    """
    Unit tests.
    """
    def serialize_deserialize(self,
                              schema: Schema,
                              appstruct: Dict[str, Any]) -> None:
        cstruct = schema.serialize(appstruct)
        final = schema.deserialize(cstruct)
        mismatch = False
        for k, v in appstruct.items():
            if final[k] != v:
                mismatch = True
                break
        self.assertFalse(mismatch, msg=(
            "Elements of final don't match corresponding elements of starting "
            "appstruct:\n"
            f"final = {pformat(final)}\n"
            f"start = {pformat(appstruct)}"
        ))


class LoginSchemaTests(SchemaTestCase):
    def test_serialize_deserialize(self) -> None:
        appstruct = {
            ViewParam.USERNAME: "testuser",
            ViewParam.PASSWORD: "testpw",
        }
        schema = LoginSchema().bind(request=self.req)

        self.serialize_deserialize(schema, appstruct)


class TaskScheduleSchemaTests(DemoDatabaseTestCase):
    def test_invalid_for_bad_template_placeholder(self) -> None:
        schema = TaskScheduleSchema().bind(request=self.req)
        appstruct = {
            ViewParam.NAME: "test",
            ViewParam.GROUP_ID: "1",
            ViewParam.EMAIL_SUBJECT: "Subject",
            ViewParam.EMAIL_TEMPLATE: "{bad_key}",
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("'bad_key' is not a valid placeholder",
                      cm.exception.children[0].messages()[0])

    def test_invalid_for_mismatched_braces(self) -> None:
        schema = TaskScheduleSchema().bind(request=self.req)
        appstruct = {
            ViewParam.NAME: "test",
            ViewParam.GROUP_ID: "1",
            ViewParam.EMAIL_SUBJECT: "Subject",
            ViewParam.EMAIL_TEMPLATE: "{server_url",
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("Invalid email template",
                      cm.exception.children[0].messages()[0])


class TaskScheduleItemSchemaTests(SchemaTestCase):
    def test_serialize_deserialize(self) -> None:
        appstruct = {
            ViewParam.SCHEDULE_ID: 1,
            ViewParam.TABLE_NAME: "bmi",
            ViewParam.CLINICIAN_CONFIRMATION: False,
            ViewParam.DUE_FROM: Duration(days=90),
            ViewParam.DUE_WITHIN: Duration(days=100)
        }
        schema = TaskScheduleItemSchema().bind(request=self.req)
        self.serialize_deserialize(schema, appstruct)

    def test_invalid_for_clinician_task_with_no_confirmation(self) -> None:
        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: 1,
            ViewParam.TABLE_NAME: "elixhauserci",
            ViewParam.CLINICIAN_CONFIRMATION: False,
            ViewParam.DUE_FROM: Duration(days=90),
            ViewParam.DUE_WITHIN: Duration(days=100)
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("you must tick 'Allow clinician tasks'",
                      cm.exception.messages()[0])

    def test_valid_for_clinician_task_with_confirmation(self) -> None:
        schema = TaskScheduleItemSchema().bind(request=mock.Mock())
        appstruct = {
            ViewParam.SCHEDULE_ID: 1,
            ViewParam.TABLE_NAME: "elixhauserci",
            ViewParam.CLINICIAN_CONFIRMATION: True,
            ViewParam.DUE_FROM: Duration(days=90),
            ViewParam.DUE_WITHIN: Duration(days=100)
        }

        try:
            schema.serialize(appstruct)
        except Invalid:
            self.fail("Validation failed unexpectedly")

    def test_invalid_for_zero_due_within(self) -> None:
        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: 1,
            ViewParam.TABLE_NAME: "phq9",
            ViewParam.CLINICIAN_CONFIRMATION: False,
            ViewParam.DUE_FROM: Duration(days=90),
            ViewParam.DUE_WITHIN: Duration(days=0)
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("must be more than zero days",
                      cm.exception.messages()[0])

    def test_invalid_for_negative_due_within(self) -> None:
        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: 1,
            ViewParam.TABLE_NAME: "phq9",
            ViewParam.CLINICIAN_CONFIRMATION: False,
            ViewParam.DUE_FROM: Duration(days=90),
            ViewParam.DUE_WITHIN: Duration(days=-1)
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("must be more than zero days",
                      cm.exception.messages()[0])

    def test_invalid_for_negative_due_from(self) -> None:
        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: 1,
            ViewParam.TABLE_NAME: "phq9",
            ViewParam.CLINICIAN_CONFIRMATION: False,
            ViewParam.DUE_FROM: Duration(days=-1),
            ViewParam.DUE_WITHIN: Duration(days=10)
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("must be zero or more days",
                      cm.exception.messages()[0])


class TaskScheduleItemSchemaIpTests(DemoDatabaseTestCase):
    def setUp(self) -> None:
        super().setUp()

        self.schedule = TaskSchedule()
        self.schedule.group_id = self.group.id
        self.dbsession.add(self.schedule)
        self.dbsession.commit()

    def create_tasks(self) -> None:
        # Speed things up a bit
        pass

    def test_invalid_for_commercial_mismatch(self) -> None:
        self.group.ip_use.commercial = True
        self.dbsession.add(self.group)
        self.dbsession.commit()

        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: self.schedule.id,
            ViewParam.TABLE_NAME: "mfi20",
            ViewParam.CLINICIAN_CONFIRMATION: False,
            ViewParam.DUE_FROM: Duration(days=0),
            ViewParam.DUE_WITHIN: Duration(days=10)
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("prohibits commercial",
                      cm.exception.messages()[0])

    def test_invalid_for_clinical_mismatch(self) -> None:
        self.group.ip_use.clinical = True
        self.dbsession.add(self.group)
        self.dbsession.commit()

        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: self.schedule.id,
            ViewParam.TABLE_NAME: "mfi20",
            ViewParam.CLINICIAN_CONFIRMATION: False,
            ViewParam.DUE_FROM: Duration(days=0),
            ViewParam.DUE_WITHIN: Duration(days=10),
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("prohibits clinical",
                      cm.exception.messages()[0])

    def test_invalid_for_educational_mismatch(self) -> None:
        self.group.ip_use.educational = True
        self.dbsession.add(self.group)
        self.dbsession.commit()

        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: self.schedule.id,
            ViewParam.TABLE_NAME: "mfi20",
            ViewParam.CLINICIAN_CONFIRMATION: True,
            ViewParam.DUE_FROM: Duration(days=0),
            ViewParam.DUE_WITHIN: Duration(days=10),
        }

        cstruct = schema.serialize(appstruct)

        # No real world example prohibits educational use
        mock_task_class = mock.Mock(prohibits_educational=True)
        with mock.patch.object(schema, "_get_task_class",
                               return_value=mock_task_class):
            with self.assertRaises(Invalid) as cm:
                schema.deserialize(cstruct)

        self.assertIn("prohibits educational",
                      cm.exception.messages()[0])

    def test_invalid_for_research_mismatch(self) -> None:
        self.group.ip_use.research = True
        self.dbsession.add(self.group)
        self.dbsession.commit()

        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: self.schedule.id,
            ViewParam.TABLE_NAME: "moca",
            ViewParam.CLINICIAN_CONFIRMATION: True,
            ViewParam.DUE_FROM: Duration(days=0),
            ViewParam.DUE_WITHIN: Duration(days=10),
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn("prohibits research",
                      cm.exception.messages()[0])

    def test_invalid_for_missing_ip_use(self) -> None:
        self.group.ip_use = None
        self.dbsession.add(self.group)
        self.dbsession.commit()

        schema = TaskScheduleItemSchema().bind(request=self.req)
        appstruct = {
            ViewParam.SCHEDULE_ID: self.schedule.id,
            ViewParam.TABLE_NAME: "moca",
            ViewParam.CLINICIAN_CONFIRMATION: True,
            ViewParam.DUE_FROM: Duration(days=0),
            ViewParam.DUE_WITHIN: Duration(days=10),
        }

        cstruct = schema.serialize(appstruct)
        with self.assertRaises(Invalid) as cm:
            schema.deserialize(cstruct)

        self.assertIn(
            f"The group '{self.group.name}' has no intellectual property "
            f"settings",
            cm.exception.messages()[0]
        )


class DurationWidgetTests(TestCase):
    def setUp(self) -> None:
        super().setUp()
        self.request = mock.Mock(gettext=lambda t: t)

    def test_serialize_renders_template_with_values(self) -> None:
        widget = DurationWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = {
            "months": 1,
            "weeks": 2,
            "days": 3,
        }

        widget.serialize(field, cstruct, readonly=False)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0], f"{TEMPLATE_DIR}/deform/duration.pt")
        self.assertFalse(kwargs["readonly"])

        self.assertEqual(kwargs["months"], 1)
        self.assertEqual(kwargs["weeks"], 2)
        self.assertEqual(kwargs["days"], 3)

        self.assertEqual(kwargs["field"], field)

    def test_serialize_renders_readonly_template_with_values(self) -> None:
        widget = DurationWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = {
            "months": 1,
            "weeks": 2,
            "days": 3,
        }

        widget.serialize(field, cstruct, readonly=True)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0], f"{TEMPLATE_DIR}/deform/readonly/duration.pt")
        self.assertTrue(kwargs["readonly"])

    def test_serialize_renders_readonly_template_if_widget_is_readonly(
            self) -> None:
        widget = DurationWidget(self.request, readonly=True)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = {
            "months": 1,
            "weeks": 2,
            "days": 3,
        }

        widget.serialize(field, cstruct)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0], f"{TEMPLATE_DIR}/deform/readonly/duration.pt")

    def test_serialize_with_null_defaults_to_blank_values(self) -> None:
        widget = DurationWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        widget.serialize(field, null)

        args, kwargs = field.renderer.call_args

        self.assertEqual(kwargs["months"], "")
        self.assertEqual(kwargs["weeks"], "")
        self.assertEqual(kwargs["days"], "")

    def test_serialize_none_defaults_to_blank_values(self) -> None:
        widget = DurationWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        widget.serialize(field, None)

        args, kwargs = field.renderer.call_args

        self.assertEqual(kwargs["months"], "")
        self.assertEqual(kwargs["weeks"], "")
        self.assertEqual(kwargs["days"], "")

    def test_deserialize_returns_valid_values(self) -> None:
        widget = DurationWidget(self.request)

        pstruct = {
            "days": 1,
            "weeks": 2,
            "months": 3,
        }

        # noinspection PyTypeChecker
        cstruct = widget.deserialize(None, pstruct)

        self.assertEqual(cstruct["days"], 1)
        self.assertEqual(cstruct["weeks"], 2)
        self.assertEqual(cstruct["months"], 3)

    def test_deserialize_defaults_to_zero_days(self) -> None:
        widget = DurationWidget(self.request)

        # noinspection PyTypeChecker
        cstruct = widget.deserialize(None, {})

        self.assertEqual(cstruct["days"], 0)

    def test_deserialize_fails_validation(self) -> None:
        widget = DurationWidget(self.request)

        pstruct = {
            "days": "abc",
            "weeks": "def",
            "months": "ghi",
        }

        with self.assertRaises(Invalid) as cm:
            # noinspection PyTypeChecker
            widget.deserialize(None, pstruct)

        self.assertIn("Please enter a valid number of days or leave blank",
                      cm.exception.messages())
        self.assertIn("Please enter a valid number of weeks or leave blank",
                      cm.exception.messages())
        self.assertIn("Please enter a valid number of months or leave blank",
                      cm.exception.messages())
        self.assertEqual(cm.exception.value, pstruct)


class DurationTypeTests(TestCase):
    def test_deserialize_valid_duration(self) -> None:
        cstruct = {"days": 45}

        duration_type = DurationType()
        duration = duration_type.deserialize(None, cstruct)
        assert duration is not None  # for type checker

        self.assertEqual(duration.days, 45)

    def test_deserialize_none_returns_null(self) -> None:
        duration_type = DurationType()
        duration = duration_type.deserialize(None, None)
        self.assertIsNone(duration)

    def test_deserialize_ignores_invalid_days(self) -> None:
        duration_type = DurationType()
        cstruct = {"days": "abc", "months": 1, "weeks": 1}
        duration = duration_type.deserialize(None, cstruct)
        assert duration is not None  # for type checker

        self.assertEqual(duration.days, 37)

    def test_deserialize_ignores_invalid_months(self) -> None:
        duration_type = DurationType()
        cstruct = {"days": 1, "months": "abc", "weeks": 1}
        duration = duration_type.deserialize(None, cstruct)
        assert duration is not None  # for type checker

        self.assertEqual(duration.days, 8)

    def test_deserialize_ignores_invalid_weeks(self) -> None:
        duration_type = DurationType()
        cstruct = {"days": 1, "months": 1, "weeks": "abc"}
        duration = duration_type.deserialize(None, cstruct)
        assert duration is not None  # for type checker

        self.assertEqual(duration.days, 31)

    def test_serialize_valid_duration(self) -> None:
        duration = Duration(days=47)

        duration_type = DurationType()
        cstruct = duration_type.serialize(None, duration)

        # For type checker
        assert cstruct not in (null,)
        cstruct: Dict[Any, Any]

        self.assertEqual(cstruct["days"], 3)
        self.assertEqual(cstruct["months"], 1)
        self.assertEqual(cstruct["weeks"], 2)

    def test_serialize_null_returns_null(self) -> None:
        duration_type = DurationType()
        cstruct = duration_type.serialize(None, null)
        self.assertIs(cstruct, null)


class JsonWidgetTests(TestCase):
    def setUp(self) -> None:
        super().setUp()
        self.request = mock.Mock(gettext=lambda t: t)

    def test_serialize_renders_template_with_values(self) -> None:
        widget = JsonWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = json.dumps({"a": "1", "b": "2", "c": "3"})

        widget.serialize(field, cstruct, readonly=False)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0], f"{TEMPLATE_DIR}/deform/json.pt")
        self.assertFalse(kwargs["readonly"])

        self.assertEqual(kwargs["cstruct"], cstruct)
        self.assertEqual(kwargs["field"], field)

    def test_serialize_renders_readonly_template_with_values(self) -> None:
        widget = JsonWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = json.dumps({"a": "1", "b": "2", "c": "3"})

        widget.serialize(field, cstruct, readonly=True)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0], f"{TEMPLATE_DIR}/deform/readonly/json.pt")

        self.assertEqual(kwargs["cstruct"], cstruct)
        self.assertEqual(kwargs["field"], field)
        self.assertTrue(kwargs["readonly"])

    def test_serialize_renders_readonly_template_if_widget_is_readonly(
            self) -> None:
        widget = JsonWidget(self.request, readonly=True)

        field = mock.Mock()
        field.renderer = mock.Mock()

        json_text = json.dumps({"a": "1", "b": "2", "c": "3"})
        widget.serialize(field, json_text)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0], f"{TEMPLATE_DIR}/deform/readonly/json.pt")

    def test_serialize_with_null_defaults_to_empty_string(self) -> None:
        widget = JsonWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        widget.serialize(field, null)

        args, kwargs = field.renderer.call_args

        self.assertEqual(kwargs["cstruct"], "")

    def test_deserialize_passes_json(self) -> None:
        widget = JsonWidget(self.request)

        pstruct = json.dumps({"a": "1", "b": "2", "c": "3"})

        # noinspection PyTypeChecker
        cstruct = widget.deserialize(None, pstruct)

        self.assertEqual(cstruct, pstruct)

    def test_deserialize_defaults_to_empty_json_string(self) -> None:
        widget = JsonWidget(self.request)

        # noinspection PyTypeChecker
        cstruct = widget.deserialize(None, "{}")

        self.assertEqual(cstruct, "{}")

    def test_deserialize_invalid_json_fails_validation(self) -> None:
        widget = JsonWidget(self.request)

        pstruct = "{"

        with self.assertRaises(Invalid) as cm:
            # noinspection PyTypeChecker
            widget.deserialize(None, pstruct)

        self.assertIn(
            "Please enter valid JSON",
            cm.exception.messages()[0]
        )

        self.assertEqual(cm.exception.value, "{")


class JsonTypeTests(TestCase):
    def test_deserialize_valid_json(self) -> None:
        original = {"one": 1, "two": 2, "three": 3}

        json_type = JsonType()
        json_value = json_type.deserialize(None, json.dumps(original))
        self.assertEqual(json_value, original)

    def test_deserialize_null_returns_none(self) -> None:
        json_type = JsonType()
        json_value = json_type.deserialize(None, null)
        self.assertIsNone(json_value)

    def test_deserialize_none_returns_null(self) -> None:
        json_type = JsonType()
        json_value = json_type.deserialize(None, None)
        self.assertIsNone(json_value)

    def test_deserialize_invalid_json_returns_none(self) -> None:
        json_type = JsonType()
        json_value = json_type.deserialize(None, "{")
        self.assertIsNone(json_value)

    def test_serialize_valid_appstruct(self) -> None:
        original = {"one": 1, "two": 2, "three": 3}

        json_type = JsonType()
        json_string = json_type.serialize(None, original)
        self.assertEqual(json_string, json.dumps(original))

    def test_serialize_null_returns_null(self) -> None:
        json_type = JsonType()
        json_string = json_type.serialize(None, null)
        self.assertIs(json_string, null)


class TaskScheduleNodeTests(TestCase):

    def test_deserialize_not_a_json_object_fails_validation(self) -> None:
        node = TaskScheduleNode()
        with self.assertRaises(Invalid) as cm:
            node.deserialize({})

            self.assertIn(
                "Please enter a valid JSON object",
                cm.exception.messages()[0]
            )

            self.assertEqual(cm.exception.value, "[{}]")


class GroupIpUseWidgetTests(TestCase):
    def setUp(self) -> None:
        super().setUp()
        self.request = mock.Mock(gettext=lambda t: t)

    def test_serialize_renders_template_with_values(self) -> None:
        widget = GroupIpUseWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = {
            IpContexts.CLINICAL: False,
            IpContexts.COMMERCIAL: False,
            IpContexts.EDUCATIONAL: True,
            IpContexts.RESEARCH: True,
        }

        widget.serialize(field, cstruct, readonly=False)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0], f"{TEMPLATE_DIR}/deform/group_ip_use.pt")
        self.assertFalse(kwargs["readonly"])

        self.assertFalse(kwargs[IpContexts.CLINICAL])
        self.assertFalse(kwargs[IpContexts.COMMERCIAL])
        self.assertTrue(kwargs[IpContexts.EDUCATIONAL])
        self.assertTrue(kwargs[IpContexts.RESEARCH])
        self.assertEqual(kwargs["field"], field)

    def test_serialize_renders_readonly_template(self) -> None:
        widget = GroupIpUseWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = {
            IpContexts.CLINICAL: False,
            IpContexts.COMMERCIAL: False,
            IpContexts.EDUCATIONAL: True,
            IpContexts.RESEARCH: True,
        }

        widget.serialize(field, cstruct, readonly=True)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0],
                         f"{TEMPLATE_DIR}/deform/readonly/group_ip_use.pt")
        self.assertTrue(kwargs["readonly"])

    def test_serialize_readonly_widget_renders_readonly_template(self) -> None:
        widget = GroupIpUseWidget(self.request, readonly=True)

        field = mock.Mock()
        field.renderer = mock.Mock()

        cstruct = {
            IpContexts.CLINICAL: False,
            IpContexts.COMMERCIAL: False,
            IpContexts.EDUCATIONAL: True,
            IpContexts.RESEARCH: True,
        }

        widget.serialize(field, cstruct)

        args, kwargs = field.renderer.call_args

        self.assertEqual(args[0],
                         f"{TEMPLATE_DIR}/deform/readonly/group_ip_use.pt")

    def test_serialize_with_null_defaults_to_false_values(self) -> None:
        widget = GroupIpUseWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        widget.serialize(field, null)

        args, kwargs = field.renderer.call_args

        self.assertFalse(kwargs[IpContexts.CLINICAL])
        self.assertFalse(kwargs[IpContexts.COMMERCIAL])
        self.assertFalse(kwargs[IpContexts.EDUCATIONAL])
        self.assertFalse(kwargs[IpContexts.RESEARCH])

    def test_serialize_with_none_defaults_to_false_values(self) -> None:
        widget = GroupIpUseWidget(self.request)

        field = mock.Mock()
        field.renderer = mock.Mock()

        widget.serialize(field, None)

        args, kwargs = field.renderer.call_args

        self.assertFalse(kwargs[IpContexts.CLINICAL])
        self.assertFalse(kwargs[IpContexts.COMMERCIAL])
        self.assertFalse(kwargs[IpContexts.EDUCATIONAL])
        self.assertFalse(kwargs[IpContexts.RESEARCH])

    def test_deserialize_with_null_defaults_to_false_values(self) -> None:
        widget = GroupIpUseWidget(self.request)

        field = None  # Not used
        # noinspection PyTypeChecker
        cstruct = widget.deserialize(field, null)

        self.assertFalse(cstruct[IpContexts.CLINICAL])
        self.assertFalse(cstruct[IpContexts.COMMERCIAL])
        self.assertFalse(cstruct[IpContexts.EDUCATIONAL])
        self.assertFalse(cstruct[IpContexts.RESEARCH])

    def test_deserialize_converts_to_bool_values(self) -> None:
        widget = GroupIpUseWidget(self.request)

        field = None  # Not used

        # It shouldn't matter what the values are set to so long as the keys
        # are present. In practice the values will be set to "1"
        pstruct = {
            IpContexts.EDUCATIONAL: "1",
            IpContexts.RESEARCH: "1",
        }

        # noinspection PyTypeChecker
        cstruct = widget.deserialize(field, pstruct)

        self.assertFalse(cstruct[IpContexts.CLINICAL])
        self.assertFalse(cstruct[IpContexts.COMMERCIAL])
        self.assertTrue(cstruct[IpContexts.EDUCATIONAL])
        self.assertTrue(cstruct[IpContexts.RESEARCH])


class IpUseTypeTests(TestCase):
    def test_deserialize_none_returns_none(self) -> None:
        ip_use_type = IpUseType()

        node = None  # not used
        self.assertIsNone(ip_use_type.deserialize(node, None), None)

    def test_deserialize_null_returns_none(self) -> None:
        ip_use_type = IpUseType()

        node = None  # not used
        self.assertIsNone(ip_use_type.deserialize(node, null), None)

    def test_deserialize_returns_ip_use_object(self) -> None:
        ip_use_type = IpUseType()

        node = None  # not used

        cstruct = {
            IpContexts.CLINICAL: False,
            IpContexts.COMMERCIAL: True,
            IpContexts.EDUCATIONAL: False,
            IpContexts.RESEARCH: True,
        }
        ip_use = ip_use_type.deserialize(node, cstruct)

        self.assertFalse(ip_use.clinical)
        self.assertTrue(ip_use.commercial)
        self.assertFalse(ip_use.educational)
        self.assertTrue(ip_use.research)
