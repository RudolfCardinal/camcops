#!/usr/bin/env python

r"""
tools/build_qt.py

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

This script is design to download and build the prerequisites for building
the CamCOPS client, including:

    =========== ==========================
    Qt          C++ cross-platform library
    OpenSSL     Encryption
    Eigen       Matrix algebra
    SQLCipher   Encrypted SQLite
    =========== ==========================


Status
======

+---------------------+-----------------------------+-------------------------+
| Build OS            | Target OS                   | Status                  |
|                     |                             |                         |
+=====================+=============================+=========================+
| Linux, x86, 64-bit  | Linux, x86, 32-bit          | deferred                |
+---------------------+-----------------------------+-------------------------+
|                     | Linux, x86, 64-bit          | OK 2019-06-17           |
+---------------------+-----------------------------+-------------------------+
|                     | Android, x86, 32-bit        | deferred                |
|                     | (for emulator)              |                         |
+---------------------+-----------------------------+-------------------------+
|                     | Android, ARM, 32-bit        | OK 2019-06-17           |
+---------------------+-----------------------------+-------------------------+
|                     | Android, ARM, 64-bit        | OK 2019-06-17           |
+---------------------+-----------------------------+-------------------------+
| macOS (OS X), x86,  | macOS, x86, 64-bit          | OK 2019-06-17           |
| 64-bit              |                             |                         |
+---------------------+-----------------------------+-------------------------+
|                     | iOS, x86 (for emulator)     | deferred                |
+---------------------+-----------------------------+-------------------------+
|                     | iOS, ARM, 32-bit            | OK 2019-06-17           |
|                     | (for iPad etc.)             |                         |
+---------------------+-----------------------------+-------------------------+
|                     | iOS, ARM, 64-bit            | OK 2019-06-17           |
|                     | (for iPad etc.)             |                         |
+---------------------+-----------------------------+-------------------------+
| Windows, x86,       | Windows, x86, 32-bit        | OK 2019-06-17           |
| 64-bit (*)          |                             |                         |
+---------------------+-----------------------------+-------------------------+
|                     | Windows, x86, 64-bit        | OK 2019-06-17           |
+=====================+=============================+=========================+

These OS combinations are reflected in the ``--build_all`` option.

(*) May need ``--nparallel 1`` for the OpenSSL parts of the build.


Why?
====

When is it NECESSARY to compile OpenSSL from source?

- OpenSSL for Android

  - http://doc.qt.io/qt-5/opensslsupport.html
  - ... so: necessary.

When is it NECESSARY to compile Qt from source?

- Static linking of OpenSSL (non-critical)

- SQLite support (critical)
  - http://doc.qt.io/qt-5/sql-driver.html
  - ... so: necessary.


Windows
=======

Several compilers are possible, in principle.

- Cygwin

  - Very nice installation and cross-operation with native Windows.
  - May be useful for toolchains.
  - However, software that its compilers produce run under POSIX, so require an
    intermediate Cygwin DLL layer to run; we don't want that.

- Microsoft Visual Studio (free or paid)

  - An obvious potential candidate, but not open-source.

- MinGW

  - Runs under Windows and produces native code.
  - Qt supports it.
  - Provides the MSYS bash environment to assist for compilation.
  - Can also run under Linux and cross-compile to Windows.

    - More specifically: mingw-w64, which is GCC for 32- and 64-bit Windows
      - http://mingw-w64.org/
      - ... i686-w64-mingw32 for 32-bit executables
      - ... x86_64-w64-mingw32 for 64-bit executables

    - Within this option, there is MXE, which is a cross-compilation
      environment.

    - Upshot: I tried hard. As of 2017-11-19:

      - MinGW itself is the old version and has been superseded by
        mingw-w64 (a.k.a. mingw64).
      - attempts to use MinGW-W64 to build 32-bit Windows code (via the MXE
        build of mingw-w64) lead to a GCC compiler crash; this is because
        the version of mingw-w64 that MXE supports uses an old GCC.
      - getting Qt happy is very hard
      - For the 64-bit compilation, I ended up with a "make" process that
        reaches this error:

        .. code-block:: none

            /home/rudolf/dev/qt_local_build/src/qt5/qtwebglplugin/src/plugins/platforms/webgl/qwebglwindow_p.h:64:48: error: field 'defaults' has incomplete type 'std::promise<QMap<unsigned int, QVariant> >'
                 std::promise<QMap<unsigned int, QVariant>> defaults;

      - Not clear that it's worth the effort to use a manual build of
        mingw-w64 as well.
      - And none of this reached the stage of actually testing on Windows.

DECISION:

- Use Microsoft Visual Studio and native compilation under Windows.


Notes
=====

- configure: http://doc.qt.io/qt-5/configure-options.html
- sqlite: http://doc.qt.io/qt-5/sql-driver.html
- build for Android: http://wiki.qt.io/Qt5ForAndroidBuilding
- multi-core builds:
  http://stackoverflow.com/questions/9420825/how-to-compile-on-multiple-cores-using-mingw-inside-qtcreator


Standard environment variables
==============================

.. code-block::none

    ANDROID_API
    ANDROID_API_VERSION
    ANDROID_ARCH
    ANDROID_DEV
    ANDROID_EABI
    ANDROID_NDK_HOME [10]
    ANDROID_NDK_HOST [11]
    ANDROID_NDK_ROOT [11, possibly 5]
    ANDROID_SDK_ROOT [5,11]
    ANDROID_SYSROOT
    ANDROID_TOOLCHAIN
    AR [2]: archive-maintaining program, GNU default "ar"
    ARCH [3]
    BUILD_TOOLS [9]
    CC [2]: C compiler, GNU default "cc" (but e.g. clang)
    CFLAGS [2]: extra flags to give to the C compiler
    CPP [2]: C preprocessor, GNU default "$(CC) -E"
    CPPFLAGS [2]: extra flags to give to the C preprocessor
    CROSS_COMPILE [3]
    CROSS_SDK [10]
    CROSS_SYSROOT [3]
    CROSS_TOP [10]
    CXX [2]: C++ compiler, GNU default "g++" (but e.g. clang++)
    CXXFLAGS [2]: extra flags to give to the C++ compiler
    FIPS_SIG [10]
    HOME [1]
    HOSTCC [3]: the host C compiler to use
    JAVA_HOME [4]
    LDFLAGS [2]: extra flags for compilers to pass to the linker, "ld"
    LD_LIBRARY_PATH [1]: path searched for dynamic libraries under Unix
    LDLIBS [2]: library flags/names for compilers to pass to the linker
    MACHINE [10]
    NDK_SYSROOT
    PATH [1,6]
    PLATFORM [9]
    RANLIB [10]: GNU ranlib program, http://man7.org/linux/man-pages/man1/ranlib.1.html
    RELEASE [10]
    SYSROOT [8]
    SYSTEM [10]
    WindowsSdkDir [7]

[1] Unix core. For LD_LIBRARY_PATH, see
http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html.

[2] GNU toolchain standards:
https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html

[3] are GNU cross-compilation standards; e.g.
https://wiki.freebsd.org/ExternalGCC;
https://buildroot.org/downloads/manual/manual.html

[4] Java standards:
https://docs.oracle.com/cd/E19182-01/821-0917/inst_jdk_javahome_t/index.html

[5] Android standards:
https://developer.android.com/studio/command-line/variables

[6] Windows core.

[7] Windows SDK.

[8] GCC special variable; see
https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html

[9] MacOS Xcode development environment?

[10] OpenSSL; https://wiki.openssl.org/index.php/Compilation_and_Installation;
https://www.x.org/wiki/CrossCompilingXorg/; NOTES.ANDROID in the OpenSSL source
tree; https://wiki.openssl.org/index.php/Android

[11] Qt configure

If not labelled, probably arbitrary.


Problems with Qt configure
==========================

2019-06-16: extreme difficulty getting Qt to configure for Android with recent
(v18-20) Android NDKs, which use clang.

Basic configure command, from https://wiki.qt.io/Android, plus "android-arch"
etc.:

.. code-block:: bash

    export DEVROOT=/home/rudolf/dev/qt_local_build
    export ANDROID_NDK_ROOT=/home/rudolf/dev/android-ndk-r20
    export ANDROID_SDK_ROOT=/home/rudolf/dev/android-sdk-linux
    export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
    mkdir $DEVROOT/qt_temp_build
    cd $DEVROOT/qt_temp_build
    # $DEVROOT/src/qt5/configure --help | less
    $DEVROOT/src/qt5/configure \
        -prefix $DEVROOT/qt_temp_install \
        -xplatform android-clang \
        --disable-rpath \
        -nomake tests -nomake examples \
        -android-sdk $ANDROID_SDK_ROOT \
        -android-ndk $ANDROID_NDK_ROOT \
        -android-ndk-host linux-x86_64 \
        -android-ndk-platform android-23 \
        -android-arch arm64-v8a \
        -android-toolchain-version 4.9 \
        -skip qttranslations -skip qtserialport \
        -verbose \
        -opensource -confirm-license \
        -no-warnings-are-errors

If this doesn't work, there's probably a Qt bug.

In working this through, note:

- To get clang to find object files, use "-B<dir>" or "--prefix <dir>" or
  "--prefix=<dir>"; see
  https://clang.llvm.org/docs/ClangCommandLineReference.html.

- When the linker can't find "-lc++", it's looking for "libc++.so".

- To find library files, use "-L<dir>".

Looks like this was a Qt problem that they've fixed:

    https://github.com/qt/qtbase/commits/5.12/mkspecs/android-clang/qmake.conf
    https://bugreports.qt.io/browse/QTBUG-76293

To update an existing Qt5 git repository, from its root directory:

.. code-block:: bash

    # https://wiki.qt.io/Building_Qt_5_from_Git
    git pull
    perl init-repository -f  # -f for force

To update a specific submodule, e.g. qtbase:

.. code-block:: bash

    cd qtbase
    git fetch
    git checkout 067664531853a1e857c777c1cc56fc64b272e021
    # ... seems to work; that is a specific commit at
    # https://github.com/qt/qtbase/commit/067664531853a1e857c777c1cc56fc64b272e021#diff-0b4799f074ffd43c60d33464189578b7
    # that fixes this bug.

See patch_qt_for_android_ndk_20().

However, this went away with Qt 5.12.4.


Problems with 64-bit ARM
========================

2019-06-18: Qt configure runs OK, but the Qt build process fails with
"undefined reference" errors to e.g.

.. code-block:: none

    qt_memfill32
    qt_blend_rgb32_on_rgb32_neon
    qt_blend_rgb16_on_argb32_neon

    ... etc. (lots of "_neon") suffixes

Using

.. code-block:: bash

    find . -iname "*.h" -o -iname "*.cpp" -exec grep qt_blend_rgb16_on_argb32_neon -l {} \;

we find that they are in

.. code-block:: none

    qtbase/src/gui/painting/qdrawhelper.cpp
    qtbase/src/gui/painting/qdrawhelper_neon.cpp

and the conditional part is ``#ifdef __ARM_NEON__``. Therefore, see

- https://bugreports.qt.io/browse/QTBUG-58180
- https://bugreports.qt.io/browse/QTBUG-72716 ??

Switched to Qt 5.12.4 (released 2019-06-17!).

Still not working. Reported as https://bugreports.qt.io/browse/QTBUG-76445.

On 2019-07-10, patch available. From "src/qt5/qtbase" directory, execute:

.. code-block::bash

    git pull "https://codereview.qt-project.org/qt/qtbase" refs/changes/97/267497/4


Current Qt version
==================

As of 2018-06-18:

- Qt branch "5.12" is version 5.12.4 (released 2019-06-17).
- The head commit is 452e0d94d40bba15a56293a0a0f7d093dececda9.
- PATCH_QT_FOR_ANDROID_NDK_20 is no longer necessary.

Advice:

- Do not proceed ahead of official releases. Sometimes Qt Creator doesn't
  recognize the version. It's always tricky to manage.


cmake under Ubuntu
==================

- In 2019, with Ubuntu 18.04, ``cmake`` requires ``libcurl4`` which conflicts
  with ``libcurl3`` on which many applications depend (e.g. R). See
  https://bugs.launchpad.net/ubuntu/+source/curl/+bug/1754294;
  https://askubuntu.com/questions/1029273/curl-is-not-working-on-ubuntu-18-04-lts.

- Not sure what's still using, it, though! Requirement removed...

"""  # noqa


import argparse
import logging
import multiprocessing
import os
from os import chdir
from os.path import expanduser, isfile, join, split
import platform
import re
import shutil
import subprocess
import sys
import traceback
from typing import Dict, List, NoReturn, TextIO, Tuple

try:
    import cardinal_pythonlib
except ImportError:
    cardinal_pythonlib = None
    print("Please install 'cardinal_pythonlib' first, using:\n\n"
          "pip install cardinal_pythonlib")
    raise

try:
    import distro  # http://distro.readthedocs.io/en/latest/
except ImportError:
    distro = None
    if platform.system() in ["Linux"]:
        print("Please install 'distro' first, using:\n\n"
              "pip install distro")
        raise

from cardinal_pythonlib.buildfunc import (
    download_if_not_exists,
    fetch,
    git_clone,
    untar_to_directory,
)
from cardinal_pythonlib.buildfunc import run as run2
from cardinal_pythonlib.file_io import (
    replace_multiple_in_file,
)
from cardinal_pythonlib.fileops import (
    copy_tree_contents,
    mkdir_p,
    pushd,
    which_with_envpath,
)
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
    main_only_quicksetup_rootlogger,
)
from cardinal_pythonlib.network import download
from cardinal_pythonlib.platformfunc import (
    contains_unquoted_ampersand_dangerous_to_windows,
    windows_get_environment_from_batch_command,
)
from cardinal_pythonlib.tee import tee_log
import cardinal_pythonlib.version
from semantic_version import Version

assert sys.version_info >= (3, 6), "Need Python 3.6 or higher"

MINIMUM_CARDINAL_PYTHONLIB = "1.0.8"
if Version(cardinal_pythonlib.version.VERSION) < Version(MINIMUM_CARDINAL_PYTHONLIB):  # noqa
    raise ImportError(f"Need cardinal_pythonlib >= {MINIMUM_CARDINAL_PYTHONLIB}")  # noqa

log = BraceStyleAdapter(logging.getLogger(__name__))

# =============================================================================
# Constants
# =============================================================================

# -----------------------------------------------------------------------------
# Internal constants
# -----------------------------------------------------------------------------

USER_DIR = expanduser("~")
HEAD = "HEAD"  # git commit meaning "the most recent"

ENVVAR_QT_BASE = "CAMCOPS_QT_BASE_DIR"

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

# -----------------------------------------------------------------------------
# Default directories
# -----------------------------------------------------------------------------

if "GENERATING_CAMCOPS_DOCS" in os.environ:
    USER_DIR = "/path/to/user"

DEFAULT_ROOT_DIR = join(USER_DIR, "dev", "qt_local_build")

DEFAULT_ANDROID_SDK = join(USER_DIR, "dev", "android-sdk-linux")
DEFAULT_ANDROID_NDK = join(USER_DIR, "dev", "android-ndk-r20")  # from 2019-06-15, inc. 64-bit ARM  # noqa

DEFAULT_JAVA_HOME = "/usr/lib/jvm/java-11-openjdk-amd64"

DEFAULT_QT_SRC_DIRNAME = "qt5"

# -----------------------------------------------------------------------------
# Downloading and versions
# -----------------------------------------------------------------------------

# Android

ANDROID_SDK_VERSION = 23  # see changelog.rst 2018-07-17
ANDROID_NDK_VERSION = 20  # see above

DEFAULT_ANDROID_NDK_HOST = "linux-x86_64"
DEFAULT_ANDROID_TOOLCHAIN_VERSION = "4.9"

# Qt

QT_GIT_URL = "git://code.qt.io/qt/qt5.git"
QT_GIT_BRANCH = "5.12"  # is 5.12.4 as of 2019-06-18 (released 2019-06-17)
QT_GIT_COMMIT = HEAD
QT_SPECIFIC_VERSION = ""

if QT_SPECIFIC_VERSION:
    QT_VERSION = Version(QT_SPECIFIC_VERSION)
else:
    QT_VERSION = Version(QT_GIT_BRANCH, partial=True)

DEFAULT_QT_USE_OPENSSL_STATICALLY = True

QT_XCB_SUPPORT_OK = True  # see 2017-12-01 above, fixed 2017-12-08
ADD_SO_VERSION_OF_LIBQTFORANDROID = False
USE_CLANG_NOT_GCC_FOR_ANDROID_ARM = (
    QT_VERSION >= Version("5.12", partial=True)  # new feature 2019-06-15
)

# OpenSSL

# -- IF YOU CHANGE THIS, UPDATE camcops.pro
OPENSSL_VERSION = "1.1.1c"  # as of 2019-06-15, previously 1.1.0g
# ... formerly "1.0.2h", but Windows 64 builds break
# ... as of 2017-11-21, stable series is 1.1 and LTS series is 1.0.2
# ... but Qt 5.9.3 doesn't support OpenSSL 1.1.0g;
#     errors relating to "undefined type 'x509_st'"
# ... OpenSSL 1.1 requires Qt 5.10.0 alpha:
#     https://bugreports.qt.io/browse/QTBUG-52905
OPENSSL_FAILS_OWN_TESTS = True
# https://bugs.launchpad.net/ubuntu/+source/openssl/+bug/1581084
OPENSSL_SRC_URL = (
    f"https://www.openssl.org/source/openssl-{OPENSSL_VERSION}.tar.gz"
)
# OPENSSL_ANDROID_SCRIPT_URL = (
#     "https://wiki.openssl.org/images/7/70/Setenv-android.sh"
# )

# SQLCipher; https://www.zetetic.net/sqlcipher/open-source/

SQLCIPHER_GIT_URL = "https://github.com/sqlcipher/sqlcipher.git"
# SQLCIPHER_GIT_COMMIT = "c6f709fca81c910ba133aaf6330c28e01ccfe5f8"  # SQLCipher version 3.4.2, 21 Dec 2017  # noqa
SQLCIPHER_GIT_COMMIT = "750f5e32474ee23a423376203e671cab9841c67a"  # SQLCipher version 4.2.0, 29 May 2019  # noqa

# - Note that there's another URL for the Android binary packages
# - SQLCipher supports OpenSSL 1.1.0 as of SQLCipher 3.4.1
# - To find the Git commit identifier (hash) of a cloned repository, use
#   "git show -s --format=%H" (and omit the --format parameter to see more
#   info).
# - For SQLCipher, see also https://github.com/sqlcipher/sqlcipher/releases.

# Eigen

EIGEN_VERSION = "3.3.3"

# Mac things; https://gist.github.com/armadsen/b30f352a8d6f6c87a146
MIN_IOS_VERSION = "7.0"
MIN_MACOS_VERSION = "10.10"  # ... with 10.7, SQLCipher's "configure" fails

# GNU tools

CONFIG_GUESS_MASTER = "https://raw.githubusercontent.com/gcc-mirror/gcc/master/config.guess"  # noqa
CONFIG_SUB_MASTER = "https://raw.githubusercontent.com/gcc-mirror/gcc/master/config.sub"  # noqa


# -----------------------------------------------------------------------------
# Building Qt
# -----------------------------------------------------------------------------
# TO MAKE MINOR CHANGES: delete ...installdir/bin/qmake, and rerun this script.
# (Can still take ages. Not sure it saves any time, in fact.)

QT_CONFIG_COMMON_ARGS = [
    # use "configure -help" to list all of them
    # http://doc.qt.io/qt-4.8/configure-options.html  # NB better docs than 5.7
    # http://doc.qt.io/qt-5.7/configure-options.html  # less helpful
    # http://doc.qt.io/qt-5.9/configure-options.html

    # -------------------------------------------------------------------------
    # Qt license
    # -------------------------------------------------------------------------

    "-opensource", "-confirm-license",  # Choose our Qt edition.

    # -------------------------------------------------------------------------
    # debug v. release
    # -------------------------------------------------------------------------
    # Now decided manually (2017-12-04); occasionally we need a debug build.
    # We can't in general create a "simultaneously debug and release" build;
    # see https://forum.qt.io/topic/75056/configuring-qt-what-replaces-debug-and-release/7 .  # noqa

    # -------------------------------------------------------------------------
    # static v. shared
    # -------------------------------------------------------------------------
    # Now decided on a per-platform basis (2017-06-18)

    # -------------------------------------------------------------------------
    # Database support
    # -------------------------------------------------------------------------

    # v5.7.0 # "-qt-sql-sqlite",  # SQLite (v3) support built in to Qt
    # v5.9.0:
    # "-sql-sqlite",  # v5.9: explicitly add SQLite support
    # "-qt-sqlite",  # v5.9: "qt", rather than "system"
    # 2017-12-01: conflict between SQLite and SQLCipher (symbols duplicated on
    # linking); try disabling it
    "-no-sql-sqlite",

    "-no-sql-db2",  # disable other SQL drivers
    "-no-sql-ibase",
    "-no-sql-mysql",  # ... for future: maybe re-enable as a plugin
    "-no-sql-oci",
    "-no-sql-odbc",  # ... for future: maybe re-enable as a plugin
    "-no-sql-psql",  # ... for future: maybe re-enable as a plugin
    "-no-sql-sqlite2",  # this is an old SQLite version
    "-no-sql-tds",  # this one specifically was causing library problems

    # -------------------------------------------------------------------------
    # Libraries
    # -------------------------------------------------------------------------

    "-qt-doubleconversion",  # Use Qt version of double conversion library
    # NOT YET WORKING ("Qt no longer ships fonts") # "-qt-freetype",  # Qt, not system, version of Freetype  # noqa
    # NOT YET WORKING ("Qt no longer ships fonts") # "-qt-harfbuzz",  # Qt, not system, version of HarfBuzz-NG  # noqa
    "-qt-libjpeg",  # Qt, not host OS, version of JPEG library
    "-qt-libpng",  # Qt, not host OS, version of PNG library
    "-qt-zlib",  # Qt, not host OS, version of zlib

    # -------------------------------------------------------------------------
    # Compilation
    # -------------------------------------------------------------------------
    # Note: the default release build optimizes with -O2 -Os; there are
    # some 'configure' options to control this, but it's probably a good
    # default.

    "-no-warnings-are-errors",  # don't treat warnings as errors

    # -------------------------------------------------------------------------
    # Stuff to skip
    # -------------------------------------------------------------------------
    # CHANGE OF HEART: don't skip anything; it leads to trouble later!

    # not a valid "-nomake": # "-nomake", "docs",
    # not a valid "-nomake": # "-nomake", "demos",
    "-nomake", "examples",
    "-nomake", "tests",

    # "-skip", "qttranslations",

    # Don't need this on any platform, and unsupported on Android:
    "-skip", "qtserialport",

    # Except the webkit stuff, which ends up giving problems with Wayland-EGL:
    # "-skip", "qtwebkit",  # disabled 2017-10-22: "Project ERROR: -skip command line argument used with non-existent module 'qtwebkit'."  # noqa
    # "-skip", "qtwebkit-examples",  # disabled 2017-10-22: ditto
]

# -----------------------------------------------------------------------------
# Building OpenSSL
# -----------------------------------------------------------------------------

OPENSSL_COMMON_OPTIONS = [
    "shared",  # make .so files (needed by Qt sometimes) as well as .a
    # deprecated option as of 1.1.0g # "no-ssl2",  # SSL-2 is broken
    "no-ssl3",  # SSL-3 is broken. Is an SSL-3 build required for TLS 1.2?
    # "no-comp",  # disable compression independent of zlib
]

# -----------------------------------------------------------------------------
# External tools
# -----------------------------------------------------------------------------

ANT = "ant"  # for Android builds; a Java-based make tool
AR = "ar"  # manipulates archives
BASH = "bash"  # GNU Bourne-Again SHell
CL = "cl"  # Visual C++ compiler
CLANG = "clang"  # macOS XCode compiler; also used under Linux for 64-bit ARM
# CMAKE = "cmake"  # CMake
GCC = "gcc"  # GNU C compiler
GCC_AR = "gcc-ar"  # wrapper around ar
GIT = "git"  # Git
GOBJDUMP = "gobjdump"  # macOS 32-bit equivalent of readelf, via brew
INSTALL_NAME_TOOL = "install_name_tool"  # iOS dylib path fixups
JAVAC = "javac"  # for Android builds
LD = "ld"  # GNU linker
MAKE = "make"  # GNU make
MAKEDEPEND = "makedepend"  # used by OpenSSL via "make"
NASM = "nasm.exe"  # Assembler for Windows (for OpenSSL); http://www.nasm.us/
NMAKE = "nmake.exe"  # Visual Studio make tool
OBJDUMP = "objdump"  # GNU; display information from object files
OTOOL = "otool"  # macOS 64-bit equivalent of gobjdump
PERL = "perl"  # Perl
READELF = "readelf"  # read ELF-format library files
SED = "sed"  # stream editor
TAR = "tar"  # manipulate tar files
TCLSH = "tclsh"  # used by SQLCipher build process
VCVARSALL = "vcvarsall.bat"  # sets environment variables for VC++
XCODE_SELECT = "xcode-select"  # macOS tool to find paths for XCode
XCRUN = "xcrun"  # macOS XCode tool

# -----------------------------------------------------------------------------
# Logging
# -----------------------------------------------------------------------------

LOG_FORMAT = '%(asctime)s.%(msecs)03d:%(levelname)s:%(message)s'
LOG_DATEFMT = '%Y-%m-%d %H:%M:%S'

# -----------------------------------------------------------------------------
# Errors
# -----------------------------------------------------------------------------

BAD_WINDOWS_PATH_MSG = (
    "Something has put a unquoted ampersand (&) in the PATH environment "
    "variable. Whilst not technically illegal, this breaks quite a lot of "
    "software. (The usual culprit is MySQL; see "
    "https://stackoverflow.com/questions/34124636.) Please fix this (enclose "
    "that part in quotes), e.g. in your system or user environment variables "
    "via the Control Panel, and try again. The path is:\n\n"
)

# Take your pick:
CANNOT_CROSS_COMPILE_QT = (
    "Cannot, at present, cross-compile Qt from Linux to Windows.")
ERROR_COMPILE_FOR_WINDOWS_ON_LINUX = "Please use Linux to build for Windows."

QT_BUILD_DEBUG = "debug"
QT_BUILD_RELEASE = "release"
QT_BUILD_RELEASE_WITH_SYMBOLS = "release_w_symbols"
QT_POSSIBLE_BUILD_TYPES = [QT_BUILD_DEBUG,
                           QT_BUILD_RELEASE,
                           QT_BUILD_RELEASE_WITH_SYMBOLS]


# =============================================================================
# Helper functions
# =============================================================================

DEBUG_SHOW_ENV = True


def run(*args, **kwargs) -> Tuple[str, str]:
    """
    Uses our library command-running tool, but forces the debug_show_env
    parameter.
    """
    return run2(*args, **kwargs, debug_show_env=DEBUG_SHOW_ENV)


# =============================================================================
# Classes to collect constants
# =============================================================================

class Os(object):
    """
    Operating system constants.
    These strings are cosmetic and should NOT be relied on for passing to
    external tools.
    """
    ANDROID = "Android"
    LINUX = "Linux"
    WINDOWS = "Windows"
    MACOS = "macOS"  # Named Mac OS X, then OS X, then macOS
    IOS = "iOS"


ALL_OSS = [getattr(Os, _) for _ in dir(Os) if not _.startswith("_")]


class Cpu(object):
    """
    CPU constants.

    These strings are cosmetic and should NOT be relied on for passing to
    external tools.

    Intel:

    - 32-bit x86 chips are usually called "x86", "i386", "i686"
    - 64-bit x86 chips are usually called "x86_64" or "amd64"

    ARM (see https://en.wikipedia.org/wiki/ARM_architecture,
    https://en.wikipedia.org/wiki/List_of_ARM_microarchitectures,
    https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html):

    - ARMv5 is 32-bit; typically called "arm"
    - ARMv7 is 32-bit, ?typically called "armeabi"
    - ARMv8 is 64/32-bit, typically called "aarch64" or "arm64"

    "EABI" is "embedded-application binary interface".

    """
    X86_32 = "Intel x86 (32-bit)"
    X86_64 = "Intel x86 (64-bit)"
    AMD_64 = "AMD (64-bit)"
    ARM_V5_32 = "ARM v5 (32-bit)"
    ARM_V7_32 = "ARM v7 (32-bit)"
    ARM_V8_64 = "ARM v8 (64/32-bit)"


ALL_CPUS = [getattr(Cpu, _) for _ in dir(Cpu) if not _.startswith("_")]


# =============================================================================
# Information about the target system
# =============================================================================

class Platform(object):
    """
    Represents the build or target platform, defined by OS+CPU.
    """
    # noinspection PyShadowingNames
    def __init__(self, os: str, cpu: str, distro_id: str = "") -> None:
        self.os = os
        self.cpu = cpu
        self.distro_id = distro_id
        if os not in ALL_OSS:
            raise NotImplementedError(f"Unknown target OS: {os!r}")
        if cpu not in ALL_CPUS:
            raise NotImplementedError(f"Unknown target CPU: {cpu!r}")

        # 64-bit support only (thus far)?
        if os in [Os.LINUX, Os.MACOS] and not self.cpu_x86_64bit_family:
            raise NotImplementedError(
                f"Don't know how to build for CPU {cpu} on system {os}")

    # -------------------------------------------------------------------------
    # Descriptives
    # -------------------------------------------------------------------------

    def __str__(self) -> str:
        return self.description

    def __eq__(self, other: "Platform") -> bool:
        return (
            self.os == other.os and
            self.cpu == other.cpu and
            self.distro_id == other.distro_id
        )

    @property
    def description(self) -> str:
        """
        Short description, for user information.
        """
        return f"{self.os}/{self.cpu}"

    @property
    def os_shortname(self) -> str:
        """
        Short OS name, used to make directory names for compilation.
        """
        if self.os == Os.ANDROID:
            return "android"
        elif self.os == Os.LINUX:
            return "linux"
        elif self.os == Os.WINDOWS:
            return "windows"
        elif self.os == Os.MACOS:
            return "macos"
        elif self.os == Os.IOS:
            return "ios"
        else:
            raise ValueError(f"Unknown OS: {self.os!r}")

    @property
    def cpu_shortname(self) -> str:
        """
        Short CPU name, used to make directory names for compilation.
        """
        if self.cpu == Cpu.X86_32:
            return "x86_32"
        elif self.cpu_x86_64bit_family:
            return "x86_64"
        elif self.cpu == Cpu.ARM_V5_32:
            return "armv5"
        elif self.cpu == Cpu.ARM_V7_32:
            return "armv7"
        elif self.cpu == Cpu.ARM_V8_64:
            return "armv8_64"
        else:
            raise ValueError(f"Unknown CPU: {self.cpu!r}")

    @property
    def dirpart(self) -> str:
        """
        Used to name our build directories.
        """
        return f"{self.os_shortname}_{self.cpu_shortname}"

    # -------------------------------------------------------------------------
    # OS/CPU information
    # -------------------------------------------------------------------------

    @property
    def linux(self) -> bool:
        return self.os == Os.LINUX

    @property
    def debian(self) -> bool:
        # http://distro.readthedocs.io/en/latest/#distro.id
        return self.distro_id in ["ubuntu", "debian"]

    @property
    def macos(self) -> bool:
        return self.os == Os.MACOS

    @property
    def unix(self) -> bool:
        return self.linux or self.macos

    @property
    def android(self) -> bool:
        return self.os == Os.ANDROID

    @property
    def ios(self) -> bool:
        return self.os == Os.IOS

    @property
    def windows(self) -> bool:
        return self.os == Os.WINDOWS

    @property
    def mobile(self) -> bool:
        return self.os in [Os.ANDROID, Os.IOS]

    @property
    def desktop(self) -> bool:
        return not self.mobile

    @property
    def cpu_x86_family(self) -> bool:
        return self.cpu in [Cpu.X86_32, Cpu.X86_64, Cpu.AMD_64]

    @property
    def cpu_64bit(self) -> bool:
        return self.cpu in [Cpu.X86_64, Cpu.AMD_64, Cpu.ARM_V8_64]

    @property
    def cpu_x86_64bit_family(self) -> bool:
        return self.cpu_x86_family and self.cpu_64bit

    @property
    def cpu_x86_32bit_family(self) -> bool:
        return self.cpu_x86_family and not self.cpu_64bit

    @property
    def cpu_arm_family(self) -> bool:
        return self.cpu in [Cpu.ARM_V5_32, Cpu.ARM_V7_32, Cpu.ARM_V8_64]

    @property
    def cpu_arm_32bit(self) -> bool:
        return self.cpu in [Cpu.ARM_V5_32, Cpu.ARM_V7_32]

    @property
    def cpu_arm_64bit(self) -> bool:
        return self.cpu in [Cpu.ARM_V8_64]

    # -------------------------------------------------------------------------
    # Linkage method of Qt
    # -------------------------------------------------------------------------
    @property
    def qt_linkage_static(self) -> bool:
        # NOT Android; dynamic linkage then bundling into single-file APK.
        return self.desktop or self.ios

    # -------------------------------------------------------------------------
    # Library (e.g. .so, DLL) verification
    # -------------------------------------------------------------------------

    @property
    def dynamic_lib_ext(self) -> str:
        """
        What DYNAMIC/SHARED library filename extension is in use?
        """
        # I think this depends on the build system, not the target.
        # ... no; on the target.
        if self.macos or self.ios:
            return ".dylib"
            # ... sometimes also ".so" or ".bundle"
        elif self.windows:
            return ".dll"
        else:
            return ".so"

    @property
    def static_lib_ext(self) -> str:
        """
        What STATIC library filename extension is in use?
        """
        if self.macos or self.ios:
            return ".a"
        elif self.windows:
            return ".lib"
        else:
            return ".a"

    @property
    def obj_ext(self):
        """
        What OBJECT file extension is in use?
        """
        if self.windows:
            return ".obj"
        else:
            return ".o"

    def ensure_elf_reader(self) -> None:
        """
        Checks we have an ELF (Executable and Linkable Format) file reader.
        Only to be called for the build platform.
        """
        if self.linux:
            require(READELF)
            require(OBJDUMP)  # for Windows DLL files
        elif self.macos:
            if self.cpu_64bit:
                require(OTOOL)
            else:
                require(GOBJDUMP)
        elif self.windows:
            pass
        else:
            raise NotImplementedError(
                f"Don't know ELF reader for {BUILD_PLATFORM}")

    def verify_lib(self, filename: str) -> None:
        """
        Check an ELF or DLL file matches our architecture.
        """
        # Testing:
        # - "Have I built for the right architecture?"
        #   http://stackoverflow.com/questions/267941
        #   http://stackoverflow.com/questions/1085137
        #
        #   file libssl.so
        #   objdump -a libssl.so  # or -x, or...
        #   readelf -h libssl.so
        #
        # - Compare to files on the Android emulator:
        #
        #   adb pull /system/lib/libz.so  # system
        #   adb pull /data/data/org.camcops.camcops/lib/  # ours
        #
        # ... looks OK

        log.info("Verifying type of library file: {!r}", filename)
        if BUILD_PLATFORM.linux:
            if self.windows:
                dumpcmd = [OBJDUMP, "-f", filename]
                dumpresult = fetch(dumpcmd)
                pe32_tag = "file format pe-i386"
                pe64_tag = "file format pe-x86-64"
                if self.cpu_64bit and pe64_tag not in dumpresult:
                    raise ValueError(f"File {filename!r} is not a Win64 DLL")
                if not self.cpu_64bit and pe32_tag not in dumpresult:
                    raise ValueError(f"File {filename!r} is not a Win32 DLL")
            else:
                elfcmd = [READELF, "-h", filename]
                elfresult = fetch(elfcmd)
                arm32_tag_present = bool(re.search(r"Machine:\s+ARM", elfresult))  # noqa
                arm64_tag_present = bool(re.search(r"Machine:\s+AArch64", elfresult))  # noqa
                arm_tag_present = arm32_tag_present or arm64_tag_present
                if self.cpu_arm_32bit and not arm32_tag_present:
                    raise ValueError(f"File {filename} was not built for 32-bit ARM")  # noqa
                if self.cpu_arm_64bit and not arm64_tag_present:
                    raise ValueError(f"File {filename} was not built for 64-bit ARM")  # noqa
                if (not self.cpu_arm_family) and arm_tag_present:
                    raise ValueError(f"File {filename} was built for ARM but shouldn't be")  # noqa
        elif BUILD_PLATFORM.macos:
            if BUILD_PLATFORM.cpu_64bit:
                dumpcmd = [OTOOL, "-hv", "-arch", "all", filename]
                dumpresult = fetch(dumpcmd)
                # https://stackoverflow.com/questions/1085137/how-do-i-determine-the-target-architecture-of-static-library-a-on-mac-os-x  # noqa
                # Output looks like [number of spaces not right]:
                #
                # Archive : FILENAME        -- this line not always present
                # libcrypto.a(aes_cfp.o)    -- (example) this line not always present
                # Mach header
                #       magic cputype cpusubtype  caps filetype sizeofcmds flags  # noqa
                #  0xfeedface     ARM         V7  0x00        6       1564 NOUNDEFS DYLDLINK TWOLEVEL NO_REEXPORTED_DYLIBS  # noqa
                lines = dumpresult.splitlines()
                arm64tag_present = False
                for line in lines:
                    words = line.split()
                    if words[0] in ["Archive", "Mach", "magic"] or words[0].startswith(filename):
                        continue
                    assert len(words) > 1, "Unknown format of otool output"
                    cputype = words[1]
                    arm64tag_present = cputype == "ARM64"
                    break
            else:
                # https://lowlevelbits.org/parsing-mach-o-files/
                # https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
                # gobjdump --help
                dumpcmd = [GOBJDUMP, "-f", filename]
                dumpresult = fetch(dumpcmd)
                arm64tag = "file format mach-o-arm64"
                arm64tag_present = arm64tag in dumpresult
            if self.cpu == Cpu.ARM_V8_64 and not arm64tag_present:
                raise ValueError(f"File {filename} was not built for ARM64")
            elif self.cpu != Cpu.ARM_V8_64 and arm64tag_present:
                raise ValueError(f"File {filename} was built for ARM64")
        else:
            log.warning("Don't know how to verify library under build "
                        "platform {}", BUILD_PLATFORM)
            return
        log.info("Library file is good: {!r}", filename)

    # -------------------------------------------------------------------------
    # Specific descriptives used by others
    # -------------------------------------------------------------------------

    @property
    def apple_arch_name(self) -> str:
        """
        Architecture name to pass to Xcode's clang etc. Don't alter.
        Architecture conversions:

        - https://stackoverflow.com/questions/27016612/compiling-external-c-library-for-use-with-ios-project

        Which architectures does Xcode's clang support?
        - https://stackoverflow.com/questions/15036909/clang-how-to-list-supported-target-architectures
        If in doubt, running "clang -arch SOMETHING" will produce an error
        if it's unsupported. With clang-703.0.29, "x86_64" and "arm6" are
        OK.

        - iOS device processor compatibility: see
          https://developer.apple.com/library/content/documentation/DeviceInformation/Reference/iOSDeviceCompatibility/DeviceCompatibilityMatrix/DeviceCompatibilityMatrix.html
        """  # noqa
        if self.cpu_x86_64bit_family:
            return "x86_64"
        elif self.cpu_x86_32bit_family:
            return "i386"
        elif self.cpu == Cpu.ARM_V7_32:
            return "armv7"
        elif self.cpu == Cpu.ARM_V8_64:
            return "arm64"
        else:
            raise ValueError(f"apple_arch_name(): Unknown CPU {self.cpu}")

    @property
    def apple_cpu_name_for_triplet(self) -> str:
        """
        CPU name to make a cpu-vendor-os triplet.

        See :meth:`apple_arch_name`.

        Note that "arm64" is a valid architecture but fails here (e.g.
        SQLCipher ``configure``) with ``Invalid configuration
        'arm64-apple-darwin': machine 'arm64-apple' not recognized". The
        solution is apparently to use ``arm-apple-darwin`` but pass ``-m64`` to
        clang, and/or pass ``-arch arm64`` to clang (the latter is more
        plausible to me); see
        https://github.com/tpoechtrager/cctools-port/issues/6.
        """  # noqa
        if self.cpu_x86_64bit_family:
            return "x86_64"
        elif self.cpu_x86_32bit_family:
            return "i386"
        elif self.cpu_arm_family:
            # See above
            return "arm"
        else:
            raise ValueError(f"apple_cpu_name_for_triplet(): "
                             f"Unknown CPU {self.cpu}")

    @property
    def linux_windows_cpu_name(self) -> str:
        """
        - https://www.gnu.org/software/autoconf/manual/autoconf-2.65/html_node/Specifying-Target-Triplets.html
        - https://superuser.com/questions/238112/what-is-the-difference-between-i686-and-x86-64
        """  # noqa
        if self.cpu_x86_32bit_family:
            return "i686"  # verified
        elif self.cpu_x86_64bit_family:
            return "x86_64"  # verified
        elif self.cpu == Cpu.ARM_V5_32:
            return "arm"  # I think
        elif self.cpu == Cpu.ARM_V7_32:
            return "armv7a"  # verified
        elif self.cpu == Cpu.ARM_V8_64:
            return "aarch64"  # verified
        else:
            raise NotImplementedError(
                f"linux_windows_cpu_name() doesn't know {self.cpu}")

    @property
    def triplet_cpu(self) -> str:
        """
        - https://www.gnu.org/software/autoconf/manual/autoconf-2.65/html_node/Specifying-Target-Triplets.html
        - https://superuser.com/questions/238112/what-is-the-difference-between-i686-and-x86-64
        """  # noqa
        if self.os in [Os.LINUX, Os.ANDROID, Os.WINDOWS]:
            return self.linux_windows_cpu_name
        elif self.os in [Os.MACOS, Os.IOS]:
            return self.apple_cpu_name_for_triplet
        else:
            raise NotImplementedError(f"triplet_cpu() doesn't know {self.cpu}")

    @property
    def triplet_vendor(self) -> str:
        if self.os in [Os.ANDROID]:
            return "linux"
        elif self.os in [Os.LINUX, Os.WINDOWS]:
            return "unknown"
        elif self.os in [Os.MACOS, Os.IOS]:
            return "apple"
        else:
            raise NotImplementedError(f"triplet_vendor() doesn't know {self.os}")

    @property
    def triplet_os(self) -> str:
        lookup = {
            Os.ANDROID: "android",
            # e.g. empirically: "i386-apple-darwin15.6.0"
            # "uname -m" tells you whether you're 32 or 64 bit
            # "uname -r" gives you the release
            Os.MACOS: "darwin",
            Os.IOS: "ios",
            Os.LINUX: "linux",
            Os.WINDOWS: "windows",
        }

        os_name = lookup.get(self.os, None)

        if os_name is None:
            raise NotImplementedError(f"triplet_os() doesn't know {self.os}")

        return os_name

    @property
    def target_triplet(self) -> str:
        """
        https://www.gnu.org/software/autoconf/manual/autoconf-2.65/html_node/Specifying-Target-Triplets.html
        """  # noqa
        cpu = self.triplet_cpu
        vendor = self.triplet_vendor
        os_ = self.triplet_os
        return f"{cpu}-{vendor}-{os_}"

    # -------------------------------------------------------------------------
    # Android
    # -------------------------------------------------------------------------

    @property
    def android_cpu(self) -> str:
        """
        CPU name for Android builds.
        Used by android_arch_short and in turn for various variables that get
        passed to compilers using the Android SDK. Don't alter them.

        See also

        - https://developer.android.com/ndk/guides/abis.html -- slightly
          different
        - <ANDROID_NDK_DIR>/platforms/android-<VERSION>/...
          e.g. ``arch-arm``, ``arch-x86``, ``arch-arm64``
        """
        if not self.android:
            raise ValueError("Platform is not Android")
        if self.cpu_x86_64bit_family:
            return "x86_64"
        elif self.cpu_x86_32bit_family:
            return "x86"
        elif self.cpu == Cpu.ARM_V7_32:
            return "arm"
        elif self.cpu == Cpu.ARM_V5_32:
            return "armv5"
        elif self.cpu == Cpu.ARM_V8_64:
            return "arm64"
        else:
            raise NotImplementedError("Don't know how to build Android for "
                                      "CPU " + self.cpu)

    @property
    def android_arch_short(self) -> str:
        """
        Needs to match Android SDK naming. Don't alter.
        """
        return self.android_cpu

    @property
    def android_arch_full(self) -> str:
        """
        Needs to match Android SDK naming. Don't alter.
        """
        # e.g. arch-x86
        return f"arch-{self.android_arch_short}"

    # -------------------------------------------------------------------------
    # iOS
    # -------------------------------------------------------------------------

    @property
    def ios_platform_name(self) -> str:
        """
        Needs to match iOS SDK naming. Don't alter.
        """
        if not self.ios:
            raise ValueError("ios_platform_name requested but not using iOS")
        if self.cpu_x86_family:
            return "iPhoneSimulator"
        elif self.cpu_arm_family:
            return "iPhoneOS"
        else:
            raise ValueError("Unknown combination for ios_platform_name")

    # -------------------------------------------------------------------------
    # Other cross-compilation details
    # -------------------------------------------------------------------------

    def _get_tool(self, tool: str, fullpath: bool, cfg: "Config") -> str:
        """
        Work out the name of an appropriate compilation/linkage/...
        tool
        """
        if not fullpath:
            return tool
        prefix = self.cross_compile_prefix(cfg)
        if self.android:
            tooldir = cfg.android_toolchain_bin_dir(self)
            return join(tooldir, prefix + tool)
        return shutil.which(prefix + tool) or shutil.which(tool)

    def gcc(self, fullpath: bool, cfg: "Config") -> str:
        """
        Work out the name of an appropriate gcc compiler.
        """
        return self._get_tool(GCC, fullpath, cfg)

    def clang(self, fullpath: bool, cfg: "Config") -> str:
        """
        Work out the name of an appropriate clang compiler.
        """
        return self._get_tool(CLANG, fullpath, cfg)

    def ar(self, fullpath: bool, cfg: "Config") -> str:
        """
        Work out the name of an appropriate ar assembler.
        """
        return (
            self._get_tool(GCC_AR, fullpath, cfg) or
            self._get_tool(AR, fullpath, cfg)
        )

    def cross_compile_prefix(self, cfg: "Config") -> str:
        """
        Work out the CROSS_COMPILE environment variable/prefix.

        See e.g. <ANDROID_NDK_DIR>/toolchains

        Examples:

        .. code-block: none

            android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android23-clang
                                                                      ^^^^^^^^^^^^^^^^^^^^^^^^

            android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi23-clang
                                                                      ^^^^^^^^^^^^^^^^^^^^^^^^^^^

            android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/bin/i686-linux-android23-clang
                                                                      ^^^^^^^^^^^^^^^^^^^^^

            android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/bin/x86_64-linux-android23-clang
                                                                      ^^^^^^^^^^^^^^^^^^^^^^^

        """
        if BUILD_PLATFORM == self:
            # Compiling for the platform we're running on.
            return ""
        suffix = ""
        if self.android and cfg.android_clang_not_gcc:
            if self.cpu == Cpu.ARM_V7_32:
                suffix += "eabi"
            suffix += str(cfg.android_sdk_version)
        return f"{self.target_triplet}{suffix}-"

    # -------------------------------------------------------------------------
    # Support for make, cmake
    # -------------------------------------------------------------------------

    def make_args(self, cfg: "Config", extra_args: List[str] = None,
                  command: str = "", makefile: str = "",
                  env: Dict[str, str] = None) -> List[str]:
        """
        Generates command arguments for "make" or a platform equivalent.
        """
        extra_args = extra_args or []  # type: List[str]
        env = env if env is not None else os.environ
        if self.windows:
            if which_with_envpath(cfg.jom_executable, env):
                make = cfg.jom_executable
                supports_parallel = True
            else:
                make = which_with_envpath(NMAKE, env)
                supports_parallel = False
            makefile_switch = "/F"
            parallel_switch = "/J"
        else:
            make = MAKE
            supports_parallel = True
            makefile_switch = "-f"  # Unix standard
            parallel_switch = "-j"
        # require(make)
        # ... not necessarily visible now; may be on a PATH yet to be set
        args = [make]
        if supports_parallel:
            args += [parallel_switch, str(cfg.nparallel)]
        if extra_args:
            args += extra_args
        if makefile:
            args += [makefile_switch, makefile]
        if command:
            args.append(command)
        return args

    @property
    def qmake_executable(self) -> str:
        """
        Used to calculate the name of the qmake file we'll be building (so we
        can check if it's been compiled).
        """
        if self.windows:
            return "qmake.exe"
        else:
            return "qmake"

    # -------------------------------------------------------------------------
    # SQLCipher
    # -------------------------------------------------------------------------

    @property
    def sqlcipher_platform(self) -> str:
        """
        Generates the name of a platform to be passed to SQLCipher's configure
        tool (either build or target).

        Within SQLCipher, the program that parses these is config.sub

        See also
        https://www.gnu.org/software/autoconf/manual/autoconf-2.65/html_node/Specifying-Target-Triplets.html
        """  # noqa
        # See sqlcipher/config.sub, sqlcipher/config.guess
        # (You can run or source config.guess to see its answer.)
        return self.target_triplet


def get_build_platform() -> Platform:
    """
    Find the architecture this script is running on.
    """
    s = platform.system()
    if s == "Linux":
        os_ = Os.LINUX
    elif s == "Darwin":
        os_ = Os.MACOS
    elif s == "Windows":
        os_ = Os.WINDOWS
    else:
        raise NotImplementedError(f"Don't know host (build) OS {s!r}")
    m = platform.machine()
    if m == "i386":
        cpu = Cpu.X86_32
    elif m == "x86_64":
        cpu = Cpu.X86_64
    elif m == "AMD64":
        cpu = Cpu.AMD_64
    else:
        raise NotImplementedError(f"Don't know host (build) CPU {m!r}")
    distro_id = distro.id() if distro else ""
    return Platform(os_, cpu, distro_id)


BUILD_PLATFORM = get_build_platform()


# =============================================================================
# Config class, just to make sure we check the argument namespace properly
# =============================================================================
# https://stackoverflow.com/questions/42279063/python-typehints-for-argparse-namespace-objects  # noqa

class Config(object):
    # noinspection PyUnresolvedReferences
    def __init__(self, args: argparse.Namespace) -> None:
        # Architectures
        self.build_all = args.build_all  # type: bool
        self.build_android_x86_32 = args.build_android_x86_32  # type: bool
        self.build_android_arm_v7_32 = args.build_android_arm_v7_32  # type: bool  # noqa
        self.build_android_arm_v8_64 = args.build_android_arm_v8_64  # type: bool  # noqa
        self.build_linux_x86_64 = args.build_linux_x86_64  # type: bool
        self.build_macos_x86_64 = args.build_macos_x86_64  # type: bool
        self.build_windows_x86_64 = args.build_windows_x86_64  # type: bool
        self.build_windows_x86_32 = args.build_windows_x86_32  # type: bool
        self.build_ios_arm_v7_32 = args.build_ios_arm_v7_32  # type: bool
        self.build_ios_arm_v8_64 = args.build_ios_arm_v8_64  # type: bool
        self.build_ios_simulator_x86_32 = args.build_ios_simulator_x86_32  # type: bool  # noqa
        self.build_ios_simulator_x86_64 = args.build_ios_simulator_x86_64  # type: bool  # noqa

        if self.build_all:
            if BUILD_PLATFORM.linux:
                # Linux
                self.build_linux_x86_64 = True
                # Android
                self.build_android_arm_v7_32 = True
                # rarely used, emulator only # self.build_android_x86_32 = True
                self.build_android_arm_v8_64 = True
            elif BUILD_PLATFORM.macos:
                # MacOS
                self.build_macos_x86_64 = True
                # iOS
                self.build_ios_arm_v7_32 = True
                self.build_ios_arm_v8_64 = True
                # iOS simulators for MacOS
                if BUILD_PLATFORM.cpu_64bit:
                    self.build_ios_simulator_x86_64 = True
                else:
                    self.build_ios_simulator_x86_32 = True
            elif BUILD_PLATFORM.windows:
                self.build_windows_x86_32 = True
                self.build_windows_x86_64 = True

        # General
        self.show_config_only = args.show_config_only  # type: bool
        self.root_dir = args.root_dir  # type: str
        self.nparallel = args.nparallel  # type: int
        self.force = args.force  # type: bool
        self.verbose = args.verbose  # type: int
        self.src_rootdir = join(self.root_dir, "src")  # type: str
        self.tee_filename = args.tee  # type: str
        self.inherit_os_env = args.inherit_os_env  # type: bool

        # Qt
        # - git repository in src/qt5
        # - build to multiple directories off root
        # - each is (1) built into the "*_build" directory, then installed
        #   (via "make install") to the "*_install" directory.
        # - One points Qt Creator to "*_install/bin/qmake" to give it a Qt
        #   architecture "kit".
        self.qt_build_type = args.qt_build_type  # type: str
        assert self.qt_build_type in QT_POSSIBLE_BUILD_TYPES
        self.qt_git_url = QT_GIT_URL  # type: str
        self.qt_git_branch = QT_GIT_BRANCH  # type: str
        self.qt_git_commit = QT_GIT_COMMIT  # type: str
        self.qt_openssl_static = args.qt_openssl_static  # type: bool
        self.qt_src_gitdir = join(self.src_rootdir,
                                  args.qt_src_dirname)  # type: str  # noqa

        # Android SDK/NDK
        # - installed independently by user
        # - used for cross-compilation to Android targets
        self.android_sdk_version = ANDROID_SDK_VERSION
        self.android_ndk_version = ANDROID_NDK_VERSION
        self.android_clang_not_gcc = self.android_ndk_version >= 18
        self.android_sdk_root = args.android_sdk_root  # type: str
        self.android_ndk_root = args.android_ndk_root  # type: str
        self.android_ndk_host = args.android_ndk_host  # type: str
        self.android_toolchain_version = args.android_toolchain_version  # type: str  # noqa
        self.android_api = f"android-{self.android_sdk_version}"
        # ... see $ANDROID_SDK_ROOT/platforms/
        self.android_ndk_platform = self.android_api
        self.java_home = args.java_home  # type: str

        # iOS
        self.ios_sdk = args.ios_sdk  # type: str
        self.ios_min_version = MIN_IOS_VERSION  # type: str

        # macOS
        self.macos_min_version = MIN_MACOS_VERSION  # type: str

        # OpenSSL
        # - download tar file to src/openssl
        # - built to multiple directories off root
        self.openssl_version = OPENSSL_VERSION  # type: str
        self.openssl_src_url = OPENSSL_SRC_URL  # type: str
        # self.openssl_android_script_url = OPENSSL_ANDROID_SCRIPT_URL  # type: str  # noqa
        # ... derived:
        self.openssl_src_dir = join(self.src_rootdir, "openssl")
        self.openssl_src_filename = f"openssl-{self.openssl_version}.tar.gz"
        self.openssl_src_fullpath = join(self.openssl_src_dir,
                                         self.openssl_src_filename)
        self.openssl_android_script_fullpath = join(
            self.openssl_src_dir, "Setenv-android.sh")

        # SQLCipher
        # - git repository in "src"
        # - single build in situ; we make the amalgamation file "sqlite3.c",
        #   and as a bonus the "sqlcipher" executable for the host machine, but
        #   the latter isn't part of CamCOPS's use of SQLCipher (CamCOPS just
        #   uses the amalgamation file, which is platform-independent).
        self.sqlcipher_git_url = SQLCIPHER_GIT_URL  # type: str
        self.sqlcipher_git_commit = SQLCIPHER_GIT_COMMIT  # type: str
        self.sqlcipher_src_gitdir = join(self.src_rootdir, "sqlcipher")  # type: str  # noqa

        # Eigen
        # Changed location from bitbucket:
        # https://gitlab.com/libeigen/eigen/-/archive/3.3.8/eigen-3.3.8.tar.gz
        self.eigen_version = EIGEN_VERSION  # type: str
        self.eigen_src_url = (
            f"https://gitlab.com/libeigen/eigen/-/archive/{self.eigen_version}/eigen-{self.eigen_version}.tar.gz"  # noqa
        )

        self.eigen_src_dir = join(self.src_rootdir, "eigen")
        self.eigen_src_fullpath = join(
            self.eigen_src_dir,
            f"eigen-{self.eigen_version}.tar.gz")
        self.eigen_unpacked_dir = join(self.root_dir, "eigen")

        # jom: comes with QtCreator
        # self.jom_git_url = args.jom_git_url  # type: str
        # self.jom_src_gitdir = join(self.src_rootdir, "jom")  # type: str
        # self.jom_executable = join(self.jom_src_gitdir, "bin", "jom.exe") # type: str  # noqa
        self.jom_executable = args.jom_executable  # type: str

        self._cached_xcode_developer_path = ""

    def __repr__(self) -> str:
        elements = [f"    {k}={repr(v)}"
                    for k, v in self.__dict__.items()]
        elements.sort()
        return "{q}(\n{e}\n)".format(q=self.__class__.__qualname__,
                                     e=",\n".join(elements))

    # -------------------------------------------------------------------------
    # Directories we'll write to
    # -------------------------------------------------------------------------

    def qt_build_dir(self, target_platform: Platform) -> str:
        """
        The directory in which we will compile and build Qt.
        """
        return join(
            self.root_dir,
            f"qt_{target_platform.dirpart}_build{self._qt_dir_suffix()}")

    def qt_install_dir(self, target_platform: Platform) -> str:
        """
        The directory to which we'll install Qt, culminating in the "qmake"
        tool.
        """
        return join(
            self.root_dir,
            f"qt_{target_platform.dirpart}_install{self._qt_dir_suffix()}")

    def _qt_dir_suffix(self) -> str:
        if self.qt_build_type == QT_BUILD_RELEASE:
            return ""
        elif self.qt_build_type == QT_BUILD_DEBUG:
            return "_debug"
        elif self.qt_build_type == QT_BUILD_RELEASE_WITH_SYMBOLS:
            return "_release_symbols"
        else:
            raise ValueError("Bad qt_build_type")

    def get_openssl_rootdir_workdir(
            self, target_platform: Platform) -> Tuple[str, str]:
        """
        Calculates local OpenSSL directories: the rootdir (where we unpack
        OpenSSL) and the workdir (a subdirectory of the rootdir, where the
        interesting stuff lives).
        """
        rootdir = join(self.root_dir,
                       f"openssl_{target_platform.dirpart}_build")
        workdir = join(rootdir, f"openssl-{self.openssl_version}")
        return rootdir, workdir

    # -------------------------------------------------------------------------
    # Compile/make tools
    # -------------------------------------------------------------------------

    def make_args(self, extra_args: List[str] = None, command: str = "",
                  makefile: str = "", env: Dict[str, str] = None) -> List[str]:
        """
        Returns command arguments for "make" or a platform equivalent.
        """
        return BUILD_PLATFORM.make_args(cfg=self,
                                        extra_args=extra_args,
                                        command=command,
                                        makefile=makefile,
                                        env=env)

    # -------------------------------------------------------------------------
    # Environment variables
    # -------------------------------------------------------------------------

    def get_starting_env(self) -> Dict[str, str]:
        """
        Returns an operating system environment to begin manipulating. This is
        usually a copy of ``os.environ()`` but could be a heavily cut-down
        version of that.
        """
        plain = not self.inherit_os_env
        # 1. Beware "plain" under Windows. Some other parent environment
        # variables needed for Visual C++ compiler, or you get "cannot
        # create temporary il file" errors. Not sure which, though;
        # APPDATA, TEMP and TMP are not sufficient.
        # 2. Beware "plain" under macOS; complains about missing "HOME"
        # variable.
        if plain:
            env = {}  # type: Dict[str, str]
            keys = ["PATH"]
            # if BUILD_PLATFORM.windows:
            #     keys += ["APPDATA", "TEMP", "TMP"]
            for k in keys:
                if k in os.environ:
                    env[k] = os.environ[k]
            return env
        else:
            return os.environ.copy()

    def set_compile_env(
            self,
            env: Dict[str, str],
            target_platform: Platform,
            use_cross_compile_var: bool = True,
            building_sqlcipher: bool = False) -> None:
        """
        Adds variables to the environment for compilation or cross-compilation.
        Modifies env.
        """
        if target_platform.android:
            self._set_android_env(
                env,
                target_platform=target_platform,
                use_cross_compile_var=use_cross_compile_var)
        elif target_platform.ios:
            self._set_ios_env(env, target_platform=target_platform)
        elif target_platform.linux:
            self._set_linux_env(env,
                                use_cross_compile_var=use_cross_compile_var)
        elif target_platform.windows:
            self._set_windows_env(env, target_platform=target_platform)
        elif target_platform.macos:
            self._set_macos_env(env, target_platform=target_platform,
                                building_sqlcipher=building_sqlcipher)
        else:
            raise NotImplementedError(
                f"Don't know how to set compilation environment "
                f"for {target_platform}")

    def sysroot(self, target_platform: Platform, env: Dict[str, str]) -> str:
        """
        Gets the sysroot (e.g. where system #include files live) for a specific
        target platform.

        Under Windows, we look at the environment, which will have been set
        by VCVARSALL.BAT.
        """
        if target_platform.android:
            return self._android_sysroot(target_platform)
        elif target_platform.ios:
            return self._xcode_sdk_path(
                xcode_platform=target_platform.ios_platform_name,
                sdk_version=self._get_ios_sdk_version(
                    target_platform=target_platform))
        elif target_platform.linux or target_platform.macos:
            return "/"  # default sysroot
        elif target_platform.windows:
            return env["WindowsSdkDir"]
        raise NotImplementedError(
            f"Don't know sysroot for target: {target_platform}")

    # -------------------------------------------------------------------------
    # Linux
    # -------------------------------------------------------------------------

    def _set_linux_env(self,
                       env: Dict[str, str],
                       use_cross_compile_var: bool) -> None:
        """
        Implementation of :meth:`set_compile_env` for Linux targets.
        """
        env["AR"] = BUILD_PLATFORM.ar(fullpath=not use_cross_compile_var,
                                      cfg=self)
        env["CC"] = BUILD_PLATFORM.gcc(fullpath=not use_cross_compile_var,
                                       cfg=self)

    # -------------------------------------------------------------------------
    # Android
    # -------------------------------------------------------------------------

    def _set_android_env(self,
                         env: Dict[str, str],
                         target_platform: Platform,
                         use_cross_compile_var: bool) -> None:
        """
        Implementation of :meth:`set_compile_env` for Android targets.
        """
        android_sysroot = self._android_sysroot(target_platform)
        android_toolchain = self.android_toolchain_bin_dir(target_platform)

        env["ANDROID_API"] = self.android_api
        env["ANDROID_API_VERSION"] = self.android_api
        env["ANDROID_ARCH"] = target_platform.android_arch_full
        env["ANDROID_DEV"] = join(android_sysroot, "usr")
        env["ANDROID_EABI"] = self._android_eabi(target_platform)
        env["ANDROID_NDK_ROOT"] = self.android_ndk_root
        env["ANDROID_SDK_ROOT"] = self.android_sdk_root
        env["ANDROID_SYSROOT"] = android_sysroot
        env["ANDROID_TOOLCHAIN"] = android_toolchain
        env["AR"] = target_platform.ar(fullpath=not use_cross_compile_var,
                                       cfg=self)
        env["ARCH"] = target_platform.android_arch_short
        env["CC"] = self._android_cc(
            target_platform,
            fullpath=self.android_clang_not_gcc or not use_cross_compile_var
        )
        if use_cross_compile_var:
            env["CROSS_COMPILE"] = target_platform.cross_compile_prefix(self)
            # ... unnecessary as we are specifying AR, CC directly
        env["HOSTCC"] = BUILD_PLATFORM.gcc(fullpath=not use_cross_compile_var,
                                           cfg=self)
        env["JAVA_HOME"] = self.java_home  # added 2019-06-16 for Qt
        env["PATH"] = os.pathsep.join([
            android_toolchain,
            join(self.java_home, "bin"),  # added 2019-06-16 for Qt
            env['PATH'],
        ])
        env["SYSROOT"] = android_sysroot
        env["NDK_SYSROOT"] = android_sysroot

    # -------------------------------------------------------------------------
    # Android
    # -------------------------------------------------------------------------

    def _android_eabi(self, target_platform: Platform) -> str:
        """
        Get the name of the Android Embedded Application Binary Interface
        for ARM processors, used for the Android SDK.

        ABIs:

        - https://developer.android.com/ndk/guides/abis.html

        ARM supports two ABI types, one of which is the Embedded ABI:

        - http://kanj.githib.io/elfs/book/armMusl/cross-tools/abi.html
        - https://www.eecs.umich.edu/courses/eeecs373/readings/ARM-AAPCS-EABI-v2.08.pdf
          = Procedure Call Standard for the ARM Architecture
        """  # noqa
        if target_platform.cpu_x86_family:
            return "{}-{}".format(
                target_platform.android_arch_short,
                self.android_toolchain_version)  # e.g. x86-4.9
            # For toolchain version: ls $ANDROID_NDK_ROOT/toolchains
            # ... "-android-arch" and "-android-toolchain-version" get
            # concatenated, I think; for example, this gives the toolchain
            # "x86_64-4.9"
        elif target_platform.cpu_arm_family:
            if self.android_clang_not_gcc:
                return "llvm"
            elif target_platform.cpu == Cpu.ARM_V8_64:
                # e.g. "aarch64-linux-android-4.9"
                return "aarch-linux-android-{}".format(
                    target_platform.android_arch_short,
                    self.android_toolchain_version)
            else:
                # but ARM ones look like "arm-linux-androideabi-4.9"
                return "{}-linux-androideabi-{}".format(
                    target_platform.android_arch_short,
                    self.android_toolchain_version)
        else:
            raise NotImplementedError("Unknown CPU family for Android")

    def _android_sysroot(self, target_platform: Platform) -> str:
        """
        Get the Android sysroot (e.g. where system #include files live) for a
        specific target platform.

        e.g.

        - android-ndk-r11c/platforms/android-23/arch-x86_64

        - ... and for Qt, same principle for the android-ndk-20 with clang; see
          qt5/qtbase/mkspecs/common/android*.conf.

        - However, Qt manages by itself and we don't need to tell it the
          sysroot; what we do here is primarily for OpenSSL etc.

        - android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/sysroot
          or android-ndk-r20/sysroot -- they are quite similar but there is
          more in the former.
        """
        if self.android_clang_not_gcc:
            # return join(self.android_ndk_root, "sysroot")
            return join(self.android_toolchain_root_dir(target_platform),
                        "sysroot")
        return join(self.android_ndk_root, "platforms",
                    self.android_api, target_platform.android_arch_full)

    def android_toolchain_root_dir(self, target_platform: Platform) -> str:
        """
        Top directory of the Android toolchain.

        e.g.

        - android-ndk-r11c/toolchains/x86_64-4.9/prebuilt/linux-x86_64
        - android-ndk-r20/toolchains/llvm/prebuilt/linux-x86_64/
        """
        return join(self.android_ndk_root, "toolchains",
                    self._android_eabi(target_platform),
                    "prebuilt", self.android_ndk_host)

    def android_toolchain_bin_dir(self, target_platform: Platform) -> str:
        """
        Directory of the Android toolchain ``bin`` directory,
        where compilers (etc.) are found.
        """
        return join(self.android_toolchain_root_dir(target_platform), "bin")

    def _android_cc(self, target_platform: Platform,
                    fullpath: bool) -> str:
        """
        Gets the name of a compiler for Android.

        Remember, Environment variables:

        - CC is the C compiler, e.g. "clang", "gcc"
        - CXX is the C++ compiler, "clang++", "g++"
        - CPP, if used, is the C preprocessor
        """
        # Don't apply the CROSS_COMPILE prefix; that'll be prefixed
        # automatically.
        if self.android_clang_not_gcc:
            return target_platform.clang(fullpath, cfg=self)
        else:
            return (
                target_platform.gcc(fullpath, cfg=self) + "-" +
                self.android_toolchain_version
            )

    # -------------------------------------------------------------------------
    # Android conversion functions
    # -------------------------------------------------------------------------

    def convert_android_lib_a_to_so(self, lib_a_fullpath: str,
                                    target_platform: Platform) -> str:
        """
        Converts an Android library from static (.a) to dynamic (.so) format.
        """
        # https://stackoverflow.com/questions/3919902/method-of-converting-a-static-library-into-a-dynamically-linked-library  # noqa
        libprefix = "lib"
        directory, filename = split(lib_a_fullpath)
        basename, ext = os.path.splitext(filename)
        if not basename.startswith(libprefix):
            raise ValueError("Don't know how to convert library: " +
                             lib_a_fullpath)
        libname = basename[len(libprefix):]
        newlibbasename = libprefix + libname + ".so"
        newlibfilename = join(directory, newlibbasename)
        compiler = self._android_cc(target_platform, fullpath=True)
        run([
            compiler,
            "-o", newlibfilename,
            "-shared",
            "-Wl,--whole-archive",
            "-Wl,-soname," + newlibbasename,
            lib_a_fullpath,
            "-Wl,--no-whole-archive",
            # "-L{}".format(directory),
            # "-l{}".format(libname),
            f"--sysroot={self._android_sysroot(target_platform)}",
        ])
        target_platform.verify_lib(newlibfilename)
        return newlibfilename

    # -------------------------------------------------------------------------
    # MacOS, iOS
    # -------------------------------------------------------------------------

    # noinspection PyUnusedLocal
    def _set_macos_env(self, env: Dict[str, str],
                       target_platform: Platform,
                       building_sqlcipher: bool = False) -> None:
        """
        Implementation of :meth:`set_compile_env` for macOS targets.
        """
        # https://gist.github.com/armadsen/b30f352a8d6f6c87a146
        require(CLANG)
        env["BUILD_TOOLS"] = env.get("BUILD_TOOLS", self._xcode_developer_path)
        if building_sqlcipher:
            pass
            # must instead modify CFLAGS to the SQLCipher "configure" tool;
            # see build_sqlcipher()
        else:
            # This bit breaks SQLCipher compilation for macOS, which wants to
            # autodiscover gcc:
            env["CC"] = (
                f"{shutil.which(CLANG)} "
                f"-mmacosx-version-min={self.macos_min_version}"
            )
            # ... but it's necessary for OpenSSL.

    def _set_ios_env(self, env: Dict[str, str],
                     target_platform: Platform) -> None:
        """
        Implementation of :meth:`set_compile_env` for iOS targets.
        """
        # https://gist.github.com/foozmeat/5154962
        # https://stackoverflow.com/questions/27016612/compiling-external-c-library-for-use-with-ios-project  # noqa
        # https://gist.github.com/armadsen/b30f352a8d6f6c87a146

        use_gcc = True  # https://gist.github.com/armadsen/b30f352a8d6f6c87a146

        xcode_platform = target_platform.ios_platform_name
        arch = target_platform.apple_arch_name
        developer = self._xcode_developer_path
        sdk_version = self._get_ios_sdk_version(target_platform=target_platform)
        sdk_name = self._xcode_sdk_name(xcode_platform=xcode_platform,
                                        sdk_version=sdk_version)
        sdk_name_lower = sdk_name.lower()
        # ... must be lower-case for some functions. Try:
        #     xcodebuild -showsdks
        #     xcrun -sdk <sdkname> -find clang
        sysroot = self._xcode_sdk_path(xcode_platform=xcode_platform,
                                       sdk_version=sdk_version)
        escaped_sysroot = escape_literal_for_shell(sysroot)

        env["AR"] = fetch([XCRUN, "-sdk", sdk_name_lower,
                           "-find", "ar"]).strip()
        env["BUILD_TOOLS"] = developer
        if use_gcc:
            env["CC"] = (
                f"{os.path.join(developer, 'usr', 'bin', GCC)} "
                f"-fembed-bitcode "
                f"-mios-version-min={self.ios_min_version} "
                f"-arch {arch}"
            )
        else:
            env["CC"] = fetch([XCRUN, "-sdk", sdk_name_lower,
                               "-find", CLANG]).strip()

        cflags = [
            f"-arch {arch}",
            f"-isysroot {escaped_sysroot}",
            f"-m{xcode_platform.lower()}-version-min={self.ios_min_version}",
            # ... likely to be "-miphoneos-version-min"
            # "--sysroot={}".format(sysroot),
        ]
        env["CFLAGS"] = " ".join(cflags)
        env["CPP"] = env["CC"] + " -E"
        env["CPPFLAGS"] = env["CFLAGS"]
        env["CROSS_TOP"] = self._xcode_platform_dev_path(
            xcode_platform=xcode_platform)
        env["CROSS_SDK"] = sdk_name + ".sdk"
        env["LDFLAGS"] = f"-arch {arch} -isysroot {escaped_sysroot}"
        env["PLATFORM"] = xcode_platform
        env["RANLIB"] = fetch([XCRUN, "-sdk", sdk_name_lower, "-find",
                               "ranlib"]).strip()
        # env["SYSROOT"] = sysroot
        # ... see https://forums.developer.apple.com/thread/100545
        # ... but the problem we have is the makefile from OpenSSL configure

    @property
    def _xcode_developer_path(self) -> str:
        """
        Find XCode (the compiler suite under macOS).
        """
        if not self._cached_xcode_developer_path:
            self._cached_xcode_developer_path = fetch(
                [XCODE_SELECT, "-print-path"]).strip()
        return self._cached_xcode_developer_path
        # e.g. "/Applications/Xcode.app/Contents/Developer"

    @property
    def _xcode_platforms_path(self) -> str:
        """
        Find the directory in which XCode stores its target platforms.
        """
        return join(self._xcode_developer_path, "Platforms")

    @property
    def _xcode_tools_path(self) -> str:
        """
        Find the XCode default toolchain.
        """
        return join(self._xcode_developer_path,
                    "Toolchains", "XcodeDefault.xctoolchain", "usr", "bin")

    def _xcode_platform_dev_path(self, xcode_platform: str) -> str:
        """
        Find the XCode Developer path for a specific target platform.
        """
        return join(self._xcode_platforms_path,
                    f"{xcode_platform}.platform",
                    "Developer")

    def _xcode_all_sdks_path(self, xcode_platform: str) -> str:
        """
        Find the directory in which all SDK versions for the specified platform
        live.
        """
        return join(self._xcode_platform_dev_path(xcode_platform), "SDKs")

    @staticmethod
    def _xcode_sdk_name(xcode_platform: str, sdk_version: str) -> str:
        """
        Find the short name of a specific SDK version for a platform.
        """
        return f"{xcode_platform}{sdk_version}"

    def _xcode_sdk_path(self, xcode_platform: str, sdk_version: str) -> str:
        """
        Find the path to a specific platform SDK.
        """
        return join(self._xcode_all_sdks_path(xcode_platform),
                    self._xcode_sdk_name(xcode_platform=xcode_platform,
                                         sdk_version=sdk_version) + ".sdk")

    def _get_latest_ios_sdk_version(self, target_platform: Platform,
                                    xcode_platform: str = "",
                                    default: str = "8.0") -> str:
        """
        Get the version as a string, e.g. "9.3", of the latest SDK available
        for iOS for the specified platform.
        """
        # https://stackoverflow.com/questions/27016612/compiling-external-c-library-for-use-with-ios-project  # noqa
        xcode_platform = xcode_platform or target_platform.ios_platform_name
        sdkpath = self._xcode_all_sdks_path(xcode_platform)
        stdout = fetch(["ls", sdkpath])
        sdks = [x for x in stdout.splitlines() if x]
        # log.debug(sdks)
        if not sdks:
            log.warning("No iOS SDKs found in {}", sdkpath)
            return default
        latest_sdk = sdks[-1]  # Last item will be the current SDK, since they are alphanumerically ordered  # noqa
        suffix = ".sdk"
        sdk_name = latest_sdk[:-len(suffix)]  # remove the trailing ".sdk"
        sdk_version = sdk_name[len(xcode_platform):]  # remove the leading prefix, e.g. "iPhoneOS"  # noqa
        # log.debug("iOS SDK version: {!r}", sdk_version)
        return sdk_version

    def _get_ios_sdk_version(self, target_platform: Platform) -> str:
        """
        Get the iOS SDK version to use: either the one the user said, or the
        latest we can find.
        """
        return self.ios_sdk or self._get_latest_ios_sdk_version(
            target_platform=target_platform)

    # -------------------------------------------------------------------------
    # Windows
    # -------------------------------------------------------------------------

    def _set_windows_env(self, env: Dict[str, str],
                         target_platform: Platform) -> None:
        """
        Implementation of :meth:`set_compile_env` for Windows targets.
        """
        if BUILD_PLATFORM.linux:
            raise NotImplementedError(CANNOT_CROSS_COMPILE_QT)

        elif BUILD_PLATFORM.windows:
            # http://doc.qt.io/qt-5/windows-building.html
            if contains_unquoted_ampersand_dangerous_to_windows(env["PATH"]):
                fail(BAD_WINDOWS_PATH_MSG + env["PATH"])
            # PATH
            env["PATH"] = os.pathsep.join([
                join(self.qt_src_gitdir, "qtrepotools", "bin"),
                join(self.qt_src_gitdir, "qtbase", "bin"),
                join(self.qt_src_gitdir, "gnuwin32", "bin"),
                env["PATH"],
            ])
            # VCVARSALL.BAT

            # We can't CALL a batch file and have it change our environment,
            # so we must implement the functionality of VCVARSALL.BAT <arch>
            if target_platform.cpu_x86_32bit_family:
                # "x86" in VC\vcvarsall.bat
                arch = "x86"
            elif target_platform.cpu_x86_64bit_family:
                # "amd64" in VC\vcvarsall.bat
                arch = "amd64"
            else:
                raise NotImplementedError(
                    f"Don't know how to compile for Windows for target "
                    f"platform {target_platform}")
            # Now read the result from vcvarsall.bat directly
            args = [VCVARSALL, arch]
            fetched_env = windows_get_environment_from_batch_command(
                env_cmd=args, initial_env=env
            )
            env.update(**fetched_env)
            # Other
            env["CC"] = CL  # Visual C++
            # ... for SQLCipher "configure": if we try gcc, it will fail to
            # match the object format of OpenSSL that we previously created
            # using cl, so the configuration step will fail. We have to use cl
            # throughout.

            # Sanity checks
            if contains_unquoted_ampersand_dangerous_to_windows(env["PATH"]):
                fail(BAD_WINDOWS_PATH_MSG + env["PATH"])

        else:
            raise NotImplementedError(
                f"Don't know how to compile for Windows on build platform "
                f"{BUILD_PLATFORM}")


# =============================================================================
# Ancillary: crash out informatively
# =============================================================================

def fail(msg: str) -> NoReturn:
    log.critical(msg)
    raise ValueError(msg)


# =============================================================================
# Ancillary: environment and shell handling
# =============================================================================

def escape_literal_for_shell(x: str) -> str:
    """
    Double-quote a path if it has spaces or quotes in, for use particularly
    with:

        somecommand --cflags="--someflag --sysroot=SOMETHING"

    ... where that will eventually be passed (via configure) to ANOTHER command
    as

        compiler --someflag --sysroot=SOMETHING

    and we might have spaces in SOMETHING.

    I'm not certain this is particularly generic, so haven't moved it to
    cardinal_pythonlib.
    """
    assert not BUILD_PLATFORM.windows, (
        "Windows has terrible shell escaping and we use other methods")
    space = ' '
    dquote = '"'
    backslash = '\\'
    must_quote = [space, dquote]
    something_needs_quoting = any(c in x for c in must_quote)
    if not something_needs_quoting:
        return x
    x = x.replace(dquote, backslash + dquote)
    # if BUILD_PLATFORM.windows:
    #     # https://stackoverflow.com/questions/41607045
    #     if x.endswith(backslash):
    #         x += backslash
    # else:
    x = x.replace(backslash, backslash + backslash)
    x = f'"{x}"'
    return x


# =============================================================================
# Ancillary: check for operating system commands
# =============================================================================

UBUNTU_PACKAGE_HELP = """
Linux (Ubuntu)
-------------------------------------------------------------------------------

ar          } Should be pre-installed!
cmake       }       ... sudo apt install cmake
gcc         }
gobjdump    }
readelf     }

ant         sudo apt install ant
javac       sudo apt install openjdk-8-jdk

"""

UBUNTU_PACKAGE_HELP_DEFUNCT = """
Linux (Ubuntu) (DEFUNCT)
-------------------------------------------------------------------------------
*mingw*     } sudo apt install mingw-w64
*windres    }
"""

OS_X_PACKAGE_HELP = """
macOS (OS X)
-------------------------------------------------------------------------------
clang       Install XCode
cmake       brew update && brew install cmake
gobjdump    brew update && brew install binutils
"""

WINDOWS_PACKAGE_HELP = r"""
Windows                                                                 Cygwin
                                                                        package
-------------------------------------------------------------------------------
cmake       Install from https://cmake.org/
git         Install from https://git-scm.com/
nasm        Install from http://www.nasm.us/
tclsh       Install TCL from https://www.activestate.com/activetcl
vcvarsall.bat    Install Microsoft Visual Studio/VC++, e.g. the free Community
            edition from https://www.visualstudio.com/; download and run the
            installer; tick at least "Desktop development with C++"
perl        Install from https://www.activestate.com/activeperl

bash        Install Cygwin; part of the default installation            -
cmake       Install the Cygwin package "cmake"                          cmake
            OR (preferable) install CMake as above
ld          Install the Cygwin package "gcc-g++"                        gcc-g++
make        Install the Cygwin package "make"                           make
tar         Install Cygwin; part of the default installation            -

Don't forget to add the tools to your PATH, such as:
    C:\Perl64\bin
    C:\cygwin64\bin
    C:\Program Files\NASM
    C:\Program Files\Git\cmd

and for vcvarsall.bat (and via it, cl.exe, nmake.exe, etc.), something like one
of:
    C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build
... depending on your version of Visual Studio.

If you install Cygwin Perl (package "perl"), make sure the native Windows
version of Perl PRECEDES IT in the PATH; you don't want the Cygwin one to be
the default.

"""  # noqa

WINDOWS_PACKAGE_HELP_DEFUNCT = r"""
Windows (DEFUNCT)
-------------------------------------------------------------------------------
makedepend  Install the Cygwin (*) package "makedepend"
readelf     Install the Cygwin (*) package "binutils"
tclsh       Install the Cygwin package "tcl" AND ALSO:                  tcl
                C:\> bash
                $ cp /bin/tclsh8.6.exe /bin/tclsh.exe
            ... because the built-in "tclsh" (no .exe) isn't found by Windows.

"""


def require(command: str) -> None:
    """
    Checks that an external command is available, or raises an exception.
    """
    if shutil.which(command):
        return
    # Failure, so offer some help
    missing_msg = f"Missing OS command: {command}"
    helpmsg = "If core commands are missing:\n"
    if BUILD_PLATFORM.linux:
        helpmsg += UBUNTU_PACKAGE_HELP
    if BUILD_PLATFORM.macos:
        helpmsg += OS_X_PACKAGE_HELP
    if BUILD_PLATFORM.windows:
        helpmsg += WINDOWS_PACKAGE_HELP
    log.critical(missing_msg)
    log.warning("{}", helpmsg)
    raise ValueError(missing_msg)


def ensure_first_perl_is_not_cygwin() -> None:
    r"""
    For Windows: ensure that the Perl we get when we call "perl" isn't a Cygwin
    version.

    Why?

    ===============================================================================
    WORKING DIRECTORY: C:\Users\Rudolf\dev\qt_local_build\openssl_windows_x86_64_build\openssl-1.1.0g
    COMMAND: perl C:\Users\Rudolf\dev\qt_local_build\openssl_windows_x86_64_build\openssl-1.1.0g\Configure VC-WIN64A shared no-ssl3
    ===============================================================================
    File::Glob::glob() will disappear in perl 5.30. Use File::Glob::bsd_glob() instead. at C:\Users\Rudolf\dev\qt_local_build\openssl_windows_x86_64_build\openssl-1.1.0g\Configure line 270.
    Configuring OpenSSL version 1.1.0g (0x1010007fL)
        no-asan         [default]  OPENSSL_NO_ASAN
        no-crypto-mdebug [default]  OPENSSL_NO_CRYPTO_MDEBUG
        no-crypto-mdebug-backtrace [default]  OPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
        no-ec_nistp_64_gcc_128 [default]  OPENSSL_NO_EC_NISTP_64_GCC_128
        no-egd          [default]  OPENSSL_NO_EGD
        no-fuzz-afl     [default]  OPENSSL_NO_FUZZ_AFL
        no-fuzz-libfuzzer [default]  OPENSSL_NO_FUZZ_LIBFUZZER
        no-heartbeats   [default]  OPENSSL_NO_HEARTBEATS
        no-md2          [default]  OPENSSL_NO_MD2 (skip dir)
        no-msan         [default]  OPENSSL_NO_MSAN
        no-rc5          [default]  OPENSSL_NO_RC5 (skip dir)
        no-sctp         [default]  OPENSSL_NO_SCTP
        no-ssl-trace    [default]  OPENSSL_NO_SSL_TRACE
        no-ssl3         [option]   OPENSSL_NO_SSL3
        no-ssl3-method  [default]  OPENSSL_NO_SSL3_METHOD
        no-ubsan        [default]  OPENSSL_NO_UBSAN
        no-unit-test    [default]  OPENSSL_NO_UNIT_TEST
        no-weak-ssl-ciphers [default]  OPENSSL_NO_WEAK_SSL_CIPHERS
        no-zlib         [default]
        no-zlib-dynamic [default]
    Configuring for VC-WIN64A

    ------------------------------------------------------------------------------
    This perl implementation doesn't produce Windows like paths (with backward
    slash directory separators).  Please use an implementation that matches your
    building platform.

    This Perl version: 5.26.1 for x86_64-cygwin-threads-multi
    ------------------------------------------------------------------------------

    $ which perl
    /usr/bin/perl

    >>> shutil.which("perl")
    'C:\\cygwin64\\bin\\perl.EXE'

    # Then after installing ActiveState Perl:

    $ which perl
    /cygdrive/c/Perl64/bin/perl

    >>> shutil.which("perl")
    'C:\\Perl64\\bin\\perl.EXE'

    # ... and then it works.

    """  # noqa
    require(PERL)
    which = shutil.which(PERL)
    if "cygwin" in which.lower():  # imperfect check based on default Cygwin installation path  # noqa
        fail(
            f"The first instance of Perl on your path ({which}) is from "
            f"Cygwin. This will fail when building OpenSSL. Please re-order "
            f"your PATH so that a Windows version, e.g. ActiveState Perl, "
            f"comes first.")


def is_tclsh_windows_compatible(tclsh: str = TCLSH) -> bool:
    r"""
    If you use a Unix version of TCL to build SQLCipher under Windows, it will
    fail because it misinterprets paths. We need to be certain that the TCL
    shell is of the correct kind, i.e. built for Windows.

    First note that TCL needs backslashes escaped as \\ in literal strings.

    If you have a Windows file \tmp\test.tcl and run it from a DIFFERENT
    directory using "tclsh \tmp\test.tcl", you will get this output from a Unix
    tclsh (e.g. Ubuntu, Cygwin):

        puts [info patchlevel]      ;# may help to discriminate two versions! 8.6.8 for Cygwin for me today
        puts [file dirname "/some/path/filename.txt"]       ;# /some/path
        puts [file dirname "\\some\\path\\filename.txt"]    ;# .        -- DISCRIMINATIVE
        puts [file nativename "/some/path/filename.txt"]    ;# /some/path/filename.txt
        puts [file nativename "\\some\\path\\filename.txt"] ;# \some\path\filename.txt
        puts [info script]                                  ;# \tmp\test.tcl
        puts [file dirname [info script]]                   ;# .        -- DISCRIMINATIVE
        puts [file nativename [info script]]                ;# \tmp\test.tcl

    A Windows tclsh (e.g. ActiveState) will give you this:

        puts [info patchlevel]      ;# may help to discriminate two versions! 8.6.7 for ActiveTCL for me today
        puts [file dirname "/some/path/filename.txt"]       ;# /some/path
        puts [file dirname "\\some\\path\\filename.txt"]    ;# /some/path
        puts [file nativename "/some/path/filename.txt"]    ;# \some\path\filename.txt  -- DISCRIMINATIVE
        puts [file nativename "\\some\\path\\filename.txt"] ;# \some\path\filename.txt
        puts [info script]                                  ;# \tmp\test.tcl
        puts [file dirname [info script]]                   ;# /tmp     -- DISCRIMINATIVE
        puts [file nativename [info script]]                ;# \tmp\test.tcl

    Since "info script" requires an actual script to be created (not just
    stdin), the simplest discriminatory command is

        puts [file dirname "\\some\\path\\filename.txt"]

    """  # noqa
    tcl_cmd = r'puts -nonewline [file dirname "\\some\\path\\filename.txt"]'
    correct = r'/some/path'
    incorrect = '.'
    cmdargs = [tclsh]
    encoding = sys.getdefaultencoding()
    subproc_run_kwargs = {
        'stdout': subprocess.PIPE,
        'check': True,
        'encoding': encoding,
        'input': tcl_cmd,
    }
    # In Python 3.5, we deal with bytes objects and manually encode/decode.
    # In Python 3.6+, we can specify the encoding and deal with str objects.
    # Now we are always using Python 3.6+.
    completed_proc = subprocess.run(cmdargs, **subproc_run_kwargs)
    result = completed_proc.stdout  # type: str
    if result == correct:
        return True
    elif result == incorrect:
        log.warning(
            f"The TCL shell, {tclsh!r}, is a UNIX version (e.g. Cygwin) "
            f"incompatible with Windows backslash-delimited filenames; switch "
            f"to a Windows version (e.g. ActiveState ActiveTCL).")
        return False
    else:
        raise RuntimeError(
            f"Don't understand output from TCL shell {tclsh!r} with input "
            f"{tcl_cmd!r}; output was {result!r}")


# =============================================================================
# Ancillary: information messages
# =============================================================================

def report_all_targets_exist(package: str, targets: List[str]) -> None:
    log.info(
        "{}: All targets exist already:\n{}".format(
            package,
            "\n".join("    " + str(x) for x in targets)
        )
    )


# =============================================================================
# Building OpenSSL
# =============================================================================

def fetch_openssl(cfg: Config) -> None:
    """
    Downloads OpenSSL source code.
    """
    log.info("Fetching OpenSSL source...")
    download_if_not_exists(cfg.openssl_src_url, cfg.openssl_src_fullpath)
    # download_if_not_exists(cfg.openssl_android_script_url,
    #                        cfg.openssl_android_script_fullpath)


def openssl_target_os_args(target_platform: Platform) -> List[str]:
    """
    Returns the target OS for OpenSSL's "Configure" Perl script, +/- any other
    required target-specific parameters, as a list.

-------------------------------------------------------------------------------
OpenSSL 1.0.2h targets:
-------------------------------------------------------------------------------

BC-32 BS2000-OSD BSD-generic32 BSD-generic64 BSD-ia64 BSD-sparc64 BSD-sparcv8
BSD-x86 BSD-x86-elf BSD-x86_64 Cygwin Cygwin-x86_64 DJGPP MPE/iX-gcc OS2-EMX
OS390-Unix QNX6 QNX6-i386 ReliantUNIX SINIX SINIX-N UWIN VC-CE VC-WIN32
VC-WIN64A VC-WIN64I aix-cc aix-gcc aix3-cc aix64-cc aix64-gcc android
android-armv7 android-mips android-x86 aux3-gcc beos-x86-bone beos-x86-r5
bsdi-elf-gcc cc cray-j90 cray-t3e darwin-i386-cc darwin-ppc-cc darwin64-ppc-cc
darwin64-x86_64-cc dgux-R3-gcc dgux-R4-gcc dgux-R4-x86-gcc dist gcc hpux-cc
hpux-gcc hpux-ia64-cc hpux-ia64-gcc hpux-parisc-cc hpux-parisc-cc-o4
hpux-parisc-gcc hpux-parisc1_1-cc hpux-parisc1_1-gcc hpux-parisc2-cc
hpux-parisc2-gcc hpux64-ia64-cc hpux64-ia64-gcc hpux64-parisc2-cc
hpux64-parisc2-gcc hurd-x86 iphoneos-cross irix-cc irix-gcc irix-mips3-cc
irix-mips3-gcc irix64-mips4-cc irix64-mips4-gcc linux-aarch64
linux-alpha+bwx-ccc linux-alpha+bwx-gcc linux-alpha-ccc linux-alpha-gcc
linux-aout linux-armv4 linux-elf linux-generic32 linux-generic64
linux-ia32-icc linux-ia64 linux-ia64-icc linux-mips32 linux-mips64 linux-ppc
linux-ppc64 linux-ppc64le linux-sparcv8 linux-sparcv9 linux-x32 linux-x86_64
linux-x86_64-clang linux-x86_64-icc linux32-s390x linux64-mips64 linux64-s390x
linux64-sparcv9 mingw mingw64 ncr-scde netware-clib netware-clib-bsdsock
netware-clib-bsdsock-gcc netware-clib-gcc netware-libc netware-libc-bsdsock
netware-libc-bsdsock-gcc netware-libc-gcc newsos4-gcc nextstep nextstep3.3
osf1-alpha-cc osf1-alpha-gcc purify qnx4 rhapsody-ppc-cc sco5-cc sco5-gcc
solaris-sparcv7-cc solaris-sparcv7-gcc solaris-sparcv8-cc solaris-sparcv8-gcc
solaris-sparcv9-cc solaris-sparcv9-gcc solaris-x86-cc solaris-x86-gcc
solaris64-sparcv9-cc solaris64-sparcv9-gcc solaris64-x86_64-cc
solaris64-x86_64-gcc sunos-gcc tandem-c89 tru64-alpha-cc uClinux-dist
uClinux-dist64 ultrix-cc ultrix-gcc unixware-2.0 unixware-2.1 unixware-7
unixware-7-gcc vos-gcc vxworks-mips vxworks-ppc405 vxworks-ppc60x
vxworks-ppc750 vxworks-ppc750-debug vxworks-ppc860 vxworks-ppcgen
vxworks-simlinux debug debug-BSD-x86-elf debug-VC-WIN32 debug-VC-WIN64A
debug-VC-WIN64I debug-ben debug-ben-darwin64 debug-ben-debug
debug-ben-debug-64 debug-ben-debug-64-clang debug-ben-macos
debug-ben-macos-gcc46 debug-ben-no-opt debug-ben-openbsd
debug-ben-openbsd-debug debug-ben-strict debug-bodo debug-darwin-i386-cc
debug-darwin-ppc-cc debug-darwin64-x86_64-cc debug-geoff32 debug-geoff64
debug-levitte-linux-elf debug-levitte-linux-elf-extreme
debug-levitte-linux-noasm debug-levitte-linux-noasm-extreme debug-linux-elf
debug-linux-elf-noefence debug-linux-generic32 debug-linux-generic64
debug-linux-ia32-aes debug-linux-pentium debug-linux-ppro debug-linux-x86_64
debug-linux-x86_64-clang debug-rse debug-solaris-sparcv8-cc
debug-solaris-sparcv8-gcc debug-solaris-sparcv9-cc debug-solaris-sparcv9-gcc
debug-steve-opt debug-steve32 debug-steve64 debug-vos-gcc

-------------------------------------------------------------------------------
OpenSSL 1.1.0g targets:
-------------------------------------------------------------------------------

Usage: Configure [no-<cipher> ...] [enable-<cipher> ...] [-Dxxx] [-lxxx]
    [-Lxxx] [-fxxx] [-Kxxx] [no-hw-xxx|no-hw] [[no-]threads] [[no-]shared]
    [[no-]zlib|zlib-dynamic] [no-asm] [no-dso] [no-egd] [sctp] [386]
    [--prefix=DIR] [--openssldir=OPENSSLDIR] [--with-xxx[=vvv]] [--config=FILE]
    os/compiler[:flags]

pick os/compiler from:
BS2000-OSD BSD-generic32 BSD-generic64 BSD-ia64 BSD-sparc64 BSD-sparcv8
BSD-x86 BSD-x86-elf BSD-x86_64 Cygwin Cygwin-i386 Cygwin-i486 Cygwin-i586
Cygwin-i686 Cygwin-x86 Cygwin-x86_64 DJGPP MPE/iX-gcc OS390-Unix QNX6
QNX6-i386 UEFI UWIN VC-CE VC-WIN32 VC-WIN64A VC-WIN64A-masm VC-WIN64I aix-cc
aix-gcc aix64-cc aix64-gcc android android-armeabi android-mips android-x86
android64 android64-aarch64 bsdi-elf-gcc cc darwin-i386-cc darwin-ppc-cc
darwin64-debug-test-64-clang darwin64-ppc-cc darwin64-x86_64-cc dist gcc
haiku-x86 haiku-x86_64 hpux-ia64-cc hpux-ia64-gcc hpux-parisc-cc
hpux-parisc-gcc hpux-parisc1_1-cc hpux-parisc1_1-gcc hpux64-ia64-cc
hpux64-ia64-gcc hpux64-parisc2-cc hpux64-parisc2-gcc hurd-x86 ios-cross
ios64-cross iphoneos-cross irix-mips3-cc irix-mips3-gcc irix64-mips4-cc
irix64-mips4-gcc linux-aarch64 linux-alpha-gcc linux-aout linux-arm64ilp32
linux-armv4 linux-c64xplus linux-elf linux-generic32 linux-generic64
linux-ia64 linux-mips32 linux-mips64 linux-ppc linux-ppc64 linux-ppc64le
linux-sparcv8 linux-sparcv9 linux-x32 linux-x86 linux-x86-clang linux-x86_64
linux-x86_64-clang linux32-s390x linux64-mips64 linux64-s390x linux64-sparcv9
mingw mingw64 nextstep nextstep3.3 purify qnx4 sco5-cc sco5-gcc
solaris-sparcv7-cc solaris-sparcv7-gcc solaris-sparcv8-cc solaris-sparcv8-gcc
solaris-sparcv9-cc solaris-sparcv9-gcc solaris-x86-gcc solaris64-sparcv9-cc
solaris64-sparcv9-gcc solaris64-x86_64-cc solaris64-x86_64-gcc tru64-alpha-cc
tru64-alpha-gcc uClinux-dist uClinux-dist64 unixware-2.0 unixware-2.1
unixware-7 unixware-7-gcc vms-alpha vms-alpha-p32 vms-alpha-p64 vms-ia64
vms-ia64-p32 vms-ia64-p64 vos-gcc vxworks-mips vxworks-ppc405 vxworks-ppc60x
vxworks-ppc750 vxworks-ppc750-debug vxworks-ppc860 vxworks-ppcgen
vxworks-simlinux debug debug-erbridge debug-linux-ia32-aes debug-linux-pentium
debug-linux-ppro debug-test-64-clang

-------------------------------------------------------------------------------
OpenSSL 1.1.1c targets (reformatted for clarity):
-------------------------------------------------------------------------------

Usage: Configure [no-<cipher> ...] [enable-<cipher> ...] [-Dxxx] [-lxxx]
    [-Lxxx] [-fxxx] [-Kxxx] [no-hw-xxx|no-hw] [[no-]threads] [[no-]shared]
    [[no-]zlib|zlib-dynamic] [no-asm] [no-egd] [sctp] [386] [--prefix=DIR]
    [--openssldir=OPENSSLDIR] [--with-xxx[=vvv]] [--config=FILE]
    os/compiler[:flags]

pick os/compiler from:
BS2000-OSD BSD-generic32 BSD-generic64 BSD-ia64 BSD-sparc64 BSD-sparcv8
BSD-x86 BSD-x86-elf BSD-x86_64 Cygwin Cygwin-i386 Cygwin-i486 Cygwin-i586
Cygwin-i686 Cygwin-x86 Cygwin-x86_64 DJGPP MPE/iX-gcc UEFI UWIN

    VC-CE VC-WIN32 VC-WIN32-ARM VC-WIN32-ONECORE VC-WIN64-ARM VC-WIN64A
    VC-WIN64A-ONECORE VC-WIN64A-masm VC-WIN64I

aix-cc aix-gcc aix64-cc aix64-gcc

    android-arm android-arm64 android-armeabi android-mips android-mips64
    android-x86 android-x86_64 android64 android64-aarch64 android64-mips64
    android64-x86_64

bsdi-elf-gcc cc

    darwin-i386-cc darwin-ppc-cc darwin64-ppc-cc darwin64-x86_64-cc

gcc haiku-x86 haiku-x86_64 hpux-ia64-cc hpux-ia64-gcc
hpux-parisc-cc hpux-parisc-gcc hpux-parisc1_1-cc hpux-parisc1_1-gcc
hpux64-ia64-cc hpux64-ia64-gcc hpux64-parisc2-cc hpux64-parisc2-gcc hurd-x86

    ios-cross ios-xcrun ios64-cross ios64-xcrun iossimulator-xcrun
    iphoneos-cross

irix-mips3-cc irix-mips3-gcc irix64-mips4-cc irix64-mips4-gcc

linux-aarch64 linux-alpha-gcc linux-aout linux-arm64ilp32 linux-armv4
linux-c64xplus linux-elf linux-generic32 linux-generic64 linux-ia64
linux-mips32 linux-mips64 linux-ppc linux-ppc64 linux-ppc64le linux-sparcv8
linux-sparcv9 linux-x32 linux-x86 linux-x86-clang linux-x86_64
linux-x86_64-clang linux32-s390x linux64-mips64 linux64-s390x linux64-sparcv9

mingw mingw64 nextstep
nextstep3.3 sco5-cc sco5-gcc solaris-sparcv7-cc solaris-sparcv7-gcc
solaris-sparcv8-cc solaris-sparcv8-gcc solaris-sparcv9-cc solaris-sparcv9-gcc
solaris-x86-gcc solaris64-sparcv9-cc solaris64-sparcv9-gcc solaris64-x86_64-cc
solaris64-x86_64-gcc tru64-alpha-cc tru64-alpha-gcc uClinux-dist
uClinux-dist64 unixware-2.0 unixware-2.1 unixware-7 unixware-7-gcc vms-alpha
vms-alpha-p32 vms-alpha-p64 vms-ia64 vms-ia64-p32 vms-ia64-p64 vos-gcc
vxworks-mips vxworks-ppc405 vxworks-ppc60x vxworks-ppc750 vxworks-ppc750-debug
vxworks-ppc860 vxworks-ppcgen vxworks-simlinux

NOTE: If in doubt, on Unix-ish systems use './config'.

    """

    # http://doc.qt.io/qt-5/opensslsupport.html

    # Revised 2019-06-16 for OpenSSL 1.1.1c:
    if target_platform.android:
        if target_platform.cpu_arm_32bit:
            return ["android-arm"]
        elif target_platform.cpu_arm_64bit:
            return ["android-arm64"]
        elif target_platform.cpu_x86_32bit_family:
            return ["android-x86"]
        elif target_platform.cpu_x86_64bit_family:
            return ["android-x86_64"]
        # if we get here: will raise error below

    elif target_platform.linux:
        if target_platform.cpu_x86_32bit_family:
            return ["linux-x86"]
        elif target_platform.cpu_x86_64bit_family:
            return ["linux-x86_64"]

    elif target_platform.macos:
        if target_platform.cpu_x86_32bit_family:
            return ["darwin-i386-cc"]
        elif target_platform.cpu_x86_64bit_family:
            # https://gist.github.com/tmiz/1441111
            return ["darwin64-x86_64-cc"]

    elif target_platform.ios:
        # https://gist.github.com/foozmeat/5154962
        # https://gist.github.com/felix-schwarz/c61c0f7d9ab60f53ebb0
        # https://gist.github.com/armadsen/b30f352a8d6f6c87a146 <<< ESP. THIS
        # If Bitcode is later required, see the other ones above and
        # https://stackoverflow.com/questions/30722606/what-does-enable-bitcode-do-in-xcode-7  # noqa
        if target_platform.cpu_arm_32bit:  # iOS on 32-bit devices
            return ["ios-cross"]
        elif target_platform.cpu_arm_64bit:  # iOS on 64-bit devices
            return ["ios64-cross"]
        elif target_platform.cpu_x86_64bit_family:  # iOS on 64-bit simulator
            return ["darwin64-x86_64-cc", "no-asm"]  # unsure if "no-asm" required  # noqa
        elif target_platform.cpu_x86_32bit_family:  # iOS on 32-bit simulator
            return ["darwin-i386-cc"]

    elif target_platform.windows:
        if BUILD_PLATFORM.windows:
            # http://p-nand-q.com/programming/windows/building_openssl_with_visual_studio_2013.html  # noqa
            if target_platform.cpu_x86_64bit_family:
                return ["VC-WIN64A", "-FS"]
                # I'm not sure what "VC-WIN64I" is. Intel vs AMD? Ah, no:
                # https://stackoverflow.com/questions/38151387/build-openssl-for-both-x64-and-x86-side-by-side-installation  # noqa
                # ... "WIN64I denotes IA-64 and WIN64A - AMD64"
                # ... where IA-64 means Intel Itanium: https://en.wikipedia.org/wiki/IA-64  # noqa
                # ... so we want "-A" for x86-64.
                #
                # "/FS" to allow parallel compilation;
                # ... what's after "+" or "-" becomes part of CFLAGS; see
                # https://wiki.openssl.org/index.php/Compilation_and_Installation#Configure_Options  # noqa
                # ... but note that the "+" or "-" are themselves passed;
                #     so we rely on the fact that cl.exe will interpret "-FS"
                #     and "/FS" identically.
            elif target_platform.cpu_x86_32bit_family:
                return ["VC-WIN32"]

    raise NotImplementedError(
        f"Don't known OpenSSL target name for {target_platform}")

    # For new platforms: if you're not sure, use target_os = "crashme" and
    # you'll get the list of permitted values, which as of 2017-11-12 is:


def build_openssl(cfg: Config, target_platform: Platform) -> None:
    """
    Builds OpenSSL.

    The target_os parameter is paseed to OpenSSL's Configure script.
    Use "./Configure LIST" for all possibilities.

        https://wiki.openssl.org/index.php/Compilation_and_Installation
    """
    log.info("Building OpenSSL for {}...", target_platform)

    # -------------------------------------------------------------------------
    # OpenSSL: Prerequisites
    # -------------------------------------------------------------------------
    if BUILD_PLATFORM.windows:
        ensure_first_perl_is_not_cygwin()
        require(NASM)

    # -------------------------------------------------------------------------
    # OpenSSL: Set up filenames we expect to be generated
    # -------------------------------------------------------------------------
    rootdir, workdir = cfg.get_openssl_rootdir_workdir(target_platform)
    dynamic_lib_ext = target_platform.dynamic_lib_ext
    static_lib_ext = target_platform.static_lib_ext
    openssl_verparts = cfg.openssl_version.split(".")

    if BUILD_PLATFORM.windows:
        openssl_major = f"-{openssl_verparts[0]}_{openssl_verparts[1]}"
        if target_platform.cpu_x86_64bit_family:
            fname_arch = "-x64"
        else:
            fname_arch = ""
        fname_extra = openssl_major + fname_arch  # e.g. "-1_1-x64"
    else:
        fname_extra = ""

    # Only build what is required because Qt can end up linking with the
    # dynamic libraries instead of the static ones, even if you told it
    # not to.
    if target_platform.qt_linkage_static:
        main_targets = [
            join(workdir, f"libssl{static_lib_ext}"),
            join(workdir, f"libcrypto{static_lib_ext}"),
        ]
    else:
        main_targets = [
            join(workdir, f"libssl{fname_extra}{dynamic_lib_ext}"),
            join(workdir, f"libcrypto{fname_extra}{dynamic_lib_ext}"),
        ]

    # Now, also: Linux likes to use "-lcrypto" and have that mean "look at
    # libcrypto.so", whereas under Windows we seem to have to use
    # "-llibcrypto" instead. However, some things, like SQLCipher,
    # hard-code the "-lcrypto" (in that example, in its test suite as it
    # compiles conftest.c). So we're best off using the Linux notation but
    # making additional copies of the libraries:
    shadow_targets = []  # type: List[str]
    libprefix = "lib"
    if BUILD_PLATFORM.windows:
        for t in main_targets:
            dirname, basename = os.path.split(t)
            assert basename.startswith(libprefix)
            shortbasename = basename[len(libprefix):]
            shadow_targets.append(join(dirname, shortbasename))

    targets = main_targets + shadow_targets
    if not cfg.force and all(isfile(x) for x in targets):
        report_all_targets_exist("OpenSSL", targets)
        return

    # -------------------------------------------------------------------------
    # OpenSSL: Unpack source
    # -------------------------------------------------------------------------
    untar_to_directory(cfg.openssl_src_fullpath, rootdir, run_func=run,
                       chdir_via_python=True)

    # -------------------------------------------------------------------------
    # OpenSSL: Environment 1/2
    # -------------------------------------------------------------------------
    env = cfg.get_starting_env()
    cfg.set_compile_env(env, target_platform)
    sysroot = cfg.sysroot(target_platform, env)

    # https://github.com/openssl/openssl/issues/1681
    # or: "error: invalid 'asm': invalid operand for code 'w'"
    if not target_platform.android:
        # CROSS_SYSROOT is deprecated for Android in OpenSSL 1.1.1c; see
        # NOTES.ANDROID.
        env["CROSS_SYSROOT"] = sysroot

    if target_platform.android:
        env["ANDROID_NDK_HOME"] = cfg.android_ndk_root

    # -------------------------------------------------------------------------
    # OpenSSL: Special mucking around
    # -------------------------------------------------------------------------

    if target_platform.ios:
        # iOS specials
        # e.g. https://gist.github.com/armadsen/b30f352a8d6f6c87a146
        run([
            SED,
            "-ie",
            "s!static volatile sig_atomic_t intr_signal;!static volatile intr_signal;!",  # noqa
            join(workdir, "crypto", "ui", "ui_openssl.c")
        ])

    # At some point (transiently!) we also got something like:
    #    ar  r ../../libcrypto.a o_names.o obj_dat.o obj_lib.o
    #        obj_err.o obj_xref.
    # failing with:
    #     /usr/bin/ranlib: archive member: libcrypto.a(....o) size too
    #         large (archive member extends past the end of the file)
    #     ar: internal ranlib command failed
    # ... not sure why.

    # -------------------------------------------------------------------------
    # OpenSSL: Configure options
    # -------------------------------------------------------------------------
    # The OpenSSL "config" sh script guesses the OS, then passes details
    # to its "Configure" Perl script.
    # For Android, OpenSSL suggest using their Setenv-android.sh script, then
    # running "config".
    # However, it does seem to be screwing up. Let's try Configure instead.
    # As of OpenSSL 1.1.1c, that's what they advise (see NOTES.ANDROID).

    # If we don't do this, the binaries end up in a non-writeable folder
    if target_platform.ios:
        sysroot = workdir

    configure_args = openssl_target_os_args(target_platform)
    target_os = configure_args[0]  # may be used below
    configure_args += [
        "--prefix=" + sysroot,
        # "--cross-compile-prefix={}".format(
        #     target_platform.cross_compile_prefix),
    ] + OPENSSL_COMMON_OPTIONS
    if target_platform.mobile:
        configure_args += [
            "no-hw",  # disable hardware support ("useful on mobile devices")
            "no-engine",  # disable hardware support ("useful on mobile devices")  # noqa
        ]
    # OpenSSL's Configure script applies optimizations by default.
    if target_platform.android:
        configure_args += [
            f"-D__ANDROID_API__={cfg.android_sdk_version}"
        ]
    if target_platform.ios:
        configure_args += [
            "no-makedepend",
        ]

    if target_platform.qt_linkage_static:
        configure_args += [
            "no-shared",
        ]

    # -------------------------------------------------------------------------
    # OpenSSL: Environment 2/2
    # -------------------------------------------------------------------------
    if target_platform.android:
        # https://wiki.openssl.org/index.php/Android
        # We're not using the Setenv-android.sh script, but replicating its
        # functions; cfg.set_compile_env() does much of that.
        # Also:
        env["FIPS_SIG"] = ""  # OK to leave blank if not building FIPS
        env["MACHINE"] = "i686"
        env["RELEASE"] = "2.6.37"  # ??
        env["SYSTEM"] = target_os  # e.g. "android", "android-armv7"

    # -------------------------------------------------------------------------
    # OpenSSL: Makefile tweaking prior to running Configure
    # -------------------------------------------------------------------------
    # https://wiki.openssl.org/index.php/Android
    # ... none at present

    with pushd(workdir):
        # ---------------------------------------------------------------------
        # OpenSSL: Configure (or config, though we're avoiding that)
        # ---------------------------------------------------------------------
        use_configure = True  # Better!
        if use_configure or not target_platform.android:
            # http://doc.qt.io/qt-5/opensslsupport.html
            if BUILD_PLATFORM.windows:
                log.warning(
                    "The OpenSSL Configure script may warn about nmake.exe "
                    "being missing when it isn't. (Or when it is...)")
            run([PERL, join(workdir, "Configure")] + configure_args, env)
        else:
            # The "config" script guesses the OS then runs "Configure".
            # https://wiki.openssl.org/index.php/Android
            # and "If in doubt, on Unix-ish systems use './config'."
            # https://wiki.openssl.org/index.php/Compilation_and_Installation
            run([join(workdir, "config")] + configure_args, env)

        # ---------------------------------------------------------------------
        # OpenSSL: Make
        # ---------------------------------------------------------------------
        makefile = join(workdir, "Makefile")  # written to by Configure

        if target_platform.ios:
            # https://gist.github.com/armadsen/b30f352a8d6f6c87a146
            # add -isysroot to CC=
            run([
                SED,
                "-i",  # in place
                "-e",  # execute the script expression that follows
                (
                    f"s"  # s/regexp/replacement/
                    f"!"  # use '!' not '/' to delineate regex
                    f"^CFLAGS="
                    f"!"
                    f"CFLAGS=-isysroot {env['CROSS_TOP']}/SDKs/{env['CROSS_SDK']} "  # noqa
                    f"-mios-version-min={cfg.ios_min_version} "
                    f"-miphoneos-version-min={cfg.ios_min_version} "
                    f"!"
                ),
                makefile
            ])
            # 2018-08-24: was CFLAG=, now CFLAGS=
            # 2018-08-24: makefile was modified before Configure called; daft

        extra_args = []  # type: List[str]

        # A particular problem is that Android .so libraries must be
        # UNVERSIONED, i.e. named "libcrypto.so" and "libssl.so", not e.g.
        # "libcrypto.so.1.1" and "libssl.so.1.1". All references to the
        # versions must be removed.
        #
        # In some OpenSSL versions, this can be achieved by setting the
        # variable CALC_VERSIONS="SHLIB_COMPAT=; SHLIB_SOVER=", as an argument
        # to make [1, 2], or as an environment variable [3].
        #
        # However, while that was true of OpenSSL 1.0.2d, it isn't true of
        # OpenSSL 1.1.0g, in which the CALC_VERSIONS variable has vanished from
        # Makefile.shared [4].
        #
        # The 1.1.0g Makefile.shared has things like SHLIBVERSION.
        #
        # [1] http://doc.qt.io/qt-5/opensslsupport.html, 2018-07-24
        # [2] https://stackoverflow.com/questions/24204366/how-to-build-openssl-as-unversioned-shared-lib-for-android  # noqa
        # [3] https://stackoverflow.com/questions/2826029/passing-additional-variables-from-command-line-to-make  # noqa
        # [4] https://ftp.openssl.org/source/old/

        make_unversioned_libraries = target_platform.android

        if make_unversioned_libraries:
            # Work this out from the generated Makefile.
            # Look for "all" as the main target.

            # This doesn't work:
            # - Try to avoid "--environment-overrides".
            # - https://github.com/openssl/openssl/issues/3902
            # extra_args.append("SHLIB_VERSION_NUMBER=")
            # extra_args.append("SHLIB_EXT=.so")

            # Homebrew version, 2018-07-14, which works:
            replace_multiple_in_file(makefile, [
                ('SHLIBS=libcrypto.so.$(SHLIB_MAJOR).$(SHLIB_MINOR) libssl.so.$(SHLIB_MAJOR).$(SHLIB_MINOR)',  # noqa
                 'SHLIBS=libcrypto.so libssl.so'),
                ('SHLIB_INFO="libcrypto.so.$(SHLIB_MAJOR).$(SHLIB_MINOR);libcrypto.so" "libssl.so.$(SHLIB_MAJOR).$(SHLIB_MINOR);libssl.so"',  # noqa
                 'SHLIB_INFO="libcrypto.so" "libssl.so"'),
                # ... also deals with INSTALL_SHLIBS, INSTALL_SHLIB_INFO
                #     which are identical
                ('SHLIBNAME_FULL=libcrypto.so.$(SHLIB_MAJOR).$(SHLIB_MINOR)',  # noqa
                 'SHLIBNAME_FULL=libcrypto.so'),
                ('SHLIBNAME_FULL=libssl.so.$(SHLIB_MAJOR).$(SHLIB_MINOR)',
                 'SHLIBNAME_FULL=libssl.so'),
            ])

        def runmake(command: str = "") -> None:
            run(cfg.make_args(command=command, env=env,
                              extra_args=extra_args), env)

        # See INSTALL, INSTALL.WIN, etc. from the OpenSSL distribution
        runmake()

        # ---------------------------------------------------------------------
        # OpenSSL: Test
        # ---------------------------------------------------------------------
        test_openssl = (
            (not OPENSSL_FAILS_OWN_TESTS) and
            target_platform.os == BUILD_PLATFORM.os
            # can't really test e.g. Android code directly under Linux
        )
        if test_openssl:
            runmake("test")

    # -------------------------------------------------------------------------
    # OpenSSL: check libraries and/or copy libraries to their standard names.
    # -------------------------------------------------------------------------
    for i, t in enumerate(main_targets):
        target_platform.verify_lib(t)
        if BUILD_PLATFORM.windows:
            assert len(shadow_targets) == len(main_targets)
            shutil.copyfile(t, shadow_targets[i])


# =============================================================================
# Building Qt
# =============================================================================

def fetch_qt(cfg: Config) -> None:
    """
    Downloads Qt source code.
    """
    log.info("Fetching Qt source...")
    if not git_clone(prettyname="Qt",
                     url=cfg.qt_git_url,
                     branch=cfg.qt_git_branch,
                     commit=cfg.qt_git_commit,
                     directory=cfg.qt_src_gitdir,
                     run_func=run):
        return
    chdir(cfg.qt_src_gitdir)
    run([PERL, "init-repository"])
    # Now, as per https://wiki.qt.io/Android:
    if QT_SPECIFIC_VERSION:
        run([GIT, "checkout", QT_SPECIFIC_VERSION])
        run([GIT, "submodule", "update", "--recursive"])


def build_qt(cfg: Config, target_platform: Platform) -> str:
    """
    1. Builds Qt.
    2. Returns the name of the "install" directory, where the installed qmake
       is.
    """
    # http://doc.qt.io/qt-5/opensslsupport.html
    # Android:
    #       example at http://wiki.qt.io/Qt5ForAndroidBuilding
    # Windows:
    #       https://stackoverflow.com/questions/14932315/how-to-compile-qt-5-under-windows-or-linux-32-or-64-bit-static-or-dynamic-on-v  # noqa
    #       ?also http://simpleit.us/2010/05/30/enabling-openssl-for-qt-c-on-windows/  # noqa
    #       http://doc.qt.io/qt-5/windows-building.html
    #       http://wiki.qt.io/Jom
    #       http://www.holoborodko.com/pavel/2011/02/01/how-to-compile-qt-4-7-with-visual-studio-2010/
    # iOS:
    #       http://doc.qt.io/qt-5/building-from-source-ios.html
    #       http://doc.qt.io/qt-5/ios-support.html
    # macOS:
    #       http://doc.qt.io/qt-5/osx.html

    log.info("Building Qt for {}...", target_platform)

    # -------------------------------------------------------------------------
    # Qt: Setup
    # -------------------------------------------------------------------------

    # Means by which Qt links to OpenSSL?
    qt_openssl_linkage_static = cfg.qt_openssl_static and target_platform.qt_linkage_static
    # If Qt is linked dynamically, we do not let it link to OpenSSL
    # statically (it won't work).

    if target_platform.android:
        require(JAVAC)
        # ... will be called by the make process; better to know now, since the
        # relevant messages are easily lost in the torrent
        require(ANT)

        # clang will have a funny name (with a cross-compile prefix) in this
        # situation, so checking for "clang" isn't enough. Will Qt get this
        # right automatically? Yes.
        # if USE_CLANG_NOT_GCC_FOR_ANDROID_ARM:
        #     require(CLANG)

    opensslrootdir, opensslworkdir = cfg.get_openssl_rootdir_workdir(target_platform)  # noqa
    openssl_include_root = join(opensslworkdir, "include")
    openssl_lib_root = opensslworkdir

    builddir = cfg.qt_build_dir(target_platform)
    installdir = cfg.qt_install_dir(target_platform)

    targets = [join(installdir, "bin", target_platform.qmake_executable)]
    if not cfg.force and all(isfile(x) for x in targets):
        report_all_targets_exist("Qt", targets)
        return installdir

    # -------------------------------------------------------------------------
    # Qt: clean from old configure
    # -------------------------------------------------------------------------
    # No need to clean anything in the source directory, as long as you don't
    # build there.
    # https://stackoverflow.com/questions/24261974 (comments)

    # rmtree(builddir)
    # rmtree(installdir)
    # ... do this if something goes wrong, but it is slow; maybe not routinely
    # (unless you're developing this script)?

    # -------------------------------------------------------------------------
    # Qt: Environment
    # -------------------------------------------------------------------------
    env = cfg.get_starting_env()
    crypto_dependencies = ""
    if target_platform.linux:
        crypto_dependencies = "-ldl -lpthread"
    elif target_platform.windows:
        # Copying openssl/Configurations/10-main.conf
        crypto_dependencies = "-lws2_32 -lgdi32 -ladvapi32 -lcrypt32 -luser32"

    openssl_libs = f"-L{openssl_lib_root} -lssl -lcrypto {crypto_dependencies}"

    # See also https://bugreports.qt.io/browse/QTBUG-62016
    env['OPENSSL_LIBS'] = openssl_libs
    # Setting OPENSSL_LIBS as an *environment variable* may be unnecessary,
    # but is suggested by Qt; http://doc.qt.io/qt-4.8/ssl.html
    # However, it seems necessary to set it as an *option* to configure; see
    # below.
    cfg.set_compile_env(env, target_platform)

    # -------------------------------------------------------------------------
    # Qt: Directories
    # -------------------------------------------------------------------------
    log.info("Configuring {} build in {}",
             target_platform.description, builddir)
    mkdir_p(builddir)
    mkdir_p(installdir)

    # -------------------------------------------------------------------------
    # Qt: Work out options to configure
    # -------------------------------------------------------------------------
    # -xplatform options are in src/qt5/qtbase/mkspecs/
    if BUILD_PLATFORM.windows:
        configure_prog_name = "configure.bat"
    else:
        configure_prog_name = "configure"
    # sysroot = cfg.sysroot(target_platform, env)
    includedirs = [
        openssl_include_root,  # #include files for OpenSSL
    ]
    objdirs = []  # type: List[str]
    libdirs = [
        openssl_lib_root,  # libraries for OpenSSL
    ]
    qt_config_args = [
        join(cfg.qt_src_gitdir, configure_prog_name),
        # General options:
        "-prefix", installdir,  # where to install Qt
        "-recheck-all",  # don't cache from previous configure runs
        "OPENSSL_LIBS=" + openssl_libs,
        # "-sysroot": not required; Qt's configure should handle this
        # "-gcc-sysroot": not required
    ]
    if target_platform.qt_linkage_static:
        qt_config_args.append("-static")
        # makes a static Qt library (cf. default of "-shared")
        # ... NB ALSO NEEDS "CONFIG += static" in the .pro file

    # In Qt 5.10 (as of 2017-11-21), "configure --list-features" does not
    # show "printing-and-pdf", unlike e.g. https://blog.basyskom.com/2017/qt-lite,  # noqa
    # but something in "configure" knows about it because it crashes when
    # creating qmake with "Project ERROR: Unknown feature object
    # printing-and-pdf in expression 'config.unix && features.printing-and-pdf'."  # noqa
    #
    # no, doesn't work: # qt_config_args += ["-no-feature-printing-and-pdf"]

    extra_qmake_cxxflags = []  # type: List[str]
    extra_qmake_lflags = []  # type: List[str]
    if cfg.verbose >= 2:
        extra_qmake_cxxflags += ["-v"]  # make clang++ verbose
        extra_qmake_cxxflags += ["-Xlinker --verbose=2"]  # make linker verbose
        # extra_qmake_lflags += ["--verbose=2"]  # make ld verbose

    if target_platform.android:
        # We use a dynamic build of Qt (bundled into the APK), not a static
        # version; see android_compilation.txt
        if target_platform.cpu == Cpu.X86_32:
            android_arch_short = "x86"
        elif target_platform.cpu == Cpu.ARM_V7_32:
            android_arch_short = "armeabi-v7a"
        elif target_platform.cpu == Cpu.ARM_V8_64:
            # https://developer.android.com/ndk/guides/abis.html
            android_arch_short = "arm64-v8a"
        else:
            raise NotImplementedError(
                f"Don't know how to use CPU {target_platform.cpu!r} "
                f"for Android")
        qt_config_args += [
            "-android-sdk", cfg.android_sdk_root,
            "-android-ndk", cfg.android_ndk_root,
            "-android-ndk-platform", cfg.android_ndk_platform,  # https://wiki.qt.io/Android  # noqa
            "-android-ndk-host", cfg.android_ndk_host,
            "-android-arch", android_arch_short,
            "-android-toolchain-version", cfg.android_toolchain_version,
            "--disable-rpath",  # 2019-06-16; https://wiki.qt.io/Android
            # MAY POSSIBLY NEED:
            # (https://wiki.qt.io/Qt5_platform_configurations,
            # https://wiki.qt.io/Android)
            # "-skip", "qttools",
            # "-skip", "qttranslations",
            # "-skip", "qtwebkit",
            # "-skip", "qtwebkit-examples",
            # we always skip qtserialport (see QT_CONFIG_COMMON_ARGS)
        ]
        if cfg.android_clang_not_gcc:
            qt_config_args += ["-xplatform", "android-clang"]
            # log.critical(sysroot)
            # libdir1 = join(sysroot, "usr", "lib", target_platform.target_triplet)  # noqa
            # libdir2 = join(libdir1, str(cfg.android_sdk_version))
            # libdirs.extend([libdir1, libdir2])
            # objdirs.append(libdir2)
        else:
            qt_config_args += ["-xplatform", "android-g++"]

    elif target_platform.linux:
        if QT_XCB_SUPPORT_OK:
            qt_config_args.append("-qt-xcb")  # use XCB source bundled with Qt
        else:
            qt_config_args.append("-system-xcb")  # use system XCB libraries
            # http://doc.qt.io/qt-5/linux-requirements.html
        qt_config_args += ["-gstreamer", "1.0"]  # gstreamer version; see troubleshooting below  # noqa

    elif target_platform.macos:
        if BUILD_PLATFORM.macos:
            qt_config_args += []  # not cross-compiling
        else:
            raise NotImplementedError(
                f"Don't know how to compile Qt for MacOS on "
                f"{target_platform}")

    elif target_platform.ios:
        # http://doc.qt.io/qt-5/building-from-source-ios.html
        # "A default build builds both the simulator and device libraries."
        qt_config_args += ["-xplatform", "macx-ios-clang"]

    elif target_platform.windows:
        if BUILD_PLATFORM.windows:
            qt_config_args += []  # not cross-compiling
        else:
            raise NotImplementedError(
                f"Don't know how to compile Qt for Windows on "
                f"{target_platform}")

    else:
        raise NotImplementedError("Don't know how to compile Qt for " +
                                  str(target_platform))

    for objdir in objdirs:
        extra_qmake_cxxflags.append(f"-B{objdir}")

    if extra_qmake_cxxflags:
        qt_config_args.append("QMAKE_CXXFLAGS += {}".format(
            " ".join(extra_qmake_cxxflags)))
    if extra_qmake_lflags:
        qt_config_args.append("QMAKE_LFLAGS += {}".format(
            " ".join(extra_qmake_lflags)))

    for includedir in includedirs:
        qt_config_args.extend(["-I", includedir])
    for libdir in libdirs:
        qt_config_args.extend(["-L", libdir])

    qt_config_args.extend(QT_CONFIG_COMMON_ARGS)

    # Debug or release build of Qt?
    if cfg.qt_build_type == QT_BUILD_DEBUG:
        qt_config_args.append("-debug")
    elif cfg.qt_build_type == QT_BUILD_RELEASE:
        # Make a release-mode library. (Default is release.)
        qt_config_args.append("-release")
    elif cfg.qt_build_type == QT_BUILD_RELEASE_WITH_SYMBOLS:
        qt_config_args.append("-force-debug-info")
        # Not free, though: e.g. transforms your program's executable from
        # 210 Mb to 830 Mb!
    else:
        raise ValueError("Unknown Qt build type")
        # "-debug-and-release",  # make a release library as well: MAC ONLY
        # ... debug was the default in 4.8, but not in 5.7
        # ... release is default in 5.7 (as per "configure -h")
        # ... check with "readelf --debug-dump=decodedline <LIBRARY.so>"
        # ... http://stackoverflow.com/questions/1999654
        # ... https://forum.qt.io/topic/75056/configuring-qt-what-replaces-debug-and-release/7  # noqa

    # OpenSSL linkage?
    # For testing a new OpenSSL build, have cfg.qt_openssl_static=False, or you
    # have to rebuild Qt every time... extremely slow.
    if qt_openssl_linkage_static:
        qt_config_args.append("-openssl-linked")  # OpenSSL
        # http://doc.qt.io/qt-4.8/ssl.html
        # http://stackoverflow.com/questions/20843180
    else:
        qt_config_args.append("-openssl")  # OpenSSL

    if cfg.verbose >= 1:
        qt_config_args.append("-v")  # verbose
    if cfg.verbose >= 2:
        qt_config_args.append("-v")  # more verbose

    # Fix other Qt bugs:
    # ... cleaned up, none relevant at present

    # -------------------------------------------------------------------------
    # Qt: configure
    # -------------------------------------------------------------------------
    with pushd(builddir):
        try:
            run(qt_config_args, env)  # The configure step takes a few seconds.
        except subprocess.CalledProcessError:
            log.warning("""
..

===============================================================================
Troubleshooting Qt 'configure' failures
===============================================================================

-   gstreamer (used for Unix audio etc.)
    gstreamer version 1.0 version (for Unix) requires:
        sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
    ... NB some things try to remove it, it seems! (Maybe autoremove?)

-   Qt configure can't find make or gmake in PATH...

    If they are in the PATH, then check permissions on
          qtbase/config.tests/unix/which.test
    ... if not executable, permissions have been altered wrongly.

-   NB actual configure scripts are, from local build directory:
        .../src/qt5/configure
        .../src/qt5/configure/qtbase/configure
        .../src/qt5/configure/qtbase/configure.json

-   "recipe for target 'sub-plugins-make_first' failed", or similar:

    If configure fails, try more or less verbose (--verbose 0, --verbose 2) and
    also try "--nparallel 1" so you can see which point is failing more
    clearly. This is IMPORTANT or other error messages incorrectly distract
    you.

""")
            sys.exit(EXIT_FAILURE)

    # -------------------------------------------------------------------------
    # Qt: make (can take several hours)
    # -------------------------------------------------------------------------
    log.info(
        f"Making Qt {target_platform.description} build into {installdir}")
    with pushd(builddir):
        # run(cfg.make_args(command="qmake_all", env=env), env)
        try:
            run(cfg.make_args(env=env), env)
        except subprocess.CalledProcessError:
            log.warning("""
..

===============================================================================
Troubleshooting Qt 'make' failures
===============================================================================

Q.  If this is the first time you've had this error...
A.  RE-RUN THE SCRIPT; sometimes Qt builds fail then pick themselves up the
    next time.

Q.  If you can't see the error...
A.  Try with the "--nparallel 1" option.

Q.  (macOS) Errors like:
        "fatal error: 'os/log.h' file not found"
    or
        "error: use of undeclared identifier 'NSEventTypeMouseMoved';
        did you mean 'kEventMouseMoved'?"
A.  Standard header files like os/log.h should live within
    /Applications/Xcode.app. If they're missing:
    - Upgrade Xcode. (Xcode 7 is too old on macOS 10.13. Try Xcode 9.4.1.)
      That should install SDKs for iOS 11.4 and macOS 10.13.4.

""")  # noqa
            sys.exit(EXIT_FAILURE)

    # -------------------------------------------------------------------------
    # Qt: make install
    # -------------------------------------------------------------------------
    with pushd(builddir):
        run(cfg.make_args(command="install", env=env), env)
    # ... installs to installdir because of -prefix earlier
    return installdir


def make_missing_libqtforandroid_so(cfg: Config,
                                    target_platform: Platform) -> None:
    log.info(
        f"Making Android Qt dynamic library (from static version) for "
        f"{target_platform}")
    qt_install_dir = cfg.qt_install_dir(target_platform)
    parent_dir = join(qt_install_dir, "plugins", "platforms")
    starting_lib_dir = join(parent_dir, "android")
    static_ext = target_platform.static_lib_ext
    starting_a_lib = join(starting_lib_dir, "libqtforandroid" + static_ext)
    newlib_path = cfg.convert_android_lib_a_to_so(starting_a_lib,
                                                  target_platform)
    _, newlib_basename = split(newlib_path)
    extra_copy_newlib = join(parent_dir, newlib_basename)
    shutil.copyfile(newlib_path, extra_copy_newlib)


# =============================================================================
# SQLCipher
# =============================================================================

def fetch_sqlcipher(cfg: Config) -> None:
    """
    Downloads SQLCipher source code.
    """
    log.info("Fetching SQLCipher source...")
    git_clone(prettyname="SQLCipher",
              url=cfg.sqlcipher_git_url,
              commit=cfg.sqlcipher_git_commit,
              directory=cfg.sqlcipher_src_gitdir,
              clone_options=["--config", "core.autocrlf=false"],
              run_func=run)
    # We must have LF endings, not CR+LF, because we're going to use Unix tools
    # even under Windows.

    # The versions of config.guess and config.sub that come with SQLCipher
    # are taken (I think) from SQLite, and as of 2019-06-15, don't support
    # "aarch64". So let's fetch some proper ones:
    log.info("Replacing config.guess and config.sub with recent GNU versions, "
             "to detect newer architectures e.g. aarch64")
    download(CONFIG_GUESS_MASTER,
             join(cfg.sqlcipher_src_gitdir, "config.guess"))
    download(CONFIG_SUB_MASTER,
             join(cfg.sqlcipher_src_gitdir, "config.sub"))


def build_sqlcipher(cfg: Config, target_platform: Platform) -> None:
    """
    Builds SQLCipher, an open-source encrypted version of SQLite.
    Our source is the public version; our destination is an "amalgamation"
    .h and .c file (equivalent to the amalgamation sqlite3.h and sqlite3.c
    of SQLite itself). Actually, they have the same names, too.

    CROSS-COMPILATION OF SQLITE/SQLCIPHER:
    [1] https://vicente-hernando.appspot.com/sqlite3-cross-compile-arm-howto
    [2] https://discuss.zetetic.net/t/cross-compile-sqlicipher-for-arm/2104
    [3] https://github.com/sqlcipher/sqlcipher/issues/176
    """

    log.info("Building SQLCipher for {}...", target_platform)

    # -------------------------------------------------------------------------
    # SQLCipher: setup
    # -------------------------------------------------------------------------
    destdir = join(cfg.root_dir,
                   "sqlcipher_" + target_platform.dirpart,
                   "sqlcipher")  # this allows #include <sqlcipher/sqlite3.h>

    target_h = join(destdir, "sqlite3.h")
    target_c = join(destdir, "sqlite3.c")
    target_o = join(destdir, "sqlite3" + target_platform.obj_ext)
    target_exe = join(destdir, "sqlcipher")  # not always wanted

    want_exe = not target_platform.mobile and not BUILD_PLATFORM.windows

    targets = [target_c, target_h, target_o]
    if want_exe:
        targets.append(target_exe)
    if all(isfile(x) for x in targets):
        report_all_targets_exist("SQLCipher", targets)
        return

    copy_tree_contents(cfg.sqlcipher_src_gitdir, destdir, destroy=True)

    env = cfg.get_starting_env()
    cfg.set_compile_env(env, target_platform, use_cross_compile_var=False,
                        building_sqlcipher=True)

    _, openssl_workdir = cfg.get_openssl_rootdir_workdir(target_platform)
    openssl_include_dir = join(openssl_workdir, "include")

    # noinspection PyListCreation
    if BUILD_PLATFORM.windows:
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # SQLCipher/Windows
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # https://github.com/sqlitebrowser/sqlitebrowser/wiki/Win64-setup-%E2%80%94-Compiling-SQLCipher
        # We use a Windows native method, because:
        #   1. Can't get gcc to do everything
        #   2. Cygwin can't call cl.exe cleanly; paths look like "/cygdrive..."
        #      and Windows thinks that's a switch.
        #   3. When using "nmake /f Makefile.msc", the Cygwin tclsh fails,
        #      whereas the ActiveState one works.
        with pushd(destdir):
            makefile = "Makefile.msc"
            extra_tcc_rcc = "-DSQLITE_HAS_CODEC -I" + openssl_include_dir
            replace_multiple_in_file(
                filename=makefile,
                replacements=[
                    ("TCC = $(TCC) -DSQLITE_TEMP_STORE=1",
                     "TCC = $(TCC) -DSQLITE_TEMP_STORE=2 " + extra_tcc_rcc),

                    ("RCC = $(RCC) -DSQLITE_TEMP_STORE=1",
                     "RCC = $(RCC) -DSQLITE_TEMP_STORE=2 " + extra_tcc_rcc),
                ]
            )
            if not is_tclsh_windows_compatible():
                raise RuntimeError("Incompatible TCL interpreter; stopping")
            nmake = which_with_envpath(NMAKE, env)
            run([
                nmake,
                "/f", makefile,
                "sqlite3.h",
                "sqlite3.c",
                "libsqlite3.lib",
            ], env)
            # The Makefile.msc deletes all .obj files but compiles to .lo
            # files, which I think are identical (see LTCOMPILE, which calls
            # the compiler with the -Fo switch);
            # https://docs.microsoft.com/en-gb/cpp/build/reference/fo-object-file-name  # noqa
            shutil.copyfile("sqlite3.lo", target_o)

    else:
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        # SQLCipher/Unix: something other than Windows
        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        # ---------------------------------------------------------------------
        # SQLCipher/Unix: configure args
        # ---------------------------------------------------------------------

        # Compiler:
        cflags = [
            "-DSQLITE_HAS_CODEC",
            f"-I{openssl_include_dir}",
            # ... sqlite.c does e.g. "#include <openssl/rand.h>"
        ]
        if target_platform.android and USE_CLANG_NOT_GCC_FOR_ANDROID_ARM:
            cflags.append("-fPIC")
            # Otherwise, when linking CamCOPS, you get "error:
            # .../sqlcipher_android_armv7/sqlcipher/sqlite3.o: requires
            # unsupported dynamic reloc R_ARM_REL32; recompile with -fPIC".
            # For explanation, see
            # - https://stackoverflow.com/questions/5311515/gcc-fpic-option
            # The flag applies to clang as well as gcc.
        if target_platform.macos:
            cflags.append(f"-mmacosx-version-min={cfg.macos_min_version}")
        if "CFLAGS" in env:
            # inherit this too; 2018-08-24
            cflags.append(env["CFLAGS"])
        gccflags = [
            "-Wfatal-errors",  # all errors are fatal
        ]

        # Linker:
        ldflags = [f"-L{openssl_workdir}"]

        link_openssl_statically = target_platform.qt_linkage_static
        # ... try for dynamic linking on Android
        if link_openssl_statically:
            log.info("Linking OpenSSL into SQLCipher STATICALLY")
            static_ext = target_platform.static_lib_ext
            static_openssl_lib = join(openssl_workdir,
                                      "libcrypto" + static_ext)
            # Not working:
            # ldflags.append("-static")
            # ldflags.append("-l:libcrypto.a")
            # ... Note the colon! Search for ":filename" in "man ld"
            #
            # Try this:
            ldflags.append(static_openssl_lib)
            # ... https://github.com/sqlcipher/sqlcipher
        else:
            log.info("Linking OpenSSL into SQLCipher DYNAMICALLY")
            # make the executable load OpenSSL dynamically
            ldflags.append('-lcrypto')
        # Note that "--with-crypto-lib" isn't helpful here:
        # https://www.zetetic.net/blog/2013/6/27/sqlcipher-220-release.html

        trace_include = False
        if trace_include:
            cflags.append("-H")

        cflags.append("--sysroot={}".format(
            escape_literal_for_shell(cfg.sysroot(target_platform, env))))
        # ... or, for Android, configure will call ld which will say:
        #     ld: error: cannot open crtbegin_dynamic.o: No such file or directory  # noqa
        # ... escape_literal_for_shell() needed for paths with spaces in

        # bug in PyCharm list creation detector, I think, so:
        config_args = []  # type: List[str]
        config_args += [
            join(destdir, "configure"),
            "--enable-tempstore=yes",  # see README.md; equivalent to SQLITE_TEMP_STORE=2  # noqa
            # no quotes (they're fine on the command line but not here):
            f'CFLAGS={" ".join(cflags + gccflags)}',
            f'LDFLAGS={" ".join(ldflags)}',
        ]
        # By default, SQLCipher compiles with "-O2" optimizations under gcc;
        # see its "configure" script.

        # Platform-specific tweaks; cross-compilation.
        # The CROSS_COMPILE prefix doesn't appear in any files, so is
        # presumably not supported, but "--build" and "--host" are used (where
        # "host" means "target").

        config_args.append(f"--build={BUILD_PLATFORM.sqlcipher_platform}")
        config_args.append(f"--host={target_platform.sqlcipher_platform}")
        config_args.append(f"--prefix={cfg.sysroot(target_platform, env)}")

        # ---------------------------------------------------------------------
        # SQLCipher/Unix: configure
        # ---------------------------------------------------------------------
        with pushd(destdir):
            run(config_args, env)

        # ---------------------------------------------------------------------
        # SQLCipher/Unix: make
        # ---------------------------------------------------------------------
        with pushd(destdir):
            # Don't use cfg.make_args(); we want "make" even under Windows (via
            # Cygwin).
            require(MAKE)
            require(TCLSH)
            if not isfile(target_c) or not isfile(target_h):
                # Under Windows, if we were to use cl rather than gcc, e.g. by
                # setting env["CC"], it fails because the make environment uses
                # Unix-style paths. So we let it use gcc.
                run([MAKE, "sqlite3.c"], env)  # the amalgamation target
            if not isfile(target_exe) or not isfile(target_o):
                run([MAKE, "sqlite3" + target_platform.obj_ext], env)  # for static linking  # noqa
            if want_exe and not isfile(target_exe):
                run([MAKE, "sqlcipher"], env)  # the command-line executable

        # -------------------------------------------------------------------------
        # SQLCipher/Unix: Check and report
        # -------------------------------------------------------------------------
        target_platform.verify_lib(target_o)

    log.info(
        f"If successful, you should have the amalgation files:\n"
        f"- {target_c}\n"
        f"- {target_h}\n"
        f"and the library:\n"
        f"- {target_o}\n"
        f"and, on non-mobile platforms, the executable:\n"
        f"- {target_exe}")


# =============================================================================
# Eigen
# =============================================================================
# A better matrix system than mlpack, not least in that Eigen is headers-only

def fetch_eigen(cfg: Config) -> None:
    """
    Downloads Eigen.
    http://eigen.tuxfamily.org
    """
    log.info("Fetching Eigen source...")
    download_if_not_exists(cfg.eigen_src_url, cfg.eigen_src_fullpath)


def build_eigen(cfg: Config) -> None:
    """
    'Build' simply means 'unpack' -- header-only template library.
    """
    log.info("Building (unpacking) Eigen...")
    untar_to_directory(tarfile=cfg.eigen_src_fullpath,
                       directory=cfg.eigen_unpacked_dir,
                       gzipped=True,
                       run_func=run,
                       chdir_via_python=True)


# =============================================================================
# Master build function
# =============================================================================

def master_builder(args) -> None:
    """
    Do the work!
    """
    # =========================================================================
    # Calculated args
    # =========================================================================
    cfg = Config(args)
    log.debug("Args: {}", args)
    log.debug("Config: {}", cfg)
    log.info("Running on {}", BUILD_PLATFORM)

    if cfg.show_config_only:
        sys.exit(EXIT_SUCCESS)

    # =========================================================================
    # Test the environment passing
    # =========================================================================
    # run(["/usr/local/bin/shared/print_params_and_env_then_abort", "hello",
    #      "world"])  # full environment passed through
    # run(["/usr/local/bin/shared/print_params_and_env_then_abort", "hello",
    #      "world"], env={"SOMEVAR": "someval"})  # only what Bash added

    # =========================================================================
    # Common requirements
    # =========================================================================
    # require(CMAKE)
    require(GIT)
    require(PERL)
    require(TAR)
    if BUILD_PLATFORM.windows:
        require(VCVARSALL)
    BUILD_PLATFORM.ensure_elf_reader()

    # =========================================================================
    # Fetch
    # =========================================================================
    fetch_qt(cfg)
    fetch_openssl(cfg)
    fetch_sqlcipher(cfg)
    fetch_eigen(cfg)

    # =========================================================================
    # Build
    # =========================================================================

    build_eigen(cfg)

    installdirs = []
    done_extra = False

    # noinspection PyShadowingNames
    def build_for(os: str, cpu: str) -> None:
        target_platform = Platform(os, cpu)
        log.info(f"Building (1) OpenSSL, (2) SQLite/SQLCipher, (3) Qt "
                 f"for {target_platform}")
        build_openssl(cfg, target_platform)
        build_sqlcipher(cfg, target_platform)
        installdirs.append(
            build_qt(cfg, target_platform)
        )
        if target_platform.android and ADD_SO_VERSION_OF_LIBQTFORANDROID:
            make_missing_libqtforandroid_so(cfg, target_platform)

    if cfg.build_android_x86_32:  # for x86 Android emulator
        build_for(Os.ANDROID, Cpu.X86_32)

    if cfg.build_android_arm_v7_32:  # for native Android, 32-bit ARM
        build_for(Os.ANDROID, Cpu.ARM_V7_32)

    if cfg.build_android_arm_v8_64:  # for native Android, 64-bit ARM
        build_for(Os.ANDROID, Cpu.ARM_V8_64)

    if cfg.build_linux_x86_64:  # for 64-bit Linux
        build_for(Os.LINUX, Cpu.X86_64)

    if cfg.build_macos_x86_64:  # for 64-bit Intel macOS
        build_for(Os.MACOS, Cpu.X86_64)

    if cfg.build_windows_x86_64:  # for 64-bit Windows
        build_for(Os.WINDOWS, Cpu.X86_64)

    if cfg.build_windows_x86_32:  # for 32-bit Windows
        if BUILD_PLATFORM.linux:
            fail(CANNOT_CROSS_COMPILE_QT)
        build_for(Os.WINDOWS, Cpu.X86_32)

    if cfg.build_ios_arm_v7_32:  # for iOS (e.g. iPad) with 32-bit ARM processor  # noqa
        build_for(Os.IOS, Cpu.ARM_V7_32)

    if cfg.build_ios_arm_v8_64:  # for iOS (e.g. iPad) with 64-bit ARM processor  # noqa
        build_for(Os.IOS, Cpu.ARM_V8_64)

    if cfg.build_ios_simulator_x86_32:  # 32-bit iOS simulator under Intel macOS  # noqa
        build_for(Os.IOS, Cpu.X86_32)

    if cfg.build_ios_simulator_x86_64:  # 64-bit iOS simulator under Intel macOS  # noqa
        build_for(Os.IOS, Cpu.X86_64)

    if not installdirs and not done_extra:
        log.warning("Nothing more to do. Run with --help argument for help.")
        sys.exit(EXIT_FAILURE)

    log.info("""
..

===============================================================================
Now, to compile CamCOPS using Qt Creator:
===============================================================================

See tablet_qt/notes/QT_PROJECT_SETTINGS.txt

    """)
    sys.exit(EXIT_SUCCESS)


# =============================================================================
# Main
# =============================================================================

def main() -> None:
    """
    Main entry point.
    """
    # -------------------------------------------------------------------------
    # Command-line arguments
    # -------------------------------------------------------------------------

    # noinspection PyTypeChecker
    parser = argparse.ArgumentParser(
        description="Build Qt and other libraries for CamCOPS",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    # General
    general = parser.add_argument_group("General", "General options")
    default_root_dir = os.environ.get(ENVVAR_QT_BASE) or DEFAULT_ROOT_DIR
    general.add_argument(
        "--show_config_only", action="store_true",
        help="Show config, then quit")
    general.add_argument(
        "--root_dir", default=default_root_dir,
        help=(
            f"Root directory for source and builds (default taken from "
            f"environment variable {ENVVAR_QT_BASE} if present)"
        )
    )
    general.add_argument(
        "--nparallel", type=int, default=multiprocessing.cpu_count(),
        help="Number of parallel processes to run")
    general.add_argument(
        "--force", action="store_true", help="Force build")
    general.add_argument(
        "--tee", type=str, default=None,
        help="Copy stdout/stderr to this named file")
    general.add_argument(
        "--verbose", "-v", type=int, default=0, choices=[0, 1, 2],
        help="Verbosity level")
    general.add_argument(
        "--inherit_os_env", dest="inherit_os_env", action="store_true",
        help="Inherit the parent OS environment variables")
    general.add_argument(
        "--no_inherit_os_env", dest="inherit_os_env", action="store_false",
        help="Do not inherit the parent OS environment variables")
    parser.set_defaults(inherit_os_env=not BUILD_PLATFORM.linux)

    # Architectures
    archgroup = parser.add_argument_group(
        "Architecture",
        "Choose architecture for which to build")
    archgroup.add_argument(
        "--build_all", action="store_true",
        help=(
            f"Build for all architectures supported on this host (this host "
            f"is: {BUILD_PLATFORM})"
        )
    )
    archgroup.add_argument(
        "--build_android_x86_32", action="store_true",
        help="An architecture target (Android under an "
             "Intel x86 32-bit emulator)")
    archgroup.add_argument(
        "--build_android_arm_v7_32", action="store_true",
        help="An architecture target (Android with a 32-bit ARM processor)")
    archgroup.add_argument(
        "--build_android_arm_v8_64", action="store_true",
        help="An architecture target (Android with a 64-bit ARM processor)")
    archgroup.add_argument(
        "--build_linux_x86_64", action="store_true",
        help="An architecture target (native Linux with a 64-bit Intel/AMD "
             "CPU; check with 'lscpu' and 'uname -a'")
    archgroup.add_argument(
        "--build_macos_x86_64", action="store_true",
        help="An architecture target (macOS under an Intel 64-bit CPU; "
             "check with 'sysctl -a|grep cpu', and see "
             "https://support.apple.com/en-gb/HT201948 )")
    archgroup.add_argument(
        "--build_windows_x86_64", action="store_true",
        help="An architecture target (Windows with an Intel/AMD 64-bit CPU)"
    )
    archgroup.add_argument(
        "--build_windows_x86_32", action="store_true",
        help="An architecture target (Windows with an Intel/AMD 32-bit CPU)"
    )
    archgroup.add_argument(
        "--build_ios_arm_v7_32", action="store_true",
        help="An architecture target (iOS with a 32-bit ARM processor)"
    )
    archgroup.add_argument(
        "--build_ios_arm_v8_64", action="store_true",
        help="An architecture target (iOS with a 64-bit ARM processor)"
    )
    archgroup.add_argument(
        "--build_ios_simulator_x86_32", action="store_true",
        help="An architecture target (iOS with an Intel 32-bit CPU, for the "
             "iOS simulator)"
    )
    archgroup.add_argument(
        "--build_ios_simulator_x86_64", action="store_true",
        help="An architecture target (iOS with an Intel 64-bit CPU, for the "
             "iOS simulator)"
    )

    # Qt
    qt = parser.add_argument_group(
        "Qt",
        "Qt options [Qt must be built from source for SQLite support, and "
        "also if static OpenSSL linkage is desired; note that static OpenSSL "
        "linkage requires a Qt rebuild (slow!) if you rebuild OpenSSL]")
    qt.add_argument(
        "--qt_build_type", type=str, default=QT_BUILD_RELEASE,
        choices=QT_POSSIBLE_BUILD_TYPES,
        help="Qt build type (release = small and quick)")
    qt.add_argument(
        "--qt_src_dirname", default=DEFAULT_QT_SRC_DIRNAME,
        help="Qt source directory")
    qt.add_argument(
        "--qt_openssl_static", dest="qt_openssl_static", action="store_true",
        help="Link OpenSSL statically (ONLY if Qt is statically linked) "
             "[True=static, False=dynamic]")
    qt.add_argument(
        "--qt_openssl_linked", dest="qt_openssl_static", action="store_false",
        help="Link OpenSSL dynamically [True=static, False=dynamic]")
    parser.set_defaults(qt_openssl_static=DEFAULT_QT_USE_OPENSSL_STATICALLY)

    # Android
    android = parser.add_argument_group(
        "Android",
        "Android options (NB you must install the Android SDK and NDK "
        "separately, BEFOREHAND)")
    android.add_argument(
        "--android_sdk_root", default=DEFAULT_ANDROID_SDK,
        help="Android SDK root directory")
    android.add_argument(
        "--android_ndk_root", default=DEFAULT_ANDROID_NDK,
        help="Android NDK root directory")
    android.add_argument(
        "--android_ndk_host", default=DEFAULT_ANDROID_NDK_HOST,
        help="Android NDK host architecture")
    android.add_argument(
        "--android_toolchain_version", default=DEFAULT_ANDROID_TOOLCHAIN_VERSION,  # noqa
        help="Android toolchain version")
    android.add_argument(
        "--java_home", default=DEFAULT_JAVA_HOME,
        help="JAVA_HOME directory"
    )

    # iOS
    ios = parser.add_argument_group("iOS", "iOS options")
    ios.add_argument(
        "--ios_sdk", default="",
        help="iOS SDK to use (leave blank for system default)"
    )

    # jom
    jom = parser.add_argument_group(
        "jom",
        "'jom' parallel make tool for Windows"
    )
    jom.add_argument(
        "--jom_executable", default=r"C:\Qt\Tools\QtCreator\bin\jom.exe",
        help="jom executable (typically installed with QtCreator)"
    )

    args = parser.parse_args()

    # =========================================================================
    # Logging, including a tee facility
    # =========================================================================
    # noinspection PyUnresolvedReferences
    loglevel = logging.DEBUG if args.verbose >= 1 else logging.INFO
    if main_only_quicksetup_rootlogger:
        main_only_quicksetup_rootlogger(level=loglevel)
    else:
        logging.basicConfig(level=loglevel, format=LOG_FORMAT,
                            datefmt=LOG_DATEFMT)
    if args.tee:
        with open(args.tee, "wt") as tee_file:  # type: TextIO
            with tee_log(tee_file, loglevel=loglevel):
                master_builder(args)
    else:
        master_builder(args)

    # =========================================================================
    # Some other bits of verbosity
    # =========================================================================
    if args.verbose >= 2:
        global DEBUG_SHOW_ENV
        DEBUG_SHOW_ENV = True


if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as e:
        log.critical("External process failed:")
        traceback.print_exc()
        sys.exit(EXIT_FAILURE)
