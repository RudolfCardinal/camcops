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

// From qlayoutengine.cpp:

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

// #define DEBUG_LAYOUT

#include "qtlayouthelpers.h"
#include <QDebug>
#include <QWidget>
#include "layouts/widgetitemhfw.h"


// ============================================================================
// Constants
// ============================================================================

const QRect qtlayouthelpers::QT_DEFAULT_RECT(0, 0, 640, 480);  // as per QWidgetPrivate::init()


// ============================================================================
// Ancillary structs/classes
// ============================================================================

QDebug qtlayouthelpers::operator<<(QDebug debug,
                                   const qtlayouthelpers::QQLayoutStruct& ls)
{
    debug.nospace()
            << "QQLayoutStruct(stretch " << ls.stretch
            << ", size_hint " << ls.size_hint
            << ", maximum_size " << ls.maximum_size
            << ", minimum_size " << ls.minimum_size
            << ", spacing " << ls.spacing
            << ", expansive " << ls.expansive
            << ", empty " << ls.empty
            << " [done " << ls.done
            << ", pos " << ls.pos
            << ", size " << ls.size
            << "])";
    return debug;
}


// ============================================================================
// Helper functions for the helper functions
// ============================================================================
// from qlayoutengine_p.h

using Fixed64 = qint64;


static inline Fixed64 toFixed(const int i)
{
    return static_cast<Fixed64>(i * 256);
}


static inline int fRound(const Fixed64 i)
{
    return (i % 256 < 128)
            ? static_cast<int>(i / 256)
            : static_cast<int>(1 + i / 256);
}


// ============================================================================
// Helper functions
// ============================================================================

/*
  This is the main workhorse of the QGridLayout. It portions out
  available space to the chain's children.

  The calculation is done in fixed point: "fixed" variables are
  scaled by a factor of 256.

  If the layout runs "backwards" (i.e. RightToLeft or Up) the layout
  is computed mirror-reversed, and it's the caller's responsibility
  do reverse the values before use.

  chain contains input and output parameters describing the geometry.
  count is the count of items in the chain; pos and space give the
  interval (relative to parentWidget topLeft).

  RNC: this calculates in one direction only (e.g. in the vertical direction
  for QVBoxLayout, or the horizontal direction for QHBoxLayout). For a
  QGridLayout, it's called at least twice (e.g. QGridLayoutPrivate::distribute).

  - pos: starting position
  - space: available space
*/
void qtlayouthelpers::qGeomCalc(QVector<QQLayoutStruct>& chain,
                                const int start,
                                const int count,
                                const int pos,
                                const int space,
                                int spacer)
{
    int c_hint = 0;
    int c_min = 0;
    int c_max = 0;
    int sum_stretch = 0;
    int sum_spacing = 0;
    int expanding_count = 0;

    bool all_empty_nonstretch = true;
    int pending_spacing = -1;
    int spacer_count = 0;
    int i;

    for (i = start; i < start + count; i++) {
        QQLayoutStruct* data = &chain[i];  // RNC: extra Q

        data->done = false;
        c_hint += data->smartSizeHint();
        c_min += data->minimum_size;
        c_max += data->maximum_size;
        sum_stretch += data->stretch;
        if (!data->empty) {
            // Using pending_spacing, we ensure that the spacing for the last
            // (non-empty) item is ignored.

            if (pending_spacing >= 0) {
                sum_spacing += pending_spacing;
                ++spacer_count;
            }
            pending_spacing = data->effectiveSpacer(spacer);
        }
        if (data->expansive) {
            expanding_count++;
        }
        all_empty_nonstretch = all_empty_nonstretch && data->empty &&
                !data->expansive && data->stretch <= 0;
    }

    int extraspace = 0;

    if (space < c_min + sum_spacing) {
        // Less space than minimumSize; take from the biggest first

        const int min_size = c_min + sum_spacing;

        // shrink the spacers proportionally
        if (spacer >= 0) {
            spacer = min_size > 0 ? spacer * space / min_size : 0;
            sum_spacing = spacer * spacer_count;
        }

        QVarLengthArray<int, 32> minimum_sizes;
        minimum_sizes.reserve(count);

        for (i = start; i < start + count; i++) {
            minimum_sizes << chain.at(i).minimum_size;
        }

        std::sort(minimum_sizes.begin(), minimum_sizes.end());

        const int space_left = space - sum_spacing;

        int sum = 0;
        int idx = 0;
        int space_used = 0;
        int current = 0;
        while (idx < count && space_used < space_left) {
            current = minimum_sizes.at(idx);
            space_used = sum + current * (count - idx);
            sum += current;
            ++idx;
        }
        --idx;
        const int deficit = space_used - space_left;

        const int items = count - idx;

        // If we truncate all items to "current", we would get "deficit" too
        // many pixels. Therefore, we have to remove deficit/items from each
        // item bigger than maxval. The actual value to remove is
        // deficitPerItem + remainder/items
        // "rest" is the accumulated error from using integer arithmetic.

        const int deficit_per_item = deficit / items;
        const int remainder = deficit % items;
        const int maxval = current - deficit_per_item;

        int rest = 0;
        for (i = start; i < start + count; i++) {
            int maxv = maxval;
            rest += remainder;
            if (rest >= items) {
                maxv--;
                rest -= items;
            }
            QQLayoutStruct* data = &chain[i];  // RNC: extra Q
            data->size = qMin(data->minimum_size, maxv);
            data->done = true;
        }
    } else if (space < c_hint + sum_spacing) {
        // Less space than smartSizeHint(), but more than minimumSize.
        // Currently take space equally from each, as in Qt 2.x.
        // Commented-out lines will give more space to stretchier items.

        int n = count;
        int space_left = space - sum_spacing;
        int overdraft = c_hint - space_left;

        // first give to the fixed ones:
        for (i = start; i < start + count; i++) {
            QQLayoutStruct* data = &chain[i];  // RNC: extra q
            if (!data->done && data->minimum_size >= data->smartSizeHint()) {
                data->size = data->smartSizeHint();
                data->done = true;
                space_left -= data->smartSizeHint();
                // sumStretch -= data->stretch;
                n--;
            }
        }
        bool finished = n == 0;
        while (!finished) {
            finished = true;
            const Fixed64 fp_over = toFixed(overdraft);
            Fixed64 fp_w = 0;

            for (i = start; i < start + count; i++) {
                QQLayoutStruct* data = &chain[i];
                if (data->done) {
                    continue;
                }
                // if (sumStretch <= 0)
                fp_w += fp_over / n;
                // else
                //    fp_w += (fp_over * data->stretch) / sumStretch;
                int w = fRound(fp_w);
                data->size = data->smartSizeHint() - w;
                fp_w -= toFixed(w); // give the difference to the next
                if (data->size < data->minimum_size) {
                    data->done = true;
                    data->size = data->minimum_size;
                    finished = false;
                    overdraft -= data->smartSizeHint() - data->minimum_size;
                    // sumStretch -= data->stretch;
                    n--;
                    break;
                }
            }
        }
    } else { // extra space
        int n = count;
        int space_left = space - sum_spacing;
        // first give to the fixed ones, and handle non-expansiveness
        for (i = start; i < start + count; i++) {
            QQLayoutStruct* data = &chain[i];  // RNC: extra Q
            if (!data->done &&
                    (data->maximum_size <= data->smartSizeHint() ||
                     (!all_empty_nonstretch && data->empty &&
                      !data->expansive && data->stretch == 0))) {
                data->size = data->smartSizeHint();
                data->done = true;
                space_left -= data->size;
                sum_stretch -= data->stretch;
                if (data->expansive) {
                     expanding_count--;
                }
                n--;
            }
        }
        extraspace = space_left;

        // Do a trial distribution and calculate how much it is off.
        // If there are more deficit pixels than surplus pixels, give
        // the minimum size items what they need, and repeat.
        // Otherwise give to the maximum size items, and repeat.
        //
        // Paul Olav Tvete has a wonderful mathematical proof of the
        // correctness of this principle, but unfortunately this
        // comment is too small to contain it.

        int surplus, deficit;
        do {
            surplus = deficit = 0;
            const Fixed64 fp_space = toFixed(space_left);
            Fixed64 fp_w = 0;
            for (i = start; i < start + count; i++) {
                QQLayoutStruct* data = &chain[i];  // RNC: extra Q
                if (data->done) {
                    continue;
                }
                extraspace = 0;
                if (sum_stretch > 0) {
                    fp_w += (fp_space * data->stretch) / sum_stretch;
                } else if (expanding_count > 0) {
                    fp_w += (fp_space * (data->expansive ? 1 : 0)) / expanding_count;
                } else {
                    fp_w += fp_space * 1 / n;
                }
                const int w = fRound(fp_w);
                data->size = w;
                fp_w -= toFixed(w); // give the difference to the next
                if (w < data->smartSizeHint()) {
                    deficit +=  data->smartSizeHint() - w;
                } else if (w > data->maximum_size) {
                    surplus += w - data->maximum_size;
                }
            }
            if (deficit > 0 && surplus <= deficit) {
                // give to the ones that have too little
                for (i = start; i < start + count; i++) {
                    QQLayoutStruct *data = &chain[i];  // RNC: extra Q
                    if (!data->done && data->size < data->smartSizeHint()) {
                        data->size = data->smartSizeHint();
                        data->done = true;
                        space_left -= data->smartSizeHint();
                        sum_stretch -= data->stretch;
                        if (data->expansive) {
                            expanding_count--;
                        }
                        n--;
                    }
                }
            }
            if (surplus > 0 && surplus >= deficit) {
                // take from the ones that have too much
                for (i = start; i < start + count; i++) {
                    QQLayoutStruct* data = &chain[i];  // RNC: extra Q
                    if (!data->done && data->size > data->maximum_size) {
                        data->size = data->maximum_size;
                        data->done = true;
                        space_left -= data->maximum_size;
                        sum_stretch -= data->stretch;
                        if (data->expansive) {
                            expanding_count--;
                        }
                        n--;
                    }
                }
            }
        } while (n > 0 && surplus != deficit);
        if (n == 0) {
            extraspace = space_left;
        }
    }

    // As a last resort, we distribute the unwanted space equally
    // among the spacers (counting the start and end of the chain). We
    // could, but don't, attempt a sub-pixel allocation of the extra
    // space.

    const int extra = extraspace / (spacer_count + 2);
    int p = pos + extra;
    for (i = start; i < start + count; i++) {
        QQLayoutStruct* data = &chain[i];  // RNC: extra Q
        data->pos = p;
        p += data->size;
        if (!data->empty) {
            p += data->effectiveSpacer(spacer) + extra;
        }
    }

#ifdef DEBUG_LAYOUT
    qDebug() << Q_FUNC_INFO;
    qDebug() << "- start" << start <<  "count" << count
             <<  "pos" << pos <<  "space" << space <<  "spacer" << spacer;
    for (i = start; i < start + count; ++i) {
        qDebug() << "- item" << i << ':'
                 << "min" << chain[i].minimum_size
                 << "hint" << chain[i].smartSizeHint()
                 << "max" << chain[i].maximum_size
                 << "stretch" << chain[i].stretch
                 << "empty" << chain[i].empty
                 << "expansive" << chain[i].expansive
                 << "spacing" << chain[i].spacing;
        qDebug() << "- result: pos" << chain[i].pos << "size" << chain[i].size;
    }
#endif
}


QSize qtlayouthelpers::qSmartMinSize(
        const QSize& sizeHint, const QSize& minSizeHint, const QSize& minSize,
        const QSize& maxSize, const QSizePolicy& sizePolicy)
{
    QSize s(0, 0);

    if (sizePolicy.horizontalPolicy() != QSizePolicy::Ignored) {
        if (sizePolicy.horizontalPolicy() & QSizePolicy::ShrinkFlag) {
            s.setWidth(minSizeHint.width());
        } else {
            s.setWidth(qMax(sizeHint.width(), minSizeHint.width()));
        }
    }

    if (sizePolicy.verticalPolicy() != QSizePolicy::Ignored) {
        if (sizePolicy.verticalPolicy() & QSizePolicy::ShrinkFlag) {
            s.setHeight(minSizeHint.height());
        } else {
            s.setHeight(qMax(sizeHint.height(), minSizeHint.height()));
        }
    }

    s = s.boundedTo(maxSize);
    if (minSize.width() > 0) {
        s.setWidth(minSize.width());
    }
    if (minSize.height() > 0) {
        s.setHeight(minSize.height());
    }

    return s.expandedTo(QSize(0, 0));
}


QSize qtlayouthelpers::qSmartMinSize(const QWidgetItem* i)
{
    QWidget* w = const_cast<QWidgetItem*>(i)->widget();  // RNC: nasty!
    return qSmartMinSize(w->sizeHint(), w->minimumSizeHint(),
                         w->minimumSize(), w->maximumSize(),
                         w->sizePolicy());
}


QSize qtlayouthelpers::qSmartMinSize(const QWidget* w)
{
    return qSmartMinSize(w->sizeHint(), w->minimumSizeHint(),
                         w->minimumSize(), w->maximumSize(),
                         w->sizePolicy());
}


QSize qtlayouthelpers::qSmartMaxSize(
        const QSize& sizeHint, const QSize& minSize, const QSize& maxSize,
        const QSizePolicy& sizePolicy, const Qt::Alignment align)
{
    if (align & Qt::AlignHorizontal_Mask && align & Qt::AlignVertical_Mask) {
        return QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX);
    }
    QSize s = maxSize;
    const QSize hint = sizeHint.expandedTo(minSize);
    if (s.width() == QWIDGETSIZE_MAX && !(align & Qt::AlignHorizontal_Mask)) {
        if (!(sizePolicy.horizontalPolicy() & QSizePolicy::GrowFlag)) {
            s.setWidth(hint.width());
        }
    }

    if (s.height() == QWIDGETSIZE_MAX && !(align & Qt::AlignVertical_Mask)) {
        if (!(sizePolicy.verticalPolicy() & QSizePolicy::GrowFlag)) {
            s.setHeight(hint.height());
        }
    }

    if (align & Qt::AlignHorizontal_Mask) {
        s.setWidth(QLAYOUTSIZE_MAX);
    }
    if (align & Qt::AlignVertical_Mask) {
        s.setHeight(QLAYOUTSIZE_MAX);
    }
    return s;
}


QSize qtlayouthelpers::qSmartMaxSize(const QWidgetItem* i,
                                     const Qt::Alignment align)
{
    QWidget* w = const_cast<QWidgetItem*>(i)->widget();  // RNC: nasty!
    return qSmartMaxSize(w->sizeHint().expandedTo(w->minimumSizeHint()),
                         w->minimumSize(),
                         w->maximumSize(),
                         w->sizePolicy(),
                         align);
}


QSize qtlayouthelpers::qSmartMaxSize(const QWidget* w, const
                                     Qt::Alignment align)
{
    return qSmartMaxSize(w->sizeHint().expandedTo(w->minimumSizeHint()),
                         w->minimumSize(),
                         w->maximumSize(),
                         w->sizePolicy(),
                         align);
}


int qtlayouthelpers::qSmartSpacing(const QLayout* layout,
                                   const QStyle::PixelMetric pm)
{
    QObject* parent = layout->parent();
    if (!parent) {
        return -1;
    }
    if (parent->isWidgetType()) {
        auto pw = static_cast<QWidget*>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    }
    return static_cast<QLayout*>(parent)->spacing();
}


// from qlayoutengine_p.h
// original is static: http://stackoverflow.com/questions/558122/what-is-a-static-function
// ... and inline
/*
  Modify total maximum (max), total expansion (exp), and total empty
  when adding boxmax/boxexp.

  Expansive boxes win over non-expansive boxes.
  Non-empty boxes win over empty boxes.
*/
void qtlayouthelpers::qMaxExpCalc(int& max,
                                  bool& exp,
                                  bool &empty,
                                  const int boxmax,
                                  const bool boxexp,
                                  const bool boxempty)
{
    if (exp) {
        if (boxexp) {
            max = qMax(max, boxmax);
        }
    } else {
        if (boxexp || (empty && (!boxempty || max == 0))) {
            max = boxmax;
        } else if (empty == boxempty) {
            max = qMin(max, boxmax);
        }
    }
    exp = exp || boxexp;
    empty = empty && boxempty;
}


// ============================================================================
// Static-looking things from QLayoutPrivate
// ============================================================================

/*
RNC: REMOVED:

// Static item factory functions that allow for hooking things in Designer
QLayoutPrivate::QWidgetItemFactoryMethod QLayoutPrivate::widgetItemFactoryMethod = 0;
QLayoutPrivate::QSpacerItemFactoryMethod QLayoutPrivate::spacerItemFactoryMethod = 0;
*/

// was QLayoutPrivate::createWidgetItem
QWidgetItem* qtlayouthelpers::createWidgetItem(const QLayout* layout,
                                               QWidget* widget,
                                               const bool use_hfw_capable_item)
{
    Q_UNUSED(layout)  // RNC
    /*  // RNC: removed
    if (widgetItemFactoryMethod)
        if (QWidgetItem* wi = (*widgetItemFactoryMethod)(layout, widget))
            return wi;
    */
    if (use_hfw_capable_item) {
        return new WidgetItemHfw(widget);
    }
    return new QWidgetItemV2(widget);
}


// was QLayoutPrivate::createSpacerItem
QSpacerItem* qtlayouthelpers::createSpacerItem(
        const QLayout* layout,
        const int w, const int h,
        const QSizePolicy::Policy h_policy,
        const QSizePolicy::Policy v_policy)
{
    Q_UNUSED(layout)  // RNC
    /*  // RNC: removed
    if (spacerItemFactoryMethod)
        if (QSpacerItem* si = (*spacerItemFactoryMethod)(layout, w, h, hPolicy, vPolicy))
            return si;
    */
    return new QSpacerItem(w, h,  h_policy, v_policy);
}


/*
    Returns \c true if the \a widget can be added to the \a layout;
    otherwise returns \c false.

    RNC: Was originally QLayoutPrivate::checkWidget
    ... "from" parameter added to make it a standalone function
*/
bool qtlayouthelpers::checkWidget(QWidget* widget, QLayout* from)
{
    if (Q_UNLIKELY(!widget)) {
        qWarning("QLayout: Cannot add a null widget to %s/%ls",
                 from->metaObject()->className(),
                 qUtf16Printable(from->objectName()));
        return false;
    }
    if (Q_UNLIKELY(widget == from->parentWidget())) {
        qWarning("QLayout: Cannot add parent widget %s/%ls to its child layout %s/%ls",
                 widget->metaObject()->className(),
                 qUtf16Printable(widget->objectName()),
                 from->metaObject()->className(),
                 qUtf16Printable(from->objectName()));
        return false;
    }
    return true;
}


/*
    Returns \c true if the \a otherLayout can be added to the \a layout;
    otherwise returns \c false.

    RNC: Was originally QLayoutPrivate::checkLayout
    ... "from" parameter added to make it a standalone function
*/
bool qtlayouthelpers::checkLayout(QLayout* other_layout, QLayout* from)
{
    if (Q_UNLIKELY(!other_layout)) {
        qWarning("QLayout: Cannot add a null layout to %s/%ls",
                 from->metaObject()->className(),
                 qUtf16Printable(from->objectName()));
        return false;
    }
    if (Q_UNLIKELY(other_layout == from)) {
        qWarning("QLayout: Cannot add layout %s/%ls to itself",
                 from->metaObject()->className(),
                 qUtf16Printable(from->objectName()));
        return false;
    }
    return true;
}


// ============================================================================
// RNC extras
// ============================================================================

QRect qtlayouthelpers::defaultRectOfWidth(const int width)
{
    return QRect(QPoint(0, 0), QSize(width, QLAYOUTSIZE_MAX));
}
