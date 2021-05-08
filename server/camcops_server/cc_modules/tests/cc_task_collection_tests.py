#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_taskcollection_tests.py

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

from kombu.serialization import dumps, loads
from camcops_server.cc_modules.cc_taskcollection import (
    TaskCollection,
    TaskSortMethod,
)

from camcops_server.cc_modules.cc_taskfilter import TaskFilter
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase


# =============================================================================
# Unit tests
# =============================================================================

class TaskCollectionTests(DemoDatabaseTestCase):
    def create_tasks(self) -> None:
        return

    def test_it_can_be_serialized(self) -> None:
        taskfilter = TaskFilter()
        taskfilter.task_types = ['task1', 'task2', 'task3']
        taskfilter.group_ids = [1, 2, 3]

        coll = TaskCollection(
            self.req,
            taskfilter=taskfilter,
            as_dump=True,
            sort_method_by_class=TaskSortMethod.CREATION_DATE_ASC
        )
        content_type, encoding, data = dumps(coll, serializer="json")
        new_coll = loads(data, content_type, encoding)

        self.assertEqual(new_coll._as_dump, True)
        self.assertEqual(new_coll._sort_method_by_class,
                         TaskSortMethod.CREATION_DATE_ASC)
        self.assertEqual(new_coll._filter.task_types,
                         ['task1', 'task2', 'task3'])
        self.assertEqual(new_coll._filter.group_ids,
                         [1, 2, 3])
