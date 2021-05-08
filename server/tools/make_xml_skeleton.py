#!/usr/bin/env python

"""
tools/make_xml_skeleton.py

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

**Creates an XML string file skeleton (with placeholder text) from a real
one.**

For developer use only.

"""

from typing import TextIO

import argparse
import re
import sys

STRING_RE = re.compile('(<string .*>).*(</string>)', re.MULTILINE)
#                       ^^^^^^^^^^^^^  ^^^^^^^^^^^
#                       capture 1      capture 2


def make_skeleton(xml_filename: str, replacement_text: str) -> str:
    """
    Creates the XML skeleton.

    Args:
        xml_filename: filename to read
        replacement_text: text to replace contents with
    """
    with open(xml_filename, "r") as file:
        original_content = file.read()
    replacement_re = fr"\1{replacement_text}\2"
    skeleton = STRING_RE.sub(replacement_re, original_content)
    return skeleton


def print_skeleton(xml_filename: str,
                   replacement_text: str,
                   outfile: TextIO) -> None:
    """
    Creates and prints the XML skeleton.

    Args:
        xml_filename: filename to read
        replacement_text: text to replace contents with
        outfile: output file, to write to
    """
    xml = make_skeleton(xml_filename, replacement_text)
    print(xml, file=outfile)


def main() -> None:
    """
    Command-line entry point.
    """
    # noinspection PyTypeChecker
    parser = argparse.ArgumentParser(
        "Create an XML string file skeleton, with placeholder text, from a "
        "real but secret string file. Writes to stdout.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "filename", type=str,
        help="Input filename"
    )
    parser.add_argument(
        "--replacement", type=str, default="XXX",
        help="Replace string contents with this"
    )
    args = parser.parse_args()
    print_skeleton(xml_filename=args.filename,
                   replacement_text=args.replacement,
                   outfile=sys.stdout)


if __name__ == "__main__":
    main()
