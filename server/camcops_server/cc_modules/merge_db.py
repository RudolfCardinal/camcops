#!/usr/bin/env python

"""
camcops_server/cc_modules/merge_db.py

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

**Tool to merge data from one CamCOPS database into another.**

Has special code to deal with old databases.

"""

import logging
from pprint import pformat
from typing import Any, cast, Dict, List, Optional, Type, TYPE_CHECKING

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.sqlalchemy.merge_db import merge_db, TranslationContext
from cardinal_pythonlib.sqlalchemy.schema import get_table_names
from cardinal_pythonlib.sqlalchemy.session import get_safe_url_from_engine
from cardinal_pythonlib.sqlalchemy.table_identity import TableIdentity
from sqlalchemy.engine import create_engine
from sqlalchemy.engine.base import Engine
from sqlalchemy.orm.exc import MultipleResultsFound, NoResultFound
from sqlalchemy.orm.session import Session
from sqlalchemy.sql.expression import column, func, select, table, text

from camcops_server.cc_modules.cc_audit import AuditEntry
from camcops_server.cc_modules.cc_constants import (
    FP_ID_NUM,
    NUMBER_OF_IDNUMS_DEFUNCT,
)
from camcops_server.cc_modules.cc_db import GenericTabletRecordMixin
from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_dirtytables import DirtyTable
from camcops_server.cc_modules.cc_email import Email
from camcops_server.cc_modules.cc_exportmodels import (
    ExportedTask,
    ExportedTaskEmail,
    ExportedTaskFileGroup,
    ExportedTaskHL7Message,
)
from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient
from camcops_server.cc_modules.cc_group import Group, group_group_table
from camcops_server.cc_modules.cc_idnumdef import IdNumDefinition
from camcops_server.cc_modules.cc_membership import UserGroupMembership
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import (
    fake_tablet_id_for_patientidnum,
    PatientIdNum,
)
from camcops_server.cc_modules.cc_request import get_command_line_request
from camcops_server.cc_modules.cc_session import CamcopsSession
from camcops_server.cc_modules.cc_serversettings import (
    server_stored_var_table_defunct,
    ServerSettings,
    ServerStoredVarNamesDefunct,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_taskindex import reindex_everything
from camcops_server.cc_modules.cc_user import (
    SecurityAccountLockout,
    SecurityLoginFailure,
    User,
)

if TYPE_CHECKING:
    from sqlalchemy.engine.result import ResultProxy

log = BraceStyleAdapter(logging.getLogger(__name__))

DEBUG_VIA_PDB = False


# =============================================================================
# Information relating to the source database
# =============================================================================

def get_skip_tables(src_tables: List[str]) -> List[TableIdentity]:
    """
    From the list of source table names provided, return details of tables in
    the metadata to skip because they are not in the source database.

    Also checks that some core CamCOPS tables are present in the source, or
    raises :exc:`ValueError`.

    Args:
        src_tables: list of all table names in the source database

    Returns:
        list of
        :class:`cardinal_pythonlib.sqlalchemy.table_identity.TableIdentity`
        objects representing tables to skip

    Note that other tables to skip are defined in :func:`merge_camcops_db`.

    """
    skip_tables = []  # type: List[TableIdentity]

    # Check we have some core tables present in the sources

    for tname in [Patient.__tablename__, User.__tablename__]:
        if tname not in src_tables:
            raise ValueError(
                f"Cannot proceed; table {tname!r} missing from source; "
                f"unlikely that the source is any sort of old CamCOPS "
                f"database!")

    # In general, we allow missing source tables.
    # However, we can't allow source tables to be missing if they are
    # automatically eager-loaded by relationships. This is only true in
    # CamCOPS for some high-performance queries: Patient, User,
    # PatientIdNum. In the context of merges we're going to run, that means
    # PatientIdNum.

    # SKIP -- disable eager loading instead
    # # Create patient ID number table in SOURCE database, because it's
    # # eager-loaded
    # if PatientIdNum.__tablename__ not in src_tables:
    #     create_table_from_orm_class(engine=src_engine,
    #                                 ormclass=PatientIdNum,
    #                                 without_constraints=True)

    if Group.__tablename__ not in src_tables:
        log.warning("No Group information in source database; skipping source "
                    "table {!r}; will create a default group",
                    Group.__tablename__)
        skip_tables.append(TableIdentity(tablename=Group.__tablename__))

    return skip_tables


def get_src_iddefs(src_engine: Engine,
                   src_tables: List[str]) -> Dict[int, IdNumDefinition]:
    """
    Get information about all the ID number definitions in the source database.

    Args:
        src_engine: source SQLAlchemy :class:`Engine`
        src_tables: list of all table names in the source database

    Returns:
        dictionary: ``{which_idnum: idnumdef}`` mappings, where each
        ``idnumdef`` is a
        :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition` not
        attached to any database session
    """
    iddefs = {}  # type: Dict[int, IdNumDefinition]
    if IdNumDefinition.__tablename__ in src_tables:
        # Source is a more modern CamCOPS database, with an IdNumDefinition
        # table.
        log.info("Fetching source ID number definitions from {!r} table",
                 IdNumDefinition.__tablename__)
        # noinspection PyUnresolvedReferences
        q = (
            select([IdNumDefinition.which_idnum,
                    IdNumDefinition.description,
                    IdNumDefinition.short_description])
            .select_from(IdNumDefinition.__table__)
            .order_by(IdNumDefinition.which_idnum)
        )
        rows = src_engine.execute(q).fetchall()
        for row in rows:
            which_idnum = row[0]
            iddefs[which_idnum] = IdNumDefinition(
                which_idnum=which_idnum,
                description=row[1],
                short_description=row[2]
            )
    elif server_stored_var_table_defunct.name in src_tables:
        # Source is an older CamCOPS database.
        log.info("Fetching source ID number definitions from {!r} table",
                 server_stored_var_table_defunct.name)
        for which_idnum in range(1, NUMBER_OF_IDNUMS_DEFUNCT + 1):
            nstr = str(which_idnum)
            qd = (
                select([server_stored_var_table_defunct.columns.valueText])
                .select_from(server_stored_var_table_defunct)
                .where(server_stored_var_table_defunct.columns.name ==
                       ServerStoredVarNamesDefunct.ID_DESCRIPTION_PREFIX
                       + nstr)
            )
            rd = src_engine.execute(qd).fetchall()
            qs = (
                select([server_stored_var_table_defunct.columns.valueText])
                .select_from(server_stored_var_table_defunct)
                .where(server_stored_var_table_defunct.columns.name ==
                       ServerStoredVarNamesDefunct.ID_SHORT_DESCRIPTION_PREFIX
                       + nstr)
            )
            rs = src_engine.execute(qs).fetchall()
            iddefs[which_idnum] = IdNumDefinition(
                which_idnum=which_idnum,
                description=rd[0][0] if rd else None,
                short_description=rs[0][0] if rs else None
            )
    else:
        log.warning("No information available on source ID number "
                    "descriptions")
    return iddefs


# =============================================================================
# Information relating to the destination database
# =============================================================================

def group_exists(group_id: int, dst_session: Session) -> bool:
    """
    Does a group exist in the destination session with the specified group ID?

    Args:
        group_id: integer group ID
        dst_session: destination SQLAlchemy :class:`Session`
    """
    return Group.group_exists(dbsession=dst_session, group_id=group_id)


def fetch_group_id_by_name(group_name: str, dst_session: Session) -> int:
    """
    Returns the group ID of the group with the specified name, in the
    destination session.

    If there are multiple such groups, that's a bug, and
    :exc:`MultipleResultsFound` will be raised.

    If there's no such group in the destination database with that name, one
    will be created, and its ID returned.

    Args:
        group_name: group name
        dst_session: destination SQLAlchemy :class:`Session`

    Returns:
        group ID in the destination database

    """
    try:
        group = (
            dst_session.query(Group)
            .filter(Group.name == group_name)
            .one()
        )  # type: Group
        # ... will fail if there are 0 or >1 results
    except MultipleResultsFound:
        log.critical("Nasty bug: can't have two groups with the same name! "
                     "Group name was {!r}", group_name)
        raise
    except NoResultFound:
        log.info("Creating new group named {!r}", group_name)
        group = Group()
        group.name = group_name
        dst_session.add(group)
        flush_session(dst_session)  # creates the PK
        # https://stackoverflow.com/questions/1316952/sqlalchemy-flush-and-get-inserted-id  # noqa
        log.info("... new group has ID {!r}", group.id)
    return group.id


def get_dst_group(dest_groupnum: int,
                  dst_session: Session) -> Group:
    """
    Ensures that the specified group number exists in the destination database
    and returns the corresponding group.

    Args:
        dest_groupnum: group number
        dst_session: SQLAlchemy session for the destination database

    Returns:
        the group

    Raises:
        :exc:`ValueError` upon failure
    """
    try:
        group = (
            dst_session.query(Group)
            .filter(Group.id == dest_groupnum)
            .one()
        )  # type: Group
        # ... will fail if there are 0 or >1 results
    except MultipleResultsFound:
        log.critical("Nasty bug: can't have two groups with the same ID! "
                     "Group ID was {!r}", dest_groupnum)
        raise
    except NoResultFound:
        raise ValueError(f"Group with ID {dest_groupnum} missing from "
                         f"destination database")
    return group


def ensure_dest_iddef_exists(which_idnum: int,
                             dst_session: Session) -> IdNumDefinition:
    """
    Ensures that the specified ID number type exists in the destination
    database.

    Args:
        which_idnum: ID number type
        dst_session: SQLAlchemy session for the destination database

    Raises:
        :exc:`ValueError` upon failure
    """
    try:
        iddef = (
            dst_session.query(IdNumDefinition)
            .filter(IdNumDefinition.which_idnum == which_idnum)
            .one()
        )  # type: IdNumDefinition
        # ... will fail if there are 0 or >1 results
    except MultipleResultsFound:
        log.critical("Nasty bug: can't have two ID number types with the same "
                     "which_idnum! which_idnum was {!r}", which_idnum)
        raise
    except NoResultFound:
        raise ValueError(f"ID number type with which_idnum={which_idnum} "
                         f"missing from destination database")
    return iddef


def get_dst_iddef(dst_session: Session,
                  which_idnum: int) -> Optional[IdNumDefinition]:
    """
    Fetches an ID number definition from the destination database, ensuring it
    exists.

    Args:
        dst_session: destination SQLAlchemy :class:`Session`
        which_idnum: integer expressing which ID number type to look up

    Returns:
        an :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition`, or
        ``None`` if none was found

    """
    return (
        dst_session.query(IdNumDefinition)
        .filter(IdNumDefinition.which_idnum == which_idnum)
        .first()
    )


# =============================================================================
# Extra translation to be applied to individual objects
# =============================================================================
# The extra logic for this database:

def flush_session(dst_session: Session) -> None:
    """
    Flushes the destination SQLAlchemy session.
    """
    log.debug("Flushing session")
    dst_session.flush()


def ensure_default_group_id(trcon: TranslationContext) -> None:
    """
    Ensure that the :class:`TranslationContext` has a ``default_group_id``
    key in its ``info`` dictionary. This is the ID, in the destination
    database, of the group to put records in where those records come from
    an older, pre-group-based CamCOPS database.

    The user may have specified that ``default_group_id` on the command line.
    Otherwise, they may have specified a ``default_group_name``, so we'll use
    the ID of that group (creating it if necessary). If they specified neither,
    we will raise an :exc:`AssertionError`, because we have come to a
    situation where we need one or the other.

    Args:
        trcon: the :class:`TranslationContext`

    """
    default_group_id = trcon.info["default_group_id"]  # type: Optional[int]
    if default_group_id is not None:
        # The user specified a group ID to use for records without one
        assert group_exists(group_id=default_group_id,
                            dst_session=trcon.dst_session), (
            "User specified default_group_id={!r}, and object {!r} needs "
            "a _group_id (directly or indirectly), but that ID doesn't exist "
            "in the {!r} table of the destination database".format(
                default_group_id, trcon.oldobj, Group.__tablename__)
        )
    else:
        default_group_name = trcon.info["default_group_name"]  # type: Optional[str]  # noqa
        if not default_group_name:
            assert False, (
                "User specified neither default_group_id or "
                "default_group_name, but object {!r} needs a "
                "_group_id, directly or indirectly".format(trcon.oldobj)
            )
        default_group_id = fetch_group_id_by_name(
            group_name=default_group_name,
            dst_session=trcon.dst_session
        )
        trcon.info["default_group_id"] = default_group_id  # for next time!


'''
# SUPERSEDED BY MORE CONSERVATIVE MECHANISM, 2019-03-05

def ensure_idnumdef(trcon: TranslationContext,
                    which_idnum: int) -> IdNumDefinition:
    """
    Ensure that the destination database contains an ID number definition with
    the same ``which_idnum`` as in the source database, or create one.

    If an ID number definition with that ``which_idnum`` was present in the
    source and the destination, ensure they don't clash (i.e. ensure that they
    represent the same sort of ID number).

    Args:
        trcon: the :class:`TranslationContext`
        which_idnum: integer expressing which ID number type to look up

    Returns:
        the :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition`,
        attached to the destination database

    """
    dst_iddef = get_dst_iddef(trcon.dst_session, which_idnum=which_idnum)
    src_iddefs = trcon.info['src_iddefs']  # type: Dict[int, IdNumDefinition]  # noqa
    if dst_iddef:
        # Present in the destination
        if which_idnum in src_iddefs.keys():
            # Also present in the source
            src_iddef = src_iddefs[which_idnum]
            ensure_no_iddef_clash(src_iddef=src_iddef, dst_iddef=dst_iddef)
        return dst_iddef
    else:
        # Not present in the destination
        assert which_idnum in src_iddefs.keys(), (
            "Descriptions for ID#{} are missing from the source "
            "database!".format(which_idnum)
        )
        src_iddef = src_iddefs[which_idnum]
        new_iddef = IdNumDefinition(
            which_idnum=src_iddef.which_idnum,
            description=src_iddef.description,
            short_description=src_iddef.short_description
        )
        log.info("Adding ID number definition: {!r}", new_iddef)
        trcon.dst_session.add(new_iddef)
        flush_session(trcon.dst_session)  # required, or database FK checks fail  # noqa
        return new_iddef
'''


def ensure_no_iddef_clash(src_iddef: IdNumDefinition,
                          dst_iddef: IdNumDefinition) -> None:
    """
    Ensure that a given source and destination pair of ID number definitions,
    which must match on ``which_idnum``, have the same description and short
    description, or raise :exc:`ValueError`.

    Args:
        src_iddef: source
            :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition`
        dst_iddef: destination
            :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition`
    """
    assert src_iddef.which_idnum == dst_iddef.which_idnum, (
        "Bug: ensure_no_iddef_clash() called with IdNumDefinition objects"
        "that don't share the same value for which_idnum (silly!)."
    )
    if src_iddef.description != dst_iddef.description:
        raise ValueError(
            "ID description mismatch for ID#{}: source {!r}, "
            "destination {!r}".format(
                src_iddef.which_idnum,
                src_iddef.description,
                dst_iddef.description
            )
        )
    if src_iddef.short_description != dst_iddef.short_description:
        raise ValueError(
            "ID short_description mismatch for ID#{}: source {!r}, "
            "destination {!r}".format(
                src_iddef.which_idnum,
                src_iddef.short_description,
                dst_iddef.short_description
            )
        )


def log_warning_srcobj(srcobj: Any) -> None:
    """
    Prints a source (old) object to the log.

    Args:
        srcobj: the source object
    """
    log.warning("Source was:\n\n{}\n\n", pformat(srcobj.__dict__))


def get_dest_groupnum(src_groupnum: int,
                      trcon: TranslationContext,
                      oldobj: Any) -> int:
    """
    For a given source group number, returns the corresponding destination
    group number (validating en route).

    Args:
        src_groupnum: the group number in the source database
        trcon: the :class:`TranslationContext`
        oldobj: the source object

    Returns:
        the corresponding which_idnum in the destination database

    Raises:
        :exc:`ValueError` if bad
    """
    groupnum_map = trcon.info["groupnum_map"]  # type: Dict[int, int]
    if src_groupnum not in groupnum_map:
        log_warning_srcobj(oldobj)
        log.critical(
            "Old database contains group number {} and equivalent "
            "group in destination not known", src_groupnum)
        raise ValueError("Bad group mapping")
    return groupnum_map[src_groupnum]


def get_dest_which_idnum(src_which_idnum: int,
                         trcon: TranslationContext,
                         oldobj: Any) -> int:
    """
    For a given source ID number type, returns the corresponding destination
    ID number type (validating en route).

    Args:
        src_which_idnum: which_idnum in the source database
        trcon: the :class:`TranslationContext`
        oldobj: the source object

    Returns:
        the corresponding which_idnum in the destination database

    Raises:
        :exc:`ValueError` if bad

    """
    whichidnum_map = trcon.info["whichidnum_map"]  # type: Dict[int, int]
    if src_which_idnum not in whichidnum_map:
        log_warning_srcobj(oldobj)
        log.critical(
            "Old database contains ID number definitions of type {} and "
            "equivalent ID number type in destination not known",
            src_which_idnum)
        raise ValueError("Bad ID number type mapping")
    return whichidnum_map[src_which_idnum]


# noinspection PyProtectedMember
def translate_fn(trcon: TranslationContext) -> None:
    """
    Function to translate source objects to their destination counterparts,
    where special processing is required. Called as a callback from
    :func:`cardinal_pythonlib.sqlalchemy.merge_db.merge_db`.

    Args:
        trcon: the :class:`TranslationContext`; all the relevant information is
            in here, and our function modifies its members.

    This function does the following things:

    - For any records uploaded from tablets: set ``_group_id``, if it's blank.

    - For :class:`camcops_server.cc_modules.cc_user.User` objects: if an
      identical user is found in the destination database, merge on it rather
      than creating a new one. Users with matching usernames are considered to
      be identical.

    - For :class:`Device` objects: if an identical device is found, merge on it
      rather than creating a new one. Devices with matching names are
      considered to be identical.

    - For :class:`camcops_server.cc_modules.cc_group.Group` objects: if an
      identical group is found, merge on it rather than creating a new one.
      Groups with matching names are considered to be identical.

    - For :class:`camcops_server.cc_modules.cc_patient.Patient` objects: if any
      have ID numbers in the old format (as columns in the Patient table),
      convert them to the :class:`PatientIdNum` system.

    - If we're inserting a :class:`PatientIdNum`, make sure there is a
      corresponding
      :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition`, and that
      it's valid.

    - If we're merging from a more modern database with the
      :class:`camcops_server.cc_modules.cc_idnumdef.IdNumDefinition` table,
      check our ID number definitions don't conflict.

    - Check we're not creating duplicates for anything uploaded.

    """
    log.debug("Translating object from table: {!r}", trcon.tablename)
    oldobj = trcon.oldobj
    newobj = trcon.newobj
    # log.debug("Translating: {}", auto_repr(oldobj))

    # -------------------------------------------------------------------------
    # Set _group_id correctly for tablet records
    # -------------------------------------------------------------------------
    if isinstance(oldobj, GenericTabletRecordMixin):
        if ("_group_id" in trcon.missing_src_columns or
                oldobj._group_id is None):
            # ... order that "if" statement carefully; if the _group_id column
            # is missing from the source, don't touch oldobj._group_id or
            # it'll trigger a DB query that fails.
            #
            # Set _group_id because it's blank
            #
            ensure_default_group_id(trcon)
            default_group_id = trcon.info["default_group_id"]  # type: int
            log.debug("Assiging new _group_id of {!r}", default_group_id)
            newobj._group_id = default_group_id
        else:
            #
            # Re-map _group_id
            #
            newobj._group_id = get_dest_groupnum(oldobj._group_id,
                                                 trcon, oldobj)

    # -------------------------------------------------------------------------
    # If an identical user is found, merge on it rather than creating a new
    # one. Users with matching usernames are considered to be identical.
    # -------------------------------------------------------------------------
    if trcon.tablename == User.__tablename__:
        src_user = cast(User, oldobj)
        src_username = src_user.username
        matching_user = (
            trcon.dst_session.query(User)
            .filter(User.username == src_username)
            .one_or_none()
        )  # type: Optional[User]
        if matching_user is not None:
            log.debug("Matching User (username {!r}) found; merging",
                      matching_user.username)
            trcon.newobj = matching_user  # so that related records will work

    # -------------------------------------------------------------------------
    # If an identical device is found, merge on it rather than creating a
    # new one. Devices with matching names are considered to be identical.
    # -------------------------------------------------------------------------
    if trcon.tablename == Device.__tablename__:
        src_device = cast(Device, oldobj)
        src_devicename = src_device.name
        matching_device = (
            trcon.dst_session.query(Device)
            .filter(Device.name == src_devicename)
            .one_or_none()
        )  # type: Optional[Device]
        if matching_device is not None:
            log.debug("Matching Device (name {!r}) found; merging",
                      matching_device.name)
            trcon.newobj = matching_device

        # BUT BEWARE, BECAUSE IF YOU MERGE THE SAME DATABASE TWICE (even if
        # that's a silly thing to do...), MERGING DEVICES WILL BREAK THE KEY
        # RELATIONSHIPS. For example,
        #   source:
        #       pk = 1, id = 1, device = 100, era = 'NOW', current = 1
        #   dest after first merge:
        #       pk = 1, id = 1, device = 100, era = 'NOW', current = 1
        #   dest after second merge:
        #       pk = 1, id = 1, device = 100, era = 'NOW', current = 1
        #       pk = 2, id = 1, device = 100, era = 'NOW', current = 1
        # ... so you get a clash/duplicate.
        # Mind you, that's fair, because there is a duplicate.
        # SO WE DO SEPARATE DUPLICATE CHECKING, below.

    # -------------------------------------------------------------------------
    # Don't copy Group records; the user must set these up manually and specify
    # groupnum_map, for safety
    # -------------------------------------------------------------------------
    if trcon.tablename == Group.__tablename__:
        trcon.newobj = None  # don't insert this object
        # ... don't set "newobj = None"; that wouldn't alter trcon
        # Now make sure the map is OK:
        src_group = cast(Group, oldobj)
        trcon.objmap[oldobj] = get_dst_group(
            dest_groupnum=get_dest_groupnum(src_group.id, trcon, src_group),
            dst_session=trcon.dst_session
        )

    # -------------------------------------------------------------------------
    # If there are any patient numbers in the old format (as a set of
    # columns in the Patient table) which were not properly converted
    # to the new format (as individual records in the PatientIdNum
    # table), create new entries.
    # Only worth bothering with for _current entries.
    # (More explicitly: do not create new PatientIdNum entries for non-current
    # patients; it's very fiddly if there might be asynchrony between
    # Patient and PatientIdNum objects for that patient.)
    # -------------------------------------------------------------------------
    if trcon.tablename == Patient.__tablename__:
        # (a) Find old patient numbers
        old_patient = cast(Patient, oldobj)
        # noinspection PyUnresolvedReferences
        src_pt_query = (
            select([text('*')])
            .select_from(table(trcon.tablename))
            .where(column(Patient.id.name) == old_patient.id)
            .where(column(Patient._current.name) == True)  # noqa: E712
            .where(column(Patient._device_id.name) == old_patient._device_id)
            .where(column(Patient._era.name) == old_patient._era)
        )
        rows = trcon.src_session.execute(src_pt_query)  # type: ResultProxy
        list_of_dicts = [dict(row.items()) for row in rows]
        assert len(list_of_dicts) == 1, (
            "Failed to fetch old patient IDs correctly; bug?"
        )
        old_patient_dict = list_of_dicts[0]

        # (b) If any don't exist in the new database, create them.
        # -- no, that's not right; we will be processing Patient before
        # PatientIdNum, so that should be: if any don't exist in the *source*
        # database, create them.
        src_tables = trcon.src_table_names
        for src_which_idnum in range(1, NUMBER_OF_IDNUMS_DEFUNCT + 1):
            old_fieldname = FP_ID_NUM + str(src_which_idnum)
            idnum_value = old_patient_dict[old_fieldname]
            if idnum_value is None:
                # Old Patient record didn't contain this ID number
                continue
            # Old Patient record *did* contain the ID number...
            if PatientIdNum.__tablename__ in src_tables:
                # noinspection PyUnresolvedReferences
                src_idnum_query = (
                    select([func.count()])
                    .select_from(table(PatientIdNum.__tablename__))
                    .where(column(PatientIdNum.patient_id.name) ==
                           old_patient.id)
                    .where(column(PatientIdNum._current.name) ==
                           old_patient._current)
                    .where(column(PatientIdNum._device_id.name) ==
                           old_patient._device_id)
                    .where(column(PatientIdNum._era.name) ==
                           old_patient._era)
                    .where(column(PatientIdNum.which_idnum.name) ==
                           src_which_idnum)
                )
                n_present = trcon.src_session.execute(src_idnum_query).scalar()
                #                 ^^^
                #                  !
                if n_present != 0:
                    # There was already a PatientIdNum for this which_idnum
                    continue
            pidnum = PatientIdNum()
            # PatientIdNum fields:
            pidnum.id = fake_tablet_id_for_patientidnum(
                patient_id=old_patient.id, which_idnum=src_which_idnum)
            # ... guarantees a pseudo client (tablet) PK
            pidnum.patient_id = old_patient.id
            pidnum.which_idnum = get_dest_which_idnum(src_which_idnum,
                                                      trcon, oldobj)
            pidnum.idnum_value = idnum_value
            # GenericTabletRecordMixin fields:
            # _pk: autogenerated
            # noinspection PyUnresolvedReferences
            pidnum._device_id = (
                trcon.objmap[old_patient._device].id
            )
            pidnum._era = old_patient._era
            pidnum._current = old_patient._current
            pidnum._when_added_exact = old_patient._when_added_exact
            pidnum._when_added_batch_utc = old_patient._when_added_batch_utc
            # noinspection PyUnresolvedReferences
            pidnum._adding_user_id = (
                trcon.objmap[old_patient._adding_user].id
                if old_patient._adding_user is not None else None
            )
            pidnum._when_removed_exact = old_patient._when_removed_exact
            pidnum._when_removed_batch_utc = old_patient._when_removed_batch_utc  # noqa
            # noinspection PyUnresolvedReferences
            pidnum._removing_user_id = (
                trcon.objmap[old_patient._removing_user].id
                if old_patient._removing_user is not None else None
            )
            # noinspection PyUnresolvedReferences
            pidnum._preserving_user_id = (
                trcon.objmap[old_patient._preserving_user].id
                if old_patient._preserving_user is not None else None
            )
            pidnum._forcibly_preserved = old_patient._forcibly_preserved
            pidnum._predecessor_pk = None  # Impossible to calculate properly
            pidnum._successor_pk = None  # Impossible to calculate properly
            pidnum._manually_erased = old_patient._manually_erased
            pidnum._manually_erased_at = old_patient._manually_erased_at
            # noinspection PyUnresolvedReferences
            pidnum._manually_erasing_user_id = (
                trcon.objmap[old_patient._manually_erasing_user].id
                if old_patient._manually_erasing_user is not None else None
            )
            pidnum._camcops_version = old_patient._camcops_version
            pidnum._addition_pending = old_patient._addition_pending
            pidnum._removal_pending = old_patient._removal_pending
            pidnum._group_id = newobj._group_id
            # ... will have been set above if it was blank

            # OK.
            log.debug("Inserting new PatientIdNum: {}", pidnum)
            trcon.dst_session.add(pidnum)

    # -------------------------------------------------------------------------
    # If we're inserting a PatientIdNum, make sure there is a corresponding
    # IdNumDefinition, and that it's valid
    # -------------------------------------------------------------------------
    if trcon.tablename == PatientIdNum.__tablename__:
        src_pidnum = cast(PatientIdNum, oldobj)
        src_which_idnum = src_pidnum.which_idnum
        # Is it present?
        if src_which_idnum is None:
            raise ValueError(f"Bad PatientIdNum: {src_pidnum!r}")
        # Ensure the new object has an appropriate ID number FK:
        dst_pidnum = cast(PatientIdNum, newobj)
        dst_pidnum.which_idnum = get_dest_which_idnum(src_which_idnum,
                                                      trcon, oldobj)

    # -------------------------------------------------------------------------
    # If we're merging from a more modern database with the IdNumDefinition
    # table, skip source IdNumDefinition records; the user must set these up
    # manually and specify whichidnum_map, for safety
    # -------------------------------------------------------------------------
    if trcon.tablename == IdNumDefinition.__tablename__:
        trcon.newobj = None  # don't insert this object
        # ... don't set "newobj = None"; that wouldn't alter trcon
        # Now make sure the map is OK:
        src_iddef = cast(IdNumDefinition, oldobj)
        trcon.objmap[oldobj] = get_dst_iddef(
            which_idnum=get_dest_which_idnum(src_iddef.which_idnum,
                                             trcon, src_iddef),
            dst_session=trcon.dst_session
        )

    # -------------------------------------------------------------------------
    # Check we're not creating duplicates for anything uploaded
    # -------------------------------------------------------------------------
    if isinstance(oldobj, GenericTabletRecordMixin):
        # noinspection PyTypeChecker
        cls = newobj.__class__  # type: Type[GenericTabletRecordMixin]
        # Records uploaded from tablets must be unique on the combination of:
        #       id                  = table PK
        #       _device_id          = device
        #       _era                = device era
        #       _when_removed_exact = removal date or NULL
        # noinspection PyUnresolvedReferences
        exists_query = (
            select([func.count()])
            .select_from(table(trcon.tablename))
            .where(column(cls.id.name) == oldobj.id)
            .where(column(cls._device_id.name) ==
                   trcon.objmap[oldobj._device].id)
            .where(column(cls._era.name) == oldobj._era)
            .where(column(cls._when_removed_exact.name) ==
                   oldobj._when_removed_exact)
        )
        # Note re NULLs... Although it's an inconvenient truth in SQL that
        #   SELECT NULL = NULL; -- returns NULL
        # in this code we have a comparison of a column to a Python value.
        # SQLAlchemy is clever and renders "IS NULL" if the Python value is
        # None, or an "=" comparison otherwise.
        # If we were comparing a column to another column, we'd have to do
        # more; e.g.
        #
        # WRONG one-to-one join to self:
        #
        #   SELECT a._pk, b._pk, a._when_removed_exact
        #   FROM phq9 a
        #   INNER JOIN phq9 b
        #       ON a._pk = b._pk
        #       AND a._when_removed_exact = b._when_removed_exact;
        #
        #   -- drops all rows
        #
        # CORRECT one-to-one join to self:
        #
        #   SELECT a._pk, b._pk, a._when_removed_exact
        #   FROM phq9 a
        #   INNER JOIN phq9 b
        #       ON a._pk = b._pk
        #       AND (a._when_removed_exact = b._when_removed_exact
        #            OR (a._when_removed_exact IS NULL AND
        #                b._when_removed_exact IS NULL));
        #
        #   -- returns all rows
        n_exists = trcon.dst_session.execute(exists_query).scalar()
        if n_exists > 0:
            # noinspection PyUnresolvedReferences
            existing_rec_q = (
                select(['*'])
                .select_from(table(trcon.tablename))
                .where(column(cls.id.name) == oldobj.id)
                .where(column(cls._device_id.name) ==
                       trcon.objmap[oldobj._device].id)
                .where(column(cls._era.name) == oldobj._era)
                .where(column(cls._when_removed_exact.name) ==
                       oldobj._when_removed_exact)
            )
            resultproxy = trcon.dst_session.execute(existing_rec_q).fetchall()
            existing_rec = [dict(row) for row in resultproxy]
            log.critical(
                "Source record, inheriting from GenericTabletRecordMixin and "
                "shown below, already exists in destination database... "
                "in table {t!r}, clashing on: "
                "id={i!r}, device_id={d!r}, era={e!r}, "
                "_when_removed_exact={w!r}.\n"
                "ARE YOU TRYING TO MERGE THE SAME DATABASE IN TWICE? "
                "DON'T.",
                t=trcon.tablename,
                i=oldobj.id,
                d=oldobj._device_id,
                e=oldobj._era,
                w=oldobj._when_removed_exact,
            )
            if (trcon.tablename == PatientIdNum.__tablename__ and
                    (oldobj.id % NUMBER_OF_IDNUMS_DEFUNCT == 0)):
                log.critical(
                    'Since this error has occurred for table {t!r} '
                    '(and for id % {n} == 0), '
                    'this error may reflect a previous bug in the patient ID '
                    'number fix for the database upload script, in which all '
                    'ID numbers for patients with patient.id = n were given '
                    'patient_idnum.id = n * {n} themselves (or possibly were '
                    'all given patient_idnum.id = 0). '
                    'Fix this by running, on the source database:\n\n'
                    '    UPDATE patient_idnum SET id = _pk;\n\n',
                    t=trcon.tablename,
                    n=NUMBER_OF_IDNUMS_DEFUNCT,
                )
            # Print the actual instance last; accessing them via pformat can
            # lead to crashes if there are missing source fields, as an
            # on-demand SELECT is executed sometimes (e.g. when a PatientIdNum
            # is printed, its Patient is selected, including the [user]
            # 'fullname' attribute that is absent in old databases).
            # Not a breaking point, since we're going to crash anyway, but
            # inelegant.
            # Since lazy loading (etc.) is configured at query time, the best
            # thing (as per Michael Bayer) is to detach the object from the
            # session:
            # https://groups.google.com/forum/#!topic/sqlalchemy/X_wA8K97smE
            trcon.src_session.expunge(oldobj)  # prevent implicit queries
            # Then all should work:
            log_warning_srcobj(oldobj)
            log.critical(
                "Existing record(s) in destination DB was/were:\n\n{}\n\n",
                pformat(existing_rec))
            raise ValueError("Attempt to insert duplicate record; see log "
                             "message above.")


# =============================================================================
# Postprocess
# =============================================================================

# noinspection PyUnusedLocal
def postprocess(src_engine: Engine, dst_session: Session) -> None:
    """
    Implement any extra processing after :func:`merge_db` has been called.

    - Reindexes tasks.
    - Warns you about things that need to be done manually.

    Args:
        src_engine: source database SQLAlchemy engine
        dst_session: destination database SQLAlchemy session
    """
    log.info("Reindexing destination database")
    reindex_everything(dst_session)
    log.warning("NOT IMPLEMENTED AUTOMATICALLY: copying user/group mapping "
                "from table {!r}; do this by hand.",
                UserGroupMembership.__tablename__)
    log.warning("NOT IMPLEMENTED AUTOMATICALLY: copying group/group mapping "
                "from table {!r}; do this by hand.", group_group_table.name)


# =============================================================================
# Main
# =============================================================================

def merge_camcops_db(src: str,
                     echo: bool,
                     report_every: int,
                     dummy_run: bool,
                     info_only: bool,
                     default_group_id: Optional[int],
                     default_group_name: Optional[str],
                     groupnum_map: Dict[int, int],
                     whichidnum_map: Dict[int, int],
                     skip_export_logs: bool = True,
                     skip_audit_logs: bool = True) -> None:
    """
    Merge an existing database (with a pre-v2 or later structure) into a
    comtemporary CamCOPS database.

    Args:
        src:
            source database SQLAlchemy URL

        echo:
            echo the SQL that is produced?

        report_every:
            provide a progress report every *n* records

        dummy_run:
            don't alter the destination database

        info_only:
            show info, then stop

        default_group_id:
            integer group ID (in the destination database) to use for source
            records that have no group (because they come from a very old
            source database) but need one

        default_group_name:
            group name (in the destination database) to use for source
            records that have no group (because they come from a very old
            source database) but need one

        groupnum_map:
            dictionary mapping group ID values from the source database to
            the destination database

        whichidnum_map:
            dictionary mapping ``which_idnum`` values from the source database
            to the destination database

        skip_export_logs:
            skip export log tables

        skip_audit_logs:
            skip audit log table

    """
    req = get_command_line_request()  # requires manual COMMIT; see below
    src_engine = create_engine(src, echo=echo, pool_pre_ping=True)
    log.info("SOURCE: " + get_safe_url_from_engine(src_engine))
    log.info("DESTINATION: " + get_safe_url_from_engine(req.engine))
    log.info("Destination ID number type map (source:destination) is: {!r}",
             whichidnum_map)
    log.info("Group number type map (source:destination) is {!r}",
             groupnum_map)

    # Delay the slow import until we've checked our syntax
    log.info("Loading all models...")
    # noinspection PyUnresolvedReferences
    import camcops_server.cc_modules.cc_all_models  # delayed import  # import side effects (ensure all models registered)  # noqa
    log.info("Models loaded.")

    # Now, any special dependencies?
    # From the point of view of translating any tablet-related fields, the
    # actual (server) PK values are irrelevant; all relationships will be
    # identical if you change any PK (not standard database practice, but
    # convenient here).
    # The dependencies that do matter are server-side things, like user_id
    # variables.

    # For debugging only, some junk:
    # test_dependencies = [
    #     TableDependency(parent_tablename="patient",
    #                     child_tablename="_dirty_tables")
    # ]

    # -------------------------------------------------------------------------
    # Tables to skip
    # -------------------------------------------------------------------------

    skip_tables = [
        # Transient stuff we don't want to copy across, or wouldn't want to
        # overwrite the destination with, or where the PK structure has
        # changed and we don't care about old data:
        TableIdentity(tablename=x)
        for x in [
            CamcopsSession.__tablename__,
            DirtyTable.__tablename__,
            ServerSettings.__tablename__,
            SecurityAccountLockout.__tablename__,
            SecurityLoginFailure.__tablename__,
            UserGroupMembership.__tablename__,
            group_group_table.name,
        ]
    ]

    # Tedious and bulky stuff the user may want to skip:
    if skip_export_logs:
        skip_tables.extend([
            TableIdentity(tablename=x)
            for x in [
                Email.__tablename__,
                ExportRecipient.__tablename__,
                ExportedTask.__tablename__,
                ExportedTaskEmail.__tablename__,
                ExportedTaskFileGroup.__tablename__,
                ExportedTaskHL7Message.__tablename__,
            ]
        ])
    if skip_audit_logs:
        skip_tables.append(TableIdentity(tablename=AuditEntry.__tablename__))

    # -------------------------------------------------------------------------
    # Initial operations on SOURCE database
    # -------------------------------------------------------------------------

    src_tables = get_table_names(src_engine)
    skip_tables += get_skip_tables(src_tables=src_tables)
    src_iddefs = get_src_iddefs(src_engine, src_tables)
    log.info("Source ID number definitions: {!r}", src_iddefs)

    # -------------------------------------------------------------------------
    # Initial operations on DESTINATION database
    # -------------------------------------------------------------------------
    dst_session = req.dbsession
    # So that system users get the first ID (cosmetic!):
    _ = User.get_system_user(dbsession=dst_session)
    _ = Device.get_server_device(dbsession=dst_session)

    # -------------------------------------------------------------------------
    # Set up source-to-destination mappings
    # -------------------------------------------------------------------------

    # Map source to destination ID number types
    for src_which_idnum, dest_which_idnum in whichidnum_map.items():
        assert isinstance(src_which_idnum, int)
        assert isinstance(dest_which_idnum, int)
        src_iddef = src_iddefs[src_which_idnum]
        dst_iddef = ensure_dest_iddef_exists(dest_which_idnum, dst_session)
        ensure_no_iddef_clash(src_iddef, dst_iddef)

    # Map source to destination group numbers
    for src_groupnum, dest_groupnum in groupnum_map.items():
        assert isinstance(src_groupnum, int)
        assert isinstance(dest_groupnum, int)
        _ = get_dst_group(dest_groupnum, dst_session)

    # -------------------------------------------------------------------------
    # Merge
    # -------------------------------------------------------------------------

    # Merge! It's easy...
    trcon_info = dict(default_group_id=default_group_id,
                      default_group_name=default_group_name,
                      src_iddefs=src_iddefs,
                      whichidnum_map=whichidnum_map,
                      groupnum_map=groupnum_map)
    merge_db(
        base_class=Base,
        src_engine=src_engine,
        dst_session=dst_session,
        allow_missing_src_tables=True,
        allow_missing_src_columns=True,
        translate_fn=translate_fn,
        skip_tables=skip_tables,
        only_tables=None,
        tables_to_keep_pks_for=None,
        # extra_table_dependencies=test_dependencies,
        extra_table_dependencies=None,
        dummy_run=dummy_run,
        info_only=info_only,
        report_every=report_every,
        flush_per_table=True,
        flush_per_record=False,
        commit_with_flush=False,
        commit_at_end=True,
        prevent_eager_load=True,
        trcon_info=trcon_info
    )

    # -------------------------------------------------------------------------
    # Postprocess
    # -------------------------------------------------------------------------

    postprocess(src_engine=src_engine, dst_session=dst_session)

    # -------------------------------------------------------------------------
    # Done
    # -------------------------------------------------------------------------

    dst_session.commit()
