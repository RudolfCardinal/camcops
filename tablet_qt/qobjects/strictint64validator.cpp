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

#include "strictint64validator.h"
#include <algorithm>
#include "lib/numericfunc.h"


StrictInt64Validator::StrictInt64Validator(const bool allow_empty,
                                           QObject* parent) :
    QValidator(parent),
    m_allow_empty(allow_empty)
{
    m_b = 0;
    m_t = std::numeric_limits<qint64>::max();
}


StrictInt64Validator::StrictInt64Validator(
        qint64 bottom, qint64 top,
        const bool allow_empty, QObject* parent) :
    QValidator(parent),
    m_allow_empty(allow_empty)
{
    if (top < bottom) {  // user has supplied them backwards
        std::swap(bottom, top);
    }
    m_b = bottom;
    m_t = top;
}


StrictInt64Validator::~StrictInt64Validator()
{
}


QValidator::State StrictInt64Validator::validate(QString& s, int& pos) const
{
    Q_UNUSED(pos)
    return numeric::validateInteger(s, locale(), bottom(), top(),
                                    m_allow_empty);
}


void StrictInt64Validator::setBottom(const qint64 bottom)
{
    setRange(bottom, top());
}


void StrictInt64Validator::setTop(const qint64 top)
{
    setRange(bottom(), top);
}


void StrictInt64Validator::setRange(const qint64 bottom, const qint64 top)
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
