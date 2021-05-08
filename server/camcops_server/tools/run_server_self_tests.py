#!/usr/bin/env python

"""
camcops_server/tools/run_server_self_tests.py

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

**Run server self-tests.**

"""

import os
import subprocess

from camcops_server.cc_modules.cc_baseconstants import CAMCOPS_SERVER_DIRECTORY


DESCRIPTION = f"""- You can run tests manually via one of these methods:
    pytest                                              # all tests
    pytest FILE.py                                      # one test file
    pytest FILE.py::some_function                       # one test function
    pytest FILE.py::SomeClass                           # one test class
    pytest FILE.py::SomeClass::some_member_function     # function within class
    pytest -k search_term                               # all test functions,
                                                        # classes etc matching
- Pytest will find additional options in:
    {CAMCOPS_SERVER_DIRECTORY}/conftest.py
    {CAMCOPS_SERVER_DIRECTORY}/pytest.ini
- Note that an SQLite database is saved in
    {CAMCOPS_SERVER_DIRECTORY}
- We'll launch pytest now for the full test suite."""
# https://stackoverflow.com/questions/36456920
# examples:
# pytest client_api_tests.py::ClientApiTests::test_client_api_validators
# pytest webview_tests.py::WebviewTests::test_webview_constant_validators
# pytest cc_validator_tests.py


def main() -> None:
    print(DESCRIPTION)
    os.chdir(CAMCOPS_SERVER_DIRECTORY)
    subprocess.run("pytest", shell=True)


if __name__ == "__main__":
    main()
