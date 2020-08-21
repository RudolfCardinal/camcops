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
#include <QStringList>


namespace stringfunc {

// ============================================================================
// Basic string formatting
// ============================================================================

// Builds "<prefix><num><suffix>".
QString strnum(const QString& prefix, int num, const QString& suffix = "");

// ============================================================================
// Make sequences of strings
// ============================================================================

// Returns a list of "<prefix><num>" for "num" in the range [first, last].
// Example: stringSequence("q", 1, 3) -> {"q1", "q2", "q3"}.
QStringList strseq(const QString& prefix, int first, int last);

// Returns a list of "<prefix><num><suffix>" for "num" in "numbers".
QStringList strnumlist(const QString& prefix, const QVector<int>& numbers,
                       const QString& suffix = "");

// Returns a list of "<prefix><num><suffix>" for "num" in [first, last].
QStringList strseq(const QString& prefix, int first, int last,
                   const QString& suffix);

// Returns a list of "<prefix><num><suffix>" for "num" in [first, last] and
// for "suffix" in suffixes.
QStringList strseq(const QString& prefix, int first, int last,
                   const QStringList& suffixes);

// Returns a list of "<prefix><num><suffix>" for "prefix" in "prefixes" and
// for "num" in [first, last].
QStringList strseq(const QStringList& prefixes, int first, int last);

// Returns a list of "<prefix><num><suffix>" for "prefix" in "prefixes",
// for "num" in [first, last], and for "suffix" in "suffixes".
QStringList strseq(const QStringList& prefixes, int first, int last,
                   const QStringList& suffixes);

// ============================================================================
// HTML processing
// ============================================================================

// Returns HTML bold: "<b>str</b>"
QString bold(const QString& str);

// Returns HTML bold: "<b>x</b>"
QString bold(int x);

// Returns HTML hyperlink: '<a href="url">text</a>'
QString a(const QString& url, const QString& text);

// Returns HTML hyperlink: '<a href="url_and_text">url_and_text</a>'
QString a(const QString& url_and_text);

// Returns lines joined with "<br>".
QString joinHtmlLines(const QStringList& lines);

// Converts a string from LF (\n) to "<br>".
// If convert_embedded_literals is true, also converts a literal "\n" (two
// characters, backslash n) to "<br>".
QString& toHtmlLinebreaks(QString& str,
                          bool convert_embedded_literals = true);

// Returns "<name><separator><b><value></b><suffix>" (where "<b>" and "</b>"
// are literals and the others variables...!).
QString standardResult(const QString& name,
                       const QString& value,
                       const QString& separator = ": ",
                       const QString& suffix = ".");

// ============================================================================
// Other string processing
// ============================================================================

// Modifies str, replacing the first instance of "from" with "to", and
// returning str again.
QString& replaceFirst(QString& str, const QString& from, const QString& to);

// Replaces LF (\n) characters with the ↵ character.
QString stylizeNewlines(const QString& str, bool stylize = true);

// Ensures the string is no longer than max_len (replacing the end with suffix
// if required) and calling stylizeNewlines() on it.
QString abbreviate(const QString& str, int max_len = 255,
                   bool stylize_newlines = true,
                   const QString& suffix = "...");

}  // namespace stringfunc
