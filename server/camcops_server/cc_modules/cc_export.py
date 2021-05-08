#!/usr/bin/env python

# noinspection HttpUrlsUsage
"""
camcops_server/cc_modules/cc_export.py

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

.. _ActiveMQ: https://activemq.apache.org/
.. _AMQP: https://www.amqp.org/
.. _APScheduler: https://apscheduler.readthedocs.io/
.. _Celery: https://www.celeryproject.org/
.. _Dramatiq: https://dramatiq.io/
.. _RabbitMQ: https://www.rabbitmq.com/
.. _Redis: https://redis.io/
.. _ZeroMQ: https://zeromq.org/

**Export and research dump functions.**

Export design:

*WHICH RECORDS TO SEND?*

The most powerful mechanism is not to have a sending queue (which would then
require careful multi-instance locking), but to have a "sent" log. That way:

- A record needs sending if it's not in the sent log (for an appropriate
  recipient).
- You can add a new recipient and the system will know about the (new)
  backlog automatically.
- You can specify criteria, e.g. don't upload records before 1/1/2014, and
  modify that later, and it would catch up with the backlog.
- Successes and failures are logged in the same table.
- Multiple recipients are handled with ease.
- No need to alter database.pl code that receives from tablets.
- Can run with a simple cron job.

*LOCKING*

- Don't use database locking:
  https://blog.engineyard.com/2011/5-subtle-ways-youre-using-mysql-as-a-queue-and-why-itll-bite-you
- Locking via UNIX lockfiles:

  - https://pypi.python.org/pypi/lockfile
  - http://pythonhosted.org/lockfile/ (which also works on Windows)

  - On UNIX, ``lockfile`` uses ``LinkLockFile``:
    https://github.com/smontanaro/pylockfile/blob/master/lockfile/linklockfile.py

*MESSAGE QUEUE AND BACKEND*

Thoughts as of 2018-12-22.

- See https://www.fullstackpython.com/task-queues.html. Also http://queues.io/;
  https://stackoverflow.com/questions/731233/activemq-or-rabbitmq-or-zeromq-or.

- The "default" is Celery_, with ``celery beat`` for scheduling, via an
  AMQP_ broker like RabbitMQ_.

  - Downside: no longer supported under Windows as of Celery 4.

    - There are immediate bugs when running the demo code with Celery 4.2.1,
      fixed by setting the environment variable ``set
      FORKED_BY_MULTIPROCESSING=1`` before running the worker; see
      https://github.com/celery/celery/issues/4178 and
      https://github.com/celery/celery/pull/4078.

  - Downside: backend is complex; e.g. Erlang dependency of RabbitMQ.

  - Celery also supports Redis_, but Redis_ doesn't support Windows directly
    (except the Windows Subsystem for Linux in Windows 10+).

- Another possibility is Dramatiq_ with APScheduler_.

  - Of note, APScheduler_ can use an SQLAlchemy database table as its job
    store, which might be good.
  - Dramatiq_ uses RabbitMQ_ or Redis_.
  - Dramatiq_ 1.4.0 (2018-11-25) installs cleanly under Windows. Use ``pip
    install --upgrade "dramatic[rabbitmq, watch]"`` (i.e. with double quotse,
    not the single quotes it suggests, which don't work under Windows).
  - However, the basic example (https://dramatiq.io/guide.html) fails under
    Windows; when you fire up ``dramatic count_words`` (even with ``--processes
    1 --threads 1``) it crashes with an error from ``ForkingPickler`` in
    ``multiprocessing.reduction``, i.e.
    https://docs.python.org/3/library/multiprocessing.html#windows. It also
    emits a ``PermissionError: [WinError 5] Access is denied``. This is
    discussed a bit at https://github.com/Bogdanp/dramatiq/issues/75;
    https://github.com/Bogdanp/dramatiq/blob/master/docs/source/changelog.rst.
    The changelog suggests 1.4.0 should work, but it doesn't.

- Worth some thought about ZeroMQ_, which is a very different sort of thing.
  Very cross-platform. Needs work to guard against message loss (i.e. messages
  are unreliable by default). Dynamic "special socket" style.

- Possibly also ActiveMQ_.

- OK; so speed is not critical but we want message reliability, for it to work
  under Windows, and decent Python bindings with job scheduling.

  - OUT: Redis (not Windows easily), ZeroMQ (fast but not by default reliable),
    ActiveMQ (few Python frameworks?).
  - REMAINING for message handling: RabbitMQ.
  - Python options therefore: Celery (but Windows not officially supported from
    4+); Dramatiq (but Windows also not very well supported and seems a bit
    bleeding-edge).

- This is looking like a mess from the Windows perspective.

- An alternative is just to use the database, of course.

  - https://softwareengineering.stackexchange.com/questions/351449/message-queue-database-vs-dedicated-mq
  - http://mikehadlow.blogspot.com/2012/04/database-as-queue-anti-pattern.html
  - https://blog.jooq.org/2014/09/26/using-your-rdbms-for-messaging-is-totally-ok/
  - https://stackoverflow.com/questions/13005410/why-do-we-need-message-brokers-like-rabbitmq-over-a-database-like-postgresql
  - https://www.quora.com/What-is-the-best-practice-using-db-tables-or-message-queues-for-moderation-of-content-approved-by-humans

- Let's take a step back and summarize the problem.

  - Many web threads may upload tasks. This should trigger a prompt export for
    all push recipients.
  - Whichever way we schedule a backend task job, it should be as the
    combination of recipient, basetable, task PK. (That way, if one recipient
    fails, the others can proceed independently.)
  - Every job should check that it's not been completed already (in case of
    accidental job restarts), i.e. is idempotent as far as we can make it.
  - How should this interact with the non-push recipients?
  - We should use the same locking method for push and non-push recipients.
  - We should make the locking granular and use file locks -- for example, for
    each task/recipient combination (or each whole-database export for a given
    recipient).

"""  # noqa

import logging
import os
import sqlite3
import tempfile
from typing import (Dict, List, Generator, Optional,
                    Tuple, Type, TYPE_CHECKING, Union)

from cardinal_pythonlib.classes import gen_all_subclasses
from cardinal_pythonlib.datetimefunc import (
    format_datetime,
    get_now_localtz_pendulum,
    get_tz_local,
    get_tz_utc,
)
from cardinal_pythonlib.email.sendmail import CONTENT_TYPE_TEXT
from cardinal_pythonlib.fileops import relative_filename_within_dir
from cardinal_pythonlib.json.serialize import register_for_json
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.pyramid.responses import (
    OdsResponse,
    SqliteBinaryResponse,
    TextAttachmentResponse,
    XlsxResponse,
    ZipResponse,
)
from cardinal_pythonlib.sizeformatter import bytes2human
from cardinal_pythonlib.sqlalchemy.session import get_safe_url_from_engine
import lockfile
from pendulum import DateTime as Pendulum, Duration, Period
from pyramid.httpexceptions import HTTPBadRequest
from pyramid.renderers import render_to_response
from pyramid.response import Response
from sqlalchemy.engine import create_engine
from sqlalchemy.engine.result import ResultProxy
from sqlalchemy.orm import Session as SqlASession, sessionmaker
from sqlalchemy.sql.expression import text
from sqlalchemy.sql.schema import Column, MetaData, Table
from sqlalchemy.sql.sqltypes import Text

from camcops_server.cc_modules.cc_audit import audit
from camcops_server.cc_modules.cc_constants import DateFormat
from camcops_server.cc_modules.cc_dump import copy_tasks_and_summaries
from camcops_server.cc_modules.cc_email import Email
from camcops_server.cc_modules.cc_exportmodels import (
    ExportedTask,
    ExportRecipient,
    gen_tasks_having_exportedtasks,
    get_collection_for_export,
)
from camcops_server.cc_modules.cc_forms import UserDownloadDeleteForm
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
from camcops_server.cc_modules.cc_simpleobjects import TaskExportOptions
from camcops_server.cc_modules.cc_sqlalchemy import sql_from_sqlite_database
from camcops_server.cc_modules.cc_task import Task
from camcops_server.cc_modules.cc_tsv import TsvCollection, TsvPage
from camcops_server.cc_modules.celery import (
    create_user_download,
    email_basic_dump,
    export_task_backend,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest
    from camcops_server.cc_modules.cc_taskcollection import TaskCollection

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

INFOSCHEMA_PAGENAME = "_camcops_information_schema_columns"


# =============================================================================
# Export tasks from the back end
# =============================================================================

def print_export_queue(req: "CamcopsRequest",
                       recipient_names: List[str] = None,
                       all_recipients: bool = False,
                       via_index: bool = True,
                       pretty: bool = False) -> None:
    """
    Called from the command line.

    Shows tasks that would be exported.

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        recipient_names: list of export recipient names (as per the config
            file)
        all_recipients: use all recipients?
        via_index: use the task index (faster)?
        pretty: use ``str(task)`` not ``repr(task)`` (prettier, slower because
            it has to query the patient)
    """
    recipients = req.get_export_recipients(
        recipient_names=recipient_names,
        all_recipients=all_recipients,
        save=False
    )
    if not recipients:
        log.warning("No export recipients")
        return
    for recipient in recipients:
        log.info("Tasks to be exported for recipient: {}", recipient)
        collection = get_collection_for_export(req, recipient,
                                               via_index=via_index)
        for task in collection.gen_tasks_by_class():
            print(
                f"{recipient.recipient_name}: "
                f"{str(task) if pretty else repr(task)}"
            )


def export(req: "CamcopsRequest",
           recipient_names: List[str] = None,
           all_recipients: bool = False,
           via_index: bool = True,
           schedule_via_backend: bool = False) -> None:
    """
    Called from the command line.

    Exports all relevant tasks (pending incremental exports, or everything if
    applicable) for specified export recipients.

    Obtains a file lock, then iterates through all recipients.

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        recipient_names: list of export recipient names (as per the config
            file)
        all_recipients: use all recipients?
        via_index: use the task index (faster)?
        schedule_via_backend: schedule jobs via the backend instead?
    """
    recipients = req.get_export_recipients(recipient_names=recipient_names,
                                           all_recipients=all_recipients)
    if not recipients:
        log.warning("No export recipients")
        return

    for recipient in recipients:
        log.info("Exporting to recipient: {}", recipient)
        if recipient.using_db():
            if schedule_via_backend:
                raise NotImplementedError()  # todo: implement whole-database export via Celery backend  # noqa
            else:
                export_whole_database(req, recipient, via_index=via_index)
        else:
            # Non-database recipient.
            export_tasks_individually(
                req, recipient,
                via_index=via_index, schedule_via_backend=schedule_via_backend)


def export_whole_database(req: "CamcopsRequest",
                          recipient: ExportRecipient,
                          via_index: bool = True) -> None:
    """
    Exports to a database.

    Holds a recipient-specific file lock in the process.

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        recipient: an :class:`camcops_server.cc_modules.cc_exportmodels.ExportRecipient`
        via_index: use the task index (faster)?
    """  # noqa
    cfg = req.config
    lockfilename = cfg.get_export_lockfilename_db(
        recipient_name=recipient.recipient_name)
    try:
        with lockfile.FileLock(lockfilename, timeout=0):  # doesn't wait
            collection = get_collection_for_export(req, recipient,
                                                   via_index=via_index)
            dst_engine = create_engine(recipient.db_url,
                                       echo=recipient.db_echo)
            log.info("Exporting to database: {}",
                     get_safe_url_from_engine(dst_engine))
            dst_session = sessionmaker(bind=dst_engine)()  # type: SqlASession
            task_generator = gen_tasks_having_exportedtasks(collection)
            export_options = TaskExportOptions(
                include_blobs=recipient.db_include_blobs,
                db_patient_id_per_row=recipient.db_patient_id_per_row,
                db_make_all_tables_even_empty=True,
                db_include_summaries=recipient.db_add_summaries,
            )
            copy_tasks_and_summaries(
                tasks=task_generator,
                dst_engine=dst_engine,
                dst_session=dst_session,
                export_options=export_options,
                req=req,
            )
            dst_session.commit()
    except lockfile.AlreadyLocked:
        log.warning("Export logfile {!r} already locked by another process; "
                    "aborting", lockfilename)


def export_tasks_individually(req: "CamcopsRequest",
                              recipient: ExportRecipient,
                              via_index: bool = True,
                              schedule_via_backend: bool = False) -> None:
    """
    Exports all necessary tasks for a recipient.

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        recipient: an :class:`camcops_server.cc_modules.cc_exportmodels.ExportRecipient`
        via_index: use the task index (faster)?
        schedule_via_backend: schedule jobs via the backend instead?
    """  # noqa
    collection = get_collection_for_export(req, recipient, via_index=via_index)
    if schedule_via_backend:
        recipient_name = recipient.recipient_name
        for task_or_index in collection.gen_all_tasks_or_indexes():
            if isinstance(task_or_index, Task):
                basetable = task_or_index.tablename
                task_pk = task_or_index.pk
            else:
                basetable = task_or_index.task_table_name
                task_pk = task_or_index.task_pk
            log.info("Submitting background job to export task {}.{} to {}",
                     basetable, task_pk, recipient_name)
            export_task_backend.delay(
                recipient_name=recipient_name,
                basetable=basetable,
                task_pk=task_pk
            )
    else:
        for task in collection.gen_tasks_by_class():
            # Do NOT use this to check the working of export_task_backend():
            # export_task_backend(recipient.recipient_name, task.tablename, task.pk)  # noqa
            # ... it will deadlock at the database (because we're already
            # within a query of some sort, I presume)
            export_task(req, recipient, task)


def export_task(req: "CamcopsRequest",
                recipient: ExportRecipient,
                task: Task) -> None:
    """
    Exports a single task, checking that it remains valid to do so.

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        recipient: an :class:`camcops_server.cc_modules.cc_exportmodels.ExportRecipient`
        task: a :class:`camcops_server.cc_modules.cc_task.Task`
    """  # noqa

    # Double-check it's OK! Just in case, for example, an old backend task has
    # persisted, or someone's managed to get an iffy back-end request in some
    # other way.
    if not recipient.is_task_suitable(task):
        # Warning will already have been emitted (by is_task_suitable).
        return

    cfg = req.config
    lockfilename = cfg.get_export_lockfilename_task(
        recipient_name=recipient.recipient_name,
        basetable=task.tablename,
        pk=task.pk,
    )
    dbsession = req.dbsession
    try:
        with lockfile.FileLock(lockfilename, timeout=0):  # doesn't wait
            # We recheck the export status once we hold the lock, in case
            # multiple jobs are competing to export it.
            if ExportedTask.task_already_exported(
                    dbsession=dbsession,
                    recipient_name=recipient.recipient_name,
                    basetable=task.tablename,
                    task_pk=task.pk):
                log.info("Task {!r} already exported to recipient {!r}; "
                         "ignoring", task, recipient)
                # Not a warning; it's normal to see these because it allows the
                # client API to skip some checks for speed.
                return
            # OK; safe to export now.
            et = ExportedTask(recipient, task)
            dbsession.add(et)
            et.export(req)
            dbsession.commit()  # so the ExportedTask is visible to others ASAP
    except lockfile.AlreadyLocked:
        log.warning("Export logfile {!r} already locked by another process; "
                    "aborting", lockfilename)


# =============================================================================
# Helpers for task collection export functions
# =============================================================================

def gen_audited_tasks_for_task_class(
        collection: "TaskCollection",
        cls: Type[Task],
        audit_descriptions: List[str]) -> Generator[Task, None, None]:
    """
    Generates tasks from a collection, for a given task class, simultaneously
    adding to an audit description. Used for user-triggered downloads.

    Args:
        collection: a :class:`camcops_server.cc_modules.cc_taskcollection.TaskCollection`
        cls: the task class to generate
        audit_descriptions: list of strings to be modified

    Yields:
        :class:`camcops_server.cc_modules.cc_task.Task` objects
    """  # noqa
    pklist = []  # type: List[int]
    for task in collection.tasks_for_task_class(cls):
        pklist.append(task.pk)
        yield task
    audit_descriptions.append(
        f"{cls.__tablename__}: "
        f"{','.join(str(pk) for pk in pklist)}"
    )


def gen_audited_tasks_by_task_class(
        collection: "TaskCollection",
        audit_descriptions: List[str]) -> Generator[Task, None, None]:
    """
    Generates tasks from a collection, across task classes, simultaneously
    adding to an audit description. Used for user-triggered downloads.

    Args:
        collection: a :class:`camcops_server.cc_modules.cc_taskcollection.TaskCollection`
        audit_descriptions: list of strings to be modified

    Yields:
        :class:`camcops_server.cc_modules.cc_task.Task` objects
    """  # noqa
    for cls in collection.task_classes():
        for task in gen_audited_tasks_for_task_class(collection, cls,
                                                     audit_descriptions):
            yield task


def get_information_schema_query(req: "CamcopsRequest") -> ResultProxy:
    """
    Returns an SQLAlchemy query object that fetches the
    INFORMATION_SCHEMA.COLUMNS information from our source database.

    This is not sensitive; there is no data, just structure/comments.
    """
    # Find our database name
    # https://stackoverflow.com/questions/53554458/sqlalchemy-get-database-name-from-engine
    dbname = req.engine.url.database
    # Query the information schema for our database.
    # https://docs.sqlalchemy.org/en/13/core/sqlelement.html#sqlalchemy.sql.expression.text  # noqa
    query = text("""
        SELECT *
        FROM information_schema.columns
        WHERE table_schema = :dbname
    """).bindparams(dbname=dbname)
    result_proxy = req.dbsession.execute(query)
    return result_proxy


def get_information_schema_tsv_page(
        req: "CamcopsRequest",
        page_name: str = INFOSCHEMA_PAGENAME) -> TsvPage:
    """
    Returns the server database's ``INFORMATION_SCHEMA.COLUMNS`` table as a
    :class:`camcops_server.cc_modules.cc_tsv.TsvPage``.
    """
    result_proxy = get_information_schema_query(req)
    return TsvPage.from_resultproxy(page_name, result_proxy)


def write_information_schema_to_dst(
        req: "CamcopsRequest",
        dst_session: SqlASession,
        dest_table_name: str = INFOSCHEMA_PAGENAME) -> None:
    """
    Writes the server's information schema to a separate database session
    (which will be an SQLite database being created for download).

    There must be no open transactions (i.e. please COMMIT before you call
    this function), since we need to create a table.
    """
    # 1. Read the structure of INFORMATION_SCHEMA.COLUMNS itself.
    # https://stackoverflow.com/questions/21770829/sqlalchemy-copy-schema-and-data-of-subquery-to-another-database  # noqa
    src_engine = req.engine
    dst_engine = dst_session.bind
    metadata = MetaData(bind=dst_engine)
    table = Table(
        "columns",  # table name; see also "schema" argument
        metadata,  # "load with the destination metadata"
        # Override some specific column types by hand, or they'll fail as
        # SQLAlchemy fails to reflect the MySQL LONGTEXT type properly:
        Column("COLUMN_DEFAULT", Text),
        Column("COLUMN_TYPE", Text),
        Column("GENERATION_EXPRESSION", Text),
        autoload=True,  # "read (reflect) structure from the database"
        autoload_with=src_engine,  # "read (reflect) structure from the source"
        schema="information_schema"  # schema
    )
    # 2. Write that structure to our new database.
    table.name = dest_table_name  # create it with a different name
    table.schema = ""  # we don't have a schema in the destination database
    table.create(dst_engine)  # CREATE TABLE
    # 3. Fetch data.
    query = get_information_schema_query(req)
    # 4. Write the data.
    for row in query:
        dst_session.execute(table.insert(row))
    # 5. COMMIT
    dst_session.commit()


# =============================================================================
# Convert task collections to different export formats for user download
# =============================================================================

@register_for_json
class DownloadOptions(object):
    """
    Represents options for the process of the user downloading tasks.
    """
    DELIVERY_MODES = [
        ViewArg.DOWNLOAD,
        ViewArg.EMAIL,
        ViewArg.IMMEDIATELY,
    ]

    def __init__(self,
                 user_id: int,
                 viewtype: str,
                 delivery_mode: str,
                 spreadsheet_sort_by_heading: bool = False,
                 db_include_blobs: bool = False,
                 db_patient_id_per_row: bool = False,
                 include_information_schema_columns: bool = True) -> None:
        """
        Args:
            user_id:
                ID of the user creating the request (may be needed to pass to
                the back-end)
            viewtype:
                file format for receiving data (e.g. XLSX, SQLite)
            delivery_mode:
                method of delivery (e.g. immediate, e-mail)
            spreadsheet_sort_by_heading:
                (For spreadsheets.)
                Sort columns within each page by heading name?
            db_include_blobs:
                (For database downloads.)
                Include BLOBs?
            db_patient_id_per_row:
                (For database downloads.)
                Denormalize by include the patient ID in all rows of
                patient-related tables?
            include_information_schema_columns:
                Include descriptions of the columns provided?
        """
        assert delivery_mode in self.DELIVERY_MODES
        self.user_id = user_id
        self.viewtype = viewtype
        self.delivery_mode = delivery_mode
        self.spreadsheet_sort_by_heading = spreadsheet_sort_by_heading
        self.db_include_blobs = db_include_blobs
        self.db_patient_id_per_row = db_patient_id_per_row
        self.include_information_schema_columns = include_information_schema_columns  # noqa


class TaskCollectionExporter(object):
    """
    Class to provide tasks for user download.
    """

    def __init__(self,
                 req: "CamcopsRequest",
                 collection: "TaskCollection",
                 options: DownloadOptions):
        """
        Args:
            req:
                a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            collection:
                a :class:`camcops_server.cc_modules.cc_taskcollection.TaskCollection`
            options:
                :class:`DownloadOptions` governing the download
        """  # noqa
        self.req = req
        self.collection = collection
        self.options = options

    @property
    def viewtype(self) -> str:
        raise NotImplementedError("Exporter needs to implement 'viewtype'")

    @property
    def file_extension(self) -> str:
        raise NotImplementedError(
            "Exporter needs to implement 'file_extension'"
        )

    def get_filename(self) -> str:
        """
        Returns the filename for the download.
        """
        timestamp = format_datetime(self.req.now, DateFormat.FILENAME)
        return f"CamCOPS_dump_{timestamp}.{self.file_extension}"

    def immediate_response(self, req: "CamcopsRequest") -> Response:
        """
        Returns either a :class:`Response` with the data, or a
        :class:`Response` saying how the user will obtain their data later.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        """
        if self.options.delivery_mode == ViewArg.EMAIL:
            self.schedule_email()
            return render_to_response(
                "email_scheduled.mako",
                dict(),
                request=req
            )
        elif self.options.delivery_mode == ViewArg.DOWNLOAD:
            self.schedule_download()
            return render_to_response(
                "download_scheduled.mako",
                dict(),
                request=req
            )
        else:  # ViewArg.IMMEDIATELY
            return self.download_now()

    def download_now(self) -> Response:
        """
        Download the data dump in the selected format
        """
        filename, body = self.to_file()
        return self.get_data_response(body=body, filename=filename)

    def schedule_email(self) -> None:
        """
        Schedule the export asynchronously and e-mail the logged in user
        when done
        """
        email_basic_dump.delay(self.collection, self.options)

    def send_by_email(self) -> None:
        """
        Send the data dump by e-mail to the logged in user
        """
        _ = self.req.gettext
        config = self.req.config

        filename, body = self.to_file()
        email_to = self.req.user.email
        email = Email(
            # date: automatic
            from_addr=config.email_from,
            to=email_to,
            subject=_("CamCOPS research data dump"),
            body=_("The research data dump you requested is attached."),
            content_type=CONTENT_TYPE_TEXT,
            charset="utf8",
            attachments_binary=[(filename, body)],
        )
        email.send(
            host=config.email_host,
            username=config.email_host_username,
            password=config.email_host_password,
            port=config.email_port,
            use_tls=config.email_use_tls,
        )

        if email.sent:
            log.info(f"Research dump emailed to {email_to}")
        else:
            log.error(
                f"Failed to email research dump to {email_to}"
            )

    def schedule_download(self) -> None:
        """
        Schedule a background export to a file that the user can download
        later.
        """
        create_user_download.delay(self.collection, self.options)

    def create_user_download_and_email(self) -> None:
        """
        Creates a user download, and e-mails the user to let them know.
        """
        _ = self.req.gettext
        config = self.req.config

        download_dir = self.req.user_download_dir
        space = self.req.user_download_bytes_available
        filename, contents = self.to_file()
        size = len(contents)

        if size > space:
            # Not enough space
            total_permitted = self.req.user_download_bytes_permitted
            msg = _(
                "You do not have enough space to create this download. "
                "You are allowed {total_permitted} bytes and you are have "
                "{space} bytes free. This download would need {size} bytes."
            ).format(total_permitted=total_permitted, space=space, size=size)
        else:
            # Create file
            fullpath = os.path.join(download_dir, filename)
            try:
                with open(fullpath, "wb") as f:
                    f.write(contents)
                # Success
                log.info(f"Created user download: {fullpath}")
                msg = _(
                    "The research data dump you requested is ready to be "
                    "downloaded. You will find it in your download area. "
                    "It is called %s"
                ) % filename
            except Exception as e:
                # Some other error
                msg = _(
                    "Failed to create file {filename}. Error was: {message}"
                ).format(filename=filename, message=e)

        # E-mail the user, if they have an e-mail address
        email_to = self.req.user.email
        if email_to:
            email = Email(
                # date: automatic
                from_addr=config.email_from,
                to=email_to,
                subject=_("CamCOPS research data dump"),
                body=msg,
                content_type=CONTENT_TYPE_TEXT,
                charset="utf8",
            )
            email.send(
                host=config.email_host,
                username=config.email_host_username,
                password=config.email_host_password,
                port=config.email_port,
                use_tls=config.email_use_tls,
            )

    def get_data_response(self, body: bytes, filename: str) -> Response:
        raise NotImplementedError("Exporter needs to implement 'get_response'")

    def to_file(self) -> Tuple[str, bytes]:
        """
        Returns the tuple ``filename, file_contents``.
        """
        return self.get_filename(), self.get_file_body()

    def get_file_body(self) -> bytes:
        """
        Returns binary data to be stored as a file.
        """
        raise NotImplementedError("Exporter needs to implement 'get_file_body'")

    def get_tsv_collection(self) -> TsvCollection:
        """
        Converts the collection of tasks to a collection of spreadsheet-style
        data. Also audits the request as a basic data dump.

        Returns:
            a :class:`camcops_server.cc_modules.cc_tsv.TsvCollection` object
        """  # noqa
        audit_descriptions = []  # type: List[str]
        # Task may return >1 file for TSV output (e.g. for subtables).
        tsvcoll = TsvCollection()
        # Iterate through tasks, creating the TSV collection
        for cls in self.collection.task_classes():
            for task in gen_audited_tasks_for_task_class(self.collection, cls,
                                                         audit_descriptions):
                tsv_pages = task.get_tsv_pages(self.req)
                tsvcoll.add_pages(tsv_pages)

        if self.options.include_information_schema_columns:
            info_schema_page = get_information_schema_tsv_page(self.req)
            tsvcoll.add_page(info_schema_page)

        tsvcoll.sort_pages()
        if self.options.spreadsheet_sort_by_heading:
            tsvcoll.sort_headings_within_all_pages()

        audit(self.req, f"Basic dump: {'; '.join(audit_descriptions)}")

        return tsvcoll


class OdsExporter(TaskCollectionExporter):
    """
    Converts a set of tasks to an OpenOffice ODS file.
    """
    file_extension = "ods"
    viewtype = ViewArg.ODS

    def get_file_body(self) -> bytes:
        return self.get_tsv_collection().as_ods()

    def get_data_response(self, body: bytes, filename: str) -> Response:
        return OdsResponse(body=body, filename=filename)


class RExporter(TaskCollectionExporter):
    """
    Converts a set of tasks to an R script.
    """
    file_extension = "R"
    viewtype = ViewArg.R

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.encoding = "utf-8"

    def get_file_body(self) -> bytes:
        return self.get_r_script().encode(self.encoding)

    def get_r_script(self) -> str:
        return self.get_tsv_collection().as_r()

    def get_data_response(self, body: bytes, filename: str) -> Response:
        filename = self.get_filename()
        r_script = self.get_r_script()
        return TextAttachmentResponse(body=r_script, filename=filename)


class TsvZipExporter(TaskCollectionExporter):
    """
    Converts a set of tasks to a set of TSV (tab-separated value) file, (one
    per table) in a ZIP file.
    """
    file_extension = "zip"
    viewtype = ViewArg.TSV_ZIP

    def get_file_body(self) -> bytes:
        return self.get_tsv_collection().as_zip()

    def get_data_response(self, body: bytes, filename: str) -> Response:
        return ZipResponse(body=body, filename=filename)


class XlsxExporter(TaskCollectionExporter):
    """
    Converts a set of tasks to an Excel XLSX file.
    """
    file_extension = "xlsx"
    viewtype = ViewArg.XLSX

    def get_file_body(self) -> bytes:
        return self.get_tsv_collection().as_xlsx()

    def get_data_response(self, body: bytes, filename: str) -> Response:
        return XlsxResponse(body=body, filename=filename)


class SqliteExporter(TaskCollectionExporter):
    """
    Converts a set of tasks to an SQLite binary file.
    """
    file_extension = "sqlite"
    viewtype = ViewArg.SQLITE

    def get_export_options(self) -> TaskExportOptions:
        return TaskExportOptions(
            include_blobs=self.options.db_include_blobs,
            db_include_summaries=True,
            db_make_all_tables_even_empty=True,  # debatable, but more consistent!  # noqa
            db_patient_id_per_row=self.options.db_patient_id_per_row,
        )

    def get_sqlite_data(self, as_text: bool) -> Union[bytes, str]:
        """
        Returns data as a binary SQLite database, or SQL text to create it.

        Args:
            as_text: textual SQL, rather than binary SQLite?

        Returns:
            ``bytes`` or ``str``, according to ``as_text``
        """
        # ---------------------------------------------------------------------
        # Create memory file, dumper, and engine
        # ---------------------------------------------------------------------

        # This approach failed:
        #
        #   memfile = io.StringIO()
        #
        #   def dump(querysql, *multiparams, **params):
        #       compsql = querysql.compile(dialect=engine.dialect)
        #       memfile.write("{};\n".format(compsql))
        #
        #   engine = create_engine('{dialect}://'.format(dialect=dialect_name),
        #                          strategy='mock', executor=dump)
        #   dst_session = sessionmaker(bind=engine)()  # type: SqlASession
        #
        # ... you get the error
        #   AttributeError: 'MockConnection' object has no attribute 'begin'
        # ... which is fair enough.
        #
        # Next best thing: SQLite database.
        # Two ways to deal with it:
        # (a) duplicate our C++ dump code (which itself duplicate the SQLite
        #     command-line executable's dump facility), then create the
        #     database, dump it to a string, serve the string; or
        # (b) offer the binary SQLite file.
        # Or... (c) both.
        # Aha! pymysqlite.iterdump does this for us.
        #
        # If we create an in-memory database using create_engine('sqlite://'),
        # can we get the binary contents out? Don't think so.
        #
        # So we should first create a temporary on-disk file, then use that.

        # ---------------------------------------------------------------------
        # Make temporary file (one whose filename we can know).
        # ---------------------------------------------------------------------
        # We use tempfile.mkstemp() for security, or NamedTemporaryFile,
        # which is a bit easier. However, you can't necessarily open the file
        # again under all OSs, so that's no good. The final option is
        # TemporaryDirectory, which is secure and convenient.
        #
        # https://docs.python.org/3/library/tempfile.html
        # https://security.openstack.org/guidelines/dg_using-temporary-files-securely.html  # noqa
        # https://stackoverflow.com/questions/3924117/how-to-use-tempfile-namedtemporaryfile-in-python  # noqa
        db_basename = "temp.sqlite3"
        with tempfile.TemporaryDirectory() as tmpdirname:
            db_filename = os.path.join(tmpdirname, db_basename)
            # ---------------------------------------------------------------------
            # Make SQLAlchemy session
            # ---------------------------------------------------------------------
            url = "sqlite:///" + db_filename
            engine = create_engine(url, echo=False)
            dst_session = sessionmaker(bind=engine)()  # type: SqlASession
            # ---------------------------------------------------------------------
            # Iterate through tasks, creating tables as we need them.
            # ---------------------------------------------------------------------
            audit_descriptions = []  # type: List[str]
            task_generator = gen_audited_tasks_by_task_class(self.collection,
                                                             audit_descriptions)
            # ---------------------------------------------------------------------
            # Next bit very tricky. We're trying to achieve several things:
            # - a copy of part of the database structure
            # - a copy of part of the data, with relationships intact
            # - nothing sensitive (e.g. full User records) going through
            # - adding new columns for Task objects offering summary values
            # - Must treat tasks all together, because otherwise we will insert
            #   duplicate dependency objects like Group objects.
            # ---------------------------------------------------------------------
            copy_tasks_and_summaries(tasks=task_generator,
                                     dst_engine=engine,
                                     dst_session=dst_session,
                                     export_options=self.get_export_options(),
                                     req=self.req)
            dst_session.commit()
            if self.options.include_information_schema_columns:
                # Must have committed before we do this:
                write_information_schema_to_dst(self.req, dst_session)
            # ---------------------------------------------------------------------
            # Audit
            # ---------------------------------------------------------------------
            audit(self.req, f"SQL dump: {'; '.join(audit_descriptions)}")
            # ---------------------------------------------------------------------
            # Fetch file contents, either as binary, or as SQL
            # ---------------------------------------------------------------------
            if as_text:
                # SQL text
                connection = sqlite3.connect(db_filename)  # type: sqlite3.Connection  # noqa
                sql_text = sql_from_sqlite_database(connection)
                connection.close()
                return sql_text
            else:
                # SQLite binary
                with open(db_filename, 'rb') as f:
                    binary_contents = f.read()
                return binary_contents

    def get_file_body(self) -> bytes:
        return self.get_sqlite_data(as_text=False)

    def get_data_response(self, body: bytes, filename: str) -> Response:
        return SqliteBinaryResponse(body=body, filename=filename)


class SqlExporter(SqliteExporter):
    """
    Converts a set of tasks to the textual SQL needed to create an SQLite file.
    """
    file_extension = "sql"
    viewtype = ViewArg.SQL

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.encoding = "utf-8"

    def get_file_body(self) -> bytes:
        return self.get_sql().encode(self.encoding)

    def get_sql(self) -> str:
        """
        Returns SQL text representing the SQLite database.
        """
        return self.get_sqlite_data(as_text=True)

    def download_now(self) -> Response:
        """
        Download the data dump in the selected format
        """
        filename = self.get_filename()
        sql_text = self.get_sql()
        return TextAttachmentResponse(body=sql_text, filename=filename)

    def get_data_response(self, body: bytes, filename: str) -> Response:
        """
        Unused.
        """
        pass


# Create mapping from "viewtype" to class.
# noinspection PyTypeChecker
DOWNLOADER_CLASSES = {}  # type: Dict[str, Type[TaskCollectionExporter]]
for _cls in gen_all_subclasses(TaskCollectionExporter):  # type: Type[TaskCollectionExporter]  # noqa
    # noinspection PyTypeChecker
    DOWNLOADER_CLASSES[_cls.viewtype] = _cls


def make_exporter(req: "CamcopsRequest",
                  collection: "TaskCollection",
                  options: DownloadOptions) -> TaskCollectionExporter:
    """

    Args:
        req:
            a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        collection:
            a
            :class:`camcops_server.cc_modules.cc_taskcollection.TaskCollection`
        options:
            :class:`camcops_server.cc_modules.cc_export.DownloadOptions`
            governing the download

    Returns:
        a :class:`BasicTaskCollectionExporter`

    Raises:
        :exc:`HTTPBadRequest` if the arguments are bad
    """
    _ = req.gettext
    if options.delivery_mode not in DownloadOptions.DELIVERY_MODES:
        raise HTTPBadRequest(
            f"{_('Bad delivery mode:')} {options.delivery_mode!r} "
            f"({_('permissible:')} "
            f"{DownloadOptions.DELIVERY_MODES!r})")
    try:
        downloader_class = DOWNLOADER_CLASSES[options.viewtype]
    except KeyError:
        raise HTTPBadRequest(
            f"{_('Bad output type:')} {options.viewtype!r} "
            f"({_('permissible:')} {DOWNLOADER_CLASSES.keys()!r})")
    return downloader_class(
        req=req,
        collection=collection,
        options=options
    )


# =============================================================================
# Represent files for users to download
# =============================================================================

class UserDownloadFile(object):
    """
    Represents a file that has been generated for the user to download.

    Test code:

    .. code-block:: python

        from camcops_server.cc_modules.cc_export import UserDownloadFile
        x = UserDownloadFile("/etc/hosts")

        print(x.when_last_modified)  # should match output of: ls -l /etc/hosts

        many = UserDownloadFile.from_directory_scan("/etc")

    """
    def __init__(self, filename: str, directory: str = "",
                 permitted_lifespan_min: float = 0,
                 req: "CamcopsRequest" = None) -> None:
        """
        Args:
            filename: filename relative to ``directory``
            directory: directory
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`

        Notes:

        - The Unix ``ls`` command shows timestamps in the current timezone.
          Try ``TZ=utc ls -l <filename>`` or ``TZ="America/New_York" ls -l
          <filename>`` to see this.
        - The underlying timestamp is the time (in seconds) since the Unix
          "epoch", which is 00:00:00 UTC on 1 Jan 1970
          (https://en.wikipedia.org/wiki/Unix_time).
        """
        self.filename = filename
        self.directory = directory
        self.permitted_lifespan_min = permitted_lifespan_min
        self.req = req

        self.basename = os.path.basename(filename)
        _, self.extension = os.path.splitext(filename)
        if directory:
            self.fullpath = os.path.join(directory, filename)
        else:
            self.fullpath = filename
        try:
            self.statinfo = os.stat(self.fullpath)
            self.exists = True
        except FileNotFoundError:
            self.statinfo = None  # type: Optional[os.stat_result]
            self.exists = False

    # -------------------------------------------------------------------------
    # Size
    # -------------------------------------------------------------------------

    @property
    def size(self) -> Optional[int]:
        """
        Size of the file, in bytes. Returns ``None`` if the file does not
        exist.
        """
        return self.statinfo.st_size if self.exists else None

    @property
    def size_str(self) -> str:
        """
        Returns a pretty-format string describing the file's size.
        """
        size_bytes = self.size
        if size_bytes is None:
            return ""
        return bytes2human(size_bytes)

    # -------------------------------------------------------------------------
    # Timing
    # -------------------------------------------------------------------------

    @property
    def when_last_modified(self) -> Optional[Pendulum]:
        """
        Returns the file's modification time, or ``None`` if it doesn't exist.

        (Creation time is harder! See
        https://stackoverflow.com/questions/237079/how-to-get-file-creation-modification-date-times-in-python.)
        """  # noqa
        if not self.exists:
            return None
        # noinspection PyTypeChecker
        creation = Pendulum.fromtimestamp(self.statinfo.st_mtime,
                                          tz=get_tz_utc())  # type: Pendulum
        # ... gives the correct time in the UTC timezone
        # ... note that utcfromtimestamp() gives a time without a timezone,
        #     which is unhelpful!
        # We would like this to display in the current timezone:
        return creation.in_timezone(get_tz_local())

    @property
    def when_last_modified_str(self) -> str:
        """
        Returns a formatted string with the file's modification time.
        """
        w = self.when_last_modified
        if not w:
            return ""
        return format_datetime(w, DateFormat.ISO8601_HUMANIZED_TO_SECONDS)

    @property
    def time_left(self) -> Optional[Duration]:
        """
        Returns the amount of time that this file has left to live before
        the server will delete it. Returns ``None`` if the file does not exist.
        """
        if not self.exists:
            return None
        now = get_now_localtz_pendulum()
        death = (
            self.when_last_modified +
            Duration(minutes=self.permitted_lifespan_min)
        )
        remaining = death - now  # type: Period
        # Note that Period is a subclass of Duration, but its __str__()
        # method is different. Duration maps __str__() to in_words(), but
        # Period maps __str__() to __repr__().
        return remaining

    @property
    def time_left_str(self) -> str:
        """
        A string version of :meth:`time_left`.
        """
        t = self.time_left
        if not t:
            return ""
        return t.in_words()  # Duration and Period do nice formatting

    def older_than(self, when: Pendulum) -> bool:
        """
        Was the file created before the specified time?
        """
        m = self.when_last_modified
        if not m:
            return False
        return m < when

    # -------------------------------------------------------------------------
    # Deletion
    # -------------------------------------------------------------------------

    @property
    def delete_form(self) -> str:
        """
        Returns HTML for a form to delete this file.
        """
        if not self.req:
            return ""
        dest_url = self.req.route_url(Routes.DELETE_FILE)
        form = UserDownloadDeleteForm(
            request=self.req,
            action=dest_url
        )
        appstruct = {ViewParam.FILENAME: self.filename}
        rendered_form = form.render(appstruct)
        return rendered_form

    def delete(self) -> None:
        """
        Deletes the file. Does not raise an exception if the file does not
        exist.
        """
        try:
            os.remove(self.fullpath)
            log.info(f"Deleted file: {self.fullpath}")
        except OSError:
            pass

    # -------------------------------------------------------------------------
    # Downloading
    # -------------------------------------------------------------------------

    @property
    def download_url(self) -> str:
        """
        Returns a URL to download this file.
        """
        if not self.req:
            return ""
        querydict = {
            ViewParam.FILENAME: self.filename
        }
        return self.req.route_url(Routes.DOWNLOAD_FILE, _query=querydict)

    @property
    def contents(self) -> Optional[bytes]:
        """
        The file contents. May raise :exc:`OSError` if the read fails.
        """
        if not self.exists:
            return None
        with open(self.fullpath, "rb") as f:
            return f.read()

    # -------------------------------------------------------------------------
    # Bulk creation
    # -------------------------------------------------------------------------

    @classmethod
    def from_directory_scan(
            cls, directory: str,
            permitted_lifespan_min: float = 0,
            req: "CamcopsRequest" = None) -> List["UserDownloadFile"]:
        """
        Scans the directory and returns a list of :class:`UserDownloadFile`
        objects, one for each file in the directory.

        For each object, ``directory`` is the root directory (our parameter
        here), and ``filename`` is the filename RELATIVE to that.

        Args:
            directory: directory to scan
            permitted_lifespan_min: lifespan for each file
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        """
        results = []  # type: List[UserDownloadFile]
        # Imagine directory == "/etc":
        for root, dirs, files in os.walk(directory):
            # ... then root might at times be "/etc/apache2"
            for f in files:
                fullpath = os.path.join(root, f)
                relative_filename = relative_filename_within_dir(
                    fullpath, directory)
                results.append(UserDownloadFile(
                    filename=relative_filename,
                    directory=directory,
                    permitted_lifespan_min=permitted_lifespan_min,
                    req=req
                ))
        return results
