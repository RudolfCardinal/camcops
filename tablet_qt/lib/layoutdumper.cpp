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

#include "layoutdumper.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <QDebug>
#include <QScrollArea>
#include <QString>
#include <QStringBuilder>
#include <QtWidgets/QLayout>
#include <QtWidgets/QWidget>
#include "lib/convert.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"

namespace layoutdumper {

const QString NULL_WIDGET_STRING("<null_widget>");


QString toString(const QSizePolicy::Policy& policy)
{
    switch (policy) {
    case QSizePolicy::Fixed: return "Fixed";
    case QSizePolicy::Minimum: return "Minimum";
    case QSizePolicy::Maximum: return "Maximum";
    case QSizePolicy::Preferred: return "Preferred";
    case QSizePolicy::MinimumExpanding: return "MinimumExpanding";
    case QSizePolicy::Expanding: return "Expanding";
    case QSizePolicy::Ignored: return "Ignored";
    }
    return "unknown_QSizePolicy";
}


QString toString(const QSizePolicy& policy)
{
    QString result = QString("(%1, %2) [hasHeightForWidth=%3]")
            .arg(toString(policy.horizontalPolicy()),
                 toString(policy.verticalPolicy()),
                 toString(policy.hasHeightForWidth()));
    return result;
}


QString toString(const QLayout::SizeConstraint constraint)
{
    switch (constraint) {
    case QLayout::SetDefaultConstraint: return "SetDefaultConstraint";
    case QLayout::SetNoConstraint: return "SetNoConstraint";
    case QLayout::SetMinimumSize: return "SetMinimumSize";
    case QLayout::SetFixedSize: return "SetFixedSize";
    case QLayout::SetMaximumSize: return "SetMaximumSize";
    case QLayout::SetMinAndMaxSize: return "SetMinAndMaxSize";
    }
    return "unknown_SizeConstraint";
}


QString toString(const Qt::Alignment& alignment)
{
    QStringList elements;

    if (alignment & Qt::AlignLeft) {
        elements.append("AlignLeft");
    }
    if (alignment & Qt::AlignRight) {
        elements.append("AlignRight");
    }
    if (alignment & Qt::AlignHCenter) {
        elements.append("AlignHCenter");
    }
    if (alignment & Qt::AlignJustify) {
        elements.append("AlignJustify");
    }
    if (alignment & Qt::AlignAbsolute) {
        elements.append("AlignAbsolute");
    }
    if ((alignment & Qt::AlignHorizontal_Mask) == 0) {
        elements.append("<horizontal_none>");
    }

    if (alignment & Qt::AlignTop) {
        elements.append("AlignTop");
    }
    if (alignment & Qt::AlignBottom) {
        elements.append("AlignBottom");
    }
    if (alignment & Qt::AlignVCenter) {
        elements.append("AlignVCenter");
    }
    if (alignment & Qt::AlignBaseline) {
        elements.append("AlignBaseline");
    }
    if ((alignment & Qt::AlignVertical_Mask) == 0) {
        elements.append("<vertical_none>");
    }

    return elements.join(" | ");
}


QString toString(const void* pointer)
{
    return convert::prettyPointer(pointer);
}


QString toString(const bool boolean)
{
    return boolean ? "true" : "false";
}


QString getWidgetDescriptor(const QWidget* w)
{
    if (!w) {
        return NULL_WIDGET_STRING;
    }
    return QString("%1<%2 '%3'>")
            .arg(w->metaObject()->className(),
                 toString(reinterpret_cast<const void*>(w)),
                 w->objectName());
}


QString getWidgetInfo(const QWidget* w, const DumperConfig& config)
{
    if (!w) {
        return NULL_WIDGET_STRING;
    }

    const QRect& geom = w->geometry();

    // Can't have >9 arguments to QString arg() system.
    // Using QStringBuilder with % leads to more type faff.
    QStringList elements;
    elements.append(getWidgetDescriptor(w));
    elements.append(w->isVisible() ? "visible" : "HIDDEN");
    elements.append(QString("pos[DOWN] (%1, %2)")
                    .arg(geom.x())
                    .arg(geom.y()));
    elements.append(QString("size[DOWN] (%1 x %2)")
                    .arg(geom.width())
                    .arg(geom.height()));
    elements.append(QString("hasHeightForWidth()[UP] %1")
                    .arg(w->hasHeightForWidth() ? "true" : "false"));
    elements.append(QString("heightForWidth(%1[DOWN])[UP] %2")
                    .arg(geom.width())
                    .arg(w->heightForWidth(geom.width())));
    elements.append(QString("minimumSize (%1 x %2)")
                    .arg(w->minimumSize().width())
                    .arg(w->minimumSize().height()));
    elements.append(QString("maximumSize (%1 x %2)")
                    .arg(w->maximumSize().width())
                    .arg(w->maximumSize().height()));
    elements.append(QString("sizeHint[UP] (%1 x %2)")
                    .arg(w->sizeHint().width())
                    .arg(w->sizeHint().height()));
    elements.append(QString("minimumSizeHint[UP] (%1 x %2)")
                    .arg(w->minimumSizeHint().width())
                    .arg(w->minimumSizeHint().height()));
    elements.append(QString("sizePolicy[UP] %1")
                    .arg(toString(w->sizePolicy())));
    elements.append(QString("stylesheet: %1")
                    .arg(w->styleSheet().isEmpty() ? "false" : "true"));

    if (config.show_all_widget_attributes ||
            config.show_set_widget_attributes) {
        elements.append(QString("attributes: [%1]")
                        .arg(getWidgetAttributeInfo(
                                 w, config.show_all_widget_attributes)));
    }

    if (config.show_widget_properties) {
        const QString properties = getDynamicProperties(w);
        if (!properties.isEmpty()) {
            elements.append(QString("properties: [%1]").arg(properties));
        }
    }

    if (config.show_widget_stylesheets) {
        elements.append(QString("stylesheet contents: %1").arg(
                            convert::stringToCppLiteral(w->styleSheet())));
    }

    // Geometry within bounds?
    if (geom.width() < w->minimumSize().width()) {
        elements.append("[BUG? geometry().width() < minimumSize().width()]");
    }
    if (geom.height() < w->minimumSize().height()) {
        elements.append("[BUG? geometry().height() < minimumSize().height()]");
    }
    if (geom.width() < w->minimumSizeHint().width()) {
        elements.append("[WARNING: geometry().width() < "
                        "minimumSizeHint().width()]");
    }
    if (!w->hasHeightForWidth() &&
            geom.height() < w->minimumSizeHint().height()) {
        elements.append("[WARNING: geometry().height() < "
                        "minimumSizeHint().height()]");
    }
    if (geom.width() > w->maximumSize().width()) {
        elements.append("[BUG? geometry().width() > maximumSize().width()]");
    }
    if (geom.height() > w->maximumSize().height()) {
        elements.append("[BUG? geometry().height() > maximumSize().height()]");
    }
    if (w->hasHeightForWidth() &&
            geom.height() < w->heightForWidth(geom.width())) {
        const bool can_shrink_vertically =
                sizehelpers::canHFWPolicyShrinkVertically(w->sizePolicy());
        if (!can_shrink_vertically) {
            elements.append(
                "[WARNING: geometry().height() < "
                "heightForWidth(geometry().width()) and policy doesn't allow "
                "vertical shrinkage]");
        }
    }

    // Bounds themselves consistent?
    if (w->sizeHint().width() != -1 && w->sizeHint().height() != -1) {
        if (w->sizeHint().width() < w->minimumSizeHint().width()) {
            elements.append("[WIDGET BUG? sizeHint().width() < "
                            "minimumSizeHint().width()]");
        }
        /*
        // Not clear that these are wrong; the layout may have other reasons
        // for setting our minimumSize() bigger than our sizeHint().
        if (w->sizeHint().width() < w->minimumSize().width()) {
            elements.append("[BUG? sizeHint().width() < "
                            "minimumSize().width()]");
        }
        if (w->sizeHint().width() > w->maximumSize().width()) {
            elements.append("[BUG? sizeHint().width() > "
                            "maximumSize().width()]");
        }
        */
        if (w->sizeHint().height() < w->minimumSizeHint().height()) {
            elements.append("[WIDGET BUG? sizeHint().height() < "
                            "minimumSizeHint().height()]");
        }
        if (!w->hasHeightForWidth()) {
            /*
            if (w->sizeHint().height() < w->minimumSize().height()) {
                elements.append("[BUG? sizeHint().height() < "
                                "minimumSize().height()]");
            }
            if (w->sizeHint().height() > w->maximumSize().height()) {
                elements.append("[BUG? sizeHint().height() > "
                                "maximumSize().height()]");
            }
            */
        }
    }

    return elements.join(", ");
}


#define ADD_WIDGET_ATTR(x) add(Qt::x, #x)


QString getWidgetAttributeInfo(const QWidget* w, const bool all)
{
    // http://doc.qt.io/qt-5/qt.html#WidgetAttribute-enum
    if (!w) {
        return NULL_WIDGET_STRING;
    }
    QStringList elements;
    auto add = [&all, &w, &elements](Qt::WidgetAttribute attr,
            const QString& desc) {
        const bool set = w->testAttribute(attr);
        if (all) {
            elements.append(QString("%1 %2").arg(desc, set));
        } else {
            if (set) {
                elements.append(desc);
            }
        }
    };

    // http://doc.qt.io/qt-5/qt.html#WidgetAttribute-enum
    // ... sorted

    ADD_WIDGET_ATTR(WA_AcceptTouchEvents);
    ADD_WIDGET_ATTR(WA_AlwaysShowToolTips);
    ADD_WIDGET_ATTR(WA_AlwaysStackOnTop);
    ADD_WIDGET_ATTR(WA_ContentsPropagated);
    ADD_WIDGET_ATTR(WA_CustomWhatsThis);
    ADD_WIDGET_ATTR(WA_DeleteOnClose);
    ADD_WIDGET_ATTR(WA_Disabled);
    ADD_WIDGET_ATTR(WA_DontCreateNativeAncestors);
    ADD_WIDGET_ATTR(WA_DontShowOnScreen);
    ADD_WIDGET_ATTR(WA_ForceDisabled);
    ADD_WIDGET_ATTR(WA_ForceUpdatesDisabled);
    ADD_WIDGET_ATTR(WA_GroupLeader);
    ADD_WIDGET_ATTR(WA_Hover);
    ADD_WIDGET_ATTR(WA_InputMethodEnabled);
    ADD_WIDGET_ATTR(WA_KeyboardFocusChange);
    ADD_WIDGET_ATTR(WA_KeyCompression);
    ADD_WIDGET_ATTR(WA_LayoutOnEntireRect);
    ADD_WIDGET_ATTR(WA_LayoutUsesWidgetRect);
    ADD_WIDGET_ATTR(WA_MacAlwaysShowToolWindow);
    ADD_WIDGET_ATTR(WA_MacBrushedMetal);
    ADD_WIDGET_ATTR(WA_MacFrameworkScaled);
    ADD_WIDGET_ATTR(WA_MacMiniSize);
    ADD_WIDGET_ATTR(WA_MacNoClickThrough);
    ADD_WIDGET_ATTR(WA_MacNormalSize);
    ADD_WIDGET_ATTR(WA_MacOpaqueSizeGrip);
    ADD_WIDGET_ATTR(WA_MacShowFocusRect);
    ADD_WIDGET_ATTR(WA_MacSmallSize);
    ADD_WIDGET_ATTR(WA_MacVariableSize);
    ADD_WIDGET_ATTR(WA_Mapped);
    ADD_WIDGET_ATTR(WA_MouseNoMask);
    ADD_WIDGET_ATTR(WA_MouseTracking);
    ADD_WIDGET_ATTR(WA_Moved);
    ADD_WIDGET_ATTR(WA_MSWindowsUseDirect3D);
    ADD_WIDGET_ATTR(WA_NativeWindow);
    ADD_WIDGET_ATTR(WA_NoBackground);
    ADD_WIDGET_ATTR(WA_NoChildEventsForParent);
    ADD_WIDGET_ATTR(WA_NoChildEventsFromChildren);
    ADD_WIDGET_ATTR(WA_NoMousePropagation);
    ADD_WIDGET_ATTR(WA_NoMouseReplay);
    ADD_WIDGET_ATTR(WA_NoSystemBackground);
    ADD_WIDGET_ATTR(WA_OpaquePaintEvent);
    ADD_WIDGET_ATTR(WA_OutsideWSRange);
    ADD_WIDGET_ATTR(WA_PaintOnScreen);
    ADD_WIDGET_ATTR(WA_PaintUnclipped);
    ADD_WIDGET_ATTR(WA_PendingMoveEvent);
    ADD_WIDGET_ATTR(WA_PendingResizeEvent);
    ADD_WIDGET_ATTR(WA_QuitOnClose);
    ADD_WIDGET_ATTR(WA_Resized);
    ADD_WIDGET_ATTR(WA_RightToLeft);
    ADD_WIDGET_ATTR(WA_SetCursor);
    ADD_WIDGET_ATTR(WA_SetFont);
    ADD_WIDGET_ATTR(WA_SetLocale);
    ADD_WIDGET_ATTR(WA_SetPalette);
    ADD_WIDGET_ATTR(WA_SetStyle);
    ADD_WIDGET_ATTR(WA_ShowModal);
    ADD_WIDGET_ATTR(WA_ShowWithoutActivating);
    ADD_WIDGET_ATTR(WA_StaticContents);
    ADD_WIDGET_ATTR(WA_StyledBackground);
    ADD_WIDGET_ATTR(WA_StyleSheet);
    // ADD_WIDGET_ATTR(WA_TabletTracking);  // too new
    ADD_WIDGET_ATTR(WA_TouchPadAcceptSingleTouchEvents);
    ADD_WIDGET_ATTR(WA_TranslucentBackground);
    ADD_WIDGET_ATTR(WA_TransparentForMouseEvents);
    ADD_WIDGET_ATTR(WA_UnderMouse);
    ADD_WIDGET_ATTR(WA_UpdatesDisabled);
    ADD_WIDGET_ATTR(WA_WindowModified);
    ADD_WIDGET_ATTR(WA_WindowPropagation);
    ADD_WIDGET_ATTR(WA_X11DoNotAcceptFocus);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeCombo);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeDesktop);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeDialog);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeDND);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeDock);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeDropDownMenu);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeMenu);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeNotification);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypePopupMenu);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeSplash);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeToolBar);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeToolTip);
    ADD_WIDGET_ATTR(WA_X11NetWmWindowTypeUtility);

    return elements.join(", ");
}


QString getDynamicProperties(const QWidget* w)
{
    if (!w) {
        return NULL_WIDGET_STRING;
    }
    QStringList elements;
    const QList<QByteArray> property_names = w->dynamicPropertyNames();
    for (const QByteArray& arr : property_names) {
        const QString name(arr);
        const QVariant value = w->property(arr);
        const QString value_string = uifunc::escapeString(value.toString());
        elements.append(QString("%1=%2").arg(name, value_string));
    }
    return elements.join(", ");
}


QString getLayoutInfo(const QLayout* layout)
{
    if (!layout) {
        return "null_layout";
    }
    const QMargins margins = layout->contentsMargins();
    const QSize sizehint = layout->sizeHint();
    const QSize minsize = layout->minimumSize();
    const QSize maxsize = layout->maximumSize();
    const QString name = layout->metaObject()->className();
    QWidget* parent = layout->parentWidget();
    // usually unhelpful (blank): layout->objectName()
    QStringList elements;
    elements.append(name);
    elements.append(QString("constraint %1")
                    .arg(toString(layout->sizeConstraint())));
    elements.append(QString("minimumSize[UP] (%1 x %2)")
                    .arg(minsize.width())
                    .arg(minsize.height()));
    elements.append(QString("sizeHint[UP] (%1 x %2)")
                    .arg(sizehint.width())
                    .arg(sizehint.height()));
    elements.append(QString("maximumSize[UP] (%1 x %2)")
                    .arg(maxsize.width())
                    .arg(maxsize.height()));
    elements.append(QString("hasHeightForWidth[UP] %3")
                    .arg(toString(layout->hasHeightForWidth())));
    elements.append(QString("margin (l=%1,t=%2,r=%3,b=%4)")
                    .arg(margins.left())
                    .arg(margins.top())
                    .arg(margins.right())
                    .arg(margins.bottom()));
    elements.append(QString("spacing[UP] %1")
                    .arg(layout->spacing()));

    // Check hints are consistent
    if (sizehint.width() < minsize.width()) {
        elements.append("[BUG? sizeHint().width() < minimumSize().width()]");
    }
    if (sizehint.height() < minsize.height()) {
        elements.append("[BUG? sizeHint().height() < minimumSize().height()]");
    }
    if (sizehint.width() > maxsize.width()) {
        elements.append("[BUG? sizeHint().width() > maximumSize().width()]");
    }
    if (sizehint.height() > maxsize.height()) {
        elements.append("[BUG? sizeHint().height() > maximumSize().height()]");
    }

    // Check parent size is appropriate
    if (parent) {
        const QSize parent_size = parent->size();
        const int parent_width = parent_size.width();
        elements.append(QString("heightForWidth(%1[parent_width])[UP] %2")
                        .arg(parent_width)
                        .arg(layout->heightForWidth(parent_width)));
        elements.append(QString("minimumHeightForWidth(%1[parent_width])[UP] %2")
                        .arg(parent_width)
                        .arg(layout->minimumHeightForWidth(parent_width)));
        if (parent_width < minsize.width()) {
            elements.append("[WARNING: parent->size().width() < "
                            "minimumSize().width()]");
        }
        if (parent_size.height() < minsize.height()) {
            elements.append("[WARNING: parent->size().height() < "
                            "minimumSize().height()]");
        }
    }
    return elements.join(", ");
}


QString getSpacerInfo(QSpacerItem* si)
{
    const QRect& geom = si->geometry();
    const QSize si_hint = si->sizeHint();
    const QLayout* si_layout = si->layout();
    QStringList elements;
    elements.append("QSpacerItem");
    elements.append(QString("pos[DOWN] (%1, %2)")
                    .arg(geom.x())
                    .arg(geom.y()));
    elements.append(QString("size[DOWN] (%1 x %2)")
                    .arg(geom.width())
                    .arg(geom.height()));
    elements.append(QString("sizeHint (%1 x %2)")
                    .arg(si_hint.width())
                    .arg(si_hint.height()));
    elements.append(QString("sizePolicy %1")
                    .arg(toString(si->sizePolicy())));
    elements.append(QString("constraint %1 [alignment %2]")
                    .arg(si_layout ? toString(si_layout->sizeConstraint())
                                   : "<no_layout>",
                         toString(si->alignment())));
    return elements.join(", ");
}


QString paddingSpaces(const int level, const int spaces_per_level)
{
    return QString(level * spaces_per_level, ' ');
}


QVector<const QWidget*> dumpLayoutAndChildren(QDebug& os,
                                              const QLayout* layout,
                                              const int level,
                                              const DumperConfig& config)
{
    const QString padding = paddingSpaces(level, config.spaces_per_level);
    const QString next_padding = paddingSpaces(level + 1, config.spaces_per_level);
    QVector<const QWidget*> dumped_children;

    os << padding << "Layout: " << getLayoutInfo(layout);

    auto box_layout = dynamic_cast<const QBoxLayout*>(layout);
    if (box_layout) {
        os << ", spacing " <<  box_layout->spacing();
    }
    os << "\n";

    if (layout->isEmpty()) {
        os << padding << "... empty layout\n";
    } else {
        const int num_items = layout->count();
        for (int i = 0; i < num_items; i++) {
            QLayoutItem* layout_item = layout->itemAt(i);
            QLayout* child_layout = layout_item->layout();
            auto wi = dynamic_cast<QWidgetItem*>(layout_item);
            auto si = dynamic_cast<QSpacerItem*>(layout_item);
            if (wi && wi->widget()) {
                QString alignment = QString(" [alignment from layout: %1]")
                        .arg(toString(wi->alignment()));
                dumped_children.append(
                    dumpWidgetAndChildren(os, wi->widget(), level + 1,
                                          alignment, config));
            } else if (child_layout) {
                dumped_children.append(
                    dumpLayoutAndChildren(os, child_layout, level + 1,
                                          config));
            } else if (si) {
                os << next_padding << getSpacerInfo(si) << "\n";
            } else {
                os << next_padding << "<unknown_QLayoutItem>\n";
            }
        }
    }
    return dumped_children;
}


QVector<const QWidget*> dumpWidgetAndChildren(QDebug& os,
                                              const QWidget* w,
                                              const int level,
                                              const QString& alignment,
                                              const DumperConfig& config)
{
    const QString padding = paddingSpaces(level, config.spaces_per_level);

    os << padding
       << getWidgetInfo(w, config)
       << alignment << "\n";

    QVector<const QWidget*> dumped_children;
    dumped_children.append(w);

    QLayout* layout = w->layout();
    if (layout) {
        dumped_children.append(
            dumpLayoutAndChildren(os, layout, level + 1, config));
    }

    // Scroll areas contain but aren't necessarily the parents of their widgets
    // However, they contain a 'qt_scrollarea_viewport' widget that is.
    auto scroll = dynamic_cast<const QScrollArea*>(w);
    if (scroll) {
        dumped_children.append(
            dumpWidgetAndChildren(os, scroll->viewport(), level + 1, "",
                                  config));
    }

    // now output any child widgets that weren't dumped as part of the layout
    QList<QWidget*> widgets = w->findChildren<QWidget*>(
                QString(), Qt::FindDirectChildrenOnly);
    // Search options: FindDirectChildrenOnly or FindChildrenRecursively.
    QVector<QWidget*> undumped_children;
    foreach (QWidget* child, widgets) {
        if (!dumped_children.contains(child)) {
            undumped_children.push_back(child);
        }
    }
    if (!undumped_children.empty()) {
        os << padding << "... Non-layout children of "
           << getWidgetDescriptor(w) << ":\n";
        foreach (QWidget* child, undumped_children) {
            dumped_children.append(
                dumpWidgetAndChildren(os, child, level + 1, "", config));
        }
    }
    return dumped_children;
}


void dumpWidgetHierarchy(const QWidget* w, const DumperConfig& config)
{
    QDebug os = qDebug().noquote().nospace();
    os << "WIDGET HIERARCHY:\n";
    if (config.use_ultimate_parent) {
        w = ultimateParentWidget(w);
    }
    dumpWidgetAndChildren(os, w, 0, "", config);
}


const QWidget* ultimateParentWidget(const QWidget* w)
{
    while (const QWidget* parent = w->parentWidget()) {
        w = parent;
    }
    return w;
}


}  // namespace layoutdumper
