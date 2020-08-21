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

// #define DEBUG_OPENABLE_WIDGET_LAYOUT  // Dumps layout when widget shown
// #define DEBUG_PRESS_D_TO_DUMP
// #define DEBUG_RESIZE

#include "openablewidget.h"
#include <QGraphicsView>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "layouts/qtlayouthelpers.h"
#include "lib/layoutdumper.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "qobjects/showwatcher.h"


OpenableWidget::OpenableWidget(QWidget* parent) :
    QWidget(parent),
    m_subwidget(nullptr),
    m_wants_fullscreen(false),
    m_escape_key_can_abort(true),
    m_escape_aborts_without_confirmation(false)
{
#ifdef DEBUG_OPENABLE_WIDGET_LAYOUT
    auto showwatcher = new ShowWatcher(this, true);
    Q_UNUSED(showwatcher)
#endif
}


void OpenableWidget::build()
{
    QWidget* widget = m_subwidget.data();
    auto openable_widget = dynamic_cast<OpenableWidget*>(widget);
    if (openable_widget) {
        openable_widget->build();
    }
}


bool OpenableWidget::wantsFullscreen() const
{
    return m_wants_fullscreen;
}


void OpenableWidget::setWantsFullscreen(const bool fullscreen)
{
    m_wants_fullscreen = fullscreen;
}


void OpenableWidget::setWidgetAsOnlyContents(QWidget* widget,
                                             const int margin,
                                             const bool fullscreen,
                                             const bool esc_can_abort)
{
    // Remove any existing layout
    m_subwidget = nullptr;
    QLayout* l = layout();
    delete l;
    // https://stackoverflow.com/questions/7528680/how-to-delete-an-already-existing-layout-on-a-widget
    // https://stackoverflow.com/questions/6731331/is-it-still-safe-to-delete-nullptr-in-c0x

    auto vl = new QVBoxLayout(this);
    setLayout(vl);
    vl->setMargin(margin);
    vl->addWidget(widget);

    m_subwidget = widget;

    // Full screen?
    setWantsFullscreen(fullscreen);  // in case we're not shown yet
    emit (fullscreen ? enterFullscreen() : leaveFullscreen());  // in case we're already showing

    // Escape key behaviour?
    // - Note that one reason not to have multiple widgets active but not
    //   shown is performance; another is defining which grabs the Escape key
    //   (or other keys); a third is connecting up all the signals correctly.
    setEscapeKeyCanAbort(esc_can_abort, false);
}


bool OpenableWidget::escapeKeyCanAbort() const
{
    return m_escape_key_can_abort;
}


void OpenableWidget::setEscapeKeyCanAbort(const bool esc_can_abort,
                                          const bool without_confirmation)
{
    m_escape_key_can_abort = esc_can_abort;
    m_escape_aborts_without_confirmation = without_confirmation;
}


void OpenableWidget::resizeEvent(QResizeEvent* event)
{
#ifdef DEBUG_RESIZE
    qDebug().nospace()
            << Q_FUNC_INFO
            << ": resized to " << event->size()
            << "; minimumSizeHint() " << minimumSizeHint()
            << "; qSmartMinSize(this) " << qtlayouthelpers::qSmartMinSize(this);
#else
    Q_UNUSED(event)
#endif
}


void OpenableWidget::keyPressEvent(QKeyEvent* event)
{
    if (!event) {
        return;
    }
    const int key = event->key();
    const QEvent::Type type = event->type();
    if (key == Qt::Key_Escape && type == QEvent::KeyPress) {
        // Escape key pressed
        if (m_escape_key_can_abort) {
            if (m_escape_aborts_without_confirmation ||
                    uifunc::confirm(tr("Abort: are you sure?"),
                                    tr("Abort?"),
                                    "", "", this)) {
                // User confirms: abort
                emit aborting();
                emit finished();
            }
        }
    }
#ifdef DEBUG_PRESS_D_TO_DUMP
    if (key == Qt::Key_D && type == QEvent::KeyPress) {
        layoutdumper::dumpWidgetHierarchy(this);
    }
#endif
}
