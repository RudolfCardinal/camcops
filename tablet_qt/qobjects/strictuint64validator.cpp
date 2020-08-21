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

#include "strictuint64validator.h"
#include <algorithm>
#include "lib/numericfunc.h"


StrictUInt64Validator::StrictUInt64Validator(const bool allow_empty,
                                             QObject* parent) :
    QValidator(parent),
    m_allow_empty(allow_empty)
{
    m_b = 0;
    m_t = std::numeric_limits<quint64>::max();
}


StrictUInt64Validator::StrictUInt64Validator(quint64 bottom,
                                             quint64 top,
                                             const bool allow_empty,
                                             QObject* parent) :
    QValidator(parent),
    m_allow_empty(allow_empty)
{
    if (top < bottom) {  // user has supplied them backwards
        std::swap(bottom, top);
    }
    m_b = bottom;
    m_t = top;
}


StrictUInt64Validator::~StrictUInt64Validator()
{
}


QValidator::State StrictUInt64Validator::validate(QString& s, int& pos) const
{
    Q_UNUSED(pos)
    return numeric::validateInteger(s, locale(), bottom(), top(),
                                    m_allow_empty);
}


void StrictUInt64Validator::setBottom(const quint64 bottom)
{
    setRange(bottom, top());
}


void StrictUInt64Validator::setTop(const quint64 top)
{
    setRange(bottom(), top);
}


void StrictUInt64Validator::setRange(const quint64 bottom, const quint64 top)
{
    bool rangeChanged = false;
    if (m_b != bottom) {
        m_b = bottom;
        rangeChanged = true;
        emit bottomChanged(m_b);
    }

    if (m_t != top) {
        m_t = top;
        rangeChanged = true;
        emit topChanged(m_t);
    }

    if (rangeChanged) {
        emit changed();
    }
}
