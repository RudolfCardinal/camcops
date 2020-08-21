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

#include "mcqgridsubtitle.h"

McqGridSubtitle::McqGridSubtitle(const int pos, const QString& string,
                                 const bool repeat_options) :
    m_pos(pos),
    m_string(string),
    m_repeat_options(repeat_options)
{
}


int McqGridSubtitle::pos() const
{
    return m_pos;
}


QString McqGridSubtitle::string() const
{
    return m_string;
}


bool McqGridSubtitle::repeatOptions() const
{
    return m_repeat_options;
}
