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
#include <QColor>
#include <QString>
#include <QStringList>
#include <QVector>


class IDED3DExemplars {
public:
    IDED3DExemplars();
    IDED3DExemplars(const QStringList& dimensions,
                    const QVector<QVector<int>>& indices);
    QStringList dimensions;
    QVector<QVector<int>> indices;
    QVector<int> getExemplars(const QString& dim_name) const;
    QVector<int> getShapes() const;
    QVector<int> getColours() const;
    // QStringList getColourNames() const;
    QVector<int> getNumbers() const;

    static int nShapes();
    static QString shapeSvg(int shape_num);
    // static QString colourName(int colour_number);
    static QColor colour(int colour_number);
    static QStringList possibleDimensions();
    static QVector<QVector<int>> possibilities(int number_min,
                                               int number_max);
    static QString allShapesAsJson();
    static QString allColoursAsJson();
protected:
    static QVector<int> possibleShapeIndices();
    static QVector<int> possibleColourIndices();

    static const QString DIM_SHAPE;
    static const QString DIM_COLOUR;
    static const QString DIM_NUMBER;
    static const QStringList VALID_DIMENSION_NAMES;
};
