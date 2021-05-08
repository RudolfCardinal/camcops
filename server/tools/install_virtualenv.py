#!/usr/bin/env python

"""
tools/install_virtualenv.py

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

**Creates a Python virtual environment.**

As of 2018-11-24, uses ``venv`` not ``virtualenv``; see
https://www.reddit.com/r/learnpython/comments/4hsudz/pyvenv_vs_virtualenv/.
This is present in the Python system library from Python 3.3+

"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from typing import List

try:
    import distro
except ImportError:
    distro = None

assert sys.version_info >= (3, 6), "Need Python 3.6 or higher"
LINUX = platform.system() == 'Linux'
if distro:
    LINUX_DIST = distro.linux_distribution()[0].lower()
else:
    LINUX_DIST = ''
DEB = LINUX_DIST in ['ubuntu', 'debian']
RPM = LINUX_DIST in ['fedora', 'rhel', 'centos']

DESCRIPTION = """
Make a new virtual environment.
Please specify the directory in which the virtual environment should be
created. For example, for a testing environment
    {script} ~/MYPROJECT_virtualenv

or for a production environment:
    sudo --user=www-data XDG_CACHE_HOME=/usr/share/MYPROJECT/.cache \\
        {script} /usr/share/MYPROJECT/virtualenv
""".format(script=os.path.basename(__file__))

PYTHON = sys.executable  # Windows needs this before Python executables
PYTHONBASE = os.path.basename(PYTHON)
PIP = shutil.which('pip3')

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_BASE_DIR = os.path.abspath(os.path.join(CURRENT_DIR, os.pardir))
# PIP_REQ_FILE = os.path.join(PROJECT_BASE_DIR, 'requirements-pip.txt')
DEB_REQ_FILE = os.path.join(PROJECT_BASE_DIR, 'requirements-deb.txt')
RPM_REQ_FILE = os.path.join(PROJECT_BASE_DIR, 'requirements-rpm.txt')

SEP = "=" * 79


def title(msg: str) -> None:
    """
    Prints a title.
    """
    print(SEP)
    print(msg)
    print(SEP)


def get_lines_without_comments(filename: str) -> List[str]:
    """
    Retrieves all non-comment lines from a file.
    """
    lines = []
    with open(filename) as f:
        for line in f:
            line = line.partition('#')[0]
            line = line.rstrip()
            line = line.lstrip()
            if line:
                lines.append(line)
    return lines


def cmd_returns_zero_success(cmdargs: List[str]) -> bool:
    """
    Runs a command; returns True if it succeeded and False if it failed.
    """
    print("Checking result of command: {}".format(cmdargs))
    try:
        subprocess.check_call(cmdargs)
        return True
    except subprocess.CalledProcessError:
        return False


def check_call(cmdargs: List[str]) -> None:
    """
    Displays its intent, executes a command, and checks that it succeeded.
    """
    print("Command: {}".format(cmdargs))
    subprocess.check_call(cmdargs)


def require_deb(package: str) -> None:
    """
    Ensure that a specific Debian package is installed.
    Exits the program entirely if it isn't.
    """
    if cmd_returns_zero_success(['dpkg', '-l', package]):
        return
    print("You must install the package {package}. On Ubuntu, use the command:"
          "\n"
          "    sudo apt-get install {package}".format(package=package))
    sys.exit(1)


def require_rpm(package: str) -> None:
    """
    Ensure that a specific RPM package is installed.
    Exits the program entirely if it isn't.
    """
    # PROBLEM: can't call yum inside yum. See --skippackagechecks option.
    if cmd_returns_zero_success(['yum', 'list', 'installed', package]):
        return
    print("You must install the package {package}. On CentOS, use the command:"
          "\n"
          "    sudo yum install {package}".format(package=package))
    sys.exit(1)


def main() -> None:
    """
    Create a virtual environment. See DESCRIPTION.
    """
    if not LINUX:
        raise AssertionError("Installation requires Linux.")
    # noinspection PyTypeChecker
    parser = argparse.ArgumentParser(
        description=DESCRIPTION,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("virtualenv", help="New virtual environment directory")
    parser.add_argument("--virtualenv_minimum_version", default="13.1.2",
                        help="Minimum version of virtualenv tool")
    parser.add_argument("--skippackagechecks", action="store_true",
                        help="Skip verification of system packages (use this "
                             "when calling script from a yum install, for "
                             "example).")
    args = parser.parse_args()

    # venv_tool = 'virtualenv'
    venv_tool = 'venv'  # Python 3.3+
    venv_python = os.path.join(args.virtualenv, 'bin', 'python')
    venv_pip = os.path.join(args.virtualenv, 'bin', 'pip')
    activate = "source " + os.path.join(args.virtualenv, 'bin', 'activate')

    print("XDG_CACHE_HOME: {}".format(os.environ.get('XDG_CACHE_HOME',
                                                     None)))
    if not args.skippackagechecks:
        if DEB:
            title("Prerequisites, from " + DEB_REQ_FILE)
            packages = get_lines_without_comments(DEB_REQ_FILE)
            for pkg in packages:
                require_deb(pkg)
        elif RPM:
            title("Prerequisites, from " + RPM_REQ_FILE)
            packages = get_lines_without_comments(RPM_REQ_FILE)
            for pkg in packages:
                require_rpm(pkg)
        else:
            raise AssertionError("Not DEB, not RPM; don't know what to do")
        print('OK')

    # title("Ensuring virtualenv is installed for system"
    #       " Python ({})".format(PYTHON))
    # check_call([PIP, 'install',
    #             'virtualenv>={}'.format(args.virtualenv_minimum_version)])
    # print('OK')

    title("Using system Python ({}) and venv ({}) to make {}".format(
          PYTHON, venv_tool, args.virtualenv))
    check_call([PYTHON, '-m', venv_tool, args.virtualenv])
    print('OK')

    title("Upgrading pip within virtualenv")
    check_call([venv_pip, 'install', '--upgrade', 'pip'])

    title("Checking version of tools within new virtualenv")
    print(venv_python)
    check_call([venv_python, '--version'])
    print(venv_pip)
    check_call([venv_pip, '--version'])

    # title("Use pip within the new virtualenv to install dependencies")
    # check_call([venv_pip, 'install', '-r', PIP_REQ_FILE])
    # print('OK')
    # print('--- Virtual environment installed successfully')

    print("To activate the virtual environment, use\n"
          "    {activate}\n\n".format(activate=activate))


if __name__ == '__main__':
    main()
