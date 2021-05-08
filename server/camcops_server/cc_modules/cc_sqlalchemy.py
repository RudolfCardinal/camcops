#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_sqlalchemy.py

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

**SQLAlchemy helper functions and constants.**

We define our metadata ``Base`` here, and things like our index naming
convention and MySQL table formats.

A few random notes:

- SQLAlchemy will automatically warn about clashing columns:

  .. :code-block:: python

    from sqlalchemy import Column, Integer
    from sqlalchemy.ext.declarative import declarative_base

    Base = declarative_base()

    class Thing(Base):
        __tablename__ = "thing"
        a = Column("a", Integer, primary_key=True)
        b = Column("b", Integer)
        c = Column("b", Integer)  # produces a warning:

  .. code-block:: none

    SAWarning: On class 'Thing', Column object 'b' named directly multiple
    times, only one will be used: b, c. Consider using orm.synonym instead

"""

from abc import ABCMeta
from io import StringIO
import logging
import sqlite3

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.sqlalchemy.dialect import SqlaDialectName
from cardinal_pythonlib.sqlalchemy.dump import dump_ddl
from cardinal_pythonlib.sqlalchemy.session import (
    make_sqlite_url,
    SQLITE_MEMORY_URL,
)
from pendulum import DateTime as Pendulum

from sqlalchemy.engine import create_engine
from sqlalchemy.engine.base import Engine
from sqlalchemy.ext.declarative import declarative_base, DeclarativeMeta
from sqlalchemy.sql.schema import MetaData

from camcops_server.cc_modules.cc_cache import cache_region_static, fkg

log = BraceStyleAdapter(logging.getLogger(__name__))

# =============================================================================
# Naming convention; metadata; Base
# =============================================================================
# https://alembic.readthedocs.org/en/latest/naming.html
# http://docs.sqlalchemy.org/en/latest/core/constraints.html#configuring-constraint-naming-conventions  # noqa

MYSQL_MAX_IDENTIFIER_LENGTH = 64
LONG_COLUMN_NAME_WARNING_LIMIT = 30

NAMING_CONVENTION = {
    # - Note that constraint names must be unique in the DATABASE, not the
    #   table;
    #   https://dev.mysql.com/doc/refman/5.6/en/create-table-foreign-keys.html
    # - Index names only have to be unique for the table;
    #   https://stackoverflow.com/questions/30653452/do-index-names-have-to-be-unique-across-entire-database-in-mysql  # noqa

    # INDEX:
    "ix": 'ix_%(column_0_label)s',

    # UNIQUE CONSTRAINT:
    "uq": "uq_%(table_name)s_%(column_0_name)s",
    # "uq": "uq_%(column_0_name)s",

    # CHECK CONSTRAINT:
    # "ck": "ck_%(table_name)s_%(constraint_name)s",  # too long for MySQL
    # ... https://groups.google.com/forum/#!topic/sqlalchemy/SIT4D8S9dUg
    # "ck": "ck_%(table_name)s_%(column_0_name)s",
    # Problem 2018-09-14:
    # - constraints must be unique across database
    # - MySQL only accepts 64 characters for constraint name
    # - using "%(column_0_name)" means that explicit constrant names are
    #   ignored
    # - using "%(constraint_name)" means that all constraints have to be named
    #   explicitly (very tedious)
    # - so truncate?
    #   https://docs.python.org/3/library/stdtypes.html#old-string-formatting
    #   https://www.python.org/dev/peps/pep-0237/
    # - The main problem is BOOL columns, e.g.
    #   cpft_lps_discharge.management_specialling_behavioural_disturbance
    # - Example:
    #   longthing = "abcdefghijklmnopqrstuvwxyz"
    #   d = {"thing": longthing}
    #   "hello %(thing).10s world" % d  # LEFT TRUNCATE
    #   # ... gives 'hello abcdefghij world'
    # "ck": "ck_%(table_name).30s_%(column_0_name).30s",
    # 3 for "ck_" leaves 61; 30 for table, 1 for "_", 30 for column
    # ... no...
    # "obs_contamination_bodily_waste_*"
    "ck": "ck_%(table_name)s_%(column_0_name)s",  # unique but maybe too long

    # FOREIGN KEY:
    # "fk": "fk_%(table_name)s_%(column_0_name)s_%(referred_table_name)s",  # too long for MySQL sometimes!  # noqa
    "fk": "fk_%(table_name)s_%(column_0_name)s",
    # "fk": "fk_%(column_0_name)s",

    # PRIMARY KEY:
    "pk": "pk_%(table_name)s"
}
MASTER_META = MetaData(naming_convention=NAMING_CONVENTION)

# The base of all our model classes:
Base = declarative_base(metadata=MASTER_META)

# Special options:
Base.__table_args__ = {
    # -------------------------------------------------------------------------
    # MySQL special options
    # -------------------------------------------------------------------------
    # SQLAlchemy __table_args__:
    #   http://docs.sqlalchemy.org/en/latest/orm/extensions/declarative/table_config.html  # noqa
    # SQLAlchemy sends keyword arguments like 'mysql_keyword_name' to be
    # rendered as KEYWORD_NAME in the CREATE TABLE statement:
    #   http://docs.sqlalchemy.org/en/latest/dialects/mysql.html

    # Engine: InnoDB
    'mysql_engine': 'InnoDB',

    # Barracuda: COMPRESSED or DYNAMIC
    # https://dev.mysql.com/doc/refman/5.7/en/innodb-row-format-dynamic.html
    # https://xenforo.com/community/threads/anyone-running-their-innodb-tables-with-row_format-compressed.99606/  # noqa
    # We shouldn't compress everything by default; performance hit.
    'mysql_row_format': 'DYNAMIC',

    # SEE server_troubleshooting.rst FOR BUG DISCUSSION

    'mysql_charset': 'utf8mb4 COLLATE utf8mb4_unicode_ci',

    # Character set
    # REPLACED # 'mysql_charset': 'utf8mb4',
    # https://dev.mysql.com/doc/refman/5.5/en/charset-unicode-utf8mb4.html

    # Collation
    # Which collation for MySQL? See
    # - https://stackoverflow.com/questions/766809/whats-the-difference-between-utf8-general-ci-and-utf8-unicode-ci  # noqa
    # REPLACED # 'mysql_collate': 'utf8mb4_unicode_ci'
    # Note that COLLATION rules are, from least to greatest precedence:
    #       Server collation
    #       Connection-specific collation
    #       Database collation
    #       Table collation
    #       Column collation
    #       Query collation (using CAST or CONVERT)
    # - https://stackoverflow.com/questions/24356090/difference-between-database-table-column-collation  # noqa
    # Therefore, we can set the table collation for all our tables, and not
    # worry about the column collation, e.g. Text(collation=...).
    #
    # To check a MySQL database, and connection/server settings:
    #       SHOW VARIABLES LIKE '%character%';
    #       SHOW VARIABLES LIKE '%collation%';
    # To check tables:
    #       SHOW TABLE STATUS WHERE NAME LIKE 'my_tablename'\G
    # ... note use of \G to produce long-form output!
    # To check columns:
    #       SHOW FULL COLUMNS FROM my_tablename;
    #
    # ONE THING IN PARTICULAR TO BEWARE: utf8mb4_unicode_ci produces
    # CASE-INSENSITIVE COMPARISON. For example:
    #       SELECT 'a' = 'A';  -- produces 1
    #       SELECT 'a' = 'B';  -- produces 0
    #       SELECT BINARY 'a' = BINARY 'A';  -- produces 0
    # This is a PROBLEM FOR PASSWORD FIELDS IF WE INTEND TO DO DATABASE-LEVEL
    # COMPARISONS WITH THEM. In that case we must ensure a different collation
    # is set; specifically, use
    #
    #       utf8mb4_bin
    #
    # and see also
    #       SHOW COLLATION WHERE `Collation` LIKE 'utf8mb4%';
    # and
    #   https://dev.mysql.com/doc/refman/5.6/en/charset-binary-collations.html
    #
    # To check, run
    #       SHOW FULL COLUMNS FROM _security_users;
}

# MySQL things we can't set via SQLAlchemy, but would like to be set:
# - max_allowed_packet: should be at least 32M
# - innodb_strict_mode: should be 1, but less of a concern with SQLAlchemy

# MySQL things we don't care about too much:
# - innodb_file_per_table: desirable, but up to the user.


class DeclarativeAndABCMeta(DeclarativeMeta, ABCMeta):
    """
    Metaclass for classes that want to inherit from Base and also ABC:
    """
    pass


# =============================================================================
# Convenience functions
# =============================================================================

def make_memory_sqlite_engine(echo: bool = False) -> Engine:
    """
    Create an SQLAlchemy :class:`Engine` for an in-memory SQLite database.
    """
    return create_engine(SQLITE_MEMORY_URL, echo=echo)


def make_file_sqlite_engine(filename: str, echo: bool = False) -> Engine:
    """
    Create an SQLAlchemy :class:`Engine` for an on-disk SQLite database.
    """
    return create_engine(make_sqlite_url(filename), echo=echo)


def sql_from_sqlite_database(connection: sqlite3.Connection) -> str:
    """
    Returns SQL to describe an SQLite database.

    Args:
        connection: connection to SQLite database via ``sqlite3`` module

    Returns:
        the SQL

    """
    with StringIO() as f:
        # noinspection PyTypeChecker
        for line in connection.iterdump():
            f.write(line + "\n")
        f.flush()
        return f.getvalue()


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def get_all_ddl(dialect_name: str = SqlaDialectName.MYSQL) -> str:
    """
    Returns the DDL (data definition language; SQL ``CREATE TABLE`` commands)
    for our SQLAlchemy metadata.

    Args:
        dialect_name: SQLAlchemy dialect name
    """
    metadata = Base.metadata  # type: MetaData
    with StringIO() as f:
        dump_ddl(metadata, dialect_name=dialect_name, fileobj=f)
        f.flush()
        text = f.getvalue()
    return text


def log_all_ddl(dialect_name: str = SqlaDialectName.MYSQL) -> None:
    """
    Send the DDL for our SQLAlchemy metadata to the Python log.

    Args:
        dialect_name: SQLAlchemy dialect name
    """
    text = get_all_ddl(dialect_name)
    log.info(text)
    log.info("DDL length: {} characters", len(text))


def assert_constraint_name_ok(table_name: str, column_name: str) -> None:
    """
    Checks that the automatically generated name of a constraint isn't too long
    for specific databases.

    Args:
        table_name: table name
        column_name: column name

    Raises:
        AssertionError, if something will break
    """
    d = {
        "table_name": table_name,
        "column_0_name": column_name,
    }
    anticipated_name = NAMING_CONVENTION["ck"] % d
    if len(anticipated_name) > MYSQL_MAX_IDENTIFIER_LENGTH:
        raise AssertionError(
            f"Constraint name too long for table {table_name!r}, column "
            f"{column_name!r}; will be {anticipated_name!r} "
            f"of length {len(anticipated_name)}")


# =============================================================================
# Database engine hacks
# =============================================================================

def hack_pendulum_into_pymysql() -> None:
    """
    Hack in support for :class:`pendulum.DateTime` into the ``pymysql``
    database interface.

    See https://pendulum.eustace.io/docs/#limitations.
    """
    try:
        # noinspection PyUnresolvedReferences
        from pymysql.converters import encoders, escape_datetime
        encoders[Pendulum] = escape_datetime
    except ImportError:
        pass


hack_pendulum_into_pymysql()
