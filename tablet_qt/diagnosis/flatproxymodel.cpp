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

#include "flatproxymodel.h"


void FlatProxyModel::setSourceModel(QAbstractItemModel* src_model)
{
    QAbstractProxyModel::setSourceModel(src_model);
    buildMap(src_model);
    connect(src_model, &QAbstractItemModel::dataChanged,
            this, &FlatProxyModel::sourceDataChanged);
}


int FlatProxyModel::buildMap(QAbstractItemModel* src_model,
                             const QModelIndex& src_parent,
                             int proxy_row)
{
    if (proxy_row == 0) {
        m_row_from_src_index.clear();
        m_src_index_from_row.clear();
    }
    const int nrows = src_model->rowCount(src_parent);
    for (int r = 0; r < nrows; ++r) {
        QModelIndex index = src_model->index(r, 0, src_parent);
        m_row_from_src_index[index] = proxy_row;
        m_src_index_from_row[proxy_row] = index;
        proxy_row += 1;
        if (src_model->hasChildren(index)) {
            proxy_row = buildMap(src_model, index, proxy_row);
        }
    }
    return proxy_row;
}


void FlatProxyModel::sourceDataChanged(
        const QModelIndex& src_top_left,
        const QModelIndex& src_bottom_right,
        const QVector<int>& roles)
{
    emit dataChanged(mapFromSource(src_top_left),
                     mapFromSource(src_bottom_right),
                     roles);
}


QModelIndex FlatProxyModel::mapFromSource(
        const QModelIndex& src_index) const
{
    if (!m_row_from_src_index.contains(src_index)) {
        return {};
    }
    return createIndex(m_row_from_src_index[src_index], src_index.column());
}


QModelIndex FlatProxyModel::mapToSource(
        const QModelIndex& proxy_index) const
{
    if (!proxy_index.isValid() ||
            !m_src_index_from_row.contains(proxy_index.row())) {
        return {};
    }
    return m_src_index_from_row[proxy_index.row()];
}


int FlatProxyModel::columnCount(const QModelIndex& proxy_parent) const
{
    return sourceModel()->columnCount(mapToSource(proxy_parent));
}


int FlatProxyModel::rowCount(const QModelIndex& proxy_parent) const
{
    // The root has all the children, and the root's index is invalid.
    return proxy_parent.isValid() ? 0 : m_row_from_src_index.size();
}


QModelIndex FlatProxyModel::index(const int proxy_row, const int proxy_column,
                                  const QModelIndex& proxy_parent) const
{
    return proxy_parent.isValid()
            ? QModelIndex()
            : createIndex(proxy_row, proxy_column);
}


QModelIndex FlatProxyModel::parent(const QModelIndex& proxy_child) const
{
    Q_UNUSED(proxy_child)
    return {};
}


bool FlatProxyModel::hasChildren(const QModelIndex& proxy_parent) const
{
    if (proxy_parent.isValid()) {
        // Not the root; therefore, no children
        return false;
    }
    // The root, so children if we're not empty
    return !m_row_from_src_index.empty();
}
