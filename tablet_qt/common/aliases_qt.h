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

#include <QMap>
#include <QString>
#include <QVariant>
#include <QVector>

// Arguments to SQL queries.
using ArgList = QVector<QVariant>;

// SQL "order by" clauses.
// In each QPair, the string is the fieldname; the bool is "ascending?".
using OrderBy = QVector<QPair<QString, bool>>;

// SQL UPDATE key/value pairs.
// Map column to value.
using UpdateValues = QMap<QString, QVariant>;
