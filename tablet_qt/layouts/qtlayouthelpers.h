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

// From qlayoutengine_p.h:

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
#include <QDebug>
#include <QLayout>
#include <QStyle>
#include <QVector>
#include "lib/margins.h"

class QLayoutItem;
class QSpacerItem;
class QWidgetItem;

// ============================================================================
// POLICY
// - This file replicates and/or extends functionality of Qt private classes.
// - As such, DO NOT USE THE EXACT SAME NAMES. To do so requires that some, at
//   least, use the Qt binary versions (e.g. qGeomCalc is implemented in
//   qlayoutengine.cpp and linked in with Qt). Consequently, there is a risk
//   of binary incompatibility.
// - Better to re-implement.
// - Identically re-implemented things have a 'Q' prefixed, e.g.
//      (class) QLayoutStruct -> QQLayoutStruct
//      (function) qGeomCalc -> qqGeomCalc
// ============================================================================


namespace qtlayouthelpers {

// ============================================================================
// Constants
// ============================================================================

extern const QRect QT_DEFAULT_RECT;  // as per QWidgetPrivate::init()


// ============================================================================
// Ancillary structs/classes
// ============================================================================

struct QQLayoutStruct
{
    // RNC: this is QLayoutStruct, from qlayoutengine_p.h, except with minor
    // variable name changes.
    // The parameters are written by QBoxLayout::setGeometry().
    // The results (pos, size) are written by qqGeomCalc().
    // However, in the Qt original, I can't see that anything called init(),
    // which was "inline void init(int stretch_factor = 0, int min_size = 0)",
    // so I've converted it to a constructor (I think variables may have been
    // used uninitialized).
    // ... ah, no, init() is used by QGridLayout(), so extra bits added

    QQLayoutStruct(int stretch_factor = 0, int min_size = 0) :
        // RNC: These are not set by init(), but we don't want them
        // uninitialized:
        done(false),
        pos(-1),
        size(-1)
    {
        init(stretch_factor, min_size);
    }

    inline void init(int stretch_factor = 0, int min_size = 0)
    {
        stretch = stretch_factor;
        minimum_size = size_hint = min_size;
        maximum_size = QLAYOUTSIZE_MAX;
        expansive = false;
        empty = true;
        spacing = 0;
    }

    int smartSizeHint()
    {
        return (stretch > 0) ? minimum_size : size_hint;
    }

    int effectiveSpacer(int uniform_spacer) const
    {
        Q_ASSERT(uniform_spacer >= 0 || spacing >= 0);
        return (uniform_spacer >= 0) ? uniform_spacer : spacing;
    }

    // parameters
    int stretch;
    int size_hint;  // RNC: in the direction of layout travel (e.g. vertical for a vertical layout)
    int maximum_size;  // RNC: in the direction of layout travel
    int minimum_size;  // RNC: in the direction of layout travel
    int spacing;
    bool expansive;
    bool empty;

    // temporary storage
    bool done;

    // result
    int pos;  // position, e.g. x for horizontal, y for vertical
    int size;  // width for horizontal, height for vertical
};


QDebug operator<<(QDebug debug, const QQLayoutStruct& ls);


// ============================================================================
// Helper functions
// ... declared in qlayoutengine_p.h, implemented in qlayoutengine.cpp
// ============================================================================

void qGeomCalc(QVector<QQLayoutStruct>& chain, int start, int count,
                int pos, int space, int spacer = -1);

QSize qSmartMinSize(const QSize& sizeHint, const QSize& minSizeHint,
                    const QSize& minSize, const QSize& maxSize,
                    const QSizePolicy& sizePolicy);
QSize qSmartMinSize(const QWidgetItem* i);
QSize qSmartMinSize(const QWidget* w);

QSize qSmartMaxSize(const QSize& sizeHint,
                    const QSize& minSize, const QSize& maxSize,
                    const QSizePolicy& sizePolicy, Qt::Alignment align = 0);
QSize qSmartMaxSize(const QWidgetItem* i, Qt::Alignment align = 0);
QSize qSmartMaxSize(const QWidget* w, Qt::Alignment align = 0);

int qSmartSpacing(const QLayout* layout, QStyle::PixelMetric pm);

void qMaxExpCalc(int& max, bool& exp, bool &empty,
                 int boxmax, bool boxexp, bool boxempty);


// ============================================================================
// Static-looking things from QLayoutPrivate
// ============================================================================

// was QLayoutPrivate::createWidgetItem, from qlayout.cpp,
// with RNC extra of use_hfw_capable_item
QWidgetItem* createWidgetItem(const QLayout* layout, QWidget* widget,
                              bool use_hfw_capable_item = false);

// was QLayoutPrivate::createSpacerItem, from qlayout.cpp
QSpacerItem* createSpacerItem(
        const QLayout* layout, int w, int h,
        QSizePolicy::Policy h_policy = QSizePolicy::Minimum,
        QSizePolicy::Policy v_policy = QSizePolicy::Minimum);

// was QLayoutPrivate::checkWidget(QWidget* widget), from qlayout.cpp
bool checkWidget(QWidget* widget, QLayout* from);

// was QLayoutPrivate::checkLayout(QLayout* otherLayout), from qlayout.cpp
bool checkLayout(QLayout* other_layout, QLayout* from);


// ============================================================================
// Static-looking things from QLayoutPrivate
// ============================================================================

QRect defaultRectOfWidth(int width);


}  // namespace qtlayouthelpers
