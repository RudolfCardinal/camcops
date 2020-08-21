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

#include "ided3dexemplars.h"
#include <QColor>
#include "lib/containers.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
using mathfunc::seq;
using mathfunc::range;


// Dimensions
const QString IDED3DExemplars::DIM_SHAPE("shape");
const QString IDED3DExemplars::DIM_COLOUR("colour");
const QString IDED3DExemplars::DIM_NUMBER("number");
const QStringList IDED3DExemplars::VALID_DIMENSION_NAMES{
    IDED3DExemplars::DIM_SHAPE,
    IDED3DExemplars::DIM_COLOUR,
    IDED3DExemplars::DIM_NUMBER
};


// Shapes
const QStringList SHAPE_DEFINITIONS{
    /*
        List of paths.
        MULTI.PAT contained 96, but were only 12 things repeated 8 times.
        All stimuli redrawn.
        Good online editor:
            http://jsfiddle.net/DFhUF/1393/
        ... being jsfiddle set to the Raphael 2.1.0 framework "onLoad".
        Code:

var path = [
    // DETAILS HERE
    ["m10,-53 l20,100 l-60,0 z m50,60 l-120,20 l0,-50 z"], // 0: up-pointing triangle and right-pointing triangle
    ["m0,-50 l-57,57 l28,28 l28,-28 l28,28 l28,-28 z"], // 1: stealth bomber flying up
    ["m-15,-50 l-45,25 l90,0 z m15,35 l-45,25 l90,0 z m15,35 l-45,25 l90,0 z"], // 2: stacked triangle hats slightly offset horizontally
    ["m-60,-11 l94,55 l26,-28 l-38,-15 l38,-15 l-26,-28 l-94,55 z"], // 3: small-tailed fish with gaping mouth pointing right
    ["m-20,-50 l-40,50 l45,0 l0,50 l30,0 l0,-50 l45,0 l-45,-50 z"], // 4: top-truncated tree
    ["m-60,-36 l120,0 l0,72 l-40,0 l0,-36 l-40,0 l0,36, l-40,0 z"], // 5: side view of block table, or blocky inverted U
    ["m0,-40 l60,40 l-40,27 l0,13 l-40,0 l0,-13 l-40,-27 z"], // 6: diamond-like tree
    ["m-33,40 l-27,-40 l27,-40 l33,27 l33,-27 l27,40 l-27,40 l-33,-27 z"], // 7: bow tie
    ["m-60,-30 l60,-30 l60,30 l0,60 l-60,30 l-60,-30 z"], // 8: hexagon
    ["m-60,60 l120,0 l-60,-60 z m0,-120 l120,0 l-60,60 z"], // 9: hourglass of triangles
    ["m-60,-40 l0,68 l120,0 l-45,-30 l0,11 l-45,-38 l0,23 z"], // 10: mountain range
    ["m-60,0 l34,-43 l86,0 l-34,43 l34,43 l-86,0 z"], // 11: left-pointing arrow feathers
],
index = 10,  // currently working on this one
s = 120,  // target size; width and height
c = 250,  // centre
paper = Raphael(0, 0, c*2, c*2),
crosshairs = ["M", 0, c, "L", c*2, c, "M", c, 0,  "L", c, c*2],
chattr = {stroke: "#f00", opacity: 1, "stroke-width" : 1},
gridattr = {stroke: "#888", opacity: 0.5, "stroke-width" : 1},
textattr = {fill: "red", font: "20px Arial", "text-anchor": "middle"},
pathattr = {stroke: "#808", opacity: 1, "stroke-width" : 1, fill: "#ccf"},
i;
paper.path(path[index]).translate(c, c).attr(pathattr);
for (i = 0; i < 2*c; i += 10) {
    paper.path(["M", 0, i, "L", 2*c, i]).attr(gridattr);
    paper.path(["M", i, 0, "L", i, 2*c]).attr(gridattr);
}
paper.rect(c - s/2, c - s/2, s, s).attr(chattr);
paper.path(crosshairs).attr(chattr);
paper.text(c, c, "0").attr(textattr);

    */

    // 0: up-pointing triangle and right-pointing triangle
    "m10,-53 l20,100 l-60,0 z m50,60 l-120,20 l0,-50 z",

    // 1: stealth bomber flying up
    "m0,-50 l-57,57 l28,28 l28,-28 l28,28 l28,-28 z",

    // 2: stacked triangle hats slightly offset horizontally
    "m-15,-50 l-45,25 l90,0 z m15,35 l-45,25 l90,0 z m15,35 l-45,25 l90,0 z",

    // 3: small-tailed fish with gaping mouth pointing right
    "m-60,-11 l94,55 l26,-28 l-38,-15 l38,-15 l-26,-28 l-94,55 z",

    // 4: top-truncated tree
    "m-20,-50 l-40,50 l45,0 l0,50 l30,0 l0,-50 l45,0 l-45,-50 z",

    // 5: side view of block table, or blocky inverted U
    "m-60,-36 l120,0 l0,72 l-40,0 l0,-36 l-40,0 l0,36, l-40,0 z",

    // 6: diamond-like tree
    "m0,-40 l60,40 l-40,27 l0,13 l-40,0 l0,-13 l-40,-27 z",

    // 7: bow tie
    "m-33,40 l-27,-40 l27,-40 l33,27 l33,-27 l27,40 l-27,40 l-33,-27 z",

    // 8: hexagon
    "m-60,-30 l60,-30 l60,30 l0,60 l-60,30 l-60,-30 z",

    // 9: hourglass of triangles
    "m-60,60 l120,0 l-60,-60 z m0,-120 l120,0 l-60,60 z",

    // 10: mountain range
    "m-60,-40 l0,68 l120,0 l-45,-30 l0,11 l-45,-38 l0,23 z",

    // 11: left-pointing arrow feathers
    "m-60,0 l34,-43 l86,0 l-34,43 l34,43 l-86,0 z",
};

const QVector<QColor> POSSIBLE_COLOURS{
    // HTML colour definitions of CGA colours.
    // Note that these are fine for static initialiation (they don't depend
    // on the statically-initialized RGB colour name table in qcolor.cpp).
    QColor("#555"), // CGA: dark grey
    QColor("#55f"), // CGA: light blue
    QColor("#5f5"), // CGA: light green
    QColor("#5ff"), // CGA: light cyan
    QColor("#f55"), // CGA: light red
    QColor("#f5f"), // CGA: light magenta
    QColor("#ff5"), // CGA: yellow
    QColor("#fff"), // white
};


IDED3DExemplars::IDED3DExemplars()
{
}


IDED3DExemplars::IDED3DExemplars(const QStringList& dimensions,
                                 const QVector<QVector<int>>& indices) :
    dimensions(dimensions),
    indices(indices)
{
    Q_ASSERT(dimensions.length() >= 1);
    Q_ASSERT(dimensions.length() == indices.length());
    Q_ASSERT(containers::containsAll(VALID_DIMENSION_NAMES, dimensions));
}


QVector<int> IDED3DExemplars::getExemplars(const QString& dim_name) const
{
    for (int i = 0; i < dimensions.length(); ++i) {
        if (dimensions.at(i) == dim_name) {
            return indices.at(i);
        }
    }
    Q_ASSERT(false);
    return QVector<int>();
}


QVector<int> IDED3DExemplars::getShapes() const
{
    return getExemplars(DIM_SHAPE);
}


QVector<int> IDED3DExemplars::getColours() const
{
    return getExemplars(DIM_COLOUR);
}


/*
QStringList IDED3DExemplars::getColourNames() const
{
    QVector<int> colour_indices = getColours();
    QStringList names;
    for (auto i : colour_indices) {
        Q_ASSERT(i >= 0 && i <= POSSIBLE_COLOURS.length());
        names.append(POSSIBLE_COLOURS.at(i).name());
    }
    return names;
}
*/


QVector<int> IDED3DExemplars::getNumbers() const
{
    return getExemplars(DIM_NUMBER);
}


int IDED3DExemplars::nShapes()
{
    return SHAPE_DEFINITIONS.length();
}


QString IDED3DExemplars::shapeSvg(const int shape_num)
{
    return SHAPE_DEFINITIONS.at(shape_num);
}


/*
QString IDED3DExemplars::colourName(const int colour_number)
{
    return POSSIBLE_COLOURS.at(colour_number).name();
}
*/


QColor IDED3DExemplars::colour(const int colour_number)
{
    return POSSIBLE_COLOURS.at(colour_number);
}


QVector<int> IDED3DExemplars::possibleShapeIndices()
{
    return range(SHAPE_DEFINITIONS.length());
}


QVector<int> IDED3DExemplars::possibleColourIndices()
{
    return range(POSSIBLE_COLOURS.length());
}


QStringList IDED3DExemplars::possibleDimensions()
{
    return QStringList{
        DIM_SHAPE,
        DIM_COLOUR,
        DIM_NUMBER
    };
}


QVector<QVector<int>> IDED3DExemplars::possibilities(const int number_min,
                                                     const int number_max)
{
    // Order of dimensions in vector must match possibleDimensions()
    const QVector<int> possible_shapes = possibleShapeIndices();
    const QVector<int> possible_colours = possibleColourIndices();
    const QVector<int> possible_numbers = seq(number_min,
                                                 number_max);
    return QVector<QVector<int>>{
        possible_shapes,
        possible_colours,
        possible_numbers
    };
}


QString IDED3DExemplars::allShapesAsJson()
{
    return convert::stringListToJson(SHAPE_DEFINITIONS);
}


QString IDED3DExemplars::allColoursAsJson()
{
    QStringList colours;
    for (const QColor& colour : POSSIBLE_COLOURS) {
        colours.append(colour.name());
    }
    return convert::stringListToJson(colours);
}
