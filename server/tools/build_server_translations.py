#!/usr/bin/env python

"""
tools/build_server_translations.py

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

**Make translation files for the CamCOPS server via Babel.**

For developer use only.

"""

import argparse
import logging
from os.path import abspath, dirname, isfile, join
from typing import List

from cardinal_pythonlib.argparse_func import (
    RawDescriptionArgumentDefaultsHelpFormatter,
)
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)
from cardinal_pythonlib.subproc import check_call_verbose

from camcops_server.cc_modules.cc_baseconstants import (
    CAMCOPS_SERVER_DIRECTORY,
    TRANSLATIONS_DIR,
)
from camcops_server.cc_modules.cc_version_string import (
    CAMCOPS_SERVER_VERSION_STRING,
)
from camcops_server.cc_modules.cc_language import (
    DEFAULT_LOCALE,
    GETTEXT_DOMAIN as DOMAIN,
    POSSIBLE_LOCALES,
)

log = BraceStyleAdapter(logging.getLogger(__name__))

CURRENT_DIR = dirname(abspath(__file__))  # camcops/server/tools
POT_FILE = join(TRANSLATIONS_DIR, "camcops_translations.pot")
MAPPING_FILE = join(CURRENT_DIR, "babel.cfg")
SOURCE_DIRS = [CAMCOPS_SERVER_DIRECTORY]  # it recurses below this directory

PROJECT_NAME = "CamCOPS"
COPYRIGHT_HOLDER = "Rudolf Cardinal"
MSGID_BUGS_ADDR = "rudolf@pobox.com"
CHARSET = "utf-8"

OP_EXTRACT = "extract"
OP_INIT_MISSING = "init_missing"
OP_UPDATE = "update"
OP_COMPILE = "compile"
OP_ALL = "all"
ALL_OPERATIONS = [OP_EXTRACT, OP_INIT_MISSING, OP_UPDATE, OP_COMPILE, OP_ALL]

LOCALES = [_ for _ in POSSIBLE_LOCALES if _ != DEFAULT_LOCALE]
LC_MESSAGES = "LC_MESSAGES"
COMMENT_TAGS = ["TRANSLATOR:"]


def run(cmdargs: List[str]) -> None:
    """
    Runs a sub-command.

    Raises:
        :exc:CalledProcessError on failure

    Note that it is *critical* to abort on error; otherwise, for example, a
    process breaks the .pot file, and then this script chugs on and uses the
    broken .po file to break (for example) your Danish .po file.
    """
    check_call_verbose(cmdargs)


def get_po_basefilename(locale: str) -> str:
    """
    Returns the base filename of the ``.po`` file for a given locale.
    This should include "camcops" and the locale name, so that it's obvious
    when using a PO file editor.
    """
    return f"{DOMAIN}_{locale}.po"


def get_po_filename(locale: str) -> str:
    """
    Returns the full-path filename of the ``.po`` file for a given locale.
    """
    return join(TRANSLATIONS_DIR, locale, LC_MESSAGES,
                get_po_basefilename(locale))


def get_mo_basefilename() -> str:
    """
    Returns the base filename of the ``.mo`` file for a given locale.
    See :func:`get_mo_filename`; this is constrained.
    """
    return f"{DOMAIN}.mo"


def get_mo_filename(locale: str) -> str:
    """
    Returns the full-path filename of the ``.mo`` file for a given locale.

    Note that Python's ``gettext`` module, specifically ``gettext.find``,
    explicitly expects ``localedir/lang/LC_MESSAGES/<domain>.mo``.
    """
    return join(TRANSLATIONS_DIR, locale, LC_MESSAGES, get_mo_basefilename())


def main() -> None:
    """
    Create translation files for the CamCOPS server.
    """  # noqa
    # noinspection PyTypeChecker
    parser = argparse.ArgumentParser(
        description=f"""
Create translation files for CamCOPS server. CamCOPS knows about the following
locales:

    {LOCALES}

Operations:

    {OP_EXTRACT}
        Extract strings from code that looks like, for example,
            _("please translate me")
        in Python and Mako files. Write the strings to this .pot file:
            {POT_FILE}
        
    {OP_INIT_MISSING}
        For any locales that do not have a .po file, create one.

    {OP_UPDATE}
        Updates all .po files from the .pot file.

    [At this stage, edit the .po files with Poedit or similar.]
        
    {OP_COMPILE}
        Converts each .po file to an equivalent .mo file.
    
    {OP_ALL}
        Executes all other operations in sequence.""",
        formatter_class=RawDescriptionArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "operation", choices=ALL_OPERATIONS,
        metavar="operation",
        help=f"Operation to perform; possibilities are {ALL_OPERATIONS!r}"
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Be verbose"
    )
    args = parser.parse_args()
    main_only_quicksetup_rootlogger(
        level=logging.DEBUG if args.verbose else logging.INFO)
    op = args.operation  # type: str

    pybabel = "pybabel"

    if op in [OP_EXTRACT, OP_ALL]:
        log.info(f"EXTRACT: from code to a .pot file: {POT_FILE}")
        comment_tags_csv = ",".join(COMMENT_TAGS)
        cmdargs = [
            pybabel, "extract",
            # "--help",
            f"--charset={CHARSET}",
            f"--mapping-file={MAPPING_FILE}",
            f"--output-file={POT_FILE}",
            "--no-wrap",
            f"--msgid-bugs-address={MSGID_BUGS_ADDR}",
            f"--copyright-holder={COPYRIGHT_HOLDER}",
            f"--project={PROJECT_NAME}",
            f"--version={CAMCOPS_SERVER_VERSION_STRING}",
            f"--add-comments={comment_tags_csv}"
        ] + SOURCE_DIRS
        run(cmdargs)

    if op in [OP_INIT_MISSING, OP_ALL]:
        # Note that "pybabel init" will overwrite existing files, so don't
        # initialize if the .po file already exists.
        for locale in LOCALES:
            po_filename = get_po_filename(locale)
            if isfile(po_filename):
                log.debug(f"Skipping init for existing .po file: {po_filename}")
                continue
            log.info(f"Making new .po file for {locale}: {po_filename}")
            cmdargs = [
                pybabel, "init",
                f"--domain={DOMAIN}",
                f"--input-file={POT_FILE}",
                f"--locale={locale}",
                f"--output-file={po_filename}",
                "--no-wrap"
            ]
            run(cmdargs)

    if op in [OP_UPDATE, OP_ALL]:
        for locale in LOCALES:
            po_filename = get_po_filename(locale)
            if not isfile(po_filename):
                log.warning(f"Missing .po file: {po_filename}")
                continue
            log.info(f"Using .pot file to update .po file: {po_filename}")
            cmdargs = [
                pybabel, "update",
                f"--domain={DOMAIN}",
                f"--input-file={POT_FILE}",
                f"--output-file={po_filename}",
                f"--locale={locale}",
                "--no-wrap",
                "--ignore-obsolete",
                "--previous",
            ]
            run(cmdargs)

    if op in [OP_COMPILE, OP_ALL]:
        for locale in LOCALES:
            po_filename = get_po_filename(locale)
            mo_filename = get_mo_filename(locale)
            log.info(f"Compiling .po file to .mo file: {mo_filename}")
            cmdargs = [
                pybabel, "compile",
                f"--domain={DOMAIN}",
                f"--input-file={po_filename}",
                f"--output-file={mo_filename}",
                f"--locale={locale}",
                "--statistics",
            ]
            run(cmdargs)


if __name__ == "__main__":
    main()
