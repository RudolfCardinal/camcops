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

#include "qulineedituint64.h"
#include <QDebug>
#include "qobjects/strictuint64validator.h"


QuLineEditUInt64::QuLineEditUInt64(FieldRefPtr fieldref,
                                   const bool allow_empty) :
    QuLineEditUInt64(fieldref,
                     std::numeric_limits<quint64>::min(),
                     std::numeric_limits<quint64>::max(),
                     allow_empty)
{
}


QuLineEditUInt64::QuLineEditUInt64(FieldRefPtr fieldref,
                                   const quint64 minimum,
                                   const quint64 maximum,
                                   const bool allow_empty) :
    QuLineEdit(fieldref),
    m_minimum(minimum),
    m_maximum(maximum),
    m_allow_empty(allow_empty)
{
    qWarning() << "SQLite v3 does not properly support unsigned 64-bit "
                  "integers (https://www.sqlite.org/datatype3.html); "
                  "use signed if possible";
    // see also http://jakegoulding.com/blog/2011/02/06/sqlite-64-bit-integers/
    setHint(QString("integer, range %1 to %2").arg(m_minimum).arg(m_maximum));
}


void QuLineEditUInt64::extraLineEditCreation(QLineEdit* editor)
{
    editor->setValidator(new StrictUInt64Validator(m_minimum, m_maximum,
                                                   m_allow_empty, this));
    editor->setInputMethodHints(Qt::ImhFormattedNumbersOnly);
}
