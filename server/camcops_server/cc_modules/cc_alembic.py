#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_alembic.py

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

**Functions to talk to Alembic; specifically, those functions that may be used
by users/administrators, such as to upgrade a database.**

If you're a developer and want to create a new database migration, see
``tools/create_database_migration.py`` instead.

"""

import logging
from typing import TYPE_CHECKING
import os

# from alembic import command
from alembic.config import Config
from cardinal_pythonlib.fileops import preserve_cwd
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.sqlalchemy.alembic_func import (
    # get_current_and_head_revision,
    downgrade_database,
    upgrade_database,
    stamp_allowing_unusual_version_table,
)

from camcops_server.cc_modules.cc_baseconstants import (
    ALEMBIC_BASE_DIR,
    ALEMBIC_CONFIG_FILENAME,
    ALEMBIC_VERSION_TABLE,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base

if TYPE_CHECKING:
    from sqlalchemy.sql.schema import MetaData
    from camcops_server.cc_modules.cc_config import CamcopsConfig

log = BraceStyleAdapter(logging.getLogger(__name__))


def import_all_models():
    """
    Imports all SQLAlchemy models. (This has side effects including setting up
    the SQLAlchemy metadata properly.)
    """
    # noinspection PyUnresolvedReferences
    import camcops_server.cc_modules.cc_all_models  # delayed import  # import side effects (ensure all models registered)  # noqa


def upgrade_database_to_head(show_sql_only: bool = False) -> None:
    """
    The primary upgrade method. Modifies the database structure from where it
    is, stepwise through revisions, to the head revision.

    Args:
        show_sql_only: just show the SQL; don't execute it
    """
    upgrade_database_to_revision(revision="head", show_sql_only=show_sql_only)


def upgrade_database_to_revision(revision: str,
                                 show_sql_only: bool = False) -> None:
    """
    Upgrades the database to a specific revision. Modifies the database
    structure from where it is, stepwise through revisions, to the specified
    revision.

    Args:
        revision: destination revision
        show_sql_only: just show the SQL; don't execute it
    """
    import_all_models()  # delayed, for command-line interfaces
    upgrade_database(alembic_base_dir=ALEMBIC_BASE_DIR,
                     alembic_config_filename=ALEMBIC_CONFIG_FILENAME,
                     destination_revision=revision,
                     version_table=ALEMBIC_VERSION_TABLE,
                     as_sql=show_sql_only)
    # ... will get its config information from the OS environment; see
    # run_alembic() in alembic/env.py


def downgrade_database_to_revision(revision: str,
                                   show_sql_only: bool = False,
                                   confirm_downgrade_db: bool = False) -> None:
    """
    Developer option. Takes the database to a specific revision.

    Args:
        revision: destination revision
        show_sql_only: just show the SQL; don't execute it
        confirm_downgrade_db: has the user confirmed? Necessary for the
            (destructive) database operation.
    """
    if not show_sql_only and not confirm_downgrade_db:
        log.critical("Destructive action not confirmed! Refusing.")
        return
    if show_sql_only:
        log.warning("Current Alembic v1.0.0 bug in downgrading with "
                    "as_sql=True; may fail")
    import_all_models()  # delayed, for command-line interfaces
    downgrade_database(alembic_base_dir=ALEMBIC_BASE_DIR,
                       alembic_config_filename=ALEMBIC_CONFIG_FILENAME,
                       destination_revision=revision,
                       version_table=ALEMBIC_VERSION_TABLE,
                       as_sql=show_sql_only)
    # ... will get its config information from the OS environment; see
    # run_alembic() in alembic/env.py


@preserve_cwd
def create_database_from_scratch(cfg: "CamcopsConfig") -> None:
    """
    Takes the database from nothing to the "head" revision in one step, by
    bypassing Alembic's revisions and taking the state directly from the
    SQLAlchemy ORM metadata.

    See
    https://alembic.zzzcomputing.com/en/latest/cookbook.html#building-an-up-to-date-database-from-scratch

    This function ASSUMES that the head revision "frozen" into the latest
    ``alembic/version/XXX.py`` file MATCHES THE STATE OF THE SQLALCHEMY ORM
    METADATA as judged by ``Base.metadata``. If that's not the case, things
    will go awry later! (Alembic will think the database is at the state of its
    "head" revision, but it won't be.)

    It also ASSUMES (as many things do) that importing ``.cc_all_models``
    imports all the models (or ``Base.metadata`` will be incomplete).
    """  # noqa
    import_all_models()  # delayed, for command-line interfaces

    log.warning("Performing one-step database creation.")
    metadata = Base.metadata  # type: MetaData
    engine = cfg.get_sqla_engine()
    metadata.create_all(engine)

    alembic_cfg = Config(ALEMBIC_CONFIG_FILENAME)
    os.chdir(ALEMBIC_BASE_DIR)
    # command.stamp(alembic_cfg, "head")
    stamp_allowing_unusual_version_table(alembic_cfg, "head",
                                         version_table=ALEMBIC_VERSION_TABLE)
    log.info("One-step database creation complete.")
