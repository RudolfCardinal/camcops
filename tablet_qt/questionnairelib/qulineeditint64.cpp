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

#include "qulineeditint64.h"
#include "qobjects/strictint64validator.h"


QuLineEditInt64::QuLineEditInt64(FieldRefPtr fieldref,
                                 const bool allow_empty) :
    QuLineEditInt64(fieldref,
                    std::numeric_limits<qint64>::min(),
                    std::numeric_limits<qint64>::max(),
                    allow_empty)
{
}


QuLineEditInt64::QuLineEditInt64(FieldRefPtr fieldref,
                                 const qint64 minimum,
                                 const qint64 maximum,
                                 const bool allow_empty) :
    QuLineEdit(fieldref),
    m_minimum(minimum),
    m_maximum(maximum),
    m_allow_empty(allow_empty)
{
    setHint(QString("integer, range %1 to %2").arg(m_minimum).arg(m_maximum));
}


void QuLineEditInt64::extraLineEditCreation(QLineEdit* editor)
{
    editor->setValidator(new StrictInt64Validator(m_minimum, m_maximum,
                                                  m_allow_empty, this));
    editor->setInputMethodHints(Qt::ImhFormattedNumbersOnly);
}
