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
#include <QPen>


// Represents a graphical button with text in it.
// Used by graphicsfunc::makeTextButton().
//
// We use pixels, not points, for font sizes here.
// In general, this is deprecated, because it makes things device-specific,
// i.e. dependent on the dots-per-inch (DPI) setting. However, in this context
// we are working in a pixel-based graphics system, which is then scaled by the
// ScreenLikeGraphicsView. It's not clear that "DPI" makes sense here, and we
// want our text size to be predictable.

struct ButtonConfig {
public:
    ButtonConfig(int padding_px,
                 int font_size_px,
                 const QColor& text_colour,
                 Qt::Alignment text_alignment,
                 const QColor& background_colour,
                 const QColor& pressed_background_colour,
                 const QPen& border_pen,
                 int corner_radius_px);
    ButtonConfig clone() const;
    ButtonConfig& setPadding(int font_size_px);
    ButtonConfig& setFontSize(int font_size_px);
    ButtonConfig& setTextColour(const QColor& text_colour);
    ButtonConfig& setTextAlignment(Qt::Alignment text_alignment);
    ButtonConfig& setBackgroundColour(const QColor& background_colour);
    ButtonConfig& setPressedBackgroundColour(const QColor& pressed_background_colour);
    ButtonConfig& setBorderPen(const QPen& border_pen);
    ButtonConfig& setCornerRadius(int corner_radius_px);
public:
    int padding_px;
    int font_size_px;
    QColor text_colour;
    Qt::Alignment text_alignment;
    QColor background_colour;
    QColor pressed_background_colour;
    QPen border_pen;
    int corner_radius_px;
};
