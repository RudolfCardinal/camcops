#!/usr/bin/env python

"""
camcops_server/alembic/env.py

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

**This file configures and runs Alembic.**

It is loaded directly by Alembic, via a pseudo-"main" environment.

"""

# =============================================================================
# Imports
# =============================================================================

import logging
import os
from typing import List, Optional, Tuple, Union

from alembic import context
from alembic.config import Config
from alembic.runtime.migration import MigrationContext
from alembic.operations.ops import (
    AlterColumnOp,
    ModifyTableOps,
    MigrationScript,
    OpContainer,
    UpgradeOps,
)
from cardinal_pythonlib.sqlalchemy.alembic_func import get_current_revision
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)
from cardinal_pythonlib.sqlalchemy.session import get_safe_url_from_url
from sqlalchemy import engine_from_config, pool
from sqlalchemy.dialects.mysql.types import LONGTEXT, TINYINT
from sqlalchemy.sql.sqltypes import Boolean, UnicodeText
from sqlalchemy.sql.type_api import TypeEngine
from sqlalchemy.sql.schema import Column, MetaData

# No relative imports from within the Alembic zone.
from camcops_server.cc_modules.cc_baseconstants import ALEMBIC_VERSION_TABLE
from camcops_server.cc_modules.cc_config import get_default_config_from_os_env
from camcops_server.cc_modules.cc_sqlalchemy import Base
# noinspection PyUnresolvedReferences
import camcops_server.cc_modules.cc_all_models  # import side effects (ensure all models registered)  # noqa

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Sort out unwanted autogenerated things; see
# - https://alembic.zzzcomputing.com/en/latest/api/autogenerate.html
# - https://alembic.zzzcomputing.com/en/latest/cookbook.html
# - https://bitbucket.org/zzzeek/alembic/issues/46/mysqltinyint-display_width-1-vs-saboolean  # noqa
# - http://alembic.zzzcomputing.com/en/latest/api/autogenerate.html
# =============================================================================
def debug_op_object(op: Union[List, OpContainer, Tuple],
                    level: int = 0) -> str:
    """
    Describes a :class:`OpContainer`.
    """
    lines = []  # type: List[str]
    spacer = "    " * level
    thisobj = spacer + str(op)
    if isinstance(op, ModifyTableOps):
        thisobj += " for table {}".format(op.table_name)
    if isinstance(op, AlterColumnOp):
        thisobj += " for column {}.{}".format(op.table_name, op.column_name)
    lines.append(thisobj)
    if hasattr(op, "ops"):
        for sub_op in op.ops:
            lines.append(debug_op_object(sub_op, level + 1))
    return "\n".join(lines)


def is_tinyint_and_bool(inspected_type: TypeEngine,
                        metadata_type: TypeEngine) -> bool:
    return (isinstance(inspected_type, TINYINT) and
            inspected_type.display_width == 1 and
            isinstance(metadata_type, Boolean))


def is_longtext_and_unicode(inspected_type: TypeEngine,
                            metadata_type: TypeEngine) -> bool:
    return (isinstance(inspected_type, LONGTEXT) and
            inspected_type.collation == 'utf8mb4_unicode_ci' and
            isinstance(metadata_type, UnicodeText) and
            metadata_type.length == 4294967295)


def custom_compare_type(context: MigrationContext,
                        inspected_column: Column,
                        metadata_column: Column,
                        inspected_type: TypeEngine,
                        metadata_type: TypeEngine) -> Optional[bool]:
    """
    Perform type comparison?

    Args:
        context: frontend to database
        inspected_column: column from the database
        metadata_column: column from the SQLAlchemy metadata
        inspected_type: column type reflected from the database
        metadata_type: column type from the SQLAlchemy metadata

    Returns:
        False if the metadata type is the same as the inspected type
        None to allow the default implementation to compare these

        A return value of True would mean the two types do not
        match and should result in a type change operation

    Specifically, it detects:

    - MySQL ``TINYINT(1)`` is equivalent to SQLAlchemy ``Boolean()``, because
      ``TINYINT(1)`` is the correct instantiation of ``Boolean()``.

    - ``LONGTEXT(collation='utf8mb4_unicode_ci')`` is the MySQL database
      version of ``UnicodeText(length=4294967295)``
    """

    checkers = (
        is_tinyint_and_bool,
        is_longtext_and_unicode,
    )

    for types_equivalent in checkers:
        if types_equivalent(inspected_type, metadata_type):
            log.debug(
                "Skipping duff type change of {!r} to {!r} for {}.{}",
                inspected_type, metadata_type,
                inspected_column.table.name, inspected_column.name
            )

            return False

    return None


# noinspection PyUnusedLocal
def process_revision_directives(context_: MigrationContext,  # empirically!
                                revision: Tuple[str],  # empirically!
                                directives: List[MigrationScript]) -> None:
    """
    Process autogenerated migration scripts and fix these problems.
    """
    if context_.config.cmd_opts.autogenerate:
        log.warning("Checking autogenerated operations")
        script = directives[0]

        # Check/filter our upgrade table ops.
        upgrade_ops = script.upgrade_ops  # type: UpgradeOps

        # If no changes to the schema are produced, don't generate a revision
        # file:
        log.info("upgrade_ops:\n{}", debug_op_object(upgrade_ops))
        if upgrade_ops.is_empty():
            log.info("No changes; not generating a revision file.")
            directives[:] = []


# =============================================================================
# Migration functions
# =============================================================================

def run_migrations_offline(config: Config,
                           target_metadata: MetaData) -> None:
    """
    Run migrations in 'offline' mode.

    This configures the context with just a URL and not an Engine, though an
    Engine is acceptable here as well.  By skipping the Engine creation we
    don't even need a DBAPI to be available.

    Calls to context.execute() here emit the given string to the script output.
    """
    url = config.get_main_option("sqlalchemy.url")
    # RNC
    context.configure(
        url=url,
        target_metadata=target_metadata,
        render_as_batch=True,  # for SQLite mode; http://stackoverflow.com/questions/30378233  # noqa
        literal_binds=True,
        version_table=ALEMBIC_VERSION_TABLE,
        compare_type=custom_compare_type,
        # ... http://blog.code4hire.com/2017/06/setting-up-alembic-to-detect-the-column-length-change/  # noqa
        # ... https://eshlox.net/2017/08/06/alembic-migration-for-string-length-change/  # noqa

        # process_revision_directives=writer,
        process_revision_directives=process_revision_directives,
    )
    with context.begin_transaction():
        context.run_migrations()


def run_migrations_online(config: Config,
                          target_metadata: MetaData) -> None:
    """
    Run migrations in 'online' mode.

    In this scenario we need to create an Engine and associate a connection
    with the context.
    """
    connectable = engine_from_config(
        config.get_section(config.config_ini_section),
        prefix='sqlalchemy.',
        poolclass=pool.NullPool)

    with connectable.connect() as connection:
        # RNC
        context.configure(
            connection=connection,
            target_metadata=target_metadata,
            render_as_batch=True,  # for SQLite mode; http://stackoverflow.com/questions/30378233  # noqa
            version_table=ALEMBIC_VERSION_TABLE,
            compare_type=custom_compare_type,

            # process_revision_directives=writer,
            process_revision_directives=process_revision_directives,
        )
        with context.begin_transaction():
            context.run_migrations()


# =============================================================================
# Main commands
# =============================================================================
# We're in a pseudo-"main" environment.
# We need to reconfigure our logger, but __name__ is not "__main__".

def run_alembic() -> None:
    alembic_config = context.config  # type: Config
    target_metadata = Base.metadata
    camcops_config = get_default_config_from_os_env()
    dburl = camcops_config.db_url
    alembic_config.set_main_option('sqlalchemy.url', dburl)
    log.warning("Applying migrations to database at URL: {}",
                get_safe_url_from_url(dburl))
    log.info("Current database revision is {!r}",
             get_current_revision(dburl, ALEMBIC_VERSION_TABLE))

    if context.is_offline_mode():
        run_migrations_offline(alembic_config, target_metadata)
    else:
        run_migrations_online(alembic_config, target_metadata)


if not os.environ.get("_SPHINX_AUTODOC_IN_PROGRESS", None):
    main_only_quicksetup_rootlogger(level=logging.DEBUG)
    # log.debug("IN CAMCOPS MIGRATION SCRIPT env.py")
    run_alembic()
