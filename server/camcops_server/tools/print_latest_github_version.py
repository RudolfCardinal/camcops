#!/usr/bin/env python

"""
camcops_server/tools/print_latest_github_version.py

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

**Print latest version tag found in CamCOPS repository.**

Test code for version sorting:

.. code-block:: python

    import random
    from semantic_version import Version
    version_strings = [
        "2.3.9",
        "2.3.10",
        "2.3.11",
    ]
    versions = [Version(v) for v in version_strings]
    random.shuffle(versions)
    versions.sort()
    print(versions)

"""

import logging
import subprocess
import sys
from typing import List

from cardinal_pythonlib.logs import main_only_quicksetup_rootlogger
from semantic_version import Version

log = logging.getLogger(__name__)


REPOSITORY = "https://github.com/RudolfCardinal/camcops"


def main() -> None:
    log.info(f"Checking repository: {REPOSITORY}")
    cmdargs = [
        "git",
        "ls-remote",
        "--tag",
        REPOSITORY
    ]
    output = subprocess.check_output(cmdargs).decode(sys.getdefaultencoding())
    log.debug(f"Tags found:\n{output}")
    version_strings = [
        line.split()[1].replace("refs/tags/v", "")
        for line in output.splitlines()
    ]
    versions = []  # type: List[Version]
    for v in version_strings:
        try:
            versions.append(Version(v))
        except ValueError:
            pass  # e.g. "2.3.0-server^{}"
    versions.sort()
    if not versions:
        log.error("No versions found", file=sys.stderr)
        sys.exit(1)
    latest_version = versions[-1]
    log.info(f"Latest version: {latest_version}")
    print(latest_version)
    sys.exit(0)


if __name__ == "__main__":
    main_only_quicksetup_rootlogger(level=logging.DEBUG)
    main()
