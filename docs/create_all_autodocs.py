#!/usr/bin/env python
# docs/create_all_autodocs.py

"""
..

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

..
"""

import argparse
import logging
import os
import sys
from typing import List

from cardinal_pythonlib.fileops import rmtree
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)
from cardinal_pythonlib.sphinxtools import AutodocIndex, AutodocMethod

from camcops_server.cc_modules.cc_pythonversion import assert_minimum_python_version  # noqa

assert_minimum_python_version()

log = BraceStyleAdapter(logging.getLogger(__name__))

# Work out directories
THIS_DIR = os.path.dirname(os.path.realpath(__file__))  # .../docs
PROJECT_ROOT_DIR = os.path.abspath(os.path.join(THIS_DIR, os.pardir))  # .../
PYTHON_PACKAGE_ROOT_DIR = os.path.join(PROJECT_ROOT_DIR, "server")
CODE_ROOT_DIR = PROJECT_ROOT_DIR
AUTODOC_DIR = os.path.join(THIS_DIR, "source", "autodoc")
INDEX_FILENAME = "_index.rst"
TOP_AUTODOC_INDEX = os.path.join(AUTODOC_DIR, INDEX_FILENAME)

COPYRIGHT_COMMENT = r"""
..  Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).
    .
    This file is part of CamCOPS.
    .
    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    .
    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    .
    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.
"""
AUTODOC_TITLE = "Source code"
INTRODUCTORY_RST = r"""

This section contains automatic documentation generated from the CamCOPS
source code. For the source code itself, see
https://github.com/RudolfCardinal/camcops.

"""

PYGMENTS_OVERRIDE = {
    # map file extension to Pygments language name
    ".pro": "none",  # Qt project files, not Prolog
}


def make_subindex(directory: str,
                  skip_globs: List[str] = None,
                  method: AutodocMethod = AutodocMethod.BEST) -> AutodocIndex:
    return AutodocIndex(
        index_filename=os.path.join(AUTODOC_DIR, directory, INDEX_FILENAME),
        project_root_dir=PROJECT_ROOT_DIR,
        python_package_root_dir=PYTHON_PACKAGE_ROOT_DIR,
        autodoc_rst_root_dir=AUTODOC_DIR,
        highest_code_dir=CODE_ROOT_DIR,
        source_filenames_or_globs=[
            os.path.join(CODE_ROOT_DIR, directory, "**/*.cpp"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.css"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.h"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.iss"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.java"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.mako"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.pro"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.py"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.qml"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.rc"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.sci"),
            os.path.join(CODE_ROOT_DIR, directory, "**/*.xml"),
        ],
        rst_prefix=COPYRIGHT_COMMENT,
        title=directory,  # path style, not module style
        skip_globs=skip_globs,
        # source_rst_title_style_python=False,
        index_heading_underline_char="~",
        source_rst_heading_underline_char="^",
        method=method,
        pygments_language_override=PYGMENTS_OVERRIDE
    )


def make_autodoc(make: bool, destroy_first: bool) -> None:
    if destroy_first:
        if make and os.path.exists(AUTODOC_DIR):
            log.info("Deleting directory {!r}", AUTODOC_DIR)
            rmtree(AUTODOC_DIR)
        else:
            log.warning("Would delete directory {!r} (not doing so as in mock "
                        "mode)", AUTODOC_DIR)
    top_idx = AutodocIndex(
        index_filename=TOP_AUTODOC_INDEX,
        project_root_dir=PROJECT_ROOT_DIR,
        python_package_root_dir=PYTHON_PACKAGE_ROOT_DIR,
        autodoc_rst_root_dir=AUTODOC_DIR,
        highest_code_dir=CODE_ROOT_DIR,
        toctree_maxdepth=1,
        rst_prefix=COPYRIGHT_COMMENT,
        index_heading_underline_char="-",
        source_rst_heading_underline_char="~",
        title=AUTODOC_TITLE,
        introductory_rst=INTRODUCTORY_RST,
        pygments_language_override=PYGMENTS_OVERRIDE,
    )
    top_idx.add_indexes([
        make_subindex("tablet_qt",
                      method=AutodocMethod.CONTENTS),
        make_subindex(os.path.join("server", "camcops_server")),
    ])
    top_idx.write_index_and_rst_files(overwrite=True, mock=not make)
    # print(top_idx.index_content())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--make", action="store_true",
        help="Do things! Otherwise will just show its intent.")
    parser.add_argument(
        "--destroy_first", action="store_true",
        help="Destroy all existing autodocs first")
    parser.add_argument(
        "--verbose", action="store_true",
        help="Be verbose")
    args = parser.parse_args()

    main_only_quicksetup_rootlogger(
        level=logging.DEBUG if args.verbose else logging.INFO)

    make_autodoc(make=args.make,
                 destroy_first=args.destroy_first)


if __name__ == '__main__':
    main()
