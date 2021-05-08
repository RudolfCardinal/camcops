#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_request.py

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

**Implements a Pyramid Request object customized for CamCOPS.**

"""

import collections
from contextlib import contextmanager
import datetime
import gettext
import logging
import os
import secrets
from typing import (Any, Dict, Generator, List, Optional, Set,
                    Tuple, TYPE_CHECKING, Union)
import urllib.parse

from cardinal_pythonlib.datetimefunc import (
    coerce_to_pendulum,
    coerce_to_pendulum_date,
    convert_datetime_to_utc,
    format_datetime,
    pendulum_to_utc_datetime_without_tz,
)
# from cardinal_pythonlib.debugging import get_caller_stack_info
from cardinal_pythonlib.fileops import get_directory_contents_size, mkdir_p
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.plot import (
    png_img_html_from_pyplot_figure,
    svg_html_from_pyplot_figure,
)
import cardinal_pythonlib.rnc_web as ws
from cardinal_pythonlib.wsgi.constants import WsgiEnvVar
import lockfile
from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.font_manager import FontProperties
from pendulum import Date, DateTime as Pendulum, Duration
from pendulum.parsing.exceptions import ParserError
from pyramid.config import Configurator
from pyramid.decorator import reify
from pyramid.httpexceptions import HTTPBadRequest, HTTPException
from pyramid.interfaces import ISession
from pyramid.request import Request
from pyramid.response import Response
from pyramid.testing import DummyRequest
from sqlalchemy.engine.base import Engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.orm import Session as SqlASession
from webob.multidict import MultiDict

# Note: everything uder the sun imports this file, so keep the intra-package
# imports as minimal as possible.
from camcops_server.cc_modules.cc_baseconstants import (
    DOCUMENTATION_URL,
    ENVVAR_CONFIG_FILE,
    TRANSLATIONS_DIR,
)
from camcops_server.cc_modules.cc_config import (
    CamcopsConfig,
    get_config,
    get_config_filename_from_os_env,
)
from camcops_server.cc_modules.cc_constants import (
    CSS_PAGED_MEDIA,
    DateFormat,
    PlotDefaults,
    USE_SVG_IN_HTML,
)
from camcops_server.cc_modules.cc_idnumdef import (
    get_idnum_definitions,
    IdNumDefinition,
    validate_id_number,
)
from camcops_server.cc_modules.cc_language import (
    DEFAULT_LOCALE,
    GETTEXT_DOMAIN,
)
# noinspection PyUnresolvedReferences
import camcops_server.cc_modules.cc_plot  # import side effects (configure matplotlib)  # noqa
from camcops_server.cc_modules.cc_pyramid import (
    camcops_add_mako_renderer,
    CamcopsAuthenticationPolicy,
    CamcopsAuthorizationPolicy,
    CookieKey,
    get_session_factory,
    Permission,
    RequestMethod,
    RouteCollection,
    STATIC_CAMCOPS_PACKAGE_PATH,
)
from camcops_server.cc_modules.cc_response import camcops_response_factory
from camcops_server.cc_modules.cc_serversettings import (
    get_server_settings,
    ServerSettings,
)
from camcops_server.cc_modules.cc_string import (
    all_extra_strings_as_dicts,
    APPSTRING_TASKNAME,
    MISSING_LOCALE,
)
from camcops_server.cc_modules.cc_tabletsession import TabletSession
from camcops_server.cc_modules.cc_text import SS, server_string
from camcops_server.cc_modules.cc_user import User
from camcops_server.cc_modules.cc_validators import (
    STRING_VALIDATOR_TYPE,
    validate_alphanum_underscore,
    validate_redirect_url,
)

if TYPE_CHECKING:
    from matplotlib.axis import Axis
    from matplotlib.axes import Axes
    # from matplotlib.figure import SubplotBase
    from matplotlib.text import Text
    from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient
    from camcops_server.cc_modules.cc_exportrecipientinfo import ExportRecipientInfo  # noqa
    from camcops_server.cc_modules.cc_session import CamcopsSession
    from camcops_server.cc_modules.cc_snomed import SnomedConcept

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Debugging options
# =============================================================================

DEBUG_ADD_ROUTES = False
DEBUG_AUTHORIZATION = False
DEBUG_CAMCOPS_SESSION = False
DEBUG_DBSESSION_MANAGEMENT = False
DEBUG_GETTEXT = False
DEBUG_REQUEST_CREATION = False
DEBUG_TABLET_SESSION = False

if any([DEBUG_ADD_ROUTES,
        DEBUG_AUTHORIZATION,
        DEBUG_CAMCOPS_SESSION,
        DEBUG_DBSESSION_MANAGEMENT,
        DEBUG_GETTEXT,
        DEBUG_REQUEST_CREATION,
        DEBUG_TABLET_SESSION]):
    log.warning("cc_request: Debugging options enabled!")


# =============================================================================
# Constants
# =============================================================================

TRUE_STRINGS_LOWER_CASE = ["true", "t", "1", "yes", "y"]
FALSE_STRINGS_LOWER_CASE = ["false", "f", "0", "no", "n"]


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
    """
    The CamcopsRequest is an object central to all HTTP requests. It is the
    main thing passed all around the server, and embodies what we need to know
    about the client request -- including user information, ways of accessing
    the database, and so on.
    """
    def __init__(self, *args, **kwargs):
        """
        This is called as the Pyramid request factory; see
        ``config.set_request_factory(CamcopsRequest)``

        What's the best way of handling the database client?

        - With Titanium, we were constrained not to use cookies. With Qt, we
          have the option.
        - But are cookies a good idea?
          Probably not; they are somewhat overcomplicated for this.
          See also

          - https://softwareengineering.stackexchange.com/questions/141019/
          - https://stackoverflow.com/questions/6068113/do-sessions-really-violate-restfulness

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

        """  # noqa
        super().__init__(*args, **kwargs)
        self.use_svg = False  # use SVG (not just PNG) for graphics
        self.provide_png_fallback_for_svg = True  # for SVG: provide PNG fallback image?  # noqa
        self.add_response_callback(complete_request_add_cookies)
        self._camcops_session = None  # type: Optional[CamcopsSession]
        self._debugging_db_session = None  # type: Optional[SqlASession]  # for unit testing only  # noqa
        self._debugging_user = None  # type: Optional[User]  # for unit testing only  # noqa
        self._pending_export_push_requests = []  # type: List[Tuple[str, str, int]]  # noqa
        self._cached_sstring = {}  # type: Dict[SS, str]
        # Don't make the _camcops_session yet; it will want a Registry, and
        # we may not have one yet; see command_line_request().
        if DEBUG_REQUEST_CREATION:
            log.debug("CamcopsRequest.__init__: args={!r}, kwargs={!r}",
                      args, kwargs)

    # -------------------------------------------------------------------------
    # HTTP nonce
    # -------------------------------------------------------------------------

    @reify
    def nonce(self) -> str:
        """
        Return a nonce that is generated at random for each request, but
        remains constant for that request (because we use ``@reify``).

        See https://content-security-policy.com/examples/allow-inline-style/.

        And for how to make one:
        https://stackoverflow.com/questions/5590170/what-is-the-standard-method-for-generating-a-nonce-in-python
        """  # noqa
        return secrets.token_urlsafe()

    # -------------------------------------------------------------------------
    # CamcopsSession
    # -------------------------------------------------------------------------

    @property
    def camcops_session(self) -> "CamcopsSession":
        """
        Returns the
        :class:`camcops_server.cc_modules.cc_session.CamcopsSession` for this
        request (q.v.).

        Contrast:

        .. code-block:: none

            ccsession = request.camcops_session  # type: CamcopsSession
            pyramid_session = request.session  # type: ISession
        """
        if self._camcops_session is None:
            from camcops_server.cc_modules.cc_session import CamcopsSession  # delayed import  # noqa
            self._camcops_session = CamcopsSession.get_session_using_cookies(
                self)
            if DEBUG_CAMCOPS_SESSION:
                log.debug("{!r}", self._camcops_session)
        return self._camcops_session

    def replace_camcops_session(self, ccsession: "CamcopsSession") -> None:
        """
        Replaces any existing
        :class:`camcops_server.cc_modules.cc_session.CamcopsSession` with a new
        one.

        Rationale:

        We may have created a new HTTP session because the request had no
        cookies (added to the DB session but not yet saved), but we might
        then enter the database/tablet upload API and find session details,
        not from the cookies, but from the POST data. At that point, we
        want to replace the session in the Request, without committing the
        first one to disk.
        """
        if self._camcops_session is not None:
            self.dbsession.expunge(self._camcops_session)
        self._camcops_session = ccsession

    def complete_request_add_cookies(self) -> None:
        """
        Finializes the response by adding session cookies.
        We do this late so that we can hot-swap the session if we're using the
        database/tablet API rather than a human web browser.

        Response callbacks are called in the order first-to-most-recently-added.
        See :class:`pyramid.request.CallbackMethodsMixin`.

        That looks like we can add a callback in the process of running a
        callback. And when we add a cookie to a Pyramid session, that sets a
        callback. Let's give it a go...
        """
        # 2019-03-21: If we've not used a CamcopsSession (e.g. for serving
        # a static view), do we care?
        if self._camcops_session is None:
            return

        dbsession = self.dbsession
        dbsession.flush()  # sets the PK for ccsession, if it wasn't set
        # Write the details back to the Pyramid session (will be persisted
        # via the Response automatically):
        pyramid_session = self.session  # type: ISession
        ccsession = self.camcops_session
        pyramid_session[CookieKey.SESSION_ID] = str(ccsession.id)
        pyramid_session[CookieKey.SESSION_TOKEN] = ccsession.token
        # ... should cause the ISession to add a callback to add cookies,
        # which will be called immediately after this one.

    # -------------------------------------------------------------------------
    # Config
    # -------------------------------------------------------------------------

    @reify
    def config_filename(self) -> str:
        """
        Gets the CamCOPS config filename in use.
        """
        return get_config_filename_from_os_env()

    @reify
    def config(self) -> CamcopsConfig:
        """
        Return an instance of
        :class:`camcops_server/cc_modules/cc_config.CamcopsConfig` for the
        request.

        Access it as ``request.config``, with no brackets.
        """
        config = get_config(config_filename=self.config_filename)
        return config

    # -------------------------------------------------------------------------
    # Database
    # -------------------------------------------------------------------------

    @reify
    def engine(self) -> Engine:
        """
        Returns the SQLAlchemy :class:`Engine` for the request.
        """
        cfg = self.config
        return cfg.get_sqla_engine()

    @reify
    def dbsession(self) -> SqlASession:
        """
        Return an SQLAlchemy session for the relevant request.

        The use of ``@reify`` makes this elegant. If and only if a view wants a
        database, it can say

        .. code-block:: python

            dbsession = request.dbsession

        and if it requests that, the cleanup callbacks (COMMIT or ROLLBACK) get
        installed.
        """
        # log.critical("CamcopsRequest.dbsession: caller stack:\n{}",
        #              "\n".join(get_caller_stack_info()))
        _dbsession = self.get_bare_dbsession()

        def end_sqlalchemy_session(req: Request) -> None:
            # noinspection PyProtectedMember
            req._finish_dbsession()

        # - For command-line pseudo-requests, add_finished_callback is no use,
        #   because that's called by the Pyramid routing framework.
        # - So how do we autocommit a command-line session?
        # - Hooking into CamcopsRequest.__del__ did not work: called, yes, but
        #   object state (e.g. newly inserted User objects) went wrong (e.g.
        #   the objects had been blanked somehow, or that's what the INSERT
        #   statements looked like).
        # - Use a context manager instead; see below.
        self.add_finished_callback(end_sqlalchemy_session)

        if DEBUG_DBSESSION_MANAGEMENT:
            log.warning(
                "Returning SQLAlchemy session as CamcopsRequest.dbsession")

        return _dbsession

    def _finish_dbsession(self) -> None:
        """
        A database session has finished. COMMIT or ROLLBACK, depending on how
        things went.
        """
        # Do NOT roll back "if req.exception is not None"; that includes
        # all sorts of exceptions like HTTPFound, HTTPForbidden, etc.
        # See also
        # - https://docs.pylonsproject.org/projects/pyramid_cookbook/en/latest/pylons/exceptions.html  # noqa
        # But they are neatly subclasses of HTTPException, and isinstance()
        # deals with None, so:
        session = self.dbsession
        if (self.exception is not None and
                not isinstance(self.exception, HTTPException)):
            log.critical(
                "Request raised exception that wasn't an HTTPException; "
                "rolling back; exception was: {!r}", self.exception)
            session.rollback()
        else:
            if DEBUG_DBSESSION_MANAGEMENT:
                log.warning("Committing to database")
            session.commit()
            if self._pending_export_push_requests:
                self._process_pending_export_push_requests()
        if DEBUG_DBSESSION_MANAGEMENT:
            log.warning("Closing SQLAlchemy session")
        session.close()

    def get_bare_dbsession(self) -> SqlASession:
        """
        Returns a bare SQLAlchemy session for the request.

        See :func:`dbsession`, the more commonly used wrapper function.
        """
        if self._debugging_db_session:
            log.debug("Request is using debugging SQLAlchemy session")
            return self._debugging_db_session
        if DEBUG_DBSESSION_MANAGEMENT:
            log.warning("Making SQLAlchemy session")
        engine = self.engine
        maker = sessionmaker(bind=engine)
        session = maker()  # type: SqlASession
        return session

    # -------------------------------------------------------------------------
    # TabletSession
    # -------------------------------------------------------------------------

    @reify
    def tabletsession(self) -> TabletSession:
        """
        Request a
        :class:`camcops_server.cc_modules.cc_tabletsession.TabletSession`,
        which is an information structure geared to client (tablet) database
        accesses.

        If we're using this interface, we also want to ensure we're using
        the :class:`camcops_server.cc_modules.cc_session.CamcopsSession` for
        the information provided by the tablet in the POST request, not
        anything already loaded/reset via cookies.
        """
        from camcops_server.cc_modules.cc_session import CamcopsSession  # delayed import  # noqa
        ts = TabletSession(self)
        new_cc_session = CamcopsSession.get_session_for_tablet(ts)
        # ... does login
        self.replace_camcops_session(new_cc_session)
        ts.set_session_id_token(new_cc_session.id, new_cc_session.token)
        if DEBUG_TABLET_SESSION:
            log.debug("CamcopsRequest: {!r}", self)
            log.debug("CamcopsRequest.tabletsession: {!r}", ts)
            log.debug("CamcopsRequest.camcops_session: {!r}",
                      self._camcops_session)
        return ts

    # -------------------------------------------------------------------------
    # Date/time
    # -------------------------------------------------------------------------

    @reify
    def now(self) -> Pendulum:
        """
        Returns the time of the request as an Pendulum object.

        (Reified, so a request only ever has one time.)
        Exposed as a property.
        """
        return Pendulum.now()

    @reify
    def now_utc(self) -> Pendulum:
        """
        Returns the time of the request as a UTC Pendulum.
        """
        p = self.now  # type: Pendulum
        return convert_datetime_to_utc(p)

    @reify
    def now_utc_no_tzinfo(self) -> datetime.datetime:
        """
        Returns the time of the request as a datetime in UTC with no timezone
        information attached. For when you want to compare to something similar
        without getting the error "TypeError: can't compare offset-naive and
        offset-aware datetimes".
        """
        p = self.now  # type: Pendulum
        return pendulum_to_utc_datetime_without_tz(p)

    @reify
    def now_era_format(self) -> str:
        """
        Returns the request time in an ISO-8601 format suitable for use as a
        CamCOPS ``era``.
        """
        return format_datetime(self.now_utc, DateFormat.ERA)

    @property
    def today(self) -> Date:
        """
        Returns today's date.
        """
        # noinspection PyTypeChecker
        return self.now.date()

    # -------------------------------------------------------------------------
    # Logos, static files, and other institution-specific stuff
    # -------------------------------------------------------------------------

    @property
    def url_local_institution(self) -> str:
        """
        Returns the local institution's home URL.
        """
        return self.config.local_institution_url

    @property
    def url_camcops_favicon(self) -> str:
        """
        Returns a URL to the favicon (see
        https://en.wikipedia.org/wiki/Favicon) from within the CamCOPS static
        files.
        """
        # Cope with reverse proxies, etc.
        # https://docs.pylonsproject.org/projects/pyramid/en/latest/api/request.html#pyramid.request.Request.static_url  # noqa
        return self.static_url(STATIC_CAMCOPS_PACKAGE_PATH +
                               "favicon_camcops.png")

    @property
    def url_camcops_logo(self) -> str:
        """
        Returns a URL to the CamCOPS logo from within our static files.
        Returns:

        """
        return self.static_url(STATIC_CAMCOPS_PACKAGE_PATH +
                               "logo_camcops.png")

    @property
    def url_local_logo(self) -> str:
        """
        Returns a URL to the local institution's logo, from somewhere on our
        server.
        """
        return self.static_url(STATIC_CAMCOPS_PACKAGE_PATH + "logo_local.png")

    @property
    def url_camcops_docs(self) -> str:
        """
        Returns the URL to the CamCOPS documentation.
        """
        return DOCUMENTATION_URL

    # -------------------------------------------------------------------------
    # Low-level HTTP information
    # -------------------------------------------------------------------------

    @reify
    def remote_port(self) -> Optional[int]:
        """
        What port number is the client using?

        The ``remote_port`` variable is an optional WSGI extra provided by some
        frameworks, such as mod_wsgi.

        The WSGI spec:
        - https://www.python.org/dev/peps/pep-0333/

        The CGI spec:
        - https://en.wikipedia.org/wiki/Common_Gateway_Interface

        The Pyramid Request object:
        - https://docs.pylonsproject.org/projects/pyramid/en/latest/api/request.html#pyramid.request.Request
        - ... note: that includes ``remote_addr``, but not ``remote_port``.
        """  # noqa
        try:
            return int(self.environ.get("REMOTE_PORT", ""))
        except (TypeError, ValueError):
            return None

    # -------------------------------------------------------------------------
    # HTTP request convenience functions
    # -------------------------------------------------------------------------

    def has_param(self, key: str) -> bool:
        """
        Is the parameter in the request?

        Args:
            key: the parameter's name
        """
        return key in self.params

    def get_str_param(
            self,
            key: str,
            default: str = None,
            lower: bool = False,
            upper: bool = False,
            validator: STRING_VALIDATOR_TYPE = validate_alphanum_underscore) \
            -> Optional[str]:
        """
        Returns an HTTP parameter from the request (GET or POST). If it does
        not exist, or is blank, return ``default``. If it fails the validator,
        raise :exc:`pyramid.httpexceptions.HTTPBadRequest`.

        Args:
            key: the parameter's name
            default: the value to return if the parameter is not found
            lower: convert to lower case?
            upper: convert to upper case?
            validator: validator function

        Returns:
            the parameter's (string) contents, or ``default``

        """
        # HTTP parameters are always strings at heart
        if key not in self.params:  # missing from request?
            return default
        value = self.params.get(key)
        if not value:  # blank, e.g. "source=" in URL?
            return default
        assert isinstance(value, str)  # ... or we wouldn't have got here
        if lower:
            value = value.lower()
        elif upper:
            value = value.upper()
        try:
            validator(value, self)
            return value
        except ValueError as e:
            raise HTTPBadRequest(f"Bad {key!r} parameter: {e}")

    def get_str_list_param(
            self,
            key: str,
            lower: bool = False,
            upper: bool = False,
            validator: STRING_VALIDATOR_TYPE = validate_alphanum_underscore) \
            -> List[str]:
        """
        Returns a list of HTTP parameter values from the request. Ensures all
        have been validated.

        Args:
            key: the parameter's name
            lower: convert to lower case?
            upper: convert to upper case?
            validator: validator function

        Returns:
            a list of string values

        """
        values = self.params.getall(key)
        if lower:
            values = [x.lower() for x in values]
        elif upper:
            values = [x.upper() for x in values]
        try:
            for v in values:
                validator(v, self)
        except ValueError as e:
            raise HTTPBadRequest(
                f"Parameter {key!r} contains a bad value: {e}")
        return values

    def get_int_param(self, key: str, default: int = None) -> Optional[int]:
        """
        Returns an integer parameter from the HTTP request.

        Args:
            key: the parameter's name
            default: the value to return if the parameter is not found or is
                not a valid integer

        Returns:
            an integer, or ``default``

        """
        try:
            return int(self.params[key])
        except (KeyError, TypeError, ValueError):
            return default

    def get_int_list_param(self, key: str) -> List[int]:
        """
        Returns a list of integer parameter values from the HTTP request.

        Args:
            key: the parameter's name

        Returns:
            a list of integer values

        """
        values = self.params.getall(key)
        try:
            return [int(x) for x in values]
        except (KeyError, TypeError, ValueError):
            return []

    def get_bool_param(self, key: str, default: bool) -> bool:
        """
        Returns a boolean parameter from the HTTP request.

        Args:
            key: the parameter's name
            default: the value to return if the parameter is not found or is
                not a valid boolean value

        Returns:
            an integer, or ``default``

        Valid "true" and "false" values (case-insensitive): see
        ``TRUE_STRINGS_LOWER_CASE``, ``FALSE_STRINGS_LOWER_CASE``.
        """
        try:
            param_str = self.params[key].lower()
            if param_str in TRUE_STRINGS_LOWER_CASE:
                return True
            elif param_str in FALSE_STRINGS_LOWER_CASE:
                return False
            else:
                return default
        except (AttributeError, KeyError, TypeError, ValueError):
            return default

    def get_date_param(self, key: str) -> Optional[Date]:
        """
        Returns a date parameter from the HTTP request. If it is missing or
        looks bad, return ``None``.

        Args:
            key: the parameter's name

        Returns:
            a :class:`pendulum.Date`, or ``None``
        """
        try:
            return coerce_to_pendulum_date(self.params[key])
        except (KeyError, ParserError, TypeError, ValueError):
            return None

    def get_datetime_param(self, key: str) -> Optional[Pendulum]:
        """
        Returns a datetime parameter from the HTTP request. If it is missing or
        looks bad, return ``None``.

        Args:
            key: the parameter's name

        Returns:
            a :class:`pendulum.DateTime`, or ``None``
        """
        try:
            return coerce_to_pendulum(self.params[key])
        except (KeyError, ParserError, TypeError, ValueError):
            return None

    def get_redirect_url_param(self,
                               key: str,
                               default: str = None) -> Optional[str]:
        """
        Returns a redirection URL parameter from the HTTP request, validating
        it. (The validation process does not allow all types of URLs!)
        If it was missing, return ``default``. If it was bad, raise
        :exc:`pyramid.httpexceptions.HTTPBadRequest`.

        Args:
            key:
                the parameter's name
            default:
                the value to return if the parameter is not found, or is
                invalid

        Returns:
            a URL string, or ``default``
        """
        return self.get_str_param(key, default=default,
                                  validator=validate_redirect_url)

    # -------------------------------------------------------------------------
    # Routing
    # -------------------------------------------------------------------------

    def route_url_params(self, route_name: str,
                         paramdict: Dict[str, Any]) -> str:
        """
        Provides a simplified interface to :func:`Request.route_url` when you
        have parameters to pass.

        It does two things:

        (1) convert all params to their ``str()`` form;
        (2) allow you to pass parameters more easily using a string
            parameter name.

        The normal Pyramid Request use is:

        .. code-block:: python

            Request.route_url(route_name, param1=value1, param2=value2)

        where "param1" is the literal name of the parameter, but here we can do

        .. code-block:: python

            CamcopsRequest.route_url_params(route_name, {
                PARAM1_NAME: value1_not_necessarily_str,
                PARAM2_NAME: value2
            })

        """
        strparamdict = {k: str(v) for k, v in paramdict.items()}
        return self.route_url(route_name, **strparamdict)

    # -------------------------------------------------------------------------
    # Strings
    # -------------------------------------------------------------------------

    @reify
    def _all_extra_strings(self) -> Dict[str, Dict[str, Dict[str, str]]]:
        """
        Returns all CamCOPS "extra strings" (from XML files) in the format
        used by :func:`camcops_server.cc_string.all_extra_strings_as_dicts`.
        """
        return all_extra_strings_as_dicts(self.config_filename)

    def xstring(self,
                taskname: str,
                stringname: str,
                default: str = None,
                provide_default_if_none: bool = True,
                language: str = None) -> Optional[str]:
        """
        Looks up a string from one of the optional extra XML string files.

        Args:
            taskname: task name (top-level key)
            stringname: string name within task (second-level key)
            default: default to return if the string is not found
            provide_default_if_none: if ``True`` and ``default is None``,
                return a helpful missing-string message in the style
                "string x.y not found"
            language: language code to use, e.g. ``en-GB``; if ``None`` is
                passed, the default behaviour is to look up the current
                language for this request (see :meth:`language`).

        Returns:
            the "extra string"

        """
        # For speed, calculate default only if needed:
        allstrings = self._all_extra_strings
        if taskname in allstrings:
            taskstrings = allstrings[taskname]
            if stringname in taskstrings:
                langversions = taskstrings[stringname]
                if language is None:
                    language = self.language
                if language:  # Specific language requested
                    # 1. Requested language, e.g. "en-GB"
                    if language in langversions:
                        return langversions[language]
                    # 2. Same language, different country, e.g. "en-US"
                    shortlang = language[:2]  # e.g. "en"
                    for key in langversions.keys():
                        if key.startswith(shortlang):
                            return langversions[shortlang]
                # 3. Default language
                if DEFAULT_LOCALE in langversions:
                    return langversions[DEFAULT_LOCALE]
                # 4. Strings with no language specified in the XML
                if MISSING_LOCALE in langversions:
                    return langversions[MISSING_LOCALE]
        # Not found
        if default is None and provide_default_if_none:
            default = (
                f"EXTRA_STRING_NOT_FOUND({taskname}.{stringname}[{language}])"
            )
        return default

    def wxstring(self,
                 taskname: str,
                 stringname: str,
                 default: str = None,
                 provide_default_if_none: bool = True,
                 language: str = None) -> Optional[str]:
        """
        Returns a web-safe version of an :func:`xstring` (q.v.).
        """
        value = self.xstring(taskname, stringname, default,
                             provide_default_if_none=provide_default_if_none,
                             language=language)
        if value is None and not provide_default_if_none:
            return None
        return ws.webify(value)

    def wappstring(self,
                   stringname: str,
                   default: str = None,
                   provide_default_if_none: bool = True,
                   language: str = None) -> Optional[str]:
        """
        Returns a web-safe version of an appstring (an app-wide extra string).
        This uses the XML file shared between the client and the server.
        """
        value = self.xstring(APPSTRING_TASKNAME, stringname, default,
                             provide_default_if_none=provide_default_if_none,
                             language=language)
        if value is None and not provide_default_if_none:
            return None
        return ws.webify(value)

    def get_all_extra_strings(self) -> List[Tuple[str, str, str, str]]:
        """
        Returns all extra strings, as a list of ``task, name, language, value``
        tuples.

        2019-09-16: these are filtered according to the :ref:`RESTRICTED_TASKS
        <RESTRICTED_TASKS>` option.
        """
        restricted_tasks = self.config.restricted_tasks
        user_group_names = None  # type: Optional[Set[str]]

        def task_permitted(task_xml_name: str) -> bool:
            nonlocal user_group_names
            if task_xml_name not in restricted_tasks:
                return True
            if user_group_names is None:
                user_group_names = set(self.user.group_names)
            permitted_group_names = set(restricted_tasks[task_xml_name])
            return bool(permitted_group_names.intersection(user_group_names))

        allstrings = self._all_extra_strings
        rows = []
        for task, taskstrings in allstrings.items():
            if not task_permitted(task):
                log.debug(f"Skipping extra string download for task {task}: "
                          f"not permitted for user {self.user.username}")
                continue
            for name, langversions in taskstrings.items():
                for language, value in langversions.items():
                    rows.append((task, name, language, value))
        return rows

    def task_extrastrings_exist(self, taskname: str) -> bool:
        """
        Has the server been supplied with any extra strings for a specific
        task?
        """
        allstrings = self._all_extra_strings
        return taskname in allstrings

    def extrastring_families(self, sort: bool = True) -> List[str]:
        """
        Which sets of extra strings do we have? A "family" here means, for
        example, "the server itself", "the PHQ9 task", etc.
        """
        families = list(self._all_extra_strings.keys())
        if sort:
            families.sort()
        return families

    @reify
    def language(self) -> str:
        """
        Returns the language code selected by the current user, or if none is
        selected (or the user isn't logged in) the server's default language.

        Returns:
            str: a language code of the form ``en-GB``

        """
        if self.user is not None:
            language = self.user.language
            if language:
                return language
        # Fallback to default
        return self.config.language

    def gettext(self, message: str) -> str:
        """
        Returns a version of ``msg`` translated into the current language.
        This is used for server-only strings.

        The ``gettext()`` function is normally aliased to ``_()`` for
        auto-translation tools to read the souce code.
        """
        lang = self.language
        # We can't work out if the string is missing; gettext falls back to
        # the source message.
        if lang == DEFAULT_LOCALE:
            translated = message
        else:
            try:
                translator = gettext.translation(
                    domain=GETTEXT_DOMAIN,
                    localedir=TRANSLATIONS_DIR,
                    languages=[lang]
                )
                translated = translator.gettext(message)
            except OSError:  # e.g. translation file not found
                log.warning(f"Failed to find translation files for {lang}")
                translated = message
        if DEBUG_GETTEXT:
            return f"[{message}→{lang}→{translated}]"
        else:
            return translated

    def wgettext(self, message: str) -> str:
        """
        A web-safe version of :func:`gettext`.
        """
        return ws.webify(self.gettext(message))

    def sstring(self, which_string: SS) -> str:
        """
        Returns a translated server string via a lookup mechanism.

        Args:
            which_string:
                which string? A :class:`camcops_server.cc_modules.cc_text.SS`
                enumeration value

        Returns:
            str: the string

        """
        try:
            result = self._cached_sstring[which_string]
        except KeyError:
            result = server_string(self, which_string)
            self._cached_sstring[which_string] = result
        return result

    def wsstring(self, which_string: SS) -> str:
        """
        Returns a web-safe version of a translated server string via a lookup
        mechanism.

        Args:
            which_string:
                which string? A :class:`camcops_server.cc_modules.cc_text.SS`
                enumeration value

        Returns:
            str: the string

        """
        return ws.webify(self.sstring(which_string))

    # -------------------------------------------------------------------------
    # PNG versus SVG output, so tasks don't have to care (for e.g. PDF/web)
    # -------------------------------------------------------------------------

    def prepare_for_pdf_figures(self) -> None:
        """
        Switch the server (for this request) to producing figures in a format
        most suitable for PDF.
        """
        if CSS_PAGED_MEDIA:
            # unlikely -- we use wkhtmltopdf instead now
            self.switch_output_to_png()
            # ... even weasyprint's SVG handling is inadequate
        else:
            # This is the main method -- we use wkhtmltopdf these days
            self.switch_output_to_svg(provide_png_fallback=False)
            # ... wkhtmltopdf can cope with SVGs

    def prepare_for_html_figures(self) -> None:
        """
        Switch the server (for this request) to producing figures in a format
        most suitable for HTML.
        """
        self.switch_output_to_svg()

    def switch_output_to_png(self) -> None:
        """
        Switch server (for this request) to producing figures in PNG format.
        """
        self.use_svg = False

    def switch_output_to_svg(self, provide_png_fallback: bool = True) -> None:
        """
        Switch server (for this request) to producing figures in SVG format.

        Args:
            provide_png_fallback:
                Offer a PNG fallback option/
        """
        self.use_svg = True
        self.provide_png_fallback_for_svg = provide_png_fallback

    @staticmethod
    def create_figure(**kwargs) -> Figure:
        """
        Creates and returns a :class:`matplotlib.figure.Figure` with a canvas.
        The canvas will be available as ``fig.canvas``.
        """
        fig = Figure(**kwargs)
        # noinspection PyUnusedLocal
        canvas = FigureCanvas(fig)  # noqa: F841
        # The canvas will be now available as fig.canvas, since
        # FigureCanvasBase.__init__ calls fig.set_canvas(self); similarly, the
        # figure is available from the canvas as canvas.figure

        # How do we set the font, so the caller doesn't have to?
        # The "nasty global" way is:
        #       matplotlib.rc('font', **fontdict)
        #       matplotlib.rc('legend', **fontdict)
        # or similar. Then matplotlib often works its way round to using its
        # global rcParams object, which is Not OK in a multithreaded context.
        #
        # https://github.com/matplotlib/matplotlib/issues/6514
        # https://github.com/matplotlib/matplotlib/issues/6518
        #
        # The other way is to specify a fontdict with each call, e.g.
        #       ax.set_xlabel("some label", **fontdict)
        # https://stackoverflow.com/questions/21321670/how-to-change-fonts-in-matplotlib-python  # noqa
        # Relevant calls with explicit "fontdict: Dict" parameters:
        #       ax.set_xlabel(..., fontdict=XXX, ...)
        #       ax.set_ylabel(..., fontdict=XXX, ...)
        #       ax.set_xticklabels(..., fontdict=XXX, ...)
        #       ax.set_yticklabels(..., fontdict=XXX, ...)
        #       ax.text(..., fontdict=XXX, ...)
        #       ax.set_label_text(..., fontdict=XXX, ...)
        #       ax.set_title(..., fontdict=XXX, ...)
        #
        # And with "fontproperties: FontProperties"
        #       sig.suptitle(..., fontproperties=XXX, ...)
        #
        # And with "prop: FontProperties":
        #       ax.legend(..., prop=XXX, ...)
        #
        # Then, some things are automatically plotted...

        return fig

    @reify
    def fontdict(self) -> Dict[str, Any]:
        """
        Returns a font dictionary for use with Matplotlib plotting.

        **matplotlib font handling and fontdict parameter**

        - https://stackoverflow.com/questions/3899980
        - https://matplotlib.org/users/customizing.html
        - matplotlib/font_manager.py

          - Note that the default TrueType font is "DejaVu Sans"; see
            :class:`matplotlib.font_manager.FontManager`

        - Example sequence:

          - CamCOPS does e.g. ``ax.set_xlabel("Date/time",
            fontdict=self.req.fontdict)``

          - matplotlib.axes.Axes.set_xlabel:
            https://matplotlib.org/api/_as_gen/matplotlib.axes.Axes.set_xlabel.html

            - matplotlib.axes.Axes.text documentation, explaining the fontdict
              parameter:
              https://matplotlib.org/api/_as_gen/matplotlib.axes.Axes.text.html

          - What's created is probably a :class:`matplotlib.text.Text` object,
            whose ``update()`` function is called with the dictionary. Via its
            superclass :class:`matplotlib.artist.Artist` and its ``update()``
            function, this sets attributes on the Text object. Ultimately,
            without having explored this in too much depth, it's probably the
            ``self._fontproperties`` object of Text that holds this info.

          - That is an instance of
            :class:`matplotlib.font_manager.FontProperties`.

        **Linux fonts**

        Anyway, the main things are (1) that the relevant fonts need to be
        installed, and (2) that the default is DejaVu Sans.

        - Linux fonts are installed in ``/usr/share/fonts``, and TrueType fonts
          within ``/usr/share/fonts/truetype``.

        - Use ``fc-match`` to see the font mappings being used.

        - Use ``fc-list`` to list available fonts.

        - Use ``fc-cache`` to rebuild the font cache.

        - Files in ``/etc/fonts/conf.avail/`` do some thinking.

        **Problems with pixellated fonts in PDFs made via wkhtmltopdf**

        - See also https://github.com/wkhtmltopdf/wkhtmltopdf/issues/2193,
          about pixellated fonts via wkhtmltopdf (which was our problem for a
          subset of the fonts in trackers, on 2020-06-28, using wkhtmltopd
          0.12.5 with patched Qt).

        - When you get pixellated fonts in a PDF, look also at the embedded
          font list in the PDF (e.g. in Okular: File -> Properties -> Fonts).

        - Matplotlib helpfully puts the text (rendered as lines in SVG) as
          comments.

        - As a debugging sequence, we can manually trim the "pdfhtml" output
          down to just the SVG file. Still has problems. Yet there's no text
          in it; the text is made of pure SVG lines. And Chrome renders it
          perfectly. As does Firefox.

        - The rendering bug goes away entirely if you delete the opacity
          styling throughout the SVG:

          .. code-block:: none

            <g style="opacity:0.5;" transform=...>
               ^^^^^^^^^^^^^^^^^^^^
               this

        - So, simple fix:

          - rather than opacity (alpha) 0.5 and on top...

          - 50% grey colour and on the bottom.

        """  # noqa
        fontsize = self.config.plot_fontsize
        return dict(
            family='sans-serif',
            # ... serif, sans-serif, cursive, fantasy, monospace
            style='normal',  # normal (roman), italic, oblique
            variant='normal',  # normal, small-caps
            weight='normal',
            # ... normal [=400], bold [=700], bolder [relative to current],
            # lighter [relative], 100, 200, 300, ..., 900
            size=fontsize  # in pt (default 12)
        )

    @reify
    def fontprops(self) -> FontProperties:
        """
        Return a :class:`matplotlib.font_manager.FontProperties` object for
        use with Matplotlib plotting.
        """
        return FontProperties(**self.fontdict)

    def set_figure_font_sizes(self,
                              ax: "Axes",  # "SubplotBase",
                              fontdict: Dict[str, Any] = None,
                              x_ticklabels: bool = True,
                              y_ticklabels: bool = True) -> None:
        """
        Sets font sizes for the axes of the specified Matplotlib figure.

        Args:
            ax: the figure to modify
            fontdict: the font dictionary to use (if omitted, the default
                will be used)
            x_ticklabels: if ``True``, modify the X-axis tick labels
            y_ticklabels: if ``True``, modify the Y-axis tick labels
        """
        final_fontdict = self.fontdict.copy()
        if fontdict:
            final_fontdict.update(fontdict)
        fp = FontProperties(**final_fontdict)

        axes = []  # type: List[Axis]
        if x_ticklabels:  # and hasattr(ax, "xaxis"):
            axes.append(ax.xaxis)
        if y_ticklabels:  # and hasattr(ax, "yaxis"):
            axes.append(ax.yaxis)
        for axis in axes:
            for ticklabel in axis.get_ticklabels(which='both'):  # type: Text  # I think!  # noqa
                ticklabel.set_fontproperties(fp)

    def get_html_from_pyplot_figure(self, fig: Figure) -> str:
        """
        Make HTML (as PNG or SVG) from pyplot
        :class:`matplotlib.figure.Figure`.
        """
        if USE_SVG_IN_HTML and self.use_svg:
            result = svg_html_from_pyplot_figure(fig)
            if self.provide_png_fallback_for_svg:
                # return both an SVG and a PNG image, for browsers that can't
                # deal with SVG; the Javascript header will sort this out
                # http://www.voormedia.nl/blog/2012/10/displaying-and-detecting-support-for-svg-images  # noqa
                result += png_img_html_from_pyplot_figure(
                    fig, PlotDefaults.DEFAULT_PLOT_DPI, "pngfallback")
            return result
        else:
            return png_img_html_from_pyplot_figure(
                fig, PlotDefaults.DEFAULT_PLOT_DPI)

    # -------------------------------------------------------------------------
    # Convenience functions for user information
    # -------------------------------------------------------------------------

    @property
    def user(self) -> Optional["User"]:
        """
        Returns the :class:`camcops_server.cc_modules.cc_user.User` for the
        current request.
        """
        return self._debugging_user or self.camcops_session.user

    @property
    def user_id(self) -> Optional[int]:
        """
        Returns the integer user ID for the current request.
        """
        if self._debugging_user:
            return self._debugging_user.id
        return self.camcops_session.user_id

    # -------------------------------------------------------------------------
    # ID number definitions
    # -------------------------------------------------------------------------

    @reify
    def idnum_definitions(self) -> List[IdNumDefinition]:
        """
        Returns all
        :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition` objects.
        """
        return get_idnum_definitions(self.dbsession)  # no longer cached

    @reify
    def valid_which_idnums(self) -> List[int]:
        """
        Returns the ``which_idnum`` values for all ID number definitions.
        """
        return [iddef.which_idnum for iddef in self.idnum_definitions]
        # ... pre-sorted

    def get_idnum_definition(self,
                             which_idnum: int) -> Optional[IdNumDefinition]:
        """
        Retrieves an
        :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition` for the
        specified ``which_idnum`` value.
        """
        return next((iddef for iddef in self.idnum_definitions
                     if iddef.which_idnum == which_idnum), None)

    def get_id_desc(self, which_idnum: int,
                    default: str = None) -> Optional[str]:
        """
        Get the server's ID description for the specified ``which_idnum``
        value.
        """
        return next((iddef.description for iddef in self.idnum_definitions
                     if iddef.which_idnum == which_idnum),
                    default)

    def get_id_shortdesc(self, which_idnum: int,
                         default: str = None) -> Optional[str]:
        """
        Get the server's short ID description for the specified ``which_idnum``
        value.
        """
        return next((iddef.short_description
                     for iddef in self.idnum_definitions
                     if iddef.which_idnum == which_idnum),
                    default)

    def is_idnum_valid(self, which_idnum: int,
                       idnum_value: Optional[int]) -> bool:
        """
        Does the ID number pass any extended validation checks?

        Args:
            which_idnum: which ID number type is this?
            idnum_value: ID number value

        Returns:
            bool: valid?
        """
        idnumdef = self.get_idnum_definition(which_idnum)
        if not idnumdef:
            return False
        valid, _ = validate_id_number(self,
                                      idnum_value, idnumdef.validation_method)
        return valid

    def why_idnum_invalid(self, which_idnum: int,
                          idnum_value: Optional[int]) -> str:
        """
        Why does the ID number fail any extended validation checks?

        Args:
            which_idnum: which ID number type is this?
            idnum_value: ID number value

        Returns:
            str: why invalid? (Human-readable string.)
        """
        idnumdef = self.get_idnum_definition(which_idnum)
        if not idnumdef:
            _ = self.gettext
            return _("Can't fetch ID number definition")
        _, why = validate_id_number(self,
                                    idnum_value, idnumdef.validation_method)
        return why

    # -------------------------------------------------------------------------
    # Server settings
    # -------------------------------------------------------------------------

    @reify
    def server_settings(self) -> ServerSettings:
        """
        Return the
        :class:`camcops_server.cc_modules.cc_serversettings.ServerSettings` for
        the server.
        """
        return get_server_settings(self)

    @reify
    def database_title(self) -> str:
        """
        Return the database friendly title for the server.
        """
        ss = self.server_settings
        return ss.database_title or ""

    def set_database_title(self, title: str) -> None:
        """
        Sets the database friendly title for the server.
        """
        ss = self.server_settings
        ss.database_title = title

    # -------------------------------------------------------------------------
    # SNOMED-CT
    # -------------------------------------------------------------------------

    @reify
    def snomed_supported(self) -> bool:
        """
        Is SNOMED-CT supported for CamCOPS tasks?
        """
        return bool(self.config.get_task_snomed_concepts())

    def snomed(self, lookup: str) -> "SnomedConcept":
        """
        Fetches a SNOMED-CT concept for a CamCOPS task.

        Args:
            lookup: a CamCOPS SNOMED lookup string

        Returns:
            a :class:`camcops_server.cc_modules.cc_snomed.SnomedConcept`

        Raises:
            :exc:`KeyError`, if the lookup cannot be found (e.g. UK data not
                installed)
        """
        concepts = self.config.get_task_snomed_concepts()
        assert concepts, "No SNOMED-CT data available for CamCOPS tasks"
        return concepts[lookup]

    @reify
    def icd9cm_snomed_supported(self) -> bool:
        """
        Is SNOMED-CT supported for ICD-9-CM codes?
        """
        return bool(self.config.get_icd9cm_snomed_concepts())

    def icd9cm_snomed(self, code: str) -> List["SnomedConcept"]:
        """
        Fetches a SNOMED-CT concept for an ICD-9-CM code

        Args:
            code: an ICD-9-CM code

        Returns:
            a :class:`camcops_server.cc_modules.cc_snomed.SnomedConcept`

        Raises:
            :exc:`KeyError`, if the lookup cannot be found (e.g. data not
                installed)
        """
        concepts = self.config.get_icd9cm_snomed_concepts()
        assert concepts, "No SNOMED-CT data available for ICD-9-CM"
        return concepts[code]

    @reify
    def icd10_snomed_supported(self) -> bool:
        """
        Is SNOMED-CT supported for ICD-10 codes?
        """
        return bool(self.config.get_icd9cm_snomed_concepts())

    def icd10_snomed(self, code: str) -> List["SnomedConcept"]:
        """
        Fetches a SNOMED-CT concept for an ICD-10 code

        Args:
            code: an ICD-10 code

        Returns:
            a :class:`camcops_server.cc_modules.cc_snomed.SnomedConcept`

        Raises:
            :exc:`KeyError`, if the lookup cannot be found (e.g. data not
                installed)
        """
        concepts = self.config.get_icd10_snomed_concepts()
        assert concepts, "No SNOMED-CT data available for ICD-10"
        return concepts[code]

    # -------------------------------------------------------------------------
    # Export recipients
    # -------------------------------------------------------------------------

    def get_export_recipients(self,
                              recipient_names: List[str] = None,
                              all_recipients: bool = False,
                              all_push_recipients: bool = False,
                              save: bool = True,
                              database_versions: bool = True) \
            -> List[Union["ExportRecipient", "ExportRecipientInfo"]]:
        """
        Returns a list of export recipients, with some filtering if desired.
        Validates them against the database.

        - If ``all_recipients``, return all.
        - Otherwise, if ``all_push_recipients``, return all "push" recipients.
        - Otherwise, return all named in ``recipient_names``.

          - If any are invalid, raise an error.
          - If any are duplicate, raise an error.

        Args:
            all_recipients: use all recipients?
            all_push_recipients: use all "push" recipients?
            recipient_names: recipient names
            save: save any freshly created recipient records to the DB?
            database_versions: return ExportRecipient objects that are attached
                to a database session (rather than ExportRecipientInfo objects
                that aren't)?

        Returns:
            list: of :class:`camcops_server.cc_modules.cc_exportrecipient.ExportRecipient`

        Raises:
            - :exc:`ValueError` if a name is invalid
            - :exc:`ValueError` if a name is duplicated
            - :exc:`camcops_server.cc_modules.cc_exportrecipient.InvalidExportRecipient`
              if an export recipient configuration is invalid
        """  # noqa
        # Delayed imports
        from camcops_server.cc_modules.cc_exportrecipient import \
            ExportRecipient  # delayed import  # noqa

        # Check parameters
        recipient_names = recipient_names or []  # type: List[str]
        if save and not database_versions:
            raise AssertionError("Can't save unless taking database versions")

        # Start with ExportRecipientInfo objects:
        recipientinfolist = self.config.get_all_export_recipient_info()

        # Restrict
        if not all_recipients:
            if all_push_recipients:
                recipientinfolist = [r for r in recipientinfolist if r.push]
            else:
                # Specified by name
                duplicates = [name for name, count in
                              collections.Counter(recipient_names).items()
                              if count > 1]
                if duplicates:
                    raise ValueError(f"Duplicate export recipients "
                                     f"specified: {duplicates!r}")
                valid_names = set(r.recipient_name for r in recipientinfolist)
                bad_names = [name for name in recipient_names
                             if name not in valid_names]
                if bad_names:
                    raise ValueError(
                        f"Bad export recipients specified: {bad_names!r}. "
                        f"Valid recipients are: {valid_names!r}")
                recipientinfolist = [r for r in recipientinfolist
                                     if r.recipient_name in recipient_names]

        # Complete validation
        for r in recipientinfolist:
            r.validate(self)

        # Does the caller want them as ExportRecipientInfo objects
        if not database_versions:
            return recipientinfolist

        # Convert to SQLAlchemy ORM ExportRecipient objects:
        recipients = [ExportRecipient(x) for x in recipientinfolist]  # type: List[ExportRecipient]  # noqa

        final_recipients = []  # type: List[ExportRecipient]
        dbsession = self.dbsession

        def process_final_recipients(_save: bool) -> None:
            for r in recipients:
                other = ExportRecipient.get_existing_matching_recipient(
                    dbsession, r)
                if other:
                    # This other one matches, and is already in the database.
                    # Use it. But first...
                    for attrname in ExportRecipient.NEEDS_RECOPYING_EACH_TIME_FROM_CONFIG_ATTRNAMES:  # noqa
                        setattr(other, attrname, getattr(r, attrname))
                    # OK.
                    final_recipients.append(other)
                else:
                    # Our new object doesn't match. Use (+/- save) it.
                    if save:
                        log.debug(
                            "Creating new ExportRecipient record in database")
                        dbsession.add(r)
                    r.current = True
                    final_recipients.append(r)

        if save:
            lockfilename = self.config.get_master_export_recipient_lockfilename()  # noqa
            with lockfile.FileLock(lockfilename, timeout=None):  # waits forever if necessary  # noqa
                process_final_recipients(_save=True)
        else:
            process_final_recipients(_save=False)

        # OK
        return final_recipients

    def get_export_recipient(self,
                             recipient_name: str,
                             save: bool = True) -> "ExportRecipient":
        """
        Returns a single validated export recipient, given its name.

        Args:
            recipient_name: recipient name
            save: save any freshly created recipient records to the DB?

        Returns:
            list: of :class:`camcops_server.cc_modules.cc_exportrecipient.ExportRecipient`

        Raises:
            - :exc:`ValueError` if a name is invalid
            - :exc:`camcops_server.cc_modules.cc_exportrecipient.InvalidExportRecipient`
              if an export recipient configuration is invalid
        """  # noqa
        recipients = self.get_export_recipients([recipient_name], save=save)
        assert len(recipients) == 1
        return recipients[0]

    @reify
    def all_push_recipients(self) -> List["ExportRecipient"]:
        """
        Cached for speed (will potentially be called for multiple tables in
        a bulk upload).
        """
        return self.get_export_recipients(
            all_push_recipients=True,
            save=False,
            database_versions=True,  # we need group ID info somehow
        )

    def add_export_push_request(self,
                                recipient_name: str,
                                basetable: str,
                                task_pk: int) -> None:
        """
        Adds a request to push a task to an export recipient.

        The reason we use this slightly convoluted approach is because
        otherwise, it's very easy to generate a backend request for a new task
        before it's actually been committed (so the backend finds no task).

        Args:
            recipient_name: name of the recipient
            basetable: name of the task's base table
            task_pk: server PK of the task
        """
        self._pending_export_push_requests.append(
            (recipient_name, basetable, task_pk)
        )

    def _process_pending_export_push_requests(self) -> None:
        """
        Sends pending export push requests to the backend.

        Called after the COMMIT.
        """
        from camcops_server.cc_modules.celery import export_task_backend  # delayed import  # noqa

        for recipient_name, basetable, task_pk in self._pending_export_push_requests:  # noqa
            export_task_backend.delay(
                recipient_name=recipient_name,
                basetable=basetable,
                task_pk=task_pk
            )

    # -------------------------------------------------------------------------
    # User downloads
    # -------------------------------------------------------------------------

    @property
    def user_download_dir(self) -> str:
        """
        The directory in which this user's downloads should be/are stored, or a
        blank string if user downloads are not available. Also ensures it
        exists.
        """
        if self.config.user_download_max_space_mb <= 0:
            return ""
        basedir = self.config.user_download_dir
        if not basedir:
            return ""
        user_id = self.user_id
        if user_id is None:
            return ""
        userdir = os.path.join(basedir, str(user_id))
        mkdir_p(userdir)
        return userdir

    @property
    def user_download_bytes_permitted(self) -> int:
        """
        Amount of space the user is permitted.
        """
        if not self.user_download_dir:
            return 0
        return self.config.user_download_max_space_mb * 1024 * 1024

    @reify
    def user_download_bytes_used(self) -> int:
        """
        Returns the disk space used by this user.
        """
        download_dir = self.user_download_dir
        if not download_dir:
            return 0
        return get_directory_contents_size(download_dir)

    @property
    def user_download_bytes_available(self) -> int:
        """
        Returns the available space for this user in their download area.
        """
        permitted = self.user_download_bytes_permitted
        used = self.user_download_bytes_used
        available = permitted - used
        return available

    @property
    def user_download_lifetime_duration(self) -> Duration:
        """
        Returns the lifetime of user download objects.
        """
        return Duration(minutes=self.config.user_download_file_lifetime_min)


# noinspection PyUnusedLocal
def complete_request_add_cookies(req: CamcopsRequest,
                                 response: Response) -> None:
    """
    Finializes the response by adding session cookies.

    See :meth:`CamcopsRequest.complete_request_add_cookies`.
    """
    req.complete_request_add_cookies()


# =============================================================================
# Configurator
# =============================================================================

@contextmanager
def camcops_pyramid_configurator_context(
        debug_toolbar: bool = False,
        static_cache_duration_s: int = 0) -> Configurator:
    """
    Context manager to create a Pyramid configuration context, for making
    (for example) a WSGI server or a debugging request. That means setting up
    things like:

    - the authentication and authorization policies
    - our request and session factories
    - our Mako renderer
    - our routes and views

    Args:
        debug_toolbar:
            Add the Pyramid debug toolbar?
        static_cache_duration_s:
            Lifetime (in seconds) for the HTTP cache-control setting for
            static content.

    Returns:
        a :class:`Configurator` object

    Note this includes settings that transcend the config file.

    Most things should be in the config file. This enables us to run multiple
    configs (e.g. multiple CamCOPS databases) through the same process.
    However, some things we need to know right now, to make the WSGI app.
    Here, OS environment variables and command-line switches are appropriate.
    """

    # -------------------------------------------------------------------------
    # 1. Base app
    # -------------------------------------------------------------------------
    settings = {  # Settings that can't be set directly?
        'debug_authorization': DEBUG_AUTHORIZATION,
        # ... see https://docs.pylonsproject.org/projects/pyramid/en/latest/narr/security.html#debugging-view-authorization-failures  # noqa
    }
    with Configurator(settings=settings) as config:
        # ---------------------------------------------------------------------
        # Authentication; authorizaion (permissions)
        # ---------------------------------------------------------------------
        authentication_policy = CamcopsAuthenticationPolicy()
        config.set_authentication_policy(authentication_policy)
        # Let's not use ACLAuthorizationPolicy, which checks an access control
        # list for a resource hierarchy of objects, but instead:
        authorization_policy = CamcopsAuthorizationPolicy()
        config.set_authorization_policy(authorization_policy)
        config.set_default_permission(Permission.HAPPY)
        # ... applies to all SUBSEQUENT view configuration registrations

        # ---------------------------------------------------------------------
        # Factories
        # ---------------------------------------------------------------------
        config.set_request_factory(CamcopsRequest)
        # ... for request attributes: config, database, etc.
        config.set_session_factory(get_session_factory())
        # ... for request.session
        config.set_response_factory(camcops_response_factory)

        # ---------------------------------------------------------------------
        # Renderers
        # ---------------------------------------------------------------------
        camcops_add_mako_renderer(config, extension='.mako')

        # deform_bootstrap.includeme(config)

        # ---------------------------------------------------------------------
        # Routes and accompanying views
        # ---------------------------------------------------------------------

        # Add static views
        # https://docs.pylonsproject.org/projects/pyramid/en/latest/narr/assets.html#serving-static-assets  # noqa
        # Hmm. We cannot fail to set up a static file route, because otherwise
        # we can't provide URLs to them.
        static_filepath = STATIC_CAMCOPS_PACKAGE_PATH
        static_name = RouteCollection.STATIC.route
        log.debug("... including static files from {!r} at Pyramid static "
                  "name {!r}", static_filepath, static_name)
        # ... does the name needs to start with "/" or the pattern "static/"
        # will override the later "deform_static"? Not sure.

        # We were doing this:
        #       config.add_static_view(name=static_name, path=static_filepath)
        # But now we need to (a) add the
        # "cache_max_age=static_cache_duration_s" argument, and (b) set the
        # HTTP header 'Cache-Control: no-cache="Set-Cookie, Set-Cookie2"',
        # for the ZAP penetration tester:
        # ... https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html#web-content-caching  # noqa
        # We can do the former, but not the latter, via add_static_view(),
        # because it sends its keyword arguments to add_route(), not the view
        # creation. So, alternatives ways...
        # - from https://github.com/Pylons/pyramid/issues/1486
        # - and https://stackoverflow.com/questions/24854300/
        # - to https://github.com/Pylons/pyramid/pull/2021
        # - to https://docs.pylonsproject.org/projects/pyramid/en/latest/narr/hooks.html#view-derivers  # noqa

        config.add_static_view(name=static_name,
                               path=static_filepath,
                               cache_max_age=static_cache_duration_s)

        # Add all the routes:
        for pr in RouteCollection.all_routes():
            if DEBUG_ADD_ROUTES:
                log.info("{} -> {}", pr.route, pr.path)
            config.add_route(pr.route, pr.path)
        # See also:
        # https://stackoverflow.com/questions/19184612/how-to-ensure-urls-generated-by-pyramids-route-url-and-route-path-are-valid  # noqa

        # Routes added EARLIER have priority. So add this AFTER our custom
        # bugfix:
        config.add_static_view(name='/deform_static',
                               path='deform:static/',
                               cache_max_age=static_cache_duration_s)

        # Most views are using @view_config() which calls add_view().
        # Scan for @view_config decorators, to map views to routes:
        # https://docs.pylonsproject.org/projects/venusian/en/latest/api.html
        config.scan("camcops_server.cc_modules")

        # ---------------------------------------------------------------------
        # Add tweens (inner to outer)
        # ---------------------------------------------------------------------
        # We will use implicit positioning:
        # - https://www.slideshare.net/aconrad/alex-conrad-pyramid-tweens-ploneconf-2011  # noqa

        # config.add_tween('camcops_server.camcops.http_session_tween_factory')

        # ---------------------------------------------------------------------
        # Debug toolbar
        # ---------------------------------------------------------------------
        if debug_toolbar:
            log.debug("Enabling Pyramid debug toolbar")
            config.include('pyramid_debugtoolbar')  # BEWARE! SIDE EFFECTS
            # ... Will trigger an import that hooks events into all
            # SQLAlchemy queries. There's a bug somewhere relating to that;
            # see notes below relating to the "mergedb" function.
            config.add_route(RouteCollection.DEBUG_TOOLBAR.route,
                             RouteCollection.DEBUG_TOOLBAR.path)

        yield config


# =============================================================================
# Debugging requests
# =============================================================================

def make_post_body_from_dict(d: Dict[str, str],
                             encoding: str = "utf8") -> bytes:
    """
    Makes an HTTP POST body from a dictionary.

    For debugging HTTP requests.

    It mimics how the tablet operates.
    """
    # https://docs.pylonsproject.org/projects/pyramid-cookbook/en/latest/testing/testing_post_curl.html  # noqa
    txt = urllib.parse.urlencode(query=d)
    # ... this encoding mimics how the tablet operates
    body = txt.encode(encoding)
    return body


class CamcopsDummyRequest(CamcopsRequest, DummyRequest):
    """
    Request class that allows manual manipulation of GET/POST parameters
    for debugging.

    Notes:

    - The important base class is :class:`webob.request.BaseRequest`.
    - ``self.params`` is a :class:`NestedMultiDict` (see
      ``webob/multidict.py``); these are intrinsically read-only.
    - ``self.params`` is also a read-only property. When read, it combines
      data from ``self.GET`` and ``self.POST``.
    - What we do here is to manipulate the underlying GET/POST data.

    """
    _CACHE_KEY = "webob._parsed_query_vars"
    _QUERY_STRING_KEY = "QUERY_STRING"

    # def __init__(self, *args, **kwargs) -> None:
    #     super().__init__(*args, **kwargs)
    #     # Just a technique worth noting:
    #     #
    #     # self._original_params_property = CamcopsRequest.params  # type: property  # noqa
    #     # self._original_params = self._original_params_property.fget(self)  # type: NestedMultiDict  # noqa
    #     # self._fake_params = self._original_params.copy()  # type: MultiDict
    #     # if params:
    #     #     self._fake_params.update(params)
    #
    # @property
    # def params(self):
    #     log.debug(repr(self._fake_params))
    #     return self._fake_params
    #     # Returning the member object allows clients to call
    #     #       dummyreq.params.update(...)
    #
    # @params.setter
    # def params(self, value):
    #     self._fake_params = value

    def set_method_get(self) -> None:
        """
        Sets the fictional request method to GET.
        """
        self.method = RequestMethod.GET

    def set_method_post(self) -> None:
        """
        Sets the fictional request method to POST.
        """
        self.method = RequestMethod.POST

    def clear_get_params(self) -> None:
        """
        Clear all GET parameters.
        """
        env = self.environ
        if self._CACHE_KEY in env:
            del env[self._CACHE_KEY]
        env[self._QUERY_STRING_KEY] = ""

    def add_get_params(self, d: Dict[str, str],
                       set_method_get: bool = True) -> None:
        """
        Add GET parameters.

        Args:
            d: dictionary of ``{parameter: value}`` pairs.
            set_method_get: also set the request's method to GET?
        """
        if not d:
            return
        # webob.request.BaseRequest.GET reads from self.environ['QUERY_STRING']
        paramdict = self.GET.copy()  # type: MultiDict
        paramdict.update(d)
        env = self.environ
        # Delete the cached version.
        if self._CACHE_KEY in env:
            del env[self._CACHE_KEY]
        # Write the new version
        env[self._QUERY_STRING_KEY] = urllib.parse.urlencode(query=paramdict)
        if set_method_get:
            self.set_method_get()

    def set_get_params(self, d: Dict[str, str],
                       set_method_get: bool = True) -> None:
        """
        Clear any GET parameters, and then set them to new values.
        See :func:`add_get_params`.
        """
        self.clear_get_params()
        self.add_get_params(d, set_method_get=set_method_get)

    def set_post_body(self, body: bytes,
                      set_method_post: bool = True) -> None:
        """
        Sets the fake POST body.

        Args:
            body: the body to set
            set_method_post: also set the request's method to POST?
        """
        log.debug("Applying fake POST body: {!r}", body)
        self.body = body
        self.content_length = len(body)
        if set_method_post:
            self.set_method_post()

    def fake_request_post_from_dict(self,
                                    d: Dict[str, str],
                                    encoding: str = "utf8",
                                    set_method_post: bool = True) -> None:
        """
        Sets the request's POST body according to a dictionary.

        Args:
            d: dictionary of ``{parameter: value}`` pairs.
            encoding: character encoding to use
            set_method_post: also set the request's method to POST?
        """
        # webob.request.BaseRequest.POST reads from 'body' (indirectly).
        body = make_post_body_from_dict(d, encoding=encoding)
        self.set_post_body(body, set_method_post=set_method_post)


_ = """
# A demonstration of the manipulation of superclass properties:

class Test(object):
    def __init__(self):
        self.a = 3

    @property
    def b(self):
        return 4


class Derived(Test):
    def __init__(self):
        super().__init__()
        self._superclass_b = super().b
        self._b = 4

    @property
    def b(self):
        print("Superclass b: {}".format(self._superclass_b.fget(self)))
        print("Self _b: {}".format(self._b))
        return self._b
    @b.setter
    def b(self, value):
        self._b = value


x = Test()
x.a  # 3
x.a = 5
x.a  # 5
x.b  # 4
x.b = 6  # can't set attribute

y = Derived()
y.a  # 3
y.a = 5
y.a  # 5
y.b  # 4
y.b = 6
y.b  # 6

"""


def get_core_debugging_request() -> CamcopsDummyRequest:
    """
    Returns a basic :class:`CamcopsDummyRequest`.
    """
    with camcops_pyramid_configurator_context(debug_toolbar=False) as pyr_cfg:
        req = CamcopsDummyRequest(
            environ={
                ENVVAR_CONFIG_FILE: "nonexistent_camcops_config_file.nonexistent",  # noqa
                WsgiEnvVar.PATH_INFO: '/',
                WsgiEnvVar.SCRIPT_NAME: '',
                WsgiEnvVar.SERVER_NAME: '127.0.0.1',
                WsgiEnvVar.SERVER_PORT: '8000',
                WsgiEnvVar.WSGI_URL_SCHEME: 'http',
            }
        )
        # ... must pass an actual dict to the "environ" parameter; os.environ
        # itself isn't OK ("TypeError: WSGI environ must be a dict; you passed
        # environ({'key1': 'value1', ...})

        req.registry = pyr_cfg.registry
        pyr_cfg.begin(request=req)
        return req


def get_command_line_request(user_id: int = None) -> CamcopsRequest:
    """
    Creates a dummy CamcopsRequest for use on the command line.
    By default, it does so for the system user. Optionally, you can specify a
    user by their ID number.

    - Presupposes that ``os.environ[ENVVAR_CONFIG_FILE]`` has been set, as it
      is in :func:`camcops_server.camcops.main`.

    **WARNING:** this does not provide a COMMIT/ROLLBACK context. If you use
    this directly, you must manage that yourself. Consider using
    :func:`command_line_request_context` instead.
    """
    log.debug(f"Creating command-line pseudo-request (user_id={user_id})")
    req = get_core_debugging_request()

    # If we proceed with an out-of-date database, we will have problems, and
    # those problems may not be immediately apparent, which is bad. So:
    req.config.assert_database_ok()

    # Ensure we have a user
    if user_id is None:
        req._debugging_user = User.get_system_user(req.dbsession)
    else:
        req._debugging_user = User.get_user_by_id(
            req.dbsession, user_id)

    return req


@contextmanager
def command_line_request_context(user_id: int = None) \
        -> Generator[CamcopsRequest, None, None]:
    """
    Request objects are ubiquitous, and allow code to refer to the HTTP
    request, config, HTTP session, database session, and so on. Here we make
    a special sort of request for use from the command line, and provide it
    as a context manager that will COMMIT the database afterwards (because the
    normal method, via the Pyramid router, is unavailable).
    """
    req = get_command_line_request(user_id=user_id)
    yield req
    # noinspection PyProtectedMember
    req._finish_dbsession()


def get_unittest_request(dbsession: SqlASession,
                         params: Dict[str, Any] = None) -> CamcopsDummyRequest:
    """
    Creates a :class:`CamcopsDummyRequest` for use by unit tests.

    - Points to an existing database (e.g. SQLite in-memory database).
    - Presupposes that ``os.environ[ENVVAR_CONFIG_FILE]`` has been set, as it
      is in :func:`camcops_server.camcops.main`.
    """
    log.debug("Creating unit testing pseudo-request")
    req = get_core_debugging_request()
    req.set_get_params(params)

    req._debugging_db_session = dbsession
    user = User()
    user.superuser = True
    req._debugging_user = user

    return req
