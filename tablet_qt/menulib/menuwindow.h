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

#define MENUWINDOW_USE_HFW_LISTWIDGET  // good
// #define MENUWINDOW_USE_HFW_LAYOUT  // bad; window contains scroll area, which gets too short

#if defined(MENUWINDOW_USE_HFW_LISTWIDGET) == defined(MENUWINDOW_USE_HFW_LAYOUT)
#error Define MENUWINDOW_USE_HFW_LISTWIDGET xor MENUWINDOW_USE_HFW_LAYOUT
#endif

#include <QPointer>
#include <QSharedPointer>
#include <QVector>
#include "common/aliases_camcops.h"
#include "core/camcopsapp.h"  // for LockState
#include "layouts/layouts.h"
#include "menulib/menuitem.h"
#include "widgets/heightforwidthlistwidget.h"
#include "widgets/openablewidget.h"

class MenuHeader;
class Questionnaire;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

// A CamCOPS menu.

class MenuWindow : public OpenableWidget
{
    Q_OBJECT

public:
    MenuWindow(CamcopsApp& app,
               const QString& icon = "", bool top = false,
               bool offer_search = false);
    // Derived constructors should be LIGHTWEIGHT, as
    // MenuItem::MenuItem(MenuProxyPtr, CamcopsApp&) will create an INSTANCE
    // to get the title/subtitle.
    // ... note that we can't have a virtual static function (as we would in
    // Python: a classmethod that can be overridden), so title etc.
    // can't be static.
    // ... Note also that destroying and recreating the menu header etc.
    // seem to lead to dangers and loose signals (well, doubled signals --
    // possibly because we were connecting signals during a signal call), so we
    // do need to create those.

    // If it's cheap, populate m_items in the constructor.
    // If it's expensive (e.g. task lists), override build() to do:
    // (a) populate m_items;
    // (b) call MenuWindow::build();
    // (c) +/- any additional work (e.g. signals/slots).

    // Set the menu's icon (displayed on other menus leading to it, and at the
    // top of the menu itself. The parameter is a CamCOPS icon filename stub.
    void setIcon(const QString& icon);
    QString icon() const;

    // Menu title. Dynamic, so that the language can be changed dynamically.
    virtual QString title() const = 0;

    // Menu subtitle.
    virtual QString subtitle() const;

    // Returns the zero-based index of the currently selected item.
    int currentIndex() const;

    // Returns the task instance represented by the currently selected item.
    TaskPtr currentTask() const;

    // Returns the patient instance represented by the currently selected item.
    PatientPtr currentPatient() const;

    // Catch generic events
    virtual bool event(QEvent* e) override;

    // Complain that the task isn't offering an editor, so can't be
    // viewed or edited.
    static void complainTaskNotOfferingEditor();

    // Connect Questionnaire::editStarted  -> Task::editStarted
    //     and Questionnaire::editFinished -> Task::editFinished
    static void connectQuestionnaireToTask(OpenableWidget* widget, Task* task);

protected:
    // Ensures items are recreated in full
    void rebuild(bool rebuild_header = true);

    // Make the Qt widget layout. Calls extraLayoutCreation().
    void makeLayout();

    // Additional function that subclasses can override to specialize layout.
    virtual void extraLayoutCreation() {}

    // Called by the default implementation of build(), for simplicity
    virtual void makeItems() {}

    // Create widgets. Called by the OpenableWidget framework prior to opening.
    void build() override;

    // Called by build() as it finishes. Allows subclasses to do extra
    // processing, e.g. emitting signals.
    virtual void afterBuild() {}

    // Load or reload the stylesheet on our widget.
    void reloadStyleSheet();
    void loadStyleSheet();

signals:
    // "The menu header should offer the 'add' button (or not).'
    void offerAdd(bool offer_add);

    // "The menu header should offer the 'view' button (or not).'
    void offerView(bool offer_view);

    // "The menu header should offer the 'edit'/'delete' buttons (or not).'
    void offerEditDelete(bool offer_edit, bool offer_delete);

    // "The menu header should offer the 'finish' flag (or not).'
    void offerFinishFlag(bool offer_finish_flag);

public slots:
    // "The menu selection has changed."
    void menuItemSelectionChanged();

    // "A menu item has been clicked."
    void menuItemClicked(QListWidgetItem *item);

    // "The application's lock state has changed."
    void lockStateChanged(CamcopsApp::LockState lockstate);

    // "View the current item."
    virtual void viewItem();

    // "Edit the current item."
    virtual void editItem();

    // "Delete the current item."
    virtual void deleteItem();

    // "Print the menu layout to the debugging stream."
    void debugLayout();

protected slots:
    // "The search text has changed; re-filter the list of menu items."
    void searchTextChanged(const QString& text);

protected:
    // View a task, if one is selected.
    void viewTask();

    // Edit a task, if one is selected and editable. Check first.
    void editTask();

    // Edit a task, if one is selected and editable. Do it now.
    void editTaskConfirmed(const TaskPtr& task);

    // Delete a task, if one is selected
    void deleteTask();

    // Toggle the finish flag of the currently selected task/patient.
    void toggleFinishFlag();

protected:
    CamcopsApp& m_app;
    QString m_icon;
    bool m_top;
    bool m_offer_search;
    QVector<MenuItem> m_items;
#ifdef MENUWINDOW_USE_HFW_LAYOUT
    QPointer<VBoxLayout> m_mainlayout;
#else
    QPointer<QVBoxLayout> m_mainlayout;
#endif
    QPointer<MenuHeader> m_p_header;
    QPointer<QLineEdit> m_search_box;
#ifdef MENUWINDOW_USE_HFW_LISTWIDGET
    QPointer<HeightForWidthListWidget> m_p_listwidget;
#else
    QPointer<QListWidget> m_p_listwidget;
#endif
};
