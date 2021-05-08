#!/usr/bin/env python

# noinspection HttpUrlsUsage
"""
camcops_server/cc_modules/cc_config.py

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

**Read and represent a CamCOPS config file.**

Also contains various types of demonstration config file (CamCOPS, but also
``supervisord``, Apache, etc.) and demonstration helper scripts (e.g. MySQL).

There are CONDITIONAL AND IN-FUNCTION IMPORTS HERE; see below. This is to
minimize the number of modules loaded when this is used in the context of the
client-side database script, rather than the webview.

Moreover, it should not use SQLAlchemy objects directly; see ``celery.py``.

In particular, I tried hard to use a "database-unaware" (unbound) SQLAlchemy
ExportRecipient object. However, when the backend re-calls the config to get
its recipients, we get errors like:

.. code-block:: none

    [2018-12-25 00:56:00,118: ERROR/ForkPoolWorker-7] Task camcops_server.cc_modules.celery_tasks.export_to_recipient_backend[ab2e2691-c2fa-4821-b8cd-2cbeb86ddc8f] raised unexpected: DetachedInstanceError('Instance <ExportRecipient at 0x7febbeeea7b8> is not bound to a Session; attribute refresh operation cannot proceed',)
    Traceback (most recent call last):
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/celery/app/trace.py", line 382, in trace_task
        R = retval = fun(*args, **kwargs)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/celery/app/trace.py", line 641, in __protected_call__
        return self.run(*args, **kwargs)
      File "/home/rudolf/Documents/code/camcops/server/camcops_server/cc_modules/celery_tasks.py", line 103, in export_to_recipient_backend
        schedule_via_backend=False)
      File "/home/rudolf/Documents/code/camcops/server/camcops_server/cc_modules/cc_export.py", line 255, in export
        req, recipient_names=recipient_names, all_recipients=all_recipients)
      File "/home/rudolf/Documents/code/camcops/server/camcops_server/cc_modules/cc_config.py", line 1460, in get_export_recipients
        valid_names = set(r.recipient_name for r in recipients)
      File "/home/rudolf/Documents/code/camcops/server/camcops_server/cc_modules/cc_config.py", line 1460, in <genexpr>
        valid_names = set(r.recipient_name for r in recipients)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/sqlalchemy/orm/attributes.py", line 242, in __get__
        return self.impl.get(instance_state(instance), dict_)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/sqlalchemy/orm/attributes.py", line 594, in get
        value = state._load_expired(state, passive)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/sqlalchemy/orm/state.py", line 608, in _load_expired
        self.manager.deferred_scalar_loader(self, toload)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/sqlalchemy/orm/loading.py", line 813, in load_scalar_attributes
        (state_str(state)))
    sqlalchemy.orm.exc.DetachedInstanceError: Instance <ExportRecipient at 0x7febbeeea7b8> is not bound to a Session; attribute refresh operation cannot proceed (Background on this error at: http://sqlalche.me/e/bhk3)

"""  # noqa

import codecs
import collections
import configparser
import contextlib
import datetime
import os
import logging
import re
from typing import Any, Dict, Generator, List, Optional, Union

from cardinal_pythonlib.configfiles import (
    get_config_parameter,
    get_config_parameter_boolean,
    get_config_parameter_loglevel,
    get_config_parameter_multiline
)
from cardinal_pythonlib.docker import running_under_docker
from cardinal_pythonlib.fileops import relative_filename_within_dir
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.randomness import create_base64encoded_randomness
from cardinal_pythonlib.reprfunc import auto_repr
from cardinal_pythonlib.sqlalchemy.alembic_func import (
    get_current_and_head_revision,
)
from cardinal_pythonlib.sqlalchemy.engine_func import (
    is_sqlserver,
    is_sqlserver_2008_or_later,
)
from cardinal_pythonlib.sqlalchemy.logs import pre_disable_sqlalchemy_extra_echo_log  # noqa
from cardinal_pythonlib.sqlalchemy.schema import get_table_names
from cardinal_pythonlib.sqlalchemy.session import get_safe_url_from_engine
from cardinal_pythonlib.wsgi.reverse_proxied_mw import ReverseProxiedMiddleware
import celery.schedules
from sqlalchemy.engine import create_engine
from sqlalchemy.engine.base import Engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.orm import Session as SqlASession

from camcops_server.cc_modules.cc_baseconstants import (
    ALEMBIC_BASE_DIR,
    ALEMBIC_CONFIG_FILENAME,
    ALEMBIC_VERSION_TABLE,
    ENVVAR_CONFIG_FILE,
    LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR,
    ON_READTHEDOCS,
)
from camcops_server.cc_modules.cc_cache import cache_region_static, fkg
from camcops_server.cc_modules.cc_constants import (
    CONFIG_FILE_EXPORT_SECTION,
    CONFIG_FILE_SERVER_SECTION,
    CONFIG_FILE_SITE_SECTION,
    ConfigDefaults,
    ConfigParamExportGeneral,
    ConfigParamExportRecipient,
    ConfigParamServer,
    ConfigParamSite,
    DockerConstants,
)
from camcops_server.cc_modules.cc_exportrecipientinfo import (
    ExportRecipientInfo,
)
from camcops_server.cc_modules.cc_exception import raise_runtime_error
from camcops_server.cc_modules.cc_filename import (
    PatientSpecElementForFilename,
)
from camcops_server.cc_modules.cc_language import POSSIBLE_LOCALES
from camcops_server.cc_modules.cc_pyramid import MASTER_ROUTE_CLIENT_API
from camcops_server.cc_modules.cc_snomed import (
    get_all_task_snomed_concepts,
    get_icd9_snomed_concepts_from_xml,
    get_icd10_snomed_concepts_from_xml,
    SnomedConcept,
)
from camcops_server.cc_modules.cc_validators import (
    validate_export_recipient_name,
    validate_group_name,
)
from camcops_server.cc_modules.cc_version_string import (
    CAMCOPS_SERVER_VERSION_STRING,
)

log = BraceStyleAdapter(logging.getLogger(__name__))

pre_disable_sqlalchemy_extra_echo_log()

# =============================================================================
# Constants
# =============================================================================

VALID_RECIPIENT_NAME_REGEX = r"^[\w_-]+$"
# ... because we'll use them for filenames, amongst other things
# https://stackoverflow.com/questions/10944438/
# https://regexr.com/

# Windows paths: irrelevant, as Windows doesn't run supervisord
DEFAULT_LINUX_CAMCOPS_CONFIG = "/etc/camcops/camcops.conf"
DEFAULT_LINUX_CAMCOPS_BASE_DIR = "/usr/share/camcops"
DEFAULT_LINUX_CAMCOPS_VENV_DIR = os.path.join(
    DEFAULT_LINUX_CAMCOPS_BASE_DIR, "venv")
DEFAULT_LINUX_CAMCOPS_VENV_BIN_DIR = os.path.join(
    DEFAULT_LINUX_CAMCOPS_VENV_DIR, "bin")
DEFAULT_LINUX_CAMCOPS_EXECUTABLE = os.path.join(
    DEFAULT_LINUX_CAMCOPS_VENV_BIN_DIR, "camcops_server")
DEFAULT_LINUX_CAMCOPS_STATIC_DIR = os.path.join(
    DEFAULT_LINUX_CAMCOPS_VENV_DIR,
    "lib", "python3.6", "site-packages", "camcops_server", "static")
DEFAULT_LINUX_LOGDIR = "/var/log/supervisor"
DEFAULT_LINUX_USER = "www-data"  # Ubuntu default


# =============================================================================
# Helper functions
# =============================================================================

def warn_if_not_within_docker_dir(param_name: str,
                                  filespec: str,
                                  permit_cfg: bool = False,
                                  permit_venv: bool = False,
                                  permit_tmp: bool = False,
                                  param_contains_not_is: bool = False) -> None:
    """
    If the specified filename isn't within a relevant directory that will be
    used by CamCOPS when operating within a Docker Compose application, warn
    the user.

    Args:
        param_name:
            Name of the parameter in the CamCOPS config file.
        filespec:
            Filename (or filename-like thing) to check.
        permit_cfg:
            Permit the file to be in the configuration directory.
        permit_venv:
            Permit the file to be in the virtual environment directory.
        permit_tmp:
            Permit the file to be in the shared temporary space.
        param_contains_not_is:
            The parameter "contains", not "is", the filename.
    """
    if not filespec:
        return
    is_phrase = "contains" if param_contains_not_is else "is"
    permitted_dirs = []  # type: List[str]
    if permit_cfg:
        permitted_dirs.append(DockerConstants.CONFIG_DIR)
    if permit_venv:
        permitted_dirs.append(DockerConstants.VENV_DIR)
    if permit_tmp:
        permitted_dirs.append(DockerConstants.TMP_DIR)
    ok = any(
        relative_filename_within_dir(filespec, d)
        for d in permitted_dirs
    )
    if not ok:
        log.warning(
            f"Config parameter {param_name} {is_phrase} {filespec!r}, "
            f"which is not within the permitted Docker directories "
            f"{permitted_dirs!r}"
        )


def warn_if_not_docker_value(param_name: str,
                             actual_value: Any,
                             required_value: Any) -> None:
    """
    Warn the user if a parameter does not match the specific value required
    when operating under Docker.

    Args:
        param_name:
            Name of the parameter in the CamCOPS config file.
        actual_value:
            Value in the config file.
        required_value:
            Value that should be used.
    """
    if actual_value != required_value:
        log.warning(
            f"Config parameter {param_name} is {actual_value!r}, "
            f"but should be {required_value!r} when running inside Docker"
        )


def warn_if_not_present(param_name: str, value: Any) -> None:
    """
    Warn the user if a parameter is not set (None, or an empty string), for
    when operating under Docker.

    Args:
        param_name:
            Name of the parameter in the CamCOPS config file.
        value:
            Value in the config file.
    """
    if value is None or value == "":
        log.warning(
            f"Config parameter {param_name} is not specified, "
            f"but should be specified when running inside Docker"
        )


# =============================================================================
# Demo config
# =============================================================================

# Cosmetic demonstration constants:
DEFAULT_DB_READONLY_USER = 'QQQ_USERNAME_REPLACE_ME'
DEFAULT_DB_READONLY_PASSWORD = 'PPP_PASSWORD_REPLACE_ME'
DUMMY_INSTITUTION_URL = 'https://www.mydomain/'


def get_demo_config(for_docker: bool = False) -> str:
    """
    Returns a demonstration config file based on the specified parameters.

    Args:
        for_docker:
            Adjust defaults for the Docker environment.
    """
    # ...
    # http://www.debian.org/doc/debian-policy/ch-opersys.html#s-writing-init
    # https://people.canonical.com/~cjwatson/ubuntu-policy/policy.html/ch-opersys.html  # noqa
    session_cookie_secret = create_base64encoded_randomness(num_bytes=64)

    cd = ConfigDefaults(docker=for_docker)
    return f"""
# Demonstration CamCOPS server configuration file.
#
# Created by CamCOPS server version {CAMCOPS_SERVER_VERSION_STRING}.
# See help at https://camcops.readthedocs.io/.
#
# Using defaults for Docker environment: {for_docker}

# =============================================================================
# CamCOPS site
# =============================================================================

[{CONFIG_FILE_SITE_SECTION}]

# -----------------------------------------------------------------------------
# Database connection
# -----------------------------------------------------------------------------

{ConfigParamSite.DB_URL} = {cd.demo_db_url}
{ConfigParamSite.DB_ECHO} = {cd.DB_ECHO}

# -----------------------------------------------------------------------------
# URLs and paths
# -----------------------------------------------------------------------------

{ConfigParamSite.LOCAL_INSTITUTION_URL} = {DUMMY_INSTITUTION_URL}
{ConfigParamSite.LOCAL_LOGO_FILE_ABSOLUTE} = {cd.LOCAL_LOGO_FILE_ABSOLUTE}
{ConfigParamSite.CAMCOPS_LOGO_FILE_ABSOLUTE} = {cd.CAMCOPS_LOGO_FILE_ABSOLUTE}

{ConfigParamSite.EXTRA_STRING_FILES} = {cd.EXTRA_STRING_FILES}
{ConfigParamSite.RESTRICTED_TASKS} =
{ConfigParamSite.LANGUAGE} = {cd.LANGUAGE}

{ConfigParamSite.SNOMED_TASK_XML_FILENAME} =
{ConfigParamSite.SNOMED_ICD9_XML_FILENAME} =
{ConfigParamSite.SNOMED_ICD10_XML_FILENAME} =

{ConfigParamSite.WKHTMLTOPDF_FILENAME} =

# -----------------------------------------------------------------------------
# Login and session configuration
# -----------------------------------------------------------------------------

{ConfigParamSite.SESSION_COOKIE_SECRET} = camcops_autogenerated_secret_{session_cookie_secret}
{ConfigParamSite.SESSION_TIMEOUT_MINUTES} = {cd.SESSION_TIMEOUT_MINUTES}
{ConfigParamSite.PASSWORD_CHANGE_FREQUENCY_DAYS} = {cd.PASSWORD_CHANGE_FREQUENCY_DAYS}
{ConfigParamSite.LOCKOUT_THRESHOLD} = {cd.LOCKOUT_THRESHOLD}
{ConfigParamSite.LOCKOUT_DURATION_INCREMENT_MINUTES} = {cd.LOCKOUT_DURATION_INCREMENT_MINUTES}
{ConfigParamSite.DISABLE_PASSWORD_AUTOCOMPLETE} = {cd.DISABLE_PASSWORD_AUTOCOMPLETE}

# -----------------------------------------------------------------------------
# Suggested filenames for saving PDFs from the web view
# -----------------------------------------------------------------------------

{ConfigParamSite.PATIENT_SPEC_IF_ANONYMOUS} = {cd.PATIENT_SPEC_IF_ANONYMOUS}
{ConfigParamSite.PATIENT_SPEC} = {{{PatientSpecElementForFilename.SURNAME}}}_{{{PatientSpecElementForFilename.FORENAME}}}_{{{PatientSpecElementForFilename.ALLIDNUMS}}}

{ConfigParamSite.TASK_FILENAME_SPEC} = CamCOPS_{{patient}}_{{created}}_{{tasktype}}-{{serverpk}}.{{filetype}}
{ConfigParamSite.TRACKER_FILENAME_SPEC} = CamCOPS_{{patient}}_{{now}}_tracker.{{filetype}}
{ConfigParamSite.CTV_FILENAME_SPEC} = CamCOPS_{{patient}}_{{now}}_clinicaltextview.{{filetype}}

# -----------------------------------------------------------------------------
# E-mail options
# -----------------------------------------------------------------------------

{ConfigParamSite.EMAIL_HOST} = mysmtpserver.mydomain
{ConfigParamSite.EMAIL_PORT} = {cd.EMAIL_PORT}
{ConfigParamSite.EMAIL_USE_TLS} = {cd.EMAIL_USE_TLS}
{ConfigParamSite.EMAIL_HOST_USERNAME} = myusername
{ConfigParamSite.EMAIL_HOST_PASSWORD} = mypassword
{ConfigParamSite.EMAIL_FROM} = CamCOPS computer <noreply@myinstitution.mydomain>
{ConfigParamSite.EMAIL_SENDER} =
{ConfigParamSite.EMAIL_REPLY_TO} = CamCOPS clinical administrator <admin@myinstitution.mydomain>

# -----------------------------------------------------------------------------
# User download options
# -----------------------------------------------------------------------------

{ConfigParamSite.PERMIT_IMMEDIATE_DOWNLOADS} = {cd.PERMIT_IMMEDIATE_DOWNLOADS}
{ConfigParamSite.USER_DOWNLOAD_DIR} = {cd.USER_DOWNLOAD_DIR}
{ConfigParamSite.USER_DOWNLOAD_FILE_LIFETIME_MIN} = {cd.USER_DOWNLOAD_FILE_LIFETIME_MIN}
{ConfigParamSite.USER_DOWNLOAD_MAX_SPACE_MB} = {cd.USER_DOWNLOAD_MAX_SPACE_MB}

# -----------------------------------------------------------------------------
# Debugging options
# -----------------------------------------------------------------------------

{ConfigParamSite.WEBVIEW_LOGLEVEL} = {cd.WEBVIEW_LOGLEVEL_TEXTFORMAT}
{ConfigParamSite.CLIENT_API_LOGLEVEL} = {cd.CLIENT_API_LOGLEVEL_TEXTFORMAT}
{ConfigParamSite.ALLOW_INSECURE_COOKIES} = {cd.ALLOW_INSECURE_COOKIES}


# =============================================================================
# Web server options
# =============================================================================

[{CONFIG_FILE_SERVER_SECTION}]

# -----------------------------------------------------------------------------
# Common web server options
# -----------------------------------------------------------------------------

{ConfigParamServer.HOST} = {cd.HOST}
{ConfigParamServer.PORT} = {cd.PORT}
{ConfigParamServer.UNIX_DOMAIN_SOCKET} =

# If you host CamCOPS behind Apache, it’s likely that you’ll want Apache to
# handle HTTPS and CamCOPS to operate unencrypted behind a reverse proxy, in
# which case don’t set SSL_CERTIFICATE or SSL_PRIVATE_KEY.
{ConfigParamServer.SSL_CERTIFICATE} =
{ConfigParamServer.SSL_PRIVATE_KEY} =
{ConfigParamServer.STATIC_CACHE_DURATION_S} = {cd.STATIC_CACHE_DURATION_S}

# -----------------------------------------------------------------------------
# WSGI options
# -----------------------------------------------------------------------------

{ConfigParamServer.DEBUG_REVERSE_PROXY} = {cd.DEBUG_REVERSE_PROXY}
{ConfigParamServer.DEBUG_TOOLBAR} = {cd.DEBUG_TOOLBAR}
{ConfigParamServer.SHOW_REQUESTS} = {cd.SHOW_REQUESTS}
{ConfigParamServer.SHOW_REQUEST_IMMEDIATELY} = {cd.SHOW_REQUEST_IMMEDIATELY}
{ConfigParamServer.SHOW_RESPONSE} = {cd.SHOW_RESPONSE}
{ConfigParamServer.SHOW_TIMING} = {cd.SHOW_TIMING}
{ConfigParamServer.PROXY_HTTP_HOST} =
{ConfigParamServer.PROXY_REMOTE_ADDR} =
{ConfigParamServer.PROXY_REWRITE_PATH_INFO} = {cd.PROXY_REWRITE_PATH_INFO}
{ConfigParamServer.PROXY_SCRIPT_NAME} =
{ConfigParamServer.PROXY_SERVER_NAME} =
{ConfigParamServer.PROXY_SERVER_PORT} =
{ConfigParamServer.PROXY_URL_SCHEME} =
{ConfigParamServer.TRUSTED_PROXY_HEADERS} =
    HTTP_X_FORWARDED_HOST
    HTTP_X_FORWARDED_SERVER
    HTTP_X_FORWARDED_PORT
    HTTP_X_FORWARDED_PROTO
    HTTP_X_FORWARDED_FOR
    HTTP_X_SCRIPT_NAME

# -----------------------------------------------------------------------------
# CherryPy options
# -----------------------------------------------------------------------------

{ConfigParamServer.CHERRYPY_SERVER_NAME} = {cd.CHERRYPY_SERVER_NAME}
{ConfigParamServer.CHERRYPY_THREADS_START} = {cd.CHERRYPY_THREADS_START}
{ConfigParamServer.CHERRYPY_THREADS_MAX} = {cd.CHERRYPY_THREADS_MAX}
{ConfigParamServer.CHERRYPY_LOG_SCREEN} = {cd.CHERRYPY_LOG_SCREEN}
{ConfigParamServer.CHERRYPY_ROOT_PATH} = {cd.CHERRYPY_ROOT_PATH}

# -----------------------------------------------------------------------------
# Gunicorn options
# -----------------------------------------------------------------------------

{ConfigParamServer.GUNICORN_NUM_WORKERS} = {cd.GUNICORN_NUM_WORKERS}
{ConfigParamServer.GUNICORN_DEBUG_RELOAD} = {cd.GUNICORN_DEBUG_RELOAD}
{ConfigParamServer.GUNICORN_TIMEOUT_S} = {cd.GUNICORN_TIMEOUT_S}
{ConfigParamServer.DEBUG_SHOW_GUNICORN_OPTIONS} = {cd.DEBUG_SHOW_GUNICORN_OPTIONS}


# =============================================================================
# Export options
# =============================================================================

[{CONFIG_FILE_EXPORT_SECTION}]

{ConfigParamExportGeneral.CELERY_BEAT_EXTRA_ARGS} =
{ConfigParamExportGeneral.CELERY_BEAT_SCHEDULE_DATABASE} = {cd.CELERY_BEAT_SCHEDULE_DATABASE}
{ConfigParamExportGeneral.CELERY_BROKER_URL} = {cd.CELERY_BROKER_URL}
{ConfigParamExportGeneral.CELERY_WORKER_EXTRA_ARGS} =
    --maxtasksperchild=1000
{ConfigParamExportGeneral.CELERY_EXPORT_TASK_RATE_LIMIT} = 100/m
{ConfigParamExportGeneral.EXPORT_LOCKDIR} = {cd.EXPORT_LOCKDIR}

{ConfigParamExportGeneral.RECIPIENTS} =

{ConfigParamExportGeneral.SCHEDULE_TIMEZONE} = {cd.SCHEDULE_TIMEZONE}
{ConfigParamExportGeneral.SCHEDULE} =


# =============================================================================
# Details for each export recipient
# =============================================================================

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Example recipient
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Example (disabled because it's not in the {ConfigParamExportGeneral.RECIPIENTS} list above)

[recipient:recipient_A]

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # How to export
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.TRANSMISSION_METHOD} = hl7
{ConfigParamExportRecipient.PUSH} = true
{ConfigParamExportRecipient.TASK_FORMAT} = pdf
{ConfigParamExportRecipient.XML_FIELD_COMMENTS} = {cd.XML_FIELD_COMMENTS}

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # What to export
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.ALL_GROUPS} = false
{ConfigParamExportRecipient.GROUPS} =
    myfirstgroup
    mysecondgroup
{ConfigParamExportRecipient.TASKS} =

{ConfigParamExportRecipient.START_DATETIME_UTC} =
{ConfigParamExportRecipient.END_DATETIME_UTC} =
{ConfigParamExportRecipient.FINALIZED_ONLY} = {cd.FINALIZED_ONLY}
{ConfigParamExportRecipient.INCLUDE_ANONYMOUS} = {cd.INCLUDE_ANONYMOUS}
{ConfigParamExportRecipient.PRIMARY_IDNUM} = 1
{ConfigParamExportRecipient.REQUIRE_PRIMARY_IDNUM_MANDATORY_IN_POLICY} = {cd.REQUIRE_PRIMARY_IDNUM_MANDATORY_IN_POLICY}

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Options applicable to database exports
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.DB_URL} = some_sqlalchemy_url
{ConfigParamExportRecipient.DB_ECHO} = {cd.DB_ECHO}
{ConfigParamExportRecipient.DB_INCLUDE_BLOBS} = {cd.DB_INCLUDE_BLOBS}
{ConfigParamExportRecipient.DB_ADD_SUMMARIES} = {cd.DB_ADD_SUMMARIES}
{ConfigParamExportRecipient.DB_PATIENT_ID_PER_ROW} = {cd.DB_PATIENT_ID_PER_ROW}

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Options applicable to e-mail exports
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.EMAIL_TO} =
    Perinatal Psychiatry Admin <perinatal@myinstitution.mydomain>

{ConfigParamExportRecipient.EMAIL_CC} =
    Dr Alice Bradford <alice.bradford@myinstitution.mydomain>
    Dr Charles Dogfoot <charles.dogfoot@myinstitution.mydomain>

{ConfigParamExportRecipient.EMAIL_BCC} =
    superuser <root@myinstitution.mydomain>

{ConfigParamExportRecipient.EMAIL_PATIENT_SPEC_IF_ANONYMOUS} = anonymous
{ConfigParamExportRecipient.EMAIL_PATIENT_SPEC} = {{{PatientSpecElementForFilename.SURNAME}}}, {{{PatientSpecElementForFilename.FORENAME}}}, {{{PatientSpecElementForFilename.ALLIDNUMS}}}
{ConfigParamExportRecipient.EMAIL_SUBJECT} = CamCOPS task for {{patient}}, created {{created}}: {{tasktype}}, PK {{serverpk}}
{ConfigParamExportRecipient.EMAIL_BODY_IS_HTML} = false
{ConfigParamExportRecipient.EMAIL_BODY} =
    Please find attached a new CamCOPS task for manual filing to the electronic
    patient record of

        {{patient}}

    Task type: {{tasktype}}
    Created: {{created}}
    CamCOPS server primary key: {{serverpk}}

    Yours faithfully,

    The CamCOPS computer.

{ConfigParamExportRecipient.EMAIL_KEEP_MESSAGE} = {cd.HL7_KEEP_MESSAGE}

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Options applicable to HL7
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.HL7_HOST} = myhl7server.mydomain
{ConfigParamExportRecipient.HL7_PORT} = {cd.HL7_PORT}
{ConfigParamExportRecipient.HL7_PING_FIRST} = {cd.HL7_PING_FIRST}
{ConfigParamExportRecipient.HL7_NETWORK_TIMEOUT_MS} = {cd.HL7_NETWORK_TIMEOUT_MS}
{ConfigParamExportRecipient.HL7_KEEP_MESSAGE} = {cd.HL7_KEEP_MESSAGE}
{ConfigParamExportRecipient.HL7_KEEP_REPLY} = {cd.HL7_KEEP_REPLY}
{ConfigParamExportRecipient.HL7_DEBUG_DIVERT_TO_FILE} = {cd.HL7_DEBUG_DIVERT_TO_FILE}
{ConfigParamExportRecipient.HL7_DEBUG_TREAT_DIVERTED_AS_SENT} = {cd.HL7_DEBUG_TREAT_DIVERTED_AS_SENT}

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Options applicable to file transfers/attachments
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.FILE_PATIENT_SPEC} = {{surname}}_{{forename}}_{{idshortdesc1}}{{idnum1}}
{ConfigParamExportRecipient.FILE_PATIENT_SPEC_IF_ANONYMOUS} = {cd.FILE_PATIENT_SPEC_IF_ANONYMOUS}
{ConfigParamExportRecipient.FILE_FILENAME_SPEC} = /my_nfs_mount/mypath/CamCOPS_{{patient}}_{{created}}_{{tasktype}}-{{serverpk}}.{{filetype}}
{ConfigParamExportRecipient.FILE_MAKE_DIRECTORY} = {cd.FILE_MAKE_DIRECTORY}
{ConfigParamExportRecipient.FILE_OVERWRITE_FILES} = {cd.FILE_OVERWRITE_FILES}
{ConfigParamExportRecipient.FILE_EXPORT_RIO_METADATA} = {cd.FILE_EXPORT_RIO_METADATA}
{ConfigParamExportRecipient.FILE_SCRIPT_AFTER_EXPORT} =

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Extra options for RiO metadata for file-based export
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.RIO_IDNUM} = 2
{ConfigParamExportRecipient.RIO_UPLOADING_USER} = CamCOPS
{ConfigParamExportRecipient.RIO_DOCUMENT_TYPE} = CC

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # Extra options for REDCap export
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

{ConfigParamExportRecipient.REDCAP_API_URL} = https://domain.of.redcap.server/api/
{ConfigParamExportRecipient.REDCAP_API_KEY} = myapikey
{ConfigParamExportRecipient.REDCAP_FIELDMAP_FILENAME} = /location/of/fieldmap.xml

    """.strip()  # noqa


# =============================================================================
# Demo configuration files, other than the CamCOPS config file itself
# =============================================================================

DEFAULT_SOCKET_FILENAME = "/run/camcops/camcops.socket"


def get_demo_supervisor_config() -> str:
    """
    Returns a demonstration ``supervisord`` config file based on the
    specified parameters.
    """
    redirect_stderr = "true"
    autostart = "true"
    autorestart = "true"
    startsecs = "30"
    stopwaitsecs = "60"
    return f"""
# =============================================================================
# Demonstration 'supervisor' (supervisord) config file for CamCOPS.
# Created by CamCOPS version {CAMCOPS_SERVER_VERSION_STRING}.
# =============================================================================
# See https://camcops.readthedocs.io/en/latest/administrator/server_configuration.html#start-camcops

[program:camcops_server]

command = {DEFAULT_LINUX_CAMCOPS_EXECUTABLE} serve_gunicorn
    --config {DEFAULT_LINUX_CAMCOPS_CONFIG}

directory = {DEFAULT_LINUX_CAMCOPS_BASE_DIR}
environment = MPLCONFIGDIR="{LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR}"
user = {DEFAULT_LINUX_USER}
stdout_logfile = {DEFAULT_LINUX_LOGDIR}/camcops_server.log
redirect_stderr = {redirect_stderr}
autostart = {autostart}
autorestart = {autorestart}
startsecs = {startsecs}
stopwaitsecs = {stopwaitsecs}

[program:camcops_workers]

command = {DEFAULT_LINUX_CAMCOPS_EXECUTABLE} launch_workers
    --config {DEFAULT_LINUX_CAMCOPS_CONFIG}

directory = {DEFAULT_LINUX_CAMCOPS_BASE_DIR}
environment = MPLCONFIGDIR="{LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR}"
user = {DEFAULT_LINUX_USER}
stdout_logfile = {DEFAULT_LINUX_LOGDIR}/camcops_workers.log
redirect_stderr = {redirect_stderr}
autostart = {autostart}
autorestart = {autorestart}
startsecs = {startsecs}
stopwaitsecs = {stopwaitsecs}

[program:camcops_scheduler]

command = {DEFAULT_LINUX_CAMCOPS_EXECUTABLE} launch_scheduler
    --config {DEFAULT_LINUX_CAMCOPS_CONFIG}

directory = {DEFAULT_LINUX_CAMCOPS_BASE_DIR}
environment = MPLCONFIGDIR="{LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR}"
user = {DEFAULT_LINUX_USER}
stdout_logfile = {DEFAULT_LINUX_LOGDIR}/camcops_scheduler.log
redirect_stderr = {redirect_stderr}
autostart = {autostart}
autorestart = {autorestart}
startsecs = {startsecs}
stopwaitsecs = {stopwaitsecs}

[group:camcops]

programs = camcops_server, camcops_workers, camcops_scheduler

    """.strip()  # noqa


def get_demo_apache_config(
        rootpath: str = "camcops",  # no slash
        specimen_internal_port: int = None,
        specimen_socket_file: str = DEFAULT_SOCKET_FILENAME) -> str:
    """
    Returns a demo Apache HTTPD config file section applicable to CamCOPS.
    """
    cd = ConfigDefaults()
    specimen_internal_port = specimen_internal_port or cd.PORT
    urlbase = "/" + rootpath
    # noinspection HttpUrlsUsage
    return f"""
# Demonstration Apache config file section for CamCOPS.
# Created by CamCOPS version {CAMCOPS_SERVER_VERSION_STRING}.
#
# Under Ubuntu, the Apache config will be somewhere in /etc/apache2/
# Under CentOS, the Apache config will be somewhere in /etc/httpd/
#
# This section should go within the <VirtualHost> directive for the secure
# (SSL, HTTPS) part of the web site.

<VirtualHost *:443>
    # ...

    # =========================================================================
    # CamCOPS
    # =========================================================================
    # Apache operates on the principle that the first match wins. So, if we
    # want to serve CamCOPS but then override some of its URLs to serve static
    # files faster, we define the static stuff first.

        # ---------------------------------------------------------------------
        # 1. Serve static files
        # ---------------------------------------------------------------------
        # a) offer them at the appropriate URL
        # b) provide permission
        # c) disable ProxyPass for static files

        # CHANGE THIS: aim the alias at your own institutional logo.

    Alias {urlbase}/static/logo_local.png {DEFAULT_LINUX_CAMCOPS_STATIC_DIR}/logo_local.png

        # We move from more specific to less specific aliases; the first match
        # takes precedence. (Apache will warn about conflicting aliases if
        # specified in a wrong, less-to-more-specific, order.)

    Alias {urlbase}/static/ {DEFAULT_LINUX_CAMCOPS_STATIC_DIR}/

    <Directory {DEFAULT_LINUX_CAMCOPS_STATIC_DIR}>
        Require all granted

        # ... for old Apache versions (e.g. 2.2), use instead:
        # Order allow,deny
        # Allow from all
    </Directory>

        # Don't ProxyPass the static files; we'll serve them via Apache.

    ProxyPassMatch ^{urlbase}/static/ !

        # ---------------------------------------------------------------------
        # 2. Proxy requests to the CamCOPS web server and back; allow access
        # ---------------------------------------------------------------------
        # ... either via an internal TCP/IP port (e.g. 1024 or higher, and NOT
        #     accessible to users);
        # ... or, better, via a Unix socket, e.g. {specimen_socket_file}
        #
        # NOTES
        #
        # - When you ProxyPass {urlbase}, you should browse to
        #
        #       https://YOURSITE{urlbase}
        #
        #   and point your tablet devices to
        #
        #       https://YOURSITE{urlbase}{MASTER_ROUTE_CLIENT_API}
        #
        # - Don't specify trailing slashes for the ProxyPass and
        #   ProxyPassReverse directives.
        #   If you do, http://host/camcops will fail though
        #              http://host/camcops/ will succeed.
        #
        #   - An alternative fix is to enable mod_rewrite (e.g. sudo a2enmod
        #     rewrite), then add these commands:
        #
        #       RewriteEngine on
        #       RewriteRule ^/{rootpath}$ {rootpath}/ [L,R=301]
        #
        #     which will redirect requests without the trailing slash to a
        #     version with the trailing slash.
        #
        # - Ensure that you put the CORRECT PROTOCOL (http, https) in the rules
        #   below.
        #
        # - For ProxyPass options, see https://httpd.apache.org/docs/2.2/mod/mod_proxy.html#proxypass
        #
        #   - Include "retry=0" to stop Apache disabling the connection for
        #     while on failure.
        #   - Consider adding a "timeout=<seconds>" option if the back-end is
        #     slow and causing timeouts.
        #
        # - CamCOPS MUST BE TOLD about its location and protocol, because that
        #   information is critical for synthesizing URLs, but is stripped out
        #   by the reverse proxy system. There are two ways:
        #
        #   (i)  specifying headers or WSGI environment variables, such as
        #        the HTTP(S) headers X-Forwarded-Proto and X-Script-Name below
        #        (and telling CamCOPS to trust them via its
        #        TRUSTED_PROXY_HEADERS setting);
        #
        #   (ii) specifying other options to "camcops_server", including
        #        PROXY_SCRIPT_NAME, PROXY_URL_SCHEME; see the help for the
        #        CamCOPS config.
        #
        # So:
        #
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # (a) Reverse proxy
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        #
        # #####################################################################
        # PORT METHOD
        # #####################################################################
        # Note the use of "http" (reflecting the backend), not https (like the
        # front end).

    # ProxyPass {urlbase} http://127.0.0.1:{specimen_internal_port} retry=0 timeout=300
    # ProxyPassReverse {urlbase} http://127.0.0.1:{specimen_internal_port}

        # #####################################################################
        # UNIX SOCKET METHOD (Apache 2.4.9 and higher)
        # #####################################################################
        # This requires Apache 2.4.9, and passes after the '|' character a URL
        # that determines the Host: value of the request; see
        # ://httpd.apache.org/docs/trunk/mod/mod_proxy.html#proxypass
        #
        # The general syntax is:
        #
        #   ProxyPass /URL_USER_SEES unix:SOCKETFILE|PROTOCOL://HOST/EXTRA_URL_FOR_BACKEND retry=0
        #
        # Note that:
        #
        #   - the protocol should be http, not https (Apache deals with the
        #     HTTPS part and passes HTTP on)
        #   - the EXTRA_URL_FOR_BACKEND needs to be (a) unique for each
        #     instance or Apache will use a single worker for multiple
        #     instances, and (b) blank for the backend's benefit. Since those
        #     two conflict when there's >1 instance, there's a problem.
        #   - Normally, HOST is given as localhost. It may be that this problem
        #     is solved by using a dummy unique value for HOST:
        #     https://bz.apache.org/bugzilla/show_bug.cgi?id=54101#c1
        #
        # If your Apache version is too old, you will get the error
        #
        #   "AH00526: Syntax error on line 56 of /etc/apache2/sites-enabled/SOMETHING:
        #    ProxyPass URL must be absolute!"
        #
        # If you get this error:
        #
        #   AH01146: Ignoring parameter 'retry=0' for worker 'unix:/tmp/.camcops_gunicorn.sock|https://localhost' because of worker sharing
        #   https://wiki.apache.org/httpd/ListOfErrors
        #
        # ... then your URLs are overlapping and should be redone or sorted;
        # see http://httpd.apache.org/docs/2.4/mod/mod_proxy.html#workers
        #
        # The part that must be unique for each instance, with no part a
        # leading substring of any other, is THIS_BIT in:
        #
        #   ProxyPass /URL_USER_SEES unix:SOCKETFILE|http://localhost/THIS_BIT retry=0
        #
        # If you get an error like this:
        #
        #   AH01144: No protocol handler was valid for the URL /SOMEWHERE. If you are using a DSO version of mod_proxy, make sure the proxy submodules are included in the configuration using LoadModule.
        #
        # Then do this:
        #
        #   sudo a2enmod proxy proxy_http
        #   sudo apache2ctl restart
        #
        # If you get an error like this:
        #
        #   ... [proxy_http:error] [pid 32747] (103)Software caused connection abort: [client 109.151.49.173:56898] AH01102: error reading status line from remote server httpd-UDS:0
        #       [proxy:error] [pid 32747] [client 109.151.49.173:56898] AH00898: Error reading from remote server returned by /camcops_bruhl/webview
        #
        # then check you are specifying http://, not https://, in the ProxyPass
        #
        # Other information sources:
        #
        # - https://emptyhammock.com/projects/info/pyweb/webconfig.html

    ProxyPass {urlbase} unix:{specimen_socket_file}|http://dummy1 retry=0 timeout=300
    ProxyPassReverse {urlbase} unix:{specimen_socket_file}|http://dummy1

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # (b) Allow proxy over SSL.
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Without this, you will get errors like:
        #   ... SSL Proxy requested for wombat:443 but not enabled [Hint: SSLProxyEngine]
        #   ... failed to enable ssl support for 0.0.0.0:0 (httpd-UDS)

    SSLProxyEngine on

    <Location /camcops>

            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            # (c) Allow access
            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        Require all granted

            # ... for old Apache versions (e.g. 2.2), use instead:
            #
            #   Order allow,deny
            #   Allow from all

            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            # (d) Tell the proxied application that we are using HTTPS, and
            #     where the application is installed
            # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            #     ... https://stackoverflow.com/questions/16042647
            #
            # Enable mod_headers (e.g. "sudo a2enmod headers") and set:

        RequestHeader set X-Forwarded-Proto https
        RequestHeader set X-Script-Name {urlbase}

            # ... then ensure the TRUSTED_PROXY_HEADERS setting in the CamCOPS
            # config file includes:
            #
            #           HTTP_X_FORWARDED_HOST
            #           HTTP_X_FORWARDED_SERVER
            #           HTTP_X_FORWARDED_PORT
            #           HTTP_X_FORWARDED_PROTO
            #           HTTP_X_SCRIPT_NAME
            #
            # (X-Forwarded-For, X-Forwarded-Host, and X-Forwarded-Server are
            # supplied by Apache automatically.)

    </Location>

    #==========================================================================
    # SSL security (for HTTPS)
    #==========================================================================

        # You will also need to install your SSL certificate; see the
        # instructions that came with it. You get a certificate by creating a
        # certificate signing request (CSR). You enter some details about your
        # site, and a software tool makes (1) a private key, which you keep
        # utterly private, and (2) a CSR, which you send to a Certificate
        # Authority (CA) for signing. They send back a signed certificate, and
        # a chain of certificates leading from yours to a trusted root CA.
        #
        # You can create your own (a 'snake-oil' certificate), but your tablets
        # and browsers will not trust it, so this is a bad idea.
        #
        # Once you have your certificate: edit and uncomment these lines:

    # SSLEngine on

    # SSLCertificateKeyFile /etc/ssl/private/my.private.key

        # ... a private file that you made before creating the certificate
        # request, and NEVER GAVE TO ANYBODY, and NEVER WILL (or your
        # security is broken and you need a new certificate).

    # SSLCertificateFile /etc/ssl/certs/my.public.cert

        # ... signed and supplied to you by the certificate authority (CA),
        # from the public certificate you sent to them.

    # SSLCertificateChainFile /etc/ssl/certs/my-institution.ca-bundle

        # ... made from additional certificates in a chain, supplied to you by
        # the CA. For example, mine is univcam.ca-bundle, made with the
        # command:
        #
        # cat TERENASSLCA.crt UTNAddTrustServer_CA.crt AddTrustExternalCARoot.crt > univcam.ca-bundle

</VirtualHost>

    """.strip()  # noqa


# =============================================================================
# Helper functions
# =============================================================================

def raise_missing(section: str, parameter: str) -> None:
    msg = (
        f"Config file: missing/blank parameter {parameter} "
        f"in section [{section}]"
    )
    raise_runtime_error(msg)


# =============================================================================
# CrontabEntry
# =============================================================================

class CrontabEntry(object):
    """
    Class to represent a ``crontab``-style entry.
    """
    def __init__(self,
                 line: str = None,
                 minute: Union[str, int, List[int]] = "*",
                 hour: Union[str, int, List[int]] = "*",
                 day_of_week: Union[str, int, List[int]] = "*",
                 day_of_month: Union[str, int, List[int]] = "*",
                 month_of_year: Union[str, int, List[int]] = "*",
                 content: str = None) -> None:
        """
        Args:
            line:
                line of the form ``m h dow dom moy content content content``.
            minute:
                crontab "minute" entry
            hour:
                crontab "hour" entry
            day_of_week:
                crontab "day_of_week" entry
            day_of_month:
                crontab "day_of_month" entry
            month_of_year:
                crontab "month_of_year" entry
            content:
                crontab "thing to run" entry

        If ``line`` is specified, it is used. Otherwise, the components are
        used; the default for each of them is ``"*"``, meaning "all". Thus, for
        example, you can specify ``minute="*/5"`` and that is sufficient to
        mean "every 5 minutes".
        """
        has_line = line is not None
        has_components = bool(minute and hour and day_of_week and
                              day_of_month and month_of_year and content)
        assert has_line or has_components, (
            "Specify either a crontab line or all the time components"
        )
        if has_line:
            line = line.split("#")[0].strip()  # everything before a '#'
            components = line.split()  # split on whitespace
            assert len(components) >= 6, (
                "Must specify 5 time components and then contents"
            )
            minute, hour, day_of_week, day_of_month, month_of_year = (
                components[0:5]
            )
            content = " ".join(components[5:])

        self.minute = minute
        self.hour = hour
        self.day_of_week = day_of_week
        self.day_of_month = day_of_month
        self.month_of_year = month_of_year
        self.content = content

    def __repr__(self) -> str:
        return auto_repr(self, sort_attrs=False)

    def __str__(self) -> str:
        return (
            f"{self.minute} {self.hour} {self.day_of_week} "
            f"{self.day_of_month} {self.month_of_year} {self.content}"
        )

    def get_celery_schedule(self) -> celery.schedules.crontab:
        """
        Returns the corresponding Celery schedule.

        Returns:
            a :class:`celery.schedules.crontab`

        Raises:
            :exc:`celery.schedules.ParseException` if the input can't be parsed
        """
        return celery.schedules.crontab(
            minute=self.minute,
            hour=self.hour,
            day_of_week=self.day_of_week,
            day_of_month=self.day_of_month,
            month_of_year=self.month_of_year,
        )


# =============================================================================
# Configuration class. (It gets cached on a per-process basis.)
# =============================================================================

class CamcopsConfig(object):
    """
    Class representing the CamCOPS configuration.
    """

    def __init__(self,
                 config_filename: str,
                 config_text: str = None) -> None:
        """
        Initialize by reading the config file.

        Args:
            config_filename:
                Filename of the config file (usual method)
            config_text:
                Text contents of the config file (alternative method for
                special circumstances); overrides ``config_filename``
        """
        def _get_str(section: str, paramname: str,
                     default: str = None) -> Optional[str]:
            return get_config_parameter(
                parser, section, paramname, str, default)

        def _get_bool(section: str, paramname: str, default: bool) -> bool:
            return get_config_parameter_boolean(
                parser, section, paramname, default)

        def _get_int(section: str, paramname: str,
                     default: int = None) -> Optional[int]:
            return get_config_parameter(
                parser, section, paramname, int, default)

        def _get_multiline(section: str, paramname: str) -> List[str]:
            # http://stackoverflow.com/questions/335695/lists-in-configparser
            return get_config_parameter_multiline(
                parser, section, paramname, [])

        def _get_multiline_ignoring_comments(section: str,
                                             paramname: str) -> List[str]:
            # Returns lines with any trailing comments removed, and any
            # comment-only lines removed.
            lines = _get_multiline(section, paramname)
            return list(filter(None,
                               (x.split("#")[0].strip() for x in lines if x)))

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Learn something about our environment
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        self.running_under_docker = running_under_docker()

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Open config file
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        self.camcops_config_filename = config_filename
        parser = configparser.ConfigParser()

        if config_text:
            log.info("Reading config from supplied string")
            parser.read_string(config_text)
        else:
            if not config_filename:
                raise AssertionError(
                    f"Environment variable {ENVVAR_CONFIG_FILE} not specified "
                    f"(and no command-line alternative given)")
            log.info("Reading from config file: {!r}", config_filename)
            with codecs.open(config_filename, "r", "utf8") as file:
                parser.read_file(file)

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Main section (in alphabetical order)
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        s = CONFIG_FILE_SITE_SECTION
        cs = ConfigParamSite
        cd = ConfigDefaults()

        self.allow_insecure_cookies = _get_bool(
            s, cs.ALLOW_INSECURE_COOKIES, cd.ALLOW_INSECURE_COOKIES)

        self.camcops_logo_file_absolute = _get_str(
            s, cs.CAMCOPS_LOGO_FILE_ABSOLUTE, cd.CAMCOPS_LOGO_FILE_ABSOLUTE)
        self.ctv_filename_spec = _get_str(s, cs.CTV_FILENAME_SPEC)

        self.db_url = parser.get(s, cs.DB_URL)
        # ... no default: will fail if not provided
        self.db_echo = _get_bool(s, cs.DB_ECHO, cd.DB_ECHO)
        self.client_api_loglevel = get_config_parameter_loglevel(
            parser, s, cs.CLIENT_API_LOGLEVEL, cd.CLIENT_API_LOGLEVEL)
        logging.getLogger("camcops_server.cc_modules.client_api")\
            .setLevel(self.client_api_loglevel)
        # ... MUTABLE GLOBAL STATE (if relatively unimportant); todo: fix

        self.disable_password_autocomplete = _get_bool(
            s, cs.DISABLE_PASSWORD_AUTOCOMPLETE,
            cd.DISABLE_PASSWORD_AUTOCOMPLETE)

        self.email_host = _get_str(s, cs.EMAIL_HOST, "")
        self.email_port = _get_int(s, cs.EMAIL_PORT, cd.EMAIL_PORT)
        self.email_use_tls = _get_bool(s, cs.EMAIL_USE_TLS, cd.EMAIL_USE_TLS)
        self.email_host_username = _get_str(s, cs.EMAIL_HOST_USERNAME, "")
        self.email_host_password = _get_str(s, cs.EMAIL_HOST_PASSWORD, "")

        self.email_from = _get_str(s, cs.EMAIL_FROM, "")
        self.email_sender = _get_str(s, cs.EMAIL_SENDER, "")
        self.email_reply_to = _get_str(s, cs.EMAIL_REPLY_TO, "")

        self.extra_string_files = _get_multiline(s, cs.EXTRA_STRING_FILES)

        self.language = _get_str(s, cs.LANGUAGE, cd.LANGUAGE)
        if self.language not in POSSIBLE_LOCALES:
            log.warning(f"Invalid language {self.language!r}, "
                        f"switching to {cd.LANGUAGE!r}")
            self.language = cd.LANGUAGE
        self.local_institution_url = _get_str(
            s, cs.LOCAL_INSTITUTION_URL, cd.LOCAL_INSTITUTION_URL)
        self.local_logo_file_absolute = _get_str(
            s, cs.LOCAL_LOGO_FILE_ABSOLUTE, cd.LOCAL_LOGO_FILE_ABSOLUTE)
        self.lockout_threshold = _get_int(
            s, cs.LOCKOUT_THRESHOLD, cd.LOCKOUT_THRESHOLD)
        self.lockout_duration_increment_minutes = _get_int(
            s, cs.LOCKOUT_DURATION_INCREMENT_MINUTES,
            cd.LOCKOUT_DURATION_INCREMENT_MINUTES)

        self.password_change_frequency_days = _get_int(
            s, cs.PASSWORD_CHANGE_FREQUENCY_DAYS,
            cd.PASSWORD_CHANGE_FREQUENCY_DAYS)
        self.patient_spec_if_anonymous = _get_str(
            s, cs.PATIENT_SPEC_IF_ANONYMOUS, cd.PATIENT_SPEC_IF_ANONYMOUS)
        self.patient_spec = _get_str(s, cs.PATIENT_SPEC)
        self.permit_immediate_downloads = _get_bool(
            s, cs.PERMIT_IMMEDIATE_DOWNLOADS,
            cd.PERMIT_IMMEDIATE_DOWNLOADS)
        # currently not configurable, but easy to add in the future:
        self.plot_fontsize = cd.PLOT_FONTSIZE

        self.restricted_tasks = {}  # type: Dict[str, List[str]]
        # ... maps XML task names to lists of authorized group names
        restricted_tasks = _get_multiline(s, cs.RESTRICTED_TASKS)
        for rt_line in restricted_tasks:
            rt_line = rt_line.split("#")[0].strip()
            # ... everything before a '#'
            if not rt_line:  # comment line
                continue
            try:
                xml_taskname, groupnames = rt_line.split(":")
            except ValueError:
                raise ValueError(
                    f"Restricted tasks line not in the format "
                    f"'xml_taskname: groupname1, groupname2, ...'. Line was:\n"
                    f"{rt_line!r}"
                )
            xml_taskname = xml_taskname.strip()
            if xml_taskname in self.restricted_tasks:
                raise ValueError(f"Duplicate restricted task specification "
                                 f"for {xml_taskname!r}")
            groupnames = [x.strip() for x in groupnames.split(",")]
            for gn in groupnames:
                validate_group_name(gn)
            self.restricted_tasks[xml_taskname] = groupnames

        self.session_timeout_minutes = _get_int(
            s, cs.SESSION_TIMEOUT_MINUTES, cd.SESSION_TIMEOUT_MINUTES)
        self.session_cookie_secret = _get_str(s, cs.SESSION_COOKIE_SECRET)
        self.session_timeout = datetime.timedelta(
            minutes=self.session_timeout_minutes)
        self.snomed_task_xml_filename = _get_str(
            s, cs.SNOMED_TASK_XML_FILENAME)
        self.snomed_icd9_xml_filename = _get_str(
            s, cs.SNOMED_ICD9_XML_FILENAME)
        self.snomed_icd10_xml_filename = _get_str(
            s, cs.SNOMED_ICD10_XML_FILENAME)

        self.task_filename_spec = _get_str(s, cs.TASK_FILENAME_SPEC)
        self.tracker_filename_spec = _get_str(s, cs.TRACKER_FILENAME_SPEC)

        self.user_download_dir = _get_str(s, cs.USER_DOWNLOAD_DIR, "")
        self.user_download_file_lifetime_min = _get_int(
            s, cs.USER_DOWNLOAD_FILE_LIFETIME_MIN,
            cd.USER_DOWNLOAD_FILE_LIFETIME_MIN)
        self.user_download_max_space_mb = _get_int(
            s, cs.USER_DOWNLOAD_MAX_SPACE_MB,
            cd.USER_DOWNLOAD_MAX_SPACE_MB)

        self.webview_loglevel = get_config_parameter_loglevel(
            parser, s, cs.WEBVIEW_LOGLEVEL, cd.WEBVIEW_LOGLEVEL)
        logging.getLogger().setLevel(self.webview_loglevel)  # root logger
        # ... MUTABLE GLOBAL STATE (if relatively unimportant); todo: fix
        self.wkhtmltopdf_filename = _get_str(s, cs.WKHTMLTOPDF_FILENAME)

        # More validity checks for the main section:
        if not self.patient_spec_if_anonymous:
            raise_missing(s, cs.PATIENT_SPEC_IF_ANONYMOUS)
        if not self.patient_spec:
            raise_missing(s, cs.PATIENT_SPEC)
        if not self.session_cookie_secret:
            raise_missing(s, cs.SESSION_COOKIE_SECRET)
        if not self.task_filename_spec:
            raise_missing(s, cs.TASK_FILENAME_SPEC)
        if not self.tracker_filename_spec:
            raise_missing(s, cs.TRACKER_FILENAME_SPEC)
        if not self.ctv_filename_spec:
            raise_missing(s, cs.CTV_FILENAME_SPEC)

        # To prevent errors:
        del s
        del cs

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Web server/WSGI section
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        ws = CONFIG_FILE_SERVER_SECTION
        cw = ConfigParamServer

        self.cherrypy_log_screen = _get_bool(ws, cw.CHERRYPY_LOG_SCREEN,
                                             cd.CHERRYPY_LOG_SCREEN)
        self.cherrypy_root_path = _get_str(
            ws, cw.CHERRYPY_ROOT_PATH, cd.CHERRYPY_ROOT_PATH)
        self.cherrypy_server_name = _get_str(
            ws, cw.CHERRYPY_SERVER_NAME, cd.CHERRYPY_SERVER_NAME)
        self.cherrypy_threads_max = _get_int(
            ws, cw.CHERRYPY_THREADS_MAX, cd.CHERRYPY_THREADS_MAX)
        self.cherrypy_threads_start = _get_int(
            ws, cw.CHERRYPY_THREADS_START, cd.CHERRYPY_THREADS_START)
        self.debug_reverse_proxy = _get_bool(ws, cw.DEBUG_REVERSE_PROXY,
                                             cd.DEBUG_REVERSE_PROXY)
        self.debug_show_gunicorn_options = _get_bool(
            ws, cw.DEBUG_SHOW_GUNICORN_OPTIONS, cd.DEBUG_SHOW_GUNICORN_OPTIONS)
        self.debug_toolbar = _get_bool(ws, cw.DEBUG_TOOLBAR, cd.DEBUG_TOOLBAR)
        self.gunicorn_debug_reload = _get_bool(
            ws, cw.GUNICORN_DEBUG_RELOAD, cd.GUNICORN_DEBUG_RELOAD)
        self.gunicorn_num_workers = _get_int(
            ws, cw.GUNICORN_NUM_WORKERS, cd.GUNICORN_NUM_WORKERS)
        self.gunicorn_timeout_s = _get_int(
            ws, cw.GUNICORN_TIMEOUT_S, cd.GUNICORN_TIMEOUT_S)
        self.host = _get_str(ws, cw.HOST, cd.HOST)
        self.port = _get_int(ws, cw.PORT, cd.PORT)
        self.proxy_http_host = _get_str(ws, cw.PROXY_HTTP_HOST)
        self.proxy_remote_addr = _get_str(ws, cw.PROXY_REMOTE_ADDR)
        self.proxy_rewrite_path_info = _get_bool(
            ws, cw.PROXY_REWRITE_PATH_INFO, cd.PROXY_REWRITE_PATH_INFO)
        self.proxy_script_name = _get_str(ws, cw.PROXY_SCRIPT_NAME)
        self.proxy_server_name = _get_str(ws, cw.PROXY_SERVER_NAME)
        self.proxy_server_port = _get_int(ws, cw.PROXY_SERVER_PORT)
        self.proxy_url_scheme = _get_str(ws, cw.PROXY_URL_SCHEME)
        self.show_request_immediately = _get_bool(
            ws, cw.SHOW_REQUEST_IMMEDIATELY, cd.SHOW_REQUEST_IMMEDIATELY)
        self.show_requests = _get_bool(ws, cw.SHOW_REQUESTS, cd.SHOW_REQUESTS)
        self.show_response = _get_bool(ws, cw.SHOW_RESPONSE, cd.SHOW_RESPONSE)
        self.show_timing = _get_bool(ws, cw.SHOW_TIMING, cd.SHOW_TIMING)
        self.ssl_certificate = _get_str(ws, cw.SSL_CERTIFICATE)
        self.ssl_private_key = _get_str(ws, cw.SSL_PRIVATE_KEY)
        self.static_cache_duration_s = _get_int(ws, cw.STATIC_CACHE_DURATION_S,
                                                cd.STATIC_CACHE_DURATION_S)
        self.trusted_proxy_headers = _get_multiline(
            ws, cw.TRUSTED_PROXY_HEADERS)
        self.unix_domain_socket = _get_str(ws, cw.UNIX_DOMAIN_SOCKET)

        for tph in self.trusted_proxy_headers:
            if tph not in ReverseProxiedMiddleware.ALL_CANDIDATES:
                raise ValueError(
                    f"Invalid {cw.TRUSTED_PROXY_HEADERS} value specified: "
                    f"was {tph!r}, options are "
                    f"{ReverseProxiedMiddleware.ALL_CANDIDATES}")

        del ws
        del cw

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Export section
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        es = CONFIG_FILE_EXPORT_SECTION
        ce = ConfigParamExportGeneral

        self.celery_beat_extra_args = _get_multiline(
            es, ce.CELERY_BEAT_EXTRA_ARGS)
        self.celery_beat_schedule_database = _get_str(
            es, ce.CELERY_BEAT_SCHEDULE_DATABASE)
        if not self.celery_beat_schedule_database:
            raise_missing(es, ce.CELERY_BEAT_SCHEDULE_DATABASE)
        self.celery_broker_url = _get_str(
            es, ce.CELERY_BROKER_URL, cd.CELERY_BROKER_URL)
        self.celery_worker_extra_args = _get_multiline(
            es, ce.CELERY_WORKER_EXTRA_ARGS)
        self.celery_export_task_rate_limit = _get_str(
            es, ce.CELERY_EXPORT_TASK_RATE_LIMIT)

        self.export_lockdir = _get_str(es, ce.EXPORT_LOCKDIR)
        if not self.export_lockdir:
            raise_missing(es, ConfigParamExportGeneral.EXPORT_LOCKDIR)

        self.export_recipient_names = _get_multiline_ignoring_comments(
            CONFIG_FILE_EXPORT_SECTION, ce.RECIPIENTS)
        duplicates = [name for name, count in
                      collections.Counter(self.export_recipient_names).items()
                      if count > 1]
        if duplicates:
            raise ValueError(
                f"Duplicate export recipients specified: {duplicates!r}")
        for recip_name in self.export_recipient_names:
            if re.match(VALID_RECIPIENT_NAME_REGEX, recip_name) is None:
                raise ValueError(
                    f"Recipient names must be alphanumeric or _- only; was "
                    f"{recip_name!r}")
        if len(set(self.export_recipient_names)) != len(self.export_recipient_names):  # noqa
            raise ValueError("Recipient names contain duplicates")
        self._export_recipients = []  # type: List[ExportRecipientInfo]
        self._read_export_recipients(parser)

        self.schedule_timezone = _get_str(
            es, ce.SCHEDULE_TIMEZONE, cd.SCHEDULE_TIMEZONE)

        self.crontab_entries = []  # type: List[CrontabEntry]
        crontab_lines = _get_multiline(es, ce.SCHEDULE)
        for crontab_line in crontab_lines:
            crontab_line = crontab_line.split("#")[0].strip()
            # ... everything before a '#'
            if not crontab_line:  # comment line
                continue
            crontab_entry = CrontabEntry(line=crontab_line)
            if crontab_entry.content not in self.export_recipient_names:
                raise ValueError(
                    f"{ce.SCHEDULE} setting exists for non-existent recipient "
                    f"{crontab_entry.content}")
            self.crontab_entries.append(crontab_entry)

        del es
        del ce

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Other attributes
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        self._sqla_engine = None

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # Docker checks
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if self.running_under_docker:
            log.info("Docker environment detected")

            # Values expected to be fixed
            warn_if_not_docker_value(
                param_name=ConfigParamExportGeneral.CELERY_BROKER_URL,
                actual_value=self.celery_broker_url,
                required_value=DockerConstants.CELERY_BROKER_URL
            )
            warn_if_not_docker_value(
                param_name=ConfigParamServer.HOST,
                actual_value=self.host,
                required_value=DockerConstants.HOST
            )

            # Values expected to be present
            #
            # - Re SSL certificates: reconsidered. People may want to run
            #   internal plain HTTP but then an Apache front end, and they
            #   wouldn't appreciate the warnings.
            #
            # warn_if_not_present(
            #     param_name=ConfigParamServer.SSL_CERTIFICATE,
            #     value=self.ssl_certificate
            # )
            # warn_if_not_present(
            #     param_name=ConfigParamServer.SSL_PRIVATE_KEY,
            #     value=self.ssl_private_key
            # )

            # Config-related files
            warn_if_not_within_docker_dir(
                param_name=ConfigParamServer.SSL_CERTIFICATE,
                filespec=self.ssl_certificate,
                permit_cfg=True
            )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamServer.SSL_PRIVATE_KEY,
                filespec=self.ssl_private_key,
                permit_cfg=True
            )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamSite.LOCAL_LOGO_FILE_ABSOLUTE,
                filespec=self.local_logo_file_absolute,
                permit_cfg=True,
                permit_venv=True
            )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamSite.CAMCOPS_LOGO_FILE_ABSOLUTE,
                filespec=self.camcops_logo_file_absolute,
                permit_cfg=True,
                permit_venv=True
            )
            for esf in self.extra_string_files:
                warn_if_not_within_docker_dir(
                    param_name=ConfigParamSite.EXTRA_STRING_FILES,
                    filespec=esf,
                    permit_cfg=True,
                    permit_venv=True,
                    param_contains_not_is=True
                )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamSite.SNOMED_ICD9_XML_FILENAME,
                filespec=self.snomed_icd9_xml_filename,
                permit_cfg=True,
                permit_venv=True
            )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamSite.SNOMED_ICD10_XML_FILENAME,
                filespec=self.snomed_icd10_xml_filename,
                permit_cfg=True,
                permit_venv=True
            )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamSite.SNOMED_TASK_XML_FILENAME,
                filespec=self.snomed_task_xml_filename,
                permit_cfg=True,
                permit_venv=True
            )

            # Temporary/scratch space that needs to be shared between Docker
            # containers
            warn_if_not_within_docker_dir(
                param_name=ConfigParamSite.USER_DOWNLOAD_DIR,
                filespec=self.user_download_dir,
                permit_tmp=True
            )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamExportGeneral.CELERY_BEAT_SCHEDULE_DATABASE,  # noqa
                filespec=self.celery_beat_schedule_database,
                permit_tmp=True
            )
            warn_if_not_within_docker_dir(
                param_name=ConfigParamExportGeneral.EXPORT_LOCKDIR,
                filespec=self.export_lockdir,
                permit_tmp=True
            )

    # -------------------------------------------------------------------------
    # Database functions
    # -------------------------------------------------------------------------

    def get_sqla_engine(self) -> Engine:
        """
        Returns an SQLAlchemy :class:`Engine`.

        I was previously misinterpreting the appropriate scope of an Engine.
        I thought: create one per request.
        But the Engine represents the connection *pool*.
        So if you create them all the time, you get e.g. a
        'Too many connections' error.

        "The appropriate scope is once per [database] URL per application,
        at the module level."

        - https://groups.google.com/forum/#!topic/sqlalchemy/ZtCo2DsHhS4
        - https://stackoverflow.com/questions/8645250/how-to-close-sqlalchemy-connection-in-mysql

        Now, our CamcopsConfig instance is cached, so there should be one of
        them overall. See get_config() below.

        Therefore, making the engine a member of this class should do the
        trick, whilst avoiding global variables.
        """  # noqa
        if self._sqla_engine is None:
            self._sqla_engine = create_engine(
                self.db_url,
                echo=self.db_echo,
                pool_pre_ping=True,
                # pool_size=0,  # no limit (for parallel testing, which failed)
            )
            log.debug("Created SQLAlchemy engine for URL {}",
                      get_safe_url_from_engine(self._sqla_engine))
        return self._sqla_engine

    @property
    @cache_region_static.cache_on_arguments(function_key_generator=fkg)
    def get_all_table_names(self) -> List[str]:
        """
        Returns all table names from the database.
        """
        log.debug("Fetching database table names")
        engine = self.get_sqla_engine()
        return get_table_names(engine=engine)

    def get_dbsession_raw(self) -> SqlASession:
        """
        Returns a raw SQLAlchemy Session.
        Avoid this -- use :func:`get_dbsession_context` instead.
        """
        engine = self.get_sqla_engine()
        maker = sessionmaker(bind=engine)
        dbsession = maker()  # type: SqlASession
        return dbsession

    @contextlib.contextmanager
    def get_dbsession_context(self) -> Generator[SqlASession, None, None]:
        """
        Context manager to provide an SQLAlchemy session that will COMMIT
        once we've finished, or perform a ROLLBACK if there was an exception.
        """
        dbsession = self.get_dbsession_raw()
        # noinspection PyBroadException
        try:
            yield dbsession
            dbsession.commit()
        except Exception:
            dbsession.rollback()
            raise
        finally:
            dbsession.close()

    def _assert_valid_database_engine(self) -> None:
        """
        Assert that our backend database is a valid type.

        Specifically, we prohibit:

        - SQL Server versions before 2008: they don't support timezones
          and we need that.
        """
        engine = self.get_sqla_engine()
        if not is_sqlserver(engine):
            return
        assert is_sqlserver_2008_or_later(engine), (
            "If you use Microsoft SQL Server as the back-end database for a "
            "CamCOPS server, it must be at least SQL Server 2008. Older "
            "versions do not have time zone awareness."
        )

    def _assert_database_is_at_head(self) -> None:
        """
        Assert that the current database is at its head (most recent) revision,
        by comparing its Alembic version number (written into the Alembic
        version table of the database) to the most recent Alembic revision in
        our ``camcops_server/alembic/versions`` directory.
        """
        current, head = get_current_and_head_revision(
            database_url=self.db_url,
            alembic_config_filename=ALEMBIC_CONFIG_FILENAME,
            alembic_base_dir=ALEMBIC_BASE_DIR,
            version_table=ALEMBIC_VERSION_TABLE,
        )
        if current == head:
            log.debug("Database is at correct (head) revision of {}", current)
        else:
            raise_runtime_error(
                f"Database structure is at version {current} but should be at "
                f"version {head}. CamCOPS will not start. Please use the "
                f"'upgrade_db' command to fix this.")

    def assert_database_ok(self) -> None:
        """
        Asserts that our database engine is OK and our database structure is
        correct.
        """
        self._assert_valid_database_engine()
        self._assert_database_is_at_head()

    # -------------------------------------------------------------------------
    # SNOMED-CT functions
    # -------------------------------------------------------------------------

    def get_task_snomed_concepts(self) -> Dict[str, SnomedConcept]:
        """
        Returns all SNOMED-CT concepts for tasks.

        Returns:
            dict: maps lookup strings to :class:`SnomedConcept` objects
        """
        if not self.snomed_task_xml_filename:
            return {}
        return get_all_task_snomed_concepts(self.snomed_task_xml_filename)

    def get_icd9cm_snomed_concepts(self) -> Dict[str, List[SnomedConcept]]:
        """
        Returns all SNOMED-CT concepts for ICD-9-CM codes supported by CamCOPS.

        Returns:
            dict: maps ICD-9-CM codes to :class:`SnomedConcept` objects
        """
        if not self.snomed_icd9_xml_filename:
            return {}
        return get_icd9_snomed_concepts_from_xml(self.snomed_icd9_xml_filename)

    def get_icd10_snomed_concepts(self) -> Dict[str, List[SnomedConcept]]:
        """
        Returns all SNOMED-CT concepts for ICD-10-CM codes supported by
        CamCOPS.

        Returns:
            dict: maps ICD-10 codes to :class:`SnomedConcept` objects
        """
        if not self.snomed_icd10_xml_filename:
            return {}
        return get_icd10_snomed_concepts_from_xml(
            self.snomed_icd10_xml_filename)

    # -------------------------------------------------------------------------
    # Export functions
    # -------------------------------------------------------------------------

    def _read_export_recipients(
            self,
            parser: configparser.ConfigParser = None) -> None:
        """
        Loads
        :class:`camcops_server.cc_modules.cc_exportrecipientinfo.ExportRecipientInfo`
        objects from the config file. Stores them in
        ``self._export_recipients``.

        Note that these objects are **not** associated with a database session.

        Args:
            parser: optional :class:`configparser.ConfigParser` object.
        """
        self._export_recipients = []  # type: List[ExportRecipientInfo]
        for recip_name in self.export_recipient_names:
            log.debug("Loading export config for recipient {!r}", recip_name)
            try:
                validate_export_recipient_name(recip_name)
            except ValueError as e:
                raise ValueError(f"Bad recipient name {recip_name!r}: {e}")
            recipient = ExportRecipientInfo.read_from_config(
                self, parser=parser, recipient_name=recip_name)
            self._export_recipients.append(recipient)

    def get_all_export_recipient_info(self) -> List["ExportRecipientInfo"]:
        """
        Returns all export recipients (in their "database unaware" form)
        specified in the config.

        Returns:
            list: of
            :class:`camcops_server.cc_modules.cc_exportrecipientinfo.ExportRecipientInfo`
        """  # noqa
        return self._export_recipients

    # -------------------------------------------------------------------------
    # File-based locks
    # -------------------------------------------------------------------------

    def get_export_lockfilename_db(self, recipient_name: str) -> str:
        """
        Returns a full path to a lockfile suitable for locking for a
        whole-database export to a particular export recipient.

        Args:
            recipient_name: name of the recipient

        Returns:
            a filename
        """
        filename = f"camcops_export_db_{recipient_name}"
        # ".lock" is appended automatically by the lockfile package
        return os.path.join(self.export_lockdir, filename)

    def get_export_lockfilename_task(self, recipient_name: str,
                                     basetable: str, pk: int) -> str:
        """
        Returns a full path to a lockfile suitable for locking for a
        single-task export to a particular export recipient.

        Args:
            recipient_name: name of the recipient
            basetable: task base table name
            pk: server PK of the task

        Returns:
            a filename
        """
        filename = f"camcops_export_task_{recipient_name}_{basetable}_{pk}"
        # ".lock" is appended automatically by the lockfile package
        return os.path.join(self.export_lockdir, filename)

    def get_master_export_recipient_lockfilename(self) -> str:
        """
        When we are modifying export recipients, we check "is this information
        the same as the current version in the database", and if not, we write
        fresh information to the database. If lots of processes do that at the
        same time, we have a problem (usually a database deadlock) -- hence
        this lock.

        Returns:
            a filename
        """
        filename = "camcops_master_export_recipient"
        # ".lock" is appended automatically by the lockfile package
        return os.path.join(self.export_lockdir, filename)

    def get_celery_beat_pidfilename(self) -> str:
        """
        Process ID file (pidfile) used by ``celery beat --pidfile ...``.
        """
        filename = "camcops_celerybeat.pid"
        return os.path.join(self.export_lockdir, filename)


# =============================================================================
# Get config filename from an appropriate environment (WSGI or OS)
# =============================================================================

def get_config_filename_from_os_env() -> str:
    """
    Returns the config filename to use, from our operating system environment
    variable.

    (We do NOT trust the WSGI environment for this.)
    """
    config_filename = os.environ.get(ENVVAR_CONFIG_FILE)
    if not config_filename:
        raise AssertionError(
            f"OS environment did not provide the required "
            f"environment variable {ENVVAR_CONFIG_FILE}")
    return config_filename


# =============================================================================
# Cached instances
# =============================================================================

@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def get_config(config_filename: str) -> CamcopsConfig:
    """
    Returns a :class:`camcops_server.cc_modules.cc_config.CamcopsConfig` from
    the specified config filename.

    Cached.
    """
    return CamcopsConfig(config_filename)


# =============================================================================
# Get default config
# =============================================================================

def get_default_config_from_os_env() -> CamcopsConfig:
    """
    Returns the :class:`camcops_server.cc_modules.cc_config.CamcopsConfig`
    representing the config filename that we read from our operating system
    environment variable.
    """
    if ON_READTHEDOCS:
        return CamcopsConfig(config_filename="", config_text=get_demo_config())
    else:
        return get_config(get_config_filename_from_os_env())
