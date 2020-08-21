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

#include "sqlargs.h"
#include "lib/convert.h"
using convert::SQUOTE;
using convert::QMARK;
using convert::toSqlLiteral;


QString SqlArgs::literalForDebuggingOnly() const
{
    QString result;
    const int nchars = sql.length();
    const int nargs = args.length();
    bool in_quote = false;
    int argidx = 0;
    for (int i = 0; i < nchars; ++i) {
        const QChar c = sql.at(i);
        if (c == SQUOTE) {
            // Found a quote
            in_quote = !in_quote;
            result += c;
        } else if (!in_quote && c == QMARK) {
            // Found a placeholder
            if (argidx < nargs) {
                result += toSqlLiteral(args.at(argidx++));
            } else {
                // Bad SQL; #placeholders > #args
                result += QMARK;
            }
        } else {
            // Anything else
            result += c;
        }
    }
    return result;
}


QDebug operator<<(QDebug debug, const SqlArgs& s)
{
    debug.nospace() << "SqlArgs(sql=" << s.sql << ", args=" << s.args << ")";
    return debug;
}
