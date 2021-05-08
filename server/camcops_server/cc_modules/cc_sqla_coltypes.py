#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_sqla_coltypes.py

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

**SQLAlchemy column types used by CamCOPS.**

Note these built-in SQLAlchemy types
(https://docs.sqlalchemy.org/en/latest/core/type_basics.html#generic-types):

    =============== ===========================================================
    SQLAlchemy type Comment
    =============== ===========================================================
    BigInteger      MySQL: -9,223,372,036,854,775,808 to
                    9,223,372,036,854,775,807 (64-bit)
                    (compare NHS number: up to 9,999,999,999)
    Boolean
    Date
    DateTime
    Enum
    Float
    Integer         MySQL: -2,147,483,648 to 2,147,483,647 (32-bit)
    Interval        For ``datetime.timedelta``
    LargeBinary     Under MySQL, maps to ``BLOB``
    MatchType       For the return type of the ``MATCH`` operator
    Numeric         For fixed-precision numbers like ``NUMERIC`` or ``DECIMAL``
    PickleType
    SchemaType
    SmallInteger
    String          ``VARCHAR``
    Text            Variably sized string type.
                    (Under MySQL, renders as ``TEXT``.)
    Time
    Unicode         Implies that the underlying column explicitly supports
                    Unicode
    UnicodeText     Variably sized version of Unicode
                    (Under MySQL, renders as ``TEXT`` too.)
    =============== ===========================================================

Not supported across all platforms:

    =============== ===========================================================
    SQL type        Comment
    =============== ===========================================================
    BIGINT UNSIGNED MySQL: 0 to 18,446,744,073,709,551,615 (64-bit).
                    Use ``sqlalchemy.dialects.mysql.BIGINT(unsigned=True)``.
    INT UNSIGNED    MySQL: 0 to 4,294,967,295 (32-bit).
                    Use ``sqlalchemy.dialects.mysql.INTEGER(unsigned=True)``.
    =============== ===========================================================

Other MySQL sizes:

    =============== ===========================================================
    MySQL type      Comment
    =============== ===========================================================
    TINYBLOB        2^8 bytes = 256 bytes
    BLOB            2^16 bytes = 64 KiB
    MEDIUMBLOB      2^24 bytes = 16 MiB
    LONGBLOB        2^32 bytes = 4 GiB
    TINYTEXT        255 (2^8 - 1) bytes
    TEXT            65,535 bytes (2^16 - 1) = 64 KiB
    MEDIUMTEXT      16,777,215 (2^24 - 1) bytes = 16 MiB
    LONGTEXT        4,294,967,295 (2^32 - 1) bytes = 4 GiB
    =============== ===========================================================

See https://stackoverflow.com/questions/13932750/tinytext-text-mediumtext-and-longtext-maximum-storage-sizes.

Also notes:

- Columns may need their character set specified explicitly under MySQL:
  https://stackoverflow.com/questions/2108824/mysql-incorrect-string-value-error-when-save-unicode-string-in-django

"""  # noqa

# =============================================================================
# Imports
# =============================================================================

import json
import logging
from typing import (Any, Generator, List, Optional, Tuple, Type, TYPE_CHECKING,
                    Union)
import uuid

from cardinal_pythonlib.datetimefunc import (
    coerce_to_pendulum,
    convert_datetime_to_utc,
    duration_from_iso,
    duration_to_iso,
    PotentialDatetimeType,
)
from cardinal_pythonlib.lists import chunks
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
)
from cardinal_pythonlib.reprfunc import auto_repr
from cardinal_pythonlib.sqlalchemy.dialect import SqlaDialectName
from cardinal_pythonlib.sqlalchemy.orm_inspect import (
    gen_columns,
    gen_relationships,
)
from cardinal_pythonlib.sqlalchemy.sqlfunc import (
    fail_unknown_dialect,
    fetch_processed_single_clause
)
from isodate.isoerror import ISO8601Error
from pendulum import DateTime as Pendulum, Duration
from pendulum.parsing.exceptions import ParserError
from semantic_version import Version
from sqlalchemy import util
from sqlalchemy.dialects import mysql
from sqlalchemy.engine.interfaces import Dialect
from sqlalchemy.ext.compiler import compiles
from sqlalchemy.orm.relationships import RelationshipProperty
from sqlalchemy.sql.elements import conv
from sqlalchemy.sql.expression import text
from sqlalchemy.sql.functions import FunctionElement
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import (
    Boolean,
    CHAR,
    DateTime,
    LargeBinary,
    String,
    Text,
    Unicode,
    UnicodeText,
)
from sqlalchemy.sql.type_api import TypeDecorator

from camcops_server.cc_modules.cc_constants import PV, StringLengths
from camcops_server.cc_modules.cc_simpleobjects import IdNumReference
from camcops_server.cc_modules.cc_sqlalchemy import (
    LONG_COLUMN_NAME_WARNING_LIMIT,
)
from camcops_server.cc_modules.cc_version import make_version

if TYPE_CHECKING:
    from sqlalchemy.sql.elements import ClauseElement  # noqa: F401
    from sqlalchemy.sql.compiler import SQLCompiler  # noqa: F401
    from camcops_server.cc_modules.cc_db import GenericTabletRecordMixin  # noqa: E501,F401

log = BraceStyleAdapter(logging.getLogger(__name__))

# =============================================================================
# Debugging options
# =============================================================================

DEBUG_DATETIME_AS_ISO_TEXT = False
DEBUG_DURATION_AS_ISO_TEXT = False
DEBUG_IDNUMDEF_LIST = False
DEBUG_INT_LIST_COLTYPE = False
DEBUG_SEMANTIC_VERSION = False
DEBUG_STRING_LIST_COLTYPE = False

if any([DEBUG_DATETIME_AS_ISO_TEXT,
        DEBUG_DURATION_AS_ISO_TEXT,
        DEBUG_SEMANTIC_VERSION,
        DEBUG_IDNUMDEF_LIST,
        DEBUG_INT_LIST_COLTYPE,
        DEBUG_STRING_LIST_COLTYPE]):
    log.warning("Debugging options enabled!")

# =============================================================================
# Constants
# =============================================================================


class RelationshipInfo(object):
    """
    Used as keys the ``info`` (user-defined) dictionary parameter to SQLAlchemy
    ``relationship`` calls; see
    https://docs.sqlalchemy.org/en/latest/orm/relationship_api.html#sqlalchemy.orm.relationship.
    """  # noqa
    IS_ANCILLARY = "is_ancillary"
    IS_BLOB = "is_blob"


# =============================================================================
# Simple derivative column types
# =============================================================================
# If you insert something too long into a VARCHAR, it just gets truncated.

AuditSourceColType = String(length=StringLengths.AUDIT_SOURCE_MAX_LEN)

# BigIntUnsigned = Integer().with_variant(mysql.BIGINT(unsigned=True), 'mysql')
# ... partly because Alembic breaks on variants (Aug 2017), and partly because
#     it's nonstandard and unnecessary, changed all BigIntUnsigned to
#     BigInteger (2017-08-25).

CharColType = String(length=1)
CharsetColType = String(length=StringLengths.CHARSET_MAX_LEN)
CurrencyColType = Unicode(length=StringLengths.CURRENCY_MAX_LEN)

DatabaseTitleColType = Unicode(length=StringLengths.DATABASE_TITLE_MAX_LEN)
DeviceNameColType = String(length=StringLengths.DEVICE_NAME_MAX_LEN)
DiagnosticCodeColType = String(length=StringLengths.DIAGNOSTIC_CODE_MAX_LEN)

EmailAddressColType = Unicode(length=StringLengths.EMAIL_ADDRESS_MAX_LEN)
EraColType = String(length=StringLengths.ISO8601_DATETIME_STRING_MAX_LEN)
ExportRecipientNameColType = String(
    length=StringLengths.EXPORT_RECIPIENT_NAME_MAX_LEN)
ExportTransmissionMethodColType = String(
    length=StringLengths.SENDING_FORMAT_MAX_LEN)

FilterTextColType = Unicode(length=StringLengths.FILTER_TEXT_MAX_LEN)
FileSpecColType = Unicode(length=StringLengths.FILESPEC_MAX_LEN)
FullNameColType = Unicode(length=StringLengths.FULLNAME_MAX_LEN)

GroupDescriptionColType = Unicode(
    length=StringLengths.GROUP_DESCRIPTION_MAX_LEN)
GroupNameColType = Unicode(length=StringLengths.GROUP_NAME_MAX_LEN)

HashedPasswordColType = String(length=StringLengths.HASHED_PW_MAX_LEN)
# ... You might think that we must ensure case-SENSITIVE comparison on this
# field. That would require the option collation='utf8mb4_bin' to String(),
# for MySQL. However, that is MySQL-specific, and SQLAlchemy currently (Oct
# 2017) doesn't support database-specific *per-column* collations. SQLite
# accepts COLLATE commands but chokes on 'utf8mb4_bin'. Now, the hashed
# password from bcrypt() is case-sensitive. HOWEVER, the important thing is
# that we always retrieve the string from the database and do a case-sensitive
# comparison in Python (see calls to is_password_valid()). So the database
# collation doesn't matter. So we don't set it.
# See further notes in cc_sqlalchemy.py
HL7AssigningAuthorityType = String(length=StringLengths.HL7_AA_MAX_LEN)
HL7IdTypeType = String(length=StringLengths.HL7_ID_TYPE_MAX_LEN)
HostnameColType = String(length=StringLengths.HOSTNAME_MAX_LEN)

IdDescriptorColType = Unicode(length=StringLengths.ID_DESCRIPTOR_MAX_LEN)
IdPolicyColType = String(length=StringLengths.ID_POLICY_MAX_LEN)
# IntUnsigned = Integer().with_variant(mysql.INTEGER(unsigned=True), 'mysql')
IPAddressColType = String(length=StringLengths.IP_ADDRESS_MAX_LEN)
# This is a plain string.
# See also e.g. http://sqlalchemy-utils.readthedocs.io/en/latest/_modules/sqlalchemy_utils/types/ip_address.html  # noqa

LanguageCodeColType = String(length=StringLengths.LANGUAGE_CODE_MAX_LEN)

# Large BLOB:
# https://stackoverflow.com/questions/43791725/sqlalchemy-how-to-make-a-longblob-column-in-mysql  # noqa
# One of these:
# noinspection PyTypeChecker
LongBlob = LargeBinary().with_variant(mysql.LONGBLOB, "mysql")
# LongBlob = LargeBinary(length=LONGBLOB_LONGTEXT_MAX_LEN)  # doesn't translate to SQL Server  # noqa

# noinspection PyTypeChecker
LongText = UnicodeText().with_variant(mysql.LONGTEXT, "mysql")
# LongText = UnicodeText(length=LONGBLOB_LONGTEXT_MAX_LEN)  # doesn't translate to SQL Server  # noqa

MimeTypeColType = String(length=StringLengths.MIMETYPE_MAX_LEN)

PatientNameColType = Unicode(length=StringLengths.PATIENT_NAME_MAX_LEN)

Rfc2822DateColType = String(length=StringLengths.RFC_2822_DATE_MAX_LEN)

SessionTokenColType = String(length=StringLengths.SESSION_TOKEN_MAX_LEN)
SexColType = String(length=1)
SummaryCategoryColType = String(
    length=StringLengths.TASK_SUMMARY_TEXT_FIELD_DEFAULT_MAX_LEN)
# ... pretty generic

TableNameColType = String(length=StringLengths.TABLENAME_MAX_LEN)

UrlColType = String(length=StringLengths.URL_MAX_LEN)
UserNameCamcopsColType = String(length=StringLengths.USERNAME_CAMCOPS_MAX_LEN)
UserNameExternalColType = String(
    length=StringLengths.USERNAME_EXTERNAL_MAX_LEN)


# =============================================================================
# Helper operations for PendulumDateTimeAsIsoTextColType
# =============================================================================
# Database string format is e.g.
#   2013-07-24T20:04:07.123456+01:00
#   2013-07-24T20:04:07.123+01:00
#   0        1         2         3      } position in string; 1-based
#   12345678901234567890123456789012    }
#
# So: rightmost 6 characters are time zone; rest is date/time.
#     leftmost 23 characters are time up to millisecond precision.
#     overall length is typically 29 (milliseconds) or 32 (microseconds)

_TZ_LEN = 6  # length of the timezone part of the ISO8601 string
_UTC_TZ_LITERAL = "'+00:00'"
_SQLITE_DATETIME_FMT_FOR_PYTHON = "'%Y-%m-%d %H:%M:%f'"

_MYSQL_DATETIME_LEN = 19
_SQLSERVER_DATETIME_LEN = 19
_SQLSERVER_DATETIME2_LEN = 27


# -----------------------------------------------------------------------------
# isotzdatetime_to_utcdatetime
# -----------------------------------------------------------------------------

# noinspection PyPep8Naming
class isotzdatetime_to_utcdatetime(FunctionElement):
    """
    Used as an SQL operation by :class:`PendulumDateTimeAsIsoTextColType`.

    Creates an SQL expression wrapping a field containing our ISO-8601 text,
    making a ``DATETIME`` out of it, in the UTC timezone.

    Implemented for different SQL dialects.
    """
    type = DateTime()
    name = 'isotzdatetime_to_utcdatetime'


# noinspection PyUnusedLocal
@compiles(isotzdatetime_to_utcdatetime)
def isotzdatetime_to_utcdatetime_default(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> None:
    """
    Default implementation for :class:`isotzdatetime_to_utcdatetime`: fail.
    """
    fail_unknown_dialect(compiler, "perform isotzdatetime_to_utcdatetime")


# noinspection PyUnusedLocal
@compiles(isotzdatetime_to_utcdatetime, SqlaDialectName.MYSQL)
def isotzdatetime_to_utcdatetime_mysql(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> str:
    """
    Implementation of :class:`isotzdatetime_to_utcdatetime` for MySQL.

    For format, see
    https://dev.mysql.com/doc/refman/5.5/en/date-and-time-functions.html#function_date-format

    Note the use of "%i" for minutes.

    Things after ``func.`` get passed to the database engine as literal SQL
    functions; https://docs.sqlalchemy.org/en/latest/core/tutorial.html
    """  # noqa
    x = fetch_processed_single_clause(element, compiler)

    # Let's do this in a clear way:
    date_time_part = f"LEFT({x}, LENGTH({x}) - {_TZ_LEN})"
    # ... drop the rightmost 6 chars (the timezone component)
    fmt = compiler.process(text("'%Y-%m-%dT%H:%i:%S.%f'"))
    # ... the text() part deals with the necessary escaping of % for the DBAPI
    the_date_time = f"STR_TO_DATE({date_time_part}, {fmt})"
    # ... STR_TO_DATE() returns a DATETIME if the string contains both date and
    #     time components.
    old_timezone = f"RIGHT({x}, {_TZ_LEN})"
    result_utc = (
        f"CONVERT_TZ({the_date_time}, {old_timezone}, {_UTC_TZ_LITERAL})"
    )

    # log.warning(result_utc)
    return result_utc


# noinspection PyUnusedLocal
@compiles(isotzdatetime_to_utcdatetime, SqlaDialectName.SQLITE)
def isotzdatetime_to_utcdatetime_sqlite(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> str:
    """
    Implementation of :class:`isotzdatetime_to_utcdatetime` for SQLite.

    - https://sqlite.org/lang_corefunc.html#substr
    - https://sqlite.org/lang_datefunc.html
    - https://www.sqlite.org/lang_expr.html

    Get an SQL expression for the timezone adjustment in hours.
    Note that if a time is 12:00+01:00, that means e.g. midday BST, which
    is 11:00+00:00 or 11:00 UTC. So you SUBTRACT the displayed timezone from
    the time, which I've always thought is a bit odd.

    Ha! Was busy implementing this, but SQLite is magic; if there's a
    timezone at the end, ``STRFTIME()`` will convert it to UTC automatically!
    Moreover, the format is the OUTPUT format that a Python datetime will
    recognize, so no 'T'.

    The output format is like this: ``2018-06-01 00:00:00.000``. Note that
    SQLite provides millisecond precision only (in general and via the ``%f``
    argument to ``STRFTIME``).

    See also SQLAlchemy's DATETIME support for SQLite:

    - https://docs.sqlalchemy.org/en/13/dialects/sqlite.html?highlight=sqlite#sqlalchemy.dialects.sqlite.DATETIME

    ... but that doesn't support timezones, so that doesn't help us.

    One further problem -- see
    :class:`camcops_server.tasks.core10.Core10ReportDateRangeTests` -- is that
    comparisons are done by SQLite as text, so e.g.

    .. code-block:: sql

        SELECT '2018-06-01 00:00:00.000' >= '2018-06-01 00:00:00.000000';  -- 0, false
        SELECT '2018-06-01 00:00:00.000' >= '2018-06-01 00:00:00.000';  -- 1, true

    and therefore we need to ensure either that the SQLite side gets translated
    to 6dp, or the bind param gets translated to 3dp. I don't think we can
    always have control over the bind parameter. So we append '000' to the
    SQLite side.

    """  # noqa
    x = fetch_processed_single_clause(element, compiler)
    fmt = compiler.process(text(_SQLITE_DATETIME_FMT_FOR_PYTHON))
    result = f"(STRFTIME({fmt}, {x}) || '000')"
    # log.warning(result)
    return result


# noinspection PyUnusedLocal
@compiles(isotzdatetime_to_utcdatetime, SqlaDialectName.SQLSERVER)
def isotzdatetime_to_utcdatetime_sqlserver(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> str:
    """
    Implementation of :class:`isotzdatetime_to_utcdatetime` for SQL Server.

    **Converting strings to DATETIME values**

    - ``CAST()``: Part of ANSI SQL.
    - ``CONVERT()``: Not part of ANSI SQL; has some extra formatting options.

    Both methods work:

    .. code-block:: sql

      SELECT CAST('2001-01-31T21:30:49.123' AS DATETIME) AS via_cast,
             CONVERT(DATETIME, '2001-01-31T21:30:49.123') AS via_convert;

    ... fine on SQL Server 2005, with milliseconds in both cases.
    However, going beyond milliseconds doesn't fail gracefully, it causes an
    error (e.g. "...21:30.49.123456") both for CAST and CONVERT.

    The ``DATETIME2`` format accepts greater precision, but requires SQL Server
    2008 or higher. Then this works:

    .. code-block:: sql

      SELECT CAST('2001-01-31T21:30:49.123456' AS DATETIME2) AS via_cast,
             CONVERT(DATETIME2, '2001-01-31T21:30:49.123456') AS via_convert;

    So as not to be too optimistic: ``CAST(x AS DATETIME2)`` ignores (silently)
    any timezone information in the string. So does ``CONVERT(DATETIME2, x, {0
    or 1})``.

    **Converting between time zones**

    NO TIME ZONE SUPPORT in SQL Server 2005.
    e.g. https://stackoverflow.com/questions/3200827/how-to-convert-timezones-in-sql-server-2005.

    .. code-block:: none

        TODATETIMEOFFSET(expression, time_zone):
              expression: something that evaluates to a DATETIME2 value
              time_zone: integer minutes, or string hours/minutes e.g. "+13.00"
          -> produces a DATETIMEOFFSET value

    Available from SQL Server 2008
    (https://docs.microsoft.com/en-us/sql/t-sql/functions/todatetimeoffset-transact-sql).

    .. code-block:: none

        SWITCHOFFSET
          -> converts one DATETIMEOFFSET value to another, preserving its UTC
             time, but changing the displayed (local) time zone.

    ... however, is that unnecessary? We want a plain ``DATETIME2`` in UTC, and
    .conversion to UTC is automatically achieved by ``CONVERT(DATETIME2,
    .some_datetimeoffset, 1)``

    ... https://stackoverflow.com/questions/4953903/how-can-i-convert-a-sql-server-2008-datetimeoffset-to-a-datetime

    ... but not by ``CAST(some_datetimeoffset AS DATETIME2)``, and not by
    ``CONVERT(DATETIME2, some_datetimeoffset, 0)``

    ... and styles 0 and 1 are the only ones permissible from SQL Server 2012
    and up (empirically, and documented for the reverse direction at
    https://docs.microsoft.com/en-us/sql/t-sql/functions/cast-and-convert-transact-sql?view=sql-server-2017)

    ... this is not properly documented re UTC conversion, as far as I can
    see. Let's use ``SWITCHOFFSET -> CAST`` to be explicit and clear.

    ``AT TIME ZONE``: From SQL Server 2016 only.
    https://docs.microsoft.com/en-us/sql/t-sql/queries/at-time-zone-transact-sql?view=sql-server-2017

    **Therefore**

    - We need to require SQL Server 2008 or higher.
    - Therefore we can use the ``DATETIME2`` type.
    - Note that ``LEN()``, not ``LENGTH()``, is ANSI SQL; SQL Server only
      supports ``LEN``.

    **Example (tested on SQL Server 2014)**

    .. code-block:: sql

        DECLARE @source AS VARCHAR(100) = '2001-01-31T21:30:49.123456+07:00';

        SELECT CAST(
            SWITCHOFFSET(
                TODATETIMEOFFSET(
                    CAST(LEFT(@source, LEN(@source) - 6) AS DATETIME2),
                    RIGHT(@source, 6)
                ),
                '+00:00'
            )
            AS DATETIME2
        )  -- 2001-01-31 14:30:49.1234560

    """  # noqa
    x = fetch_processed_single_clause(element, compiler)

    date_time_part = f"LEFT({x}, LEN({x}) - {_TZ_LEN})"  # a VARCHAR
    old_timezone = f"RIGHT({x}, {_TZ_LEN})"  # a VARCHAR
    date_time_no_tz = f"CAST({date_time_part} AS DATETIME2)"  # a DATETIME2
    date_time_offset_with_old_tz = (
        f"TODATETIMEOFFSET({date_time_no_tz}, {old_timezone})"
        # a DATETIMEOFFSET
    )
    date_time_offset_with_utc_tz = (
        f"SWITCHOFFSET({date_time_offset_with_old_tz}, {_UTC_TZ_LITERAL})"
        # a DATETIMEOFFSET in UTC
    )
    result_utc = f"CAST({date_time_offset_with_utc_tz} AS DATETIME2)"

    # log.warning(result_utc)
    return result_utc


# -----------------------------------------------------------------------------
# unknown_field_to_utcdatetime
# -----------------------------------------------------------------------------

# noinspection PyPep8Naming
class unknown_field_to_utcdatetime(FunctionElement):
    """
    Used as an SQL operation by :class:`PendulumDateTimeAsIsoTextColType`.

    Creates an SQL expression wrapping a field containing something unknown,
    which might be a ``DATETIME`` or an ISO-formatted field, and
    making a ``DATETIME`` out of it, in the UTC timezone.

    Implemented for different SQL dialects.
    """
    type = DateTime()
    name = 'unknown_field_to_utcdatetime'


# noinspection PyUnusedLocal
@compiles(unknown_field_to_utcdatetime)
def unknown_field_to_utcdatetime_default(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> None:
    """
    Default implementation for :class:`unknown_field_to_utcdatetime`: fail.
    """
    fail_unknown_dialect(compiler, "perform unknown_field_to_utcdatetime")


# noinspection PyUnusedLocal
@compiles(unknown_field_to_utcdatetime, SqlaDialectName.MYSQL)
def unknown_field_to_utcdatetime_mysql(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> str:
    """
    Implementation of :class:`unknown_field_to_utcdatetime` for MySQL.

    If it's the length of a plain ``DATETIME`` e.g. ``2013-05-30 00:00:00``
    (19), leave it as a ``DATETIME``; otherwise convert ISO -> ``DATETIME``.
    """
    x = fetch_processed_single_clause(element, compiler)
    converted = isotzdatetime_to_utcdatetime_mysql(element, compiler, **kw)
    result = f"IF(LENGTH({x}) = {_MYSQL_DATETIME_LEN}, {x}, {converted})"
    # log.warning(result)
    return result


# noinspection PyUnusedLocal
@compiles(unknown_field_to_utcdatetime, SqlaDialectName.SQLITE)
def unknown_field_to_utcdatetime_sqlite(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> str:
    """
    Implementation of :class:`unknown_field_to_utcdatetime` for SQLite.
    """
    x = fetch_processed_single_clause(element, compiler)
    fmt = compiler.process(text(_SQLITE_DATETIME_FMT_FOR_PYTHON))
    result = f"STRFTIME({fmt}, {x})"
    # log.warning(result)
    return result


# noinspection PyUnusedLocal
@compiles(unknown_field_to_utcdatetime, SqlaDialectName.SQLSERVER)
def unknown_field_to_utcdatetime_sqlserver(
        element: "ClauseElement",
        compiler: "SQLCompiler", **kw) -> str:
    """
    Implementation of :class:`unknown_field_to_utcdatetime` for SQL Server.

    We should cope also with the possibility of a ``DATETIME2`` field, not just
    ``DATETIME``. It seems consistent that ``LEN(DATETIME2) = 27``, with
    precision tenth of a microsecond, e.g. ``2001-01-31 21:30:49.1234567``
    (27).

    So, if it looks like a ``DATETIME`` or a ``DATETIME2``, then we leave it
    alone; otherwise we put it through our ISO-to-datetime function.

    Importantly, note that neither ``_SQLSERVER_DATETIME_LEN`` nor
    ``_SQLSERVER_DATETIME2_LEN`` are the length of any of our ISO strings.
    """
    x = fetch_processed_single_clause(element, compiler)
    # https://stackoverflow.com/questions/5487892/sql-server-case-when-or-then-else-end-the-or-is-not-supported  # noqa
    converted = isotzdatetime_to_utcdatetime_sqlserver(element, compiler, **kw)
    result = (
        f"CASE WHEN LEN({x}) IN "
        f"({_SQLSERVER_DATETIME_LEN}, {_SQLSERVER_DATETIME2_LEN}) THEN {x} "
        f"ELSE {converted} "
        f"END"
    )
    # log.warning(result)
    return result


# =============================================================================
# Custom date/time field as ISO-8601 text including timezone, using
# pendulum.DateTime on the Python side.
# =============================================================================

class PendulumDateTimeAsIsoTextColType(TypeDecorator):
    """
    Stores date/time values as ISO-8601, in a specific format.
    Uses Pendulum on the Python side.
    """

    impl = String(length=StringLengths.ISO8601_DATETIME_STRING_MAX_LEN)
    # ... underlying SQL type

    _coltype_name = "PendulumDateTimeAsIsoTextColType"

    @property
    def python_type(self) -> type:
        """
        The Python type of the object.
        """
        return Pendulum

    @staticmethod
    def pendulum_to_isostring(x: PotentialDatetimeType) -> Optional[str]:
        """
        From a Python datetime to an ISO-formatted string in our particular
        format.
        """
        # https://docs.python.org/3.4/library/datetime.html#strftime-strptime-behavior  # noqa
        x = coerce_to_pendulum(x)
        try:
            mainpart = x.strftime("%Y-%m-%dT%H:%M:%S.%f")  # microsecond accuracy  # noqa
            timezone = x.strftime("%z")  # won't have the colon in
            return mainpart + timezone[:-2] + ":" + timezone[-2:]
        except AttributeError:
            return None

    @staticmethod
    def isostring_to_pendulum(x: Optional[str]) -> Optional[Pendulum]:
        """
        From an ISO-formatted string to a Python Pendulum, with timezone.
        """
        try:
            return coerce_to_pendulum(x)
        except (ParserError, ValueError):
            log.warning("Bad ISO date/time string: {!r}", x)
            return None

    def process_bind_param(self, value: Optional[Pendulum],
                           dialect: Dialect) -> Optional[str]:
        """
        Convert parameters on the way from Python to the database.
        """
        retval = self.pendulum_to_isostring(value)
        if DEBUG_DATETIME_AS_ISO_TEXT:
            log.warning(
                "{}.process_bind_param("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_literal_param(self, value: Optional[Pendulum],
                              dialect: Dialect) -> Optional[str]:
        """
        Convert literals on the way from Python to the database.
        """
        retval = self.pendulum_to_isostring(value)
        if DEBUG_DATETIME_AS_ISO_TEXT:
            log.warning(
                "{}.process_literal_param("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_result_value(self, value: Optional[str],
                             dialect: Dialect) -> Optional[Pendulum]:
        """
        Convert things on the way from the database to Python.
        """
        retval = self.isostring_to_pendulum(value)
        if DEBUG_DATETIME_AS_ISO_TEXT:
            log.warning(
                "{}.process_result_value("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    # noinspection PyPep8Naming
    class comparator_factory(TypeDecorator.Comparator):
        """
        Process SQL for when we are comparing our column, in the database,
        to something else.

        We make this dialect-independent by calling functions like

        .. code-block:: none

            unknown_field_to_utcdatetime
            isotzdatetime_to_utcdatetime

        ... which we then specialize for specific dialects.

        This function itself does not appear to be able to access any
        information about the dialect.
        """

        def operate(self, op, *other, **kwargs):
            assert len(other) == 1
            assert not kwargs
            other = other[0]
            try:
                processed_other = convert_datetime_to_utc(
                    coerce_to_pendulum(other))
                # - If you try to call a dialect-specialized FunctionElement,
                #   it processes the clause to "?" (meaning "attach bind
                #   parameter here"); it's not the value itself.
                # - For our SQLite "milliseconds only" comparator problem (see
                #   above), we can't do very much here without knowing the
                #   dialect. So we make the SQLite side look like it has
                #   microseconds by appending "000"...
            except (AttributeError, ParserError, TypeError, ValueError):
                # OK. At this point, "other" could be a plain DATETIME field,
                # or a PendulumDateTimeAsIsoTextColType field (or potentially
                # something else that we don't really care about). If it's a
                # DATETIME, then we assume it is already in UTC.
                processed_other = unknown_field_to_utcdatetime(other)
            if DEBUG_DATETIME_AS_ISO_TEXT:
                log.warning("operate(self={!r}, op={!r}, other={!r})",
                            self, op, other)
                log.warning("self.expr = {!r}", self.expr)
                log.warning("processed_other = {!r}", processed_other)
                # traceback.print_stack()
            return op(isotzdatetime_to_utcdatetime(self.expr),
                      processed_other)

        def reverse_operate(self, op, *other, **kwargs):
            assert False, "I don't think this is ever being called"


# =============================================================================
# Custom duration field as ISO-8601 text, using pendulum.Duration on the Python
# side.
# =============================================================================

class PendulumDurationAsIsoTextColType(TypeDecorator):
    """
    Stores time durations as ISO-8601, in a specific format.
    Uses :class:`pendulum.Duration` on the Python side.
    """

    impl = String(length=StringLengths.ISO8601_DURATION_STRING_MAX_LEN)
    # ... underlying SQL type

    _coltype_name = "PendulumDurationAsIsoTextColType"

    @property
    def python_type(self) -> type:
        """
        The Python type of the object.
        """
        return Duration

    @staticmethod
    def pendulum_duration_to_isostring(x: Optional[Duration]) -> Optional[str]:
        """
        From a :class:`pendulum.Duration` (or ``None``) an ISO-formatted string
        in our particular format (or ``NULL``).
        """
        if x is None:
            return None
        return duration_to_iso(x, permit_years_months=True,
                               minus_sign_at_front=True)

    @staticmethod
    def isostring_to_pendulum_duration(x: Optional[str]) -> Optional[Duration]:
        """
        From an ISO-formatted string to a Python Pendulum, with timezone.
        """
        if not x:  # None (NULL) or blank string
            return None
        try:
            return duration_from_iso(x)
        except (ISO8601Error, ValueError):
            log.warning("Bad ISO duration string: {!r}", x)
            return None

    def process_bind_param(self, value: Optional[Pendulum],
                           dialect: Dialect) -> Optional[str]:
        """
        Convert parameters on the way from Python to the database.
        """
        retval = self.pendulum_duration_to_isostring(value)
        if DEBUG_DURATION_AS_ISO_TEXT:
            log.warning(
                "{}.process_bind_param("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_literal_param(self, value: Optional[Pendulum],
                              dialect: Dialect) -> Optional[str]:
        """
        Convert literals on the way from Python to the database.
        """
        retval = self.pendulum_duration_to_isostring(value)
        if DEBUG_DURATION_AS_ISO_TEXT:
            log.warning(
                "{}.process_literal_param("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_result_value(self, value: Optional[str],
                             dialect: Dialect) -> Optional[Pendulum]:
        """
        Convert things on the way from the database to Python.
        """
        retval = self.isostring_to_pendulum_duration(value)
        if DEBUG_DURATION_AS_ISO_TEXT:
            log.warning(
                "{}.process_result_value("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    # No comparator_factory; we do not use SQL to compare ISO durations.


# =============================================================================
# Semantic version column type
# =============================================================================

class SemanticVersionColType(TypeDecorator):
    """
    Stores semantic versions in the database.
    Uses :class:`semantic_version.Version` on the Python side.
    """

    impl = String(length=147)  # https://github.com/mojombo/semver/issues/79

    _coltype_name = "SemanticVersionColType"

    @property
    def python_type(self) -> type:
        """
        The Python type of the object.
        """
        return Version

    def process_bind_param(self, value: Optional[Version],
                           dialect: Dialect) -> Optional[str]:
        """
        Convert parameters on the way from Python to the database.
        """
        retval = str(value) if value is not None else None
        if DEBUG_SEMANTIC_VERSION:
            log.warning(
                "{}.process_bind_param("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_literal_param(self, value: Optional[Version],
                              dialect: Dialect) -> Optional[str]:
        """
        Convert literals on the way from Python to the database.
        """
        retval = str(value) if value is not None else None
        if DEBUG_SEMANTIC_VERSION:
            log.warning(
                "{}.process_literal_param("
                "self={!r}, value={!r}, dialect={!r}) -> !r",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_result_value(self, value: Optional[str],
                             dialect: Dialect) -> Optional[Version]:
        """
        Convert things on the way from the database to Python.
        """
        if value is None:
            retval = None
        else:
            # Here we do some slightly fancier conversion to deal with all
            # sorts of potential rubbish coming in, so we get a properly
            # ordered Version out:
            retval = make_version(value)
        if DEBUG_SEMANTIC_VERSION:
            log.warning(
                "{}.process_result_value("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    '''
    # noinspection PyPep8Naming
    class comparator_factory(TypeDecorator.Comparator):
        """
        Process SQL for when we are comparing our column, in the database,
        to something else.

        See https://docs.sqlalchemy.org/en/13/core/type_api.html#sqlalchemy.types.TypeEngine.comparator_factory.

        .. warning::

            I'm not sure this is either (a) correct or (b) used; it may
            produce a string comparison of e.g. ``14.0.0`` versus ``2.0.0``,
            which will be alphabetical and therefore wrong.
            Disabled on 2019-04-28.

        """  # noqa

        def operate(self, op, *other, **kwargs):
            assert len(other) == 1
            assert not kwargs
            other = other[0]
            if isinstance(other, Version):
                processed_other = str(Version)
            else:
                processed_other = other
            return op(self.expr, processed_other)

        def reverse_operate(self, op, *other, **kwargs):
            assert False, "I don't think this is ever being called"
    '''


# =============================================================================
# IdNumReferenceListColType
# =============================================================================

class IdNumReferenceListColType(TypeDecorator):
    """
    Stores a list of IdNumReference objects.
    On the database side, uses a comma-separated list of integers.
    """

    impl = Text()
    _coltype_name = "IdNumReferenceListColType"

    @property
    def python_type(self) -> type:
        """
        The Python type of the object.
        """
        return list

    @staticmethod
    def _idnumdef_list_to_dbstr(
            idnumdef_list: Optional[List[IdNumReference]]) -> str:
        """
        Converts an optional list of
        :class:`camcops_server.cc_modules.cc_simpleobjects.IdNumReference`
        objects to a CSV string suitable for storing in the database.
        """
        if not idnumdef_list:
            return ""
        elements = []  # type: List[int]
        for idnumdef in idnumdef_list:
            elements.append(idnumdef.which_idnum)
            elements.append(idnumdef.idnum_value)
        return ",".join(str(x) for x in elements)

    @staticmethod
    def _dbstr_to_idnumdef_list(dbstr: Optional[str]) -> List[IdNumReference]:
        """
        Converts a CSV string (from the database) to a list of
        :class:`camcops_server.cc_modules.cc_simpleobjects.IdNumReference`
        objects.
        """
        idnumdef_list = []  # type: List[IdNumReference]
        try:
            intlist = [int(numstr) for numstr in dbstr.split(",")]
        except (AttributeError, TypeError, ValueError):
            return []
        length = len(intlist)
        if length == 0 or length % 2 != 0:  # enforce pairs
            return []
        for which_idnum, idnum_value in chunks(intlist, n=2):
            if which_idnum < 0 or idnum_value < 0:  # enforce positive integers
                return []
            idnumdef_list.append(IdNumReference(which_idnum=which_idnum,
                                                idnum_value=idnum_value))
        return idnumdef_list

    def process_bind_param(self, value: Optional[List[IdNumReference]],
                           dialect: Dialect) -> str:
        """
        Convert parameters on the way from Python to the database.
        """
        retval = self._idnumdef_list_to_dbstr(value)
        if DEBUG_IDNUMDEF_LIST:
            log.warning(
                "{}.process_bind_param("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_literal_param(self, value: Optional[List[IdNumReference]],
                              dialect: Dialect) -> str:
        """
        Convert literals on the way from Python to the database.
        """
        retval = self._idnumdef_list_to_dbstr(value)
        if DEBUG_IDNUMDEF_LIST:
            log.warning(
                "{}.process_literal_param("
                "self={!r}, value={!r}, dialect={!r}) -> !r",
                self._coltype_name, self, value, dialect, retval)
        return retval

    def process_result_value(self, value: Optional[str],
                             dialect: Dialect) -> List[IdNumReference]:
        """
        Convert things on the way from the database to Python.
        """
        retval = self._dbstr_to_idnumdef_list(value)
        if DEBUG_IDNUMDEF_LIST:
            log.warning(
                "{}.process_result_value("
                "self={!r}, value={!r}, dialect={!r}) -> {!r}",
                self._coltype_name, self, value, dialect, retval)
        return retval


# =============================================================================
# UUID column type
# =============================================================================

class UuidColType(TypeDecorator):
    # Based on:
    # https://docs.sqlalchemy.org/en/13/core/custom_types.html#backend-agnostic-guid-type  # noqa: E501
    # which will use postgresql UUID if relevant, not doing that here

    impl = CHAR(32)

    @property
    def python_type(self) -> type:
        return str

    def process_bind_param(self, value: uuid.UUID,
                           dialect: Dialect) -> Optional[str]:
        if value is None:
            return None

        return "%.32x" % value.int

    def process_result_value(self, value: Optional[str],
                             dialect: Dialect) -> Optional[uuid.UUID]:
        if value is None:
            return None

        return uuid.UUID(value)


# =============================================================================
# JSON column type
# =============================================================================

class JsonColType(TypeDecorator):
    # Unlike
    # https://docs.sqlalchemy.org/en/13/core/type_basics.html#sqlalchemy.types.JSON
    # does not use vendor-specific JSON type
    impl = UnicodeText

    @property
    def python_type(self) -> type:
        return str

    def process_bind_param(self, value: Any,
                           dialect: Dialect) -> Optional[str]:
        if value is None:
            return None

        return json.dumps(value)

    def process_result_value(self, value: str, dialect: Dialect) -> Any:
        if value is None:
            return None

        return json.loads(value)


# =============================================================================
# PermittedValueChecker: used by CamcopsColumn
# =============================================================================

class PermittedValueChecker(object):
    """
    Represents permitted values (in columns belonging to CamCOPS tasks), and
    checks a value against them.
    """
    def __init__(self,
                 not_null: bool = False,
                 minimum: Union[int, float] = None,
                 maximum: Union[int, float] = None,
                 permitted_values: List[Any] = None) -> None:
        """
        Args:
            not_null: must the value not be NULL?
            minimum: if specified, a numeric minimum value
            maximum: if specified, a numeric maximum value
            permitted_values: if specified, a list of permitted values
        """
        self.not_null = not_null
        self.minimum = minimum
        self.maximum = maximum
        self.permitted_values = permitted_values

    def is_ok(self, value: Any) -> bool:
        """
        Does the value pass our tests?
        """
        if value is None:
            return not self.not_null
            # If not_null is True, then the value is not OK; return False.
            # If not_null is False, then a null value passes all other tests.
        if self.permitted_values is not None and value not in self.permitted_values:  # noqa
            return False
        if self.minimum is not None and value < self.minimum:
            return False
        if self.maximum is not None and value > self.maximum:
            return False
        return True

    def failure_msg(self, value: Any) -> str:
        """
        Why does the value not pass our tests?
        """
        if value is None:
            if self.not_null:
                return "value is None and NULL values are not permitted"
            else:
                return ""  # value is OK
        if self.permitted_values is not None and value not in self.permitted_values:  # noqa
            return (
                f"value {value!r} not in permitted values "
                f"{self.permitted_values!r}"
            )
        if self.minimum is not None and value < self.minimum:
            return f"value {value!r} less than minimum of {self.minimum!r}"
        if self.maximum is not None and value > self.maximum:
            return f"value {value!r} more than maximum of {self.maximum!r}"
        return ""

    def __repr__(self):
        return auto_repr(self)

    def permitted_values_csv(self) -> str:
        """
        Returns a CSV representation of the permitted values.

        Primarily used for CRIS data dictionaries.
        """
        if self.permitted_values:
            return ",".join(str(x) for x in self.permitted_values)
        # Take a punt that integer minima/maxima mean that only integers are
        # permitted...
        if isinstance(self.minimum, int) and isinstance(self.maximum, int):
            return ",".join(
                str(x) for x in range(self.minimum, self.maximum + 1))
        return ""


# Specific instances, to reduce object duplication and magic numbers:

MIN_ZERO_CHECKER = PermittedValueChecker(minimum=0)

BIT_CHECKER = PermittedValueChecker(permitted_values=PV.BIT)
ZERO_TO_ONE_CHECKER = PermittedValueChecker(minimum=0, maximum=1)
ZERO_TO_TWO_CHECKER = PermittedValueChecker(minimum=0, maximum=2)
ZERO_TO_THREE_CHECKER = PermittedValueChecker(minimum=0, maximum=3)
ZERO_TO_FOUR_CHECKER = PermittedValueChecker(minimum=0, maximum=4)
ZERO_TO_FIVE_CHECKER = PermittedValueChecker(minimum=0, maximum=5)
ZERO_TO_SIX_CHECKER = PermittedValueChecker(minimum=0, maximum=6)
ZERO_TO_SEVEN_CHECKER = PermittedValueChecker(minimum=0, maximum=7)
ZERO_TO_EIGHT_CHECKER = PermittedValueChecker(minimum=0, maximum=8)
ZERO_TO_NINE_CHECKER = PermittedValueChecker(minimum=0, maximum=9)
ZERO_TO_10_CHECKER = PermittedValueChecker(minimum=0, maximum=10)
ZERO_TO_100_CHECKER = PermittedValueChecker(minimum=0, maximum=100)

ONE_TO_TWO_CHECKER = PermittedValueChecker(minimum=1, maximum=2)
ONE_TO_THREE_CHECKER = PermittedValueChecker(minimum=1, maximum=3)
ONE_TO_FOUR_CHECKER = PermittedValueChecker(minimum=1, maximum=4)
ONE_TO_FIVE_CHECKER = PermittedValueChecker(minimum=1, maximum=5)
ONE_TO_SIX_CHECKER = PermittedValueChecker(minimum=1, maximum=6)
ONE_TO_SEVEN_CHECKER = PermittedValueChecker(minimum=1, maximum=7)
ONE_TO_EIGHT_CHECKER = PermittedValueChecker(minimum=1, maximum=8)
ONE_TO_NINE_CHECKER = PermittedValueChecker(minimum=1, maximum=9)


# =============================================================================
# CamcopsColumn: provides extra functions over Column.
# =============================================================================

# noinspection PyAbstractClass
class CamcopsColumn(Column):
    """
    A SQLAlchemy :class:`Column` class that supports some CamCOPS-specific
    flags, such as:

    - whether a field is a BLOB reference;
    - how it should be treated for anonymisation;
    - which values are permitted in the field (in a soft sense: duff values
      cause errors to be reported, but they're still stored).
    """
    def __init__(self,
                 *args,
                 include_in_anon_staging_db: bool = False,
                 exempt_from_anonymisation: bool = False,
                 identifies_patient: bool = False,
                 is_blob_id_field: bool = False,
                 blob_relationship_attr_name: str = "",
                 permitted_value_checker: PermittedValueChecker = None,
                 **kwargs) -> None:
        """

        Args:
            *args:
                Arguments to the :class:`Column` constructor.
            include_in_anon_staging_db:
                Ensure this is marked for inclusion in data dictionaries for an
                anonymisation staging database.
            exempt_from_anonymisation:
                If true: though this field might be text, it is guaranteed not
                to contain identifiers (e.g. it might contain only predefined
                disease severity descriptions) and does not require
                anonymisation.
            identifies_patient:
                If true: contains a patient identifier (e.g. name).
            is_blob_id_field:
                If true: this field contains a reference (client FK) to the
                BLOB table.
            blob_relationship_attr_name:
                For BLOB ID fields: the name of the associated relationship
                attribute (which, when accessed, yields the BLOB itself) in
                the owning class/object.
            permitted_value_checker:
                If specified, a :class:`PermittedValueChecker` that allows
                soft constraints to be specified on the field's contents. (That
                is, no constraints are specified at the database level, but we
                can moan if incorrect data are present.)
            **kwargs:
                Arguments to the :class:`Column` constructor.
        """
        self.include_in_anon_staging_db = include_in_anon_staging_db
        self.exempt_from_anonymisation = exempt_from_anonymisation
        self.identifies_patient = identifies_patient
        self.is_blob_id_field = is_blob_id_field
        self.blob_relationship_attr_name = blob_relationship_attr_name
        self.permitted_value_checker = permitted_value_checker
        if is_blob_id_field:
            assert blob_relationship_attr_name, (
                "If specifying a BLOB ID field, must give the attribute name "
                "of the relationship too")
        super().__init__(*args, **kwargs)

    def _constructor(self, *args, **kwargs) -> "CamcopsColumn":
        """
        SQLAlchemy method (not clearly documented) to assist in copying
        objects. Returns a copy of this object.

        See
        https://bitbucket.org/zzzeek/sqlalchemy/issues/2284/please-make-column-easier-to-subclass
        """  # noqa
        kwargs['include_in_anon_staging_db'] = self.include_in_anon_staging_db
        kwargs['exempt_from_anonymisation'] = self.exempt_from_anonymisation
        kwargs['identifies_patient'] = self.identifies_patient
        kwargs['is_blob_id_field'] = self.is_blob_id_field
        kwargs['blob_relationship_attr_name'] = self.blob_relationship_attr_name  # noqa
        kwargs['permitted_value_checker'] = self.permitted_value_checker
        # noinspection PyTypeChecker
        return self.__class__(*args, **kwargs)

    def __repr__(self) -> str:
        def kvp(attrname: str) -> str:
            return f"{attrname}={getattr(self, attrname)!r}"
        elements = [
            kvp("include_in_anon_staging_db"),
            kvp("exempt_from_anonymisation"),
            kvp("identifies_patient"),
            kvp("is_blob_id_field"),
            kvp("blob_relationship_attr_name"),
            kvp("permitted_value_checker"),
            f"super()={super().__repr__()}",
        ]
        return f"CamcopsColumn({', '.join(elements)})"

    def set_permitted_value_checker(
            self, permitted_value_checker: PermittedValueChecker) -> None:
        """
        Sets the :class:`PermittedValueChecker` attribute.
        """
        self.permitted_value_checker = permitted_value_checker


# =============================================================================
# Operate on Column/CamcopsColumn properties
# =============================================================================

def gen_columns_matching_attrnames(obj, attrnames: List[str]) \
        -> Generator[Tuple[str, Column], None, None]:
    """
    Find columns of an SQLAlchemy ORM object whose attribute names match a
    list.

    Args:
        obj: SQLAlchemy ORM object to inspect
        attrnames: attribute names

    Yields:
        ``attrname, column`` tuples

    """
    for attrname, column in gen_columns(obj):
        if attrname in attrnames:
            yield attrname, column


def gen_camcops_columns(obj) -> Generator[Tuple[str, CamcopsColumn],
                                          None, None]:
    """
    Finds all columns of an object that are
    :class:`camcops_server.cc_modules.cc_sqla_coltypes.CamcopsColumn` columns.

    Args:
        obj: SQLAlchemy ORM object to inspect

    Yields:
        ``attrname, column`` tuples
    """
    for attrname, column in gen_columns(obj):
        if isinstance(column, CamcopsColumn):
            yield attrname, column


def gen_camcops_blob_columns(obj) -> Generator[Tuple[str, CamcopsColumn],
                                               None, None]:
    """
    Finds all columns of an object that are
    :class:`camcops_server.cc_modules.cc_sqla_coltypes.CamcopsColumn` columns
    referencing the BLOB table.

    Args:
        obj: SQLAlchemy ORM object to inspect

    Yields:
        ``attrname, column`` tuples
    """
    for attrname, column in gen_camcops_columns(obj):
        if column.is_blob_id_field:
            if attrname != column.name:
                log.warning("BLOB field where attribute name {!r} != SQL "
                            "column name {!r}", attrname, column.name)
            yield attrname, column


def get_column_attr_names(obj) -> List[str]:
    """
    Get a list of column attribute names from an SQLAlchemy ORM object.
    """
    return [attrname for attrname, _ in gen_columns(obj)]


def get_camcops_column_attr_names(obj) -> List[str]:
    """
    Get a list of
    :class:`camcops_server.cc_modules.cc_sqla_coltypes.CamcopsColumn` column
    attribute names from an SQLAlchemy ORM object.
    """
    return [attrname for attrname, _ in gen_camcops_columns(obj)]


def get_camcops_blob_column_attr_names(obj) -> List[str]:
    """
    Get a list of
    :class:`camcops_server.cc_modules.cc_sqla_coltypes.CamcopsColumn` BLOB
    column attribute names from an SQLAlchemy ORM object.
    """
    return [attrname for attrname, _ in gen_camcops_blob_columns(obj)]


def permitted_value_failure_msgs(obj) -> List[str]:
    """
    Checks a SQLAlchemy ORM object instance against its permitted value checks
    (via its :class:`camcops_server.cc_modules.cc_sqla_coltypes.CamcopsColumn`
    columns), if it has any.

    Returns a list of failure messages (empty list means all OK).

    If you just want to know whether it passes, a quicker way is via
    :func:`permitted_values_ok`.
    """
    failure_msgs = []
    for attrname, camcops_column in gen_camcops_columns(obj):
        pv_checker = camcops_column.permitted_value_checker  # type: Optional[PermittedValueChecker]  # noqa
        if pv_checker is None:
            continue
        value = getattr(obj, attrname)
        failure_msg = pv_checker.failure_msg(value)
        if failure_msg:
            failure_msgs.append(
                f"Invalid value for {attrname}: {failure_msg}")
    return failure_msgs


def permitted_values_ok(obj) -> bool:
    """
    Checks whether an instance passes its permitted value checks, if it has
    any.

    If you want to know why it failed, see
    :func:`permitted_value_failure_msgs`.
    """
    for attrname, camcops_column in gen_camcops_columns(obj):
        pv_checker = camcops_column.permitted_value_checker  # type: Optional[PermittedValueChecker]  # noqa
        if pv_checker is None:
            continue
        value = getattr(obj, attrname)
        if not pv_checker.is_ok(value):
            return False
    return True


def gen_ancillary_relationships(obj) -> Generator[
        Tuple[str, RelationshipProperty, Type["GenericTabletRecordMixin"]],
        None, None]:
    """
    For an SQLAlchemy ORM object, yields tuples of ``attrname,
    relationship_property, related_class`` for all relationships that are
    marked as a CamCOPS ancillary relationship.
    """
    for attrname, rel_prop, related_class in gen_relationships(obj):
        if rel_prop.info.get(RelationshipInfo.IS_ANCILLARY, None) is True:
            yield attrname, rel_prop, related_class


def gen_blob_relationships(obj) -> Generator[
        Tuple[str, RelationshipProperty, Type["GenericTabletRecordMixin"]],
        None, None]:
    """
    For an SQLAlchemy ORM object, yields tuples of ``attrname,
    relationship_property, related_class`` for all relationships that are
    marked as a CamCOPS BLOB relationship.
    """
    for attrname, rel_prop, related_class in gen_relationships(obj):
        if rel_prop.info.get(RelationshipInfo.IS_BLOB, None) is True:
            yield attrname, rel_prop, related_class


# =============================================================================
# Specializations of CamcopsColumn to save typing
# =============================================================================

def _name_type_in_column_args(args: Tuple[Any, ...]) -> Tuple[bool, bool]:
    """
    SQLAlchemy doesn't encourage deriving from Column. If you do, you have to
    implement ``__init__()`` and ``_constructor()`` carefully. The
    ``__init__()`` function will be called by user code, and via SQLAlchemy
    internals, including via ``_constructor`` (e.g. from
    ``Column.make_proxy()``).

    It is likely that ``__init__`` will experience many combinations of the
    column name and type being passed either in ``*args`` or ``**kwargs``. It
    must pass them on to :class:`Column`. If you don't mess with the type,
    that's easy; just pass them on unmodified. But if you plan to mess with the
    type, as we do in :class:`BoolColumn` below, we must make sure that we
    don't pass either of ``name`` or ``type_`` in *both* ``args`` and
    ``kwargs``.

    This function tells you whether ``name`` and ``type_`` are present in args,
    using the same method as ``Column.__init__()``.
    """
    name_in_args = False
    type_in_args = False
    args = list(args)  # make a copy, and make it a list not a tuple
    if args:
        if isinstance(args[0], util.string_types):
            name_in_args = True
            args.pop(0)
    if args:
        coltype = args[0]
        if hasattr(coltype, "_sqla_type"):
            type_in_args = True
    return name_in_args, type_in_args


# noinspection PyAbstractClass
class BoolColumn(CamcopsColumn):
    """
    A :class:`camcops_server.cc_modules.cc_sqla_coltypes.CamcopsColumn`
    representing a boolean value.
    """
    def __init__(self, *args: Any, **kwargs: Any) -> None:
        # Must pass on all arguments, ultimately to Column, or when using
        # AbstractConcreteBase, you can get this:
        #
        # TypeError: Could not create a copy of this <class 'camcops_server.
        # cc_modules.cc_sqla_coltypes.BoolColumn'> object.  Ensure the class
        # includes a _constructor() attribute or method which accepts the
        # standard Column constructor arguments, or references the Column class
        # itself.
        #
        # During internal copying, "type_" can arrive here within kwargs, so
        # we must make sure that we don't send it on twice to super().__init().
        # Also, Column.make_proxy() calls our _constructor() with name and type
        # in args, so we must handle that, too...

        _, type_in_args = _name_type_in_column_args(args)
        self.constraint_name = kwargs.pop("constraint_name", None)  # type: Optional[str]  # noqa
        if not type_in_args:
            if self.constraint_name:
                constraint_name_conv = conv(self.constraint_name)
                # ... see help for ``conv``
            else:
                constraint_name_conv = None
            kwargs['type_'] = Boolean(name=constraint_name_conv)
            # The "name" parameter to Boolean() specifies the  name of the
            # (0, 1) constraint.
        kwargs['permitted_value_checker'] = BIT_CHECKER
        super().__init__(*args, **kwargs)
        if (not self.constraint_name and
                len(self.name) >= LONG_COLUMN_NAME_WARNING_LIMIT):
            log.warning(
                "BoolColumn with long column name and no constraint name: "
                "{!r}", self.name
            )

    def __repr__(self) -> str:
        def kvp(attrname: str) -> str:
            return f"{attrname}={getattr(self, attrname)!r}"
        elements = [
            kvp("constraint_name"),
            f"super()={super().__repr__()}",
        ]
        return f"BoolColumn({', '.join(elements)})"

    def _constructor(self, *args: Any, **kwargs: Any) -> "BoolColumn":
        """
        Make a copy; see
        https://bitbucket.org/zzzeek/sqlalchemy/issues/2284/please-make-column-easier-to-subclass
        """
        kwargs["constraint_name"] = self.constraint_name
        return super()._constructor(*args, **kwargs)
