#!/usr/bin/env python

"""
setup.py

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

**camcops_server setup.py file, to build Python packages**

To use:

    python setup.py sdist --extras

    twine upload dist/*

To install in development mode:

    pip install -e .

"""
# https://packaging.python.org/en/latest/distributing/#working-in-development-mode
# http://python-packaging-user-guide.readthedocs.org/en/latest/distributing/
# http://jtushman.github.io/blog/2013/06/17/sharing-code-across-applications-with-python/  # noqa

import argparse
import fnmatch
import os
from pprint import pformat
from setuptools import setup, find_packages
import shutil
import sys
from typing import List

from camcops_server.cc_modules.cc_version_string import (
    CAMCOPS_SERVER_VERSION_STRING,
)

# =============================================================================
# Constants
# =============================================================================

# Extensions and file patterns
SKIP_PATTERNS = ['*.pyc', '~*']  # files not to add

# Directories
THIS_DIR = os.path.abspath(os.path.dirname(__file__))  # .../camcops/server
EGG_DIR = os.path.join(THIS_DIR, 'camcops_server.egg-info')
CAMCOPS_ROOT_DIR = os.path.abspath(os.path.join(THIS_DIR, os.pardir))  # .../camcops  # noqa
CAMCOPS_SERVER_DIR = os.path.join(THIS_DIR, 'camcops_server')

# Files
MANIFEST_FILE = os.path.join(THIS_DIR, 'MANIFEST.in')
PIP_REQ_FILE = os.path.join(THIS_DIR, 'requirements.txt')

# Arguments
EXTRAS_ARG = 'extras'

# Get the long description from the README file
with open(os.path.join(THIS_DIR, 'README.rst'), encoding='utf-8') as f:
    LONG_DESCRIPTION = f.read()

# Package dependencies
# - See what's installed with: pip freeze | sort --ignore-case
INSTALL_REQUIRES = [

    # -------------------------------------------------------------------------
    # Direct dependencies of CamCOPS
    # -------------------------------------------------------------------------
    'alembic==1.4.2',  # database migrations
    'asteval==0.9.18',  # safe-ish alternative to eval

    'cardinal_pythonlib==1.0.92',  # RNC libraries
    'celery==4.4.6',  # background tasks
    'colander==1.7.0',  # serialization/deserialization from web forms
    'CherryPy==18.6.0',  # web server

    'deform==2.0.10',  # web forms
    # 'deform-bootstrap==0.2.9',  # deform with layout made easier
    'distro==1.3.0',  # detecting Linux distribution  # REMOVE ONCE DOCKER PREFERRED  # noqa
    'dogpile.cache==0.9.2',  # web caching

    # TO COME: 'fhirclient==3.2.0',  # For FHIR export
    'Faker==4.1.1',  # create fake data; for dummy database creation
    'flower==0.9.4',  # monitor for Celery

    'gunicorn==20.0.4',  # web server (Unix only)

    'hl7==0.3.5',  # For HL7 export

    'lockfile==0.12.2',  # File locking for background tasks
    'lxml==4.5.1',  # Will speed up openpyxl export [NO LONGER CRITICAL]

    'matplotlib==3.2.2',  # Used for trackers and some tasks. SLOW INSTALLATION.  # noqa

    'numpy==1.19.0',  # Used by some tasks. SLOW INSTALLATION.

    'paginate==0.5.6',  # pagination for web server
    'pendulum==2.1.0',  # better than Arrow
    'pexpect==4.8.0',  # for open_sqlcipher.py
    'pdfkit==0.6.1',  # wkhtmltopdf interface, for PDF generation from HTML
    # REDCap integration. Pip freeze reports as version 0.0.0?
    'pycap @ git+https://github.com/redcap-tools/pycap@ff0e8c6916352a11e16976d5c7b4aaaed7e500ac#egg=pycap-1.0.2.3',  # noqa
    'Pygments==2.6.1',  # Syntax highlighting for introspection/DDL
    'pyexcel-ods3==0.5.3',  # ODS spreadsheet export
    'pyexcel-xlsx==0.5.8',  # XLSX spreadsheet export
    'pyramid==1.10.4',  # web framework
    'pyramid_debugtoolbar==4.6.1',  # debugging for Pyramid

    'sadisplay==0.4.9',  # SQL Alchemy schema display script
    'scipy==1.5.0',  # used by some tasks. slow installation.
    'semantic_version==2.8.5',  # semantic versioning; better than semver
    'sqlalchemy==1.3.18',  # database access
    'statsmodels==0.11.1',  # e.g. logistic regression

    'Wand==0.6.1',  # ImageMagick binding

    # -------------------------------------------------------------------------
    # Not installed here
    # -------------------------------------------------------------------------

    # mysqlclient -- installed via Docker
    # 'PyMySQL==0.7.1',
    # ... for mysql+pymysql://... BEWARE FURTHER UPGRADES (e.g. to 0.7.11); may
    # break Pendulum handling
    # todo: setup.py: fix PyMySQL upgrade problem

    # -------------------------------------------------------------------------
    # Direct requirements of CamCOPS development tools
    # -------------------------------------------------------------------------

    'Babel==2.8.0',  # reads code, generates gettext files; dev only but was already installed  # noqa
    'flake8==3.8.3',  # code checks
    'sphinx==3.1.1',  # development only
    'sphinx_rtd_theme==0.5.0',  # development only

    # -------------------------------------------------------------------------
    # Dependencies of cardinal_pythonlib, whose versions we pin
    # -------------------------------------------------------------------------

    # mandatory
    'appdirs==1.4.4',
    'beautifulsoup4==4.9.1',
    'colorlog==4.1.0',
    'isodate==0.6.0',
    'openpyxl==3.0.4',  # also for pyexcel-xlsx
    'pandas==1.0.5',
    'prettytable==0.7.2',
    'psutil==5.7.0',
    'pyparsing==2.4.7',
    'PyPDF2==1.26.0',  # Used by cardinal_pythonlib.pdf
    'python-dateutil==2.8.1',  # date/time extensions.
    'sqlparse==0.3.1',

    # extra
    'py-bcrypt==0.4',  # used by cardinal_pythonlib.crypto

    # -------------------------------------------------------------------------
    # Dependencies of other things above
    # -------------------------------------------------------------------------

    'alabaster==0.7.12',  # for sphinx
    'amqp==2.6.0',  # for celery
    'Chameleon==3.8.1',  # for Deform
    'tornado==6.0.4',  # for celery
    'webob==1.8.6',  # for pyramid

]


# =============================================================================
# Helper functions
# =============================================================================

def deltree(path: str, verbose: bool = False) -> None:
    if verbose:
        print("Deleting directory: {}".format(path))
    shutil.rmtree(path, ignore_errors=True)


_ = '''

COPYABLE_EXTENSIONS = [
    ".cpp", ".h", ".html", ".js", ".jsx",
    ".py", ".pl", ".qml", ".xml",
    ".png"
]


def copier(src: str, dst: str, follow_symlinks: bool = True,
           verbose: bool = False) -> None:
    # Cleaner than a long "ignore" argument to shutil.copytree
    base, ext = os.path.splitext(os.path.basename(src))
    if ext.lower() not in COPYABLE_EXTENSIONS or base.startswith('.'):
        if verbose:
            print("Ignoring: {}".format(src))
        return
    if verbose:
        print("Copying {} -> {}".format(src, dst))
    # noinspection PyArgumentList
    shutil.copy2(src, dst, follow_symlinks=follow_symlinks)


def delete_empty_directories(root_dir: str, verbose: bool = False) -> None:
    # Based on
    # - http://stackoverflow.com/questions/26774892/how-to-find-recursively-empty-directories-in-python  # noqa
    # - https://docs.python.org/3/library/os.html
    empty_dirs = []
    for dir_, subdirs, files in os.walk(root_dir, topdown=False):
        all_subs_empty = True  # until proven otherwise
        for sub in subdirs:
            sub_fullpath = os.path.join(dir_, sub)
            if sub_fullpath not in empty_dirs:
                all_subs_empty = False
                break
        if all_subs_empty and len(files) == 0:
            empty_dirs.append(dir_)
            deltree(dir_, verbose=verbose)

'''


def add_all_files(root_dir: str,
                  filelist: List[str],
                  absolute: bool = False,
                  include_n_parents: int = 0,
                  verbose: bool = False,
                  skip_patterns: List[str] = None) -> None:
    skip_patterns = skip_patterns or SKIP_PATTERNS
    if absolute:
        base_dir = root_dir
    else:
        base_dir = os.path.abspath(
            os.path.join(root_dir, *(['..'] * include_n_parents)))
    for dir_, subdirs, files in os.walk(root_dir, topdown=True):
        if absolute:
            final_dir = dir_
        else:
            final_dir = os.path.relpath(dir_, base_dir)
        for filename in files:
            _, ext = os.path.splitext(filename)
            final_filename = os.path.join(final_dir, filename)
            if any(fnmatch.fnmatch(final_filename, pattern)
                   for pattern in skip_patterns):
                if verbose:
                    print("Skipping: {}".format(final_filename))
                continue
            if verbose:
                print("Adding: {}".format(final_filename))
            filelist.append(final_filename)


# =============================================================================
# There's a nasty caching effect. So remove the old ".egg_info" directory
# =============================================================================
# http://blog.codekills.net/2011/07/15/lies,-more-lies-and-python-packaging-documentation-on--package_data-/  # noqa

deltree(EGG_DIR, verbose=True)

# Also...
# 1. Empircally, MANIFEST.in can get things copied.
# 2. Empirically, no MANIFEST.in but package_data can get things copied into
#    the .gz file, but not into the final distribution after installation.
# 3. So, we'll auto-write a MANIFEST.in
# 4. Argh! Still not installing
# - http://stackoverflow.com/questions/13307408/python-packaging-data-files-are-put-properly-in-tar-gz-file-but-are-not-install  # noqa
# - http://stackoverflow.com/questions/13307408/python-packaging-data-files-are-put-properly-in-tar-gz-file-but-are-not-install/32635882#32635882  # noqa
# - I think the problem is that MANIFEST.in paths need to be relative to the
#   location of setup.py, whereas package_data paths are relative to each
#   package (e.g. camcops_server).
# 5. AND... include_package_data
# - http://stackoverflow.com/questions/13307408/python-packaging-data-files-are-put-properly-in-tar-gz-file-but-are-not-install  # noqa
# - http://danielsokolowski.blogspot.co.uk/2012/08/setuptools-includepackagedata-option.html  # noqa


# =============================================================================
# Perform special actions if we're building a package
# =============================================================================
# Our script may be run by the developer (python setup.py sdist), or by pip
# (as a consequence of "pip install ..."). We only want to do the copying of
# source tablet files into our tree in the former situation. So we accept a
# special argument, act on it if present, and pass all other arguments (by
# writing them back to sys.argv) to the Python setuptools internals.

parser = argparse.ArgumentParser()
parser.add_argument(
    '--' + EXTRAS_ARG, action='store_true',
    help=(
        "USE THIS TO CREATE PACKAGES (e.g. 'python setup.py sdist --{}. "
        "Copies extra info in.".format(EXTRAS_ARG)
    )
)
our_args, leftover_args = parser.parse_known_args()
sys.argv[1:] = leftover_args

extra_files = []  # type: List[str]

if getattr(our_args, EXTRAS_ARG):
    src_tablet_qt = os.path.join(THIS_DIR, '..', 'tablet_qt')
    required_dirs = [src_tablet_qt]

    for d in required_dirs:
        if not os.path.isdir(d):
            print("You have used the --{} argument, but directory {!r} is "
                  "missing. That argument is only for use in development, to "
                  "create a Python package.".format(EXTRAS_ARG, d))
            sys.exit(1)

    # -------------------------------------------------------------------------
    # Add extra files
    # -------------------------------------------------------------------------

    extra_files.append('alembic.ini')
    add_all_files(os.path.join(CAMCOPS_SERVER_DIR, 'alembic'),
                  extra_files, absolute=False, include_n_parents=1)
    add_all_files(os.path.join(CAMCOPS_SERVER_DIR, 'docs'),
                  extra_files, absolute=False, include_n_parents=1)
    add_all_files(os.path.join(CAMCOPS_SERVER_DIR, 'extra_strings'),
                  extra_files, absolute=False, include_n_parents=1)
    add_all_files(os.path.join(CAMCOPS_SERVER_DIR, 'extra_string_templates'),
                  extra_files, absolute=False, include_n_parents=1)
    add_all_files(os.path.join(CAMCOPS_SERVER_DIR, 'static'),
                  extra_files, absolute=False, include_n_parents=1)
    add_all_files(os.path.join(CAMCOPS_SERVER_DIR, 'templates'),
                  extra_files, absolute=False, include_n_parents=1)
    add_all_files(os.path.join(CAMCOPS_SERVER_DIR, 'translations'),
                  extra_files, absolute=False, include_n_parents=1,
                  skip_patterns=SKIP_PATTERNS + ["*.pot", "*.po"])

    extra_files.sort()
    print("EXTRA_FILES: \n{}".format(pformat(extra_files)))

    # -------------------------------------------------------------------------
    # Write manifest
    # -------------------------------------------------------------------------
    MANIFEST_LINES = ['include camcops_server/' + x for x in extra_files]
    with open(MANIFEST_FILE, 'wt') as manifest:
        manifest.writelines([
            "# This is an AUTOCREATED file, server/MANIFEST.in; see "
            "server/setup.py and DO NOT EDIT BY HAND"])
        manifest.write("\n\n" + "\n".join(MANIFEST_LINES) + "\n")

    # -------------------------------------------------------------------------
    # Write requirements.txt
    # -------------------------------------------------------------------------
    with open(PIP_REQ_FILE, "w") as req_file:
        req_file.writelines([
            "# This is an AUTOCREATED file, server/requirements.txt; see "
            "server/setup.py and DO NOT EDIT BY HAND"])
        req_file.write("\n\n" + "\n".join(INSTALL_REQUIRES) + "\n")


# =============================================================================
# setup args
# =============================================================================

setup(
    name='camcops_server',

    version=CAMCOPS_SERVER_VERSION_STRING,

    description='CamCOPS server',
    long_description=LONG_DESCRIPTION,

    # The project's main homepage.
    url='https://camcops.readthedocs.org/',

    # Author details
    author='Rudolf Cardinal',
    author_email='rudolf@pobox.com',

    # Choose your license
    license='GNU General Public License v3 or later (GPLv3+)',

    # See https://pypi.org/classifiers/
    classifiers=[
        # How mature is this project?
        'Development Status :: 5 - Production/Stable',

        # Indicate who your project is intended for
        'Intended Audience :: Healthcare Industry',
        'Intended Audience :: Science/Research',

        # Pick your license as you wish (should match "license" above)
        'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',  # noqa

        'Natural Language :: English',

        'Operating System :: OS Independent',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',

        'Topic :: Scientific/Engineering :: Medical Science Apps.'
    ],

    keywords='cardinal',

    packages=find_packages(),  # finds all the .py files in subdirectories

    # package_data ?
    # - http://blog.codekills.net/2011/07/15/lies,-more-lies-and-python-packaging-documentation-on--package_data-/  # noqa
    # - http://stackoverflow.com/questions/29036937/how-can-i-include-package-data-without-a-manifest-in-file  # noqa
    #
    # or MANIFEST.in ?
    # - http://stackoverflow.com/questions/24727709/i-dont-understand-python-manifest-in  # noqa
    # - http://stackoverflow.com/questions/1612733/including-non-python-files-with-setup-py  # noqa
    #
    # or both?
    # - http://stackoverflow.com/questions/3596979/manifest-in-ignored-on-python-setup-py-install-no-data-files-installed  # noqa
    # ... MANIFEST gets the files into the distribution
    # ... package_data gets them installed in the distribution
    #
    # data_files is from distutils, and we're using setuptools
    # - https://docs.python.org/3.5/distutils/setupscript.html#installing-additional-files  # noqa

    package_data={
        'camcops_server': extra_files,
    },

    include_package_data=True,  # use MANIFEST.in during install?

    install_requires=INSTALL_REQUIRES,

    entry_points={
        'console_scripts': [
            # Format is 'script=module:function".
            'camcops_server=camcops_server.camcops_server:main',
            'camcops_server_meta=camcops_server.camcops_server_meta:meta_main',
            'camcops_backup_mysql_database=cardinal_pythonlib.tools.backup_mysql_database:main',  # noqa
            'camcops_windows_service=camcops_server.camcops_windows_service:main',  # noqa
            'camcops_print_latest_github_version=camcops_server.tools.print_latest_github_version:main',  # noqa
            'camcops_fetch_snomed_codes=camcops_server.tools.fetch_snomed_codes:main',  # noqa
        ],
    },
)


# =============================================================================
# Clean up
# =============================================================================

# No, don't clean up, keeping the copy helps us with "pip install -e ."
