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

#include "buttonconfig.h"


ButtonConfig::ButtonConfig(const int padding_px,
                           const int font_size_px,
                           const QColor& text_colour,
                           const Qt::Alignment text_alignment,
                           const QColor& background_colour,
                           const QColor& pressed_background_colour,
                           const QPen& border_pen,
                           const int corner_radius_px) :
    padding_px(padding_px),
    font_size_px(font_size_px),
    text_colour(text_colour),
    text_alignment(text_alignment),
    background_colour(background_colour),
    pressed_background_colour(pressed_background_colour),
    border_pen(border_pen),
    corner_radius_px(corner_radius_px)
{
}


ButtonConfig ButtonConfig::clone() const
{
    return ButtonConfig(*this);
}


ButtonConfig& ButtonConfig::setPadding(const int font_size_px_)
{
    font_size_px = font_size_px_;
    return *this;
}


ButtonConfig& ButtonConfig::setFontSize(const int font_size_px_)
{
    font_size_px = font_size_px_;
    return *this;
}


ButtonConfig& ButtonConfig::setTextColour(const QColor& text_colour_)
{
    text_colour = text_colour_;
    return *this;
}


ButtonConfig& ButtonConfig::setTextAlignment(
        const Qt::Alignment text_alignment_)
{
    text_alignment = text_alignment_;
    return *this;
}


ButtonConfig& ButtonConfig::setBackgroundColour(
        const QColor& background_colour_)
{
    background_colour = background_colour_;
    return *this;
}


ButtonConfig& ButtonConfig::setPressedBackgroundColour(
        const QColor& pressed_background_colour_)
{
    pressed_background_colour = pressed_background_colour_;
    return *this;
}


ButtonConfig& ButtonConfig::setBorderPen(const QPen& border_pen_)
{
    border_pen = border_pen_;
    return *this;
}


ButtonConfig& ButtonConfig::setCornerRadius(const int corner_radius_px_)
{
    corner_radius_px = corner_radius_px_;
    return *this;
}
