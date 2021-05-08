#!/usr/bin/env python

"""
camcops_server/cc_modules/tests/cc_session_tests.py

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

from pendulum import DateTime as Pendulum

from camcops_server.cc_modules.cc_session import CamcopsSession, generate_token
from camcops_server.cc_modules.cc_taskfilter import TaskFilter
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase
from camcops_server.cc_modules.cc_user import User


# =============================================================================
# Unit tests
# =============================================================================

class SessionTests(DemoDatabaseTestCase):
    """
    Unit tests.
    """
    def test_sessions(self) -> None:
        self.announce("test_sessions")
        req = self.req
        dbsession = self.dbsession

        self.assertIsInstance(generate_token(), str)

        CamcopsSession.delete_old_sessions(req)
        self.assertIsInstance(
            CamcopsSession.get_oldest_last_activity_allowed(req), Pendulum)

        s = req.camcops_session
        u = self.dbsession.query(User).first()  # type: User
        assert u, "Missing user in demo database!"

        self.assertIsInstance(s.last_activity_utc_iso, str)
        self.assertIsInstanceOrNone(s.username, str)
        s.logout()
        s.login(u)
        self.assertIsInstance(s.get_task_filter(), TaskFilter)

        # Now test deletion cascade
        dbsession.commit()
        numfilters = dbsession.query(TaskFilter).count()
        assert numfilters == 1, "TaskFilter count should be 1"

        dbsession.delete(s)
        dbsession.commit()
        numfilters = dbsession.query(TaskFilter).count()
        assert numfilters == 0, (
            "TaskFilter count should be 0; cascade delete not working"
        )
