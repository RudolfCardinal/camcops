#!/usr/bin/env python

"""
tools/MAKE_LINUX_PACKAGES.py

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

**Creates Debian and RPM packages for Linux.**

For CentOS, you need to get Python 3 installed. For Centos 6, 64-bit:

.. code-block:: bash

    # http://stackoverflow.com/questions/8087184/installing-python3-on-rhel

    sudo yum install https://centos6.iuscommunity.org/ius-release.rpm

then for Python 3.4:

.. code-block:: bash

    sudo yum install python34u

or for Python 3.5 (with some other helpful things):

.. code-block:: bash

    sudo yum install python35u python35u-pip libxml2-devel libxslt-devel python35u-devel gcc

Note that you can get CentOS version/architecture with:

.. code-block:: bash

    cat /etc/centos-release
    uname -a

"""  # noqa

# We could use a temporary directory for the Debian build,
# but it's helpful to be able to see what it's doing as well.
# ... actually, let's do that, using mkdtemp(), so it'll linger if the build
# fails.

import argparse
import getpass
import logging
import os
from os.path import join
import shutil
import subprocess
import sys
import tempfile
from typing import List

from cardinal_pythonlib.datetimefunc import get_now_localtz_pendulum
from cardinal_pythonlib.file_io import (
    get_lines_without_comments,
    remove_gzip_timestamp,
    write_text,
    write_gzipped_text,
)
from cardinal_pythonlib.fileops import copyglob, mkdir_p
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)

from camcops_server.cc_modules.cc_baseconstants import (
    LINUX_DEFAULT_CAMCOPS_CONFIG_DIR,
    LINUX_DEFAULT_CAMCOPS_DIR,
    LINUX_DEFAULT_LOCK_DIR,
    LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR,
)
from camcops_server.cc_modules.cc_pythonversion import assert_minimum_python_version  # noqa
from camcops_server.cc_modules.cc_version_string import (
    CAMCOPS_SERVER_VERSION_STRING,
    CAMCOPS_SERVER_CHANGEDATE,
)

log = BraceStyleAdapter(logging.getLogger(__name__))

# =============================================================================
# Python version requirements
# =============================================================================

assert_minimum_python_version()


# =============================================================================
# URL defaults and other constants
# =============================================================================

PACKAGE_DEB_NAME = "camcops-server"  # no underscores for Debian
PACKAGE_DIR_NAME = "camcops_server"
ETC_DIR_NAME = "camcops"
CAMCOPS_EXECUTABLE = "camcops_server"


# =============================================================================
# Python helper functions
# =============================================================================

def workpath(workdir: str, destpath: str) -> str:
    """Suppose
        workdir == '/home/myuser/debianbuilding'
        destpath == '/usr/lib/mylib'
    then returns:
        '/home/myuser/debianbuilding/usr/lib/mylib'
    """
    if destpath[0] == os.sep:
        return join(workdir, destpath[1:])
    else:
        return join(workdir, destpath)


def call(cmdargs: List[str], **kwargs) -> None:
    if kwargs:
        log.debug("With kwargs to subprocess.check_call of {!r}", kwargs)
    log.debug("Calling external program: {!r}", cmdargs)
    subprocess.check_call(cmdargs, **kwargs)


# =============================================================================
# Bash helper functions, to go into other scripts
# =============================================================================

BASHFUNC = r"""

#------------------------------------------------------------------------------
# Helper functions
#------------------------------------------------------------------------------

command_exists()
{
    # arguments: $1 is the command to test
    # returns 0 (true) for found, 1 (false) for not found
    if command -v $1 >/dev/null; then return 0; else return 1; fi
}

running_centos()
{
    if [ -f /etc/system-release ] ; then
        SYSTEM=`cat /etc/system-release | cut -d' ' -f1`
        # VERSION=`cat /etc/system-release | cut -d' ' -f3`
        # SYSTEM_ID=\$SYSTEM-\$VERSION
        if ["$SYSTEM" == "CentOS"] ; then
            return 0  # true
        fi
    fi
    return 1  # false
}

service_exists()
{
    # arguments: $1 is the service being tested

    servicename=$1

    # Ubuntu used to give "unrecognized service"
    # ... and now gives "not-found" (16.10)
    # ... or "Unit <something>.service could not be found" (18.04)
    # So a bad method is to grep for multiple patterns [1] like this:
    #
    # if service $1 status 2>&1 | grep -E 'unrecognized service|not-found' >/dev/null ; then
    #     return 1  # false
    # fi
    # return 0  # true

    # And a better way: return codes; will be 4 for an unknown service [2, 3]:

    service $servicename status >/dev/null 2>&1
    exitcode=$?
    if [ $exitcode -eq 4 ]; then
        # 4 means "does not exist"
        echo "Service $servicename does not exist"
        return 1  # false
    fi
    # Other codes mean e.g. "running" (0) or "not running" (e.g. 1, 2, 3)
    echo "Service $servicename exists"
    return 0  # true

    # [1] http://unix.stackexchange.com/questions/37313/how-do-i-grep-for-multiple-patterns
    # [2] https://unix.stackexchange.com/questions/226484/does-an-init-script-always-return-a-proper-exit-code-when-running-status
    # [3] http://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/iniscrptact.html
}

service_supervisord_command()
{
    # argument: argument to "service", such as "start", "stop", "restart"

    # The exact supervisor program name is impossible to predict (e.g. in
    # "supervisorctl stop camcops-gunicorn"), as it's user-defined, so we just
    # start/stop everything.
    # Ubuntu: service supervisor
    # CentOS: service supervisord

    cmd=$1
    if service_exists supervisord ; then
        echo "Executing: service supervisord $cmd"
        service supervisord $cmd || echo "Can't $cmd supervisord"
    else
        if service_exists supervisor ; then
            echo "Executing: service supervisor $cmd"
            service supervisor $cmd || echo "Can't $cmd supervisor"
        else
            echo "Don't know which supervisor/supervisord service to $cmd"
        fi
    fi
}

stop_supervisord()
{
    service_supervisord_command stop
}

restart_supervisord()
{
    service_supervisord_command restart
}

system_python_executable()
{
    # Echoes the preferred Python executable on the destination system.
    # Use as: $(system_python_executable) ...

    python_options=(python3.6 python36 python3 python)
    for option in ${python_options[@]}; do
        python_exe=$(which $option)
        if [ ! -z "${python_exe}" ]; then
            >&2 echo "CamCOPS: found system Python at ${python_exe}"
            echo ${python_exe}
            return 0  # success
        fi
    done
    >&2 echo "Error: CamCOPS unable to determine system Python. Tried: ${python_options[@]}"
}


"""  # noqa


# =============================================================================
# Directory constants
# =============================================================================

THIS_DIR = os.path.dirname(os.path.abspath(__file__))  # tools
SETUP_PY_DIR = os.path.abspath(join(THIS_DIR, os.pardir))
PROJECT_BASE_DIR = os.path.abspath(join(THIS_DIR, os.pardir, os.pardir))

DSTBASEDIR = LINUX_DEFAULT_CAMCOPS_DIR

TMPDIR = tempfile.mkdtemp()
log.info("Temporary working directory: " + TMPDIR)
WRKDIR = join(TMPDIR, 'debian')
RPMTOPDIR = join(TMPDIR, 'rpmbuild')

SRCSERVERDIR = join(PROJECT_BASE_DIR, 'server')
DOCROOTDIR = join(PROJECT_BASE_DIR, 'documentation')
PACKAGEDIR = join(SRCSERVERDIR, 'packagebuild')

DSTDOCDIR = join('/usr/share/doc', PACKAGE_DEB_NAME)
WRKDOCDIR = workpath(WRKDIR, DSTDOCDIR)

WRKBASEDIR = workpath(WRKDIR, DSTBASEDIR)

DEBDIR = join(WRKDIR, 'DEBIAN')
# ... where Debian package control information lives
DEBOVERRIDEDIR = workpath(WRKDIR, '/usr/share/lintian/overrides')

DSTCONSOLEFILEDIR = '/usr/bin'
SETUPSCRIPTNAME = CAMCOPS_EXECUTABLE
WRKCONSOLEFILEDIR = workpath(WRKDIR, DSTCONSOLEFILEDIR)

DSTTEMPDIR = join(DSTBASEDIR, 'tmp')

SRCTOOLDIR = join(SRCSERVERDIR, 'tools')
WRKTOOLDIR = join(WRKBASEDIR, 'tools')
DSTTOOLDIR = join(DSTBASEDIR, 'tools')
VENVSCRIPT = 'install_virtualenv.py'
DSTVENVSCRIPT = join(DSTTOOLDIR, VENVSCRIPT)

METASCRIPTNAME = '{}_meta'.format(CAMCOPS_EXECUTABLE)

DSTMANDIR = '/usr/share/man/man1'  # section 1 for user commands
WRKMANDIR = workpath(WRKDIR, DSTMANDIR)

DSTCONFIGDIR = join('/etc', ETC_DIR_NAME)
WRKCONFIGDIR = workpath(WRKDIR, DSTCONFIGDIR)

DSTDPKGDIR = '/var/lib/dpkg/info'

DSTLOCKDIR = LINUX_DEFAULT_LOCK_DIR
DSTPYTHONVENV = join(DSTBASEDIR, 'venv')
DSTVENVBIN = join(DSTPYTHONVENV, 'bin')
DSTPYTHONCACHE = join(DSTBASEDIR, '.cache')

DSTMPLCONFIGDIR = LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR
WRKMPLCONFIGDIR = workpath(WRKDIR, DSTMPLCONFIGDIR)

# =============================================================================
# File constants
# =============================================================================

DSTCONSOLEFILE = join(DSTCONSOLEFILEDIR, SETUPSCRIPTNAME)
WRKCONSOLEFILE = join(WRKCONSOLEFILEDIR, SETUPSCRIPTNAME)

WRKMETACONSOLEFILE = join(WRKCONSOLEFILEDIR, METASCRIPTNAME)
DSTMETACONSOLEFILE = join(DSTCONSOLEFILEDIR, METASCRIPTNAME)

WRKMANFILE_BASE = join(WRKMANDIR, SETUPSCRIPTNAME + '.1')  # '.gz' appended
DSTMANFILE = join(DSTMANDIR, SETUPSCRIPTNAME + '.1.gz')
WRKMETAMANFILE_BASE = join(WRKMANDIR, METASCRIPTNAME + '.1')  # '.gz' appended
DSTMETAMANFILE = join(DSTMANDIR, METASCRIPTNAME + '.1.gz')

DSTREADME = join(DSTDOCDIR, 'README.txt')
WRKREADME = join(WRKDOCDIR, 'README.txt')

DEB_REQ_FILE = join(SRCSERVERDIR, 'requirements-deb.txt')
RPM_REQ_FILE = join(SRCSERVERDIR, 'requirements-rpm.txt')

DSTVENVPYTHON = join(DSTVENVBIN, 'python')
DSTVENVPIP = join(DSTVENVBIN, 'pip')

# For these names, see setup.py:
DST_CAMCOPS_LAUNCHER = join(DSTVENVBIN, 'camcops_server')
DST_CAMCOPS_META_LAUNCHER = join(DSTVENVBIN, 'camcops_server_meta')


# =============================================================================
# Version number and conditionals
# =============================================================================

MAINVERSION = CAMCOPS_SERVER_VERSION_STRING
CHANGEDATE = CAMCOPS_SERVER_CHANGEDATE

DEBVERSION = MAINVERSION + '-1'
PACKAGENAME = join(
    PACKAGEDIR,
    '{PACKAGE}_{DEBVERSION}_all.deb'.format(PACKAGE=PACKAGE_DEB_NAME,
                                            DEBVERSION=DEBVERSION)
)
# upstream_version-debian_revision --
# see http://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-Version


# =============================================================================
# Prerequisites
# =============================================================================

PREREQUISITES = ("alien dpkg-deb fakeroot find git gzip lintian "
                 "rpmrebuild".split())


def check_prerequisites() -> None:
    """
    Check prerequisites are in place.
    """
    # http://stackoverflow.com/questions/2806897
    if os.geteuid() == 0:
        log.critical(
            "This script should not be run using sudo or as the root user")
        sys.exit(1)

    log.info("Checking prerequisites")
    for cmd in PREREQUISITES:
        if shutil.which(cmd) is None:
            log.warning("""
    To install Alien:
        sudo apt-get install alien
    To install rpmrebuild:
        1. Download RPM from http://rpmrebuild.sourceforge.net/, e.g.
            cd /tmp
            wget http://downloads.sourceforge.net/project/rpmrebuild/rpmrebuild/2.11/rpmrebuild-2.11-1.noarch.rpm
        2. Convert to DEB:
            fakeroot alien --to-deb rpmrebuild-2.11-1.noarch.rpm
        3. Install:
            sudo dpkg --install rpmrebuild_2.11-2_all.deb
            """)  # noqa
            log.critical("{} command not found; stopping", cmd)
            sys.exit(1)

    # RPM issues
    # 1. A dummy camcops-prerequisites package works but is inelegant.
    # 2. Alien seems to strip dependencies.
    # 3. rpmrebuild does the job albeit not wholly intuitive documentation!
    #    It also allows you to see what Alien was doing.


# =============================================================================
# Debian control files
# =============================================================================

# -----------------------------------------------------------------------------
# man pages
# -----------------------------------------------------------------------------
# http://www.fnal.gov/docs/products/ups/ReferenceManual/html/manpages.html

def get_man_page_camcops_server() -> str:
    return r""".\" Manpage for {SETUPSCRIPTNAME}.
.\" Contact rudolf@pobox.com to correct errors or typos.
.TH man 1 "{CHANGEDATE}" "{MAINVERSION}" "{SETUPSCRIPTNAME} man page"

.SH NAME
{SETUPSCRIPTNAME} \- run the CamCOPS server command-line tool

.SH SYNOPSIS
.B {SETUPSCRIPTNAME} [
.I options
.B ] [
.I config-file
.B ]

.SH DESCRIPTION
The CamCOPS server command-line tool allows you to create tables and
superusers, launch the web service, and perform some data exports, test
functions, and user administration tasks. All other administration is via the
web interface.

There are prerequisites, such as setting up a database. See
https://camcops.readthedocs.io/.

For help, use

    camcops_server --help
    camcops_server docs

To create a demonstration config file, run

    camcops_server demo_camcops_config

Typically one would put the real configuration file in /etc/camcops/, and make
it readable only by the Apache user (typically www-data on Debian/Ubuntu
and apache on CentOS).

To create demonstration configuration files for support programs such as
supervisord and Apache, try

    camcops_server demo_supervisor_config
    camcops_server demo_apache_config

You will also need to point your web server (e.g. Apache) at the CamCOPS
program itself; see https://camcops.readthedocs.io/.

.SH FOR DETAILS
.IP "camcops_server --help"
show all options

.SH SEE ALSO
http://www.camcops.org/; https://camcops.readthedocs.io/

.SH AUTHOR
Rudolf Cardinal (rudolf@pobox.com)
    """.format(
        SETUPSCRIPTNAME=SETUPSCRIPTNAME,
        CHANGEDATE=CHANGEDATE,
        MAINVERSION=MAINVERSION,
    )


def get_man_page_camcops_server_meta() -> str:
    return r""".\" Manpage for {METASCRIPTNAME}.
.\" Contact rudolf@pobox.com to correct errors or typos.
.TH man 1 "{CHANGEDATE}" "{MAINVERSION}" "{METASCRIPTNAME} man page"

.SH NAME
{METASCRIPTNAME} \- run the CamCOPS server meta-command-line tool

.IP "{METASCRIPTNAME} --help"
show all options

.SH SEE ALSO
http://www.camcops.org/; https://camcops.readthedocs.io/

.SH AUTHOR
Rudolf Cardinal (rudolf@pobox.com)
    """.format(
        METASCRIPTNAME=METASCRIPTNAME,
        CHANGEDATE=CHANGEDATE,
        MAINVERSION=MAINVERSION,
    )


# -----------------------------------------------------------------------------
# README
# -----------------------------------------------------------------------------

def get_readme() -> str:
    return """
CamCOPS: the Cambridge Cognitive and Psychiatric Test Kit

See https://camcops.readthedocs.io/ for documentation.
    """


# -----------------------------------------------------------------------------
# control
# -----------------------------------------------------------------------------

def get_debian_control() -> str:
    depends_deb = get_lines_without_comments(DEB_REQ_FILE)
    return """Package: {PACKAGE}
Version: {DEBVERSION}
Section: science
Priority: optional
Architecture: all
Maintainer: Rudolf Cardinal <rudolf@pobox.com>
Depends: {DEPENDENCIES}
Recommends: mysql-workbench
Description: Cambridge Cognitive and Psychiatric Test Kit (CamCOPS), server
 packages.
 This package contains the files necessary to run a CamCOPS server and receive
 information from the CamCOPS tablet applications (desktop, Android, iOS).
 .
 For more details, see http://www.camcops.org/
""".format(
        PACKAGE=PACKAGE_DEB_NAME,
        DEBVERSION=DEBVERSION,
        DEPENDENCIES=", ".join(depends_deb),
    )


# -----------------------------------------------------------------------------
# changelog
# -----------------------------------------------------------------------------

DEBIAN_DATETIME_FORMAT = "%a, %d %b %Y %H:%M:%S %z"
# e.g. Tue, 17 Oct 2017 17:12:00 +0100
# strftime: http://man7.org/linux/man-pages/man3/strftime.3.html


def get_changelog() -> str:
    now = get_now_localtz_pendulum()
    return """camcops-server ({CAMCOPS_SERVER_VERSION_STRING}) all; urgency=low

  * Change date: {CAMCOPS_SERVER_CHANGEDATE}.
  * PLEASE SEE docs/source/changelog.rst FOR ALL CHANGES, or the online version
    at https://camcops.readthedocs.io/.
  * This file (changelog.Debian) has a very precise format:
    http://www.debian.org/doc/debian-policy/ch-source.html#s-dpkgchangelog
  * Note that newer entries are at the top.

 -- Rudolf Cardinal <rudolf@pobox.com>  {now}

    """.format(
        CAMCOPS_SERVER_VERSION_STRING=CAMCOPS_SERVER_VERSION_STRING,
        CAMCOPS_SERVER_CHANGEDATE=CAMCOPS_SERVER_CHANGEDATE,
        now=now.strftime(DEBIAN_DATETIME_FORMAT),
    )


# -----------------------------------------------------------------------------
# preinst
# -----------------------------------------------------------------------------

def get_preinst() -> str:
    # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=953428
    # #!/usr/bin/env bash not allowed?
    return """#!/bin/bash
# Exit on any errors? (Lintian strongly advises this.)
set -e

{BASHFUNC}

echo '{PACKAGE}: preinst file executing'

# Would be nice just to shut down camcops processes. But there can be
# several, on live systems, and it's hard to predict what they're called. We
# need them shut down if we're going to check/reinstall the virtual
# environment (otherwise it'll be busy). So:

stop_supervisord

echo '{PACKAGE}: preinst file finished'

    """.format(
        BASHFUNC=BASHFUNC,
        PACKAGE=PACKAGE_DEB_NAME,
    )


# -----------------------------------------------------------------------------
# postinst
# -----------------------------------------------------------------------------

def get_postinst(sdist_basefilename: str) -> str:
    dst_sdist_file = join(DSTBASEDIR, sdist_basefilename)
    # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=953428
    # #!/usr/bin/env bash not allowed?
    return """#!/bin/bash
# Exit on any errors? (Lintian strongly advises this.)
set -e

echo '{PACKAGE}: postinst file executing'

{BASHFUNC}

#------------------------------------------------------------------------------
# Install Python virtual environment with packages
#------------------------------------------------------------------------------

echo 'About to install virtual environment'
export XDG_CACHE_HOME={DSTPYTHONCACHE}
$(system_python_executable) {DSTVENVSCRIPT} {DSTPYTHONVENV} --skippackagechecks

echo 'About to install CamCOPS into virtual environment'
{DSTVENVPIP} install {dst_sdist_file}

#------------------------------------------------------------------------------
echo 'Creating lockfile directory'
#------------------------------------------------------------------------------

mkdir -p {DSTLOCKDIR}

#------------------------------------------------------------------------------
# Set permissions
#------------------------------------------------------------------------------
# Ubuntu: www-data
# CentOS: apache

APACHEUSER=""
getent passwd www-data >/dev/null 2>&1 && APACHEUSER=www-data
if [[ -z "$APACHEUSER" ]]; then
    getent passwd apache >/dev/null 2>&1 && APACHEUSER=apache
fi

echo '{PACKAGE}: Setting permissions'
if [[ ! -z "$APACHEUSER" ]]; then
    chown $APACHEUSER:$APACHEUSER {DSTLOCKDIR}
    chown $APACHEUSER:$APACHEUSER {DSTMPLCONFIGDIR}
fi

#------------------------------------------------------------------------------
# NO - GUNICORN FIXES THIS // If Apache is installed, restart it
#------------------------------------------------------------------------------
# if command_exists apachectl; then
#     # OK for Ubuntu (apachectl = link to apache2ctl), CentOS
#     echo '{PACKAGE}: Restarting Apache (apachectl restart)'
#     apachectl restart
# fi

#------------------------------------------------------------------------------
# Restart supervisor process(es)
#------------------------------------------------------------------------------

restart_supervisord

#------------------------------------------------------------------------------
# Other things that we don't want strict dependencies on, or can't install now
#------------------------------------------------------------------------------

echo "========================================================================"
echo "1.  You may also need to install:"
echo "    [Ubuntu]"
echo "        sudo apt-get install apache2 libapache2-mod-proxy-html libapache2-mod-xsendfile"
# libapache2-mod-wsgi
echo "        sudo a2enmod proxy_http  # may be unnecessary"
echo "        sudo apt-get install mysql-client mysql-server"
echo "    [CentOS]"
echo "        sudo yum install httpd mod_proxy mod_xsendfile"
# mod_wsgi
echo "        sudo yum install mysql55 mysql55-server libmysqlclient-dev"
echo
echo "2.  You also need to install a version of wkhtmltopdf with 'patched Qt',"
echo "    and at least version 0.12.2.1. Download and install a recent version"
echo "    from"
echo "        https://wkhtmltopdf.org/downloads.html"
echo "========================================================================"

echo '{PACKAGE}: postinst file finished'

    """.format(  # noqa
        BASHFUNC=BASHFUNC,
        DSTLOCKDIR=DSTLOCKDIR,
        DSTMPLCONFIGDIR=DSTMPLCONFIGDIR,
        DSTPYTHONCACHE=DSTPYTHONCACHE,
        DSTPYTHONVENV=DSTPYTHONVENV,
        dst_sdist_file=dst_sdist_file,
        DSTVENVPIP=DSTVENVPIP,
        DSTVENVSCRIPT=DSTVENVSCRIPT,
        PACKAGE=PACKAGE_DEB_NAME,
    )


# -----------------------------------------------------------------------------
# prerm
# -----------------------------------------------------------------------------

def get_prerm() -> str:
    # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=953428
    # #!/usr/bin/env bash not allowed?
    return """#!/bin/bash
set -e

{BASHFUNC}

echo '{PACKAGE}: prerm file executing'

stop_supervisord

# Must use -f or an error will cause the prerm (and package removal) to fail
# See /var/lib/dpkg/info/MYPACKAGE.prerm for manual removal!
find {DSTBASEDIR} -name '*.pyc' -delete
find {DSTBASEDIR} -name '*.pyo' -delete

# uninstall our package from venv (mainly for development, so we can re-use
# version numbers

{DSTVENVPIP} uninstall --yes camcops_server

echo '{PACKAGE}: prerm file finished'

    """.format(
        BASHFUNC=BASHFUNC,
        PACKAGE=PACKAGE_DEB_NAME,
        DSTBASEDIR=DSTBASEDIR,
        DSTVENVPIP=DSTVENVPIP,
    )


# -----------------------------------------------------------------------------
# postrm
# -----------------------------------------------------------------------------

def get_postrm() -> str:
    # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=953428
    # #!/usr/bin/env bash not allowed?
    return """#!/bin/bash
set -e

{BASHFUNC}

echo '{PACKAGE}: postrm file executing'

restart_supervisord

echo '{PACKAGE}: postrm file finished'

    """.format(
        BASHFUNC=BASHFUNC,
        PACKAGE=PACKAGE_DEB_NAME,
        DSTBASEDIR=DSTBASEDIR,
    )


# -----------------------------------------------------------------------------
# override
# -----------------------------------------------------------------------------

def get_override() -> str:
    return """
# Not an official new Debian package, so ignore this one.
# If we did want to close a new-package ITP bug:
# http://www.debian.org/doc/manuals/developers-reference/pkgs.html#upload-bugfix
{PACKAGE} binary: new-package-should-close-itp-bug
    """.format(  # noqa
        PACKAGE=PACKAGE_DEB_NAME,
    )


# -----------------------------------------------------------------------------
# copyright
# -----------------------------------------------------------------------------

def get_copyright() -> str:
    return """{PACKAGE}

CAMCOPS

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

    On Debian systems, see /usr/share/common-licenses/GPL-3

ADDITIONAL LIBRARY COMPONENTS

    Public domain or copyright (C) their respective authors (for details,
    see attribution in the source code and other license terms packaged with
    the source).

TEXT FOR SPECIFIC ASSESSMENT SCALES

    Public domain or copyright (C) their respective authors; see details in
    source code/string files distributed with this software.
    """.format(
        PACKAGE=PACKAGE_DEB_NAME,
        # DSTSTRINGFILE=DSTSTRINGFILE,
    )


# =============================================================================
# CamCOPS launch scripts
# =============================================================================

def get_camcops_server_launcher() -> str:
    return """#!/usr/bin/env bash
# Launch script for CamCOPS command-line tool.

echo 'Launching CamCOPS command-line tool...' >&2

{DST_CAMCOPS_LAUNCHER} "$@"

    """.format(
        DST_CAMCOPS_LAUNCHER=DST_CAMCOPS_LAUNCHER,
    )


def get_camcops_server_meta_launcher() -> str:
    return """#!/usr/bin/env bash
# Launch script for CamCOPS meta-command tool tool.

echo 'Launching CamCOPS meta-command tool...' >&2

{DST_CAMCOPS_META_LAUNCHER} "$@"

    """.format(
        DST_CAMCOPS_META_LAUNCHER=DST_CAMCOPS_META_LAUNCHER,
    )


# =============================================================================
# Build package
# =============================================================================

def build_package() -> None:
    """
    Builds the package.
    """
    log.info("Building Python package")

    setup_py = join(SRCSERVERDIR, 'setup.py')
    sdist_basefilename = ('camcops_server-{}.tar.gz'.format(MAINVERSION))
    src_sdist_file = join(SRCSERVERDIR, 'dist', sdist_basefilename)
    wrk_sdist_file = join(WRKBASEDIR, sdist_basefilename)

    try:
        log.info("Deleting old {} if it exists", src_sdist_file)
        os.remove(src_sdist_file)
    except OSError:
        pass
    os.chdir(SETUP_PY_DIR)  # or setup.py looks in wrong places?
    cmdargs = ['python', setup_py, 'sdist', '--extras']  # special!
    call(cmdargs)
    remove_gzip_timestamp(src_sdist_file)

    log.info("Making directories")
    mkdir_p(DEBDIR)
    mkdir_p(DEBOVERRIDEDIR)
    mkdir_p(PACKAGEDIR)
    mkdir_p(RPMTOPDIR)
    mkdir_p(WRKCONFIGDIR)
    mkdir_p(WRKCONSOLEFILEDIR)
    mkdir_p(WRKDIR)
    mkdir_p(WRKDOCDIR)
    mkdir_p(WRKMANDIR)
    mkdir_p(WRKMPLCONFIGDIR)
    mkdir_p(WRKBASEDIR)
    mkdir_p(WRKTOOLDIR)
    for d in "BUILD,BUILDROOT,RPMS,RPMS/noarch,SOURCES,SPECS,SRPMS".split(","):
        mkdir_p(join(RPMTOPDIR, d))

    log.info("Copying files")
    write_gzipped_text(join(WRKDOCDIR, 'changelog.Debian'), get_changelog())
    copyglob(join(SRCTOOLDIR, VENVSCRIPT), WRKTOOLDIR)
    shutil.copyfile(src_sdist_file, wrk_sdist_file)

    log.info("Creating man page for camcops. "
             "Will be installed as " + DSTMANFILE)
    write_gzipped_text(WRKMANFILE_BASE, get_man_page_camcops_server())

    log.info("Creating man page for camcops_server_meta. "
             "Will be installed as " + DSTMETAMANFILE)
    write_gzipped_text(WRKMETAMANFILE_BASE, get_man_page_camcops_server_meta())

    log.info("Creating links to documentation. "
             "Will be installed as " + DSTREADME)
    write_text(WRKREADME, get_readme())

    log.info("Creating camcops_server launch script. "
             "Will be installed as " + DSTCONSOLEFILE)
    write_text(WRKCONSOLEFILE, get_camcops_server_launcher())

    log.info("Creating camcops_server_meta launch script. "
             "Will be installed as " + DSTMETACONSOLEFILE)
    write_text(WRKMETACONSOLEFILE, get_camcops_server_meta_launcher())

    log.info("Creating Debian control file")

    write_text(join(DEBDIR, 'control'), get_debian_control())

    log.info("Creating preinst file. Will be installed as " +
             join(DSTDPKGDIR, PACKAGE_DEB_NAME + '.preinst'))
    write_text(join(DEBDIR, 'preinst'), get_preinst())

    log.info("Creating postinst file. Will be installed as " +
             join(DSTDPKGDIR, PACKAGE_DEB_NAME + '.postinst'))
    write_text(join(DEBDIR, 'postinst'), get_postinst(sdist_basefilename))

    log.info("Creating prerm file. Will be installed as " +
             join(DSTDPKGDIR, PACKAGE_DEB_NAME + '.prerm'))
    write_text(join(DEBDIR, 'prerm'), get_prerm())

    log.info("Creating postrm file. Will be installed as " +
             join(DSTDPKGDIR, PACKAGE_DEB_NAME + '.postrm'))
    write_text(join(DEBDIR, 'postrm'), get_postrm())

    log.info("Creating Lintian override file")
    write_text(join(DEBOVERRIDEDIR, PACKAGE_DEB_NAME), get_override())

    log.info("Creating copyright file. Will be installed as " +
             join(DSTDOCDIR, 'copyright'))
    write_text(join(WRKDOCDIR, 'copyright'), get_copyright())

    log.info("Setting ownership and permissions")
    call(['find', WRKDIR, '-type', 'd', '-exec', 'chmod', '755', '{}', ';'])
    # ... make directories executabe: must do that first, or all the subsequent
    # recursions fail
    call(['find', WRKDIR, '-type', 'f', '-exec', 'chmod', '644', '{}', ';'])
    call([
        "chmod",
        "a+x",
        WRKCONSOLEFILE,
        WRKMETACONSOLEFILE,
        join(DEBDIR, 'prerm'),
        join(DEBDIR, 'postrm'),
        join(DEBDIR, 'preinst'),
        join(DEBDIR, 'postinst'),
    ])
    call(['find', WRKDIR, '-iname', '*.py', '-exec', 'chmod', 'a+x', '{}', ';'])
    call(['find', WRKDIR, '-iname', '*.pl', '-exec', 'chmod', 'a+x', '{}', ';'])

    log.info("Removing junk")
    call(['find', WRKDIR, '-name', '*.svn', '-exec', 'rm', '-rf', '{}', ';'])
    call(['find', WRKDIR, '-name', '.git', '-exec', 'rm', '-rf', '{}', ';'])
    call(['find', WRKDOCDIR, '-name', 'LICENSE',
          '-exec', 'rm', '-rf', '{}', ';'])

    log.info("Building package")
    call(['fakeroot', 'dpkg-deb', '--build', WRKDIR, PACKAGENAME])
    # ... "fakeroot" prefix makes all files installed as root:root

    log.info("Checking with Lintian")
    call(['lintian', '--fail-on-warnings', PACKAGENAME])

    log.info("Converting to RPM")
    call(['fakeroot', 'alien', '--to-rpm', '--scripts', PACKAGENAME],
         cwd=PACKAGEDIR)
    # see "man alien"
    # NOTE: needs to be run as root for correct final permissions
    expected_main_rpm_name = "{PACKAGE}-{MAINVERSION}-2.noarch.rpm".format(
        PACKAGE=PACKAGE_DEB_NAME,
        MAINVERSION=MAINVERSION,
    )
    full_rpm_path = join(PACKAGEDIR, expected_main_rpm_name)
    myuser = getpass.getuser()
    shutil.chown(full_rpm_path, myuser, myuser)

    log.info("Changing dependencies within RPM")
    # Alien does not successfully translate the dependencies, and anyway the
    # names for packages are different on CentOS. A dummy prerequisite package
    # works (below) but is inelegant.
    # The rpmbuild commands are filters (text in via stdin, text out to
    # stdout), so replacement just needs the echo command.

    depends_rpm = get_lines_without_comments(RPM_REQ_FILE)
    echoparam = repr("Requires: {}".format(" ".join(depends_rpm)))
    call([
        'rpmrebuild',
        '--define', '_topdir ' + RPMTOPDIR,
        '--package',
        '--change-spec-requires=/bin/echo {}'.format(echoparam),
        full_rpm_path,
    ])
    # ... add "--edit-whole" as the last option before the RPM name to see what
    #     you're getting
    # ... define topdir, or it builds in ~/rpmbuild/...
    # ... --package, or it looks for an installed RPM rather than a package
    #     file
    # ... if echo parameter has brackets in, ensure it's quoted

    shutil.move(join(RPMTOPDIR, 'RPMS', 'noarch', expected_main_rpm_name),
                join(PACKAGEDIR, expected_main_rpm_name))
    # ... will overwrite its predecessor

    log.info("Deleting temporary workspace")
    shutil.rmtree(TMPDIR, ignore_errors=True)  # CAUTION!

    # Done
    log.info("=" * 79)
    log.info("Debian package should be: " + PACKAGENAME)
    log.info("RPM should be: " + full_rpm_path)


# =============================================================================
# Check command-line arguments +/- provide help
# =============================================================================

def main():
    """
    Command-line entry point.
    """
    check_prerequisites()

    parser = argparse.ArgumentParser(
        description="""
- Creates a Debian (.deb) and RPM (.rpm) distribution file for the CamCOPS
  server, for distribution under Linux.

- In brief, the following sequence is followed as the package is built:

  * The CamCOPS server is packaged up from source using
        python setup.py sdist --extras
            # ... where "--extras" is a special custom option that copies the
            # tablet source code and packages that, plus all static files
    and zipped in a Debian-safe way.

  * The principle is that the Python package should do all the work, not the
    Debian framework. This also means that a user who elects to install via pip
    gets exactly the same functional file structure.

  * A Debian package is built, containing all the usual Debian goodies (man
    packages, the preinst/postinst/prerm/postrm files, cataloguing and control
    files, etc.).

  * The package is checked with Lintian.

  * An RPM is built from the .deb package.

- The user then installs the DEB or RPM file. In addition to installing
  standard things like man pages, this then:

  * attempts to stop supervisord for the duration of the installation (because
    that's the usual way to run a CamCOPS server);

  * creates a few standard directories (e.g. for CamCOPS configuration and
    lock files), including
        {LINUX_DEFAULT_CAMCOPS_CONFIG_DIR}
        {LINUX_DEFAULT_CAMCOPS_DIR}
        {LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR}
        {LINUX_DEFAULT_LOCK_DIR}

  * checks that Python is available on the system;

  * uses the system Python to create a Python virtual environment within
    {LINUX_DEFAULT_CAMCOPS_DIR};

  * uses the virtual environment's "pip" command to install the distributed
    CamCOPS Python package within that virtual environment;
    ... which also appears to compile .py to .pyc files automatically;

  * creates master executable scripts (which call corresponding Python
    scripts):
        {DSTCONSOLEFILE}
        {DSTMETACONSOLEFILE}

  * sets some permissions (to default users such as "www-data" on Ubuntu, or
    "apache" on CentOS);

  * restarts supervisord.

        """.format(
            DSTCONSOLEFILE=DSTCONSOLEFILE,
            DSTMETACONSOLEFILE=DSTMETACONSOLEFILE,
            LINUX_DEFAULT_CAMCOPS_CONFIG_DIR=LINUX_DEFAULT_CAMCOPS_CONFIG_DIR,
            LINUX_DEFAULT_CAMCOPS_DIR=LINUX_DEFAULT_CAMCOPS_DIR,
            LINUX_DEFAULT_LOCK_DIR=LINUX_DEFAULT_LOCK_DIR,
            LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR=LINUX_DEFAULT_MATPLOTLIB_CACHE_DIR,  # noqa
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument('--verbose', '-v', action='store_true')
    args = parser.parse_args()
    main_only_quicksetup_rootlogger(level=logging.DEBUG if args.verbose
                                    else logging.INFO)

    log.info("mainversion: {}", MAINVERSION)
    log.info("changedate: {}", CHANGEDATE)

    build_package()


if __name__ == "__main__":
    main()
