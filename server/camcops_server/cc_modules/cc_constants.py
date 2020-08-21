#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_constants.py

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

**Various constants.**

"""

# Helpful UTF-8 characters: ‘’ “” – — × • ≤ ≥ ≠ ± →

import logging
import multiprocessing
import os

from cardinal_pythonlib.sqlalchemy.session import make_mysql_url

from camcops_server.cc_modules.cc_baseconstants import (
    DEFAULT_EXTRA_STRINGS_DIR,
    LINUX_DEFAULT_LOCK_DIR,
    LINUX_DEFAULT_USER_DOWNLOAD_DIR,
    STATIC_ROOT_DIR,
)
from camcops_server.cc_modules.cc_language import DEFAULT_LOCALE


# =============================================================================
# Number of ID numbers. Don't alter this lightly; influences database fields.
# =============================================================================

NUMBER_OF_IDNUMS_DEFUNCT = 8  # DEFUNCT BUT DO NOT REMOVE OR ALTER. EIGHT.
# ... In older versions: determined number of ID number fields.
# (Now this is arbitrary.) Still used to support old clients.


# =============================================================================
# File types
# =============================================================================

class FileType(object):
    """
    Used to represent output formats and their file extensions.
    """
    HTML = "html"
    PDF = "pdf"
    XML = "xml"


# =============================================================================
# Launching
# =============================================================================

DEFAULT_FLOWER_ADDRESS = "127.0.0.1"
DEFAULT_FLOWER_PORT = 5555  # http://docs.celeryproject.org/en/latest/userguide/monitoring.html  # noqa


# =============================================================================
# Webview constants
# =============================================================================

DEFAULT_ROWS_PER_PAGE = 25
DEVICE_NAME_FOR_SERVER = "server"  # Do not alter.
USER_NAME_FOR_SYSTEM = "system"  # Do not alter.


# =============================================================================
# Date formats
# =============================================================================

class DateFormat(object):
    """
    Assorted date/time formats.
    """
    SHORT_DATE = "%d %b %Y"  # e.g. 24 Jul 2013
    LONG_DATE = "%d %B %Y"  # e.g. 24 July 2013
    LONG_DATE_WITH_DAY = "%a %d %B %Y"  # e.g. Wed 24 July 2013
    LONG_DATETIME = "%d %B %Y, %H:%M %z"  # e.g. 24 July 2013, 20:04 +0100
    LONG_DATETIME_WITH_DAY = "%a %d %B %Y, %H:%M %z"  # e.g. Wed 24 July 2013, 20:04 +0100  # noqa
    LONG_DATETIME_WITH_DAY_NO_TZ = "%a %d %B %Y, %H:%M"  # e.g. Wed 24 July 2013, 20:04  # noqa
    SHORT_DATETIME_WITH_DAY_NO_TZ = "%a %d %b %Y, %H:%M"  # e.g. Wed 24 Jul 2013, 20:04  # noqa
    LONG_DATETIME_SECONDS = "%d %B %Y, %H:%M:%S %z"
    SHORT_DATETIME = "%d %b %Y, %H:%M %z"  # e.g. 24 Jul 2013, 20:04 +0100
    SHORT_DATETIME_NO_TZ = "%d %b %Y, %H:%M"  # e.g. 24 Jul 2013, 20:04
    SHORT_DATETIME_SECONDS = "%d %b %Y, %H:%M:%S %z"  # e.g. 24 Jul 2013, 20:04:23 +0100  # noqa
    HOURS_MINUTES = "%H:%M"  # e.g. 20:04
    ISO8601 = "%Y-%m-%dT%H:%M:%S%z"  # e.g. 2013-07-24T20:04:07+0100
    ISO8601_HUMANIZED_TO_MINUTES = "%Y-%m-%d %H:%M"  # e.g. 2013-07-24 20:04
    ISO8601_HUMANIZED_TO_SECONDS = "%Y-%m-%d %H:%M:%S"  # e.g. 2013-07-24 20:04:23  # noqa
    ISO8601_HUMANIZED_TO_SECONDS_TZ = "%Y-%m-%d %H:%M:%S %z"  # e.g. 2013-07-24 20:04:23 +0100  # noqa
    ISO8601_DATE_ONLY = "%Y-%m-%d"  # e.g. 2013-07-24
    FILENAME = "%Y-%m-%dT%H%M%S"  # e.g. 2013-07-24T200459
    FILENAME_DATE_ONLY = "%Y-%m-%d"  # e.g. 20130724
    HL7_DATETIME = "%Y%m%d%H%M%S%z"  # e.g. 20130724200407+0100
    HL7_DATE = "%Y%m%d"  # e.g. 20130724
    ERA = "%Y-%m-%dT%H:%M:%SZ"  # e.g. 2013-07-24T20:03:07Z
    # http://www.hl7standards.com/blog/2008/07/25/hl7-time-zone-qualification/
    RIO_EXPORT_UK = "%d/%m/%Y %H:%M"  # e.g. 01/12/2014 09:45


# =============================================================================
# Permitted values in fields: some common settings
# =============================================================================

class PV(object):
    """
    Collections of permitted values.
    """
    BIT = [0, 1]


NO_CHAR = 'N'
YES_CHAR = 'Y'

# Database values:
SEX_FEMALE = "F"
SEX_MALE = "M"
SEX_OTHER_UNSPECIFIED = "X"
POSSIBLE_SEX_VALUES = [SEX_FEMALE, SEX_MALE, SEX_OTHER_UNSPECIFIED]


# =============================================================================
# Field names/specifications
# =============================================================================

TABLET_ID_FIELD = "id"
MOVE_OFF_TABLET_FIELD = "_move_off_tablet"
CLIENT_DATE_FIELD = "when_last_modified"

# Used for old client support, and TSV field names etc.:
FP_ID_NUM = "idnum"
FP_ID_DESC = "iddesc"
FP_ID_SHORT_DESC = "idshortdesc"

# Additional fields for some exports:
EXTRA_IDNUM_FIELD_PREFIX = "_patient_idnum"
EXTRA_TASK_TABLENAME_FIELD = "_task_tablename"
EXTRA_TASK_SERVER_PK_FIELD = "_task_pk"
EXTRA_COMMENT_PREFIX = "(EXTRA) "


# =============================================================================
# Other special values
# =============================================================================

# CAMCOPS_URL = "http://www.camcops.org/"
CAMCOPS_URL = "https://camcops.readthedocs.io/"
ERA_NOW = "NOW"  # defines the current era in database records

# =============================================================================
# PDF engine: now always "pdfkit".
# =============================================================================

# PDF_ENGINE = "xhtml2pdf"  # working
PDF_ENGINE = "pdfkit"  # working
# PDF_ENGINE = "weasyprint"  # working but table <tr> element bugs
# ... value must be one of: xhtml2pdf, weasyprint, pdfkit


# =============================================================================
# Simple constants for HTML/plots/display
# =============================================================================

class PlotDefaults(object):
    """
    Defaults used with matplotlib plotting.
    """
    DEFAULT_PLOT_DPI = 300

    FULLWIDTH_PLOT_WIDTH = 6.7  # inches: full width is ~170mm

    # zorder parameter:
    # - higher = on top
    # - defaults relate to the type of thing being plotted:
    #   https://matplotlib.org/3.1.1/gallery/misc/zorder_demo.html
    #   - Patch / PatchCollection = 1
    #   - Line2D / LineCollection = 2
    #   - Text = 3
    # - within a Line2D object (points and lines), the default is
    #   "markers on top of lines"
    ZORDER_PRESET_LINES = 1
    ZORDER_PRESET_LABELS = 2
    ZORDER_DATA_LINES_POINTS = 3  # the default


class MatplotlibConstants(object):
    """
    Constants used by matplotlib
    """
    # https://matplotlib.org/tutorials/colors/colors.html
    COLOUR_BLACK = "k"
    COLOUR_BLUE = "b"
    COLOUR_GREEN = "g"
    COLOUR_GREY_50 = "0.5"
    COLOUR_GREY_90 = "0.9"  # 0.9 is close to white (0 black, 1 white)
    COLOUR_RED = "r"

    # https://matplotlib.org/gallery/lines_bars_and_markers/line_styles_reference.html  # noqa
    # https://matplotlib.org/3.1.0/gallery/lines_bars_and_markers/linestyles.html  # noqa
    LINESTYLE_DOTTED = ":"
    LINESTYLE_SOLID = "-"
    LINESTYLE_NONE = "None"

    # https://matplotlib.org/3.1.1/api/markers_api.html
    MARKER_CIRCLE = "o"
    MARKER_NONE = ""  # also "None", " "
    MARKER_PLUS = "+"
    MARKER_STAR = "*"

    WHOLE_PANEL = 111  # as in: ax = fig.add_subplot(111)


# Debugging option
USE_SVG_IN_HTML = True  # set to False for PNG debugging


# =============================================================================
# CSS/HTML constants
# =============================================================================

CSS_PAGED_MEDIA = (PDF_ENGINE != "pdfkit")

WKHTMLTOPDF_OPTIONS = {  # dict for pdfkit
    "page-size": "A4",
    "margin-left": "20mm",
    "margin-right": "20mm",
    "margin-top": "21mm",  # from paper edge down to top of content?
    # ... inaccurate
    "margin-bottom": "24mm",  # from paper edge up to bottom of content?
    # ... inaccurate
    "header-spacing": "3",  # mm, from content up to bottom of header
    "footer-spacing": "3",  # mm, from content down to top of footer
    "quiet": "",  # Suppress "Loading pages (1/6)" etc.
    "enable-local-file-access": "",
}


class CssClass(object):
    """
    CSS names.

    Values should match e.g. ``camcops_server/templates/css/css_base.mako``.
    """
    BAD_ID_POLICY_MILD = "badidpolicy_mild"
    BAD_ID_POLICY_SEVERE = "badidpolicy_severe"
    BANNER = "banner"
    BANNER_REFERRAL_GENERAL_ADULT = "banner_referral_general_adult"
    BANNER_REFERRAL_OLD_AGE = "banner_referral_old_age"
    BANNER_REFERRAL_SUBSTANCE_MISUSE = "banner_referral_substance_misuse"
    CENTREGAP_TD = "centregap_td"
    CLINICIAN = "clinician"
    COPYRIGHT = "copyright"
    CTV_DATELIMIT_START = "ctv_datelimit_start"
    CTV_DATELIMIT_END = "ctv_datelimit_end"
    CTV_TASKHEADING = "ctv_taskheading"
    CTV_FIELDHEADING = "ctv_fieldheading"
    CTV_FIELDSUBHEADING = "ctv_fieldsubheading"
    CTV_FIELDDESCRIPTION = "ctv_fielddescription"
    CTV_FIELDCONTENT = "ctv_fieldcontent"
    CTV_WARNINGS = "ctv_warnings"
    ERROR = "error"
    EXPLANATION = "explanation"
    EXTRADETAIL = "extradetail"
    EXTRADETAIL2 = "extradetail2"
    FILTER = "filter"
    FILTERS = "filters"
    FIGURE = "figure"
    FOOTNOTES = "footnotes"
    FORMTITLE = "formtitle"
    GENERAL = "general"
    GREEN = "green"
    HANGINGINDENT = "hangingindent"
    HEADING = "heading"
    HIGHLIGHT = "highlight"
    IMAGE_TD = "image_td"
    IMPORTANT = "important"
    INCOMPLETE = "incomplete"
    INDENT = "indent"
    INDENTED = "indented"
    LIVE_ON_TABLET = "live_on_tablet"
    LOGO_LEFT = "logo_left"
    LOGO_RIGHT = "logo_right"
    NAVIGATION = "navigation"
    NOBORDER = "noborder"
    NOBORDERPHOTO = "noborderphoto"
    OFFICE = "office"
    PATIENT = "patient"
    PHOTO = "photo"
    PDF_LOGO_HEADER = "pdf_logo_header"
    QA_TABLE_HEADING = "qa_tableheading"
    RESPONDENT = "respondent"
    SIGNATURE = "signature"
    SIGNATURE_LABEL = "signature_label"
    SMALLPRINT = "smallprint"
    SPECIALNOTE = "specialnote"
    SUBHEADING = "subheading"
    SUBSUBHEADING = "subsubheading"
    SUMMARY = "summary"
    SUPERUSER = "superuser"
    TASKCONFIG = "taskconfig"
    TASKDETAIL = "taskdetail"
    TASKHEADER = "taskheader"
    TRACKERHEADER = "trackerheader"
    TRACKER_ALL_CONSISTENT = "tracker_all_consistent"
    WARNING = "warning"
    WEB_LOGO_HEADER = "web_logo_header"


# =============================================================================
# Task constants
# =============================================================================

ANON_PATIENT = "XXXX"
DATA_COLLECTION_ONLY_DIV = """
    <div class="copyright">
        Reproduction of the original task/scale is not permitted.
        This is a data collection tool only; use it only in conjunction with
        a licensed copy of the original task.
    </div>
"""
DATA_COLLECTION_UNLESS_UPGRADED_DIV = """
    <div class="copyright">
        Reproduction of the original task/scale is not permitted as part of
        CamCOPS. This is a data collection tool only, unless the hosting
        institution has supplied task text via its own permissions. <b>Any such
        text, if shown here, is not part of CamCOPS, and copyright in
        it belongs to the original task’s copyright holder.</b> Use this data
        collection tool only in conjunction with a licensed copy of the
        original task.
    </div>
"""
ICD10_COPYRIGHT_DIV = """
    <div class="copyright">
        ICD-10 criteria: Copyright © 1992 World Health Organization.
        Used here with permission.
    </div>
"""
INVALID_VALUE = "[invalid_value]"

TSV_PATIENT_FIELD_PREFIX = "_patient_"

QUESTION = "Question"


# =============================================================================
# Config constants
# =============================================================================

CONFIG_FILE_SITE_SECTION = "site"
CONFIG_FILE_SERVER_SECTION = "server"
CONFIG_FILE_EXPORT_SECTION = "export"


class ConfigParamSite(object):
    """
    Parameters allowed in the main ``[site]`` section of the CamCOPS config
    file.
    """
    ALLOW_INSECURE_COOKIES = "ALLOW_INSECURE_COOKIES"
    CAMCOPS_LOGO_FILE_ABSOLUTE = "CAMCOPS_LOGO_FILE_ABSOLUTE"
    CLIENT_API_LOGLEVEL = "CLIENT_API_LOGLEVEL"
    CTV_FILENAME_SPEC = "CTV_FILENAME_SPEC"
    DB_URL = "DB_URL"
    DB_ECHO = "DB_ECHO"
    DISABLE_PASSWORD_AUTOCOMPLETE = "DISABLE_PASSWORD_AUTOCOMPLETE"
    EMAIL_FROM = "EMAIL_FROM"
    EMAIL_HOST = "EMAIL_HOST"
    EMAIL_HOST_PASSWORD = "EMAIL_HOST_PASSWORD"
    EMAIL_HOST_USERNAME = "EMAIL_HOST_USERNAME"
    EMAIL_PORT = "EMAIL_PORT"
    EMAIL_REPLY_TO = "EMAIL_REPLY_TO"
    EMAIL_SENDER = "EMAIL_SENDER"
    EMAIL_USE_TLS = "EMAIL_USE_TLS"
    EXTRA_STRING_FILES = "EXTRA_STRING_FILES"
    LANGUAGE = "LANGUAGE"
    LOCAL_INSTITUTION_URL = "LOCAL_INSTITUTION_URL"
    LOCAL_LOGO_FILE_ABSOLUTE = "LOCAL_LOGO_FILE_ABSOLUTE"
    LOCKOUT_DURATION_INCREMENT_MINUTES = "LOCKOUT_DURATION_INCREMENT_MINUTES"
    LOCKOUT_THRESHOLD = "LOCKOUT_THRESHOLD"
    PASSWORD_CHANGE_FREQUENCY_DAYS = "PASSWORD_CHANGE_FREQUENCY_DAYS"
    PATIENT_SPEC = "PATIENT_SPEC"
    PATIENT_SPEC_IF_ANONYMOUS = "PATIENT_SPEC_IF_ANONYMOUS"
    PERMIT_IMMEDIATE_DOWNLOADS = "PERMIT_IMMEDIATE_DOWNLOADS"
    RESTRICTED_TASKS = "RESTRICTED_TASKS"
    SESSION_COOKIE_SECRET = "SESSION_COOKIE_SECRET"
    SESSION_TIMEOUT_MINUTES = "SESSION_TIMEOUT_MINUTES"
    SNOMED_TASK_XML_FILENAME = "SNOMED_TASK_XML_FILENAME"
    SNOMED_ICD9_XML_FILENAME = "SNOMED_ICD9_XML_FILENAME"
    SNOMED_ICD10_XML_FILENAME = "SNOMED_ICD10_XML_FILENAME"
    TASK_FILENAME_SPEC = "TASK_FILENAME_SPEC"
    TRACKER_FILENAME_SPEC = "TRACKER_FILENAME_SPEC"
    USER_DOWNLOAD_DIR = "USER_DOWNLOAD_DIR"
    USER_DOWNLOAD_FILE_LIFETIME_MIN = "USER_DOWNLOAD_FILE_LIFETIME_MIN"
    USER_DOWNLOAD_MAX_SPACE_MB = "USER_DOWNLOAD_MAX_SPACE_MB"
    WEBVIEW_LOGLEVEL = "WEBVIEW_LOGLEVEL"
    WKHTMLTOPDF_FILENAME = "WKHTMLTOPDF_FILENAME"


class ConfigParamServer(object):
    """
    Parameters allowed in the web server (``[server]``) section of the CamCOPS
    config file.
    """
    CHERRYPY_LOG_SCREEN = "CHERRYPY_LOG_SCREEN"
    CHERRYPY_ROOT_PATH = "CHERRYPY_ROOT_PATH"
    CHERRYPY_SERVER_NAME = "CHERRYPY_SERVER_NAME"
    CHERRYPY_THREADS_MAX = "CHERRYPY_THREADS_MAX"
    CHERRYPY_THREADS_START = "CHERRYPY_THREADS_START"
    DEBUG_REVERSE_PROXY = "DEBUG_REVERSE_PROXY"
    DEBUG_SHOW_GUNICORN_OPTIONS = "DEBUG_SHOW_GUNICORN_OPTIONS"
    DEBUG_TOOLBAR = "DEBUG_TOOLBAR"
    GUNICORN_DEBUG_RELOAD = "GUNICORN_DEBUG_RELOAD"
    GUNICORN_NUM_WORKERS = "GUNICORN_NUM_WORKERS"
    GUNICORN_TIMEOUT_S = "GUNICORN_TIMEOUT_S"
    HOST = "HOST"
    PORT = "PORT"
    PROXY_HTTP_HOST = "PROXY_HTTP_HOST"
    PROXY_REMOTE_ADDR = "PROXY_REMOTE_ADDR"
    PROXY_REWRITE_PATH_INFO = "PROXY_REWRITE_PATH_INFO"
    PROXY_SCRIPT_NAME = "PROXY_SCRIPT_NAME"
    PROXY_SERVER_NAME = "PROXY_SERVER_NAME"
    PROXY_SERVER_PORT = "PROXY_SERVER_PORT"
    PROXY_URL_SCHEME = "PROXY_URL_SCHEME"
    SHOW_REQUEST_IMMEDIATELY = "SHOW_REQUEST_IMMEDIATELY"
    SHOW_REQUESTS = "SHOW_REQUESTS"
    SHOW_RESPONSE = "SHOW_RESPONSE"
    SHOW_TIMING = "SHOW_TIMING"
    SSL_CERTIFICATE = "SSL_CERTIFICATE"
    SSL_PRIVATE_KEY = "SSL_PRIVATE_KEY"
    TRUSTED_PROXY_HEADERS = "TRUSTED_PROXY_HEADERS"
    UNIX_DOMAIN_SOCKET = "UNIX_DOMAIN_SOCKET"


class ConfigParamExportGeneral(object):
    """
    Parameters allowed in the ``[export]`` section of the CamCOPS config file.
    """
    CELERY_BEAT_EXTRA_ARGS = "CELERY_BEAT_EXTRA_ARGS"
    CELERY_BEAT_SCHEDULE_DATABASE = "CELERY_BEAT_SCHEDULE_DATABASE"
    CELERY_BROKER_URL = "CELERY_BROKER_URL"
    CELERY_WORKER_EXTRA_ARGS = "CELERY_WORKER_EXTRA_ARGS"
    CELERY_EXPORT_TASK_RATE_LIMIT = "CELERY_EXPORT_TASK_RATE_LIMIT"
    EXPORT_LOCKDIR = "EXPORT_LOCKDIR"
    RECIPIENTS = "RECIPIENTS"
    SCHEDULE = "SCHEDULE"
    SCHEDULE_TIMEZONE = "SCHEDULE_TIMEZONE"


class ConfigParamExportRecipient(object):
    """
    Possible configuration file parameters that relate to "export recipient"
    definitions.
    """
    ALL_GROUPS = "ALL_GROUPS"
    DB_ADD_SUMMARIES = "DB_ADD_SUMMARIES"
    DB_ECHO = "DB_ECHO"
    DB_INCLUDE_BLOBS = "DB_INCLUDE_BLOBS"
    DB_PATIENT_ID_PER_ROW = "DB_PATIENT_ID_PER_ROW"
    DB_URL = "DB_URL"
    EMAIL_BCC = "EMAIL_BCC"
    EMAIL_BODY = "EMAIL_BODY"
    EMAIL_BODY_IS_HTML = "EMAIL_BODY_IS_HTML"
    EMAIL_CC = "EMAIL_CC"
    EMAIL_KEEP_MESSAGE = "EMAIL_KEEP_MESSAGE"
    EMAIL_RECIPIENTS = "EMAIL_RECIPIENTS"
    EMAIL_PATIENT_SPEC = "EMAIL_PATIENT_SPEC"
    EMAIL_PATIENT_SPEC_IF_ANONYMOUS = "EMAIL_PATIENT_SPEC_IF_ANONYMOUS"
    EMAIL_SUBJECT = "EMAIL_SUBJECT"
    EMAIL_TIMEOUT = "EMAIL_TIMEOUT"
    EMAIL_TO = "EMAIL_TO"
    END_DATETIME_UTC = "END_DATETIME_UTC"
    FILE_EXPORT_RIO_METADATA = "FILE_EXPORT_RIO_METADATA"
    FILE_FILENAME_SPEC = "FILE_FILENAME_SPEC"
    FILE_MAKE_DIRECTORY = "FILE_MAKE_DIRECTORY"
    FILE_OVERWRITE_FILES = "FILE_OVERWRITE_FILES"
    FILE_PATIENT_SPEC = "FILE_PATIENT_SPEC"
    FILE_PATIENT_SPEC_IF_ANONYMOUS = "FILE_PATIENT_SPEC_IF_ANONYMOUS"
    FILE_SCRIPT_AFTER_EXPORT = "FILE_SCRIPT_AFTER_EXPORT"
    FINALIZED_ONLY = "FINALIZED_ONLY"
    GROUPS = "GROUPS"
    HL7_DEBUG_DIVERT_TO_FILE = "HL7_DEBUG_DIVERT_TO_FILE"
    HL7_DEBUG_TREAT_DIVERTED_AS_SENT = "HL7_DEBUG_TREAT_DIVERTED_AS_SENT"
    HL7_HOST = "HL7_HOST"
    HL7_KEEP_MESSAGE = "HL7_KEEP_MESSAGE"
    HL7_KEEP_REPLY = "HL7_KEEP_REPLY"
    HL7_NETWORK_TIMEOUT_MS = "HL7_NETWORK_TIMEOUT_MS"
    HL7_PING_FIRST = "HL7_PING_FIRST"
    HL7_PORT = "HL7_PORT"
    IDNUM_AA_PREFIX = "IDNUM_AA_"  # unusual; prefix not parameter
    IDNUM_TYPE_PREFIX = "IDNUM_TYPE_"  # unusual; prefix not parameter
    INCLUDE_ANONYMOUS = "INCLUDE_ANONYMOUS"
    PRIMARY_IDNUM = "PRIMARY_IDNUM"
    PUSH = "PUSH"
    REDCAP_API_KEY = "REDCAP_API_KEY"
    REDCAP_API_URL = "REDCAP_API_URL"
    REDCAP_FIELDMAP_FILENAME = "REDCAP_FIELDMAP_FILENAME"
    REQUIRE_PRIMARY_IDNUM_MANDATORY_IN_POLICY = "REQUIRE_PRIMARY_IDNUM_MANDATORY_IN_POLICY"  # noqa
    RIO_DOCUMENT_TYPE = "RIO_DOCUMENT_TYPE"
    RIO_IDNUM = "RIO_IDNUM"
    RIO_UPLOADING_USER = "RIO_UPLOADING_USER"
    START_DATETIME_UTC = "START_DATETIME_UTC"
    TASK_FORMAT = "TASK_FORMAT"
    TASKS = "TASKS"
    TRANSMISSION_METHOD = "TRANSMISSION_METHOD"
    XML_FIELD_COMMENTS = "XML_FIELD_COMMENTS"


class StandardPorts(object):
    """
    Standard TCP port numbers.
    """
    ALTERNATIVE_HTTP = 8000
    AMQP = 5672
    SMTP = 25
    SMTP_TLS = 587
    HL7_MLLP = 2575
    MYSQL = 3306


class DockerConstants(object):
    """
    Constants for the Docker environment.
    """
    # Directories
    DOCKER_CAMCOPS_ROOT_DIR = "/camcops"
    CONFIG_DIR = os.path.join(DOCKER_CAMCOPS_ROOT_DIR, "cfg")
    TMP_DIR = os.path.join(DOCKER_CAMCOPS_ROOT_DIR, "tmp")
    VENV_DIR = os.path.join(DOCKER_CAMCOPS_ROOT_DIR, "venv")

    DEFAULT_USER_DOWNLOAD_DIR = os.path.join(TMP_DIR, "user_downloads")
    DEFAULT_LOCKDIR = os.path.join(TMP_DIR, "lock")

    # Container (internal) names
    CONTAINER_RABBITMQ = "rabbitmq"
    CONTAINER_MYSQL = "mysql"

    # Other
    CELERY_BROKER_URL = f"amqp://{CONTAINER_RABBITMQ}:{StandardPorts.AMQP}/"
    DEFAULT_MYSQL_CAMCOPS_USER = "camcops"
    HOST = "0.0.0.0"
    # ... not "localhost" or "127.0.0.1"; see
    # https://nickjanetakis.com/blog/docker-tip-54-fixing-connection-reset-by-peer-or-similar-errors  # noqa


# =============================================================================
# Configuration defaults
# =============================================================================

class ConfigDefaults(object):
    """
    Contains default values for the config, plus some cosmetic defaults for
    generating specimen config files.

    - Re ``CHERRYPY_THREADS_MAX``: beware the default MySQL connection limit of
      151; https://dev.mysql.com/doc/refman/5.7/en/too-many-connections.html
    """
    # [site] section
    ALLOW_INSECURE_COOKIES = False
    CAMCOPS_LOGO_FILE_ABSOLUTE = os.path.join(STATIC_ROOT_DIR,
                                              "logo_camcops.png")
    CLIENT_API_LOGLEVEL = logging.INFO
    CLIENT_API_LOGLEVEL_TEXTFORMAT = "info"  # should match CLIENT_API_LOGLEVEL
    DB_DATABASE = "camcops"  # for demo configs only
    DB_ECHO = False
    DB_PORT = StandardPorts.MYSQL  # for demo configs only
    DB_SERVER = "localhost"  # for demo configs only
    DB_USER = "YYY_USERNAME_REPLACE_ME"  # cosmetic; for demo configs only
    DB_PASSWORD = "ZZZ_PASSWORD_REPLACE_ME"  # cosmetic; for demo configs only
    DISABLE_PASSWORD_AUTOCOMPLETE = True
    EMAIL_PORT = StandardPorts.SMTP_TLS
    EMAIL_USE_TLS = True
    EXTRA_STRING_FILES = os.path.join(DEFAULT_EXTRA_STRINGS_DIR, "*.xml")  # cosmetic; for demo configs only  # noqa
    LANGUAGE = DEFAULT_LOCALE
    LOCAL_INSTITUTION_URL = "http://www.camcops.org/"
    LOCAL_LOGO_FILE_ABSOLUTE = os.path.join(STATIC_ROOT_DIR, "logo_local.png")
    LOCKOUT_DURATION_INCREMENT_MINUTES = 10
    LOCKOUT_THRESHOLD = 10
    PASSWORD_CHANGE_FREQUENCY_DAYS = 0  # zero for never
    PATIENT_SPEC_IF_ANONYMOUS = "anonymous"
    PERMIT_IMMEDIATE_DOWNLOADS = False
    SESSION_TIMEOUT_MINUTES = 30
    USER_DOWNLOAD_DIR = LINUX_DEFAULT_USER_DOWNLOAD_DIR  # for demo configs only  # noqa
    USER_DOWNLOAD_FILE_LIFETIME_MIN = 60
    USER_DOWNLOAD_MAX_SPACE_MB = 100
    WEBVIEW_LOGLEVEL = logging.INFO
    WEBVIEW_LOGLEVEL_TEXTFORMAT = "info"  # should match WEBVIEW_LOGLEVEL

    # Not yet user-configurable
    PLOT_FONTSIZE = 8

    # [server] section
    CHERRYPY_LOG_SCREEN = True
    CHERRYPY_ROOT_PATH = "/"
    CHERRYPY_SERVER_NAME = "localhost"
    CHERRYPY_THREADS_MAX = 100
    CHERRYPY_THREADS_START = 10
    DEBUG_REVERSE_PROXY = False
    DEBUG_SHOW_GUNICORN_OPTIONS = False
    DEBUG_TOOLBAR = False
    GUNICORN_DEBUG_RELOAD = False
    GUNICORN_NUM_WORKERS = 2 * multiprocessing.cpu_count()
    GUNICORN_TIMEOUT_S = 30
    HOST = "127.0.0.1"
    PORT = StandardPorts.ALTERNATIVE_HTTP
    PROXY_REWRITE_PATH_INFO = False
    SHOW_REQUEST_IMMEDIATELY = False
    SHOW_REQUESTS = False
    SHOW_RESPONSE = False
    SHOW_TIMING = False

    # [export] section
    CELERY_BROKER_URL = "amqp://"
    CELERY_BEAT_SCHEDULE_DATABASE = os.path.join(
        LINUX_DEFAULT_LOCK_DIR, "camcops_celerybeat_schedule")  # for demo configs only  # noqa
    EXPORT_LOCKDIR = LINUX_DEFAULT_LOCK_DIR  # for demo configs only
    SCHEDULE_TIMEZONE = "UTC"

    # Individual export recipients
    # DB_ECHO: as above
    ALL_GROUPS = False
    DB_ADD_SUMMARIES = True
    DB_INCLUDE_BLOBS = True
    DB_PATIENT_ID_PER_ROW = False
    EMAIL_BODY_IS_HTML = False
    EMAIL_KEEP_MESSAGE = False
    FILE_EXPORT_RIO_METADATA = False
    FILE_MAKE_DIRECTORY = False
    FILE_OVERWRITE_FILES = False
    FILE_PATIENT_SPEC_IF_ANONYMOUS = "anonymous"
    FINALIZED_ONLY = True
    HL7_DEBUG_DIVERT_TO_FILE = False
    HL7_DEBUG_TREAT_DIVERTED_AS_SENT = False
    HL7_KEEP_MESSAGE = False
    HL7_KEEP_REPLY = False
    HL7_NETWORK_TIMEOUT_MS = 10000
    HL7_PING_FIRST = True
    HL7_PORT = StandardPorts.HL7_MLLP
    INCLUDE_ANONYMOUS = False
    PUSH = False
    REQUIRE_PRIMARY_IDNUM_MANDATORY_IN_POLICY = True
    TASK_FORMAT = FileType.PDF
    XML_FIELD_COMMENTS = True

    def __init__(self, docker: bool = False) -> None:
        """
        Args:
            docker:
                Amend defaults so it works within a Docker Compose application
                without much fiddling?

        Defaults for use within Docker:

        - Note that a URL to another container/service looks like
          ``protocol://container:port/``. Values here must match the Docker
          Compose file.
        """
        if docker:
            self.CELERY_BROKER_URL = DockerConstants.CELERY_BROKER_URL
            self.CELERY_BEAT_SCHEDULE_DATABASE = os.path.join(
                DockerConstants.DEFAULT_LOCKDIR, "camcops_celerybeat_schedule")
            self.DB_SERVER = DockerConstants.CONTAINER_MYSQL
            self.DB_USER = DockerConstants.DEFAULT_MYSQL_CAMCOPS_USER
            self.EXPORT_LOCKDIR = DockerConstants.DEFAULT_LOCKDIR
            self.HOST = DockerConstants.HOST
            self.USER_DOWNLOAD_DIR = DockerConstants.DEFAULT_USER_DOWNLOAD_DIR

    @property
    def demo_db_url(self) -> str:
        return make_mysql_url(host=self.DB_SERVER,
                              port=self.DB_PORT,
                              username=self.DB_USER,
                              password=self.DB_PASSWORD,
                              dbname=self.DB_DATABASE)


MINIMUM_PASSWORD_LENGTH = 8
