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

#include "cardinalexpdetrating.h"
#include "maths/mathfunc.h"
#include "taskxtra/cardinalexpdetcommon.h"

const double POINTS_PER_RATING = 10.0;

const QStringList TX_OPTIONS{
    "No,\ndefinitely not",
    "No,\nprobably not",
    "Unsure",
    "Yes,\nprobably",
    "Yes,\ndefinitely",
};
const int CardinalExpDetRating::N_RATINGS = TX_OPTIONS.size();


CardinalExpDetRating::CardinalExpDetRating(
        const int rating,
        const bool detection_response_on_right) :
    rating(rating)
{
    Q_ASSERT(rating >= 0 && rating < N_RATINGS);
    const double rating_double = static_cast<double>(rating);
    const double centre_rating = (N_RATINGS - 1) / 2.0;
    // ... for 5 ratings, internal number 0-4, centre is 2;
    // ... for 6 ratings, internal number 0-5, centre is 2.5
    const int pos = detection_response_on_right ? rating : (N_RATINGS - 1 - rating);

    rect = getRatingButtonRect(pos, N_RATINGS);
    label = TX_OPTIONS.at(rating);
    points_multiplier = static_cast<int>(
                qAbs(rating_double - centre_rating) * POINTS_PER_RATING);
    // ... e.g. 5 ratings:         (2 ,1, 0, 1, 2) * POINTS_PER_RATING;
    //          6 ratings: (2.5, 1.5, 0.5, 0.5, 1.5, 2.5) * POINTS_PER_RATING
    means_yes = rating_double > centre_rating;
    means_dont_know = mathfunc::nearlyEqual(rating_double, centre_rating);
}


CardinalExpDetRating::CardinalExpDetRating()
{
    rating = -1;
    points_multiplier = 0;
    means_yes = false;
    means_dont_know = false;
}


QRectF CardinalExpDetRating::getRatingButtonRect(const int pos,
                                                 const int n) const
{
    const qreal ratingbutton_width = 0.8 * (SCENE_WIDTH / N_RATINGS);
    const qreal centre = (SCENE_WIDTH * (2 * pos + 1)) / (2 * n);
    return QRectF(centre - ratingbutton_width / 2,  // left
                  0.7 * SCENE_HEIGHT,  // top
                  ratingbutton_width,  // width
                  0.2 * SCENE_HEIGHT);  // height
}
