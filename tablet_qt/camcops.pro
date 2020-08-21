#-------------------------------------------------
#
# Project created by QtCreator 2016-05-30T22:51:39
#
#-------------------------------------------------

# http://doc.qt.io/qt-5/qmake-project-files.html
# http://doc.qt.io/qt-5/qmake-variable-reference.html

message("+++ CamCOPS qmake starting.")

# =============================================================================
# Prerequisites: environment variables
# =============================================================================
# Need environment variable CAMCOPS_QT_BASE_DIR
# Put something like this in your ~/.profile:
#   export CAMCOPS_QT_BASE_DIR="/home/rudolf/dev/qt_local_build"

# This file is read by qmake.
# Use $$(...) to read an environment variable at the time of qmake.
# Use $(...) to read an environment variable at the time of make.
# Use $$... or $${...} to read a Qt project file variable.
# See
# - http://doc.qt.io/qt-4.8/qmake-advanced-usage.html#variables
# - http://doc.qt.io/qt-5/qmake-test-function-reference.html
# Here, we copy an environment variable to a Qt project file variable:
# QT_BASE_DIR = $(CAMCOPS_QT_BASE_DIR)  # value at time of make

QT_BASE_DIR = $$(CAMCOPS_QT_BASE_DIR)  # value at time of qmake ("now")
isEmpty(QT_BASE_DIR) {
    error("Environment variable CAMCOPS_QT_BASE_DIR is undefined")
}
message("Using custom Qt/library base directory: $${QT_BASE_DIR}")
message("... Qt version: $$[QT_VERSION]")
message("... Qt is installed in: $$[QT_INSTALL_PREFIX]")
message("... Qt resources can be found in the following locations:")
message("... Documentation: $$[QT_INSTALL_DOCS]")
message("... Header files: $$[QT_INSTALL_HEADERS]")
message("... Libraries: $$[QT_INSTALL_LIBS]")
message("... Binary files (executables): $$[QT_INSTALL_BINS]")
message("... Plugins: $$[QT_INSTALL_PLUGINS]")
message("... Data files: $$[QT_INSTALL_DATA]")
message("... Translation files: $$[QT_INSTALL_TRANSLATIONS]")
message("... Settings: $$[QT_INSTALL_CONFIGURATION]")
message("... Examples: $$[QT_INSTALL_EXAMPLES]")

# AVOID # CAMCOPS_SOURCE_ROOT = ${PWD}  # at time of make
CAMCOPS_SOURCE_ROOT = $${PWD}  # at time of qmake ("now")
message("Expecting CamCOPS source root at: $${CAMCOPS_SOURCE_ROOT}")

# message("QMAKESPEC: $${QMAKESPEC}")
# message("QMAKE_PLATFORM: $${QMAKE_PLATFORM}")
message("... QT_ARCH: $${QT_ARCH}")

# =============================================================================
# Parts of Qt
# =============================================================================

# SQLite/Android and OpenSSL/anything requires a custom Qt build.
# ALSO TRY:
#   qmake -query  # for the qmake of the Qt build you're using

# http://doc.qt.io/qt-5/qtmultimedia-index.html
# http://wiki.qt.io/Qt_5.5.0_Multimedia_Backends
# http://doc.qt.io/qt-4.8/qmake-variable-reference.html#qt
# http://doc.qt.io/qt-5/qtmodules.html

QT += core  # included by default; QtCore module
QT += gui  # included by default; QtGui module
QT += multimedia  # or: undefined reference to QMedia*::*
QT += multimediawidgets
# QT += network  # required to #include <QtNetwork/...>
# QT += printsupport  # QCustomPlot says it needs this, but it appears not
QT += quick  # for QML, e.g. for camera
QT += quickwidgets  # for QQuickWidget
QT += sql  # required to #include <QSqlDatabase>
QT += svg  # required to #include <QGraphicsSvgItem> or <QSvgRenderer>
# QT += webkit  # for QWebView -- no, not used
# QT += webkitwidgets  # for QWebView -- no, not used
QT += widgets  # required to #include <QApplication>
# message("QT = $${QT}")
# message("QTPLUGIN = $${QTPLUGIN}")

# http://stackoverflow.com/questions/20351155/how-can-i-enable-ssl-in-qt-windows-application
# http://stackoverflow.com/questions/18663331/how-to-check-the-selected-version-of-qt-in-a-pro-file

# =============================================================================
# Overall configuration
# =============================================================================

# CONFIG += debug
    # ... no, use the QtCreator debug/release settings, which adds
    # e.g. "CONFIG+=debug CONFIG+=qml_debug" to the call to qmake
CONFIG += mobility
CONFIG += c++11

# For SQLCipher (see its README.md):
# http://stackoverflow.com/questions/16244040/is-the-qt-defines-doing-the-same-thing-as-define-in-c
DEFINES += SQLITE_HAS_CODEC
DEFINES += SQLITE_TEMP_STORE=2

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

MOBILITY =

# PKGCONFIG += openssl
# ... http://stackoverflow.com/questions/14681012/how-to-include-openssl-in-a-qt-project
# ... but no effect? Not mentioned in variable reference (above).
# ... ah, here's the reference:
#     http://doc.qt.io/qt-5/qmake-project-files.html
# LIBS += -lssl
# ... not working either? Doesn't complain, but ldd still shows that system libssl.so is in use

# =============================================================================
# Compiler and linker flags
# =============================================================================

# Warning become errors
gcc {
    # GCC
    QMAKE_CXXFLAGS += -Werror  # warnings become errors
}
msvc {
    # Microsoft Visual C++
    QMAKE_CXXFLAGS += /W3
        # ... /W4 is the highest level of warnings bar "/Wall"
        # ... but we get "D9025: overriding '/W4' with '/W3'"
    QMAKE_CXXFLAGS += /WX  # treat warnings as errors
    # QMAKE_CXXFLAGS += /showIncludes  # if you think the wrong ones are being included!
}

# In release mode, optimize heavily:
gcc {
    QMAKE_CXXFLAGS_RELEASE -= -O
    QMAKE_CXXFLAGS_RELEASE -= -O1
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -O3  # optimize heavily
}
msvc {
    QMAKE_CXXFLAGS_RELEASE -= /O1  # /O1: minimum size
    QMAKE_CXXFLAGS_RELEASE += /O2  # /O2: maximum speed

    # We also get "LNK4098: defaultlib 'LIBCMT' conflicts with use of other
    # libs; use /NODEFAULTLIB:library" but this may be because the /MD, /MT,
    # or /LD setting conflicts.
    # /MD: multithread-specific and DLL-specific version of the runtime library
    # /MT: multithread, static version of the runtime library [fails; LIBCMT conflicts...]
    # /LD: creates a DLL
    QMAKE_CXXFLAGS_RELEASE += /MD
}

# For compilers that support it, make the visibility hidden
# Note: there may be linker warnings relating to access to Qt functions, but
# they're only warnings.
gcc {
    # http://gcc.gnu.org/wiki/Visibility
    QMAKE_CXXFLAGS += -fvisibility=hidden
}
# But WARNING: this will also hide main(), which isn't a problem in Linux/
# Windows executables, but is a major problem for Android, where the Qt Java
# application wends its way to C++ then calls main() from libcamcops.so. The
# end result is an instant crash on startup. So main() must be marked for
# symbol export. [Understood/fixed 2019-06-20.]

# QMAKE_LFLAGS += --verbose  # make the linker verbose

# =============================================================================
# Build targets
# =============================================================================

TARGET = camcops
TEMPLATE = app

# =============================================================================
# Paths
# =============================================================================
# See build_qt.py for how these are built (or not built) as required.

INCLUDEPATH += "$${QT_BASE_DIR}/eigen/eigen-eigen-67e894c6cd8f"  # from which: <Eigen/...>
# INCLUDEPATH += "$${QT_BASE_DIR}/armadillo/armadillo-7.950.0/include"  # from which: <armadillo>
# INCLUDEPATH += "$${QT_BASE_DIR}/armadillo/armadillo-7.950.0/include/armadillo_bits"
# INCLUDEPATH += "$${QT_BASE_DIR}/boost/boost_1_64_0"  # from which: <boost/...>
# INCLUDEPATH += "$${QT_BASE_DIR}/src/mlpack/src"  # from which: <mlpack/...>

# =============================================================================
# OS-specific options
# =============================================================================
# https://wiki.qt.io/Technical_FAQ#How_can_I_detect_in_the_.pro_file_if_I_am_compiling_for_a_32_bit_or_a_64_bit_platform.3F

OPENSSL_VERSION = 1.1.1c
# ... see build_qt.py or changelog.rst for chronology

# -----------------------------------------------------------------------------
# Architecture
# -----------------------------------------------------------------------------

OBJ_EXT = ".o"  # unless otherwise set

# Set OS-specific variables
# Operating system tests include "linux", "unix", "macx", "android", "windows",
# "ios".
linux : !android {
    # -------------------------------------------------------------------------
    # LINUX -- and not Android Linux!
    # -------------------------------------------------------------------------
    STATIC_LIB_EXT = ".a"
    DYNAMIC_LIB_EXT = ".so"
    CAMCOPS_QT_LINKAGE = "static"
    # CAMCOPS_QT_LINKAGE = "dynamic"
    CAMCOPS_OPENSSL_LINKAGE = "static"

    # https://stackoverflow.com/questions/33117822
    # https://stackoverflow.com/questions/356666
    contains(QT_ARCH, x86_64) {
        message("Building for Linux/x86_64")
        CAMCOPS_ARCH_TAG = "linux_x86_64"
    } else {  # will be "i386"
        message("Building for Linux/x86_32")
        CAMCOPS_ARCH_TAG = "linux_x86_32"
    }
}
android {
    # -------------------------------------------------------------------------
    # ANDROID
    # -------------------------------------------------------------------------
    STATIC_LIB_EXT = ".a"
    DYNAMIC_LIB_EXT = ".so"
    CAMCOPS_QT_LINKAGE = "dynamic"
    # CAMCOPS_OPENSSL_LINKAGE = "static"
    CAMCOPS_OPENSSL_LINKAGE = "dynamic"

    contains(ANDROID_TARGET_ARCH, x86) {
        message("Building for Android/x86 (e.g. Android emulator)")
        CAMCOPS_ARCH_TAG = "android_x86"
    }
    contains(ANDROID_TARGET_ARCH, armeabi-v7a) {
        message("Building for Android/ARMv7 32-bit architecture")
        CAMCOPS_ARCH_TAG = "android_armv7"
    }
    contains(ANDROID_TARGET_ARCH, arm64-v8a) {
        message("Building for Android/ARMv8 64-bit architecture")
        CAMCOPS_ARCH_TAG = "android_armv8_64"
    }

    # http://doc.qt.io/qt-5/deployment-android.html#android-specific-qmake-variables
    ANDROID_PACKAGE_SOURCE_DIR = "$${CAMCOPS_SOURCE_ROOT}/android"
    message("ANDROID_PACKAGE_SOURCE_DIR: $${ANDROID_PACKAGE_SOURCE_DIR}")
    # ... contains things like AndroidManifest.xml
}
windows {
    # -------------------------------------------------------------------------
    # WINDOWS
    # -------------------------------------------------------------------------
    STATIC_LIB_EXT = ".lib"
    DYNAMIC_LIB_EXT = ".dll"
    OBJ_EXT = ".obj"
    CAMCOPS_QT_LINKAGE = "static"
    CAMCOPS_OPENSSL_LINKAGE = "static"

    # https://stackoverflow.com/questions/26373143
    # https://stackoverflow.com/questions/33117822
    # https://stackoverflow.com/questions/356666
    contains(QT_ARCH, x86_64) {
        message("Building for Windows/x86_64 architecture")
        CAMCOPS_ARCH_TAG = "windows_x86_64"
    } else {
        message("Building for Windows/x86_32 architecture")
        CAMCOPS_ARCH_TAG = "windows_x86_32"
    }

    RC_FILE = windows/camcops.rc
}
macx {
    # -------------------------------------------------------------------------
    # MacOS (formerly OS X)
    # -------------------------------------------------------------------------
    STATIC_LIB_EXT = ".a"
    DYNAMIC_LIB_EXT = ".dylib"
    CAMCOPS_QT_LINKAGE = "static"
    CAMCOPS_OPENSSL_LINKAGE = "static"

    contains(QT_ARCH, x86_64) {
        message("Building for MacOS/x86_64 architecture")
        CAMCOPS_ARCH_TAG = "macos_x86_64"
    } else {
        message("Building for MacOS/x86_32 architecture")
        CAMCOPS_ARCH_TAG = "macos_x86_32"
    }
}
ios {
    # -------------------------------------------------------------------------
    # iOS
    # -------------------------------------------------------------------------
    STATIC_LIB_EXT = ".a"
    DYNAMIC_LIB_EXT = ".dylib"
    CAMCOPS_QT_LINKAGE = "dynamic"
    CAMCOPS_OPENSSL_LINKAGE = "dynamic"

    contains(QT_ARCH, arm64) {
        message("Building for iOS/ARM v8 64-bit architecture")
        CAMCOPS_ARCH_TAG = "ios_armv8_64"
    } else {
        message("Building for iOS/ARM v7 (32-bit) architecture")
        CAMCOPS_ARCH_TAG = "ios_armv7"
    }
}

isEmpty(CAMCOPS_ARCH_TAG) {
    error("Unknown architecture; don't know how to build CamCOPS")
}

# -----------------------------------------------------------------------------
# Linkage method
# -----------------------------------------------------------------------------

# To have the linker show its working:
# LIBS += "-Wl,--verbose"

equals(CAMCOPS_QT_LINKAGE, "static") {  # http://doc.qt.io/qt-5/qmake-test-function-reference.html
    message("Using static linkage from CamCOPS to Qt")
    CONFIG += static
} else:equals(CAMCOPS_QT_LINKAGE, "dynamic") {
    message("Using dynamic linkage from CamCOPS to Qt")
} else {
    error("Linkage method from CamCOPS to Qt not specified")
}

equals(CAMCOPS_OPENSSL_LINKAGE, "static") {
    message("Using static linkage from CamCOPS to OpenSSL")
} else:equals(CAMCOPS_OPENSSL_LINKAGE, "dynamic") {
    message("Using dynamic linkage from CamCOPS to OpenSSL")
} else {
    error("Linkage method from CamCOPS to OpenSSL not specified")
}

# Quick tutorial on linking, since I seem to need it:
#
# FILE TYPES
#
# - Standard Linux file types [5, 6]:
#       .o      object file
#       .a      archive (= collection of one or more object files)
#       .so     shared object (approx. equiv. to Windows DLL)
#
# COMPILER AND LINKER
#
# - g++ is the C++ compiler from the Gnu Compiler Collection but also calls the
#   linker [1]. Use "g++ --version" to find the version; currently I have
#   5.4.0.
#
# - qmake will call g++, which will call GNU's ld (the linker).
#
# - ld parameters include
#       -L<path> (as per "man g++") or -L <path> (as per "man ld")
#       -l<library>
#
# - When you specify "-lxyz", the linker looks for "libxyz.a", in standard
#   system directories plus any you specify with -L [2].
#   "Normally the files found this way are library files—archive files whose
#   members are object files. The linker handles an archive file by scanning
#   through it for members which define symbols that have so far been
#   referenced but not defined. But if the file that is found is an ordinary
#   object file, it is linked in the usual fashion. The only difference
#   between using an -l option and specifying a file name is that -l surrounds
#   library with ‘lib’ and ‘.a’ and searches several directories."
#
# - There is also the "-l:filename" object, which skips the "lib" and ".a"
#   parts [3].
#
# INFORMATION ON OBJECT/LIBRARY/EXECUTABLES
#
# - ldd prints the shared objects (shared libraries) required by a program or
#   shared object (so use it for executables and .so files).
#
# - An alternative is
#       objdump -p TARGET | grep NEEDED  # or omit the grep for more info
#   ... objdump works for .so, .a, and executables.
#
# - You can also use
#       objdump -t TARGET
#   to view its symbols.
#   For example, you might find that a linked version of camcops not only
#   requires libcrypto.so.1.1, but that it contains symbols present in
#   libcrypto, such as EVP_aes_256_cfb, if we add a specific libcrypto.a to the
#   linker... so something is requiring the dynamic library even as we're
#   trying to link statically (despite configuring Qt with -openssl-linked).
#
# - Show files in an archive (.a) [4]:
#       ar -t libxyz.a
#
# APPLIED TO CAMCOPS
#
# - If you have the linker show its working with
#       -Wl,--verbose
#   then you can also see it finding libcrypto.so during the link process, but
#   then the executable not finding it when it runs, e.g.:
#
#   link:
#       $ make
#       attempt to open /home/rudolf/dev/qt_local_build/openssl_linux_x86_64_build/openssl-1.1.0g/libcrypto.so succeeded
#       -lcrypto (/home/rudolf/dev/qt_local_build/openssl_linux_x86_64_build/openssl-1.1.0g/libcrypto.so)
#   run:
#       $ ./camcops
#       ./camcops: error while loading shared libraries: libssl.so.1.1: cannot open shared object file: No such file or directory
#   run with manual library path:
#       $ LD_LIBRARY_PATH=/home/rudolf/dev/qt_local_build/openssl_linux_x86_64_build/openssl-1.1.0g ./camcops
#       ... runs.
#
# REFERENCES
#
# [1] https://stackoverflow.com/questions/172587/what-is-the-difference-between-g-and-gcc
# [2] https://gcc.gnu.org/onlinedocs/gcc-5.5.0/gcc/Link-Options.html#Link-Options
# [3] https://stackoverflow.com/questions/6578484/telling-gcc-directly-to-link-a-library-statically
# [4] http://www.yolinux.com/TUTORIALS/LibraryArchives-StaticAndDynamic.html
# [5] https://stackoverflow.com/questions/9688200/difference-between-shared-objects-so-static-libraries-a-and-dlls-so
# [6] https://en.wikipedia.org/wiki/Ar_(Unix)

# -----------------------------------------------------------------------------
# OpenSSL
# -----------------------------------------------------------------------------
OPENSSL_SUBDIR = openssl-$${OPENSSL_VERSION}
OPENSSL_DIR = "$${QT_BASE_DIR}/openssl_$${CAMCOPS_ARCH_TAG}_build/$${OPENSSL_SUBDIR}"
message("Using OpenSSL version $$OPENSSL_VERSION from $${OPENSSL_DIR}")
INCLUDEPATH += "$${OPENSSL_DIR}/include"
equals(CAMCOPS_OPENSSL_LINKAGE, "static") {
    LIBS += "-L$${OPENSSL_DIR}"  # path; shouldn't be necessary for static linkage! Residual problem.
    LIBS += "$${OPENSSL_DIR}/libcrypto$${STATIC_LIB_EXT}"  # raw filename, not -l
    LIBS += "$${OPENSSL_DIR}/libssl$${STATIC_LIB_EXT}"  # raw filename, not -l
} else {
    LIBS += "-L$${OPENSSL_DIR}"  # path
    LIBS += "-lcrypto"
    LIBS += "-lssl"
}
# Regardless of how *CamCOPS* talks to OpenSSL, under Android *Qt* talks to
# it dynamically:
ANDROID_EXTRA_LIBS += "$${OPENSSL_DIR}/libcrypto$${DYNAMIC_LIB_EXT}"  # needed for Qt
ANDROID_EXTRA_LIBS += "$${OPENSSL_DIR}/libssl$${DYNAMIC_LIB_EXT}"
# ... must start "lib" and end ".so", otherwise Qt complains.

# -----------------------------------------------------------------------------
# SQLCipher
# -----------------------------------------------------------------------------
SQLCIPHER_DIR = "$${QT_BASE_DIR}/sqlcipher_$${CAMCOPS_ARCH_TAG}"
message("Using SQLCipher from $${SQLCIPHER_DIR}")
INCLUDEPATH += "$${SQLCIPHER_DIR}"  # from which: <sqlcipher/sqlite3.h>
LIBS += "$${SQLCIPHER_DIR}/sqlcipher/sqlite3$${OBJ_EXT}"  # add .o file
# ... if that causes the error "multiple definition of 'sqlite3_free'", etc.,
#     then Qt is inappropriately linking in SQLite as well.

# -----------------------------------------------------------------------------
# All set up
# -----------------------------------------------------------------------------
message("QMAKE_CFLAGS=$${QMAKE_CFLAGS}")
message("QMAKE_CXXFLAGS=$${QMAKE_CXXFLAGS}")
message("INCLUDEPATH=$${INCLUDEPATH}")
message("LIBS=$${LIBS}")
message("... qmake will add more to INCLUDEPATH and LIBS; see Makefile")
# ... view the Makefile at the end; qmake will have added others
# ... and run "ldd camcops" to view dynamic library dependencies
android {
    message("ANDROID_EXTRA_LIBS=$${ANDROID_EXTRA_LIBS}")
}

# =============================================================================
# Resources and source files
# =============================================================================

RESOURCES += \
    camcops.qrc

SOURCES += \
    common/appstrings.cpp \
    common/cssconst.cpp \
    common/dbconst.cpp \
    common/dpi.cpp \
    common/globals.cpp \
    common/platform.cpp \
    common/textconst.cpp \
    common/uiconst.cpp \
    common/urlconst.cpp \
    common/varconst.cpp \
    common/widgetconst.cpp \
    core/camcopsapp.cpp \
    core/networkmanager.cpp \
    crypto/cryptofunc.cpp \
    crypto/secureqbytearray.cpp \
    crypto/secureqstring.cpp \
    db/ancillaryfunc.cpp \
    db/blobfieldref.cpp \
    db/databasemanager.cpp \
    db/databaseobject.cpp \
    db/databaseworkerthread.cpp \
    db/dbfunc.cpp \
    db/dbnestabletransaction.cpp \
    db/dbtransaction.cpp \
    db/dumpsql.cpp \
    db/field.cpp \
    db/fieldcreationplan.cpp \
    db/fieldref.cpp \
    db/queryresult.cpp \
    db/sqlargs.cpp \
    db/sqlcachedresult.cpp \
    db/sqlcipherdriver.cpp \
    db/sqlcipherhelpers.cpp \
    db/sqlcipherresult.cpp \
    db/sqlitepragmainfofield.cpp \
    db/threadedqueryrequest.cpp \
    db/whereconditions.cpp \
    db/whichdb.cpp \
    dbobjects/allowedservertable.cpp \
    dbobjects/blob.cpp \
    dbobjects/extrastring.cpp \
    dbobjects/idnumdescription.cpp \
    dbobjects/patient.cpp \
    dbobjects/patientidnum.cpp \
    dbobjects/patientidnumsorter.cpp \
    dbobjects/patientsorter.cpp \
    dbobjects/storedvar.cpp \
    diagnosis/diagnosissortfiltermodel.cpp \
    diagnosis/diagnosticcode.cpp \
    diagnosis/diagnosticcodeset.cpp \
    diagnosis/flatproxymodel.cpp \
    diagnosis/icd10.cpp \
    diagnosis/icd9cm.cpp \
    dialogs/logbox.cpp \
    dialogs/logmessagebox.cpp \
    dialogs/nvpchoicedialog.cpp \
    dialogs/pagepickerdialog.cpp \
    dialogs/passwordchangedialog.cpp \
    dialogs/passwordentrydialog.cpp \
    dialogs/progressbox.cpp \
    dialogs/scrollmessagebox.cpp \
    dialogs/soundtestdialog.cpp \
    dialogs/waitbox.cpp \
    graphics/buttonconfig.cpp \
    graphics/geometry.cpp \
    graphics/graphicsfunc.cpp \
    graphics/graphicspixmapitemwithopacity.cpp \
    graphics/imagefunc.cpp \
    graphics/linesegment.cpp \
    graphics/paintertranslaterotatecontext.cpp \
    graphics/penbrush.cpp \
    graphics/textconfig.cpp \
    layouts/boxlayouthfw.cpp \
    layouts/flowlayouthfw.cpp \
    layouts/gridlayouthfw.cpp \
    layouts/hboxlayouthfw.cpp \
    layouts/qtlayouthelpers.cpp \
    layouts/vboxlayouthfw.cpp \
    layouts/widgetitemhfw.cpp \
    lib/comparers.cpp \
    lib/containers.cpp \
    lib/convert.cpp \
    lib/css.cpp \
    lib/datetime.cpp \
    lib/debugfunc.cpp \
    lib/filefunc.cpp \
    lib/flagguard.cpp \
    lib/idpolicy.cpp \
    lib/layoutdumper.cpp \
    lib/margins.cpp \
    lib/nhs.cpp \
    lib/numericfunc.cpp \
    lib/openglfunc.cpp \
    lib/reentrydepthguard.cpp \
    lib/roman.cpp \
    lib/sizehelpers.cpp \
    lib/slowguiguard.cpp \
    lib/soundfunc.cpp \
    lib/stringfunc.cpp \
    lib/timerfunc.cpp \
    lib/uifunc.cpp \
    lib/version.cpp \
    main.cpp \
    maths/ccrandom.cpp \
    maths/countingcontainer.cpp \
    maths/dqrls.cpp \
    maths/eigenfunc.cpp \
    maths/endian.cpp \
    maths/glm.cpp \
    maths/linkfunctionfamily.cpp \
    maths/logisticdescriptives.cpp \
    maths/logisticregression.cpp \
    maths/mathfunc.cpp \
    maths/mlpackfunc.cpp \
    maths/statsfunc.cpp \
    menu/addictionmenu.cpp \
    menu/affectivemenu.cpp \
    menu/alltasksmenu.cpp \
    menu/anonymousmenu.cpp \
    menu/catatoniaepsemenu.cpp \
    menu/choosepatientmenu.cpp \
    menu/clinicalmenu.cpp \
    menu/clinicalsetsmenu.cpp \
    menu/cognitivemenu.cpp \
    menu/executivemenu.cpp \
    menu/globalmenu.cpp \
    menu/helpmenu.cpp \
    menu/mainmenu.cpp \
    menu/patientsummarymenu.cpp \
    menu/personalitymenu.cpp \
    menu/physicalillnessmenu.cpp \
    menu/psychosismenu.cpp \
    menu/researchmenu.cpp \
    menu/researchsetsmenu.cpp \
    menu/serviceevaluationmenu.cpp \
    menu/setmenucpftadrd.cpp \
    menu/setmenucpftperinatal.cpp \
    menu/setmenucpftpsychooncology.cpp \
    menu/setmenudeakin.cpp \
    menu/setmenufromlp.cpp \
    menu/setmenufromperinatal.cpp \
    menu/setmenukhandakerinsight.cpp \
    menu/setmenukhandakermojo.cpp \
    menu/setmenulynalliam.cpp \
    menu/setmenuobrien.cpp \
    menu/settingsmenu.cpp \
    menu/singletaskmenu.cpp \
    menu/testmenu.cpp \
    menu/whiskertestmenu.cpp \
    menu/widgettestmenu.cpp \
    menulib/choosepatientmenuitem.cpp \
    menulib/htmlinfowindow.cpp \
    menulib/htmlmenuitem.cpp \
    menulib/menuheader.cpp \
    menulib/menuitem.cpp \
    menulib/menuproxy.cpp \
    menulib/menuwindow.cpp \
    menulib/taskchainmenuitem.cpp \
    menulib/taskmenuitem.cpp \
    menulib/urlmenuitem.cpp \
    qcustomplot/qcustomplot.cpp \
    qobjects/cameraframegrabber.cpp \
    qobjects/debugeventwatcher.cpp \
    qobjects/flickcharm.cpp \
    qobjects/focuswatcher.cpp \
    qobjects/keypresswatcher.cpp \
    qobjects/nhsnumbervalidator.cpp \
    qobjects/shootabug.cpp \
    qobjects/showwatcher.cpp \
    qobjects/sizewatcher.cpp \
    qobjects/slownonguifunctioncaller.cpp \
    qobjects/strictdoublevalidator.cpp \
    qobjects/strictint64validator.cpp \
    qobjects/strictintvalidator.cpp \
    qobjects/strictuint64validator.cpp \
    qobjects/stylenofocusrect.cpp \
    qobjects/threadworker.cpp \
    questionnairelib/commonoptions.cpp \
    questionnairelib/dynamicquestionnaire.cpp \
    questionnairelib/mcqfunc.cpp \
    questionnairelib/mcqgridsubtitle.cpp \
    questionnairelib/namevalueoptions.cpp \
    questionnairelib/namevaluepair.cpp \
    questionnairelib/pagepickeritem.cpp \
    questionnairelib/quaudioplayer.cpp \
    questionnairelib/qubackground.cpp \
    questionnairelib/quboolean.cpp \
    questionnairelib/qubutton.cpp \
    questionnairelib/qucanvas.cpp \
    questionnairelib/qucountdown.cpp \
    questionnairelib/qudatetime.cpp \
    questionnairelib/qudiagnosticcode.cpp \
    questionnairelib/quelement.cpp \
    questionnairelib/questionnaire.cpp \
    questionnairelib/questionnairefunc.cpp \
    questionnairelib/questionnaireheader.cpp \
    questionnairelib/questionwithonefield.cpp \
    questionnairelib/questionwithtwofields.cpp \
    questionnairelib/quflowcontainer.cpp \
    questionnairelib/qugridcell.cpp \
    questionnairelib/qugridcontainer.cpp \
    questionnairelib/quheading.cpp \
    questionnairelib/quhorizontalcontainer.cpp \
    questionnairelib/quhorizontalline.cpp \
    questionnairelib/quimage.cpp \
    questionnairelib/qulineedit.cpp \
    questionnairelib/qulineeditdouble.cpp \
    questionnairelib/qulineeditint64.cpp \
    questionnairelib/qulineeditinteger.cpp \
    questionnairelib/qulineeditnhsnumber.cpp \
    questionnairelib/qulineedituint64.cpp \
    questionnairelib/qumcq.cpp \
    questionnairelib/qumcqgrid.cpp \
    questionnairelib/qumcqgriddouble.cpp \
    questionnairelib/qumcqgriddoublesignaller.cpp \
    questionnairelib/qumcqgridsignaller.cpp \
    questionnairelib/qumcqgridsingleboolean.cpp \
    questionnairelib/qumcqgridsinglebooleansignaller.cpp \
    questionnairelib/qumultipleresponse.cpp \
    questionnairelib/qupage.cpp \
    questionnairelib/quphoto.cpp \
    questionnairelib/qupickerinline.cpp \
    questionnairelib/qupickerpopup.cpp \
    questionnairelib/qusequencecontainerbase.cpp \
    questionnairelib/quslider.cpp \
    questionnairelib/quspacer.cpp \
    questionnairelib/quspinboxdouble.cpp \
    questionnairelib/quspinboxinteger.cpp \
    questionnairelib/qutext.cpp \
    questionnairelib/qutextedit.cpp \
    questionnairelib/quthermometer.cpp \
    questionnairelib/quthermometeritem.cpp \
    questionnairelib/quverticalcontainer.cpp \
    questionnairelib/quzoomcontainer.cpp \
    taskchains/khandakermojochain.cpp \
    tasklib/inittasks.cpp \
    tasklib/task.cpp \
    tasklib/taskchain.cpp \
    tasklib/taskfactory.cpp \
    tasklib/taskproxy.cpp \
    tasklib/taskregistrar.cpp \
    tasklib/tasksorter.cpp \
    tasks/ace3.cpp \
    tasks/aims.cpp \
    tasks/apeqcpftperinatal.cpp \
    tasks/apeqpt.cpp \
    tasks/asdas.cpp \
    tasks/audit.cpp \
    tasks/auditc.cpp \
    tasks/badls.cpp \
    tasks/bdi.cpp \
    tasks/bmi.cpp \
    tasks/bprs.cpp \
    tasks/bprse.cpp \
    tasks/cage.cpp \
    tasks/cape42.cpp \
    tasks/caps.cpp \
    tasks/cardinalexpdetthreshold.cpp \
    tasks/cardinalexpectationdetection.cpp \
    tasks/cbir.cpp \
    tasks/cecaq3.cpp \
    tasks/cesd.cpp \
    tasks/cesdr.cpp \
    tasks/cgi.cpp \
    tasks/cgii.cpp \
    tasks/cgisch.cpp \
    tasks/chit.cpp \
    tasks/cisr.cpp \
    tasks/ciwa.cpp \
    tasks/contactlog.cpp \
    tasks/copebrief.cpp \
    tasks/core10.cpp \
    tasks/cpftlpsdischarge.cpp \
    tasks/cpftlpsreferral.cpp \
    tasks/cpftlpsresetresponseclock.cpp \
    tasks/dad.cpp \
    tasks/das28.cpp \
    tasks/dast.cpp \
    tasks/deakins1healthreview.cpp \
    tasks/demoquestionnaire.cpp \
    tasks/demqol.cpp \
    tasks/demqolproxy.cpp \
    tasks/diagnosisicd10.cpp \
    tasks/diagnosisicd9cm.cpp \
    tasks/distressthermometer.cpp \
    tasks/elixhauserci.cpp \
    tasks/epds.cpp \
    tasks/eq5d5l.cpp \
    tasks/esspri.cpp \
    tasks/factg.cpp \
    tasks/fast.cpp \
    tasks/fft.cpp \
    tasks/frs.cpp \
    tasks/gad7.cpp \
    tasks/gaf.cpp \
    tasks/gbogpc.cpp \
    tasks/gbogras.cpp \
    tasks/gbogres.cpp \
    tasks/gds15.cpp \
    tasks/gmcpq.cpp \
    tasks/hads.cpp \
    tasks/hadsrespondent.cpp \
    tasks/hama.cpp \
    tasks/hamd.cpp \
    tasks/hamd7.cpp \
    tasks/honos.cpp \
    tasks/honos65.cpp \
    tasks/honosca.cpp \
    tasks/icd10depressive.cpp \
    tasks/icd10manic.cpp \
    tasks/icd10mixed.cpp \
    tasks/icd10schizophrenia.cpp \
    tasks/icd10schizotypal.cpp \
    tasks/icd10specpd.cpp \
    tasks/ided3d.cpp \
    tasks/iesr.cpp \
    tasks/ifs.cpp \
    tasks/irac.cpp \
    tasks/khandakerinsightmedical.cpp \
    tasks/khandakermojomedical.cpp \
    tasks/khandakermojomedicationtherapy.cpp \
    tasks/khandakermojosociodemographics.cpp \
    tasks/lynalliamlife.cpp \
    tasks/lynalliammedical.cpp \
    tasks/maas.cpp \
    tasks/mast.cpp \
    tasks/mdsupdrs.cpp \
    tasks/mfi20.cpp \
    tasks/moca.cpp \
    tasks/nart.cpp \
    tasks/npiq.cpp \
    tasks/ors.cpp \
    tasks/panss.cpp \
    tasks/patientsatisfaction.cpp \
    tasks/pbq.cpp \
    tasks/pcl5.cpp \
    tasks/pclc.cpp \
    tasks/pclm.cpp \
    tasks/pcls.cpp \
    tasks/pdss.cpp \
    tasks/perinatalpoem.cpp \
    tasks/photo.cpp \
    tasks/photosequence.cpp \
    tasks/phq15.cpp \
    tasks/phq9.cpp \
    tasks/progressnote.cpp \
    tasks/pswq.cpp \
    tasks/psychiatricclerking.cpp \
    tasks/qolbasic.cpp \
    tasks/qolsg.cpp \
    tasks/rand36.cpp \
    tasks/referrersatisfactiongen.cpp \
    tasks/referrersatisfactionspec.cpp \
    tasks/sfmpq2.cpp \
    tasks/shaps.cpp \
    tasks/slums.cpp \
    tasks/smast.cpp \
    tasks/srs.cpp \
    tasks/suppsp.cpp \
    tasks/swemwbs.cpp \
    tasks/wemwbs.cpp \
    tasks/wsas.cpp \
    tasks/ybocs.cpp \
    tasks/ybocssc.cpp \
    tasks/zbi12.cpp \
    taskxtra/cardinalexpdetcommon.cpp \
    taskxtra/cardinalexpdetrating.cpp \
    taskxtra/cardinalexpdetthresholdtrial.cpp \
    taskxtra/cardinalexpdettrial.cpp \
    taskxtra/cardinalexpdettrialgroupspec.cpp \
    taskxtra/diagnosisicd10item.cpp \
    taskxtra/diagnosisicd9cmitem.cpp \
    taskxtra/diagnosisitembase.cpp \
    taskxtra/diagnosistaskbase.cpp \
    taskxtra/gbocommon.cpp \
    taskxtra/ided3dexemplars.cpp \
    taskxtra/ided3dstage.cpp \
    taskxtra/ided3dtrial.cpp \
    taskxtra/khandakermojomedicationitem.cpp \
    taskxtra/khandakermojotherapyitem.cpp \
    taskxtra/kirbyrewardpair.cpp \
    taskxtra/kirbytrial.cpp \
    taskxtra/pclcommon.cpp \
    taskxtra/photosequencephoto.cpp \
    taskxtra/satisfactioncommon.cpp \
    version/camcopsversion.cpp \
    whisker/whiskerapi.cpp \
    whisker/whiskercallbackdefinition.cpp \
    whisker/whiskercallbackhandler.cpp \
    whisker/whiskerconnectionstate.cpp \
    whisker/whiskerconstants.cpp \
    whisker/whiskerdisplaycachewrapper.cpp \
    whisker/whiskerinboundmessage.cpp \
    whisker/whiskermanager.cpp \
    whisker/whiskeroutboundcommand.cpp \
    whisker/whiskerworker.cpp \
    widgets/adjustablepie.cpp \
    widgets/aspectratiopixmap.cpp \
    widgets/basewidget.cpp \
    widgets/booleanwidget.cpp \
    widgets/cameraqcamera.cpp \
    widgets/cameraqml.cpp \
    widgets/canvaswidget.cpp \
    widgets/clickablelabelnowrap.cpp \
    widgets/clickablelabelwordwrapwide.cpp \
    widgets/diagnosticcodeselector.cpp \
    widgets/fixedareahfwtestwidget.cpp \
    widgets/fixedaspectratiohfwtestwidget.cpp \
    widgets/fixednumblockshfwtestwidget.cpp \
    widgets/graphicsrectitemclickable.cpp \
    widgets/growingplaintextedit.cpp \
    widgets/growingtextedit.cpp \
    widgets/heightforwidthlistwidget.cpp \
    widgets/horizontalline.cpp \
    widgets/imagebutton.cpp \
    widgets/labelwordwrapwide.cpp \
    widgets/openablewidget.cpp \
    widgets/screenlikegraphicsview.cpp \
    widgets/spacer.cpp \
    widgets/svgwidgetclickable.cpp \
    widgets/thermometer.cpp \
    widgets/tickslider.cpp \
    widgets/treeviewcontroldelegate.cpp \
    widgets/treeviewproxystyle.cpp \
    widgets/verticalline.cpp \
    widgets/verticalscrollarea.cpp \
    widgets/verticalscrollareaviewport.cpp \
    tasks/ctqsf.cpp \
    tasks/kirby.cpp \
    common/languages.cpp \
    widgets/zoomablegraphicsview.cpp \
    widgets/zoomablewidget.cpp

HEADERS += \
    common/aliases_camcops.h \
    common/aliases_qt.h \
    common/appstrings.h \
    common/colourdefs.h \
    common/cssconst.h \
    common/dbconst.h \
    common/design_defines.h \
    common/dpi.h \
    common/globals.h \
    common/gui_defines.h \
    common/platform.h \
    common/preprocessor_aid.h \
    common/textconst.h \
    common/uiconst.h \
    common/urlconst.h \
    common/varconst.h \
    common/widgetconst.h \
    core/camcopsapp.h \
    core/networkmanager.h \
    crypto/cryptofunc.h \
    crypto/secureqbytearray.h \
    crypto/secureqstring.h \
    crypto/zallocator.h \
    db/ancillaryfunc.h \
    db/blobfieldref.h \
    db/databasemanager.h \
    db/databaseobject.h \
    db/databaseworkerthread.h \
    db/dbfunc.h \
    db/dbnestabletransaction.h \
    db/dbtransaction.h \
    db/dumpsql.h \
    db/field.h \
    db/fieldcreationplan.h \
    db/fieldref.h \
    db/queryresult.h \
    db/sqlargs.h \
    db/sqlcachedresult.h \
    db/sqlcipherdriver.h \
    db/sqlcipherhelpers.h \
    db/sqlcipherresult.h \
    db/sqlitepragmainfofield.h \
    db/threadedqueryrequest.h \
    db/whereconditions.h \
    db/whichdb.h \
    dbobjects/allowedservertable.h \
    dbobjects/blob.h \
    dbobjects/extrastring.h \
    dbobjects/idnumdescription.h \
    dbobjects/patient.h \
    dbobjects/patientidnum.h \
    dbobjects/patientidnumsorter.h \
    dbobjects/patientsorter.h \
    dbobjects/storedvar.h \
    diagnosis/diagnosissortfiltermodel.h \
    diagnosis/diagnosticcode.h \
    diagnosis/diagnosticcodeset.h \
    diagnosis/flatproxymodel.h \
    diagnosis/icd10.h \
    diagnosis/icd9cm.h \
    dialogs/logbox.h \
    dialogs/logmessagebox.h \
    dialogs/nvpchoicedialog.h \
    dialogs/pagepickerdialog.h \
    dialogs/passwordchangedialog.h \
    dialogs/passwordentrydialog.h \
    dialogs/progressbox.h \
    dialogs/scrollmessagebox.h \
    dialogs/soundtestdialog.h \
    dialogs/waitbox.h \
    graphics/buttonconfig.h \
    graphics/geometry.h \
    graphics/graphicsfunc.h \
    graphics/graphicspixmapitemwithopacity.h \
    graphics/imagefunc.h \
    graphics/linesegment.h \
    graphics/paintertranslaterotatecontext.h \
    graphics/penbrush.h \
    graphics/textconfig.h \
    layouts/boxlayouthfw.h \
    layouts/flowlayouthfw.h \
    layouts/gridlayouthfw.h \
    layouts/hboxlayouthfw.h \
    layouts/layouts.h \
    layouts/qtlayouthelpers.h \
    layouts/vboxlayouthfw.h \
    layouts/widgetitemhfw.h \
    lib/cloneable.h \
    lib/comparers.h \
    lib/containers.h \
    lib/convert.h \
    lib/css.h \
    lib/datetime.h \
    lib/debugfunc.h \
    lib/filefunc.h \
    lib/flagguard.h \
    lib/idpolicy.h \
    lib/layoutdumper.h \
    lib/margins.h \
    lib/nhs.h \
    lib/numericfunc.h \
    lib/openglfunc.h \
    lib/reentrydepthguard.h \
    lib/roman.h \
    lib/sizehelpers.h \
    lib/slowguiguard.h \
    lib/soundfunc.h \
    lib/stringfunc.h \
    lib/timerfunc.h \
    lib/uifunc.h \
    lib/version.h \
    maths/ccrandom.h \
    maths/countingcontainer.h \
    maths/dqrls.h \
    maths/eigenfunc.h \
    maths/endian.h \
    maths/floatbits.h \
    maths/floatingpoint.h \
    maths/glm.h \
    maths/ieee754.h \
    maths/include_eigen_core.h \
    maths/include_eigen_dense.h \
    maths/linkfunctionfamily.h \
    maths/logisticdescriptives.h \
    maths/logisticregression.h \
    maths/mathfunc.h \
    maths/mlpackfunc.h \
    maths/statsfunc.h \
    menu/addictionmenu.h \
    menu/affectivemenu.h \
    menu/alltasksmenu.h \
    menu/anonymousmenu.h \
    menu/catatoniaepsemenu.h \
    menu/choosepatientmenu.h \
    menu/clinicalmenu.h \
    menu/clinicalsetsmenu.h \
    menu/cognitivemenu.h \
    menu/executivemenu.h \
    menu/globalmenu.h \
    menu/helpmenu.h \
    menu/mainmenu.h \
    menu/patientsummarymenu.h \
    menu/personalitymenu.h \
    menu/physicalillnessmenu.h \
    menu/psychosismenu.h \
    menu/researchmenu.h \
    menu/researchsetsmenu.h \
    menu/serviceevaluationmenu.h \
    menu/setmenucpftadrd.h \
    menu/setmenucpftperinatal.h \
    menu/setmenucpftpsychooncology.h \
    menu/setmenudeakin.h \
    menu/setmenufromlp.h \
    menu/setmenufromperinatal.h \
    menu/setmenukhandakerinsight.h \
    menu/setmenukhandakermojo.h \
    menu/setmenulynalliam.h \
    menu/setmenuobrien.h \
    menu/settingsmenu.h \
    menu/singletaskmenu.h \
    menu/testmenu.h \
    menu/whiskertestmenu.h \
    menu/widgettestmenu.h \
    menulib/choosepatientmenuitem.h \
    menulib/htmlinfowindow.h \
    menulib/htmlmenuitem.h \
    menulib/menuheader.h \
    menulib/menuitem.h \
    menulib/menuproxy.h \
    menulib/menuwindow.h \
    menulib/taskchainmenuitem.h \
    menulib/taskmenuitem.h \
    menulib/urlmenuitem.h \
    qcustomplot/qcustomplot.h \
    qobjects/cameraframegrabber.h \
    qobjects/debugeventwatcher.h \
    qobjects/flickcharm.h \
    qobjects/focuswatcher.h \
    qobjects/keypresswatcher.h \
    qobjects/nhsnumbervalidator.h \
    qobjects/shootabug.h \
    qobjects/showwatcher.h \
    qobjects/sizewatcher.h \
    qobjects/slownonguifunctioncaller.h \
    qobjects/strictdoublevalidator.h \
    qobjects/strictint64validator.h \
    qobjects/strictintvalidator.h \
    qobjects/strictuint64validator.h \
    qobjects/stylenofocusrect.h \
    qobjects/threadworker.h \
    questionnairelib/commonoptions.h \
    questionnairelib/dynamicquestionnaire.h \
    questionnairelib/mcqfunc.h \
    questionnairelib/mcqgridsubtitle.h \
    questionnairelib/namevalueoptions.h \
    questionnairelib/namevaluepair.h \
    questionnairelib/pagepickeritem.h \
    questionnairelib/quaudioplayer.h \
    questionnairelib/qubackground.h \
    questionnairelib/quboolean.h \
    questionnairelib/qubutton.h \
    questionnairelib/qucanvas.h \
    questionnairelib/qucountdown.h \
    questionnairelib/qudatetime.h \
    questionnairelib/qudiagnosticcode.h \
    questionnairelib/quelement.h \
    questionnairelib/questionnaire.h \
    questionnairelib/questionnairefunc.h \
    questionnairelib/questionnaireheader.h \
    questionnairelib/questionwithonefield.h \
    questionnairelib/questionwithtwofields.h \
    questionnairelib/quflowcontainer.h \
    questionnairelib/qugridcell.h \
    questionnairelib/qugridcontainer.h \
    questionnairelib/quheading.h \
    questionnairelib/quhorizontalcontainer.h \
    questionnairelib/quhorizontalline.h \
    questionnairelib/quimage.h \
    questionnairelib/qulineedit.h \
    questionnairelib/qulineeditdouble.h \
    questionnairelib/qulineeditint64.h \
    questionnairelib/qulineeditinteger.h \
    questionnairelib/qulineeditnhsnumber.h \
    questionnairelib/qulineedituint64.h \
    questionnairelib/qumcq.h \
    questionnairelib/qumcqgrid.h \
    questionnairelib/qumcqgriddouble.h \
    questionnairelib/qumcqgriddoublesignaller.h \
    questionnairelib/qumcqgridsignaller.h \
    questionnairelib/qumcqgridsingleboolean.h \
    questionnairelib/qumcqgridsinglebooleansignaller.h \
    questionnairelib/qumultipleresponse.h \
    questionnairelib/qupage.h \
    questionnairelib/quphoto.h \
    questionnairelib/qupickerinline.h \
    questionnairelib/qupickerpopup.h \
    questionnairelib/qusequencecontainerbase.h \
    questionnairelib/quslider.h \
    questionnairelib/quspacer.h \
    questionnairelib/quspinboxdouble.h \
    questionnairelib/quspinboxinteger.h \
    questionnairelib/qutext.h \
    questionnairelib/qutextedit.h \
    questionnairelib/quthermometer.h \
    questionnairelib/quthermometeritem.h \
    questionnairelib/quverticalcontainer.h \
    questionnairelib/quzoomcontainer.h \
    taskchains/khandakermojochain.h \
    tasklib/inittasks.h \
    tasklib/task.h \
    tasklib/taskchain.h \
    tasklib/taskfactory.h \
    tasklib/taskproxy.h \
    tasklib/taskregistrar.h \
    tasklib/tasksorter.h \
    tasks/ace3.h \
    tasks/aims.h \
    tasks/apeqcpftperinatal.h \
    tasks/apeqpt.h \
    tasks/asdas.h \
    tasks/audit.h \
    tasks/auditc.h \
    tasks/badls.h \
    tasks/bdi.h \
    tasks/bmi.h \
    tasks/bprs.h \
    tasks/bprse.h \
    tasks/cage.h \
    tasks/cape42.h \
    tasks/caps.h \
    tasks/cardinalexpdetthreshold.h \
    tasks/cardinalexpectationdetection.h \
    tasks/cbir.h \
    tasks/cecaq3.h \
    tasks/cesd.h \
    tasks/cesdr.h \
    tasks/cgi.h \
    tasks/cgii.h \
    tasks/cgisch.h \
    tasks/chit.h \
    tasks/cisr.h \
    tasks/ciwa.h \
    tasks/contactlog.h \
    tasks/copebrief.h \
    tasks/core10.h \
    tasks/cpftlpsdischarge.h \
    tasks/cpftlpsreferral.h \
    tasks/cpftlpsresetresponseclock.h \
    tasks/dad.h \
    tasks/das28.h \
    tasks/dast.h \
    tasks/deakins1healthreview.h \
    tasks/demoquestionnaire.h \
    tasks/demqol.h \
    tasks/demqolproxy.h \
    tasks/diagnosisicd10.h \
    tasks/diagnosisicd9cm.h \
    tasks/distressthermometer.h \
    tasks/elixhauserci.h \
    tasks/epds.h \
    tasks/eq5d5l.h \
    tasks/esspri.h \
    tasks/factg.h \
    tasks/fast.h \
    tasks/fft.h \
    tasks/frs.h \
    tasks/gad7.h \
    tasks/gaf.h \
    tasks/gbogpc.h \
    tasks/gbogras.h \
    tasks/gbogres.h \
    tasks/gds15.h \
    tasks/gmcpq.h \
    tasks/hads.h \
    tasks/hadsrespondent.h \
    tasks/hama.h \
    tasks/hamd.h \
    tasks/hamd7.h \
    tasks/honos.h \
    tasks/honos65.h \
    tasks/honosca.h \
    tasks/icd10depressive.h \
    tasks/icd10manic.h \
    tasks/icd10mixed.h \
    tasks/icd10schizophrenia.h \
    tasks/icd10schizotypal.h \
    tasks/icd10specpd.h \
    tasks/ided3d.h \
    tasks/iesr.h \
    tasks/ifs.h \
    tasks/irac.h \
    tasks/khandakerinsightmedical.h \
    tasks/khandakermojomedical.h \
    tasks/khandakermojomedicationtherapy.h \
    tasks/khandakermojosociodemographics.h \
    tasks/lynalliamlife.h \
    tasks/lynalliammedical.h \
    tasks/maas.h \
    tasks/mast.h \
    tasks/mdsupdrs.h \
    tasks/mfi20.h \
    tasks/moca.h \
    tasks/nart.h \
    tasks/npiq.h \
    tasks/ors.h \
    tasks/panss.h \
    tasks/patientsatisfaction.h \
    tasks/pbq.h \
    tasks/pcl5.h \
    tasks/pclc.h \
    tasks/pclm.h \
    tasks/pcls.h \
    tasks/pdss.h \
    tasks/perinatalpoem.h \
    tasks/photo.h \
    tasks/photosequence.h \
    tasks/phq15.h \
    tasks/phq9.h \
    tasks/progressnote.h \
    tasks/pswq.h \
    tasks/psychiatricclerking.h \
    tasks/qolbasic.h \
    tasks/qolsg.h \
    tasks/rand36.h \
    tasks/referrersatisfactiongen.h \
    tasks/referrersatisfactionspec.h \
    tasks/sfmpq2.h \
    tasks/shaps.h \
    tasks/slums.h \
    tasks/smast.h \
    tasks/srs.h \
    tasks/suppsp.h \
    tasks/swemwbs.h \
    tasks/wemwbs.h \
    tasks/wsas.h \
    tasks/ybocs.h \
    tasks/ybocssc.h \
    tasks/zbi12.h \
    taskxtra/cardinalexpdetcommon.h \
    taskxtra/cardinalexpdetrating.h \
    taskxtra/cardinalexpdetthresholdtrial.h \
    taskxtra/cardinalexpdettrial.h \
    taskxtra/cardinalexpdettrialgroupspec.h \
    taskxtra/diagnosisicd10item.h \
    taskxtra/diagnosisicd9cmitem.h \
    taskxtra/diagnosisitembase.h \
    taskxtra/diagnosistaskbase.h \
    taskxtra/gbocommon.h \
    taskxtra/ided3dexemplars.h \
    taskxtra/ided3dstage.h \
    taskxtra/ided3dtrial.h \
    taskxtra/khandakermojomedicationitem.h \
    taskxtra/khandakermojotherapyitem.h \
    taskxtra/kirbyrewardpair.h \
    taskxtra/kirbytrial.h \
    taskxtra/pclcommon.h \
    taskxtra/photosequencephoto.h \
    taskxtra/satisfactioncommon.h \
    version/camcopsversion.h \
    whisker/whiskerapi.h \
    whisker/whiskercallbackdefinition.h \
    whisker/whiskercallbackhandler.h \
    whisker/whiskerconnectionstate.h \
    whisker/whiskerconstants.h \
    whisker/whiskerdisplaycachewrapper.h \
    whisker/whiskerinboundmessage.h \
    whisker/whiskermanager.h \
    whisker/whiskeroutboundcommand.h \
    whisker/whiskerworker.h \
    widgets/adjustablepie.h \
    widgets/aspectratiopixmap.h \
    widgets/basewidget.h \
    widgets/booleanwidget.h \
    widgets/cameraqcamera.h \
    widgets/cameraqml.h \
    widgets/canvaswidget.h \
    widgets/clickablelabelnowrap.h \
    widgets/clickablelabelwordwrapwide.h \
    widgets/diagnosticcodeselector.h \
    widgets/fixedareahfwtestwidget.h \
    widgets/fixedaspectratiohfwtestwidget.h \
    widgets/fixednumblockshfwtestwidget.h \
    widgets/graphicsrectitemclickable.h \
    widgets/growingplaintextedit.h \
    widgets/growingtextedit.h \
    widgets/heightforwidthlistwidget.h \
    widgets/horizontalline.h \
    widgets/imagebutton.h \
    widgets/labelwordwrapwide.h \
    widgets/openablewidget.h \
    widgets/screenlikegraphicsview.h \
    widgets/spacer.h \
    widgets/svgwidgetclickable.h \
    widgets/thermometer.h \
    widgets/tickslider.h \
    widgets/treeviewcontroldelegate.h \
    widgets/treeviewproxystyle.h \
    widgets/verticalline.h \
    widgets/verticalscrollarea.h \
    widgets/verticalscrollareaviewport.h \
    tasks/ctqsf.h \
    tasks/kirby.h \
    common/languages.h \
    widgets/zoomablegraphicsview.h \
    widgets/zoomablewidget.h

# DISTFILES/OTHER_FILES appear in the Qt Creator editing tree. Otherwise,
# there's not much practical impact:
# https://stackoverflow.com/questions/38102160/in-qt-when-should-you-use-resources-vs-distfiles-vs-other-files
# They are separate from resource files (see RESOURCES above).
OTHER_FILES += \
    camcops_windows_innosetup.iss \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/drawable-ldpi/icon.png \
    android/res/values/libs.xml \
    notes/compilation_android.txt \
    notes/compilation_linux.txt \
    notes/compilation_windows.txt \
    notes/database_performance.txt \
    notes/glm_calculations.R \
    notes/layout_notes.txt \
    notes/overall_design.txt \
    notes/qt_notes.txt \
    notes/string_formats.txt \
    tools/build_qt.py \
    tools/chord.py \
    tools/cppclean_all.sh \
    tools/decrypt_sqlcipher.py \
    tools/encrypt_sqlcipher.py \
    tools/open_sqlcipher.py \
    windows/camcops.rc

TRANSLATIONS = \
    translations/camcops_da_DK.ts


message("--- CamCOPS qmake finishing.")
