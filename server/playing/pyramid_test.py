#!/usr/bin/env python

"""
playing/pyramid_test.py

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

**Test the Pyramid web framework.**

pip install arrow==0.10.0
pip install dogpile.cache==0.6.4
pip install mysqlclient==1.3.10  # for mysql+mysqldb://...
pip install pyramid==1.9.1
pip install pyramid_debugtoolbar==4.3
pip install sqlalchemy==1.1.11
# NOT THESE:
# pip install pyramid_beaker==0.8
- Pyramid likes Beaker as its cache, which is another Mike Bayer product.

- Note that cached functions take no parameters in the programmatic API, but
  can have (non-keyword) parameters in the better decorator API:
  - https://beaker.readthedocs.io/en/latest/caching.html#decorator-api
  - https://stackoverflow.com/questions/5050110/how-do-i-use-beaker-caching-in-pyramid

- What we want to replicate is the cached config. So, the main parameter is the
  filename. However, we want all sorts of functions to say "get me the config"
  without further ado.

- We want to be able to fire up the CamCOPS server with the usual hierarchy
  of
    command-line parameter
    environment variable
    default (not applicable here?)

  So a simple way is to write the environment variable.
  Changes to the environment will not contaminate parent processes.

- This all works beautifully.
- What wouldn't be so obvious is using the config to define the cache
  mechanism. Still, 'memory' is fine for now.

- But the next problem is lock_dir, which Beaker wants (even for memory
  caching), and that is pretty hard to define at runtime, because Beaker seems
  to like the sequence
    1. CACHE_OPTS = {'cache.type': 'memory', 'lock_dir': 'XXX'}
    2. cache = CacheManager(**parse_cache_config_options(CACHE_OPTS))
    3. @cache.cache('some_name')
       def somefunc():
           # ...
  and the decorators, which are called very early, therefore need the cache,
  and I can't see a way to set/change the lock_dir parameter later.

- Moreover, even Mike Bayer suggests moving on from Beaker, including for
  reasons related to the lock files:
  http://techspot.zzzeek.org/2012/04/19/using-beaker-for-caching-why-you-ll-want-to-switch-to-dogpile.cache/

===============================================================================
2. Requests and URL routing
===============================================================================

- Fairly easy.
- A bit of additional URL-placeholder validation added.
- Didn't use the @view_config decorator; this has some limitations as to how
  the scan process works.
  - https://docs.pylonsproject.org/projects/pyramid/en/latest/api/view.html
  - https://stackoverflow.com/questions/30871765/create-dynamic-class-views-in-pyramid
- Mind you, not too many. Let's use it.
  It can also support the 'renderer' argument.
- Not path at the same time, though. See also:
  https://github.com/Pylons/pyramid/issues/2852
  Ultimately, @view_config() calls Configurator.add_view().

===============================================================================
3. SQLAlchemy sessions and models
===============================================================================

- https://docs.pylonsproject.org/projects/pyramid-cookbook/en/latest/database/sqlalchemy.html
- OK, this works really well, and sessions aren't created when they're not
  needed, but are cleaned up after when they are used.

===============================================================================
4. HTTP sessions
===============================================================================

- Pyramid options include
    - build-in
    - PyNaCl
    - Redis
    - Beaker
- Some of these store information on the client side and use digital signatures
  to prevent tampering, e.g.
    https://beaker.readthedocs.io/en/latest/sessions.html
- We adopt a rather more hard-line approach, of storing almost no information
  client-side. All that's sent to the client is (a) an integer ID, which is
  arbitrary except that it's also the server-side PK, and (b) a token, which is
  generated randomly. We ensure network security by enforcing HTTPS, and we
  make the cookie expire at the end of the session.
- All we really need from Pyramid is the ability to set/read cookies, and
  ideally to encapsulate the session request in the framework so it's always
  accessible to a view's Response object.
- Beaker does that via SessionMiddleware.
- We don't want middleware, I think:
  http://dirtsimple.org/2007/02/wsgi-middleware-considered-harmful.html
- OK, achieved nicely with a tween. Except that doesn't give us the session
  object within the request (Pyramid's request.session).
- Could also use:
    Pyramid NewRequest event
    Pyramid's built-in session framework
- Here's the order:
    https://groups.google.com/forum/#!topic/pylons-discuss/Mos6mZu-S4g
- Pyramid sessions are created lazily; see Request.session, which is a function
  with a @reify decorator ("cache me"); and also
  https://stackoverflow.com/questions/15643798/pyramid-sessions-and-static-assets
  ... in other words, Pyramid thinks of sessions as storage (which may not
  always be needed), and this question also ties in with that of
  authentication.
- OK. We can set up a Pyramid session that we never use, and a real one.
- Seems fine to add an object to the request.
- We can also embed our info within a Pyramid session, allowing us use of its
  CSRF facilities later. Let's do that.

===============================================================================
5. Authentication (how/whether to integrate with Pyramid's methods)
===============================================================================

===============================================================================
6. Alembic
===============================================================================

"""  # noqa

# =============================================================================
# Imports
# =============================================================================

import argparse
from contextlib import contextmanager
import datetime
import enum
import html
import logging
import os
import re
import sys
import traceback
from typing import Callable, Dict, List, Tuple

import arrow
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.randomness import create_base64encoded_randomness
from cardinal_pythonlib.reprfunc import auto_repr
from dogpile.cache import make_region
from pyramid.config import Configurator
from pyramid.registry import Registry
from pyramid.response import Response
from pyramid.request import Request
from pyramid.router import Router
from pyramid.session import ISession, SignedCookieSessionFactory
from pyramid.view import view_config
from sqlalchemy.engine import create_engine
from sqlalchemy.engine.base import Engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
from sqlalchemy.orm import Session as SqlASession
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import DateTime, Integer, String
from wsgiref.simple_server import make_server

log = BraceStyleAdapter(logging.getLogger(__name__))

# =============================================================================
# Constants
# =============================================================================

CONFIG_FILE_ENVVAR = "CAMCOPS_TEST_ENV_VAR_CONFIGFILE"

TASKNAME_1 = 'camcops'
TASKNAME_2 = 'phq9'
STRINGNAME_1 = 'string1'
STRINGNAME_2 = 'string2'


# =============================================================================
# Globals
# =============================================================================

static_cache_region = make_region()
# Could also have a dynamic cache for pages.

Base = declarative_base()

IPAddressColType = String(length=45)  # http://stackoverflow.com/questions/166132  # noqa
SessionTokenColType = String(length=50)


# =============================================================================
# Helpers
# =============================================================================

RE_VALID_REPLACEMENT_MARKER = re.compile("^[a-zA-Z_][a-zA-Z0-9_]*$")
# All characters must be a-z, A-Z, _, or 0-9.
# First character must not be a digit.
# https://docs.pylonsproject.org/projects/pyramid/en/latest/narr/urldispatch.html#route-pattern-syntax  # noqa


def valid_replacement_marker(marker: str) -> bool:
    return RE_VALID_REPLACEMENT_MARKER.match(marker) is not None


class UrlParamType(enum.Enum):
    STRING = 1
    POSITIVE_INTEGER = 2
    PLAIN_STRING = 3


class UrlParam(object):
    def __init__(self, name: str,
                 paramtype: UrlParamType == UrlParamType.PLAIN_STRING) -> None:
        self.name = name
        self.paramtype = paramtype
        assert valid_replacement_marker(name)

    def regex(self) -> str:
        if self.paramtype == UrlParamType.STRING:
            return ''
        elif self.paramtype == UrlParamType.POSITIVE_INTEGER:
            return r'\d+'  # digits
        elif self.paramtype == UrlParamType.PLAIN_STRING:
            return r'[a-zA-Z0-9_]+'

    def markerdef(self) -> str:
        marker = self.name
        r = self.regex()
        if r:
            marker += ':' + r
        return '{' + marker + '}'


def make_url_path(base: str, *args: UrlParam) -> str:
    assert valid_replacement_marker(base)
    parts = [base] + [arg.markerdef() for arg in args]
    return "/" + "/".join(parts)


# =============================================================================
# Routes
# =============================================================================

# Class to collect constants together
# See also http://xion.io/post/code/python-enums-are-ok.html
class ViewParams(object):
    """
    Used as parameter placeholders in URLs, and fetched from the matchdict.
    """
    PK = 'pk'
    PATIENT_ID = 'pid'
    QUERY = '_query'  # built in to Pyramid


class QueryParams(object):
    """
    Parameters for the request.GET dictionary, and in URL as '...?key=value'
    """
    SORT = 'sort'


COOKIE_NAME = 'camcops'


class CookieKeys:
    SESSION_ID = 'session_id'
    SESSION_TOKEN = 'session_token'


class RoutePath(object):
    """
    - Pyramid route names are just strings used internally for convenience.
    - Pyramid URL paths are URL fragments, like '/thing', and can contain
      placeholders, like '/thing/{bork_id}', which will result in the
      request.matchdict object containing a 'bork_id' key. Those can be
      further constrained by regular expressions, like '/thing/{bork_id:\d+}'
      to restrict to digits.
    """
    def __init__(self, route: str, path: str) -> None:
        self.route = route
        self.path = path


class Routes(object):
    DEBUG_TOOLBAR = RoutePath('debug_toolbar', '/_debug_toolbar/')  # hard-coded path  # noqa
    HOME = RoutePath('home', '/')
    OTHER = RoutePath('other', '/other')
    VIEW_WITH_PARAMS = RoutePath(
        'vwp',
        make_url_path(
            'vwp',
            UrlParam(ViewParams.PATIENT_ID, UrlParamType.POSITIVE_INTEGER),
            UrlParam(ViewParams.PK, UrlParamType.POSITIVE_INTEGER)
        )
    )

    @classmethod
    def all_routes(cls) -> List[RoutePath]:
        return [v for k, v in cls.__dict__.items()
                if not (k.startswith('_') or
                        k == 'all_routes' or
                        k == 'DEBUG_TOOLBAR')]


# =============================================================================
# Config and its caching
# =============================================================================

class DummyConfig(object):
    def __init__(self, filename: str) -> None:
        log.info("Pretending to load config from {}. Expensive; "
                 "SHOULD ONLY BE CALLED ONCE.".format(repr(filename)))
        self.filename = filename
        self.xstring_filename = "some_extra_strings.xml"
        self.something = 42
        self.use_debug_toolbar = True
        self.dburl = "mysql+mysqldb://scott:tiger@127.0.0.1:3306/test_pyramid?charset=utf8"  # noqa
        self.secure_cookies = False  # DANGER! Change for any production
        self.session_expiry_duration = datetime.timedelta(minutes=10)
        self.echo_sql = True
        self.session_cookie_secret = 'xyz'

    def __repr__(self) -> str:
        return auto_repr(self)

    def create_engine(self) -> Engine:
        return create_engine(self.dburl)
        # Don't use "echo=self.echo_sql"; things are logged twice. Set the log
        # level of the 'sqlalchemy.engine' logger; see main().


@static_cache_region.cache_on_arguments()
def get_config():  # -> DummyConfig:  # https://bitbucket.org/zzzeek/dogpile.cache/issues/96/error-in-python-35-with-use-of-deprecated  # noqa
    filename = os.environ.get(CONFIG_FILE_ENVVAR)
    return DummyConfig(filename)


# =============================================================================
# Strings and their caching
# =============================================================================

@static_cache_region.cache_on_arguments()
def get_extra_strings():  # -> Dict[Tuple[str, str], str]:  # https://bitbucket.org/zzzeek/dogpile.cache/issues/96/error-in-python-35-with-use-of-deprecated  # noqa
    cfg = get_config()  # type: DummyConfig
    log.info("Expensive string-loading function; SHOULD ONLY BE CALLED ONCE; "
             "pretending to read file {}.".format(cfg.xstring_filename))
    xstringdict = {}  # type: Dict[Tuple[str, str] -> str]
    for task in [TASKNAME_1, TASKNAME_2]:
        for stringname in [STRINGNAME_1, STRINGNAME_2]:
            task_string_pair = (task, stringname)
            xstringdict[task_string_pair] = "{}.{}".format(task, stringname)
    return xstringdict


def xstring(task: str, stringname: str, default: str = None) -> str:
    xstringdict = get_extra_strings()  # type: Dict[Tuple[str, str], str]
    task_string_pair = (task, stringname)
    return xstringdict.get(task_string_pair, default)


# =============================================================================
# Supposedly interesting things that tax config/strings
# =============================================================================

def do_something() -> None:
    cfg = get_config()  # type: DummyConfig
    log.info("Config filename: {!r}", cfg.filename)
    log.info("Extra string {}.{}: {}",
             TASKNAME_1, STRINGNAME_1, xstring(TASKNAME_1, STRINGNAME_1))


def do_something_else() -> None:
    cfg = get_config()  # type: DummyConfig
    log.info("Config something: {!r}", cfg.something)
    log.info("Extra string {}.{}: {}",
             TASKNAME_2, STRINGNAME_2, xstring(TASKNAME_2, STRINGNAME_2))


# =============================================================================
# HTTP views
# =============================================================================

def html_a(url: str, text: str) -> str:
    return "<a href='{}'>{}</a>".format(url, text)


@view_config(route_name=Routes.HOME.route)
def home_view(request: Request) -> Response:
    cfg = get_config()  # type: DummyConfig
    task = TASKNAME_1
    stringname = STRINGNAME_2
    lines = [
        "Hello, world!",
        "Config filename {cfgfile}, {task}.{stringname} = {xstring}".format(
            cfgfile=repr(cfg.filename),
            task=task,
            stringname=stringname,
            xstring=xstring(task, stringname, default="??"),
        ),
        "View {}?".format(
            html_a(request.route_url(Routes.OTHER.route), "other"),
        ),
    ]
    if cfg.use_debug_toolbar:
        lines.append("View {}?".format(
            html_a(request.route_path(Routes.DEBUG_TOOLBAR.route),
                   "debug toolbar"),
        ))
    for pk in range(5):
        for pid in range(5):
            kwargs = {ViewParams.PK: pk,
                      ViewParams.PATIENT_ID: pid}
            if pid > 3:
                kwargs[ViewParams.QUERY] = {QueryParams.SORT: 'asc'}
            lines.append("View with parameters, " + html_a(
                request.route_url(Routes.VIEW_WITH_PARAMS.route, **kwargs),
                "vwp_{}_{}".format(pk, pid)))
    return Response("<br>".join(lines))


@view_config(route_name=Routes.OTHER.route)
def other_view(request: Request) -> Response:
    dbsession = request.dbsession
    lines = [
        "All well. Go <a href='{url_home}'>home</a>?".format(
            url_home=request.route_url(Routes.HOME.route),
        ),
        "request.session (Pyramid): {}".format(repr(request.session)),
        "request.camcops_session: {}".format(repr(request.camcops_session)),
        "request.camcops_session.id: {}".format(
            repr(request.camcops_session.id)),
        "request.camcops_session.token: {}".format(
            repr(request.camcops_session.token)),
    ]
    sql = "DESCRIBE information_schema.columns"
    result = dbsession.execute(sql)
    lines.append("Result of SQL " + repr(sql) + ":")
    lines.append(html.escape(repr(result)))
    for row in result:
        lines.append(html.escape(repr(row)))
    return Response("<br>".join(lines))


@view_config(route_name=Routes.VIEW_WITH_PARAMS.route)
def view_with_params(request: Request) -> Response:
    pk = int(request.matchdict[ViewParams.PK])
    patient_id = int(request.matchdict[ViewParams.PATIENT_ID])
    get = request.GET
    return Response("View for pk={}, patient_id={}, request.GET={}".format(
        pk, patient_id, repr(get)))


# =============================================================================
# Database stuff
# =============================================================================

def dbsession_request_method(request: Request) -> SqlASession:
    """
    This is very elegant. The function gets plumbed in to the Request object
    by:
        config.add_request_method(dbsession, reify=True).
    when the WSGI app is being made. Then, if and only if a view wants a
    database, it can say
        dbsession = request.dbsession
    and if it requests that, the cleanup callbacks get installed.
    """
    maker = request.registry.dbmaker
    log.info("Making SQLAlchemy session")
    dbsession = maker()  # type: SqlASession

    def end_sqlalchemy_session(req: Request) -> None:
        if req.exception is not None:
            dbsession.rollback()
        else:
            dbsession.commit()
        log.info("Closing SQLAlchemy session")
        dbsession.close()

    request.add_finished_callback(end_sqlalchemy_session)

    return dbsession


# =============================================================================
# Other per-request stuff
# =============================================================================

# noinspection PyUnusedLocal
def now_arrow_request_method(request: Request) -> arrow.Arrow:
    return arrow.now()


def now_utc_request_method(request: Request) -> datetime.datetime:
    a = request.now_arrow  # type: arrow.Arrow
    return a.to('utc').datetime


# =============================================================================
# HTTP sessions via a tween
# =============================================================================

def generate_token(num_bytes: int = 16) -> str:
    """
    Make a new session token. Doesn't matter if another session is using it.
    """
    # http://stackoverflow.com/questions/817882/unique-session-id-in-python
    return create_base64encoded_randomness(num_bytes)


class CamcopsHttpSession(Base):
    __tablename__ = "_pretend_security_webviewer_sessions"
    id = Column(
        "id", Integer,
        primary_key=True, autoincrement=True, index=True,
        doc="Session ID (internal number for insertion speed)")
    token = Column(
        "token", SessionTokenColType,
        # ... not unique, for speed (slows down updates markedly)
        doc="Token (base 64 encoded random number)")
    ip_address = Column(
        "ip_address", IPAddressColType,
        doc="IP address of user")
    user_id = Column(
        "user_id", Integer,
        doc="User ID")
    last_activity_utc = Column(
        "last_activity_utc", DateTime,
        doc="Date/time of last activity (UTC)")

    def __init__(self,
                 ip_addr: str,
                 last_activity_utc: datetime.datetime):
        self.token = generate_token()
        self.ip_address = ip_addr
        self.last_activity_utc = last_activity_utc

    @classmethod
    def get_http_session(cls, request: Request) -> 'CamcopsHttpSession':
        dbsession = request.dbsession
        pyramid_session = request.session  # type: ISession
        try:
            # noinspection PyArgumentList
            session_id = int(pyramid_session.get(CookieKeys.SESSION_ID, None))
        except (TypeError, ValueError):
            session_id = None
        # noinspection PyArgumentList
        session_token = pyramid_session.get(CookieKeys.SESSION_TOKEN, '')
        ip_addr = request.remote_addr
        now = request.now_utc  # type: datetime.datetime
        if session_id and session_token:
            cfg = get_config()  # type: DummyConfig
            oldest_last_activity_allowed = now - cfg.session_expiry_duration
            candidate = dbsession.query(cls).\
                filter(cls.id == session_id).\
                filter(cls.token == session_token).\
                filter(cls.ip_address == ip_addr).\
                filter(cls.last_activity_utc >= oldest_last_activity_allowed).\
                first()  # type: CamcopsHttpSession
            if candidate is None:
                log.debug("Session not found in database")
        else:
            log.debug("Session ID and/or session token is missing.")
            candidate = None
        found = candidate is not None
        if found:
            candidate.last_activity_utc = now
            return candidate
        else:
            log.debug("Creating new session")
            new_http_session = cls(ip_addr=ip_addr, last_activity_utc=now)
            dbsession.add(new_http_session)
            dbsession.flush()  # sets the PK for new_http_session
            # Write the details back to the Pyramid session (will be persisted
            # via the Response automatically):
            pyramid_session[CookieKeys.SESSION_ID] = str(new_http_session.id)
            pyramid_session[CookieKeys.SESSION_TOKEN] = new_http_session.token
            return new_http_session


# noinspection PyUnusedLocal
def http_session_tween_factory(
        handler: Callable[[Request], Response],
        registry: Registry) -> Callable[[Request], Response]:
    cfg = get_config()  # type: DummyConfig

    def http_session_tween(request: Request) -> Response:
        log.debug("Starting http_session_tween")
        request.camcops_session = CamcopsHttpSession.get_http_session(request)
        response = handler(request)
        log.debug("Ending http_session_tween")
        return response

    return http_session_tween


def get_session_factory() -> SignedCookieSessionFactory:
    cfg = get_config()  # type: DummyConfig
    return SignedCookieSessionFactory(
        secret=cfg.session_cookie_secret,
        hashalg='sha512',  # the default
        salt='camcops_pyramid_session.',
        cookie_name=COOKIE_NAME,
        max_age=None,  # browser scope; session cookie
        path='/',  # the default
        domain=None,  # the default
        secure=cfg.secure_cookies,
        httponly=cfg.secure_cookies,
        timeout=None,  # we handle timeouts at the database level instead
        reissue_time=0,  # default; reissue cookie at every request
        set_on_exception=True,  # the default; cookie even if exception raised
        serializer=None,  # the default; use pyramid.session.PickleSerializer
    )
    # As max_age and expires are left at their default of None, these are
    # session cookies.


# =============================================================================
# WSGI server
# =============================================================================

def make_wsgi_app() -> Router:
    # -------------------------------------------------------------------------
    # 1. Base app
    # -------------------------------------------------------------------------
    cfg = get_config()  # type: DummyConfig
    with Configurator() as config:
        # ---------------------------------------------------------------------
        # Database
        # ---------------------------------------------------------------------
        engine = cfg.create_engine()
        config.registry.dbmaker = sessionmaker(bind=engine)
        config.add_request_method(
            callable=dbsession_request_method, name='dbsession', reify=True)
        # ... makes request.dbsession available, and caches it (reify) so even
        #     if we ask for the dbsession several times, we get the same thing
        # ... https://docs.pylonsproject.org/projects/pyramid/en/latest/api/request.html#pyramid.request.Request.set_property  # noqa

        # ---------------------------------------------------------------------
        # Other stuff to be cached, if requested, for the lifetime of the req
        # ---------------------------------------------------------------------
        config.add_request_method(
            callable=now_arrow_request_method, name='now_arrow', reify=True)
        config.add_request_method(
            callable=now_utc_request_method, name='now_utc', reify=True)

        # ---------------------------------------------------------------------
        # Routes and accompanying views
        # ---------------------------------------------------------------------
        # Most views are using @view_config() which calls add_view().
        config.scan()
        for pr in Routes.all_routes():
            config.add_route(pr.route, pr.path)

        # See also:
        # https://stackoverflow.com/questions/19184612/how-to-ensure-urls-generated-by-pyramids-route-url-and-route-path-are-valid  # noqa

        # ---------------------------------------------------------------------
        # Add tweens (inner to outer)
        # ---------------------------------------------------------------------
        # We will use implicit positioning:
        # - https://www.slideshare.net/aconrad/alex-conrad-pyramid-tweens-ploneconf-2011  # noqa

        config.add_tween('__main__.http_session_tween_factory')
        config.set_session_factory(get_session_factory())

        # ---------------------------------------------------------------------
        # Debug toolbar
        # ---------------------------------------------------------------------
        if cfg.use_debug_toolbar:
            config.include('pyramid_debugtoolbar')
            config.add_route(Routes.DEBUG_TOOLBAR.route,
                             Routes.DEBUG_TOOLBAR.path)

        # ---------------------------------------------------------------------
        # Make app
        # ---------------------------------------------------------------------
        app = config.make_wsgi_app()

    # -------------------------------------------------------------------------
    # 2. Middleware above the Pyramid level
    # -------------------------------------------------------------------------
    # ...

    # -------------------------------------------------------------------------
    # 3. Done
    # -------------------------------------------------------------------------
    return app


def serve() -> None:
    app = make_wsgi_app()
    host = '0.0.0.0'
    port = 8000
    server = make_server(host, port, app)
    server.serve_forever()


# =============================================================================
# Command-line work
# =============================================================================

@contextmanager
def session_context():
    cfg = get_config()  # type: DummyConfig
    engine = cfg.create_engine()
    maker = sessionmaker(bind=engine)
    dbsession = maker()  # type: SqlASession
    # noinspection PyUnusedLocal,PyBroadException
    try:
        yield dbsession
        log.debug("Command-line database session: committing")
        dbsession.commit()
    except Exception as e:
        log.warning("Exception:\n" + traceback.format_exc())
        log.warning("Command-line database session: rolling back")
        dbsession.rollback()
    finally:
        log.debug("Command-line database session: closing")
        dbsession.close()


def make_tables() -> None:
    with session_context() as session:  # type: SqlASession
        engine = session.get_bind()  # type: Engine
        # ... get_bind() returns an Engine (or None) unless explicitly bound
        #     to a Connection; see its definition
        Base.metadata.create_all(engine)


# =============================================================================
# main
# =============================================================================

def main() -> None:
    # -------------------------------------------------------------------------
    # Set up logging
    # -------------------------------------------------------------------------
    logging.basicConfig(level=logging.DEBUG)

    # -------------------------------------------------------------------------
    # Handle command-line arguments
    # -------------------------------------------------------------------------
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--configfile',
        default=os.environ.get(CONFIG_FILE_ENVVAR, None),
        help="Specify the CamCOPS configuration file. If this is not "
             "specified on the command line, the environment variable {} is "
             "examined.".format(CONFIG_FILE_ENVVAR))
    parser.add_argument(
        '--serve', action='store_true',
        help="Run a WSGI server"
    )
    parser.add_argument(
        '--maketables', action='store_true',
        help="Make database tables and stop"
    )
    cmdargs = parser.parse_args()

    assert cmdargs.configfile, (
        "Must specify a configuration file, either via the command line or "
        "through the {} environment variable".format(CONFIG_FILE_ENVVAR))
    os.environ[CONFIG_FILE_ENVVAR] = cmdargs.configfile

    # -------------------------------------------------------------------------
    # Configure cache(s)
    # -------------------------------------------------------------------------
    static_cache_region.configure(
        backend='dogpile.cache.memory'
        # Consider later: memcached via dogpile.cache.pylibmc
    )

    # -------------------------------------------------------------------------
    # Do application-related things
    # -------------------------------------------------------------------------

    cfg = get_config()  # caches config, to improve speed of first request
    # ... but we'll use it, too:
    if cfg.echo_sql:
        logging.getLogger('sqlalchemy.engine').setLevel(logging.INFO)

    if cmdargs.maketables:
        make_tables()
        sys.exit(0)

    do_something()
    do_something_else()
    if cmdargs.serve:
        serve()

    sys.exit(0)


if __name__ == '__main__':
    main()
