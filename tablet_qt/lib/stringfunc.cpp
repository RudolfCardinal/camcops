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

#include "stringfunc.h"
#include <QVector>


namespace stringfunc {

// ============================================================================
// Basic string formatting
// ============================================================================

QString strnum(const QString& prefix, const int num, const QString& suffix)
{
    return prefix + QString::number(num) + suffix;
}


QStringList strnumlist(const QString& prefix, const QVector<int>& numbers,
                       const QString& suffix)
{
    QStringList strings;
    for (auto num : numbers) {
        strings.append(strnum(prefix, num, suffix));
    }
    return strings;
}


// ============================================================================
// Make sequences of strings
// ============================================================================

QStringList strseq(const QString& prefix, const int first, const int last)
{
    Q_ASSERT(first >= 0 && last >= 0 && first <= last);
    QStringList list;
    for (int i = first; i <= last; ++i) {
        list.append(strnum(prefix, i));
    }
    return list;
}


QStringList strseq(const QString& prefix, const int first, const int last,
                   const QStringList& suffixes)
{
    Q_ASSERT(first >= 0 && last >= 0 && first <= last);
    QStringList list;
    for (int i = first; i <= last; ++i) {
        for (const QString& suffix : suffixes) {
            list.append(strnum(prefix, i, suffix));
        }
    }
    return list;
}


QStringList strseq(const QString& prefix, int first, int last,
                   const QString& suffix)
{
    Q_ASSERT(first >= 0 && last >= 0 && first <= last);
    QStringList list;
    for (int i = first; i <= last; ++i) {
        list.append(strnum(prefix, i, suffix));
    }
    return list;
}


QStringList strseq(const QStringList& prefixes,
                   const int first, const int last)
{
    Q_ASSERT(first >= 0 && last >= 0 && first <= last);
    QStringList list;
    for (const QString& prefix : prefixes) {
        for (int i = first; i <= last; ++i) {
            list.append(strnum(prefix, i));
        }
    }
    return list;
}


QStringList strseq(const QStringList& prefixes,
                   const int first, const int last,
                   const QStringList& suffixes)
{
    Q_ASSERT(first >= 0 && last >= 0 && first <= last);
    QStringList list;
    for (const QString& prefix : prefixes) {
        for (int i = first; i <= last; ++i) {
            for (const QString& suffix : suffixes) {
                list.append(strnum(prefix, i, suffix));
            }
        }
    }
    return list;
}


// ============================================================================
// HTML processing
// ============================================================================

QString bold(const QString& str)
{
    return QString("<b>%1</b>").arg(str);
}


QString bold(const int x)
{
    return QString("<b>%1</b>").arg(x);
}


QString a(const QString& url, const QString& text)
{
    return QString("<a href=\"%1\">%2</a>").arg(url, text);
}


QString a(const QString& url_and_text)
{
    return a(url_and_text, url_and_text);
}


QString joinHtmlLines(const QStringList& lines)
{
    return lines.join("<br>");
}


QString& toHtmlLinebreaks(QString& str, const bool convert_embedded_literals)
{
    str.replace("\n", "<br>");
    if (convert_embedded_literals) {
        str.replace("\\n", "<br>");
    }
    return str;
}


QString standardResult(const QString& name,
                       const QString& value,
                       const QString& separator,
                       const QString& suffix)
{
    return QString("%1%2<b>%3</b>%4").arg(name, separator, value, suffix);
}


// ============================================================================
// Other string processing
// ============================================================================

QString& replaceFirst(QString& str, const QString& from, const QString& to)
{
    // Replaces in situ
    return str.replace(str.indexOf(from), from.length(), to);
}


const QString STYLIZED_NEWLINE("↵");  // ⏎


QString stylizeNewlines(const QString& str, const bool stylize)
{
    if (!stylize) {
        return str;
    }
    QString nlstr = str;
    nlstr.replace("\n", STYLIZED_NEWLINE);
    return nlstr;
}


QString abbreviate(const QString& str,
                   const int max_len, const bool stylize_newlines,
                   const QString& suffix)
{
    if (str.length() <= max_len) {
        return stylizeNewlines(str, stylize_newlines);
    }
    const int fragment_len = max_len - suffix.length();
    return stylizeNewlines(str.left(fragment_len) + suffix, stylize_newlines);
}


}  // namespace stringfunc
