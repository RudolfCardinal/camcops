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

// #define OFFER_LAYOUT_DEBUG_BUTTON

// #define RESPOND_VIA_ITEM_SELECTION  // bad
#define RESPOND_VIA_ITEM_CLICKED  // good

#include "diagnosticcodeselector.h"
#include <functional>
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QListView>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "diagnosis/diagnosticcodeset.h"
#include "diagnosis/diagnosissortfiltermodel.h"
#include "diagnosis/flatproxymodel.h"
#include "layouts/layouts.h"
#include "lib/layoutdumper.h"
#include "lib/uifunc.h"
#include "widgets/basewidget.h"
#include "widgets/horizontalline.h"
#include "widgets/imagebutton.h"
#include "widgets/labelwordwrapwide.h"

/*

- To enable the selection of a non-leaf node, if desired:

- Separating out single clicks and double clicks is confusing, in that the
  standard double-click delay is noticeable, and if you react to a single click
  at the end of that time, it looks like the software has a huge latency.

  - http://stackoverflow.com/questions/22142485
  - QApplication::doubleClickInterval()

- Better would be to have a "hold" gesture.

- However, the standard "install event filter" and eventFilter() function
  doesn't pick up what's needed from QListWidget. One probably has to use
  QListView (which also offers mousePressEvent, mouseMoveEvent,
  mouseReleaseEvent). However, this probably gets non-intuitive for users.

- Therefore, buttons for "choose me" and "drill down into me", with the default
  for touches in the non-button area being "drill down into me".

- Shoving buttons inside a QListWidget isn't great. So:
    http://stackoverflow.com/questions/4004705
  ... use a QTreeWidget, to get multiple columns
  ... at which point: it's a tree!

- Then the proper way to do filtering is with a QSortFilterProxyModel, using
  a QTreeView rather than a QTreeWidget.

*/


DiagnosticCodeSelector::DiagnosticCodeSelector(
        const QString& stylesheet,
        const DiagnosticCodeSetPtr& codeset,
        const QModelIndex& selected,
        QWidget* parent) :
    OpenableWidget(parent),
    m_codeset(codeset),
    m_treeview(nullptr),
    m_flatview(nullptr),
    m_search_lineedit(nullptr),
    m_heading_tree(nullptr),
    m_heading_search(nullptr),
    m_search_button(nullptr),
    m_tree_button(nullptr),
    m_selection_model(nullptr),
    m_flat_proxy_model(nullptr),
    m_diag_filter_model(nullptr),
    m_proxy_selection_model(nullptr),
    m_searching(false)
{
    Q_ASSERT(m_codeset);

    setStyleSheet(stylesheet);

    // ========================================================================
    // Header
    // ========================================================================

    // ------------------------------------------------------------------------
    // Main row
    // ------------------------------------------------------------------------

    const Qt::Alignment button_align = Qt::AlignHCenter | Qt::AlignTop;
    const Qt::Alignment text_align = Qt::AlignHCenter | Qt::AlignVCenter;

    // Cancel button
    auto cancel = new ImageButton(uiconst::CBS_CANCEL);
    connect(cancel, &QAbstractButton::clicked,
            this, &DiagnosticCodeSelector::finished);

    // Title
    auto title_label = new LabelWordWrapWide(m_codeset->title());
    title_label->setAlignment(text_align);
    title_label->setObjectName(cssconst::TITLE);

    m_search_button = new ImageButton(uiconst::CBS_MAGNIFY);
    connect(m_search_button.data(), &QAbstractButton::clicked,
            this, &DiagnosticCodeSelector::goToSearch);

    m_tree_button = new ImageButton(uiconst::CBS_TREE_VIEW);
    connect(m_tree_button.data(), &QAbstractButton::clicked,
            this, &DiagnosticCodeSelector::goToTree);

    auto header_toprowlayout = new HBoxLayout();
    header_toprowlayout->addWidget(cancel, 0, button_align);
    header_toprowlayout->addStretch();
    header_toprowlayout->addWidget(title_label, 0, text_align);  // default alignment fills whole cell; this is better
    header_toprowlayout->addStretch();
#ifdef OFFER_LAYOUT_DEBUG_BUTTON
    auto button_debug = new QPushButton("Dump layout");
    connect(button_debug, &QAbstractButton::clicked,
            this, &DiagnosticCodeSelector::debugLayout);
    header_toprowlayout->addWidget(button_debug, 0, text_align);
#endif
    header_toprowlayout->addWidget(m_search_button, 0, button_align);
    header_toprowlayout->addWidget(m_tree_button, 0, button_align);

    // ------------------------------------------------------------------------
    // Horizontal line
    // ------------------------------------------------------------------------
    auto horizline = new HorizontalLine(uiconst::HEADER_HLINE_WIDTH);
    horizline->setObjectName(cssconst::HEADER_HORIZONTAL_LINE);

    // ------------------------------------------------------------------------
    // Header assembly
    // ------------------------------------------------------------------------
    auto header_mainlayout = new VBoxLayout();
    header_mainlayout->addLayout(header_toprowlayout);
    header_mainlayout->addWidget(horizline);
    auto header = new BaseWidget();
    header->setLayout(header_mainlayout);

    // ========================================================================
    // Selection model
    // ========================================================================

    m_selection_model = QSharedPointer<QItemSelectionModel>(
                new QItemSelectionModel(m_codeset.data()));
#ifdef RESPOND_VIA_ITEM_SELECTION
    connect(m_selection_model.data(), &QItemSelectionModel::selectionChanged,
            this, &DiagnosticCodeSelector::selectionChanged);
#endif
    m_selection_model->select(selected, QItemSelectionModel::ClearAndSelect);

    // ========================================================================
    // Tree view
    // ========================================================================
    // - To set the expand/collapse ("disclosure"? "indicator"?) icons:
    //   - https://stackoverflow.com/questions/2638974/qtreewidget-expand-sign
    //   - http://doc.qt.io/qt-5/stylesheet-examples.html#customizing-qtreeview
    //   - Probably not: QTreeView::drawBranches in qtreeview.cpp : uses styles
    //     ... search for "has-children" gives gui/text/qcssparser.cpp
    //     ... to PseudoClass_Children
    //     ... to qstylesheetstyle.cpp
    //     ... to State_Children
    //     ... to (FOR EXAMPLE) qfusionstyle.cpp
    //     ... where in QFusionStyle::drawPrimitive() we have a section for
    //         PE_IndicatorBranch and draw things like PE_IndicatorArrowDown
    //         and PE_IndicatorArrowRight.
    //   - SE_TreeViewDisclosureItem
    //   - QTreeView::drawRow
    //          d->delegateForIndex(modelIndex)->paint(painter, opt, modelIndex);
    //          -> QAbstractItemDelegate::paint()
    //          -> as default delegate is QStyledItemDelegate...
    //             [http://doc.qt.io/qt-4.8/model-view-programming.html]
    //          -> QStyledItemDelegate::paint()
    //   - https://superuser.com/questions/638139/whats-the-proper-name-of-that-symbol-to-collapse-expand-nodes-in-a-directory-tr
    //      "disclosure widget"
    //      "progressive disclosure controls"
    //      "rotating triangle"; "plus and minus controls"
    //   UPSHOT: fiddly. The trouble is that the CSS just lets us do
    //   url(filename); see qcssparser.cpp and search for "url".

    m_heading_tree = new QLabel(
                tr("Explore as tree [use icon at top right to search]:"));
    m_heading_tree->setObjectName(cssconst::HEADING);

    m_treeview = new QTreeView();
    // TreeViewControlDelegate* delegate = new TreeViewControlDelegate();
    // m_treeview->setItemDelegate(delegate);
    m_treeview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_treeview->setModel(m_codeset.data());
    m_treeview->setSelectionModel(m_selection_model.data());
    if (m_treeview->header()) {
        m_treeview->header()->close();
    }
    m_treeview->setWordWrap(true);
    m_treeview->setColumnHidden(DiagnosticCode::COLUMN_CODE, true);
    m_treeview->setColumnHidden(DiagnosticCode::COLUMN_DESCRIPTION, true);
    m_treeview->setColumnHidden(DiagnosticCode::COLUMN_FULLNAME, false);
    m_treeview->setColumnHidden(DiagnosticCode::COLUMN_SELECTABLE, true);
    m_treeview->setSortingEnabled(false);
    m_treeview->scrollTo(selected);
    uifunc::applyScrollGestures(m_treeview->viewport());
#ifdef RESPOND_VIA_ITEM_CLICKED
    connect(m_treeview.data(), &QListView::clicked,
            this, &DiagnosticCodeSelector::treeItemClicked);
#endif

    // ========================================================================
    // Search box
    // ========================================================================

    m_search_lineedit = new QLineEdit();
    connect(m_search_lineedit, &QLineEdit::textEdited,
            this, &DiagnosticCodeSelector::searchTextEdited);

    // ========================================================================
    // Proxy models: (1) flatten (2) filter
    // ========================================================================
    // http://doc.qt.io/qt-5/qsortfilterproxymodel.html#details

    m_flat_proxy_model = QSharedPointer<FlatProxyModel>(
                new FlatProxyModel());
    m_flat_proxy_model->setSourceModel(m_codeset.data());

    m_diag_filter_model = QSharedPointer<DiagnosisSortFilterModel>(
                new DiagnosisSortFilterModel());
    m_diag_filter_model->setSourceModel(m_flat_proxy_model.data());
    m_diag_filter_model->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_diag_filter_model->sort(DiagnosticCode::COLUMN_CODE, Qt::AscendingOrder);
    m_diag_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_diag_filter_model->setFilterKeyColumn(DiagnosticCode::COLUMN_DESCRIPTION);

    // ========================================================================
    // Selection model for proxy model
    // ========================================================================

    m_proxy_selection_model = QSharedPointer<QItemSelectionModel>(
                new QItemSelectionModel(m_diag_filter_model.data()));

#ifdef RESPOND_VIA_ITEM_SELECTION
    connect(m_proxy_selection_model.data(), &QItemSelectionModel::selectionChanged,
            this, &DiagnosticCodeSelector::proxySelectionChanged);
#endif
    QModelIndex proxy_selected = proxyFromSource(selected);
    m_proxy_selection_model->select(proxy_selected,
                                    QItemSelectionModel::ClearAndSelect);

    // ========================================================================
    // List view, for search
    // ========================================================================
    // We want to show all depths, not just the root nodes, and QListView
    // doesn't by default.
    // - You can make a QTreeView look like this:
    //   http://stackoverflow.com/questions/21564976
    //   ... but users can collapse/expand (and it collapses by itself) and
    //   is not ideal.
    // - The alternative is a proxy model that flattens properly for us (see
    //   same link). We'll do that, and use a real QListView.

    m_heading_search = new QLabel(
                tr("Search diagnoses [use icon at top right for tree view]:"));
    m_heading_search->setObjectName(cssconst::HEADING);

    m_flatview = new QListView();
    m_flatview->setModel(m_diag_filter_model.data());
    m_flatview->setSelectionModel(m_proxy_selection_model.data());
    m_flatview->setWordWrap(true);
    m_flatview->scrollTo(proxy_selected);
    uifunc::applyScrollGestures(m_flatview->viewport());
#ifdef RESPOND_VIA_ITEM_CLICKED
    connect(m_flatview.data(), &QListView::clicked,
            this, &DiagnosticCodeSelector::searchItemClicked);
#endif

    // ========================================================================
    // Final assembly (with "this" as main widget)
    // ========================================================================

    auto mainlayout = new QVBoxLayout();  // not HFW
    mainlayout->addWidget(header);
    mainlayout->addWidget(m_heading_tree);
    mainlayout->addWidget(m_treeview);
    mainlayout->addWidget(m_heading_search);
    mainlayout->addWidget(m_search_lineedit);
    mainlayout->addWidget(m_flatview);
    // mainlayout->addStretch();

    auto topwidget = new QWidget();
    topwidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    topwidget->setObjectName(cssconst::MENU_WINDOW_BACKGROUND);
    topwidget->setLayout(mainlayout);

    auto toplayout = new QVBoxLayout();  // not HFW
    toplayout->setContentsMargins(uiconst::NO_MARGINS);
    toplayout->addWidget(topwidget);

    setLayout(toplayout);

    // Only AFTER widgets added to layout (or standalone windows are created):
    setSearchAppearance();
}


void DiagnosticCodeSelector::selectionChanged(const QItemSelection& selected,
                                              const QItemSelection& deselected)
{
    Q_UNUSED(deselected)
#ifdef RESPOND_VIA_ITEM_SELECTION
    QModelIndexList indexes = selected.indexes();
    if (indexes.isEmpty()) {
        return;
    }
    QModelIndex index = indexes.at(0);
    itemChosen(index);
#else
    Q_UNUSED(selected)
#endif
}


void DiagnosticCodeSelector::itemChosen(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }
    // Now, we want to get an index to potentially different columns of
    // the same object. Note that index.row() is NOT unique, it's just the row
    // number for a given parent.
    // To get a different column, we go via the parent back to the child:
    // http://doc.qt.io/qt-5/qmodelindex.html#details
    // We used to be able to do parent.child(), but that has been obsoleted
    // in favour of model.index(row, column, parent), i.e.
    // QAbstractItemModel::index().
    const QModelIndex parent = index.parent();
    const int row = index.row();
    const QAbstractItemModel* model = index.model();
    const QModelIndex selectable_index = model->index(
                row, DiagnosticCode::COLUMN_SELECTABLE, parent);
    const bool selectable = selectable_index.data().toBool();
    if (!selectable) {
        // qDebug() << Q_FUNC_INFO << "Unselectable";
        return;
    }
    const QModelIndex code_index = model->index(
                row, DiagnosticCode::COLUMN_CODE, parent);
    const QModelIndex description_index = model->index(
                row, DiagnosticCode::COLUMN_DESCRIPTION, parent);
    const QString code = code_index.data().toString();
    const QString description = description_index.data().toString();

    emit codeChanged(code, description);
    emit finished();
}


void DiagnosticCodeSelector::proxySelectionChanged(
        const QItemSelection& proxy_selected,
        const QItemSelection& proxy_deselected)
{
    Q_UNUSED(proxy_deselected)
    QModelIndexList proxy_indexes = proxy_selected.indexes();
    if (proxy_indexes.isEmpty()) {
        return;
    }
    const QModelIndex proxy_index = proxy_indexes.at(0);
    const QModelIndex src_index = sourceFromProxy(proxy_index);
    itemChosen(src_index);
}


void DiagnosticCodeSelector::searchItemClicked(const QModelIndex& index)
{
    // The search view uses a proxy model.
    const QModelIndex src_index = sourceFromProxy(index);
    itemChosen(src_index);
}


void DiagnosticCodeSelector::treeItemClicked(const QModelIndex& index)
{
    // The tree view uses the underlying model directly.
    itemChosen(index);
}


//void DiagnosticCodeSelector::toggleSearch()
//{
//    m_searching = !m_searching;
//    setSearchAppearance();
//}


void DiagnosticCodeSelector::goToSearch()
{
    m_searching = true;
    setSearchAppearance();
}


void DiagnosticCodeSelector::goToTree()
{
    m_searching = false;
    setSearchAppearance();
}


void DiagnosticCodeSelector::setSearchAppearance()
{
    m_tree_button->setVisible(m_searching);
    m_search_button->setVisible(!m_searching);

    m_heading_tree->setVisible(!m_searching);
    m_treeview->setVisible(!m_searching);

    m_search_lineedit->setVisible(m_searching);
    m_flatview->setVisible(m_searching);
    m_flatview->setVisible(m_searching);
    m_heading_search->setVisible(m_searching);

    if (m_searching) {
        m_search_lineedit->setFocus();
    }

    update();
}


void DiagnosticCodeSelector::searchTextEdited(const QString& text)
{
    m_diag_filter_model->setFilterFixedString(text);
}


QModelIndex DiagnosticCodeSelector::sourceFromProxy(const QModelIndex& index)
{
    const QModelIndex intermediate = m_diag_filter_model->mapToSource(index);
    return m_flat_proxy_model->mapToSource(intermediate);
}


QModelIndex DiagnosticCodeSelector::proxyFromSource(const QModelIndex& index)
{
    const QModelIndex intermediate = m_flat_proxy_model->mapFromSource(index);
    return m_diag_filter_model->mapFromSource(intermediate);
}


void DiagnosticCodeSelector::debugLayout()
{
    layoutdumper::dumpWidgetHierarchy(this);
}
