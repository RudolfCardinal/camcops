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

#include "diagnosissortfiltermodel.h"


bool DiagnosisSortFilterModel::filterAcceptsRow(
        const int row,
        const QModelIndex& parent) const
{
    // Filter modification that accepts parents whose children meet the filter
    // criteria. (Note that calling setFilterFixedString correctly affects
    // filterRegExp(); see qsortfilterproxymodel.cpp).

    // http://doc.qt.io/qt-5/qsortfilterproxymodel.html#filterAcceptsRow
    // http://www.qtcentre.org/threads/46471-QTreeView-Filter

    const QModelIndex index = sourceModel()->index(row, 0, parent);

    if (!index.isValid()) {
        return false;
    }

    // Remove rows that are not selectable.
    if (!(index.flags() & Qt::ItemIsSelectable)) {
        return false;
    }

    // Otherwise, if it matches our search criteria, we're good:
    if (index.data().toString().contains(filterRegExp())) {
        return true;
    }

    /*
    // For tree models, but not for flat models:

    // Permit if children are shown as well
    int nrows = sourceModel()->rowCount(index);
    for (int r = 0; r < nrows; ++r) {
        if (filterAcceptsRow(r, index)) {
            return true;
        }
    }
    */

    return false;
}
