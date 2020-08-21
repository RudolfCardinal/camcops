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
#include <QPointer>
#include <QWidget>
#include "widgets/basewidget.h"
class QGraphicsView;


class OpenableWidget : public QWidget
{
    // Widget that the CamCOPS main app knows how to open as a screen.
    // - See CamcopsApp::open().
    // - Examples include MenuWindow, HtmlInfoWindow, and Questionnaire.
    // - Tasks that run plain graphics may use them directly; see e.g. QolSG,
    //   which uses a ScreenLikeGraphicsView in an OpenableWidget.
    // - It is also a widget in its own right, so you can nest them; an example
    //   is the IDED3D task, which has a Questionnaire config screen followed
    //   by a graphics view.

    Q_OBJECT
public:

    // Constructor
    OpenableWidget(QWidget* parent = nullptr);

    // Ask our subwidget to build itself, if that's an OpenableWidget.
    // This is an opportunity to do stuff between creation and opening.
    virtual void build();

    // Do we want to be in fullscreen mode?
    bool wantsFullscreen() const;

    // Set fullscreen preference.
    void setWantsFullscreen(bool fullscreen = true);

    // Sets another widget as the only contents of this OpenableWidget.
    // (Sets m_subwidget.)
    void setWidgetAsOnlyContents(QWidget* widget,
                                 int margin = 0,
                                 bool fullscreen = true,
                                 bool esc_can_abort = true);

    // Will the Escape key (potentially) cause an abort? See
    // setEscapeKeyCanAbort().
    bool escapeKeyCanAbort() const;

    // Set whether the Escape key will cause an abort.
    // If true, then if the user presses Esc:
    // - if without_confirmation, then we will emit aborting() then finished().
    // - otherwise, a dialogue will ask the user if they want to abort, and
    //   if so, we will emit aborting() then finished().
    void setEscapeKeyCanAbort(bool esc_can_abort,
                              bool without_confirmation = false);

    // Standard Qt widget overrides.
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

signals:
    // "User has aborted."
    void aborting();

    // "We've finished." [Also emitted after aborting(); see above.]
    void finished();

    // "Please put the window containing me into fullscreen mode."
    void enterFullscreen();

    // "Please take the window containing me out of fullscreen mode."
    void leaveFullscreen();

protected:
    QPointer<QWidget> m_subwidget;  // our subwidget
    bool m_wants_fullscreen;  // do we want to be in fullscreen mode?
    bool m_escape_key_can_abort;  // can the Esc key abort?
    bool m_escape_aborts_without_confirmation;  // can the Esc key abort instantly?
};
