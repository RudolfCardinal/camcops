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

#include <QSizePolicy>
#include <QStyle>
class QFrame;
class QLayout;
class QLabel;
class QPushButton;
class QStyleOptionButton;
class QStyleOptionFrame;
class QWidget;


namespace sizehelpers {

// Size policies that take a few statements to create (i.e. that have
// height-for-width set!).
// Remember: these take the widget's sizeHint() as the reference.

/*

QSizePolicy: https://doc.qt.io/qt-5/qsizepolicy.html

    QSizePolicy::GrowFlag	1	The widget can grow beyond its size hint if necessary.
    QSizePolicy::ExpandFlag	2	The widget should get as much space as possible.
    QSizePolicy::ShrinkFlag	4	The widget can shrink below its size hint if necessary.
    QSizePolicy::IgnoreFlag	8	The widget's size hint is ignored. The widget will get as much space as possible.

So...

Fixed = 0

    "Be exactly size X."

Minimum = GrowFlag

    "Be at least X. I don't need more, but you can give me more."

Maximum = ShrinkFlag

    "Be at most X. You can shrink me if you want."

Preferred = GrowFlag | ShrinkFlag

    "I'd like to be X. You can shrink me if you want. You can enlarge me if
    you want, but I don't need more."

MinimumExpanding = GrowFlag | ExpandFlag

    "Be at least X, but I'd like to be as large as possible."

Expanding = GrowFlag | ShrinkFlag | ExpandFlag

    "X is a reasonable size. You can enlarge or shrink me if you want, but
    I'd like to be as large as possible."

Ignored = ShrinkFlag | GrowFlag | IgnoreFlag

    "Be as large as possible!"

When the "height-for-width" flag is set, meaning "my height depends on my
width", these get more complicated -- see below.

The names of all the functions below have the horizontal policy then the
vertical policy.

*/


// "X is a reasonable horizontal size, but expand me far as possible
// horizontally (and you can shrink me below X if required). Once my vertical
// height Y has been determined from my actual width, it is fixed."
QSizePolicy expandingFixedHFWPolicy();

// "X is a reasonable horizontal size, but expand me far as possible
// horizontally (and you can shrink me below X if required). Once my vertical
// height Y has been determined from my actual width, it is preferred; you
// can enlarge me or shrink me vertically if you want, but I don't need more
// than Y vertically."
QSizePolicy expandingPreferredHFWPolicy();

// "X is a reasonable horizontal size, but expand me far as possible
// horizontally (and you can shrink me below X if required). Once my vertical
// height Y has been determined from my actual width, it is my vertical
// maximum, though you can shrink me vertically if you want."
QSizePolicy expandingMaximumHFWPolicy();

// "X is a reasonable horizontal size, but expand me far as possible
// horizontally (and you can shrink me below X if required). Once my vertical
// height Y has been determined from my actual width, that is a reasonable
// size; you can expand or shrink me vertically, but I would like to be as
// large as possible vertically as well."
QSizePolicy expandingExpandingHFWPolicy();

// "X is my maximum horizontal size; you can shrink me horizontally if you
// want. Once my vertical height Y has been determined from my actual width,
// then it is fixed."
QSizePolicy maximumFixedHFWPolicy();

// "X is my maximum horizontal size; you can shrink me horizontally if you
// want. Once my vertical height Y has been determined from my actual width,
// then it is my vertical maximum; you can shrink me vertically if you want."
QSizePolicy maximumMaximumHFWPolicy();

// "X is my preferred horizontal size; you can shrink me horizontally if you
// want, or expand me, but I don't want more. Once my vertical height Y has
// been determined from my actual width, then it is my preferred height, but
// you can shrink or expand me vertically if you want (but I don't need more)."
QSizePolicy preferredPreferredHFWPolicy();

// "X is my preferred horizontal size; you can shrink me horizontally if you
// want, or expand me, but I don't want more. Once my vertical height Y has
// been determined from my actual width, then it is fixed."
QSizePolicy preferredFixedHFWPolicy();

// The ugly stuff you have to do for widgets that own height-for-width
// widgets if you use Qt layouts like QVBoxLayout.
// This is NOT used within our better HFW framework; see gui_defines.h.
void resizeEventForHFWParentWidget(QWidget* widget);

// More size helper functions, e.g. "how much extra space do I need for
// the stylesheet margins?"
QSize contentsMarginsAsSize(const QWidget* widget);
QSize contentsMarginsAsSize(const QLayout* layout);

// Returns a layout's spacing() as a QSize(2 * spacing(), 2 * spacing()) --
// that is, when you add all the spacing up.
QSize spacingAsSize(const QLayout* layout);

// How much extra space does a QWidget need for its layout margins or its
// stylesheet?
// - widget: the widget to examine
// - opt: "option" parameter passed to QStyle::sizeFromContents()
// - child_size: "contentsSize" parameter passed to QStyle::sizeFromContents()
// - add_style_element: add the size for the style, as well as for the margins
//   of any layout installed on the widget?
// - contents_type: "type" parameter passed to QStyle::sizeFromContents()
QSize widgetExtraSizeForCssOrLayout(const QWidget* widget,
                                    const QStyleOption* opt,
                                    const QSize& child_size,
                                    bool add_style_element,
                                    QStyle::ContentsType contents_type);

// Guess the QStyle::ContentsType applicable to a widget.
// I don't know why this should be necessary...
QStyle::ContentsType guessStyleContentsType(const QWidget* widget);

// Autodetecting version.
QSize widgetExtraSizeForCssOrLayout(const QWidget* widget);

// widgetExtraSizeForCssOrLayout() for QPushButton.
QSize pushButtonExtraSizeRequired(const QPushButton* button,
                                  const QStyleOptionButton* opt,
                                  const QSize& child_size);

// widgetExtraSizeForCssOrLayout() for QFrame.
QSize frameExtraSizeRequired(const QFrame* frame,
                             const QStyleOptionFrame* opt,
                             const QSize& child_size);

// widgetExtraSizeForCssOrLayout() for QLabel.
QSize labelExtraSizeRequired(const QLabel* label,
                             const QStyleOptionFrame* opt,
                             const QSize& child_size);

// Does the widget have a fixed height that is equal to "height"?
bool fixedHeightEquals(QWidget* widget, int height);

// Is a size policy an HFW one that allows vertical shrinkage?
bool canHFWPolicyShrinkVertically(const QSizePolicy& sp);

// Is this an HFW widget that appears to have an area constraint, and trading
// width for height -- e.g. word-wrapped text -- detected as one whose
// preferred height appears (in a very simple test) to get SMALLER as its
// width gets larger?
bool isWidgetHFWTradingDimensions(const QWidget* widget);

// Is this an HFW widget that appears to have an aspect ratio constraint, e.g.
// a resizable widget -- detected as one whose preferred height appears (in a
// very simple test) to get LARGER as its width gets larger?
bool isWidgetHFWMaintainingAspectRatio(const QWidget* widget);


}  // namespace sizehelpers
