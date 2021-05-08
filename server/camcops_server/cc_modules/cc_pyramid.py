#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_pyramid.py

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

**Functions for the Pyramid web framework.**

"""

from enum import Enum
import logging
import os
import pprint
import re
import sys
from typing import (Any, Callable, Dict, List, Optional, Sequence, Tuple,
                    Type, TYPE_CHECKING, Union)
from urllib.parse import urlencode

# from cardinal_pythonlib.debugging import get_caller_stack_info
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.wsgi.constants import WsgiEnvVar
from mako.lookup import TemplateLookup
from paginate import Page
from pyramid.authentication import IAuthenticationPolicy
from pyramid.authorization import IAuthorizationPolicy
from pyramid.config import Configurator
from pyramid.httpexceptions import HTTPFound
from pyramid.interfaces import ILocation, ISession
from pyramid.request import Request
from pyramid.security import (
    Allowed,
    Denied,
    Authenticated,
    Everyone,
    PermitsResult,
)
from pyramid.session import SignedCookieSessionFactory
from pyramid_mako import (
    MakoLookupTemplateRenderer,
    MakoRendererFactory,
    MakoRenderingException,
    reraise,
    text_error_template,
)
from sqlalchemy.orm import Query
from sqlalchemy.sql.selectable import Select
from zope.interface import implementer

from camcops_server.cc_modules.cc_baseconstants import TEMPLATE_DIR
from camcops_server.cc_modules.cc_cache import cache_region_static
from camcops_server.cc_modules.cc_constants import DEFAULT_ROWS_PER_PAGE

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))

# =============================================================================
# Debugging options
# =============================================================================

DEBUG_ADD_ROUTES = False
DEBUG_EFFECTIVE_PRINCIPALS = False
DEBUG_TEMPLATE_PARAMETERS = False
# ... logs more information about template creation
DEBUG_TEMPLATE_SOURCE = False
# ... writes the templates in their compiled-to-Python version to a debugging
#     directory (see below), which is very informative.
DEBUGGING_MAKO_DIR = os.path.expanduser("~/tmp/camcops_mako_template_source")

if any([DEBUG_ADD_ROUTES,
        DEBUG_EFFECTIVE_PRINCIPALS,
        DEBUG_TEMPLATE_PARAMETERS,
        DEBUG_TEMPLATE_SOURCE]):
    log.warning("Debugging options enabled!")


# =============================================================================
# Constants
# =============================================================================

COOKIE_NAME = 'camcops'


class CookieKey:
    """
    Keys for HTTP cookies. We keep this to the absolute minimum; cookies
    contain enough detail to look up a session on the server, and then
    everything else is looked up on the server side.
    """
    SESSION_ID = 'session_id'
    SESSION_TOKEN = 'session_token'


class FormAction(object):
    """
    Action values for HTML forms. These values generally end up as the ``name``
    attribute (and sometimes also the ``value`` attribute) of an HTML button.
    """
    CANCEL = 'cancel'
    CLEAR_FILTERS = 'clear_filters'
    DELETE = 'delete'
    FINALIZE = 'finalize'
    SET_FILTERS = 'set_filters'
    SUBMIT = 'submit'  # the default for many forms
    SUBMIT_TASKS_PER_PAGE = 'submit_tpp'
    REFRESH_TASKS = 'refresh_tasks'


class RequestMethod(object):
    """
    Constants to distinguish HTTP GET from HTTP POST requests.
    """
    GET = "GET"
    POST = "POST"


class ViewParam(object):
    """
    View parameter constants.

    Used in the following situations:

    - as parameter names for parameterized URLs (via RoutePath to Pyramid's
      route configuration, then fetched from the matchdict);

    - as form parameter names (often with some duplication as the attribute
      names of deform Form objects, because to avoid duplication would involve
      metaclass mess).
    """
    # QUERY = "_query"  # built in to Pyramid
    ADDRESS = "address"
    ADD_SPECIAL_NOTE = "add_special_note"
    ADMIN = "admin"
    AGE_MINIMUM = "age_minimum"
    AGE_MAXIMUM = "age_maximum"
    ALL_TASKS = "all_tasks"
    ANONYMISE = "anonymise"
    CLINICIAN_CONFIRMATION = "clinician_confirmation"
    CSRF_TOKEN = "csrf_token"
    DATABASE_TITLE = "database_title"
    DELIVERY_MODE = "delivery_mode"
    DESCRIPTION = "description"
    DEVICE_ID = "device_id"
    DEVICE_IDS = "device_ids"
    DIALECT = "dialect"
    DIAGNOSES_INCLUSION = "diagnoses_inclusion"
    DIAGNOSES_EXCLUSION = "diagnoses_exclusion"
    DUMP_METHOD = "dump_method"
    DOB = "dob"
    DUE_FROM = "due_from"
    DUE_WITHIN = "due_within"
    EMAIL = "email"
    EMAIL_SUBJECT = "email_subject"
    EMAIL_TEMPLATE = "email_template"
    END_DATETIME = "end_datetime"
    INCLUDE_AUTO_GENERATED = "include_auto_generated"
    FILENAME = "filename"
    FINALIZE_POLICY = "finalize_policy"
    FORENAME = "forename"
    FULLNAME = "fullname"
    GP = "gp"
    GROUPADMIN = "groupadmin"
    GROUP_ID = "group_id"
    GROUP_IDS = "group_ids"
    HL7_ID_TYPE = "hl7_id_type"
    HL7_ASSIGNING_AUTHORITY = "hl7_assigning_authority"
    ID = "id"  # generic PK
    ID_DEFINITIONS = "id_definitions"
    ID_REFERENCES = "id_references"
    IDNUM_VALUE = "idnum_value"
    INCLUDE_BLOBS = "include_blobs"
    INCLUDE_CALCULATED = "include_calculated"
    INCLUDE_COMMENTS = "include_comments"
    INCLUDE_INFORMATION_SCHEMA_COLUMNS = "include_information_schema_columns"
    INCLUDE_PATIENT = "include_patient"
    INCLUDE_SNOMED = "include_snomed"
    IP_USE = "ip_use"
    LANGUAGE = "language"
    MANUAL = "manual"
    MAY_ADD_NOTES = "may_add_notes"
    MAY_DUMP_DATA = "may_dump_data"
    MAY_REGISTER_DEVICES = "may_register_devices"
    MAY_RUN_REPORTS = "may_run_reports"
    MAY_UPLOAD = "may_upload"
    MAY_USE_WEBVIEWER = "may_use_webviewer"
    MUST_CHANGE_PASSWORD = "must_change_password"
    NAME = "name"
    NOTE = "note"
    NOTE_ID = "note_id"
    NEW_PASSWORD = "new_password"
    OLD_PASSWORD = "old_password"
    OTHER = "other"
    COMPLETE_ONLY = "complete_only"
    PAGE = "page"
    PASSWORD = "password"
    PATIENT_ID_PER_ROW = "patient_id_per_row"
    PATIENT_TASK_SCHEDULE_ID = "patient_task_schedule_id"
    RECIPIENT_NAME = "recipient_name"
    REDIRECT_URL = "redirect_url"
    REPORT_ID = "report_id"
    REMOTE_IP_ADDR = "remote_ip_addr"
    ROWS_PER_PAGE = "rows_per_page"
    SCHEDULE_ID = "schedule_id"
    SCHEDULE_ITEM_ID = "schedule_item_id"
    SERVER_PK = "server_pk"
    SETTINGS = "settings"
    SEX = "sex"
    SHORT_DESCRIPTION = "short_description"
    SORT = "sort"
    SOURCE = "source"
    SQLITE_METHOD = "sqlite_method"
    START_DATETIME = "start_datetime"
    SUPERUSER = "superuser"
    SURNAME = "surname"
    TABLE_NAME = "table_name"
    TASKS = "tasks"
    TASK_SCHEDULES = "task_schedules"
    TEXT_CONTENTS = "text_contents"
    TRUNCATE = "truncate"
    UPLOAD_GROUP_ID = "upload_group_id"
    UPLOAD_POLICY = "upload_policy"
    USER_GROUP_MEMBERSHIP_ID = "user_group_membership_id"
    USER_ID = "user_id"
    USER_IDS = "user_ids"
    USERNAME = "username"
    VALIDATION_METHOD = "validation_method"
    VIA_INDEX = "via_index"
    VIEW_ALL_PATIENTS_WHEN_UNFILTERED = "view_all_patients_when_unfiltered"
    VIEWTYPE = "viewtype"
    WHICH_IDNUM = "which_idnum"
    WHAT = "what"
    WHEN = "when"
    WHO = "who"


class ViewArg(object):
    """
    String used as view arguments. For example,
    :class:`camcops_server.cc_modules.cc_forms.DumpTypeSelector` represents its
    choices (inside an HTTP POST request) as values from this class.
    """
    # Delivery methods
    DOWNLOAD = "download"
    EMAIL = "email"
    IMMEDIATELY = "immediately"

    # Output types
    HTML = "html"
    ODS = "ods"
    PDF = "pdf"
    PDFHTML = "pdfhtml"  # the HTML to create a PDF
    R = "r"
    SQL = "sql"
    SQLITE = "sqlite"
    TSV = "tsv"
    TSV_ZIP = "tsv_zip"
    XLSX = "xlsx"
    XML = "xml"

    # What to download
    EVERYTHING = "everything"
    SPECIFIC_TASKS_GROUPS = "specific_tasks_groups"
    USE_SESSION_FILTER = "use_session_filter"


# =============================================================================
# Templates
# =============================================================================
# Adaptation of a small part of pyramid_mako, so we can use our own Mako
# TemplateLookup, and thus dogpile.cache. See
# https://github.com/Pylons/pyramid_mako/blob/master/pyramid_mako/__init__.py

MAKO_LOOKUP = TemplateLookup(
    directories=[
        os.path.join(TEMPLATE_DIR, "base"),
        os.path.join(TEMPLATE_DIR, "css"),
        os.path.join(TEMPLATE_DIR, "menu"),
        os.path.join(TEMPLATE_DIR, "snippets"),
        os.path.join(TEMPLATE_DIR, "taskcommon"),
        os.path.join(TEMPLATE_DIR, "tasks"),
        os.path.join(TEMPLATE_DIR, "test"),
    ],

    input_encoding="utf-8",
    output_encoding="utf-8",

    module_directory=DEBUGGING_MAKO_DIR if DEBUG_TEMPLATE_SOURCE else None,

    # strict_undefined=True,  # raise error immediately upon typos
    # ... tradeoff; there are good and bad things about this!
    # One bad thing about strict_undefined=True is that a child (inheriting)
    # template must supply all variables used by its parent (inherited)
    # template, EVEN IF it replaces entirely the <%block> of the parent that
    # uses those variables.

    # -------------------------------------------------------------------------
    # Template default filtering
    # -------------------------------------------------------------------------

    default_filters=["h"],

    # -------------------------------------------------------------------------
    # Template caching
    # -------------------------------------------------------------------------
    # http://dogpilecache.readthedocs.io/en/latest/api.html#module-dogpile.cache.plugins.mako_cache  # noqa
    # http://docs.makotemplates.org/en/latest/caching.html#cache-arguments

    cache_impl="dogpile.cache",
    cache_args={
        "regions": {"local": cache_region_static},
    },

    # Now, in Mako templates, use:
    #   cached="True" cache_region="local" cache_key="SOME_CACHE_KEY"
    # on <%page>, <%def>, and <%block> regions.
    # It is VITAL that you specify "name", and that it be appropriately
    # unique, or there'll be a cache conflict.
    # The easy way is:
    #   cached="True" cache_region="local" cache_key="${self.filename}"
    #                                                 ^^^^^^^^^^^^^^^^
    #                                                   No!
    # ... with ${self.filename} you can get an inheritance deadlock:
    # See https://bitbucket.org/zzzeek/mako/issues/269/inheritance-related-cache-deadlock-when  # noqa
    #
    # HOWEVER, note also: it is the CONTENT that is cached. You can cause some
    # right disasters with this. Only stuff producing entirely STATIC content
    # should be cached. "base.mako" isn't static - it calls back to its
    # children; and if you cache it, one request produces results for an
    # entirely different request. Similarly for lots of other things like
    # "task.mako".
    # SO, THERE IS NOT MUCH REASON HERE TO USE TEMPLATE CACHING.
)


class CamcopsMakoLookupTemplateRenderer(MakoLookupTemplateRenderer):
    r"""
    A Mako template renderer that, when called:

    (a) loads the Mako template
    (b) shoves any other keys we specify into its dictionary

    Typical incoming parameters look like:

    .. code-block:: none

        spec = 'some_template.mako'
        value = {'comment': None}
        system = {
            'context': <pyramid.traversal.DefaultRootFactory ...>,
            'get_csrf_token': functools.partial(<function get_csrf_token ... >, ...>),
            'renderer_info': <pyramid.renderers.RendererHelper ...>,
            'renderer_name': 'some_template.mako',
            'req': <CamcopsRequest ...>,
            'request': <CamcopsRequest ...>,
            'view': None
        }

    Showing the incoming call stack info (see commented-out code) indicates
    that ``req`` and ``request`` (etc.) join at, and are explicitly introduced
    by, :func:`pyramid.renderers.render`. That function includes this code:

    .. code-block:: python

        if system_values is None:
            system_values = {
                'view':None,
                'renderer_name':self.name, # b/c
                'renderer_info':self,
                'context':getattr(request, 'context', None),
                'request':request,
                'req':request,
                'get_csrf_token':partial(get_csrf_token, request),
            }

    So that means, for example, that ``req`` and ``request`` are both always
    present in Mako templates as long as the ``request`` parameter was passed
    to :func:`pyramid.renderers.render_to_response`.

    What about a view configured with ``@view_config(...,
    renderer="somefile.mako")``? Yes, that too (and anything included via
    ``<%include file="otherfile.mako"/>``).

    However, note that ``req`` and ``request`` are only available in the Mako
    evaluation blocks, e.g. via ``${req.someattr}`` or via Python blocks like
    ``<% %>`` -- not via Python blocks like ``<%! %>``, because the actual
    Python generated by a Mako template like this:

    .. code-block:: none

        ## db_user_info.mako
        <%page args="offer_main_menu=False"/>

        <%!
        module_level_thing = context.kwargs  # module-level block; will crash
        %>

        <%
        thing = context.kwargs["request"]  # normal Python block; works
        %>

        <div>
            Database: <b>${ request.database_title | h }</b>.
            %if request.camcops_session.username:
                Logged in as <b>${request.camcops_session.username | h}</b>.
            %endif
            %if offer_main_menu:
                <%include file="to_main_menu.mako"/>
            %endif
        </div>

    looks like this:

    .. code-block:: python

        from mako import runtime, filters, cache
        UNDEFINED = runtime.UNDEFINED
        STOP_RENDERING = runtime.STOP_RENDERING
        __M_dict_builtin = dict
        __M_locals_builtin = locals
        _magic_number = 10
        _modified_time = 1557179054.2796485
        _enable_loop = True
        _template_filename = '...'  # edited
        _template_uri = 'db_user_info.mako'
        _source_encoding = 'utf-8'
        _exports = []

        module_level_thing = context.kwargs  # module-level block; will crash

        def render_body(context,offer_main_menu=False,**pageargs):
            __M_caller = context.caller_stack._push_frame()
            try:
                __M_locals = __M_dict_builtin(offer_main_menu=offer_main_menu,pageargs=pageargs)
                request = context.get('request', UNDEFINED)
                __M_writer = context.writer()
                __M_writer('\n\n')
                __M_writer('\n\n')

                thing = context.kwargs["request"]  # normal Python block; works

                __M_locals_builtin_stored = __M_locals_builtin()
                __M_locals.update(__M_dict_builtin([(__M_key, __M_locals_builtin_stored[__M_key]) for __M_key in ['thing'] if __M_key in __M_locals_builtin_stored]))
                __M_writer('\n\n<div>\n    Database: <b>')
                __M_writer(filters.html_escape(str( request.database_title )))
                __M_writer('</b>.\n')
                if request.camcops_session.username:
                    __M_writer('        Logged in as <b>')
                    __M_writer(filters.html_escape(str(request.camcops_session.username )))
                    __M_writer('</b>.\n')
                if offer_main_menu:
                    __M_writer('        ')
                    runtime._include_file(context, 'to_main_menu.mako', _template_uri)
                    __M_writer('\n')
                __M_writer('</div>\n')
                return ''
            finally:
                context.caller_stack._pop_frame()

        '''
        __M_BEGIN_METADATA
        {"filename": ...}  # edited
        __M_END_METADATA
        '''

    """  # noqa
    def __call__(self, value: Dict[str, Any], system: Dict[str, Any]) -> str:
        if DEBUG_TEMPLATE_PARAMETERS:
            log.debug("spec: {!r}", self.spec)
            log.debug("value: {}", pprint.pformat(value))
            log.debug("system: {}", pprint.pformat(system))
        # log.critical("\n{}", "\n    ".join(get_caller_stack_info()))

        # ---------------------------------------------------------------------
        # RNC extra values:
        # ---------------------------------------------------------------------
        # Note that <%! ... %> Python blocks are not themselves inherited.
        # So putting "import" calls in base.mako doesn't deliver the following
        # as ever-present variable. Instead, plumb them in like this:
        #
        # system['Routes'] = Routes
        # system['ViewArg'] = ViewArg
        # system['ViewParam'] = ViewParam
        #
        # ... except that we're better off with an import in the template

        # Update the system dictionary with the values from the user
        try:
            system.update(value)
        except (TypeError, ValueError):
            raise ValueError('renderer was passed non-dictionary as value')

        # Add the special "_" translation function
        request = system["request"]  # type: CamcopsRequest
        system["_"] = request.gettext

        # Check if 'context' in the dictionary
        context = system.pop('context', None)

        # Rename 'context' to '_context' because Mako internally already has a
        # variable named 'context'
        if context is not None:
            system['_context'] = context

        template = self.template
        if self.defname is not None:
            template = template.get_def(self.defname)
        # noinspection PyBroadException
        try:
            if DEBUG_TEMPLATE_PARAMETERS:
                log.debug("final dict to template: {}", pprint.pformat(system))
            result = template.render_unicode(**system)
        except Exception:
            try:
                exc_info = sys.exc_info()
                errtext = text_error_template().render(error=exc_info[1],
                                                       traceback=exc_info[2])
                reraise(MakoRenderingException(errtext), None, exc_info[2])
            finally:
                # noinspection PyUnboundLocalVariable
                del exc_info

        # noinspection PyUnboundLocalVariable
        return result


class CamcopsMakoRendererFactory(MakoRendererFactory):
    """
    A Mako renderer factory to use :class:`CamcopsMakoLookupTemplateRenderer`.
    """
    # noinspection PyTypeChecker
    renderer_factory = staticmethod(CamcopsMakoLookupTemplateRenderer)


def camcops_add_mako_renderer(config: Configurator, extension: str) -> None:
    """
    Registers a renderer factory for a given template file type.

    Replacement for :func:`add_mako_renderer` from ``pyramid_mako``, so we can
    use our own lookup.

    The ``extension`` parameter is a filename extension (e.g. ".mako").
    """
    renderer_factory = CamcopsMakoRendererFactory()  # our special function
    renderer_factory.lookup = MAKO_LOOKUP  # our lookup information
    config.add_renderer(extension, renderer_factory)  # a Pyramid function


# =============================================================================
# URL/route helpers
# =============================================================================

RE_VALID_REPLACEMENT_MARKER = re.compile("^[a-zA-Z_][a-zA-Z0-9_]*$")
# All characters must be a-z, A-Z, _, or 0-9.
# First character must not be a digit.
# https://docs.pylonsproject.org/projects/pyramid/en/latest/narr/urldispatch.html#route-pattern-syntax  # noqa


def valid_replacement_marker(marker: str) -> bool:
    """
    Is a string suitable for use as a parameter name in a templatized URL?

    (That is: is it free of odd characters?)

    See :class:`UrlParam`.
    """
    return RE_VALID_REPLACEMENT_MARKER.match(marker) is not None


class UrlParamType(Enum):
    """
    Enum for building templatized URLs.
    See :class:`UrlParam`.
    """
    STRING = 1
    POSITIVE_INTEGER = 2
    PLAIN_STRING = 3


class UrlParam(object):
    """
    Represents a parameter within a URL. For example:

    .. code-block:: python

        from camcops_server.cc_modules.cc_pyramid import *
        p = UrlParam("patient_id", UrlParamType.POSITIVE_INTEGER)
        p.markerdef()  # '{patient_id:\\d+}'

    These fragments are suitable for building into a URL for use with Pyramid's
    URL Dispatch system:
    https://docs.pylonsproject.org/projects/pyramid/en/latest/narr/urldispatch.html

    See also :class:`RoutePath`.

    """  # noqa
    def __init__(self, name: str,
                 paramtype: UrlParamType == UrlParamType.PLAIN_STRING) -> None:
        """
        Args:
            name: the name of the parameter
            paramtype: the type (e.g. string? positive integer), defined via
                the :class:`UrlParamType` enum.
        """
        self.name = name
        self.paramtype = paramtype
        assert valid_replacement_marker(name), (
            "UrlParam: invalid replacement marker: " + repr(name)
        )

    def regex(self) -> str:
        """
        Returns text for a regular expression to capture the parameter value.
        """
        if self.paramtype == UrlParamType.STRING:
            return ''
        elif self.paramtype == UrlParamType.POSITIVE_INTEGER:
            return r'\d+'  # digits
        elif self.paramtype == UrlParamType.PLAIN_STRING:
            return r'[a-zA-Z0-9_]+'
        else:
            raise AssertionError("Bug in UrlParam")

    def markerdef(self) -> str:
        """
        Returns the string to use in building the URL.
        """
        marker = self.name
        r = self.regex()
        if r:
            marker += ':' + r
        return '{' + marker + '}'


def make_url_path(base: str, *args: UrlParam) -> str:
    """
    Makes a URL path for use with the Pyramid URL dispatch system.
    See :class:`UrlParam`.

    Args:
        base: the base path, to which we will append parameter templates
        *args: a number of :class:`UrlParam` objects.

    Returns:
        the URL path, beginning with ``/``
    """
    parts = []  # type: List[str]
    if not base.startswith("/"):
        parts.append("/")
    parts += [base] + [arg.markerdef() for arg in args]
    return "/".join(parts)


# =============================================================================
# Routes
# =============================================================================

# Class to collect constants together
# See also http://xion.io/post/code/python-enums-are-ok.html
class Routes(object):
    """
    Names of Pyramid routes.

    - Used by the ``@view_config(route_name=...)`` decorator.
    - Configured via :class:`RouteCollection` / :class:`RoutePath` to the
      Pyramid route configurator.
    """
    # Hard-coded special paths
    STATIC = "static"

    # Other
    ADD_GROUP = "add_group"
    ADD_ID_DEFINITION = "add_id_definition"
    ADD_PATIENT = "add_patient"
    ADD_SPECIAL_NOTE = "add_special_note"
    ADD_TASK_SCHEDULE = "add_task_schedule"
    ADD_TASK_SCHEDULE_ITEM = "add_task_schedule_item"
    ADD_USER = "add_user"
    AUDIT_MENU = "audit_menu"
    BASIC_DUMP = "basic_dump"
    CHANGE_OTHER_PASSWORD = "change_other_password"
    CHANGE_OWN_PASSWORD = "change_own_password"
    CHOOSE_CTV = "choose_ctv"
    CHOOSE_TRACKER = "choose_tracker"
    CLIENT_API = "client_api"
    CLIENT_API_ALIAS = "client_api_alias"
    CRASH = "crash"
    CTV = "ctv"
    DELETE_FILE = "delete_file"
    DELETE_GROUP = "delete_group"
    DELETE_ID_DEFINITION = "delete_id_definition"
    DELETE_PATIENT = "delete_patient"
    DELETE_SERVER_CREATED_PATIENT = "delete_server_created_patient"
    DELETE_SPECIAL_NOTE = "delete_special_note"
    DELETE_TASK_SCHEDULE = "delete_task_schedule"
    DELETE_TASK_SCHEDULE_ITEM = "delete_task_schedule_item"
    DELETE_USER = "delete_user"
    DEVELOPER = "developer"
    DOWNLOAD_AREA = "download_area"
    DOWNLOAD_FILE = "download_file"
    EDIT_GROUP = "edit_group"
    EDIT_ID_DEFINITION = "edit_id_definition"
    EDIT_FINALIZED_PATIENT = "edit_finalized_patient"
    EDIT_SERVER_CREATED_PATIENT = "edit_server_created_patient"
    EDIT_SERVER_SETTINGS = "edit_server_settings"
    EDIT_TASK_SCHEDULE = "edit_task_schedule"
    EDIT_TASK_SCHEDULE_ITEM = "edit_task_schedule_item"
    EDIT_USER = "edit_user"
    EDIT_USER_GROUP_MEMBERSHIP = "edit_user_group_membership"
    ERASE_TASK_LEAVING_PLACEHOLDER = "erase_task_leaving_placeholder"
    ERASE_TASK_ENTIRELY = "erase_task_entirely"
    FORCIBLY_FINALIZE = "forcibly_finalize"
    HOME = "home"
    LOGIN = "login"
    LOGOUT = "logout"
    OFFER_AUDIT_TRAIL = "offer_audit_trail"
    OFFER_EXPORTED_TASK_LIST = "offer_exported_task_list"
    OFFER_REGENERATE_SUMMARIES = "offer_regenerate_summary_tables"
    OFFER_REPORT = "offer_report"
    OFFER_SQL_DUMP = "offer_sql_dump"
    OFFER_TERMS = "offer_terms"
    OFFER_BASIC_DUMP = "offer_basic_dump"
    REPORT = "report"
    REPORTS_MENU = "reports_menu"
    SET_FILTERS = "set_filters"
    SET_OTHER_USER_UPLOAD_GROUP = "set_other_user_upload_group"
    SET_OWN_USER_UPLOAD_GROUP = "set_user_upload_group"
    SQL_DUMP = "sql_dump"
    TASK = "task"
    TESTPAGE_PRIVATE_1 = "testpage_private_1"
    TESTPAGE_PRIVATE_2 = "testpage_private_2"
    TESTPAGE_PRIVATE_3 = "testpage_private_3"
    TESTPAGE_PRIVATE_4 = "testpage_private_4"
    TESTPAGE_PUBLIC_1 = "testpage_public_1"
    TRACKER = "tracker"
    UNLOCK_USER = "unlock_user"
    VIEW_ALL_USERS = "view_all_users"
    VIEW_AUDIT_TRAIL = "view_audit_trail"
    VIEW_DDL = "view_ddl"
    VIEW_EMAIL = "view_email"
    VIEW_EXPORT_RECIPIENT = "view_export_recipient"
    VIEW_EXPORTED_TASK = "view_exported_task"
    VIEW_EXPORTED_TASK_LIST = "view_exported_task_list"
    VIEW_EXPORTED_TASK_EMAIL = "view_exported_task_email"
    VIEW_EXPORTED_TASK_FILE_GROUP = "view_exported_task_file_group"
    VIEW_EXPORTED_TASK_HL7_MESSAGE = "view_exported_task_hl7_message"
    VIEW_GROUPS = "view_groups"
    VIEW_ID_DEFINITIONS = "view_id_definitions"
    VIEW_OWN_USER_INFO = "view_own_user_info"
    VIEW_PATIENT_TASK_SCHEDULE = "view_patient_task_schedule"
    VIEW_PATIENT_TASK_SCHEDULES = "view_patient_task_schedules"
    VIEW_SERVER_INFO = "view_server_info"
    VIEW_TASKS = "view_tasks"
    VIEW_TASK_SCHEDULES = "view_task_schedules"
    VIEW_TASK_SCHEDULE_ITEMS = "view_task_schedule_items"
    VIEW_USER = "view_user"
    VIEW_USER_EMAIL_ADDRESSES = "view_user_email_addresses"
    XLSX_DUMP = "xlsx_dump"


class RoutePath(object):
    r"""
    Class to hold a route/path pair.

    - Pyramid route names are just strings used internally for convenience.
    - Pyramid URL paths are URL fragments, like ``'/thing'``, and can contain
      placeholders, like ``'/thing/{bork_id}'``, which will result in the
      ``request.matchdict`` object containing a ``'bork_id'`` key. Those can be
      further constrained by regular expressions, like
      ``'/thing/{bork_id:\d+}'`` to restrict to digits.

    """
    def __init__(self, route: str, path: str = "",
                 ignore_in_all_routes: bool = False) -> None:
        self.route = route
        self.path = path or "/" + route
        self.ignore_in_all_routes = ignore_in_all_routes


MASTER_ROUTE_WEBVIEW = "/"
MASTER_ROUTE_CLIENT_API = "/api"
MASTER_ROUTE_CLIENT_API_ALIAS = "/database"
STATIC_CAMCOPS_PACKAGE_PATH = "camcops_server.static:"
# ... the "static" package (directory with __init__.py) within the
# "camcops_server" owning package


class RouteCollection(object):
    """
    All routes, with their paths, for CamCOPS.
    They will be auto-read by :func:`all_routes`.

    To make a URL on the fly, use :func:`Request.route_url` or
    :func:`CamcopsRequest.route_url_params`.

    To associate a view with a route, use the Pyramid ``@view_config``
    decorator.
    """
    # Hard-coded special paths
    DEBUG_TOOLBAR = RoutePath('debug_toolbar', '/_debug_toolbar/',
                              ignore_in_all_routes=True)  # hard-coded path
    STATIC = RoutePath(Routes.STATIC, "",  # path ignored
                       ignore_in_all_routes=True)

    # Implemented
    ADD_GROUP = RoutePath(Routes.ADD_GROUP)
    ADD_ID_DEFINITION = RoutePath(Routes.ADD_ID_DEFINITION)
    ADD_PATIENT = RoutePath(Routes.ADD_PATIENT)
    ADD_SPECIAL_NOTE = RoutePath(Routes.ADD_SPECIAL_NOTE)
    ADD_TASK_SCHEDULE = RoutePath(Routes.ADD_TASK_SCHEDULE)
    ADD_TASK_SCHEDULE_ITEM = RoutePath(Routes.ADD_TASK_SCHEDULE_ITEM)
    ADD_USER = RoutePath(Routes.ADD_USER)
    AUDIT_MENU = RoutePath(Routes.AUDIT_MENU)
    BASIC_DUMP = RoutePath(Routes.BASIC_DUMP)
    CHANGE_OTHER_PASSWORD = RoutePath(Routes.CHANGE_OTHER_PASSWORD)
    CHANGE_OWN_PASSWORD = RoutePath(Routes.CHANGE_OWN_PASSWORD)
    CHOOSE_CTV = RoutePath(Routes.CHOOSE_CTV)
    CHOOSE_TRACKER = RoutePath(Routes.CHOOSE_TRACKER)
    CLIENT_API = RoutePath(Routes.CLIENT_API, MASTER_ROUTE_CLIENT_API)
    CLIENT_API_ALIAS = RoutePath(Routes.CLIENT_API_ALIAS,
                                 MASTER_ROUTE_CLIENT_API_ALIAS)
    CRASH = RoutePath(Routes.CRASH)
    CTV = RoutePath(Routes.CTV)
    DELETE_FILE = RoutePath(Routes.DELETE_FILE)
    DELETE_GROUP = RoutePath(Routes.DELETE_GROUP)
    DELETE_ID_DEFINITION = RoutePath(Routes.DELETE_ID_DEFINITION)
    DELETE_PATIENT = RoutePath(Routes.DELETE_PATIENT)
    DELETE_SERVER_CREATED_PATIENT = RoutePath(
        Routes.DELETE_SERVER_CREATED_PATIENT
    )
    DELETE_SPECIAL_NOTE = RoutePath(Routes.DELETE_SPECIAL_NOTE)
    DELETE_TASK_SCHEDULE = RoutePath(Routes.DELETE_TASK_SCHEDULE)
    DELETE_TASK_SCHEDULE_ITEM = RoutePath(Routes.DELETE_TASK_SCHEDULE_ITEM)
    DELETE_USER = RoutePath(Routes.DELETE_USER)
    DEVELOPER = RoutePath(Routes.DEVELOPER)
    DOWNLOAD_AREA = RoutePath(Routes.DOWNLOAD_AREA)
    DOWNLOAD_FILE = RoutePath(Routes.DOWNLOAD_FILE)
    EDIT_GROUP = RoutePath(Routes.EDIT_GROUP)
    EDIT_ID_DEFINITION = RoutePath(Routes.EDIT_ID_DEFINITION)
    EDIT_FINALIZED_PATIENT = RoutePath(Routes.EDIT_FINALIZED_PATIENT)
    EDIT_SERVER_CREATED_PATIENT = RoutePath(Routes.EDIT_SERVER_CREATED_PATIENT)
    EDIT_SERVER_SETTINGS = RoutePath(Routes.EDIT_SERVER_SETTINGS)
    EDIT_TASK_SCHEDULE = RoutePath(Routes.EDIT_TASK_SCHEDULE)
    EDIT_TASK_SCHEDULE_ITEM = RoutePath(Routes.EDIT_TASK_SCHEDULE_ITEM)
    EDIT_USER = RoutePath(Routes.EDIT_USER)
    EDIT_USER_GROUP_MEMBERSHIP = RoutePath(Routes.EDIT_USER_GROUP_MEMBERSHIP)
    ERASE_TASK_LEAVING_PLACEHOLDER = RoutePath(Routes.ERASE_TASK_LEAVING_PLACEHOLDER)  # noqa
    ERASE_TASK_ENTIRELY = RoutePath(Routes.ERASE_TASK_ENTIRELY)
    FORCIBLY_FINALIZE = RoutePath(Routes.FORCIBLY_FINALIZE)
    HOME = RoutePath(Routes.HOME, MASTER_ROUTE_WEBVIEW)  # mounted at "/"
    LOGIN = RoutePath(Routes.LOGIN)
    LOGOUT = RoutePath(Routes.LOGOUT)
    OFFER_AUDIT_TRAIL = RoutePath(Routes.OFFER_AUDIT_TRAIL)
    OFFER_EXPORTED_TASK_LIST = RoutePath(Routes.OFFER_EXPORTED_TASK_LIST)
    OFFER_REPORT = RoutePath(Routes.OFFER_REPORT)
    OFFER_SQL_DUMP = RoutePath(Routes.OFFER_SQL_DUMP)
    OFFER_TERMS = RoutePath(Routes.OFFER_TERMS)
    OFFER_BASIC_DUMP = RoutePath(Routes.OFFER_BASIC_DUMP)
    REPORT = RoutePath(Routes.REPORT)
    REPORTS_MENU = RoutePath(Routes.REPORTS_MENU)
    SET_FILTERS = RoutePath(Routes.SET_FILTERS)
    SET_OTHER_USER_UPLOAD_GROUP = RoutePath(Routes.SET_OTHER_USER_UPLOAD_GROUP)
    SET_OWN_USER_UPLOAD_GROUP = RoutePath(Routes.SET_OWN_USER_UPLOAD_GROUP)
    SQL_DUMP = RoutePath(Routes.SQL_DUMP)
    TASK = RoutePath(Routes.TASK)
    TESTPAGE_PRIVATE_1 = RoutePath(Routes.TESTPAGE_PRIVATE_1)
    TESTPAGE_PRIVATE_2 = RoutePath(Routes.TESTPAGE_PRIVATE_2)
    TESTPAGE_PRIVATE_3 = RoutePath(Routes.TESTPAGE_PRIVATE_3)
    TESTPAGE_PRIVATE_4 = RoutePath(Routes.TESTPAGE_PRIVATE_4)
    TESTPAGE_PUBLIC_1 = RoutePath(Routes.TESTPAGE_PUBLIC_1)
    TRACKER = RoutePath(Routes.TRACKER)
    UNLOCK_USER = RoutePath(Routes.UNLOCK_USER)
    VIEW_ALL_USERS = RoutePath(Routes.VIEW_ALL_USERS)
    VIEW_AUDIT_TRAIL = RoutePath(Routes.VIEW_AUDIT_TRAIL)
    VIEW_DDL = RoutePath(Routes.VIEW_DDL)
    VIEW_EMAIL = RoutePath(Routes.VIEW_EMAIL)
    VIEW_EXPORT_RECIPIENT = RoutePath(Routes.VIEW_EXPORT_RECIPIENT)
    VIEW_EXPORTED_TASK = RoutePath(Routes.VIEW_EXPORTED_TASK)
    VIEW_EXPORTED_TASK_LIST = RoutePath(Routes.VIEW_EXPORTED_TASK_LIST)
    VIEW_EXPORTED_TASK_EMAIL = RoutePath(Routes.VIEW_EXPORTED_TASK_EMAIL)
    VIEW_EXPORTED_TASK_FILE_GROUP = RoutePath(Routes.VIEW_EXPORTED_TASK_FILE_GROUP)  # noqa
    VIEW_EXPORTED_TASK_HL7_MESSAGE = RoutePath(Routes.VIEW_EXPORTED_TASK_HL7_MESSAGE)  # noqa
    VIEW_GROUPS = RoutePath(Routes.VIEW_GROUPS)
    VIEW_ID_DEFINITIONS = RoutePath(Routes.VIEW_ID_DEFINITIONS)
    VIEW_OWN_USER_INFO = RoutePath(Routes.VIEW_OWN_USER_INFO)
    VIEW_PATIENT_TASK_SCHEDULE = RoutePath(Routes.VIEW_PATIENT_TASK_SCHEDULE)
    VIEW_PATIENT_TASK_SCHEDULES = RoutePath(Routes.VIEW_PATIENT_TASK_SCHEDULES)
    VIEW_SERVER_INFO = RoutePath(Routes.VIEW_SERVER_INFO)
    VIEW_TASKS = RoutePath(Routes.VIEW_TASKS)
    VIEW_TASK_SCHEDULES = RoutePath(Routes.VIEW_TASK_SCHEDULES)
    VIEW_TASK_SCHEDULE_ITEMS = RoutePath(Routes.VIEW_TASK_SCHEDULE_ITEMS)
    VIEW_USER = RoutePath(Routes.VIEW_USER)
    VIEW_USER_EMAIL_ADDRESSES = RoutePath(Routes.VIEW_USER_EMAIL_ADDRESSES)
    XLSX_DUMP = RoutePath(Routes.XLSX_DUMP)

    @classmethod
    def all_routes(cls) -> List[RoutePath]:
        """
        Fetch all routes for CamCOPS.
        """
        return [
            v for k, v in cls.__dict__.items()
            if not (k.startswith('_') or  # class hidden things
                    k == 'all_routes' or  # this function
                    v.ignore_in_all_routes)  # explicitly ignored
        ]


# =============================================================================
# Pyramid HTTP session handling
# =============================================================================

def get_session_factory() -> Callable[["CamcopsRequest"], ISession]:
    """
    We have to give a Pyramid request a way of making an HTTP session.
    We must return a session factory.

    - An example is in :class:`pyramid.session.SignedCookieSessionFactory`.
    - A session factory has the signature [1]:

      .. code-block:: none

            sessionfactory(req: CamcopsRequest) -> session_object

      - ... where session "is a namespace" [2]
      - ... but more concretely, "implements the pyramid.interfaces.ISession
        interface"

    - We want to be able to make the session by reading the
      :class:`camcops_server.cc_modules.cc_config.CamcopsConfig` from the request.

    [1] https://docs.pylonsproject.org/projects/pyramid/en/latest/glossary.html#term-session-factory

    [2] https://docs.pylonsproject.org/projects/pyramid/en/latest/glossary.html#term-session
    """  # noqa

    def factory(req: "CamcopsRequest") -> ISession:
        """
        How does the session write the cookies to the response? Like this:

        .. code-block:: none

            SignedCookieSessionFactory
                BaseCookieSessionFactory  # pyramid/session.py
                    CookieSession
                        def changed():
                            if not self._dirty:
                                self._dirty = True
                                def set_cookie_callback(request, response):
                                    self._set_cookie(response)
                                    # ...
                                self.request.add_response_callback(set_cookie_callback)

                        def _set_cookie(self, response):
                            # ...
                            response.set_cookie(...)

        """  # noqa
        cfg = req.config
        secure_cookies = not cfg.allow_insecure_cookies
        pyramid_factory = SignedCookieSessionFactory(
            secret=cfg.session_cookie_secret,
            hashalg='sha512',  # the default
            salt='camcops_pyramid_session.',
            cookie_name=COOKIE_NAME,
            max_age=None,  # browser scope; session cookie
            path='/',  # the default
            domain=None,  # the default
            secure=secure_cookies,
            httponly=secure_cookies,
            timeout=None,  # we handle timeouts at the database level instead
            reissue_time=0,  # default; reissue cookie at every request
            set_on_exception=True,  # (default) cookie even if exception raised
            serializer=None,  # (default) use pyramid.session.PickleSerializer
            # As max_age and expires are left at their default of None, these
            # are session cookies.
        )
        return pyramid_factory(req)

    return factory


# =============================================================================
# Authentication; authorization (permissions)
# =============================================================================

class Permission(object):
    """
    Pyramid permission values.

    - Permissions are strings.
    - For "logged in", use ``pyramid.security.Authenticated``
    """
    GROUPADMIN = "groupadmin"
    HAPPY = "happy"  # logged in, can use webview, no need to change p/w, agreed to terms  # noqa
    MUST_AGREE_TERMS = "must_agree_terms"
    MUST_CHANGE_PASSWORD = "must_change_password"
    SUPERUSER = "superuser"


@implementer(IAuthenticationPolicy)
class CamcopsAuthenticationPolicy(object):
    """
    CamCOPS authentication policy.

    See

    - https://docs.pylonsproject.org/projects/pyramid/en/latest/tutorials/wiki2/authorization.html
    - https://docs.pylonsproject.org/projects/pyramid-cookbook/en/latest/auth/custom.html
    - Don't actually inherit from :class:`IAuthenticationPolicy`; it ends up in
      the :class:`zope.interface.interface.InterfaceClass` metaclass and then
      breaks with "zope.interface.exceptions.InvalidInterface: Concrete
      attribute, ..."
    - But ``@implementer`` does the trick.
    """  # noqa

    @staticmethod
    def authenticated_userid(request: "CamcopsRequest") -> Optional[int]:
        """
        Returns the user ID of the authenticated user.
        """
        return request.user_id

    # noinspection PyUnusedLocal
    @staticmethod
    def unauthenticated_userid(request: "CamcopsRequest") -> Optional[int]:
        """
        Returns the user ID of the unauthenticated user.

        We don't allow users to be identified but not authenticated, so we
        return ``None``.
        """
        return None

    @staticmethod
    def effective_principals(request: "CamcopsRequest") -> List[str]:
        """
        Returns a list of strings indicating permissions that the current user
        has.
        """
        principals = [Everyone]
        user = request.user
        if user is not None:
            principals += [Authenticated, 'u:%s' % user.id]
            if user.may_use_webviewer:
                if user.must_change_password:
                    principals.append(Permission.MUST_CHANGE_PASSWORD)
                elif user.must_agree_terms:
                    principals.append(Permission.MUST_AGREE_TERMS)
                else:
                    principals.append(Permission.HAPPY)
            if user.superuser:
                principals.append(Permission.SUPERUSER)
            if user.authorized_as_groupadmin:
                principals.append(Permission.GROUPADMIN)
            # principals.extend(('g:%s' % g.name for g in user.groups))
        if DEBUG_EFFECTIVE_PRINCIPALS:
            log.debug("effective_principals: {!r}", principals)
        return principals

    # noinspection PyUnusedLocal
    @staticmethod
    def remember(request: "CamcopsRequest",
                 userid: int,
                 **kw) -> List[Tuple[str, str]]:
        return []

    # noinspection PyUnusedLocal
    @staticmethod
    def forget(request: "CamcopsRequest") -> List[Tuple[str, str]]:
        return []


@implementer(IAuthorizationPolicy)
class CamcopsAuthorizationPolicy(object):
    """
    CamCOPS authorization policy.
    """
    # noinspection PyUnusedLocal
    @staticmethod
    def permits(context: ILocation, principals: List[str], permission: str) \
            -> PermitsResult:
        if permission in principals:
            return Allowed(f"ALLOWED: permission {permission} present in "
                           f"principals {principals}")

        return Denied(f"DENIED: permission {permission} not in principals "
                      f"{principals}")

    @staticmethod
    def principals_allowed_by_permission(context: ILocation,
                                         permission: str) -> List[str]:
        raise NotImplementedError()  # don't care about this method


# =============================================================================
# Pagination
# =============================================================================
# WebHelpers 1.3 doesn't support Python 3.5.
# The successor to webhelpers.paginate appears to be paginate.

class SqlalchemyOrmQueryWrapper(object):
    """
    Wrapper class to access elements of an SQLAlchemy ORM query in an efficient
    way for pagination. We only ask the database for what we need.

    (But it will perform a ``COUNT(*)`` for the query before fetching it via
    ``LIMIT/OFFSET``.)

    See:

    - https://docs.pylonsproject.org/projects/pylons-webframework/en/latest/helpers.html
    - https://docs.pylonsproject.org/projects/webhelpers/en/latest/modules/paginate.html
    - https://github.com/Pylons/paginate
    """  # noqa
    def __init__(self, query: Query) -> None:
        self.query = query

    def __getitem__(self, cut: slice) -> List[Any]:
        """
        Return a range of objects of an :class:`sqlalchemy.orm.query.Query`
        object.

        Will apply LIMIT/OFFSET to fetch only what we need.
        """
        return self.query[cut]

    def __len__(self) -> int:
        """
        Count the number of objects in an :class:`sqlalchemy.orm.query.Query``
        object.
        """
        return self.query.count()


class CamcopsPage(Page):
    """
    Pagination class, for HTML views that display, for example,
    items 1-20 and buttons like "page 2", "next page", "last page".

    - Fixes a bug in paginate: it slices its collection BEFORE it realizes that
      the page number is out of range.
    - Also, it uses ``..`` for an ellipsis, which is just wrong.
    """
    # noinspection PyShadowingBuiltins
    def __init__(self,
                 collection: Union[Sequence[Any], Query, Select],
                 url_maker: Callable[[int], str],
                 request: "CamcopsRequest",
                 page: int = 1,
                 items_per_page: int = 20,
                 item_count: int = None,
                 wrapper_class: Type[Any] = None,
                 ellipsis: str = "&hellip;",
                 **kwargs) -> None:
        """
        See :class:`paginate.Page`. Additional arguments:

        Args:
            ellipsis: HTML text to use as the ellipsis marker
        """
        self.request = request
        self.ellipsis = ellipsis
        page = max(1, page)
        if item_count is None:
            if wrapper_class:
                item_count = len(wrapper_class(collection))
            else:
                item_count = len(collection)
        n_pages = ((item_count - 1) // items_per_page) + 1
        page = min(page, n_pages)
        super().__init__(
            collection=collection,
            page=page,
            items_per_page=items_per_page,
            item_count=item_count,
            wrapper_class=wrapper_class,
            url_maker=url_maker,
            **kwargs
        )
        # Original defines attributes outside __init__, so:
        self.radius = 2
        self.curpage_attr = {}  # type: Dict[str, str]
        self.separator = ""
        self.link_attr = {}  # type: Dict[str, str]
        self.dotdot_attr = {}  # type: Dict[str, str]
        self.url = ""

    # noinspection PyShadowingBuiltins
    def pager(self,
              format: str = None,
              url: str = None,
              show_if_single_page: bool = True,  # see below!
              separator: str = ' ',
              symbol_first: str = '&lt;&lt;',
              symbol_last: str = '&gt;&gt;',
              symbol_previous: str = '&lt;',
              symbol_next: str = '&gt;',
              link_attr: Dict[str, str] = None,
              curpage_attr: Dict[str, str] = None,
              dotdot_attr: Dict[str, str] = None,
              link_tag: Callable[[Dict[str, str]], str] = None):
        """
        See :func:`paginate.Page.pager`.

        The reason for the default for ``show_if_single_page`` being ``True``
        is that it's possible otherwise to think you've lost your tasks. For
        example: (1) have 99 tasks; (2) view 50/page; (3) go to page 2; (4) set
        number per page to 100. Or simply use the URL to go beyond the end.
        """
        format = format or self.default_pager_pattern()
        link_attr = link_attr or {}  # type: Dict[str, str]
        curpage_attr = curpage_attr or {}  # type: Dict[str, str]
        # dotdot_attr = dotdot_attr or {}  # type: Dict[str, str]
        # dotdot_attr = dotdot_attr or {'class': 'pager_dotdot'}  # our default!
        return super().pager(
            format=format,
            url=url,
            show_if_single_page=show_if_single_page,
            separator=separator,
            symbol_first=symbol_first,
            symbol_last=symbol_last,
            symbol_previous=symbol_previous,
            symbol_next=symbol_next,
            link_attr=link_attr,
            curpage_attr=curpage_attr,
            dotdot_attr=dotdot_attr,
            link_tag=link_tag,
        )

    def default_pager_pattern(self) -> str:
        """
        Allows internationalization of the pager pattern.
        """
        _ = self.request.gettext
        xlated = _("Page $page of $page_count; total $item_count records")
        return (
            f"({xlated}) "
            f"[ $link_first $link_previous ~3~ $link_next $link_last ]"
        )

    # noinspection PyShadowingBuiltins
    def link_map(self,
                 format: str = '~2~',
                 url: str = None,
                 show_if_single_page: bool = False,
                 separator: str = ' ',
                 symbol_first: str = '&lt;&lt;',
                 symbol_last: str = '&gt;&gt;',
                 symbol_previous: str = '&lt;',
                 symbol_next: str = '&gt;',
                 link_attr: Dict[str, str] = None,
                 curpage_attr: Dict[str, str] = None,
                 dotdot_attr: Dict[str, str] = None):
        """
        See equivalent in superclass.

        Fixes bugs (e.g. mutable default arguments) and nasties (e.g.
        enforcing ".." for the ellipsis) in the original.
        """
        self.curpage_attr = curpage_attr or {}  # type: Dict[str, str]
        self.separator = separator
        self.link_attr = link_attr or {}  # type: Dict[str, str]
        self.dotdot_attr = dotdot_attr or {}  # type: Dict[str, str]
        self.url = url

        regex_res = re.search(r'~(\d+)~', format)
        if regex_res:
            radius = regex_res.group(1)
        else:
            radius = 2
        radius = int(radius)
        self.radius = radius

        # Compute the first and last page number within the radius
        # e.g. '1 .. 5 6 [7] 8 9 .. 12'
        # -> leftmost_page  = 5
        # -> rightmost_page = 9
        leftmost_page = (
            max(self.first_page, (self.page - radius))
            if self.first_page else None
        )  # type: Optional[int]
        rightmost_page = (
            min(self.last_page, (self.page+radius))
            if self.last_page else None
        )  # type: Optional[int]
        nav_items = {
            "first_page": None,
            "last_page": None,
            "previous_page": None,
            "next_page": None,
            "current_page": None,
            "radius": self.radius,
            "range_pages": []
        }  # type: Dict[str, Any]

        if leftmost_page is None or rightmost_page is None:
            return nav_items

        nav_items["first_page"] = {
            "type": "first_page",
            "value": symbol_first,
            "attrs": self.link_attr,
            "number": self.first_page,
            "href": self.url_maker(self.first_page)
        }

        # Insert dots if there are pages between the first page
        # and the currently displayed page range
        if leftmost_page - self.first_page > 1:
            # Wrap in a SPAN tag if dotdot_attr is set
            nav_items["range_pages"].append({
                "type": "span",
                "value": self.ellipsis,
                "attrs": self.dotdot_attr,
                "href": "",
                "number": None
            })

        for thispage in range(leftmost_page, rightmost_page + 1):
            # Highlight the current page number and do not use a link
            if thispage == self.page:
                # Wrap in a SPAN tag if curpage_attr is set
                nav_items["range_pages"].append({
                    "type": "current_page",
                    "value": str(thispage),
                    "number": thispage,
                    "attrs": self.curpage_attr,
                    "href": self.url_maker(thispage)
                })
                nav_items["current_page"] = {
                    "value": thispage,
                    "attrs": self.curpage_attr,
                    "type": "current_page",
                    "href": self.url_maker(thispage)
                }
            # Otherwise create just a link to that page
            else:
                nav_items["range_pages"].append({
                    "type": "page",
                    "value": str(thispage),
                    "number": thispage,
                    "attrs": self.link_attr,
                    "href": self.url_maker(thispage)
                })

        # Insert dots if there are pages between the displayed
        # page numbers and the end of the page range
        if self.last_page - rightmost_page > 1:
            # Wrap in a SPAN tag if dotdot_attr is set
            nav_items["range_pages"].append({
                "type": "span",
                "value": self.ellipsis,
                "attrs": self.dotdot_attr,
                "href": "",
                "number": None
            })

        # Create a link to the very last page (unless we are on the last
        # page or there would be no need to insert '..' spacers)
        nav_items["last_page"] = {
            "type": "last_page",
            "value": symbol_last,
            "attrs": self.link_attr,
            "href": self.url_maker(self.last_page),
            "number": self.last_page
        }
        nav_items["previous_page"] = {
            "type": "previous_page",
            "value": symbol_previous,
            "attrs": self.link_attr,
            "number": self.previous_page or self.first_page,
            "href": self.url_maker(self.previous_page or self.first_page)
        }
        nav_items["next_page"] = {
            "type": "next_page",
            "value": symbol_next,
            "attrs": self.link_attr,
            "number": self.next_page or self.last_page,
            "href": self.url_maker(self.next_page or self.last_page)
        }
        return nav_items


class SqlalchemyOrmPage(CamcopsPage):
    """
    A pagination page that paginates SQLAlchemy ORM queries efficiently.
    """
    def __init__(self,
                 query: Query,
                 url_maker: Callable[[int], str],
                 request: "CamcopsRequest",
                 page: int = 1,
                 items_per_page: int = DEFAULT_ROWS_PER_PAGE,
                 item_count: int = None,
                 **kwargs) -> None:
        # Since views may accidentally throw strings our way:
        assert isinstance(page, int)
        assert isinstance(items_per_page, int)
        assert isinstance(item_count, int) or item_count is None
        super().__init__(
            collection=query,
            request=request,
            page=page,
            items_per_page=items_per_page,
            item_count=item_count,
            wrapper_class=SqlalchemyOrmQueryWrapper,
            url_maker=url_maker,
            **kwargs
        )


# From webhelpers.paginate (which is broken on Python 3.5, but good),
# modified a bit:

def make_page_url(path: str, params: Dict[str, str], page: int,
                  partial: bool = False, sort: bool = True) -> str:
    """
    A helper function for URL generators.

    I assemble a URL from its parts. I assume that a link to a certain page is
    done by overriding the 'page' query parameter.

    ``path`` is the current URL path, with or without a "scheme://host" prefix.

    ``params`` is the current query parameters as a dict or dict-like object.

    ``page`` is the target page number.

    If ``partial`` is true, set query param 'partial=1'. This is to for AJAX
    calls requesting a partial page.

    If ``sort`` is true (default), the parameters will be sorted. Otherwise
    they'll be in whatever order the dict iterates them.
    """
    params = params.copy()
    params["page"] = str(page)
    if partial:
        params["partial"] = "1"
    if sort:
        params = sorted(params.items())
    qs = urlencode(params, True)  # was urllib.urlencode, but changed in Py3.5
    return "%s?%s" % (path, qs)


class PageUrl(object):
    """
    A page URL generator for WebOb-compatible Request objects.

    I derive new URLs based on the current URL but overriding the 'page'
    query parameter.

    I'm suitable for Pyramid, Pylons, and TurboGears, as well as any other
    framework whose Request object has 'application_url', 'path', and 'GET'
    attributes that behave the same way as ``webob.Request``'s.
    """

    def __init__(self, request: "Request", qualified: bool = False):
        """
        ``request`` is a WebOb-compatible ``Request`` object.

        If ``qualified`` is false (default), generated URLs will have just the
        path and query string. If true, the "scheme://host" prefix will be
        included. The default is false to match traditional usage, and to avoid
        generating unuseable URLs behind reverse proxies (e.g., Apache's
        mod_proxy).
        """
        self.request = request
        self.qualified = qualified

    def __call__(self, page: int, partial: bool = False) -> str:
        """
        Generate a URL for the specified page.
        """
        if self.qualified:
            path = self.request.application_url
        else:
            path = self.request.path
        return make_page_url(path, self.request.GET, page, partial)


# =============================================================================
# Debugging requests and responses
# =============================================================================

def get_body_from_request(req: Request) -> bytes:
    """
    Debugging function to read the body from an HTTP request.
    May not work and will warn accordingly. Use Wireshark to be sure
    (https://www.wireshark.org/).
    """
    log.warning("Attempting to read body from request -- but a previous read "
                "may have left this empty. Consider using Wireshark!")
    wsgi_input = req.environ[WsgiEnvVar.WSGI_INPUT]
    # ... under gunicorn, is an instance of gunicorn.http.body.Body
    return wsgi_input.read()


class HTTPFoundDebugVersion(HTTPFound):
    """
    A debugging version of :class:`HTTPFound`, for debugging redirections.
    """
    def __init__(self, location: str = '', **kwargs) -> None:
        log.debug("Redirecting to {!r}", location)
        super().__init__(location, **kwargs)
