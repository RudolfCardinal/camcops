#!/usr/bin/env python

"""
camcops_server/camcops_server_core.py

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

**Main functions used by camcops_server.py**

We split these off, because the imports can be very slow, and we want a rapid
command-line response for simple commands.

Importing this module does the following:

- ensure that all models are loaded;
- provide log message around some of the slow imports.

"""

# SET UP LOGGING BEFORE WE IMPORT CAMCOPS MODULES, allowing them to log during
# imports (see e.g. cc_plot).
# Currently sets up colour logging even if under WSGI environment. This is fine
# for gunicorn from the command line; I'm less clear about whether the disk
# logs look polluted by ANSI codes; needs checking.
import logging
from cardinal_pythonlib.logs import BraceStyleAdapter
log = BraceStyleAdapter(logging.getLogger(__name__))
log.info("Imports starting")

# Main imports

import os  # noqa: E402
import platform  # noqa: E402
import sys  # noqa: E402
import subprocess  # noqa: E402
from typing import Any, Dict, List, Optional, TYPE_CHECKING  # noqa: E402

import cherrypy  # noqa: E402
try:
    from gunicorn.app.base import BaseApplication
except ImportError:
    BaseApplication = None  # e.g. on Windows: "ImportError: no module named 'fcntl'".  # noqa: E501
from wsgiref.simple_server import make_server  # noqa: E402

from cardinal_pythonlib.fileops import mkdir_p  # noqa: E402
from cardinal_pythonlib.process import nice_call  # noqa: E402
from cardinal_pythonlib.ui_commandline import ask_user, ask_user_password  # noqa: E402,E501
from cardinal_pythonlib.wsgi.request_logging_mw import RequestLoggingMiddleware  # noqa: E402,E501
from cardinal_pythonlib.wsgi.reverse_proxied_mw import (  # noqa: E402
    ReverseProxiedConfig,
    ReverseProxiedMiddleware,
)

# Import this one early:
# noinspection PyUnresolvedReferences
import camcops_server.cc_modules.cc_all_models  # import side effects (ensure all models registered)  # noqa: E402,E501

from camcops_server.cc_modules.cc_anon import (  # noqa: E402
    write_crate_data_dictionary,
    write_cris_data_dictionary,
)
# noinspection PyUnresolvedReferences
import camcops_server.cc_modules.client_api  # import side effects (register unit test)  # noqa: E402,E501,F401
from camcops_server.cc_modules.cc_config import (  # noqa: E402
    CamcopsConfig,
    get_config_filename_from_os_env,
    get_default_config_from_os_env,
)
from camcops_server.cc_modules.cc_constants import (  # noqa: E402
    ConfigDefaults,
    DEFAULT_FLOWER_ADDRESS,
    DEFAULT_FLOWER_PORT,
    USER_NAME_FOR_SYSTEM,
)
from camcops_server.cc_modules.cc_exception import raise_runtime_error  # noqa: E402,E501
from camcops_server.cc_modules.cc_export import (  # noqa: E402
    print_export_queue,
    export,
)
from camcops_server.cc_modules.cc_pyramid import RouteCollection  # noqa: E402
from camcops_server.cc_modules.cc_request import (  # noqa: E402
    CamcopsRequest,
    command_line_request_context,
    camcops_pyramid_configurator_context,
)
from camcops_server.cc_modules.cc_string import all_extra_strings_as_dicts  # noqa: E402,E501
from camcops_server.cc_modules.cc_task import Task  # noqa: E402
from camcops_server.cc_modules.cc_taskindex import (  # noqa: E402
    check_indexes,
    reindex_everything,
)
# noinspection PyUnresolvedReferences
from camcops_server.cc_modules.cc_user import (  # noqa: E402
    SecurityLoginFailure,
    set_password_directly,
    User,
)
from camcops_server.cc_modules.cc_validators import validate_new_password  # noqa: E402,E501
from camcops_server.cc_modules.celery import (  # noqa: E402
    CELERY_APP_NAME,
    CELERY_SOFT_TIME_LIMIT_SEC,
)
log.info("Imports complete")
log.info("Using {} tasks", len(Task.all_subclasses_by_tablename()))

if TYPE_CHECKING:
    from pyramid.router import Router  # noqa: F401
    from camcops_server.cc_modules.cc_exportrecipientinfo import ExportRecipientInfo  # noqa: E501,F401

# =============================================================================
# Other constants
# =============================================================================

WINDOWS = platform.system() == "Windows"

# We want to be able to run Celery from our virtual environment, but just
# running the venv Python (as opposed to using "activate") doesn't set the path
# correctly. So as per
# https://stackoverflow.com/questions/22003769/get-virtualenvs-bin-folder-path-from-script  # noqa: E501
_CELERY_NAME = "celery.exe" if WINDOWS else "celery"
CELERY = os.path.join(os.path.dirname(sys.executable), _CELERY_NAME)

DEFAULT_CLEANUP_TIMEOUT_S = 10.0


# =============================================================================
# Helper functions for web server launcher
# =============================================================================

def ensure_database_is_ok() -> None:
    """
    Opens a link to the database and checks it's of the correct version
    (or otherwise raises an assertion error).
    """
    config = get_default_config_from_os_env()
    config.assert_database_ok()


def ensure_directories_exist() -> None:
    config = get_default_config_from_os_env()
    mkdir_p(config.export_lockdir)
    if config.user_download_dir:
        mkdir_p(config.user_download_dir)


def join_url_fragments(*fragments: str) -> str:
    """
    Combines fragments to make a URL.

    (``urllib.parse.urljoin`` doesn't do what we want.)
    """
    newfrags = [f[1:] if f.startswith("/") else f for f in fragments]
    return "/".join(newfrags)


def precache() -> None:
    """
    Populates the major caches.
    """
    log.info("Prepopulating caches")
    config_filename = get_config_filename_from_os_env()
    config = get_default_config_from_os_env()
    _ = all_extra_strings_as_dicts(config_filename)
    _ = config.get_task_snomed_concepts()
    _ = config.get_icd9cm_snomed_concepts()
    _ = config.get_icd10_snomed_concepts()
    with command_line_request_context() as req:
        _ = req.get_export_recipients(all_recipients=True)


# =============================================================================
# WSGI entry point
# =============================================================================

def make_wsgi_app(debug_toolbar: bool = False,
                  reverse_proxied_config: ReverseProxiedConfig = None,
                  debug_reverse_proxy: bool = False,
                  show_requests: bool = True,
                  show_request_immediately: bool = True,
                  show_response: bool = True,
                  show_timing: bool = True,
                  static_cache_duration_s: int = 0) -> "Router":
    """
    Makes and returns a WSGI application, attaching all our special methods.

    Args:
        debug_toolbar:
            Add the Pyramid debug toolbar?
        reverse_proxied_config:
            An optional
            :class:`cardinal_pythonlib.wsgi.reverse_proxied_mw.ReverseProxiedConfig`
            object giving details about a reverse proxy configuration
            (or details that there isn't one)
        debug_reverse_proxy:
            Show debugging information about the reverse proxy middleware, if
            such middleware is required?
        show_requests:
            Write incoming requests to the Python log?
        show_request_immediately:
            [Applicable if ``show_requests``]
            Show the request immediately, so it's written to the log before the
            WSGI app does its processing, and is guaranteed to be visible even
            if the WSGI app hangs? The only reason to use ``False`` is probably
            if you intend to show response and/or timing information and you
            want to minimize the number of lines written to the log; in this
            case, only a single line is written to the log (after the wrapped
            WSGI app has finished processing).
        show_response:
            [Applicable if ``show_requests``]
            Show the HTTP response code?
        show_timing:
            [Applicable if ``show_requests``]
            Show the time that the wrapped WSGI app took?
        static_cache_duration_s:
            Lifetime (in seconds) for the HTTP cache-control setting for
            static content.

    Returns:
        the WSGI app

    QUESTION: how do we access the WSGI environment (passed to the WSGI app)
    from within a Pyramid request?

    ANSWER:

    .. code-block:: none

        Configurator.make_wsgi_app() calls Router.__init__()
        and returns: app = Router(...)
        The WSGI framework uses: response = app(environ, start_response)
        which therefore calls: Router.__call__(environ, start_response)
        which does:
              response = self.execution_policy(environ, self)
              return response(environ, start_response)
        So something LIKE this will be called:
              Router.default_execution_policy(environ, router)
                  with router.request_context(environ) as request:
                      # ...
        So the environ is handled by Router.request_context(environ)
        which will call BaseRequest.__init__()
        which does:
              d = self.__dict__
              d['environ'] = environ
        so we should be able to use
              request.environ  # type: Dict[str, str]

    """
    log.debug("Creating WSGI app")

    # Make Pyramid WSGI app
    # - camcops_pyramid_configurator_context() is our function; see that
    # - config.make_wsgi_app() is then a Pyramid function
    with camcops_pyramid_configurator_context(
            debug_toolbar=debug_toolbar,
            static_cache_duration_s=static_cache_duration_s) as config:
        app = config.make_wsgi_app()

    # Add any middleware above the Pyramid level:

    if show_requests:
        # noinspection PyTypeChecker
        app = RequestLoggingMiddleware(
            app,
            logger=logging.getLogger(__name__),
            loglevel=logging.INFO,
            show_request_immediately=show_request_immediately,
            show_response=show_response,
            show_timing=show_timing,
        )  # type: Router

    if reverse_proxied_config and reverse_proxied_config.necessary():
        # noinspection PyTypeChecker
        app = ReverseProxiedMiddleware(
            app=app,
            config=reverse_proxied_config,
            debug=debug_reverse_proxy)  # type: Router

    log.debug("WSGI app created")
    return app


# =============================================================================
# Web server launchers
# =============================================================================

def ensure_ok_for_webserver() -> None:
    """
    Prerequisites for firing up the web server.
    """
    ensure_database_is_ok()
    ensure_directories_exist()
    precache()


def test_serve_pyramid(application: "Router",
                       host: str = None,
                       port: int = None) -> None:
    """
    Launches an extremely simple Pyramid web server (via
    ``wsgiref.make_server``).
    """
    cd = ConfigDefaults()
    host = host or cd.HOST
    port = port or cd.PORT
    ensure_ok_for_webserver()
    server = make_server(host, port, application)
    log.info("Serving on host={}, port={}", host, port)
    server.serve_forever()


def serve_cherrypy(application: "Router",
                   host: str,
                   port: int,
                   unix_domain_socket_filename: str,
                   threads_start: int,
                   threads_max: int,  # -1 for no limit
                   server_name: str,
                   log_screen: bool,
                   ssl_certificate: Optional[str],
                   ssl_private_key: Optional[str],
                   root_path: str) -> None:
    """
    Start CherryPy server.

    - Multithreading.
    - Any platform.
    """
    ensure_ok_for_webserver()

    # Report on options
    if unix_domain_socket_filename:
        # If this is specified, it takes priority
        log.info("Starting CherryPy server via UNIX domain socket at: {}",
                 unix_domain_socket_filename)
    else:
        log.info("Starting CherryPy server on host {}, port {}", host, port)
    log.info("Within this web server instance, CamCOPS will be at: {}",
             root_path)
    log.info(
        "... webview at: {}",
        # urllib.parse.urljoin is useless for this
        join_url_fragments(root_path, RouteCollection.HOME.path))
    log.info(
        "... tablet client API at: {}",
        join_url_fragments(root_path, RouteCollection.CLIENT_API.path))
    log.info("Thread pool starting size: {}", threads_start)
    log.info("Thread pool max size: {}", threads_max)

    # Set up CherryPy
    cherrypy.config.update({
        # See http://svn.cherrypy.org/trunk/cherrypy/_cpserver.py
        'server.socket_host': host,
        'server.socket_port': port,
        'server.socket_file': unix_domain_socket_filename,
        'server.thread_pool': threads_start,
        'server.thread_pool_max': threads_max,
        'server.server_name': server_name,
        'server.log_screen': log_screen,
    })
    if ssl_certificate and ssl_private_key:
        cherrypy.config.update({
            'server.ssl_module': 'builtin',
            'server.ssl_certificate': ssl_certificate,
            'server.ssl_private_key': ssl_private_key,
        })

    # Mount WSGI application
    cherrypy.tree.graft(application, root_path)

    # Start server
    try:
        # log.debug("cherrypy.server.thread_pool: {}",
        #           cherrypy.server.thread_pool)
        cherrypy.engine.start()
        cherrypy.engine.block()
    except KeyboardInterrupt:
        cherrypy.engine.stop()


def serve_gunicorn(application: "Router",
                   host: str,
                   port: int,
                   unix_domain_socket_filename: str,
                   num_workers: int,
                   ssl_certificate: Optional[str],
                   ssl_private_key: Optional[str],
                   reload: bool = False,
                   timeout_s: int = 30,
                   debug_show_gunicorn_options: bool = False) -> None:
    """
    Start Gunicorn server

    - Multiprocessing; this is a Good Thing particularly in Python; see e.g.

      - https://eli.thegreenplace.net/2012/01/16/python-parallelizing-cpu-bound-tasks-with-multiprocessing/
      - https://www.dabeaz.com/python/UnderstandingGIL.pdf

    - UNIX only.

    - The Pyramid debug toolbar detects a multiprocessing web server and says
      "shan't, because I use global state".
    """  # noqa: E501
    if BaseApplication is None:
        raise_runtime_error("Gunicorn does not run under Windows. "
                            "(It relies on the UNIX fork() facility.)")

    ensure_ok_for_webserver()

    # Report on options, and calculate Gunicorn versions
    if unix_domain_socket_filename:
        # If this is specified, it takes priority
        log.info("Starting Gunicorn server via UNIX domain socket at: {}",
                 unix_domain_socket_filename)
        bind = "unix:" + unix_domain_socket_filename
    else:
        log.info("Starting Gunicorn server on host {}, port {}", host, port)
        bind = f"{host}:{port}"
    log.info("... using {} workers", num_workers)

    # We encapsulate this class definition in the function, since it inherits
    # from a class whose import will crash under Windows.

    # http://docs.gunicorn.org/en/stable/custom.html

    class StandaloneApplication(BaseApplication):
        def __init__(self,
                     app_: "Router",
                     options: Dict[str, Any] = None,
                     debug_show_known_settings: bool = False) -> None:
            self.options = options or {}  # type: Dict[str, Any]
            self.application = app_
            super().__init__()
            if debug_show_known_settings:
                # log.info("Gunicorn settings:\n{}", pformat(self.cfg.settings))
                # ... which basically tells us to look in gunicorn/config.py
                # at every class that inherits from Setting.
                # Each has helpful documentation, as follows:
                possible_keys = sorted(self.cfg.settings.keys())
                for k in possible_keys:
                    v = self.cfg.settings[k]
                    log.info("{}:\n{}", k, v.desc)

        def load_config(self) -> None:
            # The Gunicorn example looks somewhat convoluted! Let's be simpler:
            for key, value in self.options.items():
                key_lower = key.lower()
                if key_lower in self.cfg.settings and value is not None:
                    self.cfg.set(key_lower, value)

        def load(self) -> "Router":
            return self.application

    opts = {
        'bind': bind,
        'certfile': ssl_certificate,
        'keyfile': ssl_private_key,
        'reload': reload,
        'timeout': timeout_s,
        'workers': num_workers,
    }
    app = StandaloneApplication(
        application, opts,
        debug_show_known_settings=debug_show_gunicorn_options)
    app.run()


# =============================================================================
# Helper functions for command-line functions
# =============================================================================

def get_username_from_cli(req: CamcopsRequest,
                          prompt: str,
                          starting_username: str = "",
                          must_exist: bool = False,
                          must_not_exist: bool = False) -> str:
    """
    Asks the user (via stdout/stdin) for a username.

    Args:
        req: CamcopsRequest object
        prompt: textual prompt
        starting_username: try this username and ask only if it fails tests
        must_exist: the username must exist
        must_not_exist: the username must not exist

    Returns:
        the username

    """
    assert not (must_exist and must_not_exist)
    first = True
    while True:
        if first:
            username = starting_username
            first = False
        else:
            username = ""
        username = username or ask_user(prompt)
        exists = User.user_exists(req, username)
        if must_not_exist and exists:
            log.error("... user already exists!")
            continue
        if must_exist and not exists:
            log.error("... no such user!")
            continue
        if username == USER_NAME_FOR_SYSTEM:
            log.error("... username {!r} is reserved", USER_NAME_FOR_SYSTEM)
            continue
        return username


def get_new_password_from_cli(username: str) -> str:
    """
    Asks the user (via stdout/stdin) for a new password for the specified
    username. Returns the password.
    """
    while True:
        password1 = ask_user_password(f"New password for user {username}")
        try:
            validate_new_password(password1)
        except ValueError as e:
            log.error(str(e))
            continue
        password2 = ask_user_password(
            f"New password for user {username} (again)")
        if password1 != password2:
            log.error("... passwords don't match; try again")
            continue
        return password1


# =============================================================================
# Export command-line functions
# =============================================================================

def cmd_show_export_queue(recipient_names: List[str] = None,
                          all_recipients: bool = False,
                          via_index: bool = True,
                          pretty: bool = False) -> None:
    """
    Shows tasks that would be exported.

    Args:
        recipient_names: list of export recipient names (as per the config
            file)
        all_recipients: use all recipients?
        via_index: use the task index (faster)?
        pretty: use ``str(task)`` not ``repr(task)`` (prettier, slower because
            it has to query the patient)
    """
    with command_line_request_context() as req:
        print_export_queue(req,
                           recipient_names=recipient_names,
                           all_recipients=all_recipients,
                           via_index=via_index,
                           pretty=pretty)


def cmd_export(recipient_names: List[str] = None,
               all_recipients: bool = False,
               via_index: bool = True,
               schedule_via_backend: bool = False) -> None:
    """
    Send all outbound incremental export messages (e.g. HL7).

    Args:
        recipient_names:
            List of export recipient names (as per the config file).
        all_recipients:
            Use all recipients?
        via_index:
            Use the task index (faster)?
        schedule_via_backend:
            Schedule the export via the backend, rather than performing it now.
    """
    with command_line_request_context() as req:
        export(req,
               recipient_names=recipient_names,
               all_recipients=all_recipients,
               via_index=via_index,
               schedule_via_backend=schedule_via_backend)


def make_data_dictionary(filename: str, recipient_name: str,
                         cris: bool = False) -> None:
    """
    Writes a data dictionary for the CRATE anonymisation tool.
    See :func:`camcops_server.cc_export.write_crate_data_dictionary`.

    Args:
        filename: destination filename
        recipient_name: export recipient name
        cris: make DD for CRIS, not CRATE
    """
    target = "CRIS" if cris else "CRATE"
    with command_line_request_context() as req:
        recipients = req.get_export_recipients(
            recipient_names=[recipient_name],
            save=False,
            database_versions=False
        )
        recipient = recipients[0]  # type: ExportRecipientInfo
        log.info(f"Generating {target} data dictionary for export recipient "
                 f"{recipient_name!r}; writing to {filename!r}")
        with open(filename, "wt") as file:
            if cris:
                write_cris_data_dictionary(req=req, file=file,
                                           recipient=recipient)
            else:
                write_crate_data_dictionary(req=req, file=file,
                                            recipient=recipient)


# =============================================================================
# User management command-line functions
# =============================================================================

def make_superuser(username: str = None) -> bool:
    """
    Make a superuser from the command line.
    """
    with command_line_request_context() as req:
        username = get_username_from_cli(
            req=req,
            prompt="Username for new superuser (or to gain superuser status)",
            starting_username=username,
        )
        existing_user = User.get_user_by_name(req.dbsession, username)
        if existing_user:
            log.info("Giving superuser status to {!r}", username)
            existing_user.superuser = True
            success = True
        else:
            log.info("Creating superuser {!r}", username)
            password = get_new_password_from_cli(username=username)
            success = User.create_superuser(req, username, password)
        if success:
            log.info("Success")
            return True
        else:
            log.critical("Failed to create superuser")
            return False


def reset_password(username: str = None) -> bool:
    """
    Reset a password from the command line.
    """
    with command_line_request_context() as req:
        username = get_username_from_cli(
            req=req,
            prompt="Username to reset password for",
            starting_username=username,
            must_exist=True,
        )
        log.info("Resetting password for user {!r}", username)
        password = get_new_password_from_cli(username)
        success = set_password_directly(req, username, password)
        if success:
            log.info("Success")
        else:
            log.critical("Failure")
        return success


def enable_user_cli(username: str = None) -> bool:
    """
    Re-enable a locked user account from the command line.
    """
    with command_line_request_context() as req:
        if username is None:
            username = get_username_from_cli(
                req=req,
                prompt="Username to unlock",
                must_exist=True,
            )
        else:
            if not User.user_exists(req, username):
                log.critical("No such user: {!r}", username)
                return False
        SecurityLoginFailure.enable_user(req, username)
        log.info("Enabled.")
        return True


# =============================================================================
# Other command-line functions
# =============================================================================

def print_database_title() -> None:
    """
    Prints the database title (for the current config) to stdout.
    """
    with command_line_request_context() as req:
        print(req.database_title)


def show_database_schema(schemastem: str,
                         make_image: bool = False,
                         java: str = None,
                         plantuml: str = None,
                         height_width_limit: int = 20000,
                         java_memory_limit_mb: int = 2048) -> None:
    """
    Prints the database schema to a PNG picture.

    Args:
        schemastem:
            filename stem
        make_image:
            Make a PNG image? (May be impractically large!)
        java:
            (for ``make_image``) Java executable
        plantuml:
            (for ``make_image``) PlantUML Java ``.jar`` file
        height_width_limit:
            (for ``make_image``) maximum height and width for PNG; see
            https://plantuml.com/faq
        java_memory_limit_mb:
            (for ``make_image``) Java virtual machine memory limit, in Mb
    """
    # noinspection PyUnresolvedReferences
    import camcops_server.camcops_server_core as core  # delayed import; import side effects  # noqa
    import sadisplay  # delayed import
    import camcops_server.cc_modules.cc_all_models as models  # delayed import
    # ... a re-import to give it a name
    uml_filename = f"{schemastem}.plantuml"
    png_filename = f"{schemastem}.png"
    log.info(f"Making schema PlantUML: {uml_filename}")
    desc = sadisplay.describe([
        getattr(models, attr) for attr in dir(models)
    ])
    # log.debug(desc)
    with open(uml_filename, 'w') as f:
        f.write(sadisplay.plantuml(desc))
    if make_image:
        import shutil  # delayed import
        assert shutil.which(java), f"Can't find Java executable: {java}"
        assert os.path.isfile(
            plantuml), f"Can't find PlantUML JAR file: {plantuml}"  # noqa
        log.info(f"Making schema PNG: {png_filename}")
        cmd = [
            java,
            f"-Xmx{java_memory_limit_mb}m",
            f"-DPLANTUML_LIMIT_SIZE={height_width_limit}",
            '-jar', plantuml,
            uml_filename
        ]
        log.info("Arguments: {}", cmd)
        subprocess.check_call(cmd)


def reindex(cfg: CamcopsConfig) -> None:
    """
    Drops and regenerates the server task index.

    Args:
        cfg: a :class:`camcops_server.cc_modules.cc_config.CamcopsConfig`
    """
    ensure_database_is_ok()
    with cfg.get_dbsession_context() as dbsession:
        reindex_everything(dbsession)


def check_index(cfg: CamcopsConfig, show_all_bad: bool = False) -> bool:
    """
    Checks the server task index for validity.

    Args:
        cfg: a :class:`camcops_server.cc_modules.cc_config.CamcopsConfig`
        show_all_bad:
            show all bad entries? (If false, return upon the first)

    Returns:
        are the indexes all good?
    """
    ensure_database_is_ok()
    with cfg.get_dbsession_context() as dbsession:
        ok = check_indexes(dbsession, show_all_bad)
        if ok:
            log.info("All indexes good.")
        else:
            log.critical("An index is bad. Run the 'reindex' command.")
    return ok


def add_dummy_data(cfg: CamcopsConfig,
                   confirm_add_dummy_data: bool = False) -> None:
    if not confirm_add_dummy_data:
        log.critical("Destructive action not confirmed! Refusing.")
        return

    from camcops_server.cc_modules.cc_dummy_database import DummyDataFactory
    factory = DummyDataFactory(cfg)
    factory.add_data()
    reindex(cfg)


# =============================================================================
# Celery
# =============================================================================

def launch_celery_workers(
        verbose: bool = False,
        cleanup_timeout_s: float = DEFAULT_CLEANUP_TIMEOUT_S) -> None:
    """
    Launch Celery workers.

    See also advice in

    - https://medium.com/@taylorhughes/three-quick-tips-from-two-years-with-celery-c05ff9d7f9eb

    - Re ``-Ofair``:
      https://docs.celeryproject.org/en/latest/userguide/optimizing.html

    """  # noqa: E501
    config = get_default_config_from_os_env()
    cmdargs = [
        CELERY, "worker",
        "--app", CELERY_APP_NAME,
        "-O", "fair",  # optimization
        "--soft-time-limit", str(CELERY_SOFT_TIME_LIMIT_SEC),
        "--loglevel", "DEBUG" if verbose else "INFO",
    ]
    if WINDOWS:
        # See crate_anon/tools/launch_celery.py, and
        # camcops_server/cc_modules/cc_export.py
        os.environ["FORKED_BY_MULTIPROCESSING"] = "1"
        cmdargs.extend([
            "--concurrency", "1",
            "--pool", "solo",
        ])
    cmdargs += config.celery_worker_extra_args
    log.info("Launching: {!r}", cmdargs)
    nice_call(cmdargs, cleanup_timeout=cleanup_timeout_s)


def launch_celery_beat(
        verbose: bool = False,
        cleanup_timeout_s: float = DEFAULT_CLEANUP_TIMEOUT_S) -> None:
    """
    Launch the Celery Beat scheduler.

    (This can be combined with ``celery worker``, but that's not recommended;
    https://docs.celeryproject.org/en/latest/userguide/periodic-tasks.html#starting-the-scheduler).
    """  # noqa: E501
    ensure_directories_exist()
    config = get_default_config_from_os_env()
    cmdargs = [
        CELERY, "beat",
        "--app", CELERY_APP_NAME,
        "--schedule", config.celery_beat_schedule_database,
        "--pidfile", config.get_celery_beat_pidfilename(),
        "--loglevel", "DEBUG" if verbose else "INFO",
    ]
    cmdargs += config.celery_beat_extra_args
    log.info("Launching: {!r}", cmdargs)
    nice_call(cmdargs, cleanup_timeout=cleanup_timeout_s)


def launch_celery_flower(
        address: str = DEFAULT_FLOWER_ADDRESS,
        port: int = DEFAULT_FLOWER_PORT,
        cleanup_timeout_s: float = DEFAULT_CLEANUP_TIMEOUT_S) -> None:
    """
    Launch the Celery Flower monitor.
    """
    cmdargs = [
        CELERY, "flower",
        "--app", CELERY_APP_NAME,
        f"--address {address}",
        f"--port {port}",
    ]
    log.info("Launching: {!r}", cmdargs)
    nice_call(cmdargs, cleanup_timeout=cleanup_timeout_s)


def dev_cli() -> None:
    """
    Fire up a developer debug command-line.
    """
    config = get_default_config_from_os_env()
    # noinspection PyUnusedLocal
    engine = config.get_sqla_engine()  # noqa: F841
    with command_line_request_context() as req:
        # noinspection PyUnusedLocal
        dbsession = req.dbsession  # noqa: F841
        log.error("""Entering developer command-line.
    - Config is available in 'config'.
    - Database engine is available in 'engine'.
    - Dummy request is available in 'req'.
    - Database session is available in 'dbsession'.
        """)
        import pdb
        pdb.set_trace()
        # There must be a line below this, or the context is not available;
        # maybe a pdb bug; see
        # https://stackoverflow.com/questions/51743057/custom-context-manager-is-left-when-running-pdb-set-trace  # noqa
        pass  # this does the job
