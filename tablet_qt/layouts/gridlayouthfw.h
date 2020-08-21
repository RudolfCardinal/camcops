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

#define GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT  // comment out to revert to QGridLayout behaviour

#include <QLayout>
#include <QHash>
#include <QVector>
#include "lib/margins.h"
#include "layouts/qtlayouthelpers.h"
class QQGridBox;
struct QQGridLayoutSizeTriple;


class GridLayoutHfw : public QLayout
{
    // This is to QGridLayout as BoxLayoutHfw (q.v.) is to QBoxLayout.
    // Main changes are:
    // - the layout handling, conditional on #define
    //   GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    // - change from PIMPL to conventional class idiom
    // - changes from:
    //      MyClass::constFunction() const
    //      {
    //          MyClass* that = const_cast<MyClass*>(this);
    //          that->nonConstFunction();  // modifies m_member
    //      }
    //   to using "mutable"
    // - Margins objects

    Q_OBJECT
    using QLayoutStruct = qtlayouthelpers::QQLayoutStruct;

    struct GeomInfo {  // RNC
        // Describes the geometry of the whole grid.
        // One is created for every grid rectangle we want to try.

        // QLayoutStruct (and QQLayoutStruct) are small objects containing
        // measurements, used for layout calculations.
        QVector<QLayoutStruct> m_row_data;
        QVector<QLayoutStruct> m_col_data;
        QVector<QLayoutStruct> m_hfw_data;  // was a pointer

        QSize m_size_hint;  // grid preferred size
        QSize m_min_size;  // grid minimum size
        QSize m_max_size;  // grid maximum size
        Qt::Orientations m_expanding;  // can it expand horizontally? vertically?
        bool m_has_hfw;  // grid has height-for-width property
        int m_hfw_height;  // preferred height of the grid based on HFW calcs
        int m_hfw_min_height;  // minimum height of the grid based on HFW calcs
    };

public:
    explicit GridLayoutHfw(QWidget* parent = nullptr);

    ~GridLayoutHfw() override;

    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;

    void setHorizontalSpacing(int spacing);
    int horizontalSpacing() const;
    void setVerticalSpacing(int spacing);
    int verticalSpacing() const;
    void setSpacing(int spacing);
    int spacing() const;

    void setRowStretch(int row, int stretch);

    // Spare horizontal space (i.e. space available in excess of the minimum
    // width) is allocated to columns in proportion to their stretch factors.
    void setColumnStretch(int column, int stretch);

    int rowStretch(int row) const;
    int columnStretch(int column) const;

    void setRowMinimumHeight(int row, int min_size);
    void setColumnMinimumWidth(int column, int min_size);
    int rowMinimumHeight(int row) const;
    int columnMinimumWidth(int column) const;

    int columnCount() const;
    int rowCount() const;

    QRect cellRect(const GeomInfo& gi, int row, int column) const;

    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int minimumHeightForWidth(int) const override;

    Qt::Orientations expandingDirections() const override;
    void invalidate() override;

    void addWidget(QWidget* w);
    void addWidget(QWidget* w, int row, int column,
                   Qt::Alignment = Qt::Alignment());
    void addWidget(QWidget* w, int row, int column,
                   int row_span, int column_span,
                   Qt::Alignment = Qt::Alignment());
    void addLayout(QLayout* w, int row, int column,
                   Qt::Alignment = Qt::Alignment());
    void addLayout(QLayout* w, int row, int column,
                   int row_span, int column_span,
                   Qt::Alignment = Qt::Alignment());

    void setOriginCorner(Qt::Corner);
    Qt::Corner originCorner() const;

    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* itemAtPosition(int row, int column) const;
    QLayoutItem* takeAt(int index) override;
    int count() const override;

    // Main function to lay out the grid of widgets.
    void setGeometry(const QRect&) override;

    void addItem(QLayoutItem* item, int row, int column,
                 int row_span = 1, int column_span = 1,
                 Qt::Alignment = Qt::Alignment());

    void setDefaultPositioning(int n, Qt::Orientation orient);
    void getItemPosition(int idx, int* row, int* column,
                         int* row_span, int* column_span) const;

protected:
    void addItem(QLayoutItem* item) override;

private:
    // Disable copy-constructor and copy-assignment-operator:
    GridLayoutHfw(GridLayoutHfw const&) = delete;
    void operator=(GridLayoutHfw const& x) = delete;

    // ------------------------------------------------------------------------
    // From QGridLayoutPrivate:
    // ------------------------------------------------------------------------
private:
    void add(QQGridBox*, int row, int col);
    void add(QQGridBox*, int row1, int row2, int col1, int col2);
    void distribute(const QRect& layout_rect);
    inline int numRows() const { return m_nrow; }
    inline int numCols() const { return m_ncol; }
    inline int rowSpacing(int r) const { return m_r_min_heights.at(r); }
    inline int colSpacing(int c) const { return m_c_min_widths.at(c); }
    inline void setReversed(bool r, bool c) { m_h_reversed = c; m_v_reversed = r; }
    inline bool horReversed() const { return m_h_reversed; }
    inline bool verReversed() const { return m_v_reversed; }
    void setDirty();  // RNC: was inline and defined here
    inline bool isDirty() const { return m_dirty; }
    inline void getNextPos(int& row, int& col) { row = m_next_r; col = m_next_c; }

    QLayoutItem* replaceAt(int index, QLayoutItem* newitem);  // RNC: was override (of QLayoutPrivate)
    void deleteAll();

    void expand(int rows, int cols);

private:
    // Sets the "widget auto-insert" point to be the box following the one
    // specified.
    void setNextPosAfter(int r, int c);

    // Function removed:
    // void recalcHFW(int w) const;

    // Alters "gi.m_hfw_data" to update details for the single row containing
    // "box" based on information from "box", where "width" is the candidate
    // width for that box's widget.
    void addHfwData(GeomInfo& gi, QQGridBox* box, int width) const;

    // Function removed:
    // void init();

    // Update "gi" information for the row (if r is true) and column (if c is
    // true) that contains "box".
    void addData(GeomInfo& gi, QQGridBox* box,
                 const QQGridLayoutSizeTriple& sizes, bool r, bool c) const;

    // Sets the overall grid size.
    void setSize(int rows, int cols);

    // Sets chain[<rownum>].spacing across the grid.
    // Used either with orientation == Qt::Horizontal for columns,
    // or with orientation == Qt::Vertical for rows.
    void setupSpacings(QVector<QLayoutStruct> &chain, QQGridBox* grid[],
                       int fixedSpacing, Qt::Orientation orientation) const;

    // Function removed:
    // void setupHfwLayoutData() const;

    // Translates margins under MacOS (does nothing otherwise).
    // void effectiveMargins(int* left, int* top, int* right, int* bottom) const;
    Margins effectiveMargins(const Margins& contents_margins) const;  // RNC

    // Returns the margins of this grid (the unusable bit).
    Margins effectiveMargins() const;  // RNC

    // Gets the active contents rect from the overall layout rect (by
    // subtracting margins).
    QRect getContentsRect(const QRect& layout_rect) const; // RNC

    // Returns the overall size of a hypothetical grid (from a GeomInfo),
    // where the size parameter says "which sort of size?" (e.g. min, max).
    QSize findSize(const GeomInfo& gi, int QLayoutStruct::* size) const;

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT

    // Clear all caches.
    void clearCaches() const;  // RNC

    // What should our parent widget's height be, for a given GeomInfo?
    // Returns -1 if no change required.
    // Assumes that the parent comprises this layout plus parent_margins.
    int getParentTargetHeight(QWidget* parent, const Margins& parent_margins,
                              const GeomInfo& gi) const;

    // Gets geometry information for a given layout rectangle.
    // The main calculation function.
    GeomInfo getGeomInfo(const QRect& layout_rect) const;  // RNC
#else
    GeomInfo getGeomInfo() const;  // RNC
#endif

    // Create a GeomInfo for a hypothetical layout of width w, used for
    // whole-grid HFW calculations.
    GeomInfo getGeomInfoForHfw(int w) const;  // RNC

    // These describe what items we have:

    int m_nrow;  // was rr; number of rows
    int m_ncol;  // was cc; number of columns
    mutable QVector<int> m_r_stretches;  // stretch information for each row
    mutable QVector<int> m_c_stretches;  // stretch information for each column
    QVector<int> m_r_min_heights;  // minimum heights for each row
    QVector<int> m_c_min_widths;  // minimum widths for each column
    QVector<QQGridBox*> m_things;  // list of owned objects

    // These govern where new inserted items are put:

    bool m_add_vertical;  // autoinsert in columns, not rows? RNC: was uint : 1
    int m_next_r;  // row for next "autoinserted" widget
    int m_next_c;  // column for next "autoinserted" widget

    // Global settings

    int m_horizontal_spacing;  // spacing between columns
    int m_vertical_spacing;  // spacing between rows
    bool m_h_reversed;  // right-to-left display; RNC: was uint : 1  -- and not sure it ever changes
    bool m_v_reversed;  // bottom-to-top display; RNC: was uint : 1  -- and not sure it ever changes

    // This is layout/geometry/HFW data:

#ifdef GRIDLAYOUTHFW_ALTER_FROM_QGRIDLAYOUT
    mutable int m_width_last_size_constraints_based_on;  // the width we last based our size information on
    mutable QRect m_rect_for_next_size_constraints;  // the layout_rect we will base our size information on
    mutable QHash<QRect, GeomInfo> m_geom_cache;  // RNC; maps layout_rect to GeomInfo
#else
    mutable GeomInfo m_cached_geominfo;
    mutable int m_cached_hfw_width;
#endif

    mutable Margins m_effective_margins;  // RNC; replacing leftMargin, topMargin, rightMargin, bottomMargin

    mutable bool m_dirty;  // need to clear caches? RNC: was uint : 1  -- was needRecalc
    int m_reentry_depth;  // RNC; reentry counter; nasty bit for resizing parent widget

public:
    friend QDebug operator<<(QDebug debug, const GeomInfo& gi);
};
