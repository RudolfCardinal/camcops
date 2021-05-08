#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_client_api_core.py

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

**Core constants and functions used by the client (tablet device) API.**

"""

from typing import (Any, Dict, Iterable, List, NoReturn,
                    Optional, Set, TYPE_CHECKING)

from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.reprfunc import simple_repr
from pendulum import DateTime as Pendulum
from sqlalchemy.sql.expression import literal, select
from sqlalchemy.sql.schema import Table

from camcops_server.cc_modules.cc_constants import (
    CLIENT_DATE_FIELD,
    DateFormat,
    ERA_NOW,
    MOVE_OFF_TABLET_FIELD,
)
from camcops_server.cc_modules.cc_db import (
    FN_ADDITION_PENDING,
    FN_CURRENT,
    FN_DEVICE_ID,
    FN_ERA,
    FN_FORCIBLY_PRESERVED,
    FN_PK,
    FN_PREDECESSOR_PK,
    FN_PRESERVING_USER_ID,
    FN_REMOVAL_PENDING,
    FN_REMOVING_USER_ID,
    FN_SUCCESSOR_PK,
    FN_WHEN_REMOVED_BATCH_UTC,
    FN_WHEN_REMOVED_EXACT,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient
    from camcops_server.cc_modules.cc_request import CamcopsRequest


# =============================================================================
# Constants
# =============================================================================

class TabletParam(object):
    """
    Keys used by server or client (in the comments: S server, C client, B
    bidirectional).
    """
    ADDRESS = "address"  # C->S, in JSON, v2.3.0
    ANONYMOUS = "anonymous"  # S->C; new in v2.4.0
    CAMCOPS_VERSION = "camcops_version"  # C->S
    COMPLETE = "complete"  # S->C; new in v2.4.0
    DATABASE_TITLE = "databaseTitle"  # S->C
    DATEVALUES = "datevalues"  # C->S
    DBDATA = "dbdata"  # C->S, v2.3.0
    DEVICE = "device"  # C->S
    DEVICE_FRIENDLY_NAME = "devicefriendlyname"  # C->S
    DOB = "dob"  # C->S, in JSON, v2.3.0
    DUE_BY = "due_by"  # C->S; new in v2.4.0
    DUE_FROM = "due_from"  # C->S; new in v2.4.0
    EMAIL = "email"  # C->S; new in v2.4.0
    ERROR = "error"  # S->C
    FIELDS = "fields"  # B
    FINALIZING = "finalizing"  # C->S, in JSON and upload_entire_database, v2.3.0; synonym for preserving  # noqa
    FORENAME = "forename"  # C->S, in JSON, v2.3.0
    GP = "gp"  # C->S, in JSON, v2.3.0
    ID_DESCRIPTION_PREFIX = "idDescription"  # S->C
    ID_POLICY_FINALIZE = "idPolicyFinalize"  # S->C
    ID_POLICY_UPLOAD = "idPolicyUpload"  # S->C
    ID_SHORT_DESCRIPTION_PREFIX = "idShortDescription"  # S->C
    ID_VALIDATION_METHOD_PREFIX = "idValidationMethod"  # S->C; new in v2.2.8
    IDNUM_PREFIX = "idnum"  # C->S, in JSON, v2.3.0
    IP_USE_INFO = "ip_use_info"  # S->C; new in v2.4.0
    IP_USE_COMMERCIAL = "ip_use_commercial"  # S->C; new in v2.4.0
    IP_USE_CLINICAL = "ip_use_clinical"  # S->C; new in v2.4.0
    IP_USE_EDUCATIONAL = "ip_use_educational"  # S->C; new in v2.4.0
    IP_USE_RESEARCH = "ip_use_research"  # S->C; new in v2.4.0
    MOVE_OFF_TABLET_VALUES = "move_off_tablet_values"  # C->S, v2.3.0
    NFIELDS = "nfields"  # B
    NRECORDS = "nrecords"  # B
    OPERATION = "operation"  # C->S
    OTHER = "other"  # C->S, in JSON, v2.3.0
    PASSWORD = "password"  # C->S
    PATIENT_INFO = "patient_info"  # C->S; new in v2.3.0, S->C new in v2.4.0
    PATIENT_PROQUINT = "patient_proquint"  # C->S; new in v2.4.0
    PKNAME = "pkname"  # C->S
    PKNAMEINFO = "pknameinfo"  # C->S, new in v2.3.0
    PKVALUES = "pkvalues"  # C->S
    RECORD_PREFIX = "record"  # B
    RESULT = "result"  # S->C
    SERVER_CAMCOPS_VERSION = "serverCamcopsVersion"  # S->C
    SESSION_ID = "session_id"  # B
    SESSION_TOKEN = "session_token"  # B
    SETTINGS = "settings"  # S->C; new in v2.4.0
    SEX = "sex"  # C->S, in JSON, v2.3.0
    SUCCESS = "success"  # S->C
    SURNAME = "surname"  # C->S, in JSON, v2.3.0
    TABLE = "table"  # C->S
    TABLES = "tables"  # C->S
    TASK_SCHEDULES = "task_schedules"  # S->C; new in v2.4.0
    TASK_SCHEDULE_ITEMS = "task_schedule_items"  # S->C; new in v2.4.0
    TASK_SCHEDULE_NAME = "task_schedule_name"  # S->C; new in v2.4.0
    USER = "user"  # C->S
    VALUES = "values"  # C->S
    WHEN_COMPLETED = "when_completed"  # S->C; new in v2.4.0

    # Retired (part of defunct mobileweb interface):
    # WHEREFIELDS = "wherefields"
    # WHERENOTFIELDS = "wherenotfields"
    # WHERENOTVALUES = "wherenotvalues"
    # WHEREVALUES = "wherevalues"


class ExtraStringFieldNames(object):
    """
    To match ``extrastring.cpp`` on the tablet.
    """
    TASK = "task"
    NAME = "name"
    LANGUAGE = "language"
    VALUE = "value"


class AllowedTablesFieldNames(object):
    """
    To match ``allowedservertable.cpp`` on the tablet
    """
    TABLENAME = "tablename"
    MIN_CLIENT_VERSION = "min_client_version"


# =============================================================================
# Exceptions used by client API
# =============================================================================
# Note the following about exception strings:
#
# class Blah(Exception):
#     pass
#
# x = Blah("hello")
# str(x)  # 'hello'

class UserErrorException(Exception):
    """
    Exception class for when the input from the tablet is dodgy.
    """
    pass


class ServerErrorException(Exception):
    """
    Exception class for when something's broken on the server side.
    """
    pass


class IgnoringAntiqueTableException(Exception):
    """
    Special exception to return success when we're ignoring an old tablet's
    request to upload the "storedvars" table.
    """
    pass


# =============================================================================
# Return message functions
# =============================================================================

def exception_description(e: Exception) -> str:
    """
    Returns a formatted description of a Python exception.
    """
    return f"{type(e).__name__}: {str(e)}"


# NO LONGER USED:
# def succeed_generic(operation: str) -> str:
#     """
#     Generic success message to tablet.
#     """
#     return "CamCOPS: {}".format(operation)


def fail_user_error(msg: str) -> NoReturn:
    """
    Function to abort the script when the input is dodgy.

    Raises :exc:`UserErrorException`.
    """
    # While Titanium-Android can extract error messages from e.g.
    # finish("400 Bad Request: @_"), Titanium-iOS can't, and we need the error
    # messages. Therefore, we will return an HTTP success code but "Success: 0"
    # in the reply details.
    raise UserErrorException(msg)


def require_keys(dictionary: Dict[Any, Any], keys: List[Any]) -> None:
    """
    Ensure that all listed keys are present in the specified dictionary, or
    raise a :exc:`UserErrorException`.
    """
    for k in keys:
        if k not in dictionary:
            fail_user_error(f"Field {repr(k)} missing in client input")


def fail_user_error_from_exception(e: Exception) -> NoReturn:
    """
    Raise :exc:`UserErrorException` with a description that comes from
    the specified exception.
    """
    fail_user_error(exception_description(e))


def fail_server_error(msg: str) -> NoReturn:
    """
    Function to abort the script when something's broken server-side.

    Raises :exc:`ServerErrorException`.
    """
    raise ServerErrorException(msg)


def fail_server_error_from_exception(e: Exception) -> NoReturn:
    """
    Raise :exc:`ServerErrorException` with a description that comes from
    the specified exception.
    """
    fail_server_error(exception_description(e))


def fail_unsupported_operation(operation: str) -> NoReturn:
    """
    Abort the script (with a :exc:`UserErrorException`) when the
    operation is invalid.
    """
    fail_user_error(f"operation={operation}: not supported")


# =============================================================================
# Information classes used during upload
# =============================================================================

class BatchDetails(object):
    """
    Represents a current upload batch.
    """
    def __init__(self,
                 batchtime: Optional[Pendulum] = None,
                 preserving: bool = False,
                 onestep: bool = False) -> None:
        """
        Args:
            batchtime:
                the batchtime; UTC time this upload batch started; will be
                applied to all changes
            preserving:
                are we preserving (finalizing) the records -- that is, moving
                them from the current era (``NOW``) to the ``batchtime`` era,
                so they can be deleted from the tablet without apparent loss on
                the server?
            onestep:
                is this a one-step whole-database upload?
        """
        self.batchtime = batchtime
        self.preserving = preserving
        self.onestep = onestep

    def __repr__(self) -> str:
        return simple_repr(self, ["batchtime", "preserving", "onestep"])

    @property
    def new_era(self) -> str:
        """
        Returns the string used for the new era for this batch, in case we
        are preserving records.
        """
        return format_datetime(self.batchtime, DateFormat.ERA)


class WhichKeyToSendInfo(object):
    """
    Represents information the client has sent, asking us which records it
    needs to upload recordwise.
    """
    def __init__(self,
                 client_pk: int,
                 client_when: Pendulum,
                 client_move_off_tablet: bool) -> None:
        self.client_pk = client_pk
        self.client_when = client_when
        self.client_move_off_tablet = client_move_off_tablet


class ServerRecord(object):
    """
    Class to represent whether a server record exists, and/or the results of
    retrieving server records.
    """
    def __init__(self,
                 client_pk: int = None,
                 exists_on_server: bool = False,
                 server_pk: int = None,
                 server_when: Pendulum = None,
                 move_off_tablet: bool = False,
                 current: bool = False,
                 addition_pending: bool = False,
                 removal_pending: bool = False,
                 predecessor_pk: int = None,
                 successor_pk: int = None) -> None:
        """
        Args:
            client_pk: client's PK
            exists_on_server: does the record exist on the server?
            server_pk: if it exists, what's the server PK?
            server_when: if it exists, what's the server's "when"
                (``when_last_modified``) field?
            move_off_tablet: is the ``__move_off_tablet`` flag set?
            current: is the record current (``_current`` flag set)?
            addition_pending: is the ``_addition_pending`` flag set?
            removal_pending: is the ``_removal_pending`` flag set?
            predecessor_pk: predecessor server PK, or ``None``
            successor_pk: successor server PK, or ``None``
        """
        self.client_pk = client_pk
        self.exists = exists_on_server
        self.server_pk = server_pk
        self.server_when = server_when
        self.move_off_tablet = move_off_tablet
        self.current = current
        self.addition_pending = addition_pending
        self.removal_pending = removal_pending
        self.predecessor_pk = predecessor_pk
        self.successor_pk = successor_pk

    def __repr__(self) -> str:
        return simple_repr(self, [
            "client_pk", "exists", "server_pk", "server_when",
            "move_off_tablet", "current",
            "addition_pending", "removal_pending",
            "predecessor_pk", "successor_pk",
        ])


class UploadRecordResult(object):
    """
    Represents the result of uploading a record.
    """
    def __init__(self,
                 oldserverpk: Optional[int] = None,
                 newserverpk: Optional[int] = None,
                 dirty: bool = False,
                 specifically_marked_for_preservation: bool = False):
        """
        Args:
            oldserverpk:
                the server's PK of the old version of the record; ``None`` if
                the record is new
            newserverpk:
                the server's PK of the new version of the record; ``None`` if
                the record was unmodified
            dirty:
                was the database table modified? (May be true even if
                ``newserverpk`` is ``None``, if ``_move_off_tablet`` was set.
            specifically_marked_for_preservation:
                should the record(s) be preserved?
        """
        self.oldserverpk = oldserverpk
        self.newserverpk = newserverpk
        self.dirty = dirty
        self.specifically_marked_for_preservation = specifically_marked_for_preservation  # noqa
        self._specifically_marked_preservation_pks = []  # type: List[int]

    def __repr__(self) -> str:
        return simple_repr(self, [
            "oldserverpk", "newserverpk", "dirty",
            "to_be_preserved", "specifically_marked_preservation_pks"])

    def note_specifically_marked_preservation_pks(self,
                                                  pks: List[int]) -> None:
        """
        Notes that some PKs are marked specifically for preservation.
        """
        self._specifically_marked_preservation_pks.extend(pks)

    @property
    def latest_pk(self) -> Optional[int]:
        """
        Returns the latest of the two PKs.
        """
        if self.newserverpk is not None:
            return self.newserverpk
        return self.oldserverpk

    @property
    def specifically_marked_preservation_pks(self) -> List[int]:
        """
        Returns a list of server PKs of records specifically marked to be
        preserved. This may include older versions (the predecessor chain) of
        records being uploaded.
        """
        return self._specifically_marked_preservation_pks

    @property
    def addition_pks(self) -> List[int]:
        """
        Returns a list of PKs representing new records being added.
        """
        return [self.newserverpk] if self.newserverpk is not None else []

    @property
    def removal_modified_pks(self) -> List[int]:
        """
        Returns a list of PKs representing records removed because they have
        been "modified out".
        """
        if self.oldserverpk is not None and self.newserverpk is not None:
            return [self.oldserverpk]
        return []

    @property
    def all_pks(self) -> List[int]:
        """
        Returns all PKs (old, new, or both).
        """
        return list(x for x in [self.oldserverpk, self.newserverpk]
                    if x is not None)

    @property
    def current_pks(self) -> List[int]:
        """
        Returns PKs that represent current records on the server.
        """
        if self.newserverpk is not None:
            return [self.newserverpk]  # record was replaced; new one's current
        if self.oldserverpk is not None:
            return [self.oldserverpk]  # not replaced; old one's current
        return []


class UploadTableChanges(object):
    """
    Represents information to process and audit an upload to a table.
    """

    def __init__(self, table: Table) -> None:
        self.table = table
        self._addition_pks = set()  # type: Set[int]
        self._removal_modified_pks = set()  # type: Set[int]
        self._removal_deleted_pks = set()  # type: Set[int]
        self._preservation_pks = set()  # type: Set[int]
        self._current_pks = set()  # type: Set[int]

    # -------------------------------------------------------------------------
    # Basic info
    # -------------------------------------------------------------------------

    @property
    def tablename(self) -> str:
        """
        The table's name.
        """
        return self.table.name

    # -------------------------------------------------------------------------
    # Tell us about PKs
    # -------------------------------------------------------------------------

    def note_addition_pk(self, pk: int) -> None:
        """
        Records an "addition" PK.
        """
        self._addition_pks.add(pk)

    def note_addition_pks(self, pks: Iterable[int]) -> None:
        """
        Records multiple "addition" PKs.
        """
        self._addition_pks.update(pks)

    def note_removal_modified_pk(self, pk: int) -> None:
        """
        Records a "removal because modified" PK (replaced by successor).
        """
        self._removal_modified_pks.add(pk)

    def note_removal_modified_pks(self, pks: Iterable[int]) -> None:
        """
        Records multiple "removal because modified" PKs.
        """
        self._removal_modified_pks.update(pks)

    def note_removal_deleted_pk(self, pk: int) -> None:
        """
        Records a "deleted" PK (removed with no successor).
        """
        self._removal_deleted_pks.add(pk)

    def note_removal_deleted_pks(self, pks: Iterable[int]) -> None:
        """
        Records multiple "deleted" PKs (see :func:`note_removal_deleted_pk`).
        """
        self._removal_deleted_pks.update(pks)

    def note_preservation_pk(self, pk: int) -> None:
        """
        Records a "preservation" PK (a record that's being finalized).
        """
        self._preservation_pks.add(pk)

    def note_preservation_pks(self, pks: Iterable[int]) -> None:
        """
        Records multiple "preservation" PKs (records that are being finalized).
        """
        self._preservation_pks.update(pks)

    def note_current_pk(self, pk: int) -> None:
        """
        Records that a record is current on the server. For indexing.
        """
        self._current_pks.add(pk)

    def note_current_pks(self, pks: Iterable[int]) -> None:
        """
        Records multiple "current" PKs.
        """
        self._current_pks.update(pks)

    def note_urr(self, urr: UploadRecordResult,
                 preserving_new_records: bool) -> None:
        """
        Records information from a :class:`UploadRecordResult`, which is itself
        the result of calling
        :func:`camcops_server.cc_modules.client_api.upload_record_core`.

        Called by
        :func:`camcops_server.cc_modules.client_api.process_table_for_onestep_upload`.

        Args:
            urr: a :class:`UploadRecordResult`
            preserving_new_records: are new records being preserved?
        """  # noqa
        self.note_addition_pks(urr.addition_pks)
        self.note_removal_modified_pks(urr.removal_modified_pks)
        if preserving_new_records:
            self.note_preservation_pks(urr.addition_pks)
        self.note_preservation_pks(urr.specifically_marked_preservation_pks)
        self.note_current_pks(urr.current_pks)

    def note_serverrec(self, sr: ServerRecord,
                       preserving: bool) -> None:
        """
        Records information from a :class:`ServerRecord`. Called by
        :func:`camcops_server.cc_modules.client_api.commit_table`.

        Args:
            sr: a :class:`ServerRecord`
            preserving: are we preserving uploaded records?
        """
        pk = sr.server_pk
        if sr.addition_pending:
            self.note_addition_pk(pk)
            self.note_current_pk(pk)
        elif sr.removal_pending:
            if sr.successor_pk is None:
                self.note_removal_deleted_pk(pk)
            else:
                self.note_removal_modified_pk(pk)
        elif sr.current:
            self.note_current_pk(pk)
        if preserving or sr.move_off_tablet:
            self.note_preservation_pk(pk)

    # -------------------------------------------------------------------------
    # Counts
    # -------------------------------------------------------------------------

    @property
    def n_added(self) -> int:
        """
        Number of server records added.
        """
        return len(self._addition_pks)

    @property
    def n_removed_modified(self) -> int:
        """
        Number of server records "modified out" -- replaced by a modified
        version and marked as removed.
        """
        return len(self._removal_modified_pks)

    @property
    def n_removed_deleted(self) -> int:
        """
        Number of server records "deleted" -- marked as removed with no
        successor.
        """
        return len(self._removal_deleted_pks)

    @property
    def n_removed(self) -> int:
        """
        Number of server records "removed" -- marked as removed (either with or
        without a successor).
        """
        return self.n_removed_modified + self.n_removed_deleted

    @property
    def n_preserved(self) -> int:
        """
        Number of server records "preserved" (finalized) -- moved from the
        ``NOW`` era to the batch era (and no longer modifiable by the client
        device).
        """
        return len(self._preservation_pks)

    # -------------------------------------------------------------------------
    # PKs for various purposes
    # -------------------------------------------------------------------------

    @property
    def addition_pks(self) -> List[int]:
        """
        Server PKs of records being added.
        """
        return sorted(self._addition_pks)

    @property
    def removal_modified_pks(self) -> List[int]:
        """
        Server PKs of records being modified out.
        """
        return sorted(self._removal_modified_pks)

    @property
    def removal_deleted_pks(self) -> List[int]:
        """
        Server PKs of records being deleted.
        """
        return sorted(self._removal_deleted_pks)

    @property
    def removal_pks(self) -> List[int]:
        """
        Server PKs of records being removed (modified out, or deleted).
        """
        return sorted(self._removal_modified_pks | self._removal_deleted_pks)

    @property
    def preservation_pks(self) -> List[int]:
        """
        Server PKs of records being preserved.
        """
        return sorted(self._preservation_pks)

    @property
    def current_pks(self) -> List[int]:
        return sorted(self._current_pks)

    @property
    def idnum_delete_index_pks(self) -> List[int]:
        """
        Server PKs of records to delete old index entries for, if this is the
        ID number table. (Includes records that need re-indexing.)

        We don't care about preservation PKs here, as the ID number index
        doesn't incorporate that.
        """
        return sorted(self._removal_modified_pks | self._removal_deleted_pks)

    @property
    def idnum_add_index_pks(self) -> List[int]:
        """
        Server PKs of records to add index entries for, if this is the ID
        number table.
        """
        return sorted(self._addition_pks)

    @property
    def task_delete_index_pks(self) -> List[int]:
        """
        Server PKs of records to delete old index entries for, if this is a
        task table. (Includes records that need re-indexing.)
        """
        return sorted(
            (self._removal_modified_pks |  # needs reindexing
             self._removal_deleted_pks |  # gone
             self._preservation_pks) -  # needs reindexing
            self._addition_pks  # won't be indexed, so no need to delete index
        )

    @property
    def task_reindex_pks(self) -> List[int]:
        """
        Server PKs of records to rebuild index entries for, if this is a task
        table. (Includes records that need re-indexing.)

        We include records being preserved, because their era has changed,
        and the index includes era. Unless they are being removed!
        """
        return sorted(
            (
                (self._addition_pks |  # new; index
                 self._preservation_pks) -  # reindex (but only if current)
                (self._removal_modified_pks |  # modified out; don't index
                 self._removal_deleted_pks)  # deleted; don't index
            ) & self._current_pks  # only reindex current PKs
        )
        # A quick reminder, since I got this wrong:
        # | union (A or B)
        # & intersection (A and B)
        # ^ xor (A or B but not both)
        # - difference (A - B)

    def get_task_push_export_pks(self,
                                 recipient: "ExportRecipient",
                                 uploading_group_id: int) -> List[int]:
        """
        Returns PKs for tasks matching the requirements of a particular
        export recipient.

        (In practice, only "push" recipients will come our way, so we can
        ignore this.)
        """
        if not recipient.is_upload_suitable_for_push(
                tablename=self.tablename,
                uploading_group_id=uploading_group_id):
            # Not suitable
            return []

        if recipient.finalized_only:
            return sorted(
                self._preservation_pks  # finalized
                & self._current_pks  # only send current tasks
            )
        else:
            return sorted(
                (
                    self._addition_pks |  # new (may be unfinalized)
                    self._preservation_pks  # finalized
                ) & self._current_pks  # only send current tasks
            )

    # -------------------------------------------------------------------------
    # Audit info
    # -------------------------------------------------------------------------

    @property
    def any_changes(self) -> bool:
        """
        Has anything changed that we're aware of?
        """
        return (self.n_added > 0 or self.n_removed_modified > 0 or
                self.n_removed_deleted > 0 or self.n_preserved > 0)

    def __str__(self) -> str:
        return (
            f"{self.tablename}: "
            f"({self.n_added} added, "
            f"PKs {self.addition_pks}; "
            f"{self.n_removed_modified} modified out, "
            f"PKs {self.removal_modified_pks}; "
            f"{self.n_removed_deleted} deleted, "
            f"PKs {self.removal_deleted_pks}; "
            f"{self.n_preserved} preserved, "
            f"PKs {self.preservation_pks}; "
            f"current PKs {self.current_pks})"
        )

    def description(self, always_show_current_pks: bool = True) -> str:
        """
        Short description, only including bits that have changed.
        """
        parts = []  # type: List[str]
        if self._addition_pks:
            parts.append(f"{self.n_added} added, PKs {self.addition_pks}")
        if self._removal_modified_pks:
            parts.append(
                f"{self.n_removed_modified} modified out, "
                f"PKs {self.removal_modified_pks}")
        if self._removal_deleted_pks:
            parts.append(
                f"{self.n_removed_deleted} deleted, "
                f"PKs {self.removal_deleted_pks}")
        if self._preservation_pks:
            parts.append(
                f"{self.n_preserved} preserved, "
                f"PKs {self.preservation_pks}")
        if not parts:
            parts.append("no changes")
        if always_show_current_pks or self.any_changes:
            parts.append(f"current PKs {self.current_pks}")
        return f"{self.tablename} ({'; '.join(parts)})"


# =============================================================================
# Value dictionaries for updating records, to reduce repetition
# =============================================================================

def values_delete_later() -> Dict[str, Any]:
    """
    Field/value pairs to mark a record as "to be deleted later".
    """
    return {
        FN_REMOVAL_PENDING: 1,
        FN_SUCCESSOR_PK: None
    }


def values_delete_now(req: "CamcopsRequest",
                      batchdetails: BatchDetails) -> Dict[str, Any]:
    """
    Field/value pairs to mark a record as deleted now.
    """
    return {
        FN_CURRENT: 0,
        FN_REMOVAL_PENDING: 0,
        FN_REMOVING_USER_ID: req.user_id,
        FN_WHEN_REMOVED_EXACT: req.now,
        FN_WHEN_REMOVED_BATCH_UTC: batchdetails.batchtime
    }


def values_preserve_now(req: "CamcopsRequest",
                        batchdetails: BatchDetails,
                        forcibly_preserved: bool = False) -> Dict[str, Any]:
    """
    Field/value pairs to mark a record as preserved now.
    """
    return {
        FN_ERA: batchdetails.new_era,
        FN_PRESERVING_USER_ID: req.user_id,
        MOVE_OFF_TABLET_FIELD: 0,
        FN_FORCIBLY_PRESERVED: forcibly_preserved,
    }


# =============================================================================
# CamCOPS table reading functions
# =============================================================================

def get_server_live_records(req: "CamcopsRequest",
                            device_id: int,
                            table: Table,
                            clientpk_name: str = None,
                            current_only: bool = True) -> List[ServerRecord]:
    """
    Gets details of all records on the server, for the specified table,
    that are live on this client device.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        device_id: ID of the
            :class:`camcops_server.cc_modules.cc_device.Device`
        table: an SQLAlchemy :class:`Table`
        clientpk_name: the column name of the client's PK; if none is supplied,
            the client_pk fields of the results will be ``None``
        current_only: restrict to "current" (``_current``) records only?

    Returns:
        :class:`ServerRecord` objects for active records (``_current`` and in
        the 'NOW' era) for the specified device/table.
    """
    recs = []  # type: List[ServerRecord]
    client_pk_clause = table.c[clientpk_name] if clientpk_name else literal(None)   # noqa
    query = (
        select([
            client_pk_clause,  # 0: client PK (or None)
            table.c[FN_PK],  # 1: server PK
            table.c[CLIENT_DATE_FIELD],  # 2: when last modified (on the server)
            table.c[MOVE_OFF_TABLET_FIELD],  # 3: move_off_tablet
            table.c[FN_CURRENT],  # 4: current
            table.c[FN_ADDITION_PENDING],  # 5
            table.c[FN_REMOVAL_PENDING],  # 6
            table.c[FN_PREDECESSOR_PK],  # 7
            table.c[FN_SUCCESSOR_PK],  # 8
        ])
        .where(table.c[FN_DEVICE_ID] == device_id)
        .where(table.c[FN_ERA] == ERA_NOW)
    )
    if current_only:
        query = query.where(table.c[FN_CURRENT])
    rows = req.dbsession.execute(query)
    for row in rows:
        recs.append(ServerRecord(
            client_pk=row[0],
            exists_on_server=True,
            server_pk=row[1],
            server_when=row[2],
            move_off_tablet=row[3],
            current=row[4],
            addition_pending=row[5],
            removal_pending=row[6],
            predecessor_pk=row[7],
            successor_pk=row[8],
        ))
    return recs
