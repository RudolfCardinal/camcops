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

#include "strictintvalidator.h"
#include "lib/numericfunc.h"


StrictIntValidator::StrictIntValidator(const int bottom, const int top,
                                       const bool allow_empty,
                                       QObject* parent) :
    QIntValidator(bottom, top, parent),
    m_allow_empty(allow_empty)
{
    if (top < bottom) {  // user has supplied them backwards
        setRange(top, bottom);  // reverse the range
    }
}



QValidator::State StrictIntValidator::validate(QString& s, int& pos) const
{
    Q_UNUSED(pos)
    return numeric::validateInteger(s, locale(), bottom(), top(),
                                    m_allow_empty);
}
