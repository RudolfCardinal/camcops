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
#include <QRect>
#include <QSize>
class QLayout;
class QWidget;


class Margins {
    // Generic margin structure, because QRect isn't quite right for this,
    // passing around lots of separate integers is awkward and prone to
    // mis-ordering [was that getContentsMargins(&left, &top, &right, &bottom)
    // or getContentsMargins(&left, &right, &top, &bottom)?], and QMargins
    // doesn't do any of the useful things relating to widgets, layouts, or
    // calculations that you might wish.

public:
    // Construct with zero margins
    Margins();

    // Construct with specified margins.
    Margins(int left, int top, int right, int bottom);
    Margins(int each_side);
    Margins(int left_right, int top_bottom);

    // Set the margins.
    void set(int left, int top, int right, int bottom);

    // Clear everything to zero.
    void clear();

    // Ensure that all margins are >=0.
    void rationalize();

    // Are the margins all zero?
    bool isZero() const;

    // Return components:
    inline int left() const { return m_left; }
    inline int top() const { return m_top; }
    inline int right() const { return m_right; }
    inline int bottom() const { return m_bottom; }

    // Set components, and call rationalize():
    void setLeft(int width);
    void setRight(int width);
    void setTop(int height);
    void setBottom(int height);

    // Low-level access that does not call rationalize():
    int& rleft();
    int& rright();
    int& rtop();
    int& rbottom();

    // Add to specific margins:
    void addLeft(int width);
    void addRight(int width);
    void addTop(int height);
    void addBottom(int height);

    // Return the total size/height/width:
    QSize totalSize() const;
    int totalHeight() const;
    int totalWidth() const;

    // Adds the sum of our left/right margins to "width".
    int addLeftRightMarginsTo(int width) const;

    // Subtracts the sum of our left/right margins from  "width".
    int removeLeftRightMarginsFrom(int width) const;

    // Adds the sum of our top/bottom margins to "width".
    int addTopBottomMarginsTo(int height) const;

    // Subtracts the sum of our top/bottom margins from "width".
    int removeTopBottomMarginsFrom(int height) const;

    // Add our stored margins to a variety of objects:
    QSize addMarginsTo(const QSize& size) const;
    QRect addMarginsTo(const QRect& rect) const;
    void addMarginsToInPlace(QSize& size) const;
    void addMarginsToInPlace(QRect& rect) const;
    void addMarginsToInPlace(Margins& other) const;

    // Adjust a QRect by our top/left margins:
    QRect moveRectByTopLeftMargins(const QRect& rect) const;
    void moveRectByTopLeftMarginsInPlace(QRect& rect) const;

    // Remove our stored margins from a variety of objects:
    QSize removeMarginsFrom(const QSize& size) const;
    QRect removeMarginsFrom(const QRect& rect) const;
    void removeMarginsFromInPlace(QRect& rect) const;

    // Return the getContentsMargins() of a QWidget.
    static Margins getContentsMargins(const QWidget* widget);

    // Return the getContentsMargins() of a QLayout.
    static Margins getContentsMargins(const QLayout* layout);

    // Returns the margins by which "outer" is larger than "inner".
    // The results may be nonsensical if "outer" does not contain "inner".
    static Margins rectDiff(const QRect& outer, const QRect& inner);

    // Here we suppose that "inner" is a rectangle defined relative to (0,0)
    // of a rectangle with size "outer". (Prototypically: a widget with
    // geometry outer has a sub-widget, RELATIVE TO IT, with geometry inner.)
    // Returns the margins that would have to be applied to "inner" to make it
    // reach "outer".
    static Margins subRectMargins(const QSize& outer, const QRect& inner);

    // Returns subRectMargins(outer.size(), inner).
    static Margins subRectMargins(const QRect& outer, const QRect& inner);

private:
    int m_left;  // left margin
    int m_top;  // top margin
    int m_right;  // right margin
    int m_bottom;  // bottom margin

public:
    // Debugging description.
    friend QDebug operator<<(QDebug debug, const Margins& m);
};
