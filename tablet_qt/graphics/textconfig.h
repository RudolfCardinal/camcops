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
#include <Qt>
#include <QColor>

// Simple information-holding class representing text "configuration" for
// graphics functions:
//
// - font size
// - colour
// - width to word-wrap to (-1 for no wrap)
// - text alignment.
//
// We use pixels, not points, for font sizes here.
// In general, this is deprecated, because it makes things device-specific,
// i.e. dependent on the dots-per-inch (DPI) setting. However, in this context
// we are working in a pixel-based graphics system, which is then scaled by the
// ScreenLikeGraphicsView. It's not clear that "DPI" makes sense here, and we
// want our text size to be predictable.

struct TextConfig {
public:
    TextConfig(int font_size_px,
               const QColor& colour,
               int width = -1,
               Qt::Alignment alignment = Qt::AlignCenter);
    TextConfig& setFontSize(int font_size_px);
    TextConfig& setColour(const QColor& colour);
    TextConfig& setWidth(int width);
    TextConfig& setAlignment(Qt::Alignment alignment);
public:
    int font_size_px;
    QColor colour;
    int width;
    Qt::Alignment alignment;
};
