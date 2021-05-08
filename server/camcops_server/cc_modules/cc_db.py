#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_db.py

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

**Common database code, e.g. mixins for tables that are uploaded from the
client.**

"""

from collections import OrderedDict
import logging
from typing import (Any, Callable, Dict, Generator, Iterable, List, NoReturn,
                    Optional, Set, Tuple, Type, TYPE_CHECKING, TypeVar, Union)

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.sqlalchemy.orm_inspect import gen_columns
from pendulum import DateTime as Pendulum
from sqlalchemy.exc import IntegrityError
from sqlalchemy.ext.declarative import declared_attr
from sqlalchemy.orm import relationship
from sqlalchemy.orm.relationships import RelationshipProperty
from sqlalchemy.orm import Session as SqlASession
from sqlalchemy.sql.functions import func
from sqlalchemy.sql.schema import Column, ForeignKey
from sqlalchemy.sql.sqltypes import Boolean, DateTime, Integer

from camcops_server.cc_modules.cc_constants import (
    ERA_NOW,
    EXTRA_COMMENT_PREFIX,
    EXTRA_TASK_TABLENAME_FIELD,
    EXTRA_TASK_SERVER_PK_FIELD,
)
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    EraColType,
    gen_ancillary_relationships,
    gen_camcops_blob_columns,
    PendulumDateTimeAsIsoTextColType,
    PermittedValueChecker,
    RelationshipInfo,
    SemanticVersionColType,
    TableNameColType,
)
from camcops_server.cc_modules.cc_simpleobjects import TaskExportOptions
from camcops_server.cc_modules.cc_tsv import TsvPage
from camcops_server.cc_modules.cc_version import CAMCOPS_SERVER_VERSION
from camcops_server.cc_modules.cc_xml import (
    make_xml_branches_from_blobs,
    make_xml_branches_from_columns,
    make_xml_branches_from_summaries,
    XML_COMMENT_STORED,
    XML_COMMENT_CALCULATED,
    XmlElement,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_blob import Blob  # noqa: F401
    from camcops_server.cc_modules.cc_patient import Patient  # noqa: F401
    from camcops_server.cc_modules.cc_request import CamcopsRequest  # noqa: E501,F401
    from camcops_server.cc_modules.cc_summaryelement import SummaryElement  # noqa: E501,F401
    from camcops_server.cc_modules.cc_task import Task  # noqa: F401

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Hacks for specific database drivers
# =============================================================================

CRASH_ON_BAD_CONVERSIONS = False  # for debugging only!

if CRASH_ON_BAD_CONVERSIONS:
    log.error("DANGER: CRASH_ON_BAD_CONVERSIONS set in cc_db.py")

try:
    import MySQLdb
    import MySQLdb.converters
except ImportError:
    MySQLdb = None

try:
    import pymysql
    import pymysql.converters
except ImportError:
    pymysql = None

_SQL_LITERAL_TYPE = Union[int, float, str]

_MYSQL_CONVERSION_DICT_TYPE = Dict[Any, Callable]
_MYSQLDB_PYTHON_TO_DB_TYPE = Callable[[Any, _MYSQL_CONVERSION_DICT_TYPE],
                                      _SQL_LITERAL_TYPE]  # f(o, d) -> s
_MYSQLDB_DB_TO_PYTHON_TYPE = Callable[[_SQL_LITERAL_TYPE], Any]  # f(s) -> o

_PYMYSQL_ENCODER_DICT_TYPE = Dict[Type, Callable]
_PYMYSQL_PYTHON_TO_DB_TYPE = Callable[[Any, Optional[_PYMYSQL_ENCODER_DICT_TYPE]],  # noqa
                                      _SQL_LITERAL_TYPE]  # f(o, mapping) -> s
_PYMYSQL_DB_TO_PYTHON_TYPE = Callable[[_SQL_LITERAL_TYPE], Any]


def mysqldb_crash_on_bad_conversion(o: Any,
                                    d: _MYSQL_CONVERSION_DICT_TYPE) -> \
        NoReturn:
    """
    Reports a bad conversion and crashes. For debugging only (obviously)!

    **Conversions by mysqlclient (MySQLdb)**

    As per the help docstring for ``MySQLdb/converters.py``,

    - the Python-to-database conversion function has the signature ``f(o, d)``
      where ``o`` is the thing to be converted (such as a datetime.datetime)
      and ``d`` is the conversion dictionary; it returns an SQL literal value.

    - The database-to-Python conversion function has the argument ``f(s)``
      where ``s`` is a string; it returns a Python object.

    Both types of functions are stored in ``MySQLdb.converters``, which is a
    ``dict``. The keys named ``FIELD_TYPE.*`` are the database-to-Python
    converters; the others are the Python-to-database converters.

    **Conversions by pymysql**

    Similar (for back compatibility), but not the same.

    - ``pymysql.converters.conversions`` is ``pymysql.converters.decoders`` and
      contains database-to-Python converters.

    - ``pymysql.converters.encoders`` contains Python-to-database converters.

    Args:
        o: Python object
        d: MySQLdb conversion dictionary

    Returns:
        SQL literal
    """
    failmsg = (
        f"mysqldb_crash_on_bad_conversion: attempting to convert bad Python "
        f"object to database: {o!r}. Conversion dict is {d!r}."
    )
    log.critical(failmsg)
    raise RuntimeError(failmsg)


def pymysql_crash_on_bad_conversion(obj: Any,
                                    mapping: _PYMYSQL_ENCODER_DICT_TYPE) -> \
        NoReturn:
    """
    See :func:`mysqldb_crash_on_bad_conversion`.
    """
    failmsg = (
        f"pymysql_crash_on_bad_conversion: attempting to convert bad Python "
        f"object to database: {obj!r}. Mapping dict is {mapping!r}."
    )
    log.critical(failmsg)
    raise RuntimeError(failmsg)


# -----------------------------------------------------------------------------
# Pendulum; see https://pypi.org/project/pendulum/ -- but note that it says
# "pymysql.converters.conversions" but should say
# "pymysql.converters.encoders".
# -----------------------------------------------------------------------------

if MySQLdb:
    log.debug("Hacking MySQLdb to support pendulum.DateTime")
    if CRASH_ON_BAD_CONVERSIONS:
        MySQLdb.converters.conversions[Pendulum] = mysqldb_crash_on_bad_conversion  # noqa
    else:
        MySQLdb.converters.conversions[Pendulum] = MySQLdb.converters.DateTime2literal  # noqa

if pymysql:
    log.debug("Hacking pymysql to support pendulum.DateTime")
    if CRASH_ON_BAD_CONVERSIONS:
        pymysql.converters.encoders[Pendulum] = pymysql_crash_on_bad_conversion
    else:
        pymysql.converters.encoders[Pendulum] = pymysql.converters.escape_datetime  # noqa


# =============================================================================
# Constants
# =============================================================================

T = TypeVar('T')

# Database fieldname constants. Do not change. Used here and in client_api.py
FN_PK = "_pk"
FN_DEVICE_ID = "_device_id"
FN_ERA = "_era"
FN_CURRENT = "_current"
FN_WHEN_ADDED_EXACT = "_when_added_exact"
FN_WHEN_ADDED_BATCH_UTC = "_when_added_batch_utc"
FN_ADDING_USER_ID = "_adding_user_id"
FN_WHEN_REMOVED_EXACT = "_when_removed_exact"
FN_WHEN_REMOVED_BATCH_UTC = "_when_removed_batch_utc"
FN_REMOVING_USER_ID = "_removing_user_id"
FN_PRESERVING_USER_ID = "_preserving_user_id"
FN_FORCIBLY_PRESERVED = "_forcibly_preserved"
FN_PREDECESSOR_PK = "_predecessor_pk"
FN_SUCCESSOR_PK = "_successor_pk"
FN_MANUALLY_ERASED = "_manually_erased"
FN_MANUALLY_ERASED_AT = "_manually_erased_at"
FN_MANUALLY_ERASING_USER_ID = "_manually_erasing_user_id"
FN_CAMCOPS_VERSION = "_camcops_version"
FN_ADDITION_PENDING = "_addition_pending"
FN_REMOVAL_PENDING = "_removal_pending"
FN_GROUP_ID = "_group_id"

# Common fieldnames used by all tasks. Do not change.
TFN_WHEN_CREATED = "when_created"
TFN_WHEN_FIRSTEXIT = "when_firstexit"
TFN_FIRSTEXIT_IS_FINISH = "firstexit_is_finish"
TFN_FIRSTEXIT_IS_ABORT = "firstexit_is_abort"
TFN_EDITING_TIME_S = "editing_time_s"


# =============================================================================
# GenericTabletRecordMixin
# =============================================================================

# noinspection PyAttributeOutsideInit
class GenericTabletRecordMixin(object):
    """
    Mixin for all tables that are uploaded from the client, representing the
    fields that the server adds at the point of upload.

    From the server's perspective, ``_pk`` is the unique primary key.

    However, records are defined also in their tablet context, for which an
    individual tablet (defined by the combination of ``_device_id`` and
    ``_era``) sees its own PK, ``id``.
    """
    __tablename__ = None  # type: str  # sorts out some mixin type checking

    # -------------------------------------------------------------------------
    # On the server side:
    # -------------------------------------------------------------------------

    # Plain columns

    # noinspection PyMethodParameters
    @declared_attr
    def _pk(cls) -> Column:
        return Column(
            FN_PK, Integer,
            primary_key=True, autoincrement=True, index=True,
            comment="(SERVER) Primary key (on the server)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _device_id(cls) -> Column:
        return Column(
            FN_DEVICE_ID, Integer, ForeignKey("_security_devices.id",
                                              use_alter=True),
            nullable=False, index=True,
            comment="(SERVER) ID of the source tablet device"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _era(cls) -> Column:
        return Column(
            FN_ERA, EraColType,
            nullable=False, index=True,
            comment="(SERVER) 'NOW', or when this row was preserved and "
                    "removed from the source device (UTC ISO 8601)",
        )
        # ... note that _era is textual so that plain comparison
        # with "=" always works, i.e. no NULLs -- for USER comparison too, not
        # just in CamCOPS code

    # noinspection PyMethodParameters
    @declared_attr
    def _current(cls) -> Column:
        return Column(
            FN_CURRENT, Boolean,
            nullable=False, index=True,
            comment="(SERVER) Is the row current (1) or not (0)?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _when_added_exact(cls) -> Column:
        return Column(
            FN_WHEN_ADDED_EXACT, PendulumDateTimeAsIsoTextColType,
            comment="(SERVER) Date/time this row was added (ISO 8601)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _when_added_batch_utc(cls) -> Column:
        return Column(
            FN_WHEN_ADDED_BATCH_UTC, DateTime,
            comment="(SERVER) Date/time of the upload batch that added this "
                    "row (DATETIME in UTC)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _adding_user_id(cls) -> Column:
        return Column(
            FN_ADDING_USER_ID, Integer,
            ForeignKey("_security_users.id"),
            comment="(SERVER) ID of user that added this row",
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _when_removed_exact(cls) -> Column:
        return Column(
            FN_WHEN_REMOVED_EXACT, PendulumDateTimeAsIsoTextColType,
            comment="(SERVER) Date/time this row was removed, i.e. made "
                    "not current (ISO 8601)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _when_removed_batch_utc(cls) -> Column:
        return Column(
            FN_WHEN_REMOVED_BATCH_UTC, DateTime,
            comment="(SERVER) Date/time of the upload batch that removed "
                    "this row (DATETIME in UTC)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _removing_user_id(cls) -> Column:
        return Column(
            FN_REMOVING_USER_ID, Integer,
            ForeignKey("_security_users.id"),
            comment="(SERVER) ID of user that removed this row"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _preserving_user_id(cls) -> Column:
        return Column(
            FN_PRESERVING_USER_ID, Integer,
            ForeignKey("_security_users.id"),
            comment="(SERVER) ID of user that preserved this row"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _forcibly_preserved(cls) -> Column:
        return Column(
            FN_FORCIBLY_PRESERVED, Boolean, default=False,
            comment="(SERVER) Forcibly preserved by superuser (rather than "
                    "normally preserved by tablet)?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _predecessor_pk(cls) -> Column:
        return Column(
            FN_PREDECESSOR_PK, Integer,
            comment="(SERVER) PK of predecessor record, prior to modification"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _successor_pk(cls) -> Column:
        return Column(
            FN_SUCCESSOR_PK, Integer,
            comment="(SERVER) PK of successor record  (after modification) "
                    "or NULL (whilst live, or after deletion)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _manually_erased(cls) -> Column:
        return Column(
            FN_MANUALLY_ERASED, Boolean, default=False,
            comment="(SERVER) Record manually erased (content destroyed)?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _manually_erased_at(cls) -> Column:
        return Column(
            FN_MANUALLY_ERASED_AT, PendulumDateTimeAsIsoTextColType,
            comment="(SERVER) Date/time of manual erasure (ISO 8601)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _manually_erasing_user_id(cls) -> Column:
        return Column(
            FN_MANUALLY_ERASING_USER_ID, Integer,
            ForeignKey("_security_users.id"),
            comment="(SERVER) ID of user that erased this row manually"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _camcops_version(cls) -> Column:
        return Column(
            FN_CAMCOPS_VERSION, SemanticVersionColType,
            default=CAMCOPS_SERVER_VERSION,
            comment="(SERVER) CamCOPS version number of the uploading device"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _addition_pending(cls) -> Column:
        return Column(
            FN_ADDITION_PENDING, Boolean, nullable=False, default=False,
            comment="(SERVER) Addition pending?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _removal_pending(cls) -> Column:
        return Column(
            FN_REMOVAL_PENDING, Boolean, default=False,
            comment="(SERVER) Removal pending?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _group_id(cls) -> Column:
        return Column(
            FN_GROUP_ID, Integer, ForeignKey("_security_groups.id"),
            nullable=False, index=True,
            comment="(SERVER) ID of group to which this record belongs"
        )

    RESERVED_FIELDS = [  # fields that tablets can't upload
        FN_PK,
        FN_DEVICE_ID,
        FN_ERA,
        FN_CURRENT,
        FN_WHEN_ADDED_EXACT,
        FN_WHEN_ADDED_BATCH_UTC,
        FN_ADDING_USER_ID,
        FN_WHEN_REMOVED_EXACT,
        FN_WHEN_REMOVED_BATCH_UTC,
        FN_REMOVING_USER_ID,
        FN_PRESERVING_USER_ID,
        FN_FORCIBLY_PRESERVED,
        FN_PREDECESSOR_PK,
        FN_SUCCESSOR_PK,
        FN_MANUALLY_ERASED,
        FN_MANUALLY_ERASED_AT,
        FN_MANUALLY_ERASING_USER_ID,
        FN_CAMCOPS_VERSION,
        FN_ADDITION_PENDING,
        FN_REMOVAL_PENDING,
        FN_GROUP_ID,
    ]  # but more generally: they start with "_"...
    assert(all(x.startswith("_") for x in RESERVED_FIELDS))

    # -------------------------------------------------------------------------
    # Fields that *all* client tables have:
    # -------------------------------------------------------------------------

    # noinspection PyMethodParameters
    @declared_attr
    def id(cls) -> Column:
        return Column(
            "id", Integer,
            nullable=False, index=True,
            comment="(TASK) Primary key (task ID) on the tablet device"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def when_last_modified(cls) -> Column:
        return Column(
            "when_last_modified", PendulumDateTimeAsIsoTextColType,
            index=True,  # ... as used by database upload script
            comment="(STANDARD) Date/time this row was last modified on the "
                    "source tablet device (ISO 8601)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def _move_off_tablet(cls) -> Column:
        return Column(
            "_move_off_tablet", Boolean, default=False,
            comment="(SERVER/TABLET) Record-specific preservation pending?"
        )

    # -------------------------------------------------------------------------
    # Relationships
    # -------------------------------------------------------------------------

    # noinspection PyMethodParameters
    @declared_attr
    def _device(cls) -> RelationshipProperty:
        return relationship("Device")

    # noinspection PyMethodParameters
    @declared_attr
    def _adding_user(cls) -> RelationshipProperty:
        return relationship("User", foreign_keys=[cls._adding_user_id])

    # noinspection PyMethodParameters
    @declared_attr
    def _removing_user(cls) -> RelationshipProperty:
        return relationship("User", foreign_keys=[cls._removing_user_id])

    # noinspection PyMethodParameters
    @declared_attr
    def _preserving_user(cls) -> RelationshipProperty:
        return relationship("User", foreign_keys=[cls._preserving_user_id])

    # noinspection PyMethodParameters
    @declared_attr
    def _manually_erasing_user(cls) -> RelationshipProperty:
        return relationship("User",
                            foreign_keys=[cls._manually_erasing_user_id])

    # noinspection PyMethodParameters
    @declared_attr
    def _group(cls) -> RelationshipProperty:
        return relationship("Group",
                            foreign_keys=[cls._group_id])

    # -------------------------------------------------------------------------
    # Fetching attributes
    # -------------------------------------------------------------------------

    @property
    def pk(self) -> Optional[int]:
        """
        Returns the (server) primary key of this record.
        """
        return self._pk

    @property
    def era(self) -> Optional[str]:
        """
        Returns the era of this record (a text representation of the date/time
        of the point of record finalization, or ``NOW`` if the record is still
        present on the client device).
        """
        return self._era

    @property
    def device_id(self) -> Optional[int]:
        """
        Returns the client device ID of this record.
        """
        return self._device_id

    @property
    def group_id(self) -> Optional[int]:
        """
        Returns the group ID of this record.
        """
        return self._group_id

    # -------------------------------------------------------------------------
    # Autoscanning objects and their relationships
    # -------------------------------------------------------------------------

    def _get_xml_root(self,
                      req: "CamcopsRequest",
                      options: TaskExportOptions) -> XmlElement:
        """
        Called to create an XML root object for records ancillary to Task
        objects. Tasks themselves use a more complex mechanism.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            options: a :class:`camcops_server.cc_modules.cc_simpleobjects.TaskExportOptions`
        """  # noqa
        # "__tablename__" will make the type checker complain, as we're
        # defining a function for a mixin that assumes it's mixed in to a
        # SQLAlchemy Base-derived class
        # noinspection PyUnresolvedReferences
        return XmlElement(
            name=self.__tablename__,
            value=self._get_xml_branches(req=req, options=options)
        )

    def _get_xml_branches(self,
                          req: "CamcopsRequest",
                          options: TaskExportOptions) -> List[XmlElement]:
        """
        Gets the values of SQLAlchemy columns as XmlElement objects.
        Optionally, find any SQLAlchemy relationships that are relationships
        to Blob objects, and include them too.

        Used by :func:`_get_xml_root` above, but also by Tasks themselves.

        Args:
            req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            options: a :class:`camcops_server.cc_modules.cc_simpleobjects.TaskExportOptions`
        """  # noqa
        # log.debug("_get_xml_branches for {!r}", self)
        options = options or TaskExportOptions(xml_include_plain_columns=True,
                                               xml_include_calculated=True,
                                               xml_sort_by_name=True)
        branches = []  # type: List[XmlElement]
        if options.xml_with_header_comments:
            branches.append(XML_COMMENT_STORED)
        if options.xml_include_plain_columns:
            new_branches = make_xml_branches_from_columns(
                self, skip_fields=options.xml_skip_fields)
            if options.xml_sort_by_name:
                new_branches.sort(key=lambda el: el.name)
            branches += new_branches
        if options.include_blobs:
            new_branches = make_xml_branches_from_blobs(
                req, self, skip_fields=options.xml_skip_fields)
            if options.xml_sort_by_name:
                new_branches.sort(key=lambda el: el.name)
            branches += new_branches
        # Calculated
        if options.xml_include_calculated:
            if options.xml_with_header_comments:
                branches.append(XML_COMMENT_CALCULATED)
            branches.extend(make_xml_branches_from_summaries(
                self.get_summaries(req),
                skip_fields=options.xml_skip_fields,
                sort_by_name=options.xml_sort_by_name
            ))
        # log.debug("... branches for {!r}: {!r}", self, branches)
        return branches

    def _get_core_tsv_page(self, req: "CamcopsRequest",
                           heading_prefix: str = "") -> TsvPage:
        """
        Returns a single-row :class:`camcops_server.cc_modules.cc_tsv.TsvPage`,
        like an Excel "sheet", representing this record. (It may be combined
        with others later to produce a multi-row spreadsheet.)
        """
        row = OrderedDict()
        for attrname, column in gen_columns(self):
            row[heading_prefix + attrname] = getattr(self, attrname)
        for s in self.get_summaries(req):
            row[heading_prefix + s.name] = s.value
        return TsvPage(name=self.__tablename__, rows=[row])

    # -------------------------------------------------------------------------
    # Erasing (overwriting data, not deleting the database records)
    # -------------------------------------------------------------------------

    def manually_erase_with_dependants(self, req: "CamcopsRequest") -> None:
        """
        Manually erases a standard record and marks it so erased. Iterates
        through any dependants and does likewise to them.

        The object remains ``_current`` (if it was), as a placeholder, but its
        contents are wiped.

        WRITES TO THE DATABASE.
        """
        if self._manually_erased or self._pk is None or self._era == ERA_NOW:
            # ... _manually_erased: don't do it twice
            # ... _pk: basic sanity check
            # ... _era: don't erase things that are current on the tablet
            return
        # 1. "Erase my dependants"
        for ancillary in self.gen_ancillary_instances_even_noncurrent():
            ancillary.manually_erase_with_dependants(req)
        for blob in self.gen_blobs_even_noncurrent():
            blob.manually_erase_with_dependants(req)
        # 2. "Erase me"
        erasure_attrs = []  # type: List[str]
        for attrname, column in gen_columns(self):
            if attrname.startswith("_"):  # system field
                continue
            if not column.nullable:  # this should cover FKs
                continue
            if column.foreign_keys:  # ... but to be sure...
                continue
            erasure_attrs.append(attrname)
        for attrname in erasure_attrs:
            setattr(self, attrname, None)
        self._current = False
        self._manually_erased = True
        self._manually_erased_at = req.now
        self._manually_erasing_user_id = req.user_id

    def delete_with_dependants(self, req: "CamcopsRequest") -> None:
        """
        Deletes (completely from the database) this record and any
        dependant records.
        """
        if self._pk is None:
            return
        # 1. "Delete my dependants"
        for ancillary in self.gen_ancillary_instances_even_noncurrent():
            ancillary.delete_with_dependants(req)
        for blob in self.gen_blobs_even_noncurrent():
            blob.delete_with_dependants(req)
        # 2. "Delete me"
        dbsession = SqlASession.object_session(self)
        dbsession.delete(self)

    def gen_attrname_ancillary_pairs(self) \
            -> Generator[Tuple[str, "GenericTabletRecordMixin"], None, None]:
        """
        Iterates through and yields all ``_current`` "ancillary" objects
        (typically: records of subtables).

        Yields tuples of ``(attrname, related_record)``.
        """
        for attrname, rel_prop, rel_cls in gen_ancillary_relationships(self):
            if rel_prop.uselist:
                ancillaries = getattr(self, attrname)  # type: List[GenericTabletRecordMixin]  # noqa
            else:
                ancillaries = [getattr(self, attrname)]  # type: List[GenericTabletRecordMixin]  # noqa
            for ancillary in ancillaries:
                if ancillary is None:
                    continue
                yield attrname, ancillary

    def gen_ancillary_instances(self) -> Generator["GenericTabletRecordMixin",
                                                   None, None]:
        """
        Generates all ``_current`` ancillary objects of this object.
        """
        for attrname, ancillary in self.gen_attrname_ancillary_pairs():
            yield ancillary

    def gen_ancillary_instances_even_noncurrent(self) \
            -> Generator["GenericTabletRecordMixin", None, None]:
        """
        Generates all ancillary objects of this object, even non-current
        ones.
        """
        for lineage_member in self._gen_unique_lineage_objects(
                self.gen_ancillary_instances()):
            yield lineage_member

    def gen_blobs(self) -> Generator["Blob", None, None]:
        """
        Generate all ``_current`` BLOBs owned by this object.
        """
        for id_attrname, column in gen_camcops_blob_columns(self):
            relationship_attr = column.blob_relationship_attr_name
            blob = getattr(self, relationship_attr)
            if blob is None:
                continue
            yield blob

    def gen_blobs_even_noncurrent(self) -> Generator["Blob", None, None]:
        """
        Generates all BLOBs owned by this object, even non-current ones.
        """
        for lineage_member in self._gen_unique_lineage_objects(
                self.gen_blobs()):  # type: "Blob"
            yield lineage_member

    def get_lineage(self) -> List["GenericTabletRecordMixin"]:
        """
        Returns all records that are part of the same "lineage", that is:

        - of the same class;
        - matching on id/device_id/era;
        - including both current and any historical non-current versions.

        Will include the "self" object.

        """
        dbsession = SqlASession.object_session(self)
        cls = self.__class__
        q = (
            dbsession.query(cls)
            .filter(cls.id == self.id)
            .filter(cls._device_id == self._device_id)
            .filter(cls._era == self._era)
        )
        return list(q)

    @staticmethod
    def _gen_unique_lineage_objects(
            collection: Iterable["GenericTabletRecordMixin"]) \
            -> Generator["GenericTabletRecordMixin", None, None]:
        """
        Given an iterable of database records, generate all related lineage
        objects for each of them (via :meth:`get_lineage`) that are unique by
        PK.
        """
        seen_pks = set()  # type: Set[int]
        for item in collection:
            if item is None:
                continue
            for lineage_member in item.get_lineage():
                pk = lineage_member.pk
                if pk in seen_pks:
                    continue
                seen_pks.add(pk)
                yield lineage_member

    # -------------------------------------------------------------------------
    # Retrieving a linked record by client ID
    # -------------------------------------------------------------------------

    @classmethod
    def get_linked(cls, client_id: Optional[int],
                   other: "GenericTabletRecordMixin") \
            -> Optional["GenericTabletRecordMixin"]:
        """
        Returns a specific linked record, of the class of ``self``, whose
        client-side ID is ``client_id``, and which matches ``other`` in terms
        of device/era.
        """
        if client_id is None:
            return None
        dbsession = SqlASession.object_session(other)
        # noinspection PyPep8
        q = (
            dbsession.query(cls)
            .filter(cls.id == client_id)
            .filter(cls._device_id == other._device_id)
            .filter(cls._era == other._era)
            .filter(cls._current == True)  # noqa: E712
        )
        return q.first()

    # -------------------------------------------------------------------------
    # History functions for server-side editing
    # -------------------------------------------------------------------------

    def set_predecessor(self, req: "CamcopsRequest",
                        predecessor: "GenericTabletRecordMixin") -> None:
        """
        Used for some unusual server-side manipulations (e.g. editing patient
        details).

        Amends this object so the "self" object replaces the predecessor, so:

        - "self" becomes current and refers back to "predecessor";
        - "predecessor" becomes non-current and refers forward to "self".

        """
        assert predecessor._current
        # We become new and current, and refer to our predecessor
        self._device_id = predecessor._device_id
        self._era = predecessor._era
        self._current = True
        self._when_added_exact = req.now
        self._when_added_batch_utc = req.now_utc
        self._adding_user_id = req.user_id
        if self._era != ERA_NOW:
            self._preserving_user_id = req.user_id
            self._forcibly_preserved = True
        self._predecessor_pk = predecessor._pk
        self._camcops_version = predecessor._camcops_version
        self._group_id = predecessor._group_id
        # Make our predecessor refer to us
        if self._pk is None:
            req.dbsession.add(self)  # ensure we have a PK, part 1
            req.dbsession.flush()  # ensure we have a PK, part 2
        predecessor._set_successor(req, self)

    def _set_successor(self, req: "CamcopsRequest",
                       successor: "GenericTabletRecordMixin") -> None:
        """
        See :func:`set_predecessor` above.
        """
        assert successor._pk is not None
        self._current = False
        self._when_removed_exact = req.now
        self._when_removed_batch_utc = req.now_utc
        self._removing_user_id = req.user_id
        self._successor_pk = successor._pk

    def mark_as_deleted(self, req: "CamcopsRequest") -> None:
        """
        Ends the history chain and marks this record as non-current.
        """
        if self._current:
            self._when_removed_exact = req.now
            self._when_removed_batch_utc = req.now_utc
            self._removing_user_id = req.user_id
            self._current = False

    def create_fresh(self, req: "CamcopsRequest", device_id: int,
                     era: str, group_id: int) -> None:
        """
        Used to create a record from scratch.
        """
        self._device_id = device_id
        self._era = era
        self._group_id = group_id
        self._current = True
        self._when_added_exact = req.now
        self._when_added_batch_utc = req.now_utc
        self._adding_user_id = req.user_id

    def save_with_next_available_id(self, req: "CamcopsRequest",
                                    device_id: int,
                                    era: str = ERA_NOW) -> None:
        """
        Save a record with the next available client pk in sequence.
        This is of use when creating patients and ID numbers on the server
        to ensure uniqueness, or when fixing up a missing ID number for
        a patient created on a device.
        """
        cls = self.__class__

        saved_ok = False

        # MySql doesn't support "select for update" so we have to keep
        # trying the next available ID and checking for an integrity
        # error in case another user has grabbed it by the time we have
        # committed
        # noinspection PyProtectedMember
        last_id = (
            req.dbsession
            # func.max(cls.id) + 1 here will do the right thing for
            # backends that support select for update (maybe not for no rows)
            .query(func.max(cls.id))
            .filter(cls._device_id == device_id)
            .filter(cls._era == era)
            .scalar()
        ) or 0

        next_id = last_id + 1

        while not saved_ok:
            self.id = next_id

            req.dbsession.add(self)

            try:
                req.dbsession.flush()
                saved_ok = True
            except IntegrityError:
                req.dbsession.rollback()
                next_id += 1

    # -------------------------------------------------------------------------
    # Override this if you provide summaries
    # -------------------------------------------------------------------------

    # noinspection PyMethodMayBeStatic
    def get_summaries(self, req: "CamcopsRequest") -> List["SummaryElement"]:
        """
        Return a list of :class:`SummaryElement` objects, for this database
        object (not any dependent classes/tables).

        Note that this is implemented on :class:`GenericTabletRecordMixin`,
        not :class:`camcops_server.cc_modules.cc_task.Task`, so that ancillary
        objects can also provide summaries.
        """
        return []

    def get_summary_names(self, req: "CamcopsRequest") -> List[str]:
        """
        Returns a list of summary field names.
        """
        return [x.name for x in self.get_summaries(req)]


# =============================================================================
# Relationships
# =============================================================================

def ancillary_relationship(
        parent_class_name: str,
        ancillary_class_name: str,
        ancillary_fk_to_parent_attr_name: str,
        ancillary_order_by_attr_name: str = None,
        read_only: bool = True) -> RelationshipProperty:
    """
    Implements a one-to-many relationship, i.e. one parent to many ancillaries.
    """
    parent_pk_attr_name = "id"  # always
    return relationship(
        ancillary_class_name,
        primaryjoin=(
            "and_("
            " remote({a}.{fk}) == foreign({p}.{pk}), "
            " remote({a}._device_id) == foreign({p}._device_id), "
            " remote({a}._era) == foreign({p}._era), "
            " remote({a}._current) == True "
            ")".format(
                a=ancillary_class_name,
                fk=ancillary_fk_to_parent_attr_name,
                p=parent_class_name,
                pk=parent_pk_attr_name,
            )
        ),
        uselist=True,
        order_by="{a}.{f}".format(a=ancillary_class_name,
                                  f=ancillary_order_by_attr_name),
        viewonly=read_only,
        info={
            RelationshipInfo.IS_ANCILLARY: True,
        },
        # ... "info" is a user-defined dictionary; see
        # http://docs.sqlalchemy.org/en/latest/orm/relationship_api.html#sqlalchemy.orm.relationship.params.info  # noqa
        # http://docs.sqlalchemy.org/en/latest/orm/internals.html#MapperProperty.info  # noqa
    )


# =============================================================================
# Field creation assistance
# =============================================================================

# TypeEngineBase = TypeVar('TypeEngineBase', bound=TypeEngine)

def add_multiple_columns(
        cls: Type,
        prefix: str,
        start: int,
        end: int,
        coltype=Integer,
        # this type fails: Union[Type[TypeEngineBase], TypeEngine]
        # ... https://stackoverflow.com/questions/38106227
        # ... https://github.com/python/typing/issues/266
        colkwargs: Dict[str, Any] = None,
        comment_fmt: str = None,
        comment_strings: List[str] = None,
        minimum: Union[int, float] = None,
        maximum: Union[int, float] = None,
        pv: List[Any] = None,
        suffix: str = "") -> None:
    """
    Add a sequence of SQLAlchemy columns to a class.

    Called from a metaclass.
    Used to make task creation a bit easier.

    Args:
        cls:
            class to which to add columns
        prefix:
            Fieldname will be ``prefix + str(n) + suffix``, where ``n`` is
            defined as below.
        suffix:
            Optional. See ``prefix``.
        start:
            Start of range.
        end:
            End of range. Thus: ``i`` will range from ``0`` to ``(end -
            start)`` inclusive; ``n`` will range from ``start`` to ``end``
            inclusive.
        coltype:
             SQLAlchemy column type, in either of these formats: (a)
             ``Integer`` (of general type ``Type[TypeEngine]``?); (b)
             ``Integer()`` (of general type ``TypeEngine``).
        colkwargs:
            SQLAlchemy column arguments, as in
            ``Column(name, coltype, **colkwargs)``
        comment_fmt:
            Format string defining field comments. Substitutable
            values are:

            - ``{n}``: field number (from range).
            - ``{s}``: ``comment_strings[i]``, where ``i`` is a zero-based
              index as defined as above, or "" if out of range.

        comment_strings:
            see ``comment_fmt``
        minimum:
            minimum permitted value, or ``None``
        maximum:
            maximum permitted value, or ``None``
        pv:
            list of permitted values, or ``None``
    """
    colkwargs = {} if colkwargs is None else colkwargs  # type: Dict[str, Any]
    comment_strings = comment_strings or []
    for n in range(start, end + 1):
        nstr = str(n)
        i = n - start
        colname = prefix + nstr + suffix
        if comment_fmt:
            s = ""
            if 0 <= i < len(comment_strings):
                s = comment_strings[i] or ""
            colkwargs["comment"] = comment_fmt.format(n=n, s=s)
        if minimum is not None or maximum is not None or pv is not None:
            colkwargs["permitted_value_checker"] = PermittedValueChecker(
                minimum=minimum,
                maximum=maximum,
                permitted_values=pv
            )
            setattr(cls, colname, CamcopsColumn(colname, coltype, **colkwargs))
        else:
            setattr(cls, colname, Column(colname, coltype, **colkwargs))


# =============================================================================
# TaskDescendant
# =============================================================================

class TaskDescendant(object):
    """
    Information mixin for sub-tables that can be traced back to a class. Used
    to denormalize the database for export in some circumstances.

    Not used for the Blob class, which has no reasonable way of tracing itself
    back to a given task if it is used by a task's ancillary tables rather than
    a primary task row.
    """

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        """
        Returns the class of the ancestral task.

        If the descendant can descend from lots of types of task (rare; only
        applies to :class:`camcops_server.cc_modules.cc_blob.Blob` and
        :class:`camcops_server.cc_modules.cc_summaryelement.ExtraSummaryTable`),
        returns ``None``.
        """  # noqa
        raise NotImplementedError

    @classmethod
    def task_ancestor_might_have_patient(cls) -> bool:
        """
        Does this object have a single task ancestor, that is not anonymous?
        """
        taskcls = cls.task_ancestor_class()
        if not taskcls:
            return True  # e.g. Blob, ExtraSummaryTable
        return not taskcls.is_anonymous

    def task_ancestor_server_pk(self) -> Optional[int]:
        """
        Returns the server PK of the ancestral task.

        Note that this is an export-time calculation; the client may update its
        task rows without updating its descendant rows (so server PKs change
        whilst client IDs don't).
        """
        task = self.task_ancestor()
        if not task:
            return None
        return task.pk

    def task_ancestor(self) -> Optional["Task"]:
        """
        Returns the specific ancestor task of this object.
        """
        raise NotImplementedError

    def task_ancestor_patient(self) -> Optional["Patient"]:
        """
        Returns the associated patient, if there is one.
        """
        task = self.task_ancestor()
        return task.patient if task else None

    @classmethod
    def extra_task_xref_columns(cls) -> List[Column]:
        """
        Returns extra columns used to cross-reference this
        :class:`TaskDescendant` to its ancestor task, in certain export
        formats (``DB_PATIENT_ID_PER_ROW``).
        """
        return [
            Column(
                EXTRA_TASK_TABLENAME_FIELD, TableNameColType,
                comment=EXTRA_COMMENT_PREFIX + "Table name of ancestor task"
            ),
            Column(
                EXTRA_TASK_SERVER_PK_FIELD, Integer,
                comment=EXTRA_COMMENT_PREFIX + "Server PK of ancestor task"
            ),
        ]

    def add_extra_task_xref_info_to_row(self, row: Dict[str, Any]) -> None:
        """
        For the ``DB_PATIENT_ID_PER_ROW`` export option. Adds additional
        cross-referencing info to a row.

        Args:
            row: future database row, as a dictionary
        """
        ancestor = self.task_ancestor()
        if ancestor:
            row[EXTRA_TASK_TABLENAME_FIELD] = ancestor.tablename
            row[EXTRA_TASK_SERVER_PK_FIELD] = ancestor.pk
