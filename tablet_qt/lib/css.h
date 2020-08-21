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

#pragma once
#include <QString>
class QColor;
class QPen;

namespace css
{

// Returns CSS like "3px" or "0".
QString pixelCss(int px);

// Returns CSS like "11.5pt" or "0".
QString ptCss(qreal pt);

// Returns CSS like "rgba(255,255,255,1.0)".
QString colourCss(const QColor& colour);

// Returns CSS like "solid" or "dotted".
QString penStyleCss(const QPen& pen);

// Returns CSS like "3px solid rgba(255,0,0,1.0)" or "none".
QString penCss(const QPen& pen);

// Returns CSS for no border/margin/padding, a transparent background, and the
// specified foreground colour; suitable for use for label text.
QString labelCss(const QColor& colour);

}  // namespace css
