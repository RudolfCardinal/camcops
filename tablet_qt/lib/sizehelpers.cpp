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

// #define DEBUG_HFW_RESIZE_EVENT
// #define DEBUG_WIDGET_MARGINS

#include "sizehelpers.h"
#include <QAbstractItemDelegate>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QGroupBox>
#include <QHeaderView>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSizeGrip>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionFrame>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QWidget>

#ifdef DEBUG_WIDGET_MARGINS
#include "lib/layoutdumper.h"
#endif

#define QT_FREQUENT_STARTING_WIDTH 640


namespace sizehelpers {

// REMINDERS:
//
// - QSizePolicy(horizontal_policy, vertical_policy)
//
// http://doc.qt.io/qt-5/qsizepolicy.html#Policy-enum:
//
// - Fixed: fixed size, from sizeHint().
// - Minimum: sizeHint() or greater; will grow to available space, but no
//   advantage to expanding.
// - Maximum: sizeHint() or smaller; will shrink as much as required.
// - Preferred: ideally sizeHint(), but can grow or shrink as required; no
//   advantage to growing.
// - MinimumExpanding: sizeHint() or larger, and should get as much space
//   as possible.
// - Expanding: sizeHint() is sensible, but can be smaller or larger, and
//   should get as much space as possible.
// - Ignored: give it as much space as possible; ignore sizeHint().


QSizePolicy expandingFixedHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sp.setHeightForWidth(true);
    return sp;
}


QSizePolicy expandingPreferredHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    return sp;
}


QSizePolicy maximumFixedHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Maximum, QSizePolicy::Fixed);
    sp.setHeightForWidth(true);
    return sp;
}


QSizePolicy expandingMaximumHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Maximum);
    sp.setHeightForWidth(true);
    return sp;
}


QSizePolicy expandingExpandingHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sp.setHeightForWidth(true);
    return sp;
}


QSizePolicy maximumMaximumHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Maximum, QSizePolicy::Maximum);
    sp.setHeightForWidth(true);
    return sp;
}


QSizePolicy preferredPreferredHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    return sp;
}


QSizePolicy preferredFixedHFWPolicy()
{
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sp.setHeightForWidth(true);
    return sp;
}


void resizeEventForHFWParentWidget(QWidget* widget)
{
    // Call from your resizeEvent() processor passing "this" as the parameter
    // if you are a widget that contains (via a layout) height-for-width
    // widgets.
    Q_ASSERT(widget);
    QLayout* lay = widget->layout();
    if (!lay || !lay->hasHeightForWidth()) {
        return;
    }
    const int w = widget->width();
    const int h = lay->heightForWidth(w);
#ifdef DEBUG_HFW_RESIZE_EVENT
    qDebug() << Q_FUNC_INFO << "w" << w << "-> h" << h;
#endif
    const bool change = !fixedHeightEquals(widget, h);
    if (change) {
        widget->setFixedHeight(h);
        widget->updateGeometry();
    }
}


QSize contentsMarginsAsSize(const QWidget* widget)
{
    Q_ASSERT(widget);
    const QMargins margins = widget->contentsMargins();
    return QSize(margins.left() + margins.right(),
                 margins.top() + margins.bottom());
}


QSize contentsMarginsAsSize(const QLayout* layout)
{
    Q_ASSERT(layout);
    const QMargins margins = layout->contentsMargins();
    return QSize(margins.left() + margins.right(),
                 margins.top() + margins.bottom());
}


QSize spacingAsSize(const QLayout* layout)
{
    Q_ASSERT(layout);
    const int spacing = layout->spacing();
    return QSize(2 * spacing, 2 * spacing);
}


QSize widgetExtraSizeForCssOrLayout(const QWidget* widget,
                                    const QStyleOption* opt,
                                    const QSize& child_size,
                                    const bool add_style_element,
                                    const QStyle::ContentsType contents_type)
{
    // See QPushButton::sizeHint()
    Q_ASSERT(widget);
    Q_ASSERT(opt);

    QSize stylesheet_extra_size(0, 0);
    if (add_style_element) {
        QStyle* style = widget->style();
        if (style) {
            const QSize temp = style->sizeFromContents(contents_type, opt,
                                                       child_size, widget);
            stylesheet_extra_size = temp - child_size;
        }
    }

    QSize extra_for_layout_margins(0, 0);
    QLayout* layout = widget->layout();  // ... the layout manager installed on
        // THIS widget (i.e. if this widget has children), not the layout
        // to which it belongs.
    if (layout) {
        extra_for_layout_margins = contentsMarginsAsSize(layout);
    }
    // I think that if you have a style, that sets the layout margins
    // and so adding the layout margins *as well* makes the widget too big
    // (by double-counting). However, if there's no style, then this is
    // important.
    // Hmpf. No. Doing one or the other improves some things and breaks others!
    // Specifically, QuBoolean in text mode got better (no longer too big)
    // and QuBoolean in image mode with associated text got worse (too small).
    // Both forms of text are ClickableLabelWordWrapWide.

    // size_hint += stylesheet_extra_size + extra_for_layout_margins;

    // Take the maximum?
    const QSize total_extra = stylesheet_extra_size
            .expandedTo(extra_for_layout_margins)
            .expandedTo(QSize(0, 0));  // just to ensure it's never negative

#ifdef DEBUG_WIDGET_MARGINS
    qDebug().nospace() << Q_FUNC_INFO
             << "widget " << layoutdumper::getWidgetDescriptor(widget)
             << "; child_size " << child_size
             << "; stylesheet_extra_size " << stylesheet_extra_size
             << "; extra_for_layout_margins " << extra_for_layout_margins
             << " => total_extra " << total_extra;
#endif
    return total_extra;
}


QStyle::ContentsType guessStyleContentsType(const QWidget* widget)
{
    if (dynamic_cast<const QCheckBox*>(widget)) {
        return QStyle::CT_CheckBox;
    }
    if (dynamic_cast<const QComboBox*>(widget)) {
        return QStyle::CT_ComboBox;
    }
    if (dynamic_cast<const QHeaderView*>(widget)) {
        return QStyle::CT_HeaderSection;
    }
    if (dynamic_cast<const QLineEdit*>(widget)) {
        return QStyle::CT_LineEdit;
    }
    if (dynamic_cast<const QMenu*>(widget)) {
        return QStyle::CT_Menu;
    }
    if (dynamic_cast<const QMenuBar*>(widget)) {
        return QStyle::CT_MenuBar;
    }
    // No direct widget corresponding to QStyle::CT_MenuBarItem?
    if (dynamic_cast<const QProgressBar*>(widget)) {
        return QStyle::CT_ProgressBar;
    }
    if (dynamic_cast<const QPushButton*>(widget)) {
        return QStyle::CT_PushButton;
    }
    if (dynamic_cast<const QRadioButton*>(widget)) {
        return QStyle::CT_RadioButton;
    }
    if (dynamic_cast<const QSizeGrip*>(widget)) {
        return QStyle::CT_SizeGrip;
    }
    if (dynamic_cast<const QSlider*>(widget)) {
        return QStyle::CT_Slider;
    }
    if (dynamic_cast<const QScrollBar*>(widget)) {
        return QStyle::CT_ScrollBar;
    }
    if (dynamic_cast<const QSpinBox*>(widget)) {
        return QStyle::CT_SpinBox;
    }
    if (dynamic_cast<const QSplitter*>(widget)) {
        return QStyle::CT_Splitter;
    }
    if (dynamic_cast<const QTabBar*>(widget)) {
        return QStyle::CT_TabBarTab;
    }
    if (dynamic_cast<const QTabWidget*>(widget)) {
        return QStyle::CT_TabWidget;
    }
    if (dynamic_cast<const QToolButton*>(widget)) {
        return QStyle::CT_ToolButton;
    }
    if (dynamic_cast<const QGroupBox*>(widget)) {
        return QStyle::CT_GroupBox;
    }
    if (dynamic_cast<const QMdiSubWindow*>(widget)) {
        return QStyle::CT_MdiControls;
    }
    // No direct widget corresponding to QStyle::CT_ItemViewItem?

    // Default??
    return QStyle::CT_CustomBase;
}


QSize widgetExtraSizeForCssOrLayout(const QWidget* widget)
{
    QStyleOption opt;
    opt.initFrom(widget);
    const QSize child_size = widget->sizeHint();
    const bool add_style_element = true;
    const QStyle::ContentsType contents_type = guessStyleContentsType(widget);
    return widgetExtraSizeForCssOrLayout(widget, &opt, child_size,
                                         add_style_element, contents_type);
}


/*
QSize sizehelpers::widgetExtraSizeForCss(
        const QWidget* widget,
        const QStyleOption* opt,
        const QSize& child_size,
        bool add_style_element,
        QStyle::ContentsType contents_type)
{
    // As above, but NOT including layout margins.
    Q_ASSERT(widget);
    Q_ASSERT(opt);

    QSize stylesheet_extra_size(0, 0);
    if (add_style_element) {
        QStyle* style = widget->style();
        if (style) {
            QSize temp = style->sizeFromContents(contents_type, opt,
                                                 child_size, widget);
            stylesheet_extra_size = temp - child_size;
        }
    }

    QSize total_extra = stylesheet_extra_size
            .expandedTo(QSize(0, 0));  // just to ensure it's never negative

#ifdef DEBUG_WIDGET_MARGINS
    qDebug().nospace() << Q_FUNC_INFO
             << "widget " << LayoutDumper::getWidgetDescriptor(widget)
             << "; child_size " << child_size
             << "; stylesheet_extra_size " << stylesheet_extra_size
             << " => total_extra " << total_extra;
#endif
    return total_extra;
}
*/


QSize pushButtonExtraSizeRequired(const QPushButton* button,
                                  const QStyleOptionButton* opt,
                                  const QSize& child_size)
{
    return widgetExtraSizeForCssOrLayout(button, opt, child_size,
                                         true, QStyle::CT_PushButton);
}


QSize frameExtraSizeRequired(const QFrame* frame,
                             const QStyleOptionFrame* opt,
                             const QSize& child_size)
{
    return widgetExtraSizeForCssOrLayout(frame, opt, child_size,
                                         false, QStyle::CT_PushButton);
    // Is QStyle::CT_PushButton right?
}


QSize labelExtraSizeRequired(const QLabel* label,
                             const QStyleOptionFrame* opt,
                             const QSize& child_size)
{
    QSize size = widgetExtraSizeForCssOrLayout(label, opt, child_size,
                                               true, QStyle::CT_PushButton);
    // Is QStyle::CT_PushButton right?
    // Or QStyle::CT_ItemViewItem?

    // 2019-07-06: problem with a LabelWordWrapWide in in e.g. QuMcqGrid.
    // This function was returning too little; the result was inappropriate
    // word wrapping.
    // Example was a margin (marked as belonging to the QLabel when green
    // turned on in the CSS) of about 9 (perhaps 10) pixels each side, but
    // this function was returning QSize(10, 10).

    size.rwidth() *= 2;  // HELPS, BUT NOT ENTIRELY RATIONAL

    return size;
}


bool fixedHeightEquals(QWidget* widget, const int height)
{
    return height == widget->minimumHeight() &&
            height == widget->maximumHeight();
}


bool canHFWPolicyShrinkVertically(const QSizePolicy& sp)
{
    if (!sp.hasHeightForWidth()) {
        return false;
    }
    const QSizePolicy::Policy vp = sp.verticalPolicy();
    const bool can_shrink_vertically = vp & QSizePolicy::ShrinkFlag;
    return can_shrink_vertically;
}


bool isWidgetHFWTradingDimensions(const QWidget* widget)
{
    if (!widget->hasHeightForWidth()) {
        return false;
    }
    const int h_for_small_w = widget->heightForWidth(QT_FREQUENT_STARTING_WIDTH);
    const int h_for_big_w = widget->heightForWidth(QT_FREQUENT_STARTING_WIDTH * 2);
    return h_for_small_w > h_for_big_w;
}


bool isWidgetHFWMaintainingAspectRatio(const QWidget* widget)
{
    if (!widget->hasHeightForWidth()) {
        return false;
    }
    const int h_for_small_w = widget->heightForWidth(QT_FREQUENT_STARTING_WIDTH);
    const int h_for_big_w = widget->heightForWidth(QT_FREQUENT_STARTING_WIDTH * 2);
    return h_for_small_w < h_for_big_w;
}


}  // namespace sizehelpers
