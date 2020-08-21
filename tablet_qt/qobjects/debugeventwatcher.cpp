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

#include "debugeventwatcher.h"
#include <QDebug>
#include <QEvent>
#include <QGestureEvent>
#include <QGraphicsSceneEvent>
#include <QStateMachine>


DebugEventWatcher::DebugEventWatcher(QObject* parent,
                                     const EventCategories categories) :
    QObject(parent),  // owned by parent henceforth
    m_categories(categories)
{
    Q_ASSERT(parent);
    parent->installEventFilter(this);
}


bool DebugEventWatcher::eventFilter(QObject* obj, QEvent* event)
{
    const QEvent::Type type = event->type();
    if (m_categories & EventCategory::All) {
        report(obj, event);
    } else if (m_categories & EventCategory::MouseTouch && (
                   type == QEvent::Enter ||
                   type == QEvent::GrabMouse ||
                   type == QEvent::GraphicsSceneMouseDoubleClick ||
                   type == QEvent::GraphicsSceneMouseMove ||
                   type == QEvent::GraphicsSceneMousePress ||
                   type == QEvent::GraphicsSceneMouseRelease ||
                   type == QEvent::GraphicsSceneWheel ||
                   type == QEvent::HoverEnter ||
                   type == QEvent::HoverLeave ||
                   type == QEvent::HoverMove ||
                   type == QEvent::Leave ||
                   type == QEvent::NonClientAreaMouseButtonDblClick ||
                   type == QEvent::NonClientAreaMouseButtonPress ||
                   type == QEvent::NonClientAreaMouseButtonRelease ||
                   type == QEvent::NonClientAreaMouseMove ||
                   type == QEvent::MouseButtonDblClick ||
                   type == QEvent::MouseButtonPress ||
                   type == QEvent::MouseButtonRelease ||
                   type == QEvent::MouseMove ||
                   type == QEvent::MouseTrackingChange ||
                   type == QEvent::TouchBegin ||
                   type == QEvent::TouchCancel ||
                   type == QEvent::TouchEnd ||
                   type == QEvent::TouchUpdate ||
                   type == QEvent::UngrabMouse ||
                   type == QEvent::Wheel)) {
        report(obj, event);
    }
    return false;  // continue processing the event
}


template<typename EventSubType>
void reportSubtype(QDebug& debug, QEvent* event)
{
    if (auto sub = dynamic_cast<EventSubType*>(event)) {
        debug << ": " << sub;
    }
}


#define REPORT_SUBTYPE(classtype) reportSubtype<classtype>(debug, event)


void DebugEventWatcher::report(QObject* obj, QEvent* event) const
{
    QDebug debug = qDebug().nospace();
    debug << obj->objectName() << ": " << event->type();

    REPORT_SUBTYPE(QActionEvent);
    REPORT_SUBTYPE(QChildEvent);
    REPORT_SUBTYPE(QCloseEvent);
    REPORT_SUBTYPE(QDragLeaveEvent);
    REPORT_SUBTYPE(QDropEvent);
    REPORT_SUBTYPE(QDynamicPropertyChangeEvent);
    REPORT_SUBTYPE(QEnterEvent);
    REPORT_SUBTYPE(QExposeEvent);
    REPORT_SUBTYPE(QFileOpenEvent);
    REPORT_SUBTYPE(QFocusEvent);
    REPORT_SUBTYPE(QGestureEvent);
    REPORT_SUBTYPE(QGraphicsSceneEvent);
    REPORT_SUBTYPE(QHelpEvent);
    REPORT_SUBTYPE(QHideEvent);
    REPORT_SUBTYPE(QIconDragEvent);
    REPORT_SUBTYPE(QInputEvent);
    REPORT_SUBTYPE(QInputMethodEvent);
    REPORT_SUBTYPE(QInputMethodQueryEvent);
    REPORT_SUBTYPE(QMoveEvent);
    REPORT_SUBTYPE(QPaintEvent);
    REPORT_SUBTYPE(QPlatformSurfaceEvent);
    REPORT_SUBTYPE(QResizeEvent);
    REPORT_SUBTYPE(QScrollEvent);
    REPORT_SUBTYPE(QScrollPrepareEvent);
    REPORT_SUBTYPE(QShortcutEvent);
    REPORT_SUBTYPE(QShowEvent);
    REPORT_SUBTYPE(QStateMachine::SignalEvent);
    REPORT_SUBTYPE(QStateMachine::WrappedEvent);
    REPORT_SUBTYPE(QStatusTipEvent);
    REPORT_SUBTYPE(QTimerEvent);
    REPORT_SUBTYPE(QWhatsThisClickedEvent);
    REPORT_SUBTYPE(QWindowStateChangeEvent);
}
