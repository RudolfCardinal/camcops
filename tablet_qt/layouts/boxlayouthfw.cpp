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

// From qboxlayout.cpp:
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

// #define DEBUG_LAYOUT_COMMS
// #define DEBUG_LAYOUT_CALCS
// #define DISABLE_CACHING  // NOT FREE OF SIDE EFFECTS!
// #define Q_OS_MAC  // for testing only, just to be sure it compiles OK...

#define USE_WIDGETITEMHFW

#include "boxlayouthfw.h"
#include <QApplication>
#include <QDebug>
#include <QList>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QStyle>
#include <QVector>
#include <QWidget>
#include "common/widgetconst.h"
#include "layouts/qtlayouthelpers.h"
#include "lib/reentrydepthguard.h"
#include "lib/sizehelpers.h"

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
#include "common/globals.h"  // for qHash(const QRect&)
#endif

using qtlayouthelpers::checkLayout;
using qtlayouthelpers::checkWidget;
using qtlayouthelpers::createSpacerItem;
using qtlayouthelpers::createWidgetItem;
using qtlayouthelpers::defaultRectOfWidth;
using qtlayouthelpers::qGeomCalc;
using qtlayouthelpers::qMaxExpCalc;
using qtlayouthelpers::qSmartSpacing;


// ============================================================================
// Constants
// ============================================================================

#if defined BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT && defined USE_WIDGETITEMHFW
    const bool USE_HFW_CAPABLE_ITEM = true;
#else
    const bool USE_HFW_CAPABLE_ITEM = false;
#endif


// ============================================================================
// Ancillary structs/classes
// ============================================================================

struct BoxLayoutHfwItem
{
    BoxLayoutHfwItem(QLayoutItem* it, int stretch_ = 0) :
        item(it),
        stretch(stretch_),
        magic(false)
    {}
    ~BoxLayoutHfwItem() { delete item; }

    int hfw(int w) const
    {
        if (item->hasHeightForWidth()) {
            w = boundWidth(w);  // RNC
            return item->heightForWidth(w);
        }
        return item->sizeHint().height();
    }

    int minhfw(int w) const  // was mhfw
    {
        if (item->hasHeightForWidth()) {
            w = boundWidth(w);  // RNC
            return item->heightForWidth(w);
        }
        return item->minimumSize().height();
    }

    int maxhfw(int w) const  // RNC
    {
        if (item->hasHeightForWidth()) {
            w = boundWidth(w);  // RNC
            return item->heightForWidth(w);
        }
        return item->maximumSize().height();
    }

    int hStretch() const
    {
        if (stretch == 0 && item->widget()) {
            return item->widget()->sizePolicy().horizontalStretch();
        }
        return stretch;
    }

    int vStretch() const
    {
        if (stretch == 0 && item->widget()) {
            return item->widget()->sizePolicy().verticalStretch();
        }
        return stretch;
    }

    int boundWidth(int w) const  // RNC
    {
        w = qBound(item->minimumSize().width(),
                   w,
                   item->maximumSize().width());
        if (QWidget* widget = item->widget()) {
            const QSizePolicy::Policy policy = widget->sizePolicy().horizontalPolicy();
            if ((policy & QSizePolicy::PolicyFlag::ShrinkFlag) == 0) {
                // Can't shrink, so don't let w go below sizeHint() width.
                w = qMax(item->sizeHint().width(), w);
            }
            if ((policy & (QSizePolicy::PolicyFlag::GrowFlag |
                           QSizePolicy::PolicyFlag::ExpandFlag)) == 0) {
                // Can't grow/expand, so don't let w go above sizeHint() width.
                w = qMin(w, item->sizeHint().width());
            }
        }
        return w;
    }

    QLayoutItem* item;
    int stretch;
    bool magic;
};


// ============================================================================
// Helper functions
// ============================================================================

static inline bool horz(const BoxLayoutHfw::Direction dir)
{
    return dir == BoxLayoutHfw::RightToLeft || dir == BoxLayoutHfw::LeftToRight;
}


// ============================================================================
// BoxLayoutHfw
// ============================================================================

BoxLayoutHfw::BoxLayoutHfw(const Direction dir, QWidget* parent) :
    QLayout(parent),
    m_dir(dir),
    m_spacing(-1),
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    m_width_last_size_constraints_based_on(-1),
    m_rect_for_next_size_constraints(qtlayouthelpers::QT_DEFAULT_RECT),
    // ... the framework always seems to ask about QRect(0,0 640x480), from
    // QWidgetPrivate::init()), so we may as well
    // anticipate it; this will mean that minimumSize() etc. trigger a geometry
    // calculation for 640x480 at first use
#else
    m_cached_hfw_width(-1),
#endif
    m_dirty(true),
    m_reentry_depth(0)
{
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    // setSizeConstraint(QLayout::SetMinAndMaxSize);

    // Without this, you are using QLayout::SetDefaultConstraint, which
    // constrains the widget's minimum size to our minimumSize() unless the
    // widget has its own minimumSize(). With those constraints, the following
    // can happen:
    // - QWidget checks out its default rectangle of 640x480.
    // - We say "for 640 wide, we need to be 112 high".
    // - The widget says "OK, have 800x112", and sizes itself thus.
    // - We say "ah, but for 800 wide, we need to be 84 high".
    // - The widget thinks "well, you're asking for 84 and you have 112, so
    //   you're sorted" and doesn't change its height.
    // - The net effect, for a vertical layout, is usually excess vertical
    //   space between items.
    //
    // If you use QLayout::SetMinAndMaxSize, then the widget will proceed
    // to resize itself to 800x84 next, and all's well.

    // HOWEVER, abandoned, as the owning widget still wasn't obeying
    // constraints resulting from our call to QLayout::activate() from
    // setGeometry(). Instead, we use the other trick of calling the parent's
    // setFixedHeight() and updateGeometry(), removing the need for this
    // constraint.
#endif
}


/*
    Destroys this box layout.
    The layout's widgets aren't destroyed.
*/
BoxLayoutHfw::~BoxLayoutHfw()
{
    deleteAll();
}


// ----------------------------------------------------------------------------
// Add/modify/remove components
// ----------------------------------------------------------------------------

QLayoutItem* BoxLayoutHfw::replaceAt(const int index, QLayoutItem* item)
{
    if (!item) {
        return nullptr;
    }
    BoxLayoutHfwItem* b = m_list.value(index);
    if (!b) {
        return nullptr;
    }
    QLayoutItem* r = b->item;

    b->item = item;
    invalidate();
    return r;
}



void BoxLayoutHfw::deleteAll()
{
    while (!m_list.isEmpty()) {
        delete m_list.takeFirst();
    }
}


void BoxLayoutHfw::setSpacing(const int spacing)
{
    m_spacing = spacing;
    invalidate();
}


void BoxLayoutHfw::addItem(QLayoutItem* item)
{
    auto it = new BoxLayoutHfwItem(item);
    m_list.append(it);
    invalidate();
}


void BoxLayoutHfw::insertItem(int index, QLayoutItem* item)
{
    if (index < 0) {  // append
        index = m_list.count();
    }

    auto it = new BoxLayoutHfwItem(item);
    m_list.insert(index, it);
    invalidate();
}


void BoxLayoutHfw::insertSpacing(int index, const int size)
{
    if (index < 0) {  // append
        index = m_list.count();
    }

    QLayoutItem* b;
    if (horz(m_dir)) {
        b = createSpacerItem(this, size, 0,
                             QSizePolicy::Fixed, QSizePolicy::Minimum);
    } else {
        b = createSpacerItem(this, 0, size,
                             QSizePolicy::Minimum, QSizePolicy::Fixed);
    }

    try {
        auto it = new BoxLayoutHfwItem(b);
        it->magic = true;
        m_list.insert(index, it);

    } catch (...) {
        delete b;
        throw;
    }
    invalidate();
}


void BoxLayoutHfw::insertStretch(int index, const int stretch)
{
    if (index < 0) {  // append
        index = m_list.count();
    }

    QLayoutItem* b;
    if (horz(m_dir)) {
        b = createSpacerItem(this, 0, 0,
                             QSizePolicy::Expanding, QSizePolicy::Minimum);
    } else {
        b = createSpacerItem(this, 0, 0,
                             QSizePolicy::Minimum, QSizePolicy::Expanding);
    }

    auto it = new BoxLayoutHfwItem(b, stretch);
    it->magic = true;
    m_list.insert(index, it);
    invalidate();
}


void BoxLayoutHfw::insertSpacerItem(int index, QSpacerItem* spacer_item)
{
    if (index < 0) {  // append
        index = m_list.count();
    }

    auto it = new BoxLayoutHfwItem(spacer_item);
    it->magic = true;
    m_list.insert(index, it);
    invalidate();
}


void BoxLayoutHfw::insertLayout(int index, QLayout* layout,
                                const int stretch)
{
    if (!checkLayout(layout, this)) {
        return;
    }
    if (!adoptLayout(layout)) {
        return;
    }
    if (index < 0) {  // append
        index = m_list.count();
    }
    auto it = new BoxLayoutHfwItem(layout, stretch);
    m_list.insert(index, it);
    invalidate();
}


void BoxLayoutHfw::insertWidget(int index, QWidget* widget,
                                const int stretch,
                                const Qt::Alignment alignment)
{
    if (!checkWidget(widget, this)) {
        return;
    }
    addChildWidget(widget);
    if (index < 0) {  // append
        index = m_list.count();
    }
    QWidgetItem* b = createWidgetItem(this, widget, USE_HFW_CAPABLE_ITEM);
    b->setAlignment(alignment);

    BoxLayoutHfwItem* it;
    try {
        it = new BoxLayoutHfwItem(b, stretch);
    } catch (...) {
        delete b;
        throw;
    }

    try {
        m_list.insert(index, it);
    } catch (...) {
        delete it;
        throw;
    }
    invalidate();
}


void BoxLayoutHfw::addSpacing(const int size)
{
    insertSpacing(-1, size);
}


void BoxLayoutHfw::addStretch(const int stretch)
{
    insertStretch(-1, stretch);
}


void BoxLayoutHfw::addSpacerItem(QSpacerItem* spacer_item)
{
    insertSpacerItem(-1, spacer_item);
}


void BoxLayoutHfw::addWidget(QWidget* widget, const int stretch,
                             const Qt::Alignment alignment)
{
    insertWidget(-1, widget, stretch, alignment);
}


void BoxLayoutHfw::addLayout(QLayout* layout, const int stretch)
{
    insertLayout(-1, layout, stretch);
}


void BoxLayoutHfw::addStrut(const int size)
{
    QLayoutItem* b;
    if (horz(m_dir)) {
        b = createSpacerItem(this, 0, size,
                             QSizePolicy::Fixed, QSizePolicy::Minimum);
    } else {
        b = createSpacerItem(this, size, 0,
                             QSizePolicy::Minimum, QSizePolicy::Fixed);
    }

    auto it = new BoxLayoutHfwItem(b);
    it->magic = true;
    m_list.append(it);
    invalidate();
}


bool BoxLayoutHfw::setStretchFactor(QWidget* widget, const int stretch)
{
    if (!widget) {
        return false;
    }
    for (BoxLayoutHfwItem* box : m_list) {
        if (box->item->widget() == widget) {
            box->stretch = stretch;
            invalidate();
            return true;
        }
    }
    return false;
}


bool BoxLayoutHfw::setStretchFactor(QLayout* layout, const int stretch)
{
    for (BoxLayoutHfwItem* box : m_list) {
        if (box->item->layout() == layout) {
            if (box->stretch != stretch) {
                box->stretch = stretch;
                invalidate();
            }
            return true;
        }
    }
    return false;
}


void BoxLayoutHfw::setStretch(const int index, const int stretch)
{
    if (index >= 0 && index < m_list.size()) {
        BoxLayoutHfwItem* box = m_list.at(index);
        if (box->stretch != stretch) {
            box->stretch = stretch;
            invalidate();
        }
    }
}


QLayoutItem* BoxLayoutHfw::takeAt(const int index)
{
    if (index < 0 || index >= m_list.count()) {
        return nullptr;
    }
    BoxLayoutHfwItem* b = m_list.takeAt(index);
    QLayoutItem* item = b->item;
    b->item = nullptr;
    delete b;

    if (QLayout* l = item->layout()) {
        // sanity check in case the user passed something weird to QObject::setParent()
        if (l->parent() == this) {
            l->setParent(nullptr);
        }
    }

    invalidate();
    return item;
}


void BoxLayoutHfw::setDirection(const Direction direction)
{
    if (m_dir == direction) {
        return;
    }
    if (horz(m_dir) != horz(direction)) {
        //swap around the spacers (the "magic" bits)
        //#### a bit yucky, knows too much.
        //#### probably best to add access functions to spacerItem
        //#### or even a QSpacerItem::flip()
        for (BoxLayoutHfwItem* box : m_list) {
            if (box->magic) {
                QSpacerItem* sp = box->item->spacerItem();
                if (sp) {
                    if (sp->expandingDirections() == Qt::Orientations(0) /*No Direction*/) {
                        //spacing or strut
                        const QSize s = sp->sizeHint();
                        sp->changeSize(s.height(), s.width(),
                            horz(direction) ? QSizePolicy::Fixed:QSizePolicy::Minimum,
                            horz(direction) ? QSizePolicy::Minimum:QSizePolicy::Fixed);

                    } else {
                        //stretch
                        if (horz(direction)) {
                            sp->changeSize(0, 0, QSizePolicy::Expanding,
                                           QSizePolicy::Minimum);
                        } else {
                            sp->changeSize(0, 0, QSizePolicy::Minimum,
                                           QSizePolicy::Expanding);
                        }
                    }
                }
            }
        }
    }
    m_dir = direction;
    invalidate();
}


// ----------------------------------------------------------------------------
// Other public information
// ----------------------------------------------------------------------------

int BoxLayoutHfw::spacing() const
{
    if (m_spacing >= 0) {
        return m_spacing;
    }
    return qSmartSpacing(this,
                         m_dir == LeftToRight || m_dir == RightToLeft
                            ? QStyle::PM_LayoutHorizontalSpacing
                            : QStyle::PM_LayoutVerticalSpacing);
}


int BoxLayoutHfw::count() const
{
    return m_list.count();
}


QLayoutItem* BoxLayoutHfw::itemAt(const int index) const
{
    return index >= 0 && index < m_list.count()
            ? m_list.at(index)->item
            : nullptr;
}


BoxLayoutHfw::Direction BoxLayoutHfw::direction() const
{
    return m_dir;
}


int BoxLayoutHfw::stretch(const int index) const
{
    if (index >= 0 && index < m_list.size()) {
        return m_list.at(index)->stretch;
    }
    return -1;
}


// ----------------------------------------------------------------------------
// Internal information
// ----------------------------------------------------------------------------

BoxLayoutHfw::Direction BoxLayoutHfw::getVisualDir() const
{
    Direction visual_dir = m_dir;
    QWidget* parent = parentWidget();
    if (parent && parent->isRightToLeft()) {
        if (m_dir == LeftToRight) {
            visual_dir = RightToLeft;
        } else if (m_dir == RightToLeft) {
            visual_dir = LeftToRight;
        }
    }
    return visual_dir;
}


// ----------------------------------------------------------------------------
// Provide size information to owner
// ----------------------------------------------------------------------------

QSize BoxLayoutHfw::sizeHint() const
{
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
    m_width_last_size_constraints_based_on = m_rect_for_next_size_constraints.width();
#else
    GeomInfo gi = getGeomInfo();
#endif
#ifdef DEBUG_LAYOUT_COMMS
    qDebug().nospace() << Q_FUNC_INFO << " -> " << gi.m_size_hint
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
                       << " (based on notional width of "
                       << m_width_last_size_constraints_based_on << ")"
#endif
                          ;
#endif
    return gi.m_size_hint;
}


QSize BoxLayoutHfw::minimumSize() const
{
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
    m_width_last_size_constraints_based_on = m_rect_for_next_size_constraints.width();
#else
    GeomInfo gi = getGeomInfo();
#endif
#ifdef DEBUG_LAYOUT_COMMS
    qDebug().nospace() << Q_FUNC_INFO << " -> " << gi.m_min_size
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
                       << " (based on notional width of "
                       << m_width_last_size_constraints_based_on << ")"
#endif
                          ;
#endif
    return gi.m_min_size;
}


QSize BoxLayoutHfw::maximumSize() const
{
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
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
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
                       << " (based on notional width of "
                       << m_width_last_size_constraints_based_on << ")"
#endif
                          ;
#endif
    return s;
}


bool BoxLayoutHfw::hasHeightForWidth() const
{
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
    // ... seems dumb to use geometry to ask that question, but we have to have
    // calculated at least one geometry to know that we've checked our contents
    // since last invalidate(), so we may as well use the m_has_hfw from one of
    // the geometries

    // no need to set m_width_last_size_constraints_based_on here, though
#else
    GeomInfo gi = getGeomInfo();
#endif
    return gi.m_has_hfw;
}


int BoxLayoutHfw::heightForWidth(const int w) const
{
    if (!hasHeightForWidth()) {
        return -1;
    }
    const HfwInfo hfw_info = getHfwInfo(w);
#ifdef DEBUG_LAYOUT_COMMS
    qDebug() << Q_FUNC_INFO << ": width" << w
             << " -> height" << hfw_info.hfw_height;
#endif
    return hfw_info.hfw_height;
}


int BoxLayoutHfw::minimumHeightForWidth(const int w) const
{
    if (!hasHeightForWidth()) {
        return -1;
    }
    const HfwInfo hfw_info = getHfwInfo(w);
#ifdef DEBUG_LAYOUT_COMMS
    qDebug() << Q_FUNC_INFO << ": width" << w
             << " -> minimum height" << hfw_info.hfw_min_height;
#endif
    return hfw_info.hfw_min_height;
}


Qt::Orientations BoxLayoutHfw::expandingDirections() const
{
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    const GeomInfo gi = getGeomInfo(m_rect_for_next_size_constraints);
#else
    GeomInfo gi = getGeomInfo();
#endif
    // ... see hasHeightForWidth() for rationale
    return gi.m_expanding;
}


// ----------------------------------------------------------------------------
// The complex bit (1): layout - virtual functions
// ----------------------------------------------------------------------------

void BoxLayoutHfw::invalidate()
{
    // This will be called by the framework, via:
    //
    //      QLayout::activate()
    //      -> QLayout::activateRecursiveHelper()
    //      -> BoxLayoutHfw::invalidate()
    //
    //      MenuHeader::MenuHeader()
    //      -> QWidget::setLayout()
    //      -> BoxLayoutHfw::invalidate()
    //
    //      MenuHeader::MenuHeader()
    //      -> BoxLayoutHfw::addLayout()
    //      -> BoxLayoutHfw::insertLayout()
    //      -> BoxLayoutHfw::invalidate()
    //
    // and for widgets setting their style, etc.
    // There are many calls here before the layout even gets asked about its
    // geometry. So, should be FAST. Hence, use m_dirty.
    //                         ^^^^

    // ... so think twice before clearing m_cached_layout_width.
    // BUT ALSO will be called by the framework if our widgets (or their
    // children, etc.) call their updateGeometry() and have changed size.
    // So we do need to invalidate...
    // ... except not if we triggered it ourself.

    setDirty();
    QLayout::invalidate();
}


void BoxLayoutHfw::setGeometry(const QRect& initial_rect)
{
    // RNC: when this is called, it's too late to alter the layout's size;
    // the instruction is "this is your size; now lay out your children".

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
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    QRect r = initial_rect;  // we may modify it, below
#else
    const QRect& r = initial_rect;  // just an alias
#endif

    // RNC: r is the overall rectangle for the layout

    // ------------------------------------------------------------------------
    // Announce
    // ------------------------------------------------------------------------
#ifdef DEBUG_LAYOUT_CALCS
    qDebug() << Q_FUNC_INFO;
#endif

    // ------------------------------------------------------------------------
    // Skip because nothing's changed?
    // ------------------------------------------------------------------------
#ifndef DISABLE_CACHING
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    const bool geometry_previously_calculated = m_geom_cache.contains(r);
    if (geometry_previously_calculated && r == geometry()) {
#else
    if (!m_dirty && r == geometry()) {
#endif
        // Exactly the same geometry as last time, and we're all set up.
#ifdef DEBUG_LAYOUT_CALCS
        qDebug() << "... nothing to do, for" << r;
#endif
        return;
    }
#endif

    // ------------------------------------------------------------------------
    // Recalculate geometry
    // ------------------------------------------------------------------------
    // So, if we're here, we've previously calculated the geometry,
    // but the new geometry doesn't match our current geometry(); this means
    // that we need to change geometry but we've already had a pass through
    // in which we've had a chance to notify our parent widget.

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    const GeomInfo gi = getGeomInfo(r);
#else
    GeomInfo gi = getGeomInfo();
#endif

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    // This is the trickiest bit.
    // If you call QWidget::setFixedHeight(), do it *last* in this function,
    // because that can call back in. From bottom to top:
    //      BoxLayoutHfw::setGeometry
    //      QLayoutPrivate::doResize(QSize const&)
    //      QLayout::activate()
    //      QApplicationPrivate::notify_helper(QObject*, QEvent*)
    //      QApplication::notify(QObject*, QEvent*)
    //      QCoreApplication::notifyInternal2(QObject*, QEvent*)
    //      QWidgetPrivate::setGeometry_sys(int, int, int, int, bool)
    //      QWidget::resize(QSize const&)
    //      QWidget::setMaximumSize(int, int)
    //      QWidget::setFixedHeight(int)
    //      BoxLayoutHfw::setGeometry

    if (gi.m_has_hfw) {
        // Only if we have hfw can our size hints vary with width.
        if (r.width() != m_width_last_size_constraints_based_on) {
            // The width has changed since we last told our owning widget what
            // size we need to be.
            // This means that our minimum height (etc.) may be wrong. So we
            // need to invalidate the layout (at least partly).
#ifdef DEBUG_LAYOUT_CALCS
            qDebug().nospace()
                    << "... resetting width hints, for " << r
                    << " (because width=" << r.width()
                    << " but last size constraints were based on width of "
                    << m_width_last_size_constraints_based_on << ")";
#endif
            m_rect_for_next_size_constraints = r;
            // QLayout::activate();  // not invalidate(); not activate()
        }
    }
    QWidget* parent = parentWidget();
    const Margins parent_margins = Margins::getContentsMargins(parent);
    if (!parent) {
        qWarning() << Q_FUNC_INFO << "Layout has no parent widget";
    }
    const int parent_new_height = getParentTargetHeight(
                parent, parent_margins, gi);
    if (parent_new_height != -1) {
        // We will, under these circumstances, call
        // parent->updateGeometry().

        // Note, however, that calling parent->updateGeometry() doesn't
        // necessarily trigger a call back to us here. So we must lay
        // out our children (or they can fail to be drawn), and we
        // should therefore lay them out where they *will* be once the
        // parent has changed its size.

        // Moreover, if we call parent->updateGeometry(), it must be
        // the LAST thing we do, as above.

        // So:
        r.setHeight(parent_new_height - parent_margins.totalHeight());  // change

        // Don't think we need to call gi = getGeomInfo(r) again,
        // as the width hasn't changed.
    }
#endif

    // ------------------------------------------------------------------------
    // Lay out children and call QLayout::setGeometry()
    // ------------------------------------------------------------------------
    const QRect old_rect = geometry();
    QLayout::setGeometry(r);
    distribute(gi, r, old_rect);

    // ------------------------------------------------------------------------
    // Ask our parent to resize, if necessary
    // ------------------------------------------------------------------------
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
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


int BoxLayoutHfw::getParentTargetHeight(QWidget* parent,
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

    const int current_parent_height = parent->geometry().height();
    if (current_parent_height < target_min_height) {
#ifdef DEBUG_LAYOUT_CALCS
        qDebug().nospace()
                << "... will set parent height to " << target_min_height
                << " (was " << parent->geometry().height()
                << ", below our min of " << target_min_height
                << " [including parent margin height of "
                << parent_margins.totalHeight() << "])";
#endif
        parent_new_height = target_min_height;

    } else if (current_parent_height > target_max_height) {
#ifdef DEBUG_LAYOUT_CALCS
        qDebug().nospace()
                << "... will set parent height to " << target_max_height
                << " (was " << parent->geometry().height()
                << ", above our max of " << target_max_height
                << " [including parent margin height of "
                << parent_margins.totalHeight() << "])";
#endif
        parent_new_height = target_max_height;
    }

    return parent_new_height;
}


void BoxLayoutHfw::distribute(const GeomInfo& gi,
                              const QRect& layout_rect, const QRect& old_rect)
{
    const QRect& r = layout_rect;  // alias
    const QRect s = getContentsRect(layout_rect);

#ifdef DEBUG_LAYOUT_COMMS
    qDebug() << Q_FUNC_INFO;
    qDebug().nospace() << "... called with layout rect " << layout_rect
                       << ", giving final rect for children of " << s;
#endif

    QVector<QLayoutStruct> a = gi.m_geom_array;
    const int pos = horz(m_dir) ? s.x() : s.y();  // RNC: starting coordinate (left or top)
    const int space = horz(m_dir) ? s.width() : s.height();  // RNC: extent (width or height)
    const int n = a.count();

    // The idea here is that when we were asked "how big do you want to be",
    // we returned information from getGeomInfo() that encompassed the range
    // of sizes that our items would permit. However, now we're being asked
    // to lay the items out, and at that point, a height-for-width widget has
    // only one possible size, which is its heightForWidth(its width).
    // Anyway, this is the QVBoxLayout code, but it's equally true for our
    // modified layout... except that we also want to constrain the maximum
    // height.
    if (gi.m_has_hfw && !horz(m_dir)) {
        for (int i = 0; i < n; i++) {
            const BoxLayoutHfwItem* box = m_list.at(i);
            const QLayoutItem* item = box->item;
            if (item->hasHeightForWidth()) {
                const int width = qBound(item->minimumSize().width(),
                                         s.width(),
                                         item->maximumSize().width());
                a[i].size_hint = a[i].minimum_size =
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
                        a[i].maximum_size =
#endif
                        item->heightForWidth(width);
            }
        }
    }

    qGeomCalc(a, 0, n, pos, space);

    const Direction visual_dir = getVisualDir();
    const bool reverse = horz(visual_dir)
            ? ((r.right() > old_rect.right()) != (visual_dir == RightToLeft))
            : r.bottom() > old_rect.bottom();
    // ... RNC: this seems to be saying that for vertical layouts, at least,
    // then if the geometry is extending downwards (old_rect ending below
    // current), draw from the bottom up.
    QVector<QRect> childrects = getChildRects(s, a);
    for (int j = 0; j < n; j++) {
        const int i = reverse ? (n - j - 1) : j;
        BoxLayoutHfwItem* box = m_list.at(i);
        const QRect& childrect = childrects.at(i);
        box->item->setGeometry(childrect);
        // RNC: NOTE that the rectangle can be TRANSFORMED by the time it
        // reaches a widget's resizeEvent(). The sequence of calls is:
        //      - QLayoutItem::setGeometry()
        //        overridden by QWidgetItem::setGeometry
        //      - QWidget::setGeometry()
        //      - QWidgetPrivate::setGeometry_sys()
        //        ... can apply min/max constraints
        //        ... posts a QResizeEvent
#ifdef DEBUG_LAYOUT_COMMS
        qDebug() << "... item" << i
                 << "given setGeometry() instruction" << childrect;
#endif
    }

}


// ----------------------------------------------------------------------------
// The complex bit (2): layout - internal functions
// ----------------------------------------------------------------------------

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
BoxLayoutHfw::GeomInfo BoxLayoutHfw::getGeomInfo(const QRect& layout_rect) const
{
    if (m_dirty) {
        clearCaches();
    }
#ifndef DISABLE_CACHING
    if (m_geom_cache.contains(layout_rect)) {
        return m_geom_cache[layout_rect];
    }
#endif

    const QRect s = getContentsRect(layout_rect);
    const int layout_available_width = s.width();

#else
BoxLayoutHfw::GeomInfo BoxLayoutHfw::getGeomInfo() const
{
#ifndef DISABLE_CACHING
    if (!m_dirty) {
        return m_cached_geominfo;
    }
#endif
#endif

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // Start of main thinking

    GeomInfo gi;

    int maxw = horz(m_dir) ? 0 : QLAYOUTSIZE_MAX;  // layout maximum width
    int maxh = horz(m_dir) ? QLAYOUTSIZE_MAX : 0;  // layout maximum height
    int minw = 0;  // layout minimum width
    int minh = 0;  // layout minimum height
    int hintw = 0;  // layout preferred width
    int hinth = 0;  // layout preferred height

    bool horexp = false;  // horizontally expanding?
    bool verexp = false;  // vertically expanding?

    gi.m_has_hfw = false;

    const int n = m_list.count();
    gi.m_geom_array.clear();
    QVector<QLayoutStruct> a(n);  // RNC: extra Q

    QSizePolicy::ControlTypes control_types1;
    QSizePolicy::ControlTypes control_types2;
    const int fixed_spacing = spacing();
    int previous_non_empty_index = -1;

    QStyle* style = nullptr;
    if (fixed_spacing < 0) {
        if (QWidget* pw = parentWidget()) {
            style = pw->style();
        }
    }

    for (int i = 0; i < n; i++) {
        BoxLayoutHfwItem* box = m_list.at(i);
        const QSize item_min = box->item->minimumSize();
        const QSize item_hint = box->item->sizeHint();  // RNC: was just "hint"
        const QSize item_max = box->item->maximumSize();  // RNC: was just "max"
        const Qt::Orientations expdir = box->item->expandingDirections();  // RNC: was just "exp"
        const bool empty = box->item->isEmpty();
        const bool ignore = empty && box->item->widget(); // ignore hidden widgets
        int spacing = 0;
        bool dummy = true;

        if (!empty) {
            if (fixed_spacing >= 0) {
                spacing = (previous_non_empty_index >= 0) ? fixed_spacing : 0;
                // RNC: ... we don't apply spacing above the first widget, but
                // above all its successors (as a vertical example)
#ifdef Q_OS_MAC
                // RNC: alters spacing for all but the first widget, somehow,
                // for vertical layouts
                if (!horz(m_dir) && previous_non_empty_index >= 0) {
                    BoxLayoutHfwItem* sibling = (
                                m_dir == BoxLayoutHfw::TopToBottom
                                ? box
                                : m_list.at(previous_non_empty_index));
                    if (sibling) {
                        QWidget* wid = sibling->item->widget();
                        if (wid) {
                            spacing = qMax(spacing, sibling->item->geometry().top() - wid->geometry().top());
                        }
                    }
                }
#endif
            } else {
                control_types1 = control_types2;
                control_types2 = box->item->controlTypes();
                if (previous_non_empty_index >= 0) {
                    QSizePolicy::ControlTypes actual1 = control_types1;
                    QSizePolicy::ControlTypes actual2 = control_types2;
                    if (m_dir == BoxLayoutHfw::RightToLeft ||
                            m_dir == BoxLayoutHfw::BottomToTop) {
                        qSwap(actual1, actual2);
                    }

                    if (style) {
                        spacing = style->combinedLayoutSpacing(
                                    actual1, actual2,
                                    horz(m_dir) ? Qt::Horizontal : Qt::Vertical,
                                    nullptr, parentWidget());
                        if (spacing < 0) {
                            spacing = 0;
                        }
                    }
                }
            }

            if (previous_non_empty_index >= 0) {
                a[previous_non_empty_index].spacing = spacing;
            }
            previous_non_empty_index = i;
        }

        if (horz(m_dir)) {
            // ----------------------------------------------------------------
            // HORIZONTAL
            // ----------------------------------------------------------------
            const bool expand = (expdir & Qt::Horizontal || box->stretch > 0);
            horexp = horexp || expand;

            // Widths
            minw += spacing + item_min.width();
            hintw += spacing + item_hint.width();
            maxw += spacing + item_max.width();
            a[i].minimum_size = item_min.width();
            a[i].size_hint = item_hint.width();
            a[i].maximum_size = item_max.width();

            // Heights
            // ... standard height code from QBoxLayout
            // ... we will calculate the actual height below
            if (!ignore) {
                qMaxExpCalc(maxh, verexp, dummy,  // RNC: extra q; alters first three parameters (max, exp, empty)
                            item_max.height(), expdir & Qt::Vertical,
                            box->item->isEmpty());
            }
            minh = qMax(minh, item_min.height());
            hinth = qMax(hinth, item_hint.height());

            // Other
            a[i].expansive = expand;
            a[i].stretch = box->stretch ? box->stretch : box->hStretch();

        } else {
            // ----------------------------------------------------------------
            // VERTICAL
            // ----------------------------------------------------------------
            const bool expand = (expdir & Qt::Vertical || box->stretch > 0);
            verexp = verexp || expand;

            // Widths
            if (!ignore) {
                qMaxExpCalc(maxw, horexp, dummy,  // RNC: extra q; alters first three parameters (max, exp, empty)
                            item_max.width(), expdir & Qt::Horizontal,
                            box->item->isEmpty());
            }
            minw = qMax(minw, item_min.width());
            hintw = qMax(hintw, item_hint.width());

            // Heights
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
            // Here we modify by saying "for your likely width, what is the
            // minimum/maximum height"? For a height-for-width item, all the
            // heights will be the same (see BoxLayoutHfwItem). For other
            // items, we will get the same results as the QBoxLayout code.
            const int item_width = qBound(item_min.width(),
                                          layout_available_width,
                                          item_max.width());

            const int minhfw = box->minhfw(item_width);
            const int hfw = box->hfw(item_width);
            const int maxhfw = box->maxhfw(item_width);
            minh += spacing + minhfw;
            hinth += spacing + hfw;
            maxh += spacing + maxhfw;
            a[i].minimum_size = minhfw;
            a[i].size_hint = hfw;
            a[i].maximum_size = maxhfw;
            // QLayoutStruct::sizeHint is in the direction of
            // layout travel, so vertical here.
#else
            minh += spacing + item_min.height();
            hinth += spacing + item_hint.height();
            maxh += spacing + item_max.height();
            a[i].minimum_size = item_min.height();
            a[i].size_hint = item_hint.height();
            a[i].maximum_size = item_max.height();
#endif

            // Other
            a[i].expansive = expand;
            a[i].stretch = box->stretch ? box->stretch : box->vStretch();
        }

        a[i].empty = empty;
        a[i].spacing = 0;   // might be initialized with a non-zero value in a later iteration
        gi.m_has_hfw = gi.m_has_hfw || box->item->hasHeightForWidth();
    }

    gi.m_geom_array = a;

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    // RNC extra: for horizontal layouts, redo the layout max/min/hint height
    // calculations now we can work out the widths of all items
    if (gi.m_has_hfw && horz(m_dir)) {
        // Create dummy layout using "a"
        const int pos = s.x();
        const int space = s.width();
        qGeomCalc(a, 0, n, pos, space);
        minh = 0;
        maxh = QLAYOUTSIZE_MAX;
        hinth = 0;
        for (int i = 0; i < n; ++i) {
            BoxLayoutHfwItem* box = m_list.at(i);
            const Qt::Orientations expdir = box->item->expandingDirections();  // RNC: was just "exp"
            const bool empty = box->item->isEmpty();
            // for QWidgetItem: return (wid->isHidden() && !wid->sizePolicy().retainSizeWhenHidden()) || wid->isWindow();
            const bool ignore = empty && box->item->widget(); // ignore hidden widgets
            //  ... as opposed to hidden layouts?
            bool dummy = true;

            const int item_width = a[i].size;  // already solved
            const int hfw = box->hfw(item_width);
            const int minhfw = box->minhfw(item_width);
            const int maxhfw = box->maxhfw(item_width);
            // I'm not sure why QBoxLayout doesn't put the minh/hinth
            // calculations within the "if (!ignore)" test.
            minh = qMax(minh, minhfw);
            hinth = qMax(hinth, hfw);
            if (!ignore) {
                qMaxExpCalc(maxh, verexp, dummy,  // RNC: extra q; alters first three parameters (max, exp, empty)
                            maxhfw,
                            expdir & Qt::Vertical,
                            box->item->isEmpty());
            }
        }
    }
#endif

    gi.m_expanding = (Qt::Orientations) ((horexp ? Qt::Horizontal : 0)
                                         | (verexp ? Qt::Vertical : 0));

    gi.m_min_size = QSize(minw, minh);
    gi.m_max_size = QSize(maxw, maxh).expandedTo(gi.m_min_size);
    gi.m_size_hint = QSize(hintw, hinth).expandedTo(gi.m_min_size).boundedTo(gi.m_max_size);

    const Margins effmarg = effectiveMargins();  // caches content/effective margins
    const QSize extra = effmarg.totalSize();

    gi.m_min_size += extra;
    gi.m_max_size += extra;
    gi.m_size_hint += extra;

    // End of main thinking
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifdef DEBUG_LAYOUT_CALCS
    qDebug() << Q_FUNC_INFO;
    qDebug().nospace()
            << "..."
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
             << " for rect " << layout_rect
#endif
             << " n " << n
             << " m_expanding " << gi.m_expanding
             << " m_min_size " << gi.m_min_size
             << " m_max_size " << gi.m_max_size
             << " m_size_hint " << gi.m_size_hint
             << " m_has_hfw " << gi.m_has_hfw
             << " (margins " << effmarg
             << "; m_dir " << m_dir << ")";
    for (int i = 0; i < n; ++i) {
        const QLayoutStruct& ls = a.at(i);
        qDebug().nospace() << "... item " << i << ": " << ls;
    }
#endif

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    m_geom_cache[layout_rect] = gi;
#else
    m_cached_geominfo = gi;
    m_dirty = false;
#endif
    return gi;
}


BoxLayoutHfw::HfwInfo BoxLayoutHfw::getHfwInfo(const int layout_width) const
{
    int w = layout_width;  // name used in original
    // ... but original did the HFW calculations on the INNER width,
    // and we do it on the OUTER width here, so that we can be consistent
    // with getGeomInfo(), which uses the OUTER rect
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    if (m_dirty) {
        clearCaches();
    }
#ifndef DISABLE_CACHING
    if (m_hfw_cache.contains(w)) {
        return m_hfw_cache[w];
    }
#endif
#else
#ifndef DISABLE_CACHING
    if (w == m_cached_hfw_width) {
        return m_cached_hfwinfo;
    }
#endif
#endif

#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
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

    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    // Start of main thinking

    const Margins effmarg = effectiveMargins();
    w -= effmarg.totalWidth();  // RNC; see above for notes and below for compensation

    QVector<QLayoutStruct>& a = gi.m_geom_array;
    const int n = a.count();
    int h = 0;  // height for given width
    int mh = 0;  // minimum height for given width

    Q_ASSERT(n == m_list.size());

#ifdef DEBUG_LAYOUT_CALCS
    qDebug() << Q_FUNC_INFO;
#endif

    if (horz(m_dir)) {
        // RNC: HORIZONTAL: maximum of {value for each item}, for each of hfw() and mhfw()
        qtlayouthelpers::qGeomCalc(a, 0, n, 0, w);  // chain, start, count, pos, space
        for (int i = 0; i < n; i++) {
            BoxLayoutHfwItem* box = m_list.at(i);
            h = qMax(h, box->hfw(a.at(i).size));
            mh = qMax(mh, box->minhfw(a.at(i).size));
#ifdef DEBUG_LAYOUT_CALCS
            qDebug() << "... horizontal, item" << i << "width" << a.at(i).size
                     << "taking h to" << h
                     << "and mh to" << mh;
#endif
        }
    } else {
        // RNC: VERTICAL: sum of value for each item, plus spacing, for each of hfw() and mhfw()
        for (int i = 0; i < n; ++i) {
            BoxLayoutHfwItem* box = m_list.at(i);
            const int spacing = a.at(i).spacing;
            h += box->hfw(w);
            mh += box->minhfw(w);
            h += spacing;
            mh += spacing;
#ifdef DEBUG_LAYOUT_CALCS
            qDebug() << "... vertical, item" << i << "width" << w
                     << "has hfw()" << box->hfw(w)
                     << "and minhfw()" << box->minhfw(w)
                     << "taking h to" << h
                     << "and mh to" << mh;
#endif
        }
    }

    HfwInfo hfwinfo;
    hfwinfo.hfw_height = h;
    hfwinfo.hfw_min_height = mh;

    // RNC: compensation back:
    hfwinfo.hfw_height += effmarg.totalHeight();
    hfwinfo.hfw_min_height += effmarg.totalHeight();

    // End of main thinking
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifdef DEBUG_LAYOUT_CALCS
    qDebug().nospace() << "... For layout (contents) width " << w << ":"
                       << " m_hfw_height " << hfwinfo.hfw_height
                       << " m_hfw_min_height " << hfwinfo.hfw_min_height;
#endif
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    m_hfw_cache[w] = hfwinfo;
#else
    m_cached_hfw_width = w;
    m_cached_hfwinfo = hfwinfo;
#endif
    return hfwinfo;
}


QRect BoxLayoutHfw::getContentsRect(const QRect& layout_rect) const
{
    // ... following code from QBoxLayout::setGeometry()
    const QRect& r = layout_rect;  // so variable names match QBoxLayout
    const QRect cr = alignment() ? alignmentRect(r) : r;
    // RNC: ... if there is no alignment, cr is the same as r (meaning that we
    // fill our entire space), but if there is an alignment,  we alter our
    // rectangle; see http://doc.qt.io/qt-5/qlayout.html#alignmentRect

    // Margins effmarg = getEffectiveMargins();
    // QRect s(cr.x() + effmarg.left(), cr.y() + effmarg.top(),
    //         cr.width() - (effmarg.left() + effmarg.right()),
    //         cr.height() - (effmarg.top() + effmarg.bottom()));
    // return s;

    // RNC: s is cr with some margins trimmed off the edge, and looks to
    // be the proper working rectangle within which we'll lay out our
    // child widgets.
    // So, equivalently:
    return effectiveMargins().removeMarginsFrom(cr);
}


QVector<QRect> BoxLayoutHfw::getChildRects(
        const QRect& contents_rect,
        const QVector<QLayoutStruct>& items) const
{
    // following code from QBoxLayout::setGeometry()
    const QRect& s = contents_rect;
    const int n = items.count();
    QVector<QRect> rects(n);
    const Direction visual_dir = getVisualDir();
    for (int i = 0; i < n; ++i) {
        const QLayoutStruct& item = items.at(i);
        switch (visual_dir) {
        case LeftToRight:
            {
                rects[i] = QRect(
                    item.pos,  // left
                    s.y(),  // top
                    item.size,  // width
                    s.height()  // height [NB widget may e.g. align top or bottom within this]
                );
            }
            break;
        case RightToLeft:
            {
                rects[i] = QRect(
                    s.left() + s.right() - item.pos - item.size + 1,  // left
                    s.y(),  // top
                    item.size,  // width
                    s.height()  // height [NB widget may e.g. align top or bottom within this]
                );
            }
            break;
        case TopToBottom:
            rects[i] = QRect(s.x(),  // left
                             item.pos,  // top
                             s.width(),  // width
                             item.size);  // height
            // the "size" solution should equal height-for-width if applicable
            break;
        case BottomToTop:
            rects[i] = QRect(s.x(),  // left
                             s.top() + s.bottom() - item.pos - item.size + 1,  // top
                             s.width(),  // width
                             item.size);  // height
            // the "size" solution should equal height-for-width if applicable
            break;
        }
    }
    return rects;
}


// ----------------------------------------------------------------------------
// Margins
// ----------------------------------------------------------------------------

Margins BoxLayoutHfw::effectiveMargins() const
{
    // RNC: cache added, because we use this quite a lot, and (at least for
    // the #ifdef Q_OS_MAC) there's a bit of thinking involved.
#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    if (m_dirty) {
        clearCaches();
    }
#endif
    if (m_effective_margins.isZero()) {
        const Margins contents_margins = Margins::getContentsMargins(this);
        m_effective_margins = effectiveMargins(contents_margins);
    }
    return m_effective_margins;
}


/*
 The purpose of this function is to make sure that widgets are not laid out outside its layout.
 E.g. the layoutItemRect margins are only meant to take of the surrounding margins/spacings.
 However, if the margin is 0, it can easily cover the area of a widget above it.
*/
Margins BoxLayoutHfw::effectiveMargins(const Margins& contents_margins) const
{
    int l = contents_margins.left();
    int t = contents_margins.top();
    int r = contents_margins.right();
    int b = contents_margins.bottom();

#ifdef Q_OS_MAC
    // RNC: in the original, left/top/right/bottom were pointers to receive
    // values, and tested to make calculation more efficient.
    bool left = true;
    bool top  = true;
    bool right = true;
    bool bottom = true;

    if (horz(m_dir)) {
        BoxLayoutHfwItem* left_box = 0;
        BoxLayoutHfwItem* right_box = 0;

        if (left || right) {
            left_box = m_list.value(0);
            right_box = m_list.value(m_list.count() - 1);
            if (m_dir == BoxLayoutHfw::RightToLeft) {
                qSwap(left_box, right_box);
            }

            int left_delta = 0;
            int right_delta = 0;
            if (left_box) {
                QLayoutItem* itm = left_box->item;
                if (QWidget* w = itm->widget()) {
                    left_delta = itm->geometry().left() - w->geometry().left();
                }
            }
            if (right_box) {
                QLayoutItem* itm = right_box->item;
                if (QWidget* w = itm->widget()) {
                    right_delta = w->geometry().right() - itm->geometry().right();
                }
            }
            QWidget* w = parentWidget();
            Qt::LayoutDirection layout_direction = w ? w->layoutDirection()
                                                     : QApplication::layoutDirection();
            if (layout_direction == Qt::RightToLeft) {
                qSwap(left_delta, right_delta);
            }

            l = qMax(l, left_delta);
            r = qMax(r, right_delta);
        }

        int count = top || bottom ? m_list.count() : 0;
        for (int i = 0; i < count; ++i) {
            BoxLayoutHfwItem* box = m_list.at(i);
            QLayoutItem* itm = box->item;
            QWidget* w = itm->widget();
            if (w) {
                QRect lir = itm->geometry();
                QRect wr = w->geometry();
                if (top) {
                    t = qMax(t, lir.top() - wr.top());
                }
                if (bottom) {
                    b = qMax(b, wr.bottom() - lir.bottom());
                }
            }
        }
    } else {    // vertical layout
        BoxLayoutHfwItem* top_box = 0;
        BoxLayoutHfwItem* bottom_box = 0;

        if (top || bottom) {
            top_box = m_list.value(0);
            bottom_box = m_list.value(m_list.count() - 1);
            if (m_dir == BoxLayoutHfw::BottomToTop) {
                qSwap(top_box, bottom_box);
            }

            if (top && top_box) {
                QLayoutItem* itm = top_box->item;
                QWidget* w = itm->widget();
                if (w) {
                    t = qMax(t, itm->geometry().top() - w->geometry().top());
                }
            }

            if (bottom && bottom_box) {
                QLayoutItem* itm = bottom_box->item;
                QWidget* w = itm->widget();
                if (w) {
                    b = qMax(b, w->geometry().bottom() - itm->geometry().bottom());
                }
            }
        }

        int count = left || right ? m_list.count() : 0;
        for (int i = 0; i < count; ++i) {
            BoxLayoutHfwItem* box = m_list.at(i);
            QLayoutItem* itm = box->item;
            QWidget* w = itm->widget();
            if (w) {
                QRect lir = itm->geometry();
                QRect wr = w->geometry();
                if (left) {
                    l = qMax(l, lir.left() - wr.left());
                }
                if (right) {
                    r = qMax(r, wr.right() - lir.right());
                }
            }
        }
    }
#endif
    return Margins(l, t, r, b);
}


// ----------------------------------------------------------------------------
// Cache management
// ----------------------------------------------------------------------------

inline void BoxLayoutHfw::setDirty()
{
#ifdef DEBUG_LAYOUT_CALCS
    qDebug() << Q_FUNC_INFO;
#endif
    m_dirty = true;
#ifndef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
    m_cached_hfw_width = -1;
    m_cached_hfwinfo = HfwInfo();
    m_effective_margins.clear();
#endif
}


#ifdef BOXLAYOUTHFW_ALTER_FROM_QBOXLAYOUT
void BoxLayoutHfw::clearCaches() const
{
#ifdef DEBUG_LAYOUT_CALCS
    qDebug() << Q_FUNC_INFO;
#endif
    m_hfw_cache.clear();
    m_geom_cache.clear();
    m_effective_margins.clear();
    m_width_last_size_constraints_based_on = -1;
    m_dirty = false;
}
#endif
