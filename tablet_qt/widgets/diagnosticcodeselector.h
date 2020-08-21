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
#include <QModelIndex>
#include <QPointer>
#include <QSharedPointer>
#include "common/aliases_camcops.h"
#include "openablewidget.h"

class DiagnosticCodeSet;
class DiagnosisSortFilterModel;
class FlatProxyModel;

class QAbstractButton;
class QItemSelection;
class QItemSelectionModel;
class QLabel;
class QLineEdit;
class QListView;
class QModelIndex;
class QStandardItemModel;
class QTreeView;


class DiagnosticCodeSelector : public OpenableWidget
{
    // Offers both a tree browser and a search box for diagnostic codes.

    Q_OBJECT
public:

    // Constructor. The codeset might be, for example, ICD-10 or ICD-9-CM.
    DiagnosticCodeSelector(const QString& stylesheet,
                           const DiagnosticCodeSetPtr& codeset,
                           const QModelIndex& selected = QModelIndex(),
                           QWidget* parent = nullptr);

signals:
    // "The user has chosen a new code/description."
    void codeChanged(const QString& code, const QString& description);

protected slots:
    // "A new item has been selected."
    void selectionChanged(const QItemSelection& selected,
                          const QItemSelection& deselected);

    // "A new item has been selected from a proxy model."
    void proxySelectionChanged(const QItemSelection& proxy_selected,
                               const QItemSelection& proxy_deselected);

    // "An item has been clicked/touched in the search view."
    void searchItemClicked(const QModelIndex& index);

    // "An item has been clicked/touched in the tree view."
    void treeItemClicked(const QModelIndex& index);

    // "The user has changed the text in the search box."
    void searchTextEdited(const QString& text);

    // void toggleSearch();

    // "Go to the search view."
    void goToSearch();

    // "Go to the tree view."
    void goToTree();

    // "Dump our layout to the debugging stream."
    void debugLayout();

protected:

    // "A new item has been chosen."
    // Will emit codeChanged(), then finished().
    void itemChosen(const QModelIndex& index);

    // Converts a proxy index (an index within our search view's model) to
    // the proper index within the whole codeset.
    QModelIndex sourceFromProxy(const QModelIndex& index);

    // Opposite of sourceFromProxy().
    QModelIndex proxyFromSource(const QModelIndex& index);

    // Sets our visual appearance according to whether we're searching or
    // browsing the tree view.
    void setSearchAppearance();

protected:
    DiagnosticCodeSetPtr m_codeset;  // our set of diagnoses
    QPointer<QTreeView> m_treeview;  // for exploring
    QPointer<QListView> m_flatview;  // for searching
    QPointer<QLineEdit> m_search_lineedit;  // where the user types search terms
    QPointer<QLabel> m_heading_tree;  // heading for the tree view
    QPointer<QLabel> m_heading_search;  // heading for the search view
    QPointer<QAbstractButton> m_search_button;  // "Go to search"
    QPointer<QAbstractButton> m_tree_button;  // "Go to tree"
    QSharedPointer<QItemSelectionModel> m_selection_model;  // model of our codeset
    QSharedPointer<FlatProxyModel> m_flat_proxy_model;
        // ... a flat model made from our codeset's tree (for searching)
    QSharedPointer<DiagnosisSortFilterModel> m_diag_filter_model;
        // ... a model for searching/filtering; uses m_flat_proxy_model
    QSharedPointer<QItemSelectionModel> m_proxy_selection_model;
        // ... item selection model for m_diag_filter_model
    bool m_searching;  // are we currently searching (rather than at the tree)?
};
