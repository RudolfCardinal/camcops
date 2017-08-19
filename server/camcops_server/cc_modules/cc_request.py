#!/usr/bin/env python
# camcops_server/cc_modules/cc_request.py

"""
===============================================================================
    Copyright (C) 2012-2017 Rudolf Cardinal (rudolf@pobox.com).

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
===============================================================================
"""

import logging
from typing import Dict, List, Optional, Tuple, TYPE_CHECKING

import arrow
from arrow import Arrow
from cardinal_pythonlib.logs import BraceStyleAdapter
import cardinal_pythonlib.rnc_web as ws
import datetime
from pyramid.decorator import reify
from pyramid.interfaces import ISession
from pyramid.request import Request
from pyramid.response import Response
from sqlalchemy.orm import sessionmaker
from sqlalchemy.orm import Session as SqlASession

from .cc_config import CamcopsConfig, get_config, get_config_filename
from .cc_constants import (
    CAMCOPS_LOGO_FILE_WEBREF,
    DATEFORMAT,
    LOCAL_LOGO_FILE_WEBREF,
    WEB_HEAD,
)
from .cc_dt import format_datetime
from .cc_pyramid import CookieKeys
from .cc_string import all_extra_strings_as_dicts, APPSTRING_TASKNAME

if TYPE_CHECKING:
    from .cc_session import CamcopsSession

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Modified Request interface, for type checking
# =============================================================================
# https://docs.pylonsproject.org/projects/pyramid_cookbook/en/latest/auth/user_object.html
# https://rollbar.com/blog/using-pyramid-request-factory-to-write-less-code/
#
# ... everything with reify=True is cached, so if we ask for something
#     more than once, we keep getting the same thing
# ... https://docs.pylonsproject.org/projects/pyramid/en/latest/api/request.html#pyramid.request.Request.set_property  # noqa


class CamcopsRequest(Request):
    def __init__(self, *args, **kwargs):
        """
        This is called as the Pyramid request factory; see
            config.set_request_factory(CamcopsRequest)

        What's the best way of handling the database client?
        - With Titanium, we were constrained not to use cookies. With Qt, we
          have the option.
        - But are cookies a good idea?
          Probably not; they are somewhat overcomplicated for this.
          See also
          https://softwareengineering.stackexchange.com/questions/141019/
          https://stackoverflow.com/questions/6068113/do-sessions-really-violate-restfulness  # noqa
        - Let's continue to avoid cookies.
        - We don't have to cache any information (we still send username/
          password details with each request, and that is RESTful) but it
          does save authentication time to do so on calls after the first.
        - What we could try to do is:
          - look up a session here, at Request creation time;
          - add a new session if there wasn't one;
          - but allow the database API code to replace that session (BEFORE
            it's saved to the database and gains its PK) with another,
            determined by the content.
          - This gives one more database hit, but avoids the bcrypt time.

        """
        super().__init__(*args, **kwargs)
        self.use_svg = False
        self.add_response_callback(complete_request_add_cookies)
        self.camcops_session = CamcopsSession.get_session_using_cookies(self)

    @reify
    def config_filename(self) -> str:
        """
        Gets the config filename in use.
        """
        return get_config_filename(environ=self.environ)

    @reify
    def config(self) -> CamcopsConfig:
        """
        Return an instance of CamcopsConfig for the request.
        Access it as request.config, with no brackets.
        """
        config = get_config(config_filename=self.config_filename)
        return config

    @reify
    def dbsession(self) -> SqlASession:
        """
        Return an SQLAlchemy session for the relevant request.
        The use of @reify makes this elegant. If and only if a view wants a
        database, it can say
            dbsession = request.dbsession
        and if it requests that, the cleanup callbacks get installed.
        """
        log.info("Making SQLAlchemy session")
        cfg = self.config
        engine = cfg.create_engine()
        maker = sessionmaker(bind=engine)
        session = maker()  # type: SqlASession

        def end_sqlalchemy_session(req: Request) -> None:
            if req.exception is not None:
                session.rollback()
            else:
                session.commit()
            log.info("Closing SQLAlchemy session")
            session.close()

        self.add_finished_callback(end_sqlalchemy_session)

        return session

    @reify
    def now_arrow(self) -> Arrow:
        """
        Returns the time of the request as an Arrow object.
        (Reified, so a request only ever has one time.)
        Exposed as the property: request.now_arrow
        """
        return arrow.now()

    @reify
    def now_utc_datetime(self) -> datetime.datetime:
        """
        Returns the time of the request as a UTC datetime.
        Exposed as the property: request.now_utc_datetime
        """
        a = self.now_arrow  # type: Arrow
        return a.to('utc').datetime

    @reify
    def now_iso8601_era_format(self) -> str:
        return format_datetime(self.now_arrow, DATEFORMAT.ISO8601)

    @reify
    def web_logo_html(self) -> str:
        """
        Returns the time of the request as a UTC datetime.
        Exposed as the property: request.web_logo_html
        """
        # Note: HTML4 uses <img ...>; XHTML uses <img ... />;
        # HTML5 is happy with <img ... />

        # IE float-right problems: http://stackoverflow.com/questions/1820007
        # Tables are a nightmare in IE (table max-width not working unless you
        # also specify it for image size, etc.)
        cfg = self.config
        return """
            <div class="web_logo_header">
                <a href="{}"><img class="logo_left" src="{}" alt="" /></a>
                <a href="{}"><img class="logo_right" src="{}" alt="" /></a>
            </div>
        """.format(
            self.script_name, CAMCOPS_LOGO_FILE_WEBREF,
            cfg.LOCAL_INSTITUTION_URL, LOCAL_LOGO_FILE_WEBREF
        )

    @reify
    def webstart_html(self) -> str:
        """
        Returns the time of the request as a UTC datetime.
        Exposed as the property: request.webstart_html
        """
        return WEB_HEAD + self.web_logo_html

    @reify
    def _all_extra_strings(self) -> Dict[str, Dict[str, str]]:
        return all_extra_strings_as_dicts(self.config_filename)

    def xstring(self,
                taskname: str,
                stringname: str,
                default: str = None,
                provide_default_if_none: bool = True) -> Optional[str]:
        """
        Looks up a string from one of the optional extra XML string files.
        """
        # For speed, calculate default only if needed:
        allstrings = self._all_extra_strings
        if taskname in allstrings:
            if stringname in allstrings[taskname]:
                return allstrings[taskname].get(stringname)
        if default is None and provide_default_if_none:
            default = "EXTRA_STRING_NOT_FOUND({}.{})".format(taskname,
                                                             stringname)
        return default

    def wxstring(self,
                 taskname: str,
                 stringname: str,
                 default: str = None,
                 provide_default_if_none: bool = True) -> Optional[str]:
        """Returns a web-safe version of an xstring (see above)."""
        value = self.xstring(taskname, stringname, default,
                             provide_default_if_none=provide_default_if_none)
        if value is None and not provide_default_if_none:
            return None
        return ws.webify(value)

    def wappstring(self,
                   stringname: str,
                   default: str = None,
                   provide_default_if_none: bool = True) -> Optional[str]:
        """
        Returns a web-safe version of an appstring (an app-wide extra string.
        """
        value = self.xstring(APPSTRING_TASKNAME, stringname, default,
                             provide_default_if_none=provide_default_if_none)
        if value is None and not provide_default_if_none:
            return None
        return ws.webify(value)

    def get_all_extra_strings(self) -> List[Tuple[str, str, str]]:
        """
        Returns all extra strings, as a list of (task, name, value) tuples.
        """
        allstrings = self._all_extra_strings
        rows = []
        for task, subdict in allstrings.items():
            for name, value in subdict.items():
                rows.append((task, name, value))
        return rows

    def task_extrastrings_exist(self, taskname: str) -> bool:
        """
        Has the server been supplied with extra strings for a specific task?
        """
        allstrings = self._all_extra_strings
        return taskname in allstrings

    def switch_output_to_png(self) -> None:
        """Switch server to producing figures in PNG."""
        self.use_svg = False

    def switch_output_to_svg(self) -> None:
        """Switch server to producing figures in SVG."""
        self.use_svg = True

    def replace_camcops_session(self, ccsession: "CamcopsSession") -> None:
        # We may have created a new HTTP session because the request had no
        # cookies (added to the DB session but not yet saved), but we might
        # then enter the database/tablet upload API and find session details,
        # not from the cookies, but from the POST data. At that point, we
        # want to replace the session in the Request, without committing the
        # first one to disk.
        self.dbsession.expunge(self.camcops_session)
        self.camcops_session = ccsession


# noinspection PyUnusedLocal
def complete_request_add_cookies(req: CamcopsRequest, response: Response):
    """
    Finializes the response by adding session cookies.
    We do this late so that we can hot-swap the session if we're using the
    database/tablet API rather than a human web browser.

    Response callbacks are called in the order first-to-most-recently-added.
    See pyramid.request.CallbackMethodsMixin.

    That looks like we can add a callback in the process of running a callback.
    And when we add a cookie to a Pyramid session, that sets a callback.
    Let's give it a go...
    """
    dbsession = req.dbsession
    dbsession.flush()  # sets the PK for ccsession, if it wasn't set
    # Write the details back to the Pyramid session (will be persisted
    # via the Response automatically):
    pyramid_session = req.session  # type: ISession
    ccsession = req.camcops_session
    pyramid_session[CookieKeys.SESSION_ID] = str(ccsession.id)
    pyramid_session[CookieKeys.SESSION_TOKEN] = ccsession.token
    # ... should cause the ISession to add a callback to add cookies,
    # which will be called immediately after this one.
