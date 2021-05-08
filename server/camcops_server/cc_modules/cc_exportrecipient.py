#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_exportrecipient.py

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

**ExportRecipient class.**

"""

import logging
from typing import List, Optional, TYPE_CHECKING

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import simple_repr
from cardinal_pythonlib.sqlalchemy.list_types import (
    IntListType,
    StringListType,
)
from cardinal_pythonlib.sqlalchemy.orm_inspect import gen_columns
from cardinal_pythonlib.sqlalchemy.session import get_safe_url_from_url
from sqlalchemy.event.api import listens_for
from sqlalchemy.orm import reconstructor, Session as SqlASession
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import (
    BigInteger,
    Boolean,
    DateTime,
    Integer,
    Text,
)

from camcops_server.cc_modules.cc_exportrecipientinfo import (
    ExportRecipientInfo,
)
from camcops_server.cc_modules.cc_simpleobjects import TaskExportOptions
from camcops_server.cc_modules.cc_sqla_coltypes import (
    EmailAddressColType,
    ExportRecipientNameColType,
    ExportTransmissionMethodColType,
    FileSpecColType,
    HostnameColType,
    UrlColType,
    UserNameExternalColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base

if TYPE_CHECKING:
    from sqlalchemy.engine.base import Connection
    from sqlalchemy.orm.mapper import Mapper
    from camcops_server.cc_modules.cc_task import Task

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# ExportRecipient class
# =============================================================================

class ExportRecipient(ExportRecipientInfo, Base):
    """
    SQLAlchemy ORM class representing an export recipient.

    This has a close relationship with (and inherits from)
    :class:`camcops_server.cc_modules.cc_exportrecipientinfo.ExportRecipientInfo`
    (q.v.).

    Full details of parameters are in the docs for the config file.
    """  # noqa

    __tablename__ = "_export_recipients"

    IGNORE_FOR_EQ_ATTRNAMES = ExportRecipientInfo.IGNORE_FOR_EQ_ATTRNAMES + [
        # Attribute names to ignore for equality comparison
        "id",
        "current",
        "group_names",  # Python only
    ]
    NEEDS_RECOPYING_EACH_TIME_FROM_CONFIG_ATTRNAMES = [
        "email_host_password",
        "redcap_api_key",
    ]

    # -------------------------------------------------------------------------
    # Identifying this object, and whether it's the "live" version
    # -------------------------------------------------------------------------
    id = Column(
        "id", BigInteger,
        primary_key=True, autoincrement=True, index=True,
        comment="Export recipient ID (arbitrary primary key)"
    )
    recipient_name = Column(
        "recipient_name", ExportRecipientNameColType, nullable=False,
        comment="Name of export recipient"
    )
    current = Column(
        "current", Boolean, default=False, nullable=False,
        comment="Is this the current record for this recipient? (If not, it's "
                "a historical record for audit purposes.)"
    )

    # -------------------------------------------------------------------------
    # How to export
    # -------------------------------------------------------------------------
    transmission_method = Column(
        "transmission_method", ExportTransmissionMethodColType, nullable=False,
        comment="Export transmission method (e.g. hl7, file)"
    )
    push = Column(
        "push", Boolean, default=False, nullable=False,
        comment="Push (support auto-export on upload)?"
    )
    task_format = Column(
        "task_format", ExportTransmissionMethodColType,
        comment="Format that task information should be sent in (e.g. PDF), "
                "if not predetermined by the transmission method"
    )
    xml_field_comments = Column(
        "xml_field_comments", Boolean, default=True, nullable=False,
        comment="Whether to include field comments in XML output"
    )

    # -------------------------------------------------------------------------
    # What to export
    # -------------------------------------------------------------------------
    all_groups = Column(
        "all_groups", Boolean, default=False, nullable=False,
        comment="Export all groups? (If not, see group_ids.)"
    )
    group_ids = Column(
        "group_ids", IntListType,
        comment="Integer IDs of CamCOPS group to export data from (as CSV)"
    )
    tasks = Column(
        "tasks", StringListType,
        comment="Base table names of CamCOPS tasks to export data from "
                "(as CSV)"
    )
    start_datetime_utc = Column(
        "start_datetime_utc", DateTime,
        comment="Start date/time for tasks (UTC)"
    )
    end_datetime_utc = Column(
        "end_datetime_utc", DateTime,
        comment="End date/time for tasks (UTC)"
    )
    finalized_only = Column(
        "finalized_only", Boolean, default=True, nullable=False,
        comment="Send only finalized tasks"
    )
    include_anonymous = Column(
        "include_anonymous", Boolean, default=False, nullable=False,
        comment="Include anonymous tasks? "
                "Not applicable to some methods (e.g. HL7)"
    )
    primary_idnum = Column(
        "primary_idnum", Integer, nullable=False,
        comment="Which ID number is used as the primary ID?"
    )
    require_idnum_mandatory = Column(
        "require_idnum_mandatory", Boolean,
        comment="Must the primary ID number be mandatory in the relevant "
                "policy?"
    )

    # -------------------------------------------------------------------------
    # Database
    # -------------------------------------------------------------------------
    db_url = Column(
        "db_url", UrlColType,
        comment="(DATABASE) SQLAlchemy database URL for export"
    )
    db_echo = Column(
        "db_echo", Boolean, default=False, nullable=False,
        comment="(DATABASE) Echo SQL applied to destination database?"
    )
    db_include_blobs = Column(
        "db_include_blobs", Boolean, default=True, nullable=False,
        comment="(DATABASE) Include BLOBs?"
    )
    db_add_summaries = Column(
        "db_add_summaries", Boolean, default=True, nullable=False,
        comment="(DATABASE) Add summary information?"
    )
    db_patient_id_per_row = Column(
        "db_patient_id_per_row", Boolean, default=True, nullable=False,
        comment="(DATABASE) Add patient ID information per row?"
    )

    # -------------------------------------------------------------------------
    # Email
    # -------------------------------------------------------------------------
    email_host = Column(
        "email_host", HostnameColType,
        comment="(EMAIL) E-mail (SMTP) server host name/IP address"
    )
    email_port = Column(
        "email_port", Integer,
        comment="(EMAIL) E-mail (SMTP) server port number"
    )
    email_use_tls = Column(
        "email_use_tls", Boolean, default=True, nullable=False,
        comment="(EMAIL) Use explicit TLS connection?"
    )
    email_host_username = Column(
        "email_host_username", UserNameExternalColType,
        comment="(EMAIL) Username on e-mail server"
    )
    # email_host_password: not stored in database
    email_from = Column(
        "email_from", EmailAddressColType,
        comment='(EMAIL) "From:" address(es)'
    )
    email_sender = Column(
        "email_sender", EmailAddressColType,
        comment='(EMAIL) "Sender:" address(es)'
    )
    email_reply_to = Column(
        "email_reply_to", EmailAddressColType,
        comment='(EMAIL) "Reply-To:" address(es)'
    )
    email_to = Column(
        "email_to", Text,
        comment='(EMAIL) "To:" recipient(s), as a CSV list'
    )
    email_cc = Column(
        "email_cc", Text,
        comment='(EMAIL) "CC:" recipient(s), as a CSV list'
    )
    email_bcc = Column(
        "email_bcc", Text,
        comment='(EMAIL) "BCC:" recipient(s), as a CSV list'
    )
    email_patient_spec = Column(
        "email_patient", FileSpecColType,
        comment="(EMAIL) Patient specification"
    )
    email_patient_spec_if_anonymous = Column(
        "email_patient_spec_if_anonymous", FileSpecColType,
        comment="(EMAIL) Patient specification for anonymous tasks"
    )
    email_subject = Column(
        "email_subject", FileSpecColType,
        comment="(EMAIL) Subject specification"
    )
    email_body_as_html = Column(
        "email_body_as_html", Boolean, default=False, nullable=False,
        comment="(EMAIL) Is the body HTML, rather than plain text?"
    )
    email_body = Column(
        "email_body", Text,
        comment="(EMAIL) Body contents"
    )
    email_keep_message = Column(
        "email_keep_message", Boolean, default=False, nullable=False,
        comment="(EMAIL) Keep entire message?"
    )

    # -------------------------------------------------------------------------
    # HL7
    # -------------------------------------------------------------------------
    hl7_host = Column(
        "hl7_host", HostnameColType,
        comment="(HL7) Destination host name/IP address"
    )
    hl7_port = Column(
        "hl7_port", Integer,
        comment="(HL7) Destination port number"
    )
    hl7_ping_first = Column(
        "hl7_ping_first", Boolean, default=False, nullable=False,
        comment="(HL7) Ping via TCP/IP before sending HL7 messages?"
    )
    hl7_network_timeout_ms = Column(
        "hl7_network_timeout_ms", Integer,
        comment="(HL7) Network timeout (ms)."
    )
    hl7_keep_message = Column(
        "hl7_keep_message", Boolean, default=False, nullable=False,
        comment="(HL7) Keep copy of message in database? (May be large!)"
    )
    hl7_keep_reply = Column(
        "hl7_keep_reply", Boolean, default=False, nullable=False,
        comment="(HL7) Keep copy of server's reply in database?"
    )
    hl7_debug_divert_to_file = Column(
        "hl7_debug_divert_to_file", Boolean, default=False, nullable=False,
        comment="(HL7 debugging option) Divert messages to files?"
    )
    hl7_debug_treat_diverted_as_sent = Column(
        "hl7_debug_treat_diverted_as_sent", Boolean,
        default=False, nullable=False,
        comment="(HL7 debugging option) Treat messages diverted to file as sent"  # noqa
    )

    # -------------------------------------------------------------------------
    # File
    # -------------------------------------------------------------------------
    file_patient_spec = Column(
        "file_patient_spec", FileSpecColType,
        comment="(FILE) Patient part of filename specification"
    )
    file_patient_spec_if_anonymous = Column(
        "file_patient_spec_if_anonymous", FileSpecColType,
        comment="(FILE) Patient part of filename specification for anonymous tasks"  # noqa
    )
    file_filename_spec = Column(
        "file_filename_spec", FileSpecColType,
        comment="(FILE) Filename specification"
    )
    file_make_directory = Column(
        "file_make_directory", Boolean, default=True, nullable=False,
        comment="(FILE) Make destination directory if it doesn't already exist"
    )
    file_overwrite_files = Column(
        "file_overwrite_files", Boolean, default=False, nullable=False,
        comment="(FILE) Overwrite existing files"
    )
    file_export_rio_metadata = Column(
        "file_export_rio_metadata", Boolean, default=False, nullable=False,
        comment="(FILE) Export RiO metadata file along with main file?"
    )
    file_script_after_export = Column(
        "file_script_after_export", Text,
        comment="(FILE) Command/script to run after file export"
    )

    # -------------------------------------------------------------------------
    # File/RiO
    # -------------------------------------------------------------------------
    rio_idnum = Column(
        "rio_idnum", Integer,
        comment="(FILE / RiO) RiO metadata: which ID number is the RiO ID?"
    )
    rio_uploading_user = Column(
        "rio_uploading_user", Text,
        comment="(FILE / RiO) RiO metadata: name of automatic upload user"
    )
    rio_document_type = Column(
        "rio_document_type", Text,
        comment="(FILE / RiO) RiO metadata: document type for RiO"
    )

    # -------------------------------------------------------------------------
    # REDCap export
    # -------------------------------------------------------------------------
    redcap_api_url = Column(
        "redcap_api_url", Text,
        comment="(REDCap) REDCap API URL, pointing to the REDCap server"
    )
    redcap_fieldmap_filename = Column(
        "redcap_fieldmap_filename", Text,
        comment="(REDCap) File defining CamCOPS-to-REDCap field mapping"
    )

    def __init__(self, *args, **kwargs) -> None:
        """
        Creates a blank :class:`ExportRecipient` object.

        NB not called when SQLAlchemy objects loaded from database; see
        :meth:`init_on_load` instead.
        """
        super().__init__(*args, **kwargs)

    def __hash__(self) -> int:
        """
        Used by the ``merge_db`` function, and specifically the old-to-new map
        maintained by :func:`cardinal_pythonlib.sqlalchemy.merge_db.merge_db`.
        """
        return hash(f"{self.id}_{self.recipient_name}")

    @reconstructor
    def init_on_load(self) -> None:
        """
        Called when SQLAlchemy recreates an object; see
        https://docs.sqlalchemy.org/en/latest/orm/constructors.html.
        """
        # Python only:
        self.group_names = []  # type: List[str]
        self.email_host_password = ""
        self.redcap_api_key = ""

    def get_attrnames(self) -> List[str]:
        """
        Returns all relevant attribute names.
        """
        attrnames = set([attrname for attrname, _ in gen_columns(self)])
        attrnames.update(key for key in self.__dict__ if not key.startswith('_'))  # noqa
        return sorted(attrnames)

    def __repr__(self) -> str:
        return simple_repr(self, self.get_attrnames())

    def is_upload_suitable_for_push(self, tablename: str,
                                    uploading_group_id: int) -> bool:
        """
        Might an upload potentially give tasks to be "pushed"?

        Called by
        :func:`camcops_server.cc_modules.cc_client_api_core.get_task_push_export_pks`.

        Args:
            tablename: table name being uploaded
            uploading_group_id: group ID if the uploading user

        Returns:
            whether this upload should be considered further
        """
        if not self.push:
            # Not a push export recipient
            return False
        if self.tasks and tablename not in self.tasks:
            # Recipient is restricted to tasks that don't include the table
            # being uploaded (or, the table is a subtable that we don't care
            # about)
            return False
        if not self.all_groups:
            # Recipient is restricted to specific groups
            if uploading_group_id not in self.group_ids:
                # Wrong group!
                return False
        return True

    def is_task_suitable(self, task: "Task") -> bool:
        """
        Used as a double-check that a task remains suitable.

        Args:
            task: a :class:`camcops_server.cc_modules.cc_task.Task`

        Returns:
            bool: is the task suitable for this recipient?
        """
        def _warn(reason: str) -> None:
            log.info("For recipient {}, task {!r} is unsuitable: {}",
                     self, task, reason)
            # Not a warning, actually; it's normal to see these because it
            # allows the client API to skip some checks for speed.

        if self.tasks and task.tablename not in self.tasks:
            _warn(f"Task type {task.tablename!r} not included")
            return False

        if not self.all_groups:
            task_group_id = task.group_id
            if task_group_id not in self.group_ids:
                _warn(f"group_id {task_group_id} not permitted")
                return False

        if not self.include_anonymous and task.is_anonymous:
            _warn("task is anonymous")
            return False

        if self.finalized_only and not task.is_preserved():
            _warn("task not finalized")
            return False

        if self.start_datetime_utc or self.end_datetime_utc:
            task_dt = task.get_creation_datetime_utc_tz_unaware()
            if self.start_datetime_utc and task_dt < self.start_datetime_utc:
                _warn("task created before recipient start_datetime_utc")
                return False
            if self.end_datetime_utc and task_dt >= self.end_datetime_utc:
                _warn("task created at/after recipient end_datetime_utc")
                return False

        if (not task.is_anonymous and
                self.primary_idnum is not None):
            patient = task.patient
            if not patient:
                _warn("missing patient")
                return False
            if not patient.has_idnum_type(self.primary_idnum):
                _warn(f"task's patient is missing ID number type "
                      f"{self.primary_idnum}")
                return False

        return True

    @classmethod
    def get_existing_matching_recipient(cls,
                                        dbsession: SqlASession,
                                        recipient: "ExportRecipient") \
            -> Optional["ExportRecipient"]:
        """
        Retrieves an active instance from the database that matches ``other``,
        if there is one.

        Args:
            dbsession: a :class:`sqlalchemy.orm.session.Session`
            recipient: an :class:`ExportRecipient`

        Returns:
            a database instance of :class:`ExportRecipient` that matches, or
            ``None``.
        """
        # noinspection PyPep8
        q = dbsession.query(cls).filter(
            cls.recipient_name == recipient.recipient_name,
            cls.current == True)  # noqa: E712
        results = q.all()
        if len(results) > 1:
            raise ValueError(
                "Database has gone wrong: more than one active record for "
                "{t}.{c} = {r}".format(
                    t=cls.__tablename__,
                    c=cls.recipient_name.name,  # column name from Column
                    r=recipient.recipient_name,
                )
            )
        if results:
            r = results[0]
            if recipient == r:
                return r
        return None

    @property
    def db_url_obscuring_password(self) -> Optional[str]:
        """
        Returns the database URL (if present), but with its password obscured.
        """
        if not self.db_url:
            return self.db_url
        return get_safe_url_from_url(self.db_url)

    def get_task_export_options(self) -> TaskExportOptions:
        return TaskExportOptions(
            xml_include_comments=self.xml_field_comments,
            xml_with_header_comments=self.xml_field_comments,
        )


# noinspection PyUnusedLocal
@listens_for(ExportRecipient, "after_insert")
@listens_for(ExportRecipient, "after_update")
def _check_current(mapper: "Mapper",
                   connection: "Connection",
                   target: ExportRecipient) -> None:
    """
    Ensures that only one :class:`ExportRecipient` is marked as ``current``
    per ``recipient_name``.

    As per
    https://stackoverflow.com/questions/6269469/mark-a-single-row-in-a-table-in-sqlalchemy.
    """  # noqa
    if target.current:
        # noinspection PyUnresolvedReferences
        connection.execute(
            ExportRecipient.__table__.update()
            .values(current=False)
            .where(ExportRecipient.recipient_name == target.recipient_name)
            .where(ExportRecipient.id != target.id)
        )
