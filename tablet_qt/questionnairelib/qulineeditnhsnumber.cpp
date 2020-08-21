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

#include "qulineeditnhsnumber.h"
#include <QLineEdit>
#include "qobjects/nhsnumbervalidator.h"


QuLineEditNHSNumber::QuLineEditNHSNumber(FieldRefPtr fieldref,
                                         bool allow_empty) :
    QuLineEditInt64(fieldref, allow_empty)
{
    setHint("NHS number (10-digit integer with checksum)");
}


void QuLineEditNHSNumber::extraLineEditCreation(QLineEdit* editor)
{
    editor->setValidator(new NHSNumberValidator(m_allow_empty, this));
    editor->setInputMethodHints(Qt::ImhFormattedNumbersOnly);
}
