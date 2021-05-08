#!/usr/bin/env python

"""
camcops_server/cc_modules/client_api.py

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

**Implements the API through which client devices (tablets etc.) upload and
download data.**

We use primarily SQLAlchemy Core here (in contrast to the ORM used elsewhere).

This code is optimized to a degree for speed over clarity, aiming primarily to
reduce the number of database hits.

**The overall upload method is as follows**

Everything that follows refers to records relating to a specific client device
in the "current" era, only.

In the preamble, the client:

- verifies authorization via :func:`op_check_device_registered` and
  :func:`op_check_upload_user_and_device`;
- fetches and checks server ID information via :func:`op_get_id_info`;
- checks its patients are acceptable via :func:`op_validate_patients`;
- checks which tables are permitted via :func:`op_get_allowed_tables`;
- performs some internal validity checks.

Then, in the usual stepwise upload:

- :func:`op_start_upload`

  - Rolls back any previous incomplete changes via :func:`rollback_all`.
  - Creates an upload batch, via :func:`get_batch_details_start_if_needed`.

- If were are in a preserving/finalizing upload: :func:`op_start_preservation`.

  - Marks all tables as dirty.
  - Marks the upload batch as a "preserving" batch.

- Then call some or all of:

  - For tables that are empty on the client, :func:`op_upload_empty_tables`.

    - Current client records are marked as ``_removal_pending``.
    - Any table that had previous client records is marked as dirty.
    - If preserving, any table without current records is marked as clean.

  - For tables that the client wishes to send in one go,
    :func:`op_upload_table`.

    - Find current server records.
    - Use :func:`upload_record_core` to add new records and modify existing
      ones, and :func:`flag_deleted` to delete ones that weren't on the client.
    - If any records are new, modified, or deleted, mark the table as dirty.
    - If preserving and there were no server records in this table, mark the
      table as clean.

  - For tables (e.g. BLOBs) that might be too big to send in one go:

    - client sends PKs to :func:`op_delete_where_key_not`, which "deletes" all
      other records, via :func:`flag_deleted_where_clientpk_not`.
    - client sends PK and timestamp values to :func:`op_which_keys_to_send`
    - server "deletes" records that are not in the list (via
      :func:`flag_deleted_where_clientpk_not`, which marks the table as dirty
      if any records were thus modified). Note REDUNDANCY here re
      :func:`op_delete_where_key_not`.
    - server tells the client which records are new or need to be updated
    - client sends each of those via :func:`op_upload_record`

      - Calls :func`upload_record_core`.
      - Marks the table as dirty, unless the client erroneously sent an
        unchanged record.

- In addition, specific records can be marked as ``_move_off_tablet``.

  - :func:`upload_record_core` checks this for otherwise "identical" records
    and applies that flag to the server.

- When the client's finished, it calls :func:`op_end_upload`.

  - Calls :func:`commit_all`;
  - ... which, for all dirty tables, calls :func:`commit_table`;
  - ... which executes the "add", "remove", and "preserve" functions for the
    table;
  - ... and triggers the updating of special server indexes on patient ID
    numbers and tasks, via :func:`update_indexes`.
  - At the end, :func:`commit_all` clears the dirty-table list.

There's a little bit of special code to handle old tablet software, too.

As of v2.3.0, the function :func:`op_upload_entire_database` does this in one
step (faster; for use if the network packets are not excessively large).

- Code relating to this uses ``batchdetails.onestep``.

**Setup for the upload code**

- Fire up a CamCOPS client with an empty database, e.g. from the build
  directory via

  .. code-block:: bash

    ./camcops --dbdir ~/tmp/camcops_client_test

- Fire up a web browser showing both (a) the task list via the index, and (b)
  the task list without using the index. We'll use this to verify correct
  indexing. **The two versions of the view should never be different.**

- Ensure the test client device has no current records (force-finalize if
  required).

- Ensure the server's index is proper. Run ``camcops_server reindex`` if
  required.

- If required, fire up MySQL with the server database. You may wish to use
  ``pager less -SFX``, for better display of large tables.

**Testing the upload code**

Perform the following steps both (1) with the client forced to the stepwise
upload method, and (2) with it forced to one-step upload.

Note that the number of patient ID numbers uploaded (etc.) is ignored below.

*Single record*

[Checked for one-step and multi-step upload, 2018-11-21.]

#.  Create a blank ReferrerSatisfactionSurvey (table ``ref_satis_gen``).
    This has the advantage of being an anonymous single-record task.

#.  Upload/copy.

    - The server log should show 1 × ref_satis_gen added.

    - The task lists should show the task as current and incomplete.

#.  Modify it, so it's complete.

#.  Upload/copy.

    - The server log should show 1 × ref_satis_gen added, 1 × ref_satis_gen
      modified out.

    - The task lists should show the task as current and complete.

#.  Upload/move.

    - The server log should show 2 × ref_satis_gen preserved.

    - The task lists should show the task as no longer current.

#.  Create another blank one.

#.  Upload/copy.

#.  Modify it so it's complete.

#.  Specifically flag it for preservation (the chequered flags).

#.  Upload/copy.

    - The server log should show 1 × ref_satis_gen added, 1 × ref_satis_gen
      modified out, 2 × ref_satis_gen preserved.

    - The task lists should show the task as complete and no longer current.

*With a patient*

[Checked for one-step and multi-step upload, 2018-11-21.]

#.  Create a dummy patient that the server will accept.

#.  Create a Progress Note with location "loc1" and abort its creation, giving
    an incomplete task.

#.  Create a second Progress Note with location "loc2" and contents "note2".

#.  Create a third Progress Note with location "loc3" and contents "note3".

#.  Upload/copy. Verify. This checks *addition*.

    - The server log should show 1 × patient added; 3 × progressnote added.
      (Also however many patientidnum records you chose.)
    - All three tasks should be "current".
    - The first should be "incomplete".

#.  Modify the first note by adding contents "note1".

#.  Delete the second note.

#.  Upload/copy. Verify. This checks *modification*, *deletion*,
    *no-change detection*, and *reindexing*.

    - The server log should show 1 × progressnote added, 1 × progressnote
      modified out, 1 × progressnote deleted.
    - The first note should now appear as complete.
    - The second should have vanished.
    - The third should be unchanged.
    - The two remaining tasks should still be "current".

#.  Delete the contents from the first note again.

#.  Upload/move (or move-keeping-patients; that's only different on the
    client side). Verify. This checks *preservation (finalizing)* and
    *reindexing*.

    - The server log should show 1 × patient preserved; 1 × progressnote added,
      1 × progressnote modified out, 5 × progressnote preserved.
    - The two remaining tasks should no longer be "current".
    - The first should no longer be "complete".

#.  Create a complete "note 4" and an incomplete "note 5".

#.  Upload/copy.

#.  Force-finalize from the server. This tests force-finalizing including
    reindexing.

    - The "tasks to finalize" list should have just two tasks in it.
    - After force-finalizing, the tasks should remain in the index but no
      longer be marked as current.

#.  Upload/move to get rid of the residual tasks on the client.

    - The server log should show 1 × patient added, 1 × patient preserved; 2 ×
      progressnote added, 2 × progressnote preserved.

*With ancillary tables and BLOBs*

[Checked for one-step and multi-step upload, 2018-11-21.]

#.  Create a PhotoSequence with text "t1", one photo named "p1" of you holding
    up one finger vertically, and another photo named "p2" of you holding up
    two fingers vertically.

#.  Upload/copy.

    - The server log should show:

      - blobs: 2 × added;
      - patient: 1 × added;
      - photosequence: 1 × added;
      - photosequence_photos: 2 × added.

    - The task lists should look sensible.

#.  Clear the second photo and replace it with a photo of you holding up
    two fingers horizontally.

#.  Upload/copy.

    - The server log should show:

      - blobs: 1 × added, 1 × modified out;
      - photosequence: 1 × added, 1 × modified out;
      - photosequence_photos: 1 × added, 1 × modified out.

    - The task lists should look sensible.

#.  Back to two fingers vertically. (This is the fourth photo overall.)

#.  Mark that patient for specific finalization.

#.  Upload/copy.

    - The server log should show:

      - blobs: 1 × added, 1 × modified out, 4 × preserved;
      - patient: 1 × preserved;
      - photosequence: 1 × added, 1 × modified out, 3 × preserved;
      - photosequence_photos: 1 × added, 1 × modified out, 4 × preserved.

    - The tasks should no longer be current.
    - A fresh "vertical fingers" photo should be visible.

#.  Create another patient and another PhotoSequence with one photo of three
    fingers.

#.  Upload-copy.

#.  Force-finalize.

    - Should finalize: 1 × blobs, 1 × patient, 1 × photosequence, 1 ×
      photosequence_photos.

#.  Upload/move.

During any MySQL debugging, remember:

.. code-block:: none

    -- For better display:
    pager less -SFX;

    -- To view relevant parts of the BLOB table without the actual BLOB:

    SELECT
        _pk, _group_id, _device_id, _era,
        _current, _predecessor_pk, _successor_pk,
        _addition_pending, _when_added_batch_utc, _adding_user_id,
        _removal_pending, _when_removed_batch_utc, _removing_user_id,
        _move_off_tablet,
        _preserving_user_id, _forcibly_preserved,
        id, tablename, tablepk, fieldname, mimetype, when_last_modified
    FROM blobs;

"""

# =============================================================================
# Imports
# =============================================================================

import logging
import json
# from pprint import pformat
import secrets
import string
import time
from typing import (
    Any,
    Dict,
    Iterable,
    List,
    Optional,
    Sequence,
    Set,
    Tuple,
    TYPE_CHECKING
)
from cardinal_pythonlib.datetimefunc import (
    coerce_to_pendulum,
    coerce_to_pendulum_date,
    format_datetime,
)
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
)
from cardinal_pythonlib.pyramid.responses import TextResponse
from cardinal_pythonlib.sqlalchemy.core_query import (
    exists_in_table,
    fetch_all_first_values,
)
from cardinal_pythonlib.text import escape_newlines
from pyramid.httpexceptions import HTTPBadRequest
from pyramid.view import view_config
from pyramid.response import Response
from pyramid.security import NO_PERMISSION_REQUIRED
from semantic_version import Version
from sqlalchemy.engine.result import ResultProxy
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import joinedload
from sqlalchemy.sql.expression import exists, select, update
from sqlalchemy.sql.schema import Table

from camcops_server.cc_modules import cc_audit  # avoids "audit" name clash
from camcops_server.cc_modules.cc_all_models import (
    CLIENT_TABLE_MAP,
    RESERVED_FIELDS,
)
from camcops_server.cc_modules.cc_blob import Blob
from camcops_server.cc_modules.cc_cache import cache_region_static, fkg
from camcops_server.cc_modules.cc_client_api_core import (
    AllowedTablesFieldNames,
    BatchDetails,
    exception_description,
    ExtraStringFieldNames,
    fail_server_error,
    fail_unsupported_operation,
    fail_user_error,
    get_server_live_records,
    IgnoringAntiqueTableException,
    require_keys,
    ServerErrorException,
    ServerRecord,
    TabletParam,
    UploadRecordResult,
    UploadTableChanges,
    UserErrorException,
    values_delete_later,
    values_delete_now,
    values_preserve_now,
    WhichKeyToSendInfo,
)
from camcops_server.cc_modules.cc_client_api_helpers import (
    upload_commit_order_sorter,
)
from camcops_server.cc_modules.cc_constants import (
    CLIENT_DATE_FIELD,
    DateFormat,
    ERA_NOW,
    FP_ID_NUM,
    FP_ID_DESC,
    FP_ID_SHORT_DESC,
    MOVE_OFF_TABLET_FIELD,
    NUMBER_OF_IDNUMS_DEFUNCT,  # allowed; for old tablet versions
    POSSIBLE_SEX_VALUES,
    TABLET_ID_FIELD,
)
from camcops_server.cc_modules.cc_convert import (
    decode_single_value,
    decode_values,
    encode_single_value,
)
from camcops_server.cc_modules.cc_db import (
    FN_ADDING_USER_ID,
    FN_ADDITION_PENDING,
    FN_CAMCOPS_VERSION,
    FN_CURRENT,
    FN_DEVICE_ID,
    FN_ERA,
    FN_GROUP_ID,
    FN_PK,
    FN_PREDECESSOR_PK,
    FN_REMOVAL_PENDING,
    FN_REMOVING_USER_ID,
    FN_SUCCESSOR_PK,
    FN_WHEN_ADDED_BATCH_UTC,
    FN_WHEN_ADDED_EXACT,
    FN_WHEN_REMOVED_BATCH_UTC,
    FN_WHEN_REMOVED_EXACT,
)
from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_dirtytables import DirtyTable
from camcops_server.cc_modules.cc_group import Group
from camcops_server.cc_modules.cc_ipuse import IpUse
from camcops_server.cc_modules.cc_membership import UserGroupMembership
from camcops_server.cc_modules.cc_patient import (
    Patient,
    is_candidate_patient_valid_for_group,
    is_candidate_patient_valid_for_restricted_user,
)
from camcops_server.cc_modules.cc_patientidnum import (
    fake_tablet_id_for_patientidnum,
    PatientIdNum,
)
from camcops_server.cc_modules.cc_proquint import (
    InvalidProquintException,
    uuid_from_proquint,
)
from camcops_server.cc_modules.cc_pyramid import Routes
from camcops_server.cc_modules.cc_simpleobjects import (
    BarePatientInfo,
    IdNumReference,
)
from camcops_server.cc_modules.cc_specialnote import SpecialNote
from camcops_server.cc_modules.cc_task import (
    all_task_tables_with_min_client_version,
)
from camcops_server.cc_modules.cc_taskindex import update_indexes_and_push_exports  # noqa
from camcops_server.cc_modules.cc_user import User
from camcops_server.cc_modules.cc_validators import (
    STRING_VALIDATOR_TYPE,
    validate_anything,
    validate_email,
)
from camcops_server.cc_modules.cc_version import (
    CAMCOPS_SERVER_VERSION_STRING,
    MINIMUM_TABLET_VERSION,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

COPE_WITH_DELETED_PATIENT_DESCRIPTIONS = True
# ... as of client 2.0.0, ID descriptions are no longer duplicated.
# As of server 2.0.0, the fields still exist in the database, but the reporting
# and consistency check has been removed. In the next version of the server,
# the fields will be removed, and then the server should cope with old clients,
# at least for a while.

DUPLICATE_FAILED = "Failed to duplicate record"
INSERT_FAILED = "Failed to insert record"

# REGEX_INVALID_TABLE_FIELD_CHARS = re.compile("[^a-zA-Z0-9_]")
# ... the ^ within the [] means the expression will match any character NOT in
# the specified range

DEVICE_STORED_VAR_TABLENAME_DEFUNCT = "storedvars"
# ... old table, no longer in use, that Titanium clients used to upload.
# We recognize and ignore it now so that old clients can still work.

SILENTLY_IGNORE_TABLENAMES = [DEVICE_STORED_VAR_TABLENAME_DEFUNCT]

IGNORING_ANTIQUE_TABLE_MESSAGE = (
    "Ignoring user request to upload antique/defunct table, but reporting "
    "success to the client"
)

SUCCESS_MSG = "Success"
SUCCESS_CODE = "1"
FAILURE_CODE = "0"

DEBUG_UPLOAD = False


# =============================================================================
# Quasi-constants
# =============================================================================

DB_JSON_DECODER = json.JSONDecoder()  # just a plain one
PATIENT_INFO_JSON_DECODER = json.JSONDecoder()  # just a plain one


# =============================================================================
# Cached information
# =============================================================================

@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def all_tables_with_min_client_version() -> Dict[str, Version]:
    """
    For all tables that the client might upload to, return a mapping from the
    table name to the corresponding minimum client version.
    """
    d = all_task_tables_with_min_client_version()
    d[Blob.__tablename__] = MINIMUM_TABLET_VERSION
    d[Patient.__tablename__] = MINIMUM_TABLET_VERSION
    d[PatientIdNum.__tablename__] = MINIMUM_TABLET_VERSION
    return d


# =============================================================================
# Validators
# =============================================================================

def ensure_valid_table_name(req: "CamcopsRequest", tablename: str) -> None:
    """
    Ensures a table name:

    - doesn't contain bad characters,
    - isn't a reserved table that the user is prohibited from accessing, and
    - is a valid table name that's in the database.

    Raises :exc:`UserErrorException` upon failure.

    - 2017-10-08: shortcut to all that: it's OK if it's listed as a valid
      client table.
    - 2018-01-16 (v2.2.0): check also that client version is OK
    """
    if tablename not in CLIENT_TABLE_MAP:
        fail_user_error(f"Invalid client table name: {tablename}")
    tables_versions = all_tables_with_min_client_version()
    assert tablename in tables_versions
    client_version = req.tabletsession.tablet_version_ver
    minimum_client_version = tables_versions[tablename]
    if client_version < minimum_client_version:
        fail_user_error(
            f"Client CamCOPS version {client_version} is less than the "
            f"version ({minimum_client_version}) "
            f"required to handle table {tablename}"
        )


def ensure_valid_field_name(table: Table, fieldname: str) -> None:
    """
    Ensures a field name contains only valid characters, and isn't a
    reserved fieldname that the user isn't allowed to access.

    Raises :exc:`UserErrorException` upon failure.

    - 2017-10-08: shortcut: it's OK if it's a column name for a particular
      table.
    """
    # if fieldname in RESERVED_FIELDS:
    if fieldname.startswith("_"):  # all reserved fields start with _
        # ... but not all fields starting with "_" are reserved; e.g.
        # "_move_off_tablet" is allowed.
        if fieldname in RESERVED_FIELDS:
            fail_user_error(
                f"Reserved field name for table {table.name!r}: {fieldname!r}")
    if fieldname not in table.columns.keys():
        fail_user_error(
            f"Invalid field name for table {table.name!r}: {fieldname!r}")
    # Note that the reserved-field check is case-sensitive, but so is the
    # "present in table" check. So for a malicious uploader trying to use, for
    # example, "_PK", this would not be picked up as a reserved field (so would
    # pass that check) but then wouldn't be recognized as a valid field (so
    # would fail).


def ensure_string(value: Any, allow_none: bool = True) -> None:
    """
    Used when processing JSON information about patients: ensures that a value
    is a string, or raises.

    Args:
        value: value to test
        allow_none: is ``None`` allowed (not just an empty string)?
    """
    if value is None:
        if allow_none:
            return  # OK
        else:
            fail_user_error("Patient JSON contains absent string")
    if not isinstance(value, str):
        fail_user_error(f"Patient JSON contains invalid non-string: {value!r}")


def ensure_valid_patient_json(req: "CamcopsRequest",
                              group: Group,
                              pt_dict: Dict[str, Any]) -> None:
    """
    Ensures that the JSON dictionary contains valid patient details (valid for
    the group into which it's being uploaded), and that (if applicable) this
    user is allowed to upload this patient.

    Args:
        req:
            the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        group:
            the :class:`camcops_server.cc_modules.cc_group.Group` into which
            the upload is going
        pt_dict:
            a JSON dictionary from the client

    Raises:
        :exc:`UserErrorException` if invalid

    """
    if not isinstance(pt_dict, dict):
        fail_user_error("Patient JSON is not a dict")
    if not pt_dict:
        fail_user_error("Patient JSON is empty")
    valid_which_idnums = req.valid_which_idnums
    errors = []  # type: List[str]
    finalizing = None
    ptinfo = BarePatientInfo()
    idnum_types_seen = set()  # type: Set[int]
    for k, v in pt_dict.items():
        ensure_string(k, allow_none=False)

        if k == TabletParam.FORENAME:
            ensure_string(v)
            ptinfo.forename = v

        elif k == TabletParam.SURNAME:
            ensure_string(v)
            ptinfo.surname = v

        elif k == TabletParam.SEX:
            if v not in POSSIBLE_SEX_VALUES:
                fail_user_error(f"Bad sex value: {v!r}")
            ptinfo.sex = v

        elif k == TabletParam.DOB:
            ensure_string(v)
            if v:
                dob = coerce_to_pendulum_date(v)
                if dob is None:
                    fail_user_error(f"Invalid DOB: {v!r}")
            else:
                dob = None
            ptinfo.dob = dob

        elif k == TabletParam.EMAIL:
            ensure_string(v)
            if v:
                try:
                    validate_email(v)
                except ValueError:
                    fail_user_error(f"Bad e-mail address: {v!r}")
            ptinfo.email = v

        elif k == TabletParam.ADDRESS:
            ensure_string(v)
            ptinfo.address = v

        elif k == TabletParam.GP:
            ensure_string(v)
            ptinfo.gp = v

        elif k == TabletParam.OTHER:
            ensure_string(v)
            ptinfo.otherdetails = v

        elif k.startswith(TabletParam.IDNUM_PREFIX):
            nstr = k[len(TabletParam.IDNUM_PREFIX):]
            try:
                which_idnum = int(nstr)
            except (TypeError, ValueError):
                fail_user_error(f"Bad idnum key: {k!r}")
            # noinspection PyUnboundLocalVariable
            if which_idnum not in valid_which_idnums:
                fail_user_error(f"Bad ID number type: {which_idnum}")
            if which_idnum in idnum_types_seen:
                fail_user_error(f"More than one ID number supplied for ID "
                                f"number type {which_idnum}")
            idnum_types_seen.add(which_idnum)
            if v is not None and not isinstance(v, int):
                fail_user_error(f"Bad ID number value: {v!r}")
            idref = IdNumReference(which_idnum, v)
            if not idref.is_valid():
                fail_user_error(f"Bad ID number: {idref!r}")
            ptinfo.add_idnum(idref)

        elif k == TabletParam.FINALIZING:
            if not isinstance(v, bool):
                fail_user_error(f"Bad {k!r} value: {v!r}")
            finalizing = v

        else:
            fail_user_error(f"Unknown JSON key: {k!r}")

    if finalizing is None:
        fail_user_error(f"Missing {TabletParam.FINALIZING!r} JSON key")

    pt_ok, reason = is_candidate_patient_valid_for_group(
        ptinfo, group, finalizing)
    if not pt_ok:
        errors.append(f"{ptinfo} -> {reason}")
    pt_ok, reason = is_candidate_patient_valid_for_restricted_user(
        req, ptinfo)
    if not pt_ok:
        errors.append(f"{ptinfo} -> {reason}")
    if errors:
        fail_user_error(f"Invalid patient: {' // '.join(errors)}")


# =============================================================================
# Extracting information from the POST request
# =============================================================================

def get_str_var(
        req: "CamcopsRequest",
        var: str,
        mandatory: bool = True,
        validator: STRING_VALIDATOR_TYPE = validate_anything) \
        -> Optional[str]:
    """
    Retrieves a string variable from the CamcopsRequest.

    By default this performs no validation (because, for example, these strings
    can contain SQL-encoded data or JSON), but there are a number of subsequent
    operation-specific validation steps.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        var: name of variable to retrieve
        mandatory: if ``True``, raise an exception if the variable is missing
        validator: validator function to use

    Returns:
        value

    Raises:
        :exc:`UserErrorException` if the variable was mandatory and
        no value was provided
    """
    try:
        val = req.get_str_param(var, default=None, validator=validator)
        if mandatory and val is None:
            fail_user_error(f"Must provide the variable: {var}")
        return val
    except HTTPBadRequest as e:  # failed the validator
        fail_user_error(str(e))


def get_int_var(req: "CamcopsRequest", var: str) -> int:
    """
    Retrieves an integer variable from the CamcopsRequest.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        var: name of variable to retrieve

    Returns:
        value

    Raises:
        :exc:`UserErrorException` if no value was provided, or if it wasn't an
        integer
    """
    s = get_str_var(req, var, mandatory=True)
    try:
        return int(s)
    except (TypeError, ValueError):
        fail_user_error(f"Variable {var} is not a valid integer; was {s!r}")


def get_bool_int_var(req: "CamcopsRequest", var: str) -> bool:
    """
    Retrieves a Boolean variable (encoded as an integer) from the
    CamcopsRequest. Zero represents false; nonzero represents true.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        var: name of variable to retrieve

    Returns:
        value

    Raises:
        :exc:`UserErrorException` if no value was provided, or if it wasn't an
        integer
    """
    num = get_int_var(req, var)
    return bool(num)


def get_table_from_req(req: "CamcopsRequest", var: str) -> Table:
    """
    Retrieves a table name from a HTTP request, checks it's a valid client
    table, and returns that table.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        var: variable name (the variable's should be the table name)

    Returns:
        a SQLAlchemy :class:`Table`

    Raises:
        :exc:`UserErrorException` if the variable wasn't provided

        :exc:`IgnoringAntiqueTableException` if the table is one to
        ignore quietly (requested by an antique client)
    """
    tablename = get_str_var(req, var, mandatory=True)
    if tablename in SILENTLY_IGNORE_TABLENAMES:
        raise IgnoringAntiqueTableException(f"Ignoring table {tablename}")
    ensure_valid_table_name(req, tablename)
    return CLIENT_TABLE_MAP[tablename]


def get_tables_from_post_var(req: "CamcopsRequest",
                             var: str,
                             mandatory: bool = True) -> List[Table]:
    """
    Gets a list of tables from an HTTP request variable, and ensures all are
    valid.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        var: name of variable to retrieve
        mandatory: if ``True``, raise an exception if the variable is missing

    Returns:
        a list of SQLAlchemy :class:`Table` objects

    Raises:
        :exc:`UserErrorException` if the variable was mandatory and
        no value was provided, or if one or more tables was not valid
    """
    cstables = get_str_var(req, var, mandatory=mandatory)
    if not cstables:
        return []
    # can't have any commas in table names, so it's OK to use a simple
    # split() command
    tablenames = [x.strip() for x in cstables.split(",")]
    tables = []  # type: List[Table]
    for tn in tablenames:
        if tn in SILENTLY_IGNORE_TABLENAMES:
            log.warning(IGNORING_ANTIQUE_TABLE_MESSAGE)
            continue
        ensure_valid_table_name(req, tn)
        tables.append(CLIENT_TABLE_MAP[tn])
    return tables


def get_single_field_from_post_var(req: "CamcopsRequest",
                                   table: Table,
                                   var: str,
                                   mandatory: bool = True) -> str:
    """
    Retrieves a field (column) name from a the request and checks it's not a
    bad fieldname for the specified table.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        table: SQLAlchemy :class:`Table` in which the column should exist
        var: name of variable to retrieve
        mandatory: if ``True``, raise an exception if the variable is missing

    Returns:
        the field (column) name

    Raises:
        :exc:`UserErrorException` if the variable was mandatory and
        no value was provided, or if the field was not valid for the specified
        table
    """
    field = get_str_var(req, var, mandatory=mandatory)
    ensure_valid_field_name(table, field)
    return field


def get_fields_from_post_var(
        req: "CamcopsRequest",
        table: Table,
        var: str,
        mandatory: bool = True,
        allowed_nonexistent_fields: List[str] = None) -> List[str]:
    """
    Get a comma-separated list of field names from a request and checks that
    all are acceptable. Returns a list of fieldnames.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        table: SQLAlchemy :class:`Table` in which the columns should exist
        var: name of variable to retrieve
        mandatory: if ``True``, raise an exception if the variable is missing
        allowed_nonexistent_fields: fields that are allowed to be in the
            upload but not in the database (special exemptions!)

    Returns:
        a list of the field (column) names

    Raises:
        :exc:`UserErrorException` if the variable was mandatory and
        no value was provided, or if any field was not valid for the specified
        table
    """
    csfields = get_str_var(req, var, mandatory=mandatory)
    if not csfields:
        return []
    allowed_nonexistent_fields = allowed_nonexistent_fields or []  # type: List[str]  # noqa
    # can't have any commas in fields, so it's OK to use a simple
    # split() command
    fields = [x.strip() for x in csfields.split(",")]
    for f in fields:
        if f in allowed_nonexistent_fields:
            continue
        ensure_valid_field_name(table, f)
    return fields


def get_values_from_post_var(req: "CamcopsRequest",
                             var: str,
                             mandatory: bool = True) -> List[Any]:
    """
    Retrieves a list of values from a CSV-separated list of SQL values
    stored in a CGI form (including e.g. NULL, numbers, quoted strings, and
    special handling for base-64/hex-encoded BLOBs.)

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        var: name of variable to retrieve
        mandatory: if ``True``, raise an exception if the variable is missing
    """
    csvalues = get_str_var(req, var, mandatory=mandatory)
    if not csvalues:
        return []
    return decode_values(csvalues)


def get_fields_and_values(req: "CamcopsRequest",
                          table: Table,
                          fields_var: str,
                          values_var: str,
                          mandatory: bool = True) -> Dict[str, Any]:
    """
    Gets fieldnames and matching values from two variables in a request.

    See :func:`get_fields_from_post_var`, :func:`get_values_from_post_var`.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        table: SQLAlchemy :class:`Table` in which the columns should exist
        fields_var: name of CSV "column names" variable to retrieve
        values_var: name of CSV "corresponding values" variable to retrieve
        mandatory: if ``True``, raise an exception if the variable is missing

    Returns:
        a dictionary mapping column names to decoded values

    Raises:
        :exc:`UserErrorException` if the variable was mandatory and
        no value was provided, or if any field was not valid for the specified
        table
    """
    fields = get_fields_from_post_var(req, table, fields_var,
                                      mandatory=mandatory)
    values = get_values_from_post_var(req, values_var, mandatory=mandatory)
    if len(fields) != len(values):
        fail_user_error(
            f"Number of fields ({len(fields)}) doesn't match number of values "
            f"({len(values)})"
        )
    return dict(list(zip(fields, values)))


def get_json_from_post_var(req: "CamcopsRequest", key: str,
                           decoder: json.JSONDecoder = None,
                           mandatory: bool = True) -> Any:
    """
    Returns a Python object from a JSON-encoded value.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        key: the name of the variable to retrieve
        decoder: the JSON decoder object to use; if ``None``, a default is
            created
        mandatory: if ``True``, raise an exception if the variable is missing

    Returns:
        Python object, e.g. a list of values, or ``None`` if the object is
        invalid and not mandatory

    Raises:
        :exc:`UserErrorException` if the variable was mandatory and
        no value was provided or the value was invalid JSON
    """
    decoder = decoder or json.JSONDecoder()
    j = get_str_var(req, key, mandatory=mandatory)  # may raise
    if not j:  # missing but not mandatory
        return None
    try:
        return decoder.decode(j)
    except json.JSONDecodeError:
        msg = f"Bad JSON for key {key!r}"
        if mandatory:
            fail_user_error(msg)
        else:
            log.warning(msg)
            return None


# =============================================================================
# Sending stuff to the client
# =============================================================================

def get_server_id_info(req: "CamcopsRequest") -> Dict[str, str]:
    """
    Returns a reply for the tablet, as a variable-to-value dictionary, giving
    details of the server.
    """
    group = Group.get_group_by_id(req.dbsession, req.user.upload_group_id)
    reply = {
        TabletParam.DATABASE_TITLE: req.database_title,
        TabletParam.ID_POLICY_UPLOAD: group.upload_policy or "",
        TabletParam.ID_POLICY_FINALIZE: group.finalize_policy or "",
        TabletParam.SERVER_CAMCOPS_VERSION: CAMCOPS_SERVER_VERSION_STRING,
    }
    for iddef in req.idnum_definitions:
        n = iddef.which_idnum
        nstr = str(n)
        reply[TabletParam.ID_DESCRIPTION_PREFIX + nstr] = \
            iddef.description or ""
        reply[TabletParam.ID_SHORT_DESCRIPTION_PREFIX + nstr] = \
            iddef.short_description or ""
        reply[TabletParam.ID_VALIDATION_METHOD_PREFIX + nstr] = \
            iddef.validation_method or ""
    return reply


def get_select_reply(fields: Sequence[str],
                     rows: Sequence[Sequence[Any]]) -> Dict[str, str]:
    """
    Formats the result of a ``SELECT`` query for the client as a dictionary
    reply.

    Args:
        fields: list of field names
        rows: list of rows, where each row is a list of values in the same
            order as ``fields``

    Returns:

        a dictionary of the format:

        .. code-block:: none

            {
                "nfields": NUMBER_OF_FIELDS,
                "fields": FIELDNAMES_AS_CSV,
                "nrecords": NUMBER_OF_RECORDS,
                "record0": VALUES_AS_CSV_LIST_OF_ENCODED_SQL_VALUES,
                    ...
                "record{nrecords - 1}": VALUES_AS_CSV_LIST_OF_ENCODED_SQL_VALUES
            }

    The final reply to the server is then formatted as text as per
    :func:`client_api`.

    """  # noqa
    nrecords = len(rows)
    reply = {
        TabletParam.NFIELDS: len(fields),
        TabletParam.FIELDS: ",".join(fields),
        TabletParam.NRECORDS: nrecords,
    }
    for r in range(nrecords):
        row = rows[r]
        encodedvalues = []  # type: List[str]
        for val in row:
            encodedvalues.append(encode_single_value(val))
        reply[TabletParam.RECORD_PREFIX + str(r)] = ",".join(encodedvalues)
    return reply


# =============================================================================
# CamCOPS table reading functions
# =============================================================================

def record_exists(req: "CamcopsRequest",
                  table: Table,
                  clientpk_name: str,
                  clientpk_value: Any) -> ServerRecord:
    """
    Checks if a record exists, using the device's perspective of a
    table/client PK combination.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        table: an SQLAlchemy :class:`Table`
        clientpk_name: the column name of the client's PK
        clientpk_value: the client's PK value

    Returns:
        a :class:`ServerRecord` with the required information

    """
    query = (
        select([
            table.c[FN_PK],  # server PK
            table.c[CLIENT_DATE_FIELD],  # when last modified (on the server)
            table.c[MOVE_OFF_TABLET_FIELD]  # move_off_tablet
        ])
        .where(table.c[FN_DEVICE_ID] == req.tabletsession.device_id)
        .where(table.c[FN_CURRENT])
        .where(table.c[FN_ERA] == ERA_NOW)
        .where(table.c[clientpk_name] == clientpk_value)
    )
    row = req.dbsession.execute(query).fetchone()
    if not row:
        return ServerRecord(clientpk_value, False)
    server_pk, server_when, move_off_tablet = row
    return ServerRecord(clientpk_value, True, server_pk, server_when,
                        move_off_tablet)
    # Consider a warning/failure if we have >1 row meeting these criteria.
    # Not currently checked for.


def client_pks_that_exist(req: "CamcopsRequest",
                          table: Table,
                          clientpk_name: str,
                          clientpk_values: List[int]) \
        -> Dict[int, ServerRecord]:
    """
    Searches for client PK values (for this device, current, and 'now')
    matching the input list.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        table: an SQLAlchemy :class:`Table`
        clientpk_name: the column name of the client's PK
        clientpk_values: a list of the client's PK values

    Returns:
        a dictionary mapping client_pk to a :class:`ServerRecord` objects, for
        those records that match
    """
    query = (
        select([
            table.c[FN_PK],  # server PK
            table.c[clientpk_name],  # client PK
            table.c[CLIENT_DATE_FIELD],  # when last modified (on the server)
            table.c[MOVE_OFF_TABLET_FIELD]  # move_off_tablet
        ])
        .where(table.c[FN_DEVICE_ID] == req.tabletsession.device_id)
        .where(table.c[FN_CURRENT])
        .where(table.c[FN_ERA] == ERA_NOW)
        .where(table.c[clientpk_name].in_(clientpk_values))
    )
    rows = req.dbsession.execute(query)
    d = {}  # type: Dict[int, ServerRecord]
    for server_pk, client_pk, server_when, move_off_tablet in rows:
        d[client_pk] = ServerRecord(client_pk, True, server_pk, server_when,
                                    move_off_tablet)
    return d


def get_all_predecessor_pks(req: "CamcopsRequest",
                            table: Table,
                            last_pk: int,
                            include_last: bool = True) -> List[int]:
    """
    Retrieves the PKs of all records that are predecessors of the specified one

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        table: an SQLAlchemy :class:`Table`
        last_pk: the PK to start with, and work backwards
        include_last: include ``last_pk`` in the list

    Returns:
        the PKs

    """
    dbsession = req.dbsession
    pks = []  # type: List[int]
    if include_last:
        pks.append(last_pk)
    current_pk = last_pk
    finished = False
    while not finished:
        next_pk = dbsession.execute(
            select([table.c[FN_PREDECESSOR_PK]])
            .where(table.c[FN_PK] == current_pk)
        ).scalar()  # type: Optional[int]
        if next_pk is None:
            finished = True
        else:
            pks.append(next_pk)
            current_pk = next_pk
    return sorted(pks)


# =============================================================================
# Record modification functions
# =============================================================================

def flag_deleted(req: "CamcopsRequest",
                 batchdetails: BatchDetails,
                 table: Table,
                 pklist: Iterable[int]) -> None:
    """
    Marks record(s) as deleted, specified by a list of server PKs within a
    table. (Note: "deleted" means "deleted with no successor", not "modified
    and replaced by a successor record".)
    """
    if batchdetails.onestep:
        values = values_delete_now(req, batchdetails)
    else:
        values = values_delete_later()
    req.dbsession.execute(
        update(table)
        .where(table.c[FN_PK].in_(pklist))
        .values(values)
    )


def flag_all_records_deleted(req: "CamcopsRequest",
                             table: Table) -> int:
    """
    Marks all records in a table as deleted (that are current and in the
    current era).

    Returns the number of rows affected.
    """
    rp = req.dbsession.execute(
        update(table)
        .where(table.c[FN_DEVICE_ID] == req.tabletsession.device_id)
        .where(table.c[FN_CURRENT])
        .where(table.c[FN_ERA] == ERA_NOW)
        .values(values_delete_later())
    )  # type: ResultProxy
    return rp.rowcount
    # https://docs.sqlalchemy.org/en/latest/core/connections.html?highlight=rowcount#sqlalchemy.engine.ResultProxy.rowcount  # noqa


def flag_deleted_where_clientpk_not(req: "CamcopsRequest",
                                    table: Table,
                                    clientpk_name: str,
                                    clientpk_values: Sequence[Any]) -> None:
    """
    Marks for deletion all current/current-era records for a device, within a
    specific table, defined by a list of client-side PK values (and the name of
    the client-side PK column).
    """
    rp = req.dbsession.execute(
        update(table)
        .where(table.c[FN_DEVICE_ID] == req.tabletsession.device_id)
        .where(table.c[FN_CURRENT])
        .where(table.c[FN_ERA] == ERA_NOW)
        .where(table.c[clientpk_name].notin_(clientpk_values))
        .values(values_delete_later())
    )  # type: ResultProxy
    if rp.rowcount > 0:
        mark_table_dirty(req, table)
    # ... but if we are preserving, do NOT mark this table as clean; there may
    # be other records that still require preserving.


def flag_modified(req: "CamcopsRequest",
                  batchdetails: BatchDetails,
                  table: Table,
                  pk: int,
                  successor_pk: int) -> None:
    """
    Marks a record as old, storing its successor's details.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: SQLAlchemy :class:`Table`
        pk: server PK of the record to mark as old
        successor_pk: server PK of its successor
    """
    if batchdetails.onestep:
        req.dbsession.execute(
            update(table)
            .where(table.c[FN_PK] == pk)
            .values({
                FN_CURRENT: 0,
                FN_REMOVAL_PENDING: 0,
                FN_SUCCESSOR_PK: successor_pk,
                FN_REMOVING_USER_ID: req.user_id,
                FN_WHEN_REMOVED_EXACT: req.now,
                FN_WHEN_REMOVED_BATCH_UTC: batchdetails.batchtime,
            })
        )
    else:
        req.dbsession.execute(
            update(table)
            .where(table.c[FN_PK] == pk)
            .values({
                FN_REMOVAL_PENDING: 1,
                FN_SUCCESSOR_PK: successor_pk
            })
        )


def flag_multiple_records_for_preservation(
        req: "CamcopsRequest",
        batchdetails: BatchDetails,
        table: Table,
        pks_to_preserve: List[int]) -> None:
    """
    Low-level function to mark records for preservation by server PK.
    Does not concern itself with the predecessor chain (for which, see
    :func:`flag_record_for_preservation`).

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: SQLAlchemy :class:`Table`
        pks_to_preserve: server PK of the records to mark as preserved
    """
    if batchdetails.onestep:
        req.dbsession.execute(
            update(table)
            .where(table.c[FN_PK].in_(pks_to_preserve))
            .values(values_preserve_now(req, batchdetails))
        )
        # Also any associated special notes:
        new_era = batchdetails.new_era
        # noinspection PyUnresolvedReferences
        req.dbsession.execute(
            update(SpecialNote.__table__)
            .where(SpecialNote.basetable == table.name)
            .where(SpecialNote.device_id == req.tabletsession.device_id)
            .where(SpecialNote.era == ERA_NOW)
            .where(exists().select_from(table)
                   .where(table.c[TABLET_ID_FIELD] == SpecialNote.task_id)
                   .where(table.c[FN_DEVICE_ID] == SpecialNote.device_id)
                   .where(table.c[FN_ERA] == new_era))
            #             ^^^^^^^^^^^^^^^^^^^^^^^^^^
            #             This bit restricts to records being preserved.
            .values(era=new_era)
        )
    else:
        req.dbsession.execute(
            update(table)
            .where(table.c[FN_PK].in_(pks_to_preserve))
            .values({
                MOVE_OFF_TABLET_FIELD: 1
            })
        )


def flag_record_for_preservation(req: "CamcopsRequest",
                                 batchdetails: BatchDetails,
                                 table: Table,
                                 pk: int) -> List[int]:
    """
    Marks a record for preservation (moving off the tablet, changing its
    era details).

    2018-11-18: works back through the predecessor chain too, fixing an old
    bug.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: SQLAlchemy :class:`Table`
        pk: server PK of the record to mark

    Returns:
        list: all PKs being preserved
    """
    pks_to_preserve = get_all_predecessor_pks(req, table, pk)
    flag_multiple_records_for_preservation(req, batchdetails, table,
                                           pks_to_preserve)
    return pks_to_preserve


def preserve_all(req: "CamcopsRequest",
                 batchdetails: BatchDetails,
                 table: Table) -> None:
    """
    Preserves all records in a table for a device, including non-current ones.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: SQLAlchemy :class:`Table`
    """
    device_id = req.tabletsession.device_id
    req.dbsession.execute(
        update(table)
        .where(table.c[FN_DEVICE_ID] == device_id)
        .where(table.c[FN_ERA] == ERA_NOW)
        .values(values_preserve_now(req, batchdetails))
    )


# =============================================================================
# Upload helper functions
# =============================================================================

def process_upload_record_special(req: "CamcopsRequest",
                                  batchdetails: BatchDetails,
                                  table: Table,
                                  valuedict: Dict[str, Any]) -> None:
    """
    Special processing function for upload, in which we inspect the data.
    Called by :func:`upload_record_core`.

    1. Handles old clients with ID information in the patient table, etc.
       (Note: this can be IGNORED for any client using
       :func:`op_upload_entire_database`, as these are newer.)

    2. Validates ID numbers.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: an SQLAlchemy :class:`Table`
        valuedict: a dictionary of {colname: value} pairs from the client.
            May be modified.
    """
    ts = req.tabletsession
    tablename = table.name

    if tablename == Patient.__tablename__:
        # ---------------------------------------------------------------------
        # Deal with old tablets that had ID numbers in a less flexible format.
        # ---------------------------------------------------------------------
        if ts.cope_with_deleted_patient_descriptors:
            # Old tablets (pre-2.0.0) will upload copies of the ID
            # descriptions with the patient. To cope with that, we
            # remove those here:
            for n in range(1, NUMBER_OF_IDNUMS_DEFUNCT + 1):
                nstr = str(n)
                fn_desc = FP_ID_DESC + nstr
                fn_shortdesc = FP_ID_SHORT_DESC + nstr
                valuedict.pop(fn_desc, None)  # remove item, if exists
                valuedict.pop(fn_shortdesc, None)

        if ts.cope_with_old_idnums:
            # Insert records into the new ID number table from the old
            # patient table:
            for which_idnum in range(1, NUMBER_OF_IDNUMS_DEFUNCT + 1):
                nstr = str(which_idnum)
                fn_idnum = FP_ID_NUM + nstr
                idnum_value = valuedict.pop(fn_idnum, None)
                # ... and remove it from our new Patient record
                patient_id = valuedict.get("id", None)
                if idnum_value is None or patient_id is None:
                    continue
                # noinspection PyUnresolvedReferences
                mark_table_dirty(req, PatientIdNum.__table__)
                client_date_value = coerce_to_pendulum(valuedict[CLIENT_DATE_FIELD])  # noqa
                # noinspection PyUnresolvedReferences
                upload_record_core(
                    req=req,
                    batchdetails=batchdetails,
                    table=PatientIdNum.__table__,
                    clientpk_name='id',
                    valuedict={
                        'id': fake_tablet_id_for_patientidnum(
                            patient_id=patient_id,
                            which_idnum=which_idnum
                        ),  # ... guarantees a pseudo client PK
                        'patient_id': patient_id,
                        'which_idnum': which_idnum,
                        'idnum_value': idnum_value,
                        CLIENT_DATE_FIELD: client_date_value,
                        MOVE_OFF_TABLET_FIELD: valuedict[MOVE_OFF_TABLET_FIELD],  # noqa
                    }
                )
            # Now, how to deal with deletion, i.e. records missing from the
            # tablet? See our caller, op_upload_table(), which has a special
            # handler for this.
            #
            # Note that op_upload_record() is/was only used for BLOBs, so we
            # don't have to worry about special processing for that aspect
            # here; also, that method handles deletion in a different way.

    elif tablename == PatientIdNum.__tablename__:
        # ---------------------------------------------------------------------
        # Validate ID numbers.
        # ---------------------------------------------------------------------
        which_idnum = valuedict.get("which_idnum", None)
        if which_idnum not in req.valid_which_idnums:
            fail_user_error(f"No such ID number type: {which_idnum}")
        idnum_value = valuedict.get("idnum_value", None)
        if not req.is_idnum_valid(which_idnum, idnum_value):
            why_invalid = req.why_idnum_invalid(which_idnum, idnum_value)
            fail_user_error(
                f"For ID type {which_idnum}, ID number {idnum_value} is "
                f"invalid: {why_invalid}")


def upload_record_core(
        req: "CamcopsRequest",
        batchdetails: BatchDetails,
        table: Table,
        clientpk_name: str,
        valuedict: Dict[str, Any],
        server_live_current_records: List[ServerRecord] = None) \
        -> UploadRecordResult:
    """
    Uploads a record. Deals with IDENTICAL, NEW, and MODIFIED records.

    Used by :func:`upload_table` and :func:`upload_record`.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: an SQLAlchemy :class:`Table`
        clientpk_name: the column name of the client's PK
        valuedict: a dictionary of {colname: value} pairs from the client
        server_live_current_records: list of :class:`ServerRecord` objects for
            the active records on the server for this client, in this table

    Returns:
        a :class:`UploadRecordResult` object
    """
    require_keys(valuedict, [clientpk_name, CLIENT_DATE_FIELD,
                             MOVE_OFF_TABLET_FIELD])
    clientpk_value = valuedict[clientpk_name]

    if server_live_current_records:
        # All server records for this table/device/era have been prefetched.
        serverrec = next((r for r in server_live_current_records
                          if r.client_pk == clientpk_value), None)
        if serverrec is None:
            serverrec = ServerRecord(clientpk_value, False)
    else:
        # Look up this record specifically.
        serverrec = record_exists(req, table, clientpk_name, clientpk_value)

    if DEBUG_UPLOAD:
        log.debug("upload_record_core: {}, {}", table.name, serverrec)

    oldserverpk = serverrec.server_pk
    urr = UploadRecordResult(
        oldserverpk=oldserverpk,
        specifically_marked_for_preservation=bool(valuedict[MOVE_OFF_TABLET_FIELD]),  # noqa
        dirty=True
    )
    if serverrec.exists:
        # There's an existing record, which is either identical or not.
        client_date_value = coerce_to_pendulum(valuedict[CLIENT_DATE_FIELD])
        if serverrec.server_when == client_date_value:
            # The existing record is identical.
            # No action needed unless MOVE_OFF_TABLET_FIELDNAME is set.
            if not urr.specifically_marked_for_preservation:
                urr.dirty = False
        else:
            # The existing record is different. We need a logical UPDATE, but
            # maintaining an audit trail.
            process_upload_record_special(req, batchdetails, table, valuedict)
            urr.newserverpk = insert_record(req, batchdetails, table,
                                            valuedict, oldserverpk)
            flag_modified(req, batchdetails,
                          table, oldserverpk, urr.newserverpk)
    else:
        # The record is NEW. We need to INSERT it.
        process_upload_record_special(req, batchdetails, table, valuedict)
        urr.newserverpk = insert_record(req, batchdetails, table,
                                        valuedict, None)
    if urr.specifically_marked_for_preservation:
        preservation_pks = flag_record_for_preservation(req, batchdetails,
                                                        table, urr.latest_pk)
        urr.note_specifically_marked_preservation_pks(preservation_pks)

    if DEBUG_UPLOAD:
        log.debug("upload_record_core: {}, {!r}", table.name, urr)
    return urr


def insert_record(req: "CamcopsRequest",
                  batchdetails: BatchDetails,
                  table: Table,
                  valuedict: Dict[str, Any],
                  predecessor_pk: Optional[int]) -> int:
    """
    Inserts a record, or raises an exception if that fails.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: an SQLAlchemy :class:`Table`
        valuedict: a dictionary of {colname: value} pairs from the client
        predecessor_pk: an optional server PK of the record's predecessor

    Returns:
        the server PK of the new record
    """
    ts = req.tabletsession
    valuedict.update({
        FN_DEVICE_ID: ts.device_id,
        FN_ERA: ERA_NOW,
        FN_REMOVAL_PENDING: 0,
        FN_PREDECESSOR_PK: predecessor_pk,
        FN_CAMCOPS_VERSION: ts.tablet_version_str,
        FN_GROUP_ID: req.user.upload_group_id,
    })
    if batchdetails.onestep:
        valuedict.update({
            FN_CURRENT: 1,
            FN_ADDITION_PENDING: 0,
            FN_ADDING_USER_ID: req.user_id,
            FN_WHEN_ADDED_EXACT: req.now,
            FN_WHEN_ADDED_BATCH_UTC: batchdetails.batchtime,
        })
    else:
        valuedict.update({
            FN_CURRENT: 0,
            FN_ADDITION_PENDING: 1,
        })
    rp = req.dbsession.execute(
        table.insert().values(valuedict)
    )  # type: ResultProxy
    inserted_pks = rp.inserted_primary_key
    assert(isinstance(inserted_pks, list) and len(inserted_pks) == 1)
    return inserted_pks[0]


def audit_upload(req: "CamcopsRequest",
                 changes: List[UploadTableChanges]) -> None:
    """
    Writes audit information for an upload.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        changes: a list of :class:`UploadTableChanges` objects, one per table
    """
    msg = (
        f"Upload from device {req.tabletsession.device_id}, "
        f"username {req.tabletsession.username!r}: "
    )
    changes = [x for x in changes if x.any_changes]
    if changes:
        changes.sort(key=lambda x: x.tablename)
        msg += ", ".join(x.description() for x in changes)
    else:
        msg += "No changes"
    log.info("audit_upload: {}", msg)
    audit(req, msg)


# =============================================================================
# Batch (atomic) upload and preserving
# =============================================================================

def get_batch_details(req: "CamcopsRequest") -> BatchDetails:
    """
    Returns the :class:`BatchDetails` for the current upload. If none exists,
    a new batch is created and returned.

    SIDE EFFECT: if the username is different from the username that started
    a previous upload batch for this device, we restart the upload batch (thus
    rolling back previous pending changes).

    Raises:
        :exc:`camcops_server.cc_modules.cc_client_api_core.ServerErrorException`
        if the device doesn't exist
    """
    device_id = req.tabletsession.device_id
    # noinspection PyUnresolvedReferences
    query = (
        select([Device.ongoing_upload_batch_utc,
                Device.uploading_user_id,
                Device.currently_preserving])
        .select_from(Device.__table__)
        .where(Device.id == device_id)
    )
    row = req.dbsession.execute(query).fetchone()
    if not row:
        fail_server_error(f"Device {device_id} missing from Device table")  # will raise  # noqa
    upload_batch_utc, uploading_user_id, currently_preserving = row
    if not upload_batch_utc or uploading_user_id != req.user_id:
        # SIDE EFFECT: if the username changes, we restart (and thus roll back
        # previous pending changes)
        start_device_upload_batch(req)
        return BatchDetails(req.now_utc, False)
    return BatchDetails(upload_batch_utc, currently_preserving)


def start_device_upload_batch(req: "CamcopsRequest") -> None:
    """
    Starts an upload batch for a device.
    """
    rollback_all(req)
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        update(Device.__table__)
        .where(Device.id == req.tabletsession.device_id)
        .values(last_upload_batch_utc=req.now_utc,
                ongoing_upload_batch_utc=req.now_utc,
                uploading_user_id=req.tabletsession.user_id)
    )


def _clear_ongoing_upload_batch_details(req: "CamcopsRequest") -> None:
    """
    Clears upload batch details from the Device table.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
    """
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        update(Device.__table__)
        .where(Device.id == req.tabletsession.device_id)
        .values(ongoing_upload_batch_utc=None,
                uploading_user_id=None,
                currently_preserving=0)
    )


def end_device_upload_batch(req: "CamcopsRequest",
                            batchdetails: BatchDetails) -> None:
    """
    Ends an upload batch, committing all changes made thus far.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
    """
    commit_all(req, batchdetails)
    _clear_ongoing_upload_batch_details(req)


def clear_device_upload_batch(req: "CamcopsRequest") -> None:
    """
    Ensures there is nothing pending. Rools back previous changes. Wipes any
    ongoing batch details.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
    """
    rollback_all(req)
    _clear_ongoing_upload_batch_details(req)


def start_preserving(req: "CamcopsRequest") -> None:
    """
    Starts preservation (the process of moving records from the NOW era to
    an older era, so they can be removed safely from the tablet).

    Called by :func:`op_start_preservation`.

    In this situation, we start by assuming that ALL tables are "dirty",
    because they may have live records from a previous upload.
    """
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        update(Device.__table__)
        .where(Device.id == req.tabletsession.device_id)
        .values(currently_preserving=1)
    )
    mark_all_tables_dirty(req)


def mark_table_dirty(req: "CamcopsRequest", table: Table) -> None:
    """
    Marks a table as having been modified during the current upload.
    """
    tablename = table.name
    device_id = req.tabletsession.device_id
    dbsession = req.dbsession
    # noinspection PyUnresolvedReferences
    table_already_dirty = exists_in_table(
        dbsession,
        DirtyTable.__table__,
        DirtyTable.device_id == device_id,
        DirtyTable.tablename == tablename
    )
    if not table_already_dirty:
        # noinspection PyUnresolvedReferences
        dbsession.execute(
            DirtyTable.__table__.insert()
            .values(device_id=device_id,
                    tablename=tablename)
        )


def mark_tables_dirty(req: "CamcopsRequest", tables: List[Table]) -> None:
    """
    Marks multiple tables as dirty.
    """
    if not tables:
        return
    device_id = req.tabletsession.device_id
    tablenames = [t.name for t in tables]
    # Delete first
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        DirtyTable.__table__.delete()
        .where(DirtyTable.device_id == device_id)
        .where(DirtyTable.tablename.in_(tablenames))
    )
    # Then insert
    insert_values = [
        {"device_id": device_id, "tablename": tn}
        for tn in tablenames
    ]
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        DirtyTable.__table__.insert(),
        insert_values
    )


def mark_all_tables_dirty(req: "CamcopsRequest") -> None:
    """
    If we are preserving, we assume that all tables are "dirty" (require work
    when we complete the upload) unless we specifically mark them clean.
    """
    device_id = req.tabletsession.device_id
    # Delete first
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        DirtyTable.__table__.delete()
        .where(DirtyTable.device_id == device_id)
    )
    # Now insert
    # https://docs.sqlalchemy.org/en/latest/core/tutorial.html#execute-multiple
    all_client_tablenames = list(CLIENT_TABLE_MAP.keys())
    insert_values = [
        {"device_id": device_id, "tablename": tn}
        for tn in all_client_tablenames
    ]
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        DirtyTable.__table__.insert(),
        insert_values
    )


def mark_table_clean(req: "CamcopsRequest", table: Table) -> None:
    """
    Marks a table as being clean: that is,

    - the table has been scanned during the current upload
    - there is nothing to do (either from the current upload, OR A PREVIOUS
      UPLOAD).
    """
    tablename = table.name
    device_id = req.tabletsession.device_id
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        DirtyTable.__table__.delete()
        .where(DirtyTable.device_id == device_id)
        .where(DirtyTable.tablename == tablename)
    )


def mark_tables_clean(req: "CamcopsRequest", tables: List[Table]) -> None:
    """
    Marks multiple tables as clean.
    """
    if not tables:
        return
    device_id = req.tabletsession.device_id
    tablenames = [t.name for t in tables]
    # Delete first
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        DirtyTable.__table__.delete()
        .where(DirtyTable.device_id == device_id)
        .where(DirtyTable.tablename.in_(tablenames))
    )


def get_dirty_tables(req: "CamcopsRequest") -> List[Table]:
    """
    Returns tables marked as dirty for this device. (See
    :func:`mark_table_dirty`.)
    """
    query = (
        select([DirtyTable.tablename])
        .where(DirtyTable.device_id == req.tabletsession.device_id)
    )
    tablenames = fetch_all_first_values(req.dbsession, query)
    return [CLIENT_TABLE_MAP[tn] for tn in tablenames]


def commit_all(req: "CamcopsRequest", batchdetails: BatchDetails) -> None:
    """
    Commits additions, removals, and preservations for all tables.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
    """
    tables = get_dirty_tables(req)
    # log.debug("Dirty tables: {}", list(t.name for t in tables))
    tables.sort(key=upload_commit_order_sorter)

    changelist = []  # type: List[UploadTableChanges]
    for table in tables:
        auditinfo = commit_table(req, batchdetails, table, clear_dirty=False)
        changelist.append(auditinfo)

    if batchdetails.preserving:
        # Also preserve/finalize any corresponding special notes (2015-02-01),
        # but all in one go (2018-11-13).
        # noinspection PyUnresolvedReferences
        req.dbsession.execute(
            update(SpecialNote.__table__)
            .where(SpecialNote.device_id == req.tabletsession.device_id)
            .where(SpecialNote.era == ERA_NOW)
            .values(era=batchdetails.new_era)
        )

    clear_dirty_tables(req)
    audit_upload(req, changelist)

    # Performance 2018-11-13:
    # - start at 2.407 s
    # - remove final temptable clearance and COUNT(*): 1.626 to 2.118 s
    # - IN clause using Python literal not temptable: 1.18 to 1.905 s
    # - minor tidy: 1.075 to 1.65
    # - remove ORDER BY from task indexing: 1.093 to 1.607
    # - optimize special note code won't affect this: 1.076 to 1.617
    # At this point, entire upload process ~5s.
    # - big difference from commit_table() query optimization
    # - huge difference from being more careful with mark_table_dirty()
    # - further table scanning optimizations: fewer queries
    # Overall upload down to ~2.4s


def commit_table(req: "CamcopsRequest",
                 batchdetails: BatchDetails,
                 table: Table,
                 clear_dirty: bool = True) -> UploadTableChanges:
    """
    Commits additions, removals, and preservations for one table.

    Should ONLY be called by :func:`commit_all`.

    Also updates task indexes.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: SQLAlchemy :class:`Table`
        clear_dirty: remove the table from the record of dirty tables for
            this device? (If called from :func:`commit_all`, this should be
            ``False``, since it's faster to clear all dirty tables for the
            device simultaneously than one-by-one.)

    Returns:
        an :class:`UploadTableChanges` object
    """

    # Tried storing PKs in temporary tables, rather than using an IN clause
    # with Python values, as per
    # https://www.xaprb.com/blog/2006/06/28/why-large-in-clauses-are-problematic/  # noqa
    # However, it was slow.
    # We can gain a lot of efficiency (empirically) by:
    # - Storing PKs in Python
    # - Only performing updates when we need to
    # - Using a single query per table to get "add/remove/preserve" PKs

    # -------------------------------------------------------------------------
    # Helpful temporary variables
    # -------------------------------------------------------------------------
    user_id = req.user_id
    device_id = req.tabletsession.device_id
    exacttime = req.now
    dbsession = req.dbsession
    tablename = table.name
    batchtime = batchdetails.batchtime
    preserving = batchdetails.preserving

    # -------------------------------------------------------------------------
    # Fetch addition, removal, preservation, current PKs in a single query
    # -------------------------------------------------------------------------
    tablechanges = UploadTableChanges(table)
    serverrecs = get_server_live_records(req, device_id, table,
                                         current_only=False)
    for sr in serverrecs:
        tablechanges.note_serverrec(sr, preserving=preserving)

    # -------------------------------------------------------------------------
    # Additions
    # -------------------------------------------------------------------------
    # Update the records we're adding
    addition_pks = tablechanges.addition_pks
    if addition_pks:
        # log.debug("commit_table: {}, adding server PKs {}",
        #           tablename, addition_pks)
        dbsession.execute(
            update(table)
            .where(table.c[FN_PK].in_(addition_pks))
            .values({
                FN_CURRENT: 1,
                FN_ADDITION_PENDING: 0,
                FN_ADDING_USER_ID: user_id,
                FN_WHEN_ADDED_EXACT: exacttime,
                FN_WHEN_ADDED_BATCH_UTC: batchtime
            })
        )

    # -------------------------------------------------------------------------
    # Removals
    # -------------------------------------------------------------------------
    # Update the records we're removing
    removal_pks = tablechanges.removal_pks
    if removal_pks:
        # log.debug("commit_table: {}, removing server PKs {}",
        #           tablename, removal_pks)
        dbsession.execute(
            update(table)
            .where(table.c[FN_PK].in_(removal_pks))
            .values(values_delete_now(req, batchdetails))
        )

    # -------------------------------------------------------------------------
    # Preservation
    # -------------------------------------------------------------------------
    # Preserve necessary records
    preservation_pks = tablechanges.preservation_pks
    if preservation_pks:
        # log.debug("commit_table: {}, preserving server PKs {}",
        #           tablename, preservation_pks)
        new_era = batchdetails.new_era
        dbsession.execute(
            update(table)
            .where(table.c[FN_PK].in_(preservation_pks))
            .values(values_preserve_now(req, batchdetails))
        )
        if not preserving:
            # Also preserve/finalize any corresponding special notes
            # (2015-02-01), just for records being specifically preserved. If
            # we are preserving, this step happens in one go in commit_all()
            # (2018-11-13).
            # noinspection PyUnresolvedReferences
            dbsession.execute(
                update(SpecialNote.__table__)
                .where(SpecialNote.basetable == tablename)
                .where(SpecialNote.device_id == device_id)
                .where(SpecialNote.era == ERA_NOW)
                .where(exists().select_from(table)
                       .where(table.c[TABLET_ID_FIELD] == SpecialNote.task_id)
                       .where(table.c[FN_DEVICE_ID] == SpecialNote.device_id)
                       .where(table.c[FN_ERA] == new_era))
                #             ^^^^^^^^^^^^^^^^^^^^^^^^^^
                #             This bit restricts to records being preserved.
                .values(era=new_era)
            )

    # -------------------------------------------------------------------------
    # Update special indexes
    # -------------------------------------------------------------------------
    update_indexes_and_push_exports(req, batchdetails, tablechanges)

    # -------------------------------------------------------------------------
    # Remove individually from list of dirty tables?
    # -------------------------------------------------------------------------
    if clear_dirty:
        # noinspection PyUnresolvedReferences
        dbsession.execute(
            DirtyTable.__table__.delete()
            .where(DirtyTable.device_id == device_id)
            .where(DirtyTable.tablename == tablename)
        )
        # ... otherwise a call to clear_dirty_tables() must be made.

    if DEBUG_UPLOAD:
        log.debug("commit_table: {}", tablechanges)

    return tablechanges


def rollback_all(req: "CamcopsRequest") -> None:
    """
    Rolls back all pending changes for a device.
    """
    tables = get_dirty_tables(req)
    for table in tables:
        rollback_table(req, table)
    clear_dirty_tables(req)


def rollback_table(req: "CamcopsRequest", table: Table) -> None:
    """
    Rolls back changes for an individual table for a device.
    """
    device_id = req.tabletsession.device_id
    # Pending additions
    req.dbsession.execute(
        table.delete()
        .where(table.c[FN_DEVICE_ID] == device_id)
        .where(table.c[FN_ADDITION_PENDING])
    )
    # Pending deletions
    req.dbsession.execute(
        update(table)
        .where(table.c[FN_DEVICE_ID] == device_id)
        .where(table.c[FN_REMOVAL_PENDING])
        .values({
            FN_REMOVAL_PENDING: 0,
            FN_WHEN_ADDED_EXACT: None,
            FN_WHEN_REMOVED_BATCH_UTC: None,
            FN_REMOVING_USER_ID: None,
            FN_SUCCESSOR_PK: None
        })
    )
    # Record-specific preservation (set by flag_record_for_preservation())
    req.dbsession.execute(
        update(table)
        .where(table.c[FN_DEVICE_ID] == device_id)
        .values({
            MOVE_OFF_TABLET_FIELD: 0
        })
    )


def clear_dirty_tables(req: "CamcopsRequest") -> None:
    """
    Clears the dirty-table list for a device.
    """
    device_id = req.tabletsession.device_id
    # noinspection PyUnresolvedReferences
    req.dbsession.execute(
        DirtyTable.__table__.delete()
        .where(DirtyTable.device_id == device_id)
    )


# =============================================================================
# Additional helper functions for one-step upload
# =============================================================================

def process_table_for_onestep_upload(
        req: "CamcopsRequest",
        batchdetails: BatchDetails,
        table: Table,
        clientpk_name: str,
        rows: List[Dict[str, Any]]) -> UploadTableChanges:
    """
    Performs all upload steps for a table.

    Note that we arrive here in a specific and safe table order; search for
    :func:`camcops_server.cc_modules.cc_client_api_helpers.upload_commit_order_sorter`.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        batchdetails: the :class:`BatchDetails`
        table: an SQLAlchemy :class:`Table`
        clientpk_name: the name of the PK field on the client
        rows: a list of rows, where each row is a dictionary mapping field
            (column) names to values (those values being encoded as SQL-style
            literals in our extended syntax)

    Returns:
        an :class:`UploadTableChanges` object
    """  # noqa
    serverrecs = get_server_live_records(
        req, req.tabletsession.device_id, table, clientpk_name,
        current_only=False)
    servercurrentrecs = [r for r in serverrecs if r.current]
    if rows and not clientpk_name:
        fail_user_error(f"Client-side PK name not specified by client for "
                        f"non-empty table {table.name!r}")
    tablechanges = UploadTableChanges(table)
    server_pks_uploaded = []  # type: List[int]
    for row in rows:
        valuedict = {k: decode_single_value(v) for k, v in row.items()}
        urr = upload_record_core(req, batchdetails, table,
                                 clientpk_name, valuedict,
                                 server_live_current_records=servercurrentrecs)
        # ... handles addition, modification, preservation, special processing
        # But we also make a note of these for indexing:
        if urr.oldserverpk is not None:
            server_pks_uploaded.append(urr.oldserverpk)
        tablechanges.note_urr(urr,
                              preserving_new_records=batchdetails.preserving)
    # Which leaves:
    # (*) Deletion (where no record was uploaded at all)
    server_pks_for_deletion = [r.server_pk for r in servercurrentrecs
                               if r.server_pk not in server_pks_uploaded]
    if server_pks_for_deletion:
        flag_deleted(req, batchdetails, table, server_pks_for_deletion)
        tablechanges.note_removal_deleted_pks(server_pks_for_deletion)

    # Preserving all records not specifically processed above, too
    if batchdetails.preserving:
        # Preserve all, including noncurrent:
        preserve_all(req, batchdetails, table)
        # Note other preserved records, for indexing:
        tablechanges.note_preservation_pks(r.server_pk for r in serverrecs)

    # (*) Indexing (and push exports)
    update_indexes_and_push_exports(req, batchdetails, tablechanges)

    if DEBUG_UPLOAD:
        log.debug("process_table_for_onestep_upload: {}", tablechanges)

    return tablechanges


# =============================================================================
# Audit functions
# =============================================================================

def audit(req: "CamcopsRequest",
          details: str,
          patient_server_pk: int = None,
          tablename: str = None,
          server_pk: int = None) -> None:
    """
    Audit something.
    """
    # Add parameters and pass on:
    cc_audit.audit(
        req=req,
        details=details,
        patient_server_pk=patient_server_pk,
        table=tablename,
        server_pk=server_pk,
        device_id=req.tabletsession.device_id,  # added
        remote_addr=req.remote_addr,  # added
        user_id=req.user_id,  # added
        from_console=False,  # added
        from_dbclient=True  # added
    )


# =============================================================================
# Helper functions for single-user mode
# =============================================================================

def make_single_user_mode_username(client_device_name: str,
                                   patient_pk: int) -> str:
    """
    Returns the username for single-user mode.
    """
    return f"user-{client_device_name}-{patient_pk}"


def json_patient_info(patient: Patient) -> str:
    """
    Converts patient details to a string representation of a JSON list (one
    patient) containing a single JSON dictionary (detailing that patient), with
    keys/formats known to the client.

    (One item list to be consistent with patients uploaded from the tablet.)

    Args:
        patient: :class:`camcops_server.cc_modules.cc_patient.Patient`
    """
    patient_dict = {
        TabletParam.SURNAME: patient.surname,
        TabletParam.FORENAME: patient.forename,
        TabletParam.SEX: patient.sex,
        TabletParam.DOB: format_datetime(patient.dob,
                                         DateFormat.ISO8601_DATE_ONLY),
        TabletParam.EMAIL: patient.email,
        TabletParam.ADDRESS: patient.address,
        TabletParam.GP: patient.gp,
        TabletParam.OTHER: patient.other,
    }
    for idnum in patient.idnums:
        key = f"{TabletParam.IDNUM_PREFIX}{idnum.which_idnum}"
        patient_dict[key] = idnum.idnum_value
    # One item list to be consistent with patients uploaded from the tablet
    return json.dumps([patient_dict])


def get_single_server_patient(req: "CamcopsRequest") -> Patient:
    """
    Returns the patient identified by the proquint access key present in this
    request, or raises.
    """
    _ = req.gettext

    patient_proquint = get_str_var(req, TabletParam.PATIENT_PROQUINT)
    assert patient_proquint is not None  # For type checker

    try:
        uuid_obj = uuid_from_proquint(patient_proquint)
    except InvalidProquintException:
        # Checksum failed or characters in wrong place
        # We'll do the same validation on the client so in theory
        # should never get here
        fail_user_error(
            _(
                "There is no patient with access key '{access_key}'. "
                "Have you entered the key correctly?"
            ).format(access_key=patient_proquint)
        )

    server_device = Device.get_server_device(req.dbsession)

    # noinspection PyUnboundLocalVariable,PyProtectedMember
    patient = req.dbsession.query(Patient).filter(
        Patient.uuid == uuid_obj,
        Patient._device_id == server_device.id,
        Patient._era == ERA_NOW,
        Patient._current == True  # noqa: E712
    ).options(joinedload(Patient.task_schedules)).one_or_none()

    if patient is None:
        fail_user_error(
            _(
                "There is no patient with access key '{access_key}'. "
                "Have you entered the key correctly?"
            ).format(access_key=patient_proquint)
        )

    if not patient.idnums:
        # In theory should never happen. The patient must be created with at
        # least one ID number. We did see this once in testing (possibly when
        # a patient created on a device was registered)
        _ = req.gettext
        fail_server_error(_("Patient has no ID numbers"))

    return patient


def get_or_create_single_user(req: "CamcopsRequest",
                              name: str,
                              patient: Patient) -> Tuple[User, str]:
    """
    Creates a user for a patient (who's using single-user mode).

    The user is associated (via its name) with the combination of a client
    device and a patient. (If a device is re-registered to another patient, the
    username will change.)

    If the username already exists, then since we can't look up the password
    (it's irreversibly encrypted), we will set it afresh.

    - Why is a user associated with a patient? So we can enforce that the user
      can upload only data relating to that patient.

    - Why is a user associated with a device?

      - If it is: then two users (e.g. "Device1-Bob" and "Device2-Bob") can
        independently work with the same patient. This will be highly
        confusing (mainly because it will allow "double" copies of tasks to be
        created, though only by manually entering things twice).

      - If it isn't (e.g. user "Bob"): then, because registering the patient on
        Device2 will reset the password for the user, registering a new device
        for a patient will "take over" from a previous device. That has some
        potential for data loss if work was in progress (incomplete tasks won't
        be uploadable any more, and re-registering [to fix the password on the
        first device] would delete data).

      - Since some confusion is better than some data loss, we associate users
        with a device/patient combination.

    Args:
        req:
            a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        name:
            username
        patient:
            associated :class:`camcops_server.cc_modules.cc_patient.Patient`,
            which also tells us the group in which to place this user

    Returns:
        tuple: :class:`camcops_server.cc_modules.cc_user.User`, password

    """
    dbsession = req.dbsession
    password = random_password()
    group = patient.group
    assert group is not None  # for type checker

    user = User.get_user_by_name(dbsession, name)
    creating_new_user = user is None
    if creating_new_user:
        # Create a fresh user.
        user = User(username=name)
    user.upload_group = group
    user.auto_generated = True
    user.superuser = False  # should be redundant!
    # noinspection PyProtectedMember
    user.single_patient_pk = patient._pk
    user.set_password(req, password)
    if creating_new_user:
        dbsession.add(user)
        # As the username is based on a UUID, we're pretty sure another
        # request won't have created the same user, otherwise we'd need
        # to catch IntegrityError
        dbsession.flush()

    membership = UserGroupMembership(
        user_id=user.id,
        group_id=group.id,
    )
    membership.may_register_devices = True
    membership.may_upload = True
    user.user_group_memberships = [membership]  # ... only these permissions

    return user, password


def random_password(length: int = 32) -> str:
    """
    Create a random password.
    """
    # Not trying anything clever with distributions of letters, digits etc
    characters = string.ascii_letters + string.digits + string.punctuation
    # We use secrets.choice() rather than random.choices() as it's better
    # for security/cryptography purposes.
    return "".join(secrets.choice(characters) for _ in range(length))


def get_task_schedules(req: "CamcopsRequest",
                       patient: Patient) -> str:
    """
    Gets a JSON string representation of the task schedules for a specified
    patient.
    """
    dbsession = req.dbsession

    schedules = []

    for pts in patient.task_schedules:
        if pts.start_datetime is None:
            # Minutes granularity so we are consistent with the form
            pts.start_datetime = req.now_utc.replace(second=0, microsecond=0)
            dbsession.add(pts)

        items = []

        for task_info in pts.get_list_of_scheduled_tasks(req):
            due_from = task_info.start_datetime.to_iso8601_string()
            due_by = task_info.end_datetime.to_iso8601_string()

            complete = False
            when_completed = None
            task = task_info.task
            if task:
                complete = task.is_complete()
                if complete and task.when_last_modified:
                    when_completed = task.when_last_modified.to_iso8601_string()  # noqa

            if pts.settings is not None:
                settings = pts.settings.get(task_info.tablename, {})
            else:
                settings = {}

            items.append({
                TabletParam.TABLE: task_info.tablename,
                TabletParam.ANONYMOUS: task_info.is_anonymous,
                TabletParam.SETTINGS: settings,
                TabletParam.DUE_FROM: due_from,
                TabletParam.DUE_BY: due_by,
                TabletParam.COMPLETE: complete,
                TabletParam.WHEN_COMPLETED: when_completed,
            })

        schedules.append({
            TabletParam.TASK_SCHEDULE_NAME: pts.task_schedule.name,
            TabletParam.TASK_SCHEDULE_ITEMS: items,
        })

    return json.dumps(schedules)


# =============================================================================
# Action processors: allowed to any user
# =============================================================================
# If they return None, the framework uses the operation name as the reply in
# the success message. Not returning anything is the same as returning None.
# Authentication is performed in advance of these.

def op_check_device_registered(req: "CamcopsRequest") -> None:
    """
    Check that a device is registered, or raise
    :exc:`UserErrorException`.
    """
    req.tabletsession.ensure_device_registered()


def op_register_patient(req: "CamcopsRequest") -> Dict[str, Any]:
    """
    Registers a patient. That is, the client provides an access key. If all
    is well, the server returns details of that patient, as well as key
    server parameters, plus (if required) the username/password to use.
    """
    # -------------------------------------------------------------------------
    # Patient details
    # -------------------------------------------------------------------------
    patient = get_single_server_patient(req)  # may fail/raise
    patient_info = json_patient_info(patient)
    reply_dict = {
        TabletParam.PATIENT_INFO: patient_info,
    }

    # -------------------------------------------------------------------------
    # Username/password
    # -------------------------------------------------------------------------
    client_device_name = get_str_var(req, TabletParam.DEVICE)
    # noinspection PyProtectedMember
    user_name = make_single_user_mode_username(client_device_name, patient._pk)
    user, password = get_or_create_single_user(req, user_name, patient)
    reply_dict[TabletParam.USER] = user.username
    reply_dict[TabletParam.PASSWORD] = password

    # -------------------------------------------------------------------------
    # Intellectual property settings
    # -------------------------------------------------------------------------
    ip_use = patient.group.ip_use or IpUse()
    # ... if the group doesn't have an associated ip_use object, use defaults
    ip_dict = {
        TabletParam.IP_USE_COMMERCIAL: int(ip_use.commercial),
        TabletParam.IP_USE_CLINICAL: int(ip_use.clinical),
        TabletParam.IP_USE_EDUCATIONAL: int(ip_use.educational),
        TabletParam.IP_USE_RESEARCH: int(ip_use.research),
    }
    reply_dict[TabletParam.IP_USE_INFO] = json.dumps(ip_dict)

    return reply_dict


# =============================================================================
# Action processors that require REGISTRATION privilege
# =============================================================================

def op_register_device(req: "CamcopsRequest") -> Dict[str, Any]:
    """
    Register a device with the server.

    Returns:
        server information dictionary (from :func:`get_server_id_info`)
    """
    dbsession = req.dbsession
    ts = req.tabletsession
    device_friendly_name = get_str_var(req, TabletParam.DEVICE_FRIENDLY_NAME,
                                       mandatory=False)
    # noinspection PyUnresolvedReferences
    device_exists = exists_in_table(
        dbsession,
        Device.__table__,
        Device.name == ts.device_name
    )
    if device_exists:
        # device already registered, but accept re-registration
        # noinspection PyUnresolvedReferences
        dbsession.execute(
            update(Device.__table__)
            .where(Device.name == ts.device_name)
            .values(friendly_name=device_friendly_name,
                    camcops_version=ts.tablet_version_str,
                    registered_by_user_id=req.user_id,
                    when_registered_utc=req.now_utc)
        )
    else:
        # new registration
        try:
            # noinspection PyUnresolvedReferences
            dbsession.execute(
                Device.__table__.insert()
                .values(name=ts.device_name,
                        friendly_name=device_friendly_name,
                        camcops_version=ts.tablet_version_str,
                        registered_by_user_id=req.user_id,
                        when_registered_utc=req.now_utc)
            )
        except IntegrityError:
            fail_user_error(INSERT_FAILED)

    ts.reload_device()
    audit(
        req,
        f"register, device_id={ts.device_id}, "
        f"friendly_name={device_friendly_name}",
        tablename=Device.__tablename__
    )
    return get_server_id_info(req)


def op_get_extra_strings(req: "CamcopsRequest") -> Dict[str, str]:
    """
    Fetch all local extra strings from the server.

    Returns:
        a SELECT-style reply (see :func:`get_select_reply`) for the
        extra-string table
    """
    fields = [ExtraStringFieldNames.TASK,
              ExtraStringFieldNames.NAME,
              ExtraStringFieldNames.LANGUAGE,
              ExtraStringFieldNames.VALUE]
    rows = req.get_all_extra_strings()
    reply = get_select_reply(fields, rows)
    audit(req, "get_extra_strings")
    return reply


# noinspection PyUnusedLocal
def op_get_allowed_tables(req: "CamcopsRequest") -> Dict[str, str]:
    """
    Returns the names of all possible tables on the server, each paired with
    the minimum client (tablet) version that will be accepted for each table.
    (Typically these are all the same as the minimum global tablet version.)

    Uses the SELECT-like syntax (see :func:`get_select_reply`).
    """
    tables_versions = all_tables_with_min_client_version()
    fields = [AllowedTablesFieldNames.TABLENAME,
              AllowedTablesFieldNames.MIN_CLIENT_VERSION]
    rows = [[k, str(v)] for k, v in tables_versions.items()]
    reply = get_select_reply(fields, rows)
    audit(req, "get_allowed_tables")
    return reply


def op_get_task_schedules(req: "CamcopsRequest") -> Dict[str, str]:
    """
    Return details of the task schedules for the patient associated with
    this request, for single-user mode. Also returns details of the single
    patient, in case that's changed.
    """
    patient = get_single_server_patient(req)
    patient_info = json_patient_info(patient)
    task_schedules = get_task_schedules(req, patient)
    return {
        TabletParam.PATIENT_INFO: patient_info,
        TabletParam.TASK_SCHEDULES: task_schedules,
    }


# =============================================================================
# Action processors that require UPLOAD privilege
# =============================================================================

# noinspection PyUnusedLocal
def op_check_upload_user_and_device(req: "CamcopsRequest") -> None:
    """
    Stub function for the operation to check that a user is valid.

    To get this far, the user has to be valid, so this function doesn't
    actually have to do anything.
    """
    pass  # don't need to do anything!


# noinspection PyUnusedLocal
def op_get_id_info(req: "CamcopsRequest") -> Dict[str, Any]:
    """
    Fetch server ID information; see :func:`get_server_id_info`.
    """
    return get_server_id_info(req)


def op_start_upload(req: "CamcopsRequest") -> None:
    """
    Begin an upload.
    """
    start_device_upload_batch(req)


def op_end_upload(req: "CamcopsRequest") -> None:
    """
    Ends an upload and commits changes.
    """
    batchdetails = get_batch_details(req)
    # ensure it's the same user finishing as starting!
    end_device_upload_batch(req, batchdetails)


def op_upload_table(req: "CamcopsRequest") -> str:
    """
    Upload a table.

    Incoming information in the POST request includes a CSV list of fields, a
    count of the number of records being provided, and a set of variables named
    ``record0`` ... ``record{nrecords - 1}``, each containing a CSV list of
    SQL-encoded values.

    Typically used for smaller tables, i.e. most except for BLOBs.
    """
    table = get_table_from_req(req, TabletParam.TABLE)

    allowed_nonexistent_fields = []  # type: List[str]
    # noinspection PyUnresolvedReferences
    if req.tabletsession.cope_with_old_idnums and table == Patient.__table__:
        for x in range(1, NUMBER_OF_IDNUMS_DEFUNCT + 1):
            allowed_nonexistent_fields.extend([
                FP_ID_NUM + str(x),
                FP_ID_DESC + str(x),
                FP_ID_SHORT_DESC + str(x)
            ])

    fields = get_fields_from_post_var(
        req, table, TabletParam.FIELDS,
        allowed_nonexistent_fields=allowed_nonexistent_fields)
    nrecords = get_int_var(req, TabletParam.NRECORDS)

    nfields = len(fields)
    if nfields < 1:
        fail_user_error(
            f"{TabletParam.FIELDS}={nfields}: can't be less than 1")
    if nrecords < 0:
        fail_user_error(
            f"{TabletParam.NRECORDS}={nrecords}: can't be less than 0")

    batchdetails = get_batch_details(req)

    ts = req.tabletsession
    if ts.explicit_pkname_for_upload_table:  # q.v.
        # New client: tells us the PK name explicitly.
        clientpk_name = get_single_field_from_post_var(req, table,
                                                       TabletParam.PKNAME)
    else:
        # Old client. Either (a) old Titanium client, in which the client PK
        # is in fields[0], or (b) an early C++ client, in which there was no
        # guaranteed order (and no explicit PK name was sent). However, in
        # either case, the client PK name was (is) always "id".
        clientpk_name = TABLET_ID_FIELD
        ensure_valid_field_name(table, clientpk_name)
    server_pks_uploaded = []  # type: List[int]
    n_new = 0
    n_modified = 0
    n_identical = 0
    dirty = False
    serverrecs = get_server_live_records(req, ts.device_id, table,
                                         clientpk_name=clientpk_name,
                                         current_only=True)
    for r in range(nrecords):
        recname = TabletParam.RECORD_PREFIX + str(r)
        values = get_values_from_post_var(req, recname)
        nvalues = len(values)
        if nvalues != nfields:
            errmsg = (
                f"Number of fields in field list ({nfields}) doesn't match "
                f"number of values in record {r} ({nvalues})"
            )
            log.warning(errmsg + f"\nfields: {fields!r}\nvalues: {values!r}")
            fail_user_error(errmsg)
        valuedict = dict(zip(fields, values))
        # log.debug("table {!r}, record {}: {!r}", table.name, r, valuedict)
        # CORE: CALLS upload_record_core
        urr = upload_record_core(
            req, batchdetails, table, clientpk_name, valuedict,
            server_live_current_records=serverrecs)
        if urr.oldserverpk is not None:  # was an existing record
            server_pks_uploaded.append(urr.oldserverpk)
            if urr.newserverpk is None:
                n_identical += 1
            else:
                n_modified += 1
        else:  # entirely new
            n_new += 1
        if urr.dirty:
            dirty = True

    # Now deal with any ABSENT (not in uploaded data set) conditions.
    server_pks_for_deletion = [r.server_pk for r in serverrecs
                               if r.server_pk not in server_pks_uploaded]
    # Note that "deletion" means "end of the line"; records that are modified
    # and replaced were handled by upload_record_core().
    n_deleted = len(server_pks_for_deletion)
    if n_deleted > 0:
        flag_deleted(req, batchdetails, table, server_pks_for_deletion)

    # Set dirty/clean status
    if (dirty or n_new > 0 or n_modified > 0 or n_deleted > 0 or
            any(sr.move_off_tablet for sr in serverrecs)):
        # ... checks on n_new and n_modified are redundant; dirty will be True
        mark_table_dirty(req, table)
    elif batchdetails.preserving and not serverrecs:
        # We've scanned this table, and there would be no work to do to
        # preserve records from previous uploads.
        mark_table_clean(req, table)

    # Special for old tablets:
    # noinspection PyUnresolvedReferences
    if req.tabletsession.cope_with_old_idnums and table == Patient.__table__:
        # noinspection PyUnresolvedReferences
        mark_table_dirty(req, PatientIdNum.__table__)
        # Mark patient ID numbers for deletion if their parent Patient is
        # similarly being marked for deletion
        # noinspection PyUnresolvedReferences,PyProtectedMember
        req.dbsession.execute(
            update(PatientIdNum.__table__)
            .where(PatientIdNum._device_id == Patient._device_id)
            .where(PatientIdNum._era == ERA_NOW)
            .where(PatientIdNum.patient_id == Patient.id)
            .where(Patient._pk.in_(server_pks_for_deletion))
            .where(Patient._era == ERA_NOW)  # shouldn't be in doubt!
            .values(_removal_pending=1,
                    _successor_pk=None)
        )

    # Auditing occurs at commit_all.
    log.info("Upload successful; {n} records uploaded to table {t} "
             "({new} new, {mod} modified, {i} identical, {nd} deleted)",
             n=nrecords, t=table.name, new=n_new, mod=n_modified,
             i=n_identical, nd=n_deleted)
    return f"Table {table.name} upload successful"


def op_upload_record(req: "CamcopsRequest") -> str:
    """
    Upload an individual record. (Typically used for BLOBs.)
    Incoming POST information includes a CSV list of fields and a CSV list of
    values.
    """
    batchdetails = get_batch_details(req)
    table = get_table_from_req(req, TabletParam.TABLE)
    clientpk_name = get_single_field_from_post_var(req, table,
                                                   TabletParam.PKNAME)
    valuedict = get_fields_and_values(req, table,
                                      TabletParam.FIELDS, TabletParam.VALUES)
    urr = upload_record_core(
        req, batchdetails, table, clientpk_name, valuedict
    )
    if urr.dirty:
        mark_table_dirty(req, table)
    if urr.oldserverpk is None:
        log.info("upload-insert")
        return "UPLOAD-INSERT"
    else:
        if urr.newserverpk is None:
            log.info("upload-update: skipping existing record")
        else:
            log.info("upload-update")
        return "UPLOAD-UPDATE"
    # Auditing occurs at commit_all.


def op_upload_empty_tables(req: "CamcopsRequest") -> str:
    """
    The tablet supplies a list of tables that are empty at its end, and we
    will 'wipe' all appropriate tables; this reduces the number of HTTP
    requests.
    """
    tables = get_tables_from_post_var(req, TabletParam.TABLES)
    batchdetails = get_batch_details(req)
    to_dirty = []  # type: List[Table]
    to_clean = []  # type: List[Table]
    for table in tables:
        nrows_affected = flag_all_records_deleted(req, table)
        if nrows_affected > 0:
            to_dirty.append(table)
        elif batchdetails.preserving:
            # There are no records in the current era for this device.
            to_clean.append(table)
    # In the fewest number of queries:
    mark_tables_dirty(req, to_dirty)
    mark_tables_clean(req, to_clean)
    log.info("upload_empty_tables")
    # Auditing occurs at commit_all.
    return "UPLOAD-EMPTY-TABLES"


def op_start_preservation(req: "CamcopsRequest") -> str:
    """
    Marks this upload batch as one in which all records will be preserved
    (i.e. moved from NOW-era to an older era, so they can be deleted safely
    from the tablet).

    Without this, individual records can still be marked for preservation if
    their MOVE_OFF_TABLET_FIELD field (``_move_off_tablet``) is set; see
    :func:`upload_record` and the functions it calls.
    """
    get_batch_details(req)
    start_preserving(req)
    log.info("start_preservation successful")
    # Auditing occurs at commit_all.
    return "STARTPRESERVATION"


def op_delete_where_key_not(req: "CamcopsRequest") -> str:
    """
    Marks records for deletion, for a device/table, where the client PK
    is not in a specified list.
    """
    table = get_table_from_req(req, TabletParam.TABLE)
    clientpk_name = get_single_field_from_post_var(
        req, table, TabletParam.PKNAME)
    clientpk_values = get_values_from_post_var(req, TabletParam.PKVALUES)

    get_batch_details(req)
    flag_deleted_where_clientpk_not(req, table, clientpk_name, clientpk_values)
    # Auditing occurs at commit_all.
    # log.info("delete_where_key_not successful; table {} trimmed", table)
    return "Trimmed"


def op_which_keys_to_send(req: "CamcopsRequest") -> str:
    """
    Intended use: "For my device, and a specified table, here are my client-
    side PKs (as a CSV list), and the modification dates for each corresponding
    record (as a CSV list). Please tell me which records have mismatching dates
    on the server, i.e. those that I need to re-upload."

    Used particularly for BLOBs, to reduce traffic, i.e. so we don't have to
    send a lot of BLOBs.

    Note new ``TabletParam.MOVE_OFF_TABLET_VALUES`` parameter in server v2.3.0,
    with bugfix for pre-2.3.0 clients that won't send this; see changelog.
    """
    # -------------------------------------------------------------------------
    # Get details
    # -------------------------------------------------------------------------
    try:
        table = get_table_from_req(req, TabletParam.TABLE)
    except IgnoringAntiqueTableException:
        raise IgnoringAntiqueTableException("")
    clientpk_name = get_single_field_from_post_var(req, table,
                                                   TabletParam.PKNAME)
    clientpk_values = get_values_from_post_var(req, TabletParam.PKVALUES,
                                               mandatory=False)
    # ... should be autoconverted to int, but we check below
    client_dates = get_values_from_post_var(req, TabletParam.DATEVALUES,
                                            mandatory=False)
    # ... will be in string format

    npkvalues = len(clientpk_values)
    ndatevalues = len(client_dates)
    if npkvalues != ndatevalues:
        fail_user_error(
            f"Number of PK values ({npkvalues}) doesn't match number of dates "
            f"({ndatevalues})")

    # v2.3.0:
    move_off_tablet_values = []  # type: List[int]  # for type checker
    if req.has_param(TabletParam.MOVE_OFF_TABLET_VALUES):
        client_reports_move_off_tablet = True
        move_off_tablet_values = get_values_from_post_var(
            req, TabletParam.MOVE_OFF_TABLET_VALUES, mandatory=True)
        # ... should be autoconverted to int
        n_motv = len(move_off_tablet_values)
        if n_motv != npkvalues:
            fail_user_error(
                f"Number of move-off-tablet values ({n_motv}) doesn't match "
                f"number of PKs ({npkvalues})")
        try:
            move_off_tablet_values = [bool(x) for x in move_off_tablet_values]
        except (TypeError, ValueError):
            fail_user_error(
                f"Bad move-off-tablet values: {move_off_tablet_values!r}")
    else:
        client_reports_move_off_tablet = False
        log.warning(
            "op_which_keys_to_send: old client not reporting "
            "{}; requesting all records",
            TabletParam.MOVE_OFF_TABLET_VALUES
        )

    clientinfo = []  # type: List[WhichKeyToSendInfo]

    for i in range(npkvalues):
        cpkv = clientpk_values[i]
        if not isinstance(cpkv, int):
            fail_user_error(f"Bad (non-integer) client PK: {cpkv!r}")
        dt = None  # for type checker
        try:
            dt = coerce_to_pendulum(client_dates[i])
            if dt is None:
                fail_user_error(f"Missing date/time for client PK {cpkv}")
        except ValueError:
            fail_user_error(f"Bad date/time: {client_dates[i]!r}")
        clientinfo.append(WhichKeyToSendInfo(
            client_pk=cpkv,
            client_when=dt,
            client_move_off_tablet=(
                move_off_tablet_values[i]
                if client_reports_move_off_tablet else False
            )
        ))

    # -------------------------------------------------------------------------
    # Work out the answer
    # -------------------------------------------------------------------------
    batchdetails = get_batch_details(req)

    # 1. The client sends us all its PKs. So "delete" anything not in that
    #    list.
    flag_deleted_where_clientpk_not(req, table, clientpk_name, clientpk_values)

    # 2. See which ones are new or updates.
    client_pks_needed = []  # type: List[int]
    client_pk_to_serverrec = client_pks_that_exist(
        req, table, clientpk_name, clientpk_values)
    for wk in clientinfo:
        if client_reports_move_off_tablet:
            if wk.client_pk not in client_pk_to_serverrec:
                # New on the client; we want it
                client_pks_needed.append(wk.client_pk)
            else:
                # We know about some version of this client record.
                serverrec = client_pk_to_serverrec[wk.client_pk]
                if serverrec.server_when != wk.client_when:
                    # Modified on the client; we want it
                    client_pks_needed.append(wk.client_pk)
                elif serverrec.move_off_tablet != wk.client_move_off_tablet:
                    # Not modified on the client. But it is being preserved.
                    # We don't need to ask the client for it again, but we do
                    # need to mark the preservation.
                    flag_record_for_preservation(req, batchdetails, table,
                                                 serverrec.server_pk)

        else:
            # Client hasn't told us about the _move_off_tablet flag. Always
            # request the record (workaround potential bug in old clients).
            client_pks_needed.append(wk.client_pk)

    # Success
    pk_csv_list = ",".join([str(x) for x in client_pks_needed if x is not None])  # noqa
    # log.info("which_keys_to_send successful: table {}", table.name)
    return pk_csv_list


def op_validate_patients(req: "CamcopsRequest") -> str:
    """
    As of v2.3.0, the client can use this command to validate patients against
    arbitrary server criteria -- definitely the upload/finalize ID policies,
    but potentially also other criteria of the server's (like matching against
    a bank of predefined patients).

    Compare ``NetworkManager::getPatientInfoJson()`` on the client.

    There is a slight weakness with respect to "single-patient" users, in that
    the client *asks* if the patients are OK (rather than the server
    *enforcing* that they are OK, via hooks into :func:`op_upload_table`,
    :func:`op_upload_record`, :func:`op_upload_entire_database` -- made more
    complex because ID numbers are not uploaded to the same table...). In
    principle, the weakness is that a user could (a) crack their assigned
    password and (b) rework the CamCOPS client, in order to upload "bad"
    patient data into their assigned group.

    todo:
        address this by having the server *require* patient validation for
        all uploads?

    """
    pt_json_list = get_json_from_post_var(req, TabletParam.PATIENT_INFO,
                                          decoder=PATIENT_INFO_JSON_DECODER,
                                          mandatory=True)
    if not isinstance(pt_json_list, list):
        fail_user_error("Top-level JSON is not a list")
    group = Group.get_group_by_id(req.dbsession, req.user.upload_group_id)
    for pt_dict in pt_json_list:
        ensure_valid_patient_json(req, group, pt_dict)
    return SUCCESS_MSG


def op_upload_entire_database(req: "CamcopsRequest") -> str:
    """
    Perform a one-step upload of the entire database.

    - From v2.3.0.
    - Therefore, we do not have to cope with old-style ID numbers.
    """
    # Roll back and clear any outstanding changes
    clear_device_upload_batch(req)

    # Fetch the JSON, with sanity checks
    preserving = get_bool_int_var(req, TabletParam.FINALIZING)
    pknameinfo = get_json_from_post_var(
        req, TabletParam.PKNAMEINFO, decoder=DB_JSON_DECODER, mandatory=True)
    if not isinstance(pknameinfo, dict):
        fail_user_error("PK name info JSON is not a dict")
    dbdata = get_json_from_post_var(
        req, TabletParam.DBDATA, decoder=DB_JSON_DECODER, mandatory=True)
    if not isinstance(dbdata, dict):
        fail_user_error("Database data JSON is not a dict")

    # Sanity checks
    dbdata_tablenames = sorted(dbdata.keys())
    pkinfo_tablenames = sorted(pknameinfo.keys())
    if pkinfo_tablenames != dbdata_tablenames:
        fail_user_error("Table names don't match from (1) DB data (2) PK info")
    duff_tablenames = sorted(list(set(dbdata_tablenames) -
                                  set(CLIENT_TABLE_MAP.keys())))
    if duff_tablenames:
        fail_user_error(
            f"Attempt to upload nonexistent tables: {duff_tablenames!r}")

    # Perform the upload
    batchdetails = BatchDetails(req.now_utc, preserving=preserving,
                                onestep=True)  # NB special "onestep" option
    # Process the tables in a certain order:
    tables = sorted(CLIENT_TABLE_MAP.values(),
                    key=upload_commit_order_sorter)
    changelist = []  # type: List[UploadTableChanges]
    for table in tables:
        clientpk_name = pknameinfo.get(table.name, "")
        rows = dbdata.get(table.name, [])
        tablechanges = process_table_for_onestep_upload(
            req, batchdetails, table, clientpk_name, rows)
        changelist.append(tablechanges)

    # Audit
    audit_upload(req, changelist)

    # Done
    return SUCCESS_MSG


# =============================================================================
# Action maps
# =============================================================================

class Operations:
    """
    Constants giving the name of operations (commands) accepted by this API.
    """
    CHECK_DEVICE_REGISTERED = "check_device_registered"
    CHECK_UPLOAD_USER_DEVICE = "check_upload_user_and_device"
    DELETE_WHERE_KEY_NOT = "delete_where_key_not"
    END_UPLOAD = "end_upload"
    GET_ALLOWED_TABLES = "get_allowed_tables"  # v2.2.0
    GET_EXTRA_STRINGS = "get_extra_strings"
    GET_ID_INFO = "get_id_info"
    GET_TASK_SCHEDULES = "get_task_schedules"  # v2.4.0
    REGISTER = "register"
    REGISTER_PATIENT = "register_patient"  # v2.4.0
    START_PRESERVATION = "start_preservation"
    START_UPLOAD = "start_upload"
    UPLOAD_EMPTY_TABLES = "upload_empty_tables"
    UPLOAD_ENTIRE_DATABASE = "upload_entire_database"  # v2.3.0
    UPLOAD_RECORD = "upload_record"
    UPLOAD_TABLE = "upload_table"
    VALIDATE_PATIENTS = "validate_patients"  # v2.3.0
    WHICH_KEYS_TO_SEND = "which_keys_to_send"


OPERATIONS_ANYONE = {
    Operations.CHECK_DEVICE_REGISTERED: op_check_device_registered,
    # Anyone can register a patient provided they have the right unique code
    Operations.REGISTER_PATIENT: op_register_patient,
}
OPERATIONS_REGISTRATION = {
    Operations.GET_ALLOWED_TABLES: op_get_allowed_tables,  # v2.2.0
    Operations.GET_EXTRA_STRINGS: op_get_extra_strings,
    Operations.GET_TASK_SCHEDULES: op_get_task_schedules,
    Operations.REGISTER: op_register_device,
}
OPERATIONS_UPLOAD = {
    Operations.CHECK_UPLOAD_USER_DEVICE: op_check_upload_user_and_device,
    Operations.DELETE_WHERE_KEY_NOT: op_delete_where_key_not,
    Operations.END_UPLOAD: op_end_upload,
    Operations.GET_ID_INFO: op_get_id_info,
    Operations.START_PRESERVATION: op_start_preservation,
    Operations.START_UPLOAD: op_start_upload,
    Operations.UPLOAD_EMPTY_TABLES: op_upload_empty_tables,
    Operations.UPLOAD_ENTIRE_DATABASE: op_upload_entire_database,
    Operations.UPLOAD_RECORD: op_upload_record,
    Operations.UPLOAD_TABLE: op_upload_table,
    Operations.VALIDATE_PATIENTS: op_validate_patients,  # v2.3.0
    Operations.WHICH_KEYS_TO_SEND: op_which_keys_to_send,
}


# =============================================================================
# Client API main functions
# =============================================================================

def main_client_api(req: "CamcopsRequest") -> Dict[str, str]:
    """
    Main HTTP processor.

    For success, returns a dictionary to send (will use status '200 OK')
    For failure, raises an exception.
    """
    # log.info("CamCOPS database script starting at {}",
    #          format_datetime(req.now, DateFormat.ISO8601))
    ts = req.tabletsession
    fn = None

    if ts.operation in OPERATIONS_ANYONE:
        fn = OPERATIONS_ANYONE.get(ts.operation)

    elif ts.operation in OPERATIONS_REGISTRATION:
        ts.ensure_valid_user_for_device_registration()
        fn = OPERATIONS_REGISTRATION.get(ts.operation)

    elif ts.operation in OPERATIONS_UPLOAD:
        ts.ensure_valid_device_and_user_for_uploading()
        fn = OPERATIONS_UPLOAD.get(ts.operation)

    if not fn:
        fail_unsupported_operation(ts.operation)
    result = fn(req)
    if result is None:
        # generic success
        result = {TabletParam.RESULT: ts.operation}
    elif not isinstance(result, dict):
        # convert strings (etc.) to a dictionary
        result = {TabletParam.RESULT: result}
    return result


@view_config(route_name=Routes.CLIENT_API, permission=NO_PERMISSION_REQUIRED)
@view_config(route_name=Routes.CLIENT_API_ALIAS,
             permission=NO_PERMISSION_REQUIRED)
def client_api(req: "CamcopsRequest") -> Response:
    """
    View for client API. All tablet interaction comes through here.
    Wraps :func:`main_client_api`.

    Internally, replies are managed as dictionaries.
    For the final reply, the dictionary is converted to text in this format:

    .. code-block:: none

        k1:v1
        k2:v2
        k3:v3
        ...
    """
    # log.debug("{!r}", req.environ)
    # log.debug("{!r}", req.params)
    t0 = time.time()  # in seconds

    try:
        resultdict = main_client_api(req)
        resultdict[TabletParam.SUCCESS] = SUCCESS_CODE
        status = '200 OK'

    except IgnoringAntiqueTableException as e:
        log.warning(IGNORING_ANTIQUE_TABLE_MESSAGE)
        resultdict = {
            TabletParam.RESULT: escape_newlines(str(e)),
            TabletParam.SUCCESS: SUCCESS_CODE,
        }
        status = '200 OK'

    except UserErrorException as e:
        log.warning("CLIENT-SIDE SCRIPT ERROR: {}", e)
        resultdict = {
            TabletParam.SUCCESS: FAILURE_CODE,
            TabletParam.ERROR: escape_newlines(str(e))
        }
        status = '200 OK'

    except ServerErrorException as e:
        log.error("SERVER-SIDE SCRIPT ERROR: {}", e)
        # rollback? Not sure
        resultdict = {
            TabletParam.SUCCESS: FAILURE_CODE,
            TabletParam.ERROR: escape_newlines(str(e))
        }
        status = "503 Database Unavailable: " + str(e)

    except Exception as e:
        # All other exceptions. May include database write failures.
        # Let's return with status '200 OK'; though this seems dumb, it means
        # the tablet user will at least see the message.
        log.exception("Unhandled exception")  # + traceback.format_exc()
        resultdict = {
            TabletParam.SUCCESS: FAILURE_CODE,
            TabletParam.ERROR: escape_newlines(exception_description(e))
        }
        status = '200 OK'

    # Add session token information
    ts = req.tabletsession
    resultdict[TabletParam.SESSION_ID] = ts.session_id
    resultdict[TabletParam.SESSION_TOKEN] = ts.session_token

    # Convert dictionary to text in name-value pair format
    txt = "".join(f"{k}:{v}\n" for k, v in resultdict.items())

    t1 = time.time()
    log.debug("Time in script (s): {t}", t=t1 - t0)

    return TextResponse(txt, status=status)


# =============================================================================
# Unit tests
# =============================================================================

TEST_NHS_NUMBER = 4887211163  # generated at random


def get_reply_dict_from_response(response: Response) -> Dict[str, str]:
    """
    For unit testing: convert the text in a :class:`Response` back to a
    dictionary, so we can check it was correct.
    """
    txt = str(response)
    d = {}  # type: Dict[str, str]
    # Format is: "200 OK\r\n<other headers>\r\n\r\n<content>"
    # There's a blank line between the heads and the body.
    http_gap = "\r\n\r\n"
    camcops_linesplit = "\n"
    camcops_k_v_sep = ":"
    try:
        start_of_content = txt.index(http_gap) + len(http_gap)
        txt = txt[start_of_content:]
        for line in txt.split(camcops_linesplit):
            if not line:
                continue
            colon_pos = line.index(camcops_k_v_sep)
            key = line[:colon_pos]
            value = line[colon_pos + len(camcops_k_v_sep):]
            key = key.strip()
            value = value.strip()
            d[key] = value
        return d
    except ValueError:
        return {}
