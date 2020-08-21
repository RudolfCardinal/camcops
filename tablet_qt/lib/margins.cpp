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

#include "margins.h"
#include <QDebug>
#include <QLayout>
#include <QWidget>

// ============================================================================
// Construction; setting
// ============================================================================

Margins::Margins()
{
    clear();
}


Margins::Margins(const int left, const int top,
                 const int right, const int bottom)
{
    set(left, top, right, bottom);
}


Margins::Margins(const int each_side)
{
    set(each_side, each_side, each_side, each_side);
}


Margins::Margins(int left_right, int top_bottom)
{
    set(left_right, top_bottom, left_right, top_bottom);
}


void Margins::set(const int left, const int top,
                  const int right, const int bottom)
{
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
    rationalize();
}


void Margins::clear()
{
    m_left = 0;
    m_top = 0;
    m_right = 0;
    m_bottom = 0;
}


void Margins::rationalize()
{
    // Ensure nothing is negative
    m_left = qMax(0, m_left);
    m_top = qMax(0, m_top);
    m_right = qMax(0, m_right);
    m_bottom = qMax(0, m_bottom);
}


bool Margins::isZero() const
{
    return m_left == 0 && m_right == 0 && m_top == 0 && m_bottom == 0;
}


// ============================================================================
// Modification
// ============================================================================

void Margins::setLeft(const int width)
{
    m_left = width;
    rationalize();
}


void Margins::setRight(const int width)
{
    m_right = width;
    rationalize();
}


void Margins::setTop(const int height)
{
    m_top = height;
    rationalize();
}


void Margins::setBottom(const int height)
{
    m_bottom = height;
    rationalize();
}


void Margins::addLeft(const int width)
{
    m_left += width;
    rationalize();
}


void Margins::addRight(const int width)
{
    m_right += width;
    rationalize();
}


void Margins::addTop(const int height)
{
    m_top += height;
    rationalize();
}


void Margins::addBottom(const int height)
{
    m_bottom += height;
    rationalize();
}


int& Margins::rleft()
{
    return m_left;
}


int& Margins::rright()
{
    return m_right;
}


int& Margins::rtop()
{
    return m_top;
}


int& Margins::rbottom()
{
    return m_bottom;
}


// ============================================================================
// Calculated information
// ============================================================================

QSize Margins::totalSize() const
{
    return QSize(m_left + m_right, m_top + m_bottom);
}


int Margins::totalHeight() const
{
    return m_top + m_bottom;
}


int Margins::totalWidth() const
{
    return m_left + m_right;
}


int Margins::removeLeftRightMarginsFrom(const int width) const
{
    return width - totalWidth();
}


int Margins::addLeftRightMarginsTo(const int width) const
{
    return width + totalWidth();
}


int Margins::removeTopBottomMarginsFrom(const int height) const
{
    return height - totalHeight();
}


int Margins::addTopBottomMarginsTo(const int height) const
{
    return height + totalHeight();
}


QSize Margins::addMarginsTo(const QSize& size) const
{
    return QSize(size.width() + totalWidth(),
                 size.height() + totalHeight());
}


void Margins::addMarginsToInPlace(QSize& size) const
{
    size.rwidth() += totalWidth();
    size.rheight() += totalHeight();
}


void Margins::addMarginsToInPlace(Margins& other) const
{
    other.m_left += m_left;
    other.m_right += m_right;
    other.m_top += m_top;
    other.m_bottom += m_bottom;
}


QRect Margins::moveRectByTopLeftMargins(const QRect& rect) const
{
    return rect.adjusted(m_left, m_top, m_left, m_top);
}


void Margins::moveRectByTopLeftMarginsInPlace(QRect& rect) const
{
    rect.adjust(m_left, m_top, m_left, m_top);
}


QSize Margins::removeMarginsFrom(const QSize& size) const
{
    return QSize(size.width() - totalWidth(),
                 size.height() - totalHeight());
}


QRect Margins::addMarginsTo(const QRect& rect) const
{
    return rect.adjusted(-m_left, -m_top, +m_right, +m_bottom);
}


QRect Margins::removeMarginsFrom(const QRect& rect) const
{
    return rect.adjusted(+m_left, +m_top, -m_right, -m_bottom);
}


void Margins::addMarginsToInPlace(QRect& rect) const
{
    rect.adjust(-m_left, -m_top, +m_right, +m_bottom);
}


void Margins::removeMarginsFromInPlace(QRect& rect) const
{
    rect.adjust(+m_left, +m_top, -m_right, -m_bottom);
}

// ============================================================================
// Static factories
// ============================================================================

Margins Margins::getContentsMargins(const QWidget* widget)
{
    Margins m;
    if (widget) {
        int left, top, right, bottom;
        widget->getContentsMargins(&left, &top, &right, &bottom);
        m.set(left, top, right, bottom);
    }
    return m;
}


Margins Margins::getContentsMargins(const QLayout* layout)
{
    Margins m;
    if (layout) {
        int left, top, right, bottom;
        layout->getContentsMargins(&left, &top, &right, &bottom);
        m.set(left, top, right, bottom);
    }
    return m;
}


Margins Margins::rectDiff(const QRect& outer, const QRect& inner)
{
    if (!outer.contains(inner)) {
        qWarning() << Q_FUNC_INFO << "-- outer" << outer
                   << "does not contain inner" << inner;
    }
    return Margins(inner.left() - outer.left(),  // left margin
                   inner.top() - outer.top(),  // top margin
                   outer.right() - inner.right(),  // right margin
                   outer.bottom() - outer.bottom());  // bottom margin
}


Margins Margins::subRectMargins(const QSize& outer, const QRect& inner)
{
    return Margins(inner.left(),  // left
                   inner.top(),  // top
                   outer.width() - inner.width() - inner.left(),  // right
                   outer.height() - inner.height() - inner.top());  // bottom
}


Margins Margins::subRectMargins(const QRect& outer, const QRect& inner)
{
    return subRectMargins(outer.size(), inner);
}



// ========================================================================
// For friends
// ========================================================================

QDebug operator<<(QDebug debug, const Margins& m)
{
    debug.nospace()
            << "Margins(left=" << m.m_left
            << ",top=" << m.m_top
            << ",right=" << m.m_right
            << ",bottom=" << m.m_bottom << ")";
    return debug;
}
