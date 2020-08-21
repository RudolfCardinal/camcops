/*
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
*/

#pragma once

// Specific checks for compilation environments that need special workarounds.

#include <QtGlobal>  // for QT_VERSION

// ============================================================================
// Printing preprocessor variables
// ============================================================================
// https://stackoverflow.com/questions/1204202/is-it-possible-to-print-a-preprocessor-variable-in-c
#define PREPROCESSOR_STRING2(x) #x
#define PREPROCESSOR_STRING(x) PREPROCESSOR_STRING2(x)


// ============================================================================
// Compiler detection
// ============================================================================
// Note that Clang also defines __GNUC__, but also defines __clang__.
// GCC defines __GNUC__.
// Visual C++ defines _MSC_VER.

#if defined __clang__  // NB defined in Qt Creator; put this first for that reason
    #define COMPILER_IS_CLANG
#elif defined __GNUC__  // __GNUC__ is defined for GCC and clang
    #define COMPILER_IS_GCC
    #if __GNUC__ >= 7  // gcc >= 7.0
        #define GCC_AT_LEAST_7
    #endif
#elif defined _MSC_VER
    #define COMPILER_IS_VISUAL_CPP
#endif


// ============================================================================
// COMPILER_WANTS_EXPLICIT_LAMBDA_CAPTURES
// ============================================================================
// https://stackoverflow.com/questions/43467095/why-is-a-const-variable-sometimes-not-required-to-be-captured-in-a-lambda

#ifdef COMPILER_IS_VISUAL_CPP
    #define COMPILER_WANTS_EXPLICIT_LAMBDA_CAPTURES
#endif


// ============================================================================
// COMPILER_WANTS_RETURN_AFTER_NORETURN
// ============================================================================
// - If compiler pays attention to "[[ noreturn ]]", you get e.g.
//   - "'return' will never be executed".
// - clang pays attention to this. Other things seem not to.

#ifndef COMPILER_IS_CLANG
    #define COMPILER_WANTS_RETURN_AFTER_NORETURN
#endif


// ============================================================================
// COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
// ============================================================================
// - If default label is present, and compiler doesn't want it, you get e.g.
//   "default label in switch which covers all enumeration values"
// - If default label is absent, and the compiler wants it, you get e.g.
//   - "control reaches end of non-void function"
//   - "this statement may fall through"
// - clang-tidy warns about the superfluous default
//   - https://softwareengineering.stackexchange.com/questions/179269/why-does-clang-llvm-warn-me-about-using-default-in-a-switch-statement-where-all
// - The "// NOLINT" comment does not suppress clang-tidy in Qt Creator.

#ifndef COMPILER_IS_CLANG
    #define COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
#endif


// ============================================================================
// GCC_HAS_WARNING_INT_IN_BOOL_CONTEXT
// ============================================================================

#ifdef GCC_AT_LEAST_7
    // https://www.gnu.org/software/gcc/gcc-7/changes.html
    #define GCC_HAS_WARNING_INT_IN_BOOL_CONTEXT
#endif

// No need to test "#ifdef __GNUC__" first; an undefined preprocessor constant
// evalutes to 0 when tested with "#if";
// https://stackoverflow.com/questions/5085392/what-is-the-value-of-an-undefined-constant-used-in-if


// ============================================================================
// DISABLE_GCC_DATE_TIME_MACRO_WARNING
// DISABLE_CLANG_DATE_TIME_MACRO_WARNING
// ============================================================================
// "expansion of date or time macro is not reproducible"

#ifdef COMPILER_IS_GCC  // Running proper GCC
    #define DISABLE_GCC_DATE_TIME_MACRO_WARNING
#endif
#ifdef COMPILER_IS_CLANG
    #define DISABLE_CLANG_DATE_TIME_MACRO_WARNING
#endif


// ============================================================================
// QT_WORKAROUND_BUG_68889
// ============================================================================
// See https://bugreports.qt.io/browse/QTBUG-68889

#ifdef Q_OS_ANDROID
    // #pragma message "QT_VERSION = " PREPROCESSOR_STRING(QT_VERSION)
    #if QT_VERSION == ((5 << 16) | (12 << 8) | (0))
        // Qt version 5.12.0
        #define QT_WORKAROUND_BUG_68889
        // See https://bugreports.qt.io/browse/QTBUG-68889
        // Only seems to affect Android builds (Ubuntu, Arch Linux OK).
    #endif
#endif


// ============================================================================
// VISIBLE_SYMBOL
// ============================================================================
// To prevent main() being hidden when "-fvisibility=hidden" is enabled!
// See camcops.pro, changelog.rst, and:
// - http://gcc.gnu.org/wiki/Visibility
// - https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
// - https://clang.llvm.org/docs/LTOVisibility.html
// - https://docs.microsoft.com/en-us/cpp/cpp/dllexport-dllimport?view=vs-2019

#if defined COMPILER_IS_CLANG || defined COMPILER_IS_GCC
    #define VISIBLE_SYMBOL __attribute__ ((visibility ("default")))
#elif defined COMPILER_IS_VISUAL_CPP
    #define VISIBLE_SYMBOL __declspec(dllexport)
#else
    #error "Don't know how to enforce symbol visibility for this compiler!"
#endif
