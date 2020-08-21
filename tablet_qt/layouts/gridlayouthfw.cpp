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

// From qgridlayout.cpp:
/* ============================================================================
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
============================================================================ */

// #define DEBUG_LAYOUT_BASIC
// #define DEBUG_LAYOUT_DETAILED
// #define DEBUG_LAYOUT_COMMS

// #define DISABLE_CACHING
// #define Q_OS_MAC  // for testing only, just to be sure it compiles OK...

#define USE_WIDGETITEMHFW

#include "gridlayouthfw.h"
#include <QApplication>
#include <QDebug>
#include <QWidget>
#include <QList>
#include <QSizePolicy>
#include <QVector>
#include <QVarLengthArray>
#include "common/widgetconst.h"
#include "lib/reentrydepthguard.h"
#include "lib/sizehelpers.h"

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
#include "common/globals.h"  // for qHash(const QRect&)
#endif

using qtlayouthelpers::checkLayout;
using qtlayouthelpers::checkWidget;
using qtlayouthelpers::createWidgetItem;
using qtlayouthelpers::defaultRectOfWidth;
using qtlayouthelpers::QQLayoutStruct;
using qtlayouthelpers::qGeomCalc;
using qtlayouthelpers::qMaxExpCalc;
using qtlayouthelpers::qSmartSpacing;


// ============================================================================
// Constants
// ============================================================================

#if defined GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT && defined USE_WIDGETITEMHFW
    const bool USE_HFW_CAPABLE_ITEM = true;
#else
    const bool USE_HFW_CAPABLE_ITEM = false;
#endif


// ============================================================================
// QQGridLayoutSizeTriple
// ============================================================================

struct QQGridLayoutSizeTriple
{
    QSize min_s;
    QSize hint;
    QSize max_s;
};


// Three internal classes related to QGridLayout: (1) QQGridBox is a
// QLayoutItem with (row, column) information and (torow, tocolumn)
// information; (3) QGridLayoutData is the internal representation of a
// QGridLayout.

// RNC: renamed (Q prefix); also there is no (2) in that list.

// ============================================================================
// QQGridBox
// ============================================================================

class QQGridBox
{
    // A wrapper around a QLayoutItem (which owns widgets for the layout)
    // that adds (row, column) information.
public:
    QQGridBox(QLayoutItem* lit) {
        item_ = lit;
    }

    QQGridBox(const QLayout* l, QWidget* wid) {
        // Coded added 2020-03-12
        item_ = createWidgetItem(l, wid, USE_HFW_CAPABLE_ITEM);
    }

    ~QQGridBox() {
        delete item_;
    }

    QSize sizeHint() const {
        return item_->sizeHint();
    }

    QSize minimumSize() const {
        return item_->minimumSize();
    }

    QSize maximumSize() const {
        return item_->maximumSize();
    }

    Qt::Orientations expandingDirections() const {
        return item_->expandingDirections();
    }

    bool isEmpty() const {
        return item_->isEmpty();
    }

    bool hasHeightForWidth() const {
        return item_->hasHeightForWidth();
    }

    int heightForWidth(int w) const {
        return item_->heightForWidth(w);
    }

    void setAlignment(Qt::Alignment a) {
        item_->setAlignment(a);
    }

    void setGeometry(const QRect& r) {
        item_->setGeometry(r);
    }

    Qt::Alignment alignment() const {
        return item_->alignment();
    }

    QLayoutItem* item() {
        return item_;
    }

    void setItem(QLayoutItem* newitem) {
        item_ = newitem;
    }

    QLayoutItem* takeItem() {
        QLayoutItem* i = item_;
        item_ = nullptr;
        return i;
    }

    int hStretch() {
        return item_->widget()
                ? item_->widget()->sizePolicy().horizontalStretch()
                : 0;
    }

    int vStretch() {
        return item_->widget()
                ? item_->widget()->sizePolicy().verticalStretch()
                : 0;
    }

private:
    friend class GridLayoutHfw;

    inline int toRow(int rr) const {
        return torow >= 0 ? torow : rr - 1;
    }

    inline int toCol(int cc) const {
        return tocol >= 0 ? tocol : cc - 1;
    }

    QLayoutItem* item_;
    int row, col;
    int torow, tocol;
};


// ============================================================================
// from QGridLayoutPrivate
// ============================================================================

// void GridLayoutHfw::effectiveMargins(int* left, int* top,
//                                      int* right, int* bottom) const
Margins GridLayoutHfw::effectiveMargins(const Margins& contents_margins) const
{
    int l = contents_margins.left();
    int t = contents_margins.top();
    int r = contents_margins.right();
    int b = contents_margins.bottom();
#ifdef Q_OS_MAC
    int leftmost = INT_MAX;
    int topmost = INT_MAX;
    int rightmost = 0;
    int bottommost = 0;

    QWidget* w = 0;
    const int n = m_things.count();
    for (int i = 0; i < n; ++i) {
        QQGridBox* box = m_things.at(i);
        QLayoutItem* itm = box->item();
        w = itm->widget();
        if (w) {
            bool visual_h_reversed = m_h_reversed != (w->layoutDirection() == Qt::RightToLeft);
            QRect lir = itm->geometry();
            QRect wr = w->geometry();
            if (box->col <= leftmost) {
                if (box->col < leftmost) {
                    // we found an item even closer to the margin, discard.
                    leftmost = box->col;
                    if (visual_h_reversed) {
                        r = contents_margins.right();  // m_right_margin;
                    } else {
                        l = contents_margins.left();  // m_left_margin;
                    }
                }
                if (visual_h_reversed) {
                    r = qMax(r, wr.right() - lir.right());
                } else {
                    l = qMax(l, lir.left() - wr.left());
                }
            }
            if (box->row <= topmost) {
                if (box->row < topmost) {
                    // we found an item even closer to the margin, discard.
                    topmost = box->row;
                    if (m_v_reversed) {
                        b = contents_margins.bottom();  // m_bottom_margin;
                    } else {
                        t = contents_margins.top();  // m_top_margin;
                    }
                }
                if (m_v_reversed) {
                    b = qMax(b, wr.bottom() - lir.bottom());
                } else {
                    t = qMax(t, lir.top() - wr.top());
                }
            }
            if (box->toCol(m_ncol) >= rightmost) {
                if (box->toCol(m_ncol) > rightmost) {
                    // we found an item even closer to the margin, discard.
                    rightmost = box->toCol(m_ncol);
                    if (visual_h_reversed) {
                        l = contents_margins.left();  // m_left_margin;
                    } else {
                        r = contents_margins.right();  // m_right_margin;
                    }
                }
                if (visual_h_reversed) {
                    l = qMax(l, lir.left() - wr.left());
                } else {
                    r = qMax(r, wr.right() - lir.right());
                }

            }
            if (box->toRow(m_nrow) >= bottommost) {
                if (box->toRow(m_nrow) > bottommost) {
                    // we found an item even closer to the margin, discard.
                    bottommost = box->toRow(m_nrow);
                    if (m_v_reversed) {
                        t = contents_margins.top();  // m_top_margin;
                    } else {
                        b = contents_margins.bottom();  // m_bottom_margin;
                    }
                }
                if (m_v_reversed) {
                    t = qMax(t, lir.top() - wr.top());
                } else {
                    b = qMax(b, wr.bottom() - lir.bottom());
                }
            }
        }
    }

#endif
    // if (left) {
    //     *left = l;
    // }
    // if (top) {
    //     *top = t;
    // }
    // if (right) {
    //     *right = r;
    // }
    // if (bottom) {
    //     *bottom = b;
    // }

    return Margins(l, t, r, b);
}


void GridLayoutHfw::deleteAll()
{
    while (!m_things.isEmpty()) {
        delete m_things.takeFirst();
    }
}


QSize GridLayoutHfw::findSize(const GeomInfo& gi,
                              int QLayoutStruct::* size) const
{
    // "size" is a pointer to an integer non-static member of QLayoutStruct;
    // http://en.cppreference.com/w/cpp/language/pointer#Pointers_to_data_members

    int w = 0;
    int h = 0;

    const QVector<QLayoutStruct>& rowdata = gi.m_has_hfw ? gi.m_hfw_data
                                                         : gi.m_row_data;

    for (int r = 0; r < m_nrow; ++r) {
        h += rowdata.at(r).*size + rowdata.at(r).spacing;
    }
    for (int c = 0; c < m_ncol; ++c) {
        w += gi.m_col_data.at(c).*size + gi.m_col_data.at(c).spacing;
    }

    w = qMin(QLAYOUTSIZE_MAX, w);
    h = qMin(QLAYOUTSIZE_MAX, h);

    return QSize(w, h);
}


void GridLayoutHfw::setSize(const int r, const int c)
{
    if (static_cast<int>(m_r_stretches.size()) < r) {
        const int new_r = qMax(r, m_nrow * 2);
        m_r_stretches.resize(new_r);
        m_r_min_heights.resize(new_r);
        for (int i = m_nrow; i < new_r; i++) {
            m_r_stretches[i] = 0;
            m_r_min_heights[i] = 0;
        }
    }
    if (static_cast<int>(m_c_stretches.size()) < c) {
        const int new_c = qMax(c, m_ncol * 2);
        m_c_stretches.resize(new_c);
        m_c_min_widths.resize(new_c);
        for (int i = m_ncol; i < new_c; i++) {
            m_c_stretches[i] = 0;
            m_c_min_widths[i] = 0;
        }
    }
    m_nrow = r;
    m_ncol = c;
    setDirty();
}


void GridLayoutHfw::setNextPosAfter(const int row, const int col)
{
    if (m_add_vertical) {
        if (col > m_next_c || (col == m_next_c && row >= m_next_r)) {
            m_next_r = row + 1;
            m_next_c = col;
            if (m_next_r >= m_nrow) {
                m_next_r = 0;
                m_next_c++;
            }
        }
    } else {
        if (row > m_next_r || (row == m_next_r && col >= m_next_c)) {
            m_next_r = row;
            m_next_c = col + 1;
            if (m_next_c >= m_ncol) {
                m_next_c = 0;
                m_next_r++;
            }
        }
    }
}


void GridLayoutHfw::add(QQGridBox* box, const int row, const int col)
{
    expand(row + 1, col + 1);
    box->row = box->torow = row;
    box->col = box->tocol = col;
    m_things.append(box);
    setDirty();
    setNextPosAfter(row, col);
}


void GridLayoutHfw::add(QQGridBox* box,
                        const int row1, const int row2,
                        const int col1, int col2)
{
    if (Q_UNLIKELY(row2 >= 0 && row2 < row1)) {
        qWarning() << "QGridLayout: Multi-cell from-row greater than to-row";
    }
    if (Q_UNLIKELY(col2 >= 0 && col2 < col1)) {
        qWarning() << "QGridLayout: Multi-cell from-col greater than to-col";
    }
    if (row1 == row2 && col1 == col2) {
        add(box, row1, col1);
        return;
    }
    expand(qMax(row1, row2) + 1, qMax(col1, col2) + 1);
    box->row = row1;
    box->col = col1;

    box->torow = row2;
    box->tocol = col2;

    m_things.append(box);
    setDirty();
    if (col2 < 0) {
        col2 = m_ncol - 1;
    }

    setNextPosAfter(row2, col2);
}


void GridLayoutHfw::addData(GeomInfo& gi, QQGridBox* box,
                            const QQGridLayoutSizeTriple& sizes,
                            const bool r, const bool c) const
{
    const QWidget* widget = box->item()->widget();

    if (box->isEmpty() && widget) {
        return;
    }

    if (c) {
        QLayoutStruct* data = &gi.m_col_data[box->col];
        if (!m_c_stretches.at(box->col)) {
            data->stretch = qMax(data->stretch, box->hStretch());
        }
        data->size_hint = qMax(sizes.hint.width(), data->size_hint);
        data->minimum_size = qMax(sizes.min_s.width(), data->minimum_size);

        qMaxExpCalc(data->maximum_size, data->expansive, data->empty,
                    sizes.max_s.width(),
                    box->expandingDirections() & Qt::Horizontal,
                    box->isEmpty());
    }
    if (r) {
        QLayoutStruct* data = &gi.m_row_data[box->row];
        if (!m_r_stretches.at(box->row)) {
            data->stretch = qMax(data->stretch, box->vStretch());
        }
        data->size_hint = qMax(sizes.hint.height(), data->size_hint);
        data->minimum_size = qMax(sizes.min_s.height(), data->minimum_size);

        qMaxExpCalc(data->maximum_size, data->expansive, data->empty,
                    sizes.max_s.height(),
                    box->expandingDirections() & Qt::Vertical,
                    box->isEmpty());
    }
}


static void initEmptyMultiBox(QVector<QQLayoutStruct>& chain,
                              const int start, const int end)
{
    for (int i = start; i <= end; i++) {
        QQLayoutStruct* data = &chain[i];
        if (data->empty && data->maximum_size == 0) {  // truly empty box
            data->maximum_size = QWIDGETSIZE_MAX;
        }
        data->empty = false;
    }
}


static void distributeMultiBox(QVector<QQLayoutStruct>& chain,
                               const int start, const int end,
                               const int min_size, const int size_hint,
                               const QVector<int>& stretch_array,
                               const int stretch)
{
    // This function distributes objects along a single dimension.
#ifdef DEBUG_LAYOUT_DETAILED
    qDebug().nospace()
            << Q_FUNC_INFO
            << "- starting chain=" << chain
            << ", start=" << start
            << ", end=" << end
            << ", min_size=" << min_size
            << ", size_hint=" << size_hint
            << ", stretch_array=" << stretch_array
            << ", stretch=" << stretch;
#endif


    int i;
    int w = 0;  // [RNC] total minimum width (or height if vertical)
    int wh = 0;  // [RNC] total hint width (or height if vertical)
    int max = 0;  // [RNC] total max width (or height if vertical)

    for (i = start; i <= end; i++) {
        QQLayoutStruct* data = &chain[i];
        w += data->minimum_size;
        wh += data->size_hint;
        max += data->maximum_size;
        if (stretch_array.at(i) == 0) {
            data->stretch = qMax(data->stretch, stretch);
        }

        if (i != end) {
            const int spacing = data->spacing;
            w += spacing;
            wh += spacing;
            max += spacing;
        }
    }

    if (max < min_size) {  // implies w < min_size

        // We must increase the maximum size of at least one of the
        // items. qGeomCalc() will put the extra space in between the
        // items. We must recover that extra space and put it
        // somewhere. It does not really matter where, since the user
        // can always specify stretch factors and avoid this code.

        qGeomCalc(chain, start, end - start + 1, 0, min_size);
        int pos = 0;
        for (i = start; i <= end; i++) {
            QQLayoutStruct* data = &chain[i];
            const int next_pos = (i == end) ? min_size : chain.at(i + 1).pos;
            int real_size = next_pos - pos;
            if (i != end) {
                real_size -= data->spacing;
            }
            if (data->minimum_size < real_size) {
                data->minimum_size = real_size;
            }
            if (data->maximum_size < data->minimum_size) {
                data->maximum_size = data->minimum_size;
            }
            pos = next_pos;
        }
    } else if (w < min_size) {  // [RNC] minimum is less than required, but maximum is OK?
        qGeomCalc(chain, start, end - start + 1, 0, min_size);
        for (i = start; i <= end; i++) {
            QQLayoutStruct* data = &chain[i];
            if (data->minimum_size < data->size) {
                data->minimum_size = data->size;
            }
        }
    }

    // [RNC] we now know that maximum_size is OK, but redistribute to get closer to hints?
    if (wh < size_hint) {
        qGeomCalc(chain, start, end - start + 1, 0, size_hint);
        for (i = start; i <= end; i++) {
            QQLayoutStruct* data = &chain[i];
            if (data->size_hint < data->size) {
                data->size_hint = data->size;
            }
        }
    }

#ifdef DEBUG_LAYOUT_DETAILED
    qDebug() << "... modified chain:" << chain;
#endif
}


static QQGridBox* &gridAt(QQGridBox* grid[],
                          int r, int c, const int ncols,
                          const Qt::Orientation orientation = Qt::Vertical)
{
    if (orientation == Qt::Horizontal) {
        qSwap(r, c);
    }
    return grid[(r * ncols) + c];
}


void GridLayoutHfw::setupSpacings(QVector<QLayoutStruct>& chain,
                                  QQGridBox* grid[], int fixed_spacing,
                                  Qt::Orientation orientation) const
{
    int num_rows = m_nrow;       // or columns if orientation is horizontal
    int num_columns = m_ncol;    // or rows if orientation is horizontal

    if (orientation == Qt::Horizontal) {
        qSwap(num_rows, num_columns);
    }

    QStyle* style = nullptr;
    if (fixed_spacing < 0) {
        if (QWidget* parent_widget = parentWidget()) {
            style = parent_widget->style();
        }
    }

    for (int c = 0; c < num_columns; ++c) {
        QQGridBox* previous_box = nullptr;
        int previous_row = -1;       // previous *non-empty* row

        for (int r = 0; r < num_rows; ++r) {
            if (chain.at(r).empty) {
                continue;
            }

            QQGridBox* box = gridAt(grid, r, c, m_ncol, orientation);
            if (previous_row != -1 && (!box || previous_box != box)) {
                int spacing = fixed_spacing;
                if (spacing < 0) {
                    QSizePolicy::ControlTypes controlTypes1 = QSizePolicy::DefaultType;
                    QSizePolicy::ControlTypes controlTypes2 = QSizePolicy::DefaultType;
                    if (previous_box) {
                        controlTypes1 = previous_box->item()->controlTypes();
                    }
                    if (box) {
                        controlTypes2 = box->item()->controlTypes();
                    }

                    if ((orientation == Qt::Horizontal && m_h_reversed)
                            || (orientation == Qt::Vertical && m_v_reversed)) {
                        qSwap(controlTypes1, controlTypes2);
                    }

                    if (style) {
                        spacing = style->combinedLayoutSpacing(
                                    controlTypes1, controlTypes2,
                                    orientation, nullptr, parentWidget());
                    }
                } else {
                    if (orientation == Qt::Vertical) {
                        QQGridBox* sibling = m_v_reversed ? previous_box : box;
                        if (sibling) {
                            QWidget* wid = sibling->item()->widget();
                            if (wid) {
                                spacing = qMax(
                                        spacing,
                                        sibling->item()->geometry().top() -
                                            wid->geometry().top() );
                            }
                        }
                    }
                }

                if (spacing > chain.at(previous_row).spacing) {
                    chain[previous_row].spacing = spacing;
                }
            }

            previous_box = box;
            previous_row = r;
        }
    }
}


void GridLayoutHfw::addHfwData(GeomInfo& gi, QQGridBox* box, int width) const
{
    QVector<QLayoutStruct>& rdata = gi.m_hfw_data;
    QLayoutStruct& ls = rdata[box->row];  // May have been influenced by OTHER items already

    // We are setting properties for the QLayoutStruct, which represents an
    // entire row.

    if (box->hasHeightForWidth()) {

        const int hfw = box->heightForWidth(width);
#if 0
        // DON'T do this; it breaks things quite badly!
        // 2020-03-12
        QLayoutItem* item = box->item();
        QWidget* widget = item->widget();
        const bool can_shrink_vertically =
                sizehelpers::canHFWPolicyShrinkVertically(widget->sizePolicy());
        const int min_h = can_shrink_vertically ? item->minimumSize().height()
                                                : hfw;
        ls.minimum_size = qMax(min_h, ls.minimum_size);
#else
        ls.minimum_size = qMax(hfw, ls.minimum_size);
#endif
        ls.size_hint = qMax(hfw, ls.size_hint);
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
        if (ls.maximum_size >= QLAYOUTSIZE_MAX) {
            // unset, so set maximum
            ls.maximum_size = qMin(ls.size_hint, QLAYOUTSIZE_MAX);
        } else {
            // already set; we'll need to increase the maximum for the row,
            // even if it's beyond the maximum for one of the widgets
            ls.maximum_size = qMax(ls.maximum_size, ls.size_hint);
        }
#endif

    } else {

        const int hint_h = box->sizeHint().height();
        const int min_h = box->minimumSize().height();
        // Note:
        //  QQGridBox::minimumSize()
        //  -> QLayoutItem::minimumSize() [pure virtual]
        //  -> [generally] QWidgetItemV2::minimumSize()
        //  -> QWidgetItem::minimumSize()
        //  -> QSize qSmartMinSize(const QWidget *w) [from qlayoutengine_p.h / qlayoutengine.cpp]
        //  -> picks up QWidget::minimumSizeHint(), as well as sizeHint(),
        //     minimumSize(), maximumSize(), sizePolicy()
        //  -> QSize qSmartMinSize(...)
        //
        // QLayoutItem does not offer minimumSizeHint().

        ls.minimum_size = qMax(min_h, ls.minimum_size);
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
        ls.size_hint = qMax(qMax(hint_h, ls.size_hint), ls.minimum_size);

        // int max_h = box->maximumSize().height();
        if (ls.maximum_size >= QLAYOUTSIZE_MAX) {
            // unset, so set maximum
            ls.maximum_size = qMin(QLAYOUTSIZE_MAX, hint_h);
        } else {
            // already set; we'll need to increase the maximum for the row,
            // even if it's beyond the maximum for one of the widgets
            ls.maximum_size = qMax(ls.maximum_size, hint_h);
        }
        // Many widgets have a maximum size that's giant, so we can't use
        // maximumSize(), really, or the grid will grow vertically as we
        // shrink it horizontally, but then fail to shrink vertically as we
        // expand it horizontally. So use hint_h instead.
#else
        ls.size_hint = qMax(hint.height(), ls.size_hint);
#endif

    }
}


void GridLayoutHfw::distribute(const QRect& layout_rect)
{
#ifdef DEBUG_LAYOUT_BASIC
    qDebug() << Q_FUNC_INFO << "layout_rect" << layout_rect;
#endif

    bool visual_h_reversed = m_h_reversed;
    QWidget* parent = parentWidget();
    if (parent && parent->isRightToLeft()) {
        visual_h_reversed = !visual_h_reversed;
    }

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    GeomInfo gi = getGeomInfo(layout_rect);
#else
    GeomInfo gi = getGeomInfo();
#endif

#ifdef DEBUG_LAYOUT_BASIC
    qDebug() << gi;
#endif

    // int left, top, right, bottom;
    // effectiveMargins(&left, &top, &right, &bottom);
    // r.adjust(+left, +top, -right, -bottom);
    QRect r = getContentsRect(layout_rect);
    // r is now the actual rectangle we will lay out into

#ifndef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    // We can't do it in getGeomInfo() in this case, because that doesn't
    // get the rectangle.

    // Work out column widths
    qGeomCalc(gi.m_col_data, 0, m_ncol, r.x(), r.width());
    // Now work out row heights
    if (gi.m_has_hfw) {
        qGeomCalc(gi.m_hfw_data, 0, m_nrow, r.y(), r.height());
    } else {
        qGeomCalc(gi.m_row_data, 0, m_nrow, r.y(), r.height());
    }
#endif

    // RNC: rect is a member of QLayoutPrivate, which we're not using.
    // In QLayoutPrivate::doResize, we see q->setGeometry(rect);
    // Therefore I think we can recover the information with:
    QRect rect = geometry();  // RNC

    const bool reverse = (
                (r.bottom() > rect.bottom()) ||
                (r.bottom() == rect.bottom() &&
                    ((r.right() > rect.right()) != visual_h_reversed)));
    const int n = m_things.size();
    const QVector<QLayoutStruct>& rowdata = gi.m_has_hfw ? gi.m_hfw_data
                                                         : gi.m_row_data;
    for (int i = 0; i < n; ++i) {
        QQGridBox* box = m_things.at(reverse ? n-i-1 : i);
        const int r1 = box->row;
        const int c1 = box->col;
        const int r2 = box->toRow(m_nrow);
        const int c2 = box->toCol(m_ncol);

        int x = gi.m_col_data.at(c1).pos;
        int y = rowdata.at(r1).pos;
        const int x2p = gi.m_col_data.at(c2).pos + gi.m_col_data.at(c2).size; // x2+1
        const int y2p = rowdata.at(r2).pos + rowdata.at(r2).size;    // y2+1
        const int w = x2p - x;
        const int h = y2p - y;

        if (visual_h_reversed) {
            x = r.left() + r.right() - x - w + 1;
        }
        if (m_v_reversed) {
            y = r.top() + r.bottom() - y - h + 1;
        }

        QRect childrect(x, y, w, h);
        box->setGeometry(childrect);
        // ... will call QLayoutItem::setGeometry() and then, for widgets,
        // typically QWidgetItem::setGeometry() [in qlayoutitem.cpp]
#ifdef DEBUG_LAYOUT_DETAILED
        QString rowdesc = (r1 == r2)
                ? QString("row=%1").arg(r1)
                : QString("rows=%1-%2").arg(r1).arg(r2);
        QString coldesc = (c1 == c2)
                ? QString("col=%1").arg(c1)
                : QString("cols=%1-%2").arg(c1).arg(c2);
        qDebug().nospace().noquote()
                << "[distribute()] ... item " << i
                << "[" << rowdesc << ", " << coldesc << "]"
                << " given setGeometry() instruction " << childrect;
#endif
    }
}


QLayoutItem* GridLayoutHfw::replaceAt(int index, QLayoutItem* newitem)
{
    if (!newitem) {
        return nullptr;
    }
    QLayoutItem* item = nullptr;
    QQGridBox* b = m_things.value(index);
    if (b) {
        item = b->takeItem();
        b->setItem(newitem);
    }
    return item;
}


// ============================================================================
// from QGridLayout
// ============================================================================

GridLayoutHfw::GridLayoutHfw(QWidget* parent) :
    QLayout(parent),
    m_reentry_depth(0)  // RNC
{
    m_add_vertical = false;
    setDirty();
    m_nrow = m_ncol = 0;
    m_next_r = m_next_c = 0;
    m_h_reversed = false;
    m_v_reversed = false;
    m_horizontal_spacing = -1;
    m_vertical_spacing = -1;

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    m_width_last_size_constraints_based_on = -1;
    m_rect_for_next_size_constraints = qtlayouthelpers::QT_DEFAULT_RECT;
#else
    m_cached_hfw_width = -1;
#endif

    expand(1, 1);
}


// \internal (mostly)
//
// Sets the positioning mode used by addItem(). If \a orient is
// Qt::Horizontal, this layout is expanded to \a n columns, and items
// will be added columns-first. Otherwise it is expanded to \a n rows and
// items will be added rows-first.

void GridLayoutHfw::setDefaultPositioning(int n, Qt::Orientation orient)
{
    if (orient == Qt::Horizontal) {
        expand(1, n);
        m_add_vertical = false;
    } else {
        expand(n, 1);
        m_add_vertical = true;
    }
}


// Destroys the grid layout. Geometry management is terminated if
// this is a top-level grid.
//
// The layout's widgets aren't destroyed.

GridLayoutHfw::~GridLayoutHfw()
{
    deleteAll();
}


void GridLayoutHfw::setHorizontalSpacing(int spacing)
{
    m_horizontal_spacing = spacing;
    invalidate();
}


int GridLayoutHfw::horizontalSpacing() const
{
    if (m_horizontal_spacing >= 0) {
        return m_horizontal_spacing;
    }
    return qSmartSpacing(this, QStyle::PM_LayoutHorizontalSpacing);
}


void GridLayoutHfw::setVerticalSpacing(int spacing)
{
    m_vertical_spacing = spacing;
    invalidate();
}


int GridLayoutHfw::verticalSpacing() const
{
    if (m_vertical_spacing >= 0) {
        return m_vertical_spacing;
    }
    return qSmartSpacing(this, QStyle::PM_LayoutVerticalSpacing);
}


void GridLayoutHfw::setSpacing(int spacing)
{
    m_horizontal_spacing = m_vertical_spacing = spacing;
    invalidate();
}


int GridLayoutHfw::spacing() const
{
    const int h_spacing = horizontalSpacing();
    if (h_spacing == verticalSpacing()) {
        return h_spacing;
    }
    return -1;
}


int GridLayoutHfw::rowCount() const
{
    return numRows();
}


int GridLayoutHfw::columnCount() const
{
    return numCols();
}


QSize GridLayoutHfw::sizeHint() const
{
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
    m_width_last_size_constraints_based_on = m_rect_for_next_size_constraints.width();
#else
    GeomInfo gi = getGeomInfo();
#endif
#ifdef DEBUG_LAYOUT_COMMS
    qDebug().nospace() << Q_FUNC_INFO << " -> " << gi.m_size_hint
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
                       << " (based on notional width of "
                       << m_width_last_size_constraints_based_on << ")"
#endif
                          ;
#endif
    return gi.m_size_hint;
}


QSize GridLayoutHfw::minimumSize() const
{
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
    m_width_last_size_constraints_based_on = m_rect_for_next_size_constraints.width();
#else
    GeomInfo gi = getGeomInfo();
#endif
#ifdef DEBUG_LAYOUT_COMMS
    qDebug().nospace() << Q_FUNC_INFO << " -> " << gi.m_min_size
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
                       << " (based on notional width of "
                       << m_width_last_size_constraints_based_on << ")"
#endif
                          ;
#endif
    return gi.m_min_size;
}


QSize GridLayoutHfw::maximumSize() const
{
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
    m_width_last_size_constraints_based_on = m_rect_for_next_size_constraints.width();
#else
    GeomInfo gi = getGeomInfo();
#endif
    QSize s = gi.m_max_size.boundedTo(QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX));
    if (alignment() & Qt::AlignHorizontal_Mask) {
        s.setWidth(QLAYOUTSIZE_MAX);
    }
    if (alignment() & Qt::AlignVertical_Mask) {
        s.setHeight(QLAYOUTSIZE_MAX);
    }
#ifdef DEBUG_LAYOUT_COMMS
    qDebug().nospace() << Q_FUNC_INFO << " -> " << s
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
                       << " (based on notional width of "
                       << m_width_last_size_constraints_based_on << ")"
#endif
                          ;
#endif
    return s;
}


bool GridLayoutHfw::hasHeightForWidth() const
{
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
#else
    GeomInfo gi = getGeomInfo();
#endif
    return gi.m_has_hfw;
}


int GridLayoutHfw::heightForWidth(int w) const
{
    if (!hasHeightForWidth()) {
        return -1;
    }
    const GeomInfo gi = getGeomInfoForHfw(w);
    return gi.m_hfw_height;
}


int GridLayoutHfw::minimumHeightForWidth(int w) const
{
    if (!hasHeightForWidth()) {
        return -1;
    }
    const GeomInfo gi = getGeomInfoForHfw(w);
    return gi.m_hfw_min_height;
}


int GridLayoutHfw::count() const
{
    return m_things.count();
}


QLayoutItem* GridLayoutHfw::itemAt(int index) const
{
    if (index < m_things.count()) {
        return m_things.at(index)->item();
    }
    return nullptr;
}


QLayoutItem* GridLayoutHfw::itemAtPosition(int row, int column) const
{
    const int n = m_things.count();
    for (int i = 0; i < n; ++i) {
        QQGridBox* box = m_things.at(i);
        if (row >= box->row && row <= box->toRow(m_nrow)
                && column >= box->col && column <= box->toCol(m_ncol)) {
            return box->item();
        }
    }
    return nullptr;
}


QLayoutItem* GridLayoutHfw::takeAt(int index)
{
    if (index < m_things.count()) {
        if (QQGridBox* b = m_things.takeAt(index)) {
            QLayoutItem* item = b->takeItem();
            if (QLayout* l = item->layout()) {
                // sanity check in case the user passed something weird to QObject::setParent()
                if (l->parent() == this) {
                    l->setParent(nullptr);
                }
            }
            delete b;
            return item;
        }
    }
    return nullptr;
}


void GridLayoutHfw::getItemPosition(int index, int* row, int* column,
                                    int* row_span, int* column_span) const
{
    if (index < m_things.count()) {
        const QQGridBox* b =  m_things.at(index);
        const int toRow = b->toRow(m_nrow);
        const int toCol = b->toCol(m_ncol);
        *row = b->row;
        *column = b->col;
        *row_span = toRow - *row + 1;
        *column_span = toCol - *column +1;
    }
}


void GridLayoutHfw::setGeometry(const QRect& rect)
{
    // ------------------------------------------------------------------------
    // Prevent infinite recursion
    // ------------------------------------------------------------------------
    if (m_reentry_depth >= widgetconst::SET_GEOMETRY_MAX_REENTRY_DEPTH) {
        return;
    }
    ReentryDepthGuard guard(m_reentry_depth);
    Q_UNUSED(guard)

    // ------------------------------------------------------------------------
    // Initialize
    // ------------------------------------------------------------------------
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    QRect r = rect;  // we may modify it
#else
    const QRect& r = rect;  // just a reference
#endif
    // RNC: r is the overall rectangle for the layout

    // ------------------------------------------------------------------------
    // Announce
    // ------------------------------------------------------------------------
#ifdef DEBUG_LAYOUT_BASIC
    qDebug() << Q_FUNC_INFO;
#endif

    // ------------------------------------------------------------------------
    // Skip because nothing's changed?
    // ------------------------------------------------------------------------
#ifndef DISABLE_CACHING
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    const bool geometry_previously_calculated = m_geom_cache.contains(r);
    if (geometry_previously_calculated && r == geometry()) {
#else
    if (!m_dirty && r == geometry()) {
#endif
        // Exactly the same geometry as last time, and we're all set up.
#ifdef DEBUG_LAYOUT_BASIC
        qDebug() << "[setGeometry()] ... nothing to do, for" << r;
#endif
        return;
    }
#endif

    // ------------------------------------------------------------------------
    // Recalculate geometry
    // ------------------------------------------------------------------------
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    const GeomInfo gi = getGeomInfo(r);
#else
    GeomInfo gi = getGeomInfo();
#endif

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    if (gi.m_has_hfw) {
        if (r.width() != m_width_last_size_constraints_based_on) {
#ifdef DEBUG_LAYOUT_BASIC
            qDebug().nospace()
                    << "[setGeometry()] ... resetting width hints, for " << r
                    << " (because width=" << r.width()
                    << " but last size constraints were based on width of "
                    << m_width_last_size_constraints_based_on << ")";
#endif
            m_rect_for_next_size_constraints = r;
        }
    }
    QWidget* parent = parentWidget();
    const Margins parent_margins = Margins::getContentsMargins(parent);
    if (!parent) {
        qWarning() << Q_FUNC_INFO << "Layout has no parent widget";
    }
    const int parent_new_height = getParentTargetHeight(parent, parent_margins, gi);
    if (parent_new_height != -1) {
        r.setHeight(parent_new_height - parent_margins.totalHeight());  // change
    }
#endif

    // ------------------------------------------------------------------------
    // Lay out children and call QLayout::setGeometry()
    // ------------------------------------------------------------------------
    // RNC: note that distribute() is the main thinking function here
    distribute(r);
    QLayout::setGeometry(r);

    // ------------------------------------------------------------------------
    // Ask our parent to resize, if necessary
    // ------------------------------------------------------------------------
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    if (parent_new_height != -1) {
        const bool change = !sizehelpers::fixedHeightEquals(parent,
                                                            parent_new_height);
        // Don't resize if the parent is already trying its best.
        if (change) {
#ifdef DEBUG_LAYOUT_COMMS
            qDebug()
                    << Q_FUNC_INFO
                    << "Asking parent to change height from"
                    << parent->geometry().height()
                    << "to"
                    << parent_new_height;
#endif
            parent->setFixedHeight(parent_new_height);  // RISK OF INFINITE RECURSION
            // ... hence the ReentryDepthGuard
            parent->updateGeometry();
        }
    }
#endif
}


#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
int GridLayoutHfw::getParentTargetHeight(QWidget* parent,
                                         const Margins& parent_margins,
                                         const GeomInfo& gi) const
{
    // Returns -1 if no change required.

    if (!parent || !gi.m_has_hfw) {
        return -1;
    }
    int parent_new_height = -1;

    // Remember we may also have a mix of hfw and non-hfw items; the
    // non-hfw may have min/max heights that differ.
    int target_min_height = gi.m_min_size.height();
    int target_max_height = gi.m_max_size.height();

    target_min_height += parent_margins.totalHeight();
    target_max_height += parent_margins.totalHeight();

    if (parent->geometry().height() < target_min_height) {
#ifdef DEBUG_LAYOUT_BASIC
        qDebug().nospace()
                << "[getParentTargetHeight()] "
                << "... will increase parent height to " << target_min_height
                << " (was " << parent->geometry().height()
                << ", below our min of " << target_min_height
                << " [including parent margin height of "
                << parent_margins.totalHeight() << "])";
#endif
        parent_new_height = target_min_height;
    }
    if (parent->geometry().height() > target_max_height) {
#ifdef DEBUG_LAYOUT_BASIC
        qDebug().nospace()
                << "[getParentTargetHeight()] "
                << "... will decrease parent height to " << target_max_height
                << " (was " << parent->geometry().height()
                << ", above our max of " << target_max_height
                << " [including parent margin height of "
                << parent_margins.totalHeight() << "])";
#endif
        parent_new_height = target_max_height;
    }
    return parent_new_height;
}
#endif


QRect GridLayoutHfw::cellRect(const GeomInfo& gi, int row, int column) const
{
    if (row < 0 || row >= m_nrow || column < 0 || column >= m_ncol) {
        return QRect();
    }

    const QVector<QLayoutStruct>* rdataptr;
    if (gi.m_has_hfw) {
        rdataptr = &gi.m_hfw_data;
    } else {
        rdataptr = &gi.m_row_data;
    }
    return QRect(gi.m_col_data.at(column).pos, rdataptr->at(row).pos,
                 gi.m_col_data.at(column).size, rdataptr->at(row).size);
}


void GridLayoutHfw::addItem(QLayoutItem* item)
{
    int r, c;
    getNextPos(r, c);
    addItem(item, r, c);
}


void GridLayoutHfw::addItem(QLayoutItem* item, int row, int column,
                            int row_span, int column_span,
                            Qt::Alignment alignment)
{
    auto b = new QQGridBox(item);
    b->setAlignment(alignment);
    add(b,
        row, (row_span < 0) ? -1 : row + row_span - 1,
        column, (column_span < 0) ? -1 : column + column_span - 1);
    invalidate();
}


void GridLayoutHfw::addWidget(QWidget* w)
{
    // The original, as per qgridlayout.h, is:
    //      QLayout::addWidget(w);
    QWidgetItem* b = createWidgetItem(this, w, USE_HFW_CAPABLE_ITEM);
    addItem(b);
}


void GridLayoutHfw::addWidget(QWidget* widget, int row, int column,
                              Qt::Alignment alignment)
{
    if (!checkWidget(widget, this)) {
        return;
    }
    if (Q_UNLIKELY(row < 0 || column < 0)) {
        qWarning("QGridLayout: Cannot add %s/%s to %s/%s at row %d column %d",
                 widget->metaObject()->className(),
                 widget->objectName().toLocal8Bit().data(),
                 metaObject()->className(),
                 objectName().toLocal8Bit().data(), row, column);
        return;
    }
    addChildWidget(widget);
    QWidgetItem* b = createWidgetItem(this, widget, USE_HFW_CAPABLE_ITEM);
    addItem(b, row, column, 1, 1, alignment);
}


void GridLayoutHfw::addWidget(QWidget* widget, int from_row, int from_column,
                              int row_span, int column_span,
                              Qt::Alignment alignment)
{
    if (!checkWidget(widget, this)) {
        return;
    }
    const int toRow = (row_span < 0) ? -1 : from_row + row_span - 1;
    const int toColumn = (column_span < 0) ? -1 : from_column + column_span - 1;
    addChildWidget(widget);
    auto b = new QQGridBox(this, widget);
    b->setAlignment(alignment);
    add(b, from_row, toRow, from_column, toColumn);
    invalidate();
}


void GridLayoutHfw::addLayout(QLayout* layout, int row, int column,
                              Qt::Alignment alignment)
{
    if (!checkLayout(layout, this)) {
        return;
    }
    if (!adoptLayout(layout)) {
        return;
    }
    auto b = new QQGridBox(layout);
    b->setAlignment(alignment);
    add(b, row, column);
}


void GridLayoutHfw::addLayout(QLayout* layout, int row, int column,
                              int row_span, int column_span,
                              Qt::Alignment alignment)
{
    if (!checkLayout(layout, this)) {
        return;
    }
    if (!adoptLayout(layout)) {
        return;
    }
    auto b = new QQGridBox(layout);
    b->setAlignment(alignment);
    add(b,
        row, (row_span < 0) ? -1 : row + row_span - 1,
        column, (column_span < 0) ? -1 : column + column_span - 1);
}


void GridLayoutHfw::setRowStretch(int row, int stretch)
{
    expand(row + 1, 0);
    m_r_stretches[row] = stretch;
    invalidate();
}


int GridLayoutHfw::rowStretch(int row) const
{
    return m_r_stretches.at(row);
}


int GridLayoutHfw::columnStretch(int column) const
{
    return m_c_stretches.at(column);
}


void GridLayoutHfw::setColumnStretch(int column, int stretch)
{
    expand(0, column + 1);
    m_c_stretches[column] = stretch;
    invalidate();
}


void GridLayoutHfw::expand(int rows, int cols)  // was in QGridLayoutPrivate
{
    setSize(qMax(rows, m_nrow), qMax(cols, m_ncol));
}


void GridLayoutHfw::setRowMinimumHeight(int row, int min_size)
{
    expand(row + 1, 0);
    m_r_min_heights[row] = min_size;
    invalidate();
}


int GridLayoutHfw::rowMinimumHeight(int row) const
{
    return rowSpacing(row);
}


void GridLayoutHfw::setColumnMinimumWidth(int column, int min_size)
{
    expand(0, column + 1);
    m_c_min_widths[column] = min_size;
    invalidate();
}


int GridLayoutHfw::columnMinimumWidth(int column) const
{
    return colSpacing(column);
}


Qt::Orientations GridLayoutHfw::expandingDirections() const
{
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
#else
    GeomInfo gi = getGeomInfo();
#endif
    return gi.m_expanding;
}


void GridLayoutHfw::setOriginCorner(Qt::Corner corner)
{
    setReversed(corner == Qt::BottomLeftCorner || corner == Qt::BottomRightCorner,
                corner == Qt::TopRightCorner || corner == Qt::BottomRightCorner);
}


Qt::Corner GridLayoutHfw::originCorner() const
{
    if (horReversed()) {
        return verReversed() ? Qt::BottomRightCorner : Qt::TopRightCorner;
    }
    return verReversed() ? Qt::BottomLeftCorner : Qt::TopLeftCorner;
}


void GridLayoutHfw::invalidate()
{
    setDirty();
    QLayout::invalidate();
}


// ============================================================================
// RNC additional
// ============================================================================

inline void GridLayoutHfw::setDirty()
{
    // Was inline in header
    // http://stackoverflow.com/questions/3992980/c-inline-member-function-in-cpp-file
    // https://isocpp.org/wiki/faq/inline-functions#where-to-put-inline-keyword

#ifdef DEBUG_LAYOUT_DETAILED
    qDebug() << Q_FUNC_INFO;
#endif
    m_dirty = true;
#ifndef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    m_cached_hfw_width = -1;
    m_effective_margins.clear();
#endif
}


Margins GridLayoutHfw::effectiveMargins() const
{
    // RNC: cache added, because we use this quite a lot, and (at least for
    // the #ifdef Q_OS_MAC) there's a bit of thinking involved.
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    if (m_dirty) {
        clearCaches();
    }
#endif
    if (m_effective_margins.isZero()) {
        Margins contents_margins = Margins::getContentsMargins(this);
        m_effective_margins = effectiveMargins(contents_margins);
    }
    return m_effective_margins;
}


#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
void GridLayoutHfw::clearCaches() const
{
    m_geom_cache.clear();
    m_effective_margins.clear();
    m_dirty = false;
}
#endif


#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
GridLayoutHfw::GeomInfo GridLayoutHfw::getGeomInfo(const QRect& layout_rect) const
{
    if (m_dirty) {
        clearCaches();
    }
#ifndef DISABLE_CACHING
    if (m_geom_cache.contains(layout_rect)) {
        return m_geom_cache[layout_rect];
    }
#endif
#else
GridLayoutHfw::GeomInfo GridLayoutHfw::getGeomInfo() const
{
#ifndef DISABLE_CACHING
    if (!m_dirty) {
        return m_cached_geominfo;
    }
#endif
#endif


#ifdef DEBUG_LAYOUT_BASIC
    qDebug() << Q_FUNC_INFO;
#endif

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // Start of main thinking

    // Set up structures

    GeomInfo gi;
    gi.m_row_data = QVector<QLayoutStruct>(m_nrow);
    gi.m_col_data = QVector<QLayoutStruct>(m_ncol);
    gi.m_hfw_data = QVector<QLayoutStruct>(m_nrow);
    gi.m_has_hfw = false;

    // From QGridLayout::setupLayoutData:
    // ........................................................................

    for (int i = 0; i < m_nrow; ++i) {
        gi.m_row_data[i].init(m_r_stretches.at(i), m_r_min_heights.at(i));
        gi.m_row_data[i].maximum_size = m_r_stretches.at(i)
                ? QLAYOUTSIZE_MAX
                : m_r_min_heights.at(i);
    }
    for (int i = 0; i < m_ncol; ++i) {
        gi.m_col_data[i].init(m_c_stretches.at(i), m_c_min_widths.at(i));
        gi.m_col_data[i].maximum_size = m_c_stretches.at(i)
                ? QLAYOUTSIZE_MAX
                : m_c_min_widths.at(i);
    }

    const int n = m_things.size();
    QVarLengthArray<QQGridLayoutSizeTriple> sizes(n);
    bool has_multi = false;

    // Grid of items. We use it to determine which items are
    // adjacent to which and compute the spacings correctly.

    QVarLengthArray<QQGridBox*> grid(m_nrow * m_ncol);
    memset(grid.data(), 0, m_nrow * m_ncol * sizeof(QQGridBox*));

    // Initialize 'sizes' and 'grid' data structures, and insert
    // non-spanning items to our row and column data structures.

    for (int i = 0; i < n; ++i) {
        QQGridBox* const box = m_things.at(i);
        sizes[i].min_s = box->minimumSize();
        sizes[i].hint = box->sizeHint();
        sizes[i].max_s = box->maximumSize();

        if (box->hasHeightForWidth()) {
            gi.m_has_hfw = true;
        }

        if (box->row == box->toRow(m_nrow)) {  // spans 1 row
            addData(gi, box, sizes[i], true, false);
        } else {  // spans >1 row
            initEmptyMultiBox(gi.m_row_data, box->row, box->toRow(m_nrow));
            has_multi = true;
        }

        if (box->col == box->toCol(m_ncol)) {  // spans 1 col
            addData(gi, box, sizes[i], false, true);
        } else {  // spans >1 col
            initEmptyMultiBox(gi.m_col_data, box->col, box->toCol(m_ncol));
            has_multi = true;
        }

        // make each element of grid[] point to the item in it, if there is one
        for (int r = box->row; r <= box->toRow(m_nrow); ++r) {
            for (int c = box->col; c <= box->toCol(m_ncol); ++c) {
                gridAt(grid.data(), r, c, m_ncol) = box;
            }
        }
    }

    const int h_spacing = horizontalSpacing();
    const int v_spacing = verticalSpacing();
    setupSpacings(gi.m_col_data, grid.data(), h_spacing, Qt::Horizontal);
    setupSpacings(gi.m_row_data, grid.data(), v_spacing, Qt::Vertical);

    // Insert multicell items to our row and column data structures.
    // This must be done after the non-spanning items to obtain a
    // better distribution in distributeMultiBox().

    if (has_multi) {
        for (int i = 0; i < n; ++i) {
            QQGridBox* const box = m_things.at(i);

            if (box->row != box->toRow(m_nrow)) {
                distributeMultiBox(gi.m_row_data,
                                   box->row,
                                   box->toRow(m_nrow),
                                   sizes[i].min_s.height(),
                                   sizes[i].hint.height(),
                                   m_r_stretches,
                                   box->vStretch());
            }
            if (box->col != box->toCol(m_ncol)) {
                distributeMultiBox(gi.m_col_data,
                                   box->col,
                                   box->toCol(m_ncol),
                                   sizes[i].min_s.width(),
                                   sizes[i].hint.width(),
                                   m_c_stretches,
                                   box->hStretch());
            }
        }
    }

    for (int i = 0; i < m_nrow; i++) {
        gi.m_row_data[i].expansive = (gi.m_row_data.at(i).expansive ||
                                      gi.m_row_data.at(i).stretch > 0);
    }
    for (int i = 0; i < m_ncol; i++) {
        gi.m_col_data[i].expansive = (gi.m_col_data.at(i).expansive ||
                                      gi.m_col_data.at(i).stretch > 0);
    }

    // Main calculations - QGridLayout does these in distribute(), but moved
    // here
    // ........................................................................


#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    // Get actual contents rectangle
    QRect r = getContentsRect(layout_rect);
    // Work out column widths
    qGeomCalc(gi.m_col_data, 0, m_ncol, r.x(), r.width());
#endif

    // From QGridLayout::setupHfwLayoutData():
    // ........................................................................

    if (gi.m_has_hfw) {
        for (int i = 0; i < m_nrow; i++) {
            // Copy m_row_data to m_hfw_data:
            gi.m_hfw_data[i] = gi.m_row_data.at(i);
            // Modify starting minimum/hint heights:
            gi.m_hfw_data[i].minimum_size = gi.m_hfw_data[i].size_hint =
                    m_r_min_heights.at(i);
            // ... start with prespecified grid row minimum heights
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
            gi.m_hfw_data[i].maximum_size = qMax(gi.m_hfw_data[i].maximum_size,
                                                 gi.m_hfw_data[i].minimum_size);
#endif
        }
    
        for (int pass = 0; pass < 2; ++pass) {
            // Two passes used to calculate for items that cover >1 box.
            for (QQGridBox* box : m_things) {
                const int r1 = box->row;
                const int c1 = box->col;
                const int r2 = box->toRow(m_nrow);
                const int c2 = box->toCol(m_ncol);
                const int w = (
                        gi.m_col_data.at(c2).pos + gi.m_col_data.at(c2).size -
                        gi.m_col_data.at(c1).pos
                );
    
                if (r1 == r2) {
                    if (pass == 0) {
                        addHfwData(gi, box, w);
                    }
                } else {
                    if (pass == 0) {
                        initEmptyMultiBox(gi.m_hfw_data, r1, r2);
                    } else {
                        QSize hint = box->sizeHint();
                        QSize min = box->minimumSize();
                        if (box->hasHeightForWidth()) {
                            int hfwh = box->heightForWidth(w);
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
                            hint.setHeight(hfwh);
                            min.setHeight(hfwh);
#else
                            if (hfwh > hint.height())  {
                                hint.setHeight(hfwh);
                            }
                            if (hfwh > min.height()) {
                                min.setHeight(hfwh);
                            }
#endif
                        }
                        distributeMultiBox(gi.m_hfw_data, r1, r2,
                                           min.height(), hint.height(),
                                           m_r_stretches, box->vStretch());
                    }
                }
            }
        }
        for (int i = 0; i < m_nrow; i++) {
            gi.m_hfw_data[i].expansive = gi.m_hfw_data.at(i).expansive ||
                    gi.m_hfw_data.at(i).stretch > 0;
        }
    }

    // Summarizing results
    // ........................................................................

    // Expanding

    gi.m_expanding = 0;
    for (int r = 0; r < m_nrow; r++) {
        if (gi.m_row_data.at(r).expansive) {
            gi.m_expanding |= Qt::Vertical;
            break;
        }
    }
    for (int c = 0; c < m_ncol; c++) {
        if (gi.m_col_data.at(c).expansive) {
            gi.m_expanding |= Qt::Horizontal;
            break;
        }
    }

    // Size hints

    const Margins effmarg = effectiveMargins();  // RNC: stores in cache
    const QSize extra = effmarg.totalSize();
    // Note that findSize checks hasHeightForWidth() and acts accordingly:
    gi.m_min_size = findSize(gi, &QLayoutStruct::minimum_size);
    gi.m_max_size = findSize(gi, &QLayoutStruct::maximum_size);
    gi.m_size_hint = findSize(gi, &QLayoutStruct::size_hint)
            .expandedTo(gi.m_min_size)
            .boundedTo(gi.m_max_size);

    // From calcHfw (but then altered):
    if (gi.m_has_hfw) {
        gi.m_hfw_height = gi.m_size_hint.height();  // already incorporates extra
        gi.m_hfw_min_height = gi.m_min_size.height();  // already incorporates extra
    } else {
        gi.m_hfw_height = -1;
        gi.m_hfw_min_height = -1;
    }


    // More from distribute() on the actual calculation
    // ........................................................................

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    if (gi.m_has_hfw) {
        if (r.height() < gi.m_hfw_min_height) {
    #ifdef DEBUG_LAYOUT_BASIC
            qDebug() << "[getGeomInfo()] ... adjusting r height up from"
                     << r.height() << "to" << gi.m_hfw_min_height;
    #endif
            r.setHeight(gi.m_hfw_min_height);
        } else if (r.height() > gi.m_hfw_height) {
    #ifdef DEBUG_LAYOUT_BASIC
            qDebug() << "[getGeomInfo()] ... adjusting r height down from"
                     << r.height() << "to" << gi.m_hfw_height;
    #endif
            r.setHeight(gi.m_hfw_height);
        }
    }

    // Now work out row heights
    qGeomCalc(gi.m_has_hfw ? gi.m_hfw_data : gi.m_row_data,
              0, m_nrow, r.y(), r.height());
#endif

    gi.m_min_size += extra;
    gi.m_max_size += extra;
    gi.m_size_hint += extra;
    if (gi.m_has_hfw) {
        gi.m_hfw_height += extra.height();
        gi.m_hfw_min_height += extra.height();
    }

    // End of main thinking
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifdef DEBUG_LAYOUT_BASIC
    qDebug().nospace()
            << "[getGeomInfo()] ..."
#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
             << " for rect " << layout_rect
             << " (contents rect " << r << ")"
#endif
             << " n " << n
             << " m_expanding " << gi.m_expanding
             << " m_min_size " << gi.m_min_size
             << " m_max_size " << gi.m_max_size
             << " m_size_hint " << gi.m_size_hint
             << " m_has_hfw " << gi.m_has_hfw
             << " (margins " << effmarg
             << ")";
#ifdef DEBUG_LAYOUT_DETAILED
    for (int i = 0; i < m_things.size(); ++i) {
        QQGridBox* box = m_things.at(i);
        const int r1 = box->row;
        const int c1 = box->col;
        const int r2 = box->toRow(m_nrow);
        const int c2 = box->toCol(m_ncol);
        const int w = (gi.m_col_data.at(c2).pos + gi.m_col_data.at(c2).size -
                       gi.m_col_data.at(c1).pos);
        const QString rowdesc = (r1 == r2)
                ? QString("row=%1").arg(r1)
                : QString("rows=%1-%2").arg(r1).arg(r2);
        const QString coldesc = (c1 == c2)
                ? QString("col=%1").arg(c1)
                : QString("cols=%1-%2").arg(c1).arg(c2);
        qDebug().nospace().noquote()
                << "[getGeomInfo()] ... item " << i
                << ": " << rowdesc
                << ", " << coldesc
                << ", minimumSize " << box->minimumSize()
                << ", sizeHint " << box->sizeHint()
                << ", maximumSize " << box->maximumSize()
                << ", hasHeightForWidth " << box->hasHeightForWidth()
                << ", width " << w
                << ", heightForWidth(" << w << ") " << box->heightForWidth(w);
    }
    for (int i = 0; i < gi.m_col_data.size(); ++i) {
        const QLayoutStruct& ls = gi.m_col_data.at(i);
        qDebug().nospace() << "[getGeomInfo()] ... column "
                           << i << ": " << ls;
    }
    if (gi.m_has_hfw) {
        for (int i = 0; i < gi.m_hfw_data.size(); ++i) {
            const QLayoutStruct& ls = gi.m_hfw_data.at(i);
            qDebug().nospace() << "[getGeomInfo()] ... HFW row "
                               << i << ": " << ls;
        }
    } else {
        for (int i = 0; i < gi.m_row_data.size(); ++i) {
            const QLayoutStruct& ls = gi.m_row_data.at(i);
            qDebug().nospace() << "[getGeomInfo()] ... row "
                               << i << ": " << ls;
        }
    }
#endif
#endif

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    m_geom_cache[layout_rect] = gi;
#else
    m_cached_geominfo = gi;
    m_dirty = false;
#endif
    return gi;
}



GridLayoutHfw::GeomInfo GridLayoutHfw::getGeomInfoForHfw(int w) const
{
#ifndef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
#ifndef DISABLE_CACHING
    if (w == m_cached_hfw_width) {
        return m_cached_geominfo;
    }
#endif
#endif

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    // Find a precalculated GeomInfo with an appropriate width, or
    // calculate one using an arbitrary QRect of the same width.
    QHash<QRect, GeomInfo>::iterator it;
    GeomInfo gi;
    bool found = false;
    for (it = m_geom_cache.begin(); it != m_geom_cache.end(); ++it) {
        if (it.key().width() == w) {
            gi = it.value();
            found = true;
            break;
        }
    }
    if (!found) {
        gi = getGeomInfo(defaultRectOfWidth(w));
    }
#else
    GeomInfo gi = getGeomInfo();
#endif

#ifndef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    m_cached_hfw_width = w;
#endif
    return gi;
}


QRect GridLayoutHfw::getContentsRect(const QRect& layout_rect) const
{
    const QRect& r = layout_rect;  // so variable names match QBoxLayout
    const QRect cr = alignment() ? alignmentRect(r) : r;
    return effectiveMargins().removeMarginsFrom(cr);
}


// ========================================================================
// For friends
// ========================================================================

QDebug operator<<(QDebug debug, const GridLayoutHfw::GeomInfo& gi)
{
    debug.nospace()
            << "GeomInfo: m_row_data=" << gi.m_row_data
            << ", m_col_data=" << gi.m_col_data
            << ", m_hfw_data=" << gi.m_hfw_data
            << ", m_size_hint=" << gi.m_size_hint
            << ", m_min_size=" << gi.m_min_size
            << ", m_max_size=" << gi.m_max_size
            << ", m_expanding=" << gi.m_expanding
            << ", m_has_hfw=" << gi.m_has_hfw
            << ", m_hfw_height=" << gi.m_hfw_height
            << ", m_hfw_min_height=" << gi.m_hfw_min_height;
    return debug;
}
