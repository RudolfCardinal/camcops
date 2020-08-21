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
#include <QRectF>
#include <QString>


class CardinalExpDetRating
{
public:
    CardinalExpDetRating(int rating, bool detection_response_on_right);
    CardinalExpDetRating();  // so it can live in a QVector
protected:
    QRectF getRatingButtonRect(int x, int n) const;
public:
    int rating;
    QRectF rect;
    QString label;
    int points_multiplier;
    bool means_yes;
    bool means_dont_know;
    static const int N_RATINGS;
};
