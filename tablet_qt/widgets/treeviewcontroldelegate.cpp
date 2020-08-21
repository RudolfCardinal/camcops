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

/*

See discussion in diagnosticcodeselector.cpp

- http://www.qtcentre.org/threads/61716-Set-the-color-of-a-row-in-a-qtreeview
- https://stackoverflow.com/questions/7175333/howto-create-delegate-for-qtreewidget
- http://www.qtcentre.org/threads/4434-QTreeWidget-amp-QStyle-PE_IndicatorBranch

The information coming here seems insufficient.

Should we instead be overriding QTreeView::drawBranches, or setting a new
widget style() for QTreeView::drawBranches to use when it calls
    style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
?

Perhaps we should use a proxy style. See TreeViewProxyStyle.

*/

#include "treeviewcontroldelegate.h"
#include <QDebug>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>


TreeViewControlDelegate::TreeViewControlDelegate(QObject* parent) :
    QStyledItemDelegate(parent)
{
}


void TreeViewControlDelegate::paint(QPainter* painter,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    qDebug() << Q_FUNC_INFO << "option" << option << "index" << index;
    QStyledItemDelegate::paint(painter, option, index);
}
