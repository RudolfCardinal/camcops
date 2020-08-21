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

#include "textconfig.h"

TextConfig::TextConfig(int font_size_px,
                       const QColor& colour,
                       const int width,
                       const Qt::Alignment alignment) :
    font_size_px(font_size_px),
    colour(colour),
    width(width),
    alignment(alignment)
{
}

// https://stackoverflow.com/questions/228783/what-are-the-rules-about-using-an-underscore-in-a-c-identifier

TextConfig& TextConfig::setFontSize(const int font_size_px_)
{
    font_size_px = font_size_px_;
    return *this;
}


TextConfig& TextConfig::setColour(const QColor& colour_)
{
    colour = colour_;
    return *this;
}


TextConfig& TextConfig::setWidth(const int width_)
{
    width = width_;
    return *this;
}


TextConfig& TextConfig::setAlignment(const Qt::Alignment alignment_)
{
    alignment = alignment_;
    return *this;
}
