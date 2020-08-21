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

#include "paintertranslaterotatecontext.h"
#include <QPainter>


PainterTranslateRotateContext::PainterTranslateRotateContext(
        QPainter& painter,
        const QPointF& at,
        const qreal rotate_clockwise_deg) :
    m_painter(painter),
    m_at(at),
    m_rotate_clockwise_deg(rotate_clockwise_deg)
{
    m_painter.translate(m_at);
    m_painter.rotate(m_rotate_clockwise_deg);
}


PainterTranslateRotateContext::~PainterTranslateRotateContext()
{
    m_painter.rotate(-m_rotate_clockwise_deg);
    m_painter.translate(-m_at);
}
