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

// From qboxlayout.h:
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

#pragma once

#define BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT  // comment out to revert to QBoxLayout behaviour

#include <QLayout>
#include <QHash>
#include <QVector>
#include "layouts/qtlayouthelpers.h"
#include "lib/margins.h"

struct BoxLayoutHfwItem;


class BoxLayoutHfw : public QLayout
{
    // Modification of QBoxLayout (and its simple children QVBoxLayout and
    // QHBoxLayout) to support height-for-width properly.
    //
    // Specifically, these layouts will attempt to RESIZE THE WIDGET THAT OWNS
    // THEM to match the height-for-width of their contents.
    //
    // The difficulty is that layout attributes like minimumSize() are used
    // by owning widgets to set layout size, and they do not adequately convey
    // simultaneously "I'm happy to be only 20 pixels high if I can be 100
    // wide" and "if I'm 20 pixels wide, I must be at least 100 pixels high",
    // i.e. a dynamic minimum height.
    //
    // That is, the normal sequence is:
    // (1) a widget (or its owning layout in turn) asks its layout for its
    //     minimumSize(), sizeHint(), and maximumSize();
    // (2) the widget uses this information to set its size;
    // (3) the widget then asks its layout to lay out its children using
    //     setGeometry();
    // ... and the problem is that the exact rectangle width is known to the
    // layout only at step (3), but if the widget's height should be exactly
    // the height-for-width of the layout, it needed to know at step 1/2.
    //
    // This class attempts to solve this by triggering a re-layout (by forcing
    // the parent widget's height) if the geometry at step (3) is incompatible
    // with the one used by the widget previously at steps 1/2 (i.e. if the
    // parent's height is outside the min/max range).
    //
    // Triggering a re-layout before painting is better than the alternative of
    // using QWidget::resizeEvent() to call QWidget::updateGeometry(), because
    // (a) widgets owning that widget have to repeat the process (so you have
    //     to modify a whole chain of widgets rather than a single layout
    //     class), and
    // (b) that method is visually worse because (at least some) widgets are
    //     painted then repainted; with the layout method, all the thinking
    //     happens before any painting.
    //
    // WITHOUT THIS, whether or not the parent widget has height-for-width as
    // part of its size policy, the parent widget does not resize. The main
    // effect is that the layoutcan be cropped at the bottom (i.e. overspill at
    // the bottom is not shown). You might think that this would be OK if it
    // could scroll instead, but a scroll area needs to contain a widget, which
    // must get its height right if it contains an HFW layout -- so the problem
    // remains.
    //
    // UPSHOT:
    // - I have not been able to get this reliable and avoiding infinite loops.
    //   There is therefore a depth limit.
    // - The trouble is in part that so many things trigger invalidate(), and
    //   you don't know if they're important (e.g. a subwidget has changed
    //   size) or unimportant (e.g. self-triggered).
    //
    // Other notable modifications:
    // - the "private" (PIMPL) method is removed
    // - caching algorithms rewritten, with data storage structs

    Q_OBJECT
    using QLayoutStruct = qtlayouthelpers::QQLayoutStruct;  // RNC
public:
    enum Direction { LeftToRight, RightToLeft, TopToBottom, BottomToTop,
                     Down = TopToBottom, Up = BottomToTop };
    struct GeomInfo {  // RNC
        // Describes the geometry of the whole layout.
        // Created by getGeomInfo().

        // QLayoutStruct (and QQLayoutStruct) are small objects containing
        // measurements, used for layout calculations.
        QVector<QLayoutStruct> m_geom_array;

        // Then some things for the layout as a whole:
        QSize m_size_hint;  // layout preferred size
        QSize m_min_size;  // layout minimum size
        QSize m_max_size;  // layout maximum size
        int m_left_margin, m_top_margin, m_right_margin, m_bottom_margin;
        // ... layout margins (content rect is smaller than layout rect by
        // this amount)

        Qt::Orientations m_expanding;  // can it expand horizontally? vertically?
        bool m_has_hfw;  // layout has height-for-width property
    };

    struct HfwInfo {  // RNC
        // Returned by getHfwInfo(width); provides height-for-width details.
        HfwInfo() : hfw_height(-1), hfw_min_height(-1) {}
        int hfw_height;  // preferred height for the whole layout
        int hfw_min_height;  // minimum height for the whole layout
    };

public:
    explicit BoxLayoutHfw(Direction, QWidget* parent = nullptr);

    ~BoxLayoutHfw() override;

    Direction direction() const;
    void setDirection(Direction);

    void addSpacing(int size);
    void addStretch(int stretch = 0);
    void addSpacerItem(QSpacerItem* spacerItem);
    void addWidget(QWidget* widget, int stretch = 0,
                   Qt::Alignment alignment = Qt::Alignment());
    void addLayout(QLayout* layout, int stretch = 0);
    void addStrut(int size);
    void addItem(QLayoutItem* item) override;

    void insertSpacing(int index, int size);
    void insertStretch(int index, int stretch = 0);
    void insertSpacerItem(int index, QSpacerItem* spacer_item);
    void insertWidget(int index, QWidget* widget, int stretch = 0,
                      Qt::Alignment alignment = Qt::Alignment());
    void insertLayout(int index, QLayout* layout, int stretch = 0);
    void insertItem(int index, QLayoutItem* item);

    int spacing() const;
    void setSpacing(int spacing);

    bool setStretchFactor(QWidget* w, int stretch);
    bool setStretchFactor(QLayout* l, int stretch);
    void setStretch(int index, int stretch);
    int stretch(int index) const;

    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;

    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int minimumHeightForWidth(int width) const override;

    Qt::Orientations expandingDirections() const override;
    void invalidate() override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    int count() const override;

    // Main function to lay out the widgets.
    void setGeometry(const QRect& rect) override;

private:
    // Disable copy-constructor and copy-assignment-operator:
    BoxLayoutHfw(BoxLayoutHfw const&) = delete;
    void operator=(BoxLayoutHfw const& x) = delete;

protected:
    // Mark caches for clearing.
    void setDirty();

    // Remove all widgets.
    void deleteAll();

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    // What should our parent widget's height be, for a given GeomInfo?
    // Returns -1 if no change required.
    // Assumes that the parent comprises this layout plus parent_margins.
    int getParentTargetHeight(QWidget* parent, const Margins& parent_margins,
                              const GeomInfo& gi) const;  // RNC

    // Gets geometry information for a given layout rectangle.
    // The main calculation function.
    GeomInfo getGeomInfo(const QRect& layout_rect = QRect()) const;  // RNC
#else
    GeomInfo getGeomInfo() const;  // RNC
#endif

    // Returns height-for-width details (preferred and minimum height) for a
    // given layout width.
    HfwInfo getHfwInfo(int layout_width) const;  // RNC

    // Returns the margins of this grid (the unusable bit).
    Margins effectiveMargins(const Margins& contents_margins) const;  // RNC

    // Replace the widget at a particular index.
    QLayoutItem* replaceAt(int index, QLayoutItem* item);

    // Gets the active contents rect from the overall layout rect (by
    // subtracting margins).
    QRect getContentsRect(const QRect& layout_rect) const;  // RNC

    // Returns the rectangles for each cell in the layout.
    // Called by distribute().
    // Uses the contents_rect for the "whole layout" info, and then
    // items[index].pos and items[index].size for the "per item" info in the
    // layout's direction of travel.
    QVector<QRect> getChildRects(const QRect& contents_rect,
                                 const QVector<QLayoutStruct>& items) const;  // RNC

    // Gets the direction (left to right, or right to left), taking into
    // account any direction reversal being applied by our parent.
    Direction getVisualDir() const;  // RNC

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    // Clear all caches.
    void clearCaches() const;  // RNC
#endif

    // Returns the margins of this grid (the unusable bit).
    Margins effectiveMargins() const;  // RNC

    // Lay out children by setting their geometry.
    void distribute(const GeomInfo& gi,
                    const QRect& layout_rect, const QRect& old_rect);  // RNC

protected:
    QVector<BoxLayoutHfwItem*> m_list;  // our widgets
    Direction m_dir;  // visual direction
    int m_spacing;  // spacing between each widget

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    mutable int m_width_last_size_constraints_based_on;  // the width we last based our size information on
    mutable QRect m_rect_for_next_size_constraints;  // the layout_rect we will base our size information on
    mutable QHash<QRect, GeomInfo> m_geom_cache;  // RNC; maps layout_rect to GeomInfo
    mutable QHash<int, HfwInfo> m_hfw_cache;  // RNC; maps candidate width to HFW info
#else
    mutable GeomInfo m_cached_geominfo;
    mutable int m_cached_hfw_width;
    mutable HfwInfo m_cached_hfwinfo;
#endif
    mutable Margins m_contents_margins;  // RNC
    mutable Margins m_effective_margins;  // RNC
    mutable bool m_dirty;  // set by invalidate(), cleared by setupGeom(), used by lots to prevent unnecessary calls to setupGeom()
    int m_reentry_depth;  // RNC
};
