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

// #define DEBUG_LAYOUT
// #define DEBUG_SET_GEOMETRY

// #define DISABLE_SET_GEOMETRY  // for debugging

#include "widgetitemhfw.h"
#include <QDebug>
#include <QStyle>
#include <QWidget>
#include "lib/sizehelpers.h"


// ============================================================================
// Constants
// ============================================================================

const int IGNORE_SIZEHINT = QSizePolicy::IgnoreFlag;
const int CAN_SHRINK = QSizePolicy::ShrinkFlag;
const int CAN_GROW =
        QSizePolicy::GrowFlag |
        QSizePolicy::ExpandFlag |
        QSizePolicy::IgnoreFlag;
const int WANTS_TO_GROW =
        QSizePolicy::ExpandFlag |
        QSizePolicy::IgnoreFlag;


// ============================================================================
// WidgetItemHfw
// ============================================================================

WidgetItemHfw::WidgetItemHfw(QWidget* widget) :
    QWidgetItemV2(widget)
{
}


QSize WidgetItemHfw::sizeHint() const
{
    // Simpler than QWidgetItem. It doesn't support any of the internal
    // margin nonsense (we leave that to the layout). That is, we ignore:
    // - Qt::WA_LayoutUsesWidgetRect
    // - toLayoutItemSize
    //   - toLayoutItemRect
    //     - QWidgetPrivate::leftLayoutItemMargin
    //     - QWidgetPrivate::topLayoutItemMargin
    //     - QWidgetPrivate::rightLayoutItemMargin
    //     - QWidgetPrivate::bottomLayoutItemMargin

    QSize& hint = m_cached_sizehint;  // shorthand
    if (!hint.isValid()) {
        if (isEmpty()) {
            hint = QSize(0, 0);
        } else {
            hint = wid->sizeHint()
                    .expandedTo(wid->minimumSizeHint())
                    .boundedTo(wid->maximumSize())
                    .expandedTo(wid->minimumSize());
            // But we continue to respect "ignore my size hint":
            const QSizePolicy sp = wid->sizePolicy();
            if (sp.horizontalPolicy() & IGNORE_SIZEHINT) {
                hint.setWidth(0);
            }
            if (sp.verticalPolicy() & IGNORE_SIZEHINT) {
                hint.setHeight(0);
            }
        }
#ifdef DEBUG_LAYOUT
        qDebug().nospace()
                << Q_FUNC_INFO
                << " [wid->metaObject()->className() == "
                << wid->metaObject()->className()
                << ", wid->testAttribute(Qt::WA_LayoutUsesWidgetRect) == "
                << wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
                << ", wid->minimumSize() == " << wid->minimumSize()
                << ", wid->minimumSizeHint() == " << wid->minimumSizeHint()
                << ", wid->sizeHint() == " << wid->sizeHint()
                << ", wid->sizePolicy() == " << wid->sizePolicy()
                << ", wid->sizePolicy().hasHeightForWidth() == "
                << wid->sizePolicy().hasHeightForWidth()
                // << ", wid->sizePolicy().horizontalPolicy() & QSizePolicy::ShrinkFlag == "
                // << (wid->sizePolicy().horizontalPolicy() & QSizePolicy::ShrinkFlag)
                << "]";
        qDebug() << Q_FUNC_INFO << "->" << hint;
#endif
    }
    return hint;
}


QSize WidgetItemHfw::minimumSize() const
{
    // Originals:
    //
    //    class QLayoutItem {
    //        // ...
    //        virtual QSize minimumSize() const = 0;
    //    };
    //
    //    class QWidgetItem : public QLayoutItem { ... };
    //
    //    QSize QWidgetItem::minimumSize() const
    //    {
    //        if (isEmpty())
    //            return QSize(0, 0);
    //        return !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
    //               ? toLayoutItemSize(wid->d_func(), qSmartMinSize(this))
    //               : qSmartMinSize(this);
    //    }
    //
    // ... noting that d_func() returns QWidget's "QWidgetPrivate* d_ptr;"
    // ... see https://wiki.qt.io/D-Pointer
    //
    //    class QWidgetItemV2 : public QWidgetItem { ... }
    //
    //    QSize QWidgetItemV2::minimumSize() const
    //    {
    //        if (isEmpty())
    //            return QSize(0, 0);
    //
    //        if (useSizeCache()) {  // RNC: I think generally true
    //            updateCacheIfNecessary();
    //            return q_cachedMinimumSize;
    //        } else {
    //            return QWidgetItem::minimumSize();
    //        }
    //    }
    //
    //    void QWidgetItemV2::updateCacheIfNecessary() const  // RNC: NOT VIRTUAL
    //    {
    //        if (q_cachedMinimumSize.width() != Dirty)
    //            return;
    //
    //        const QSize sizeHint(wid->sizeHint());
    //        const QSize minimumSizeHint(wid->minimumSizeHint());
    //        const QSize minimumSize(wid->minimumSize());
    //        const QSize maximumSize(wid->maximumSize());
    //        const QSizePolicy sizePolicy(wid->sizePolicy());
    //        const QSize expandedSizeHint(sizeHint.expandedTo(minimumSizeHint));
    //
    //        const QSize smartMinSize(qSmartMinSize(sizeHint, minimumSizeHint, minimumSize, maximumSize, sizePolicy));
    //        const QSize smartMaxSize(qSmartMaxSize(expandedSizeHint, minimumSize, maximumSize, sizePolicy, align));
    //
    //        const bool useLayoutItemRect = !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect);
    //
    //        q_cachedMinimumSize = useLayoutItemRect
    //               ? toLayoutItemSize(wid->d_func(), smartMinSize)
    //               : smartMinSize;
    //
    //        q_cachedSizeHint = expandedSizeHint;
    //        q_cachedSizeHint = q_cachedSizeHint.boundedTo(maximumSize)
    //                                           .expandedTo(minimumSize);
    //        q_cachedSizeHint = useLayoutItemRect
    //               ? toLayoutItemSize(wid->d_func(), q_cachedSizeHint)
    //               : q_cachedSizeHint;
    //
    //        if (wid->sizePolicy().horizontalPolicy() == QSizePolicy::Ignored)
    //            q_cachedSizeHint.setWidth(0);
    //        if (wid->sizePolicy().verticalPolicy() == QSizePolicy::Ignored)
    //            q_cachedSizeHint.setHeight(0);
    //
    //        q_cachedMaximumSize = useLayoutItemRect
    //                   ? toLayoutItemSize(wid->d_func(), smartMaxSize)
    //                   : smartMaxSize;
    //    }

    QSize& minsize = m_cached_minsize;  // shorthand
    if (!minsize.isValid()) {
        if (isEmpty()) {
            minsize = QSize(0, 0);
        } else {
            const QSizePolicy sp = wid->sizePolicy();
            if (sp.horizontalPolicy() & IGNORE_SIZEHINT) {
                minsize = QSize(0, 0);
            } else {
                minsize = sizeHint();
                if (sp.horizontalPolicy() & CAN_SHRINK) {
                    minsize.setWidth(0);
                }
                if (sp.verticalPolicy() & CAN_SHRINK) {
                    minsize.setHeight(0);
                }
                minsize = minsize.expandedTo(wid->minimumSize())
                        .expandedTo(wid->minimumSizeHint());
            }
        }
#ifdef DEBUG_LAYOUT
        qDebug() << Q_FUNC_INFO << "->" << minsize;
#endif
    }
    return minsize;
}


QSize WidgetItemHfw::maximumSize() const
{
    QSize& maxsize = m_cached_maxsize;  // shorthand
    if (!maxsize.isValid()) {
        if (isEmpty()) {
            maxsize = QSize(0, 0);
        } else {
            const QSizePolicy sp = wid->sizePolicy();
            if (sp.horizontalPolicy() & IGNORE_SIZEHINT) {
                maxsize = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            } else {
                maxsize = sizeHint();
                // Horizontal tweaks:
                if (sp.horizontalPolicy() & CAN_GROW) {
                    maxsize.setWidth(QWIDGETSIZE_MAX);
                }
                // Vertical tweaks:
                if (sp.verticalPolicy() & CAN_GROW) {
                    maxsize.setHeight(QWIDGETSIZE_MAX);
                } else if (hasHeightForWidth()) {
                    // A height-for-width widget that cannot expand vertically
                    // beyond its assigned height.
                    //
                    // For height-for-width widgets, the sizeHint() height
                    // isn't necessarily constraining -- it's the HFW
                    // transformation of the final width that is.
                    // We have two realistic choices:

                    // (a) We don't know, so we don't constrain.
                    maxsize.setHeight(QWIDGETSIZE_MAX);

                    // (b) HFW widgets tend to be "area conserving", in which
                    //     case their height is maximum when their width is
                    //     smallest, or "aspect ratio conserving", in which
                    //     case their height is maximum when their width is
                    //     largest. We could test accordingly:
                    //
                    //          const int h1 = heightForWidth(1);
                    //          const int h2 = heightForWidth(QWIDGETSIZE_MAX);
                    //          const int hmax = qMax(h1, h2);
                    //          maxsize.setHeight(hmax);
                    //
                    //     However, it's not impossible that a widget has
                    //     maximum height at some intermediate width -- there's
                    //     probably a composite widget (e.g. one with its own
                    //     layout) for which this is true. We could do:
                    //
                    // (c) Some sort of iteration through all possible widths,
                    //     or gradient descent, to find the maximum height.
                    //
                    // Let's try (a) for simplicity!
                }
                maxsize = maxsize.boundedTo(wid->maximumSize());
            }
        }
#ifdef DEBUG_LAYOUT
        qDebug() << Q_FUNC_INFO << "->" << maxsize;
#endif
    }
    return maxsize;
}


bool WidgetItemHfw::hasHeightForWidth() const
{
    if (isEmpty()) {
        return false;
    }
    return wid->hasHeightForWidth();
}


int WidgetItemHfw::heightForWidth(int w) const
{
    if (isEmpty()) {
        return -1;
    }
    if (!hasHeightForWidth()) {
        return -1;
    }
    if (!m_width_to_height.contains(w)) {
        const int h = wid->heightForWidth(w);
        m_width_to_height[w] = h;
    }
    return m_width_to_height[w];
}


void WidgetItemHfw::invalidate()
{
    m_cached_sizehint = QSize();
    m_cached_minsize = QSize();
    m_cached_maxsize = QSize();
    m_width_to_height.clear();
}


void WidgetItemHfw::setGeometry(const QRect& rect)
{
#ifdef DISABLE_SET_GEOMETRY
    QWidgetItemV2::setGeometry(rect);
#else
    // Note the problem that QWidgetItem::setGeometry() will mess up
    // height-for-width widgets.
    //
    // ... QLayoutItem::setGeometry()
    // ... overridden by QWidgetItem::setGeometry()
    // ... which does
    //     QSize s = r.size().boundedTo(maximumSize() + widgetRectSurplus);
    // ... which calls QWidgetItem::maximumSize()
    //     ... available for inspection
    // ... which calls qSmartMaxSize() in qlayoutengine.cpp
    //     ... which is probably the problem; under some (common?)
    //         circumstances, if the vertical policy doesn't have
    //         QSizePolicy::GrowFlag set, the maximum height is set to
    //         the sizeHint() height, without any regard to
    //         height-for-width.
    //
    // So we replace it here.
    //
    // WA_LayoutUsesWidgetRect is ignored; may be relevant under MacOS:
    // - https://stackoverflow.com/questions/3978889/why-is-qhboxlayout-causing-widgets-to-overlap
    // - https://stackoverflow.com/questions/41452512/how-to-remove-margins-of-qlayout-completely-mac-os-specific?noredirect=1&lq=1
    // - https://doc.qt.io/archives/qt-4.8/qmacnativewidget.html

#ifdef DEBUG_SET_GEOMETRY
    qDebug() << Q_FUNC_INFO << ": setting layout item geometry to" << rect;
#endif

    if (isEmpty()) {
        // No visible widget (and not an invisible widget retaining its size).
        return;
    }

    // ------------------------------------------------------------------------
    // Set the widget's target size.
    // ------------------------------------------------------------------------
    const QSize available = rect.size();
    QSize widget_size(sizeHint());  // layout item's preferred size
    // ... which in our simplified layout system is also the widget's
    //     preferred size;
    // ... except that this will be (0,0) if the widget's size policy is
    //     "Ignored".
    const QSizePolicy sp = wid->sizePolicy();  // widget's size policy

    // We are trying to get as close as possible to what we were told.
    const bool any_size_widget = !widget_size.isValid() ||
            widget_size == QSize(0, 0);
    // ... e.g. background stripe widgets made from a generic QWidget

    if (sp.horizontalPolicy() & WANTS_TO_GROW ||
            (hasHeightForWidth() && sp.horizontalPolicy() & CAN_GROW) ||
            any_size_widget) {  // e.g. background stripe widgets
        widget_size.setWidth(available.width());
    }
    if (sp.verticalPolicy() & WANTS_TO_GROW ||
            any_size_widget) {
        widget_size.setHeight(available.height());
    }

    // Apply constraints
    widget_size = widget_size
            .expandedTo(minimumSize())
            .boundedTo(maximumSize())
            .boundedTo(available);

#ifdef DEBUG_SET_GEOMETRY
    qDebug() << "... widget_size =" << widget_size;
#endif

    if (hasHeightForWidth()) {
        // Redo the height as necessary for a height-for-width widget.
        int h = heightForWidth(widget_size.width());
#ifdef DEBUG_SET_GEOMETRY
        qDebug() << "... HFW: width" << widget_size.width() << "-> height" << h;
#endif
        if (sp.verticalPolicy() & WANTS_TO_GROW) {
            h = available.height();
        }
        widget_size.setHeight(h);
        // Re-apply constraints
        widget_size = widget_size
                .expandedTo(minimumSize())
                .boundedTo(maximumSize())
                .boundedTo(available);
#ifdef DEBUG_SET_GEOMETRY
        qDebug().nospace()
                << "minimumSize() = " << minimumSize()
                << ", maximumSize() = " << maximumSize()
                << ", available = " << available;
        qDebug() << "... widget_size (after HFW) =" << widget_size;
#endif
    }

    // ------------------------------------------------------------------------
    // If the widget is smaller than the layout "box", it needs alignment.
    // ------------------------------------------------------------------------
    int x = rect.x();
    int y = rect.y();
    const Qt::Alignment align_horiz = QStyle::visualAlignment(
                wid->layoutDirection(), align);
    if (align_horiz & Qt::AlignRight) {
        // Right align
        x = x + (rect.width() - widget_size.width());
    } else if (!(align_horiz & Qt::AlignLeft)) {
        // Centre align
        x = x + (rect.width() - widget_size.width()) / 2;
    }

    if (align & Qt::AlignBottom) {
        // Bottom align
        y = y + (rect.height() - widget_size.height());
    } else if (!(align & Qt::AlignTop)) {
        // Vertical centre align
        y = y + (rect.height() - widget_size.height()) / 2;
    }

    // ------------------------------------------------------------------------
    // Tell the widget.
    // ------------------------------------------------------------------------
    const QRect widget_geom(x, y, widget_size.width(), widget_size.height());
#ifdef DEBUG_SET_GEOMETRY
    qDebug() << "... calling widget->setGeometry() with " << widget_geom;
#endif
    wid->setGeometry(widget_geom);
#endif
}
