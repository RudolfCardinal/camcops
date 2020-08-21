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

// #define DEBUG_SELECTIONS
// #define OFFER_LAYOUT_DEBUG_BUTTON
// #define SHOW_PID_TO_DEBUG_STREAM  // should be disabled for production
#define SHOW_TASK_TIMING

#include "menuwindow.h"
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "db/dbnestabletransaction.h"
#include "dbobjects/patient.h"
#include "dialogs/scrollmessagebox.h"
#include "layouts/layouts.h"
#include "lib/filefunc.h"
#include "lib/layoutdumper.h"
#include "lib/slowguiguard.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "menulib/menuheader.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "tasklib/task.h"
#include "widgets/horizontalline.h"

const int BAD_INDEX = -1;


MenuWindow::MenuWindow(CamcopsApp& app,
                       const QString& icon, const bool top,
                       const bool offer_search) :
    m_app(app),
    m_icon(icon),
    m_top(top),
    m_offer_search(offer_search)
{
    setEscapeKeyCanAbort(!top, true);

    loadStyleSheet();
    setObjectName(cssconst::MENU_WINDOW_OUTER_OBJECT);

    // ------------------------------------------------------------------------
    // Layout
    // ------------------------------------------------------------------------

    /*
    For no clear reason, I have been unable to set the background colour
    of the widget that goes inside the QStackedLayout, either by class name
    or via setObjectName(), or with setAutoFillBackground(true).

    However, it works perfectly well to set the  background colour of inner
    widgets. So instead of this:

        QStackedLayout (main app)
            QWidget (MainWindow or Questionnaire)  <-- can't set bg colour
                m_mainlayout
                    widgets of interest

    it seems we have to do this:

        QStackedLayout (main app)
            QWidget (MenuWindow or Questionnaire)
                dummy_layout
                    dummy_widget  <-- set background colour of this one
                        m_mainlayout
                            widgets of interest
    */

#ifdef MENUWINDOW_USE_HFW_LAYOUT
    auto dummy_layout = new VBoxLayout();
#else
    auto dummy_layout = new QVBoxLayout();
#endif
    dummy_layout->setContentsMargins(uiconst::NO_MARGINS);
    setLayout(dummy_layout);
    auto dummy_widget = new QWidget();  // doesn't need to be BaseWidget; contains scrolling list
    dummy_widget->setObjectName(cssconst::MENU_WINDOW_BACKGROUND);
    dummy_layout->addWidget(dummy_widget);

#ifdef MENUWINDOW_USE_HFW_LAYOUT
    m_mainlayout = new VBoxLayout();
#else
    m_mainlayout = new QVBoxLayout();
#endif
    m_mainlayout->setContentsMargins(uiconst::NO_MARGINS);
    dummy_widget->setLayout(m_mainlayout);

    // QListWidget objects scroll themselves.
    // But we want everything to scroll within a QScrollArea.
    // https://forum.qt.io/topic/2058/expanding-qlistview-within-qscrollarea/2
    // It turns out to be very fiddly, and it's also perfectly reasonable to
    // keep the menu header visible, and have scroll bars showing the position
    // within the list view (both for menus and questionnaires, I'd think).
    // So we'll stick with a simple layout.

    // ------------------------------------------------------------------------
    // Rest of layout
    // ------------------------------------------------------------------------
    // When the framework calls build(), that'll set up the layout, etc.

    // ------------------------------------------------------------------------
    // Other signals
    // ------------------------------------------------------------------------

    // Do this in main constructor, not build(), since build() can be called
    // from this signal!
    connect(&m_app, &CamcopsApp::lockStateChanged,
            this, &MenuWindow::lockStateChanged,
            Qt::UniqueConnection);
}


void MenuWindow::setIcon(const QString& icon)
{
    m_icon = icon;
    m_p_header->setIcon(icon);
}


void MenuWindow::loadStyleSheet()
{
    setStyleSheet(m_app.getSubstitutedCss(uiconst::CSS_CAMCOPS_MENU));
}


void MenuWindow::reloadStyleSheet()
{
    loadStyleSheet();
    uifunc::repolish(this);
}


void MenuWindow::rebuild(bool rebuild_header)
{
    if (rebuild_header) {
        makeLayout();
    }
    makeItems();
    build();
}


void MenuWindow::makeLayout()
{
    // ------------------------------------------------------------------------
    // Clear any existing layout (in case we're rebuilding)
    // ------------------------------------------------------------------------
    uifunc::clearLayout(m_mainlayout);

    // ------------------------------------------------------------------------
    // Header
    // ------------------------------------------------------------------------

#ifdef OFFER_LAYOUT_DEBUG_BUTTON
    const bool offer_debug_layout = true;
#else
    const bool offer_debug_layout = false;
#endif
    m_p_header = new MenuHeader(this, m_app, m_top, "", m_icon,
                                offer_debug_layout);
    // ... we'll set its title later in build()
    m_mainlayout->addWidget(m_p_header);

    // header to us
    connect(m_p_header, &MenuHeader::backClicked,
            this, &MenuWindow::finished,
            Qt::UniqueConnection);  // unique as we may rebuild... safer.
    connect(m_p_header, &MenuHeader::debugLayout,
            this, &MenuWindow::debugLayout,
            Qt::UniqueConnection);
    connect(m_p_header, &MenuHeader::viewClicked,
            this, &MenuWindow::viewItem,
            Qt::UniqueConnection);
    connect(m_p_header, &MenuHeader::editClicked,
            this, &MenuWindow::editItem,
            Qt::UniqueConnection);
    connect(m_p_header, &MenuHeader::deleteClicked,
            this, &MenuWindow::deleteItem,
            Qt::UniqueConnection);
    connect(m_p_header, &MenuHeader::finishFlagClicked,
            this, &MenuWindow::toggleFinishFlag,
            Qt::UniqueConnection);

    // us to header
    connect(this, &MenuWindow::offerAdd,
            m_p_header, &MenuHeader::offerAdd,
            Qt::UniqueConnection);
    connect(this, &MenuWindow::offerView,
            m_p_header, &MenuHeader::offerView,
            Qt::UniqueConnection);
    connect(this, &MenuWindow::offerEditDelete,
            m_p_header, &MenuHeader::offerEditDelete,
            Qt::UniqueConnection);
    connect(this, &MenuWindow::offerFinishFlag,
            m_p_header, &MenuHeader::offerFinishFlag,
            Qt::UniqueConnection);

    // ------------------------------------------------------------------------
    // Search box
    // ------------------------------------------------------------------------
    // Given that we are working with a QListWidget or derivative, searching
    // is a bit less intuitive. However...
    // [See also https://stackoverflow.com/questions/2695878/creating-a-qlineedit-search-field-for-items-displayed-in-a-qlistview]

    if (m_offer_search) {
        // Label
        auto searchlabel = new QLabel(tr("Type to filter:"));
        m_mainlayout->addWidget(searchlabel);
        // Search box
        m_search_box = new QLineEdit();
        m_mainlayout->addWidget(m_search_box);
        // Signals
        connect(m_search_box.data(), &QLineEdit::textChanged,
                this, &MenuWindow::searchTextChanged);
    }

    // ------------------------------------------------------------------------
    // List
    // ------------------------------------------------------------------------

#ifdef MENUWINDOW_USE_HFW_LISTWIDGET
    m_p_listwidget = new HeightForWidthListWidget();
#else
    m_p_listwidget = new QListWidget();
#endif
    m_mainlayout->addWidget(m_p_listwidget);

    connect(m_p_listwidget, &QListWidget::itemSelectionChanged,
            this, &MenuWindow::menuItemSelectionChanged,
            Qt::UniqueConnection);
    connect(m_p_listwidget, &QListWidget::itemClicked,
            this, &MenuWindow::menuItemClicked,
            Qt::UniqueConnection);
    connect(m_p_listwidget, &QListWidget::itemActivated,
            this, &MenuWindow::menuItemClicked,
            Qt::UniqueConnection);

    uifunc::applyScrollGestures(m_p_listwidget->viewport());

    // ------------------------------------------------------------------------
    // Subclass specialization of layout
    // ------------------------------------------------------------------------

    extraLayoutCreation();
}


void MenuWindow::build()
{
    // qDebug() << Q_FUNC_INFO;

    if (m_items.isEmpty()) {  // First time through
        makeLayout();
        makeItems();
    }

    m_p_header->setTitle(title());

    m_p_listwidget->clear();

    // Method 1: QListWidget, QListWidgetItem
    // Size hints: https://forum.qt.io/topic/17481/easiest-way-to-have-a-simple-list-with-custom-items/4
    // Note that the widgets call setSizePolicy.
    bool preselected = false;
    const int app_selected_patient_id = m_app.selectedPatientId();
    for (int i = 0; i < m_items.size(); ++i) {
        MenuItem item = m_items.at(i);
        QWidget* row = item.rowWidget(m_app);
        auto listitem = new QListWidgetItem("", m_p_listwidget);
        listitem->setData(Qt::UserRole, QVariant(i));
#ifdef MENUWINDOW_USE_HFW_LISTWIDGET
        listitem->setSizeHint(m_p_listwidget->widgetSizeHint(row));
#else
        listitem->setSizeHint(row->sizeHint());
#endif
        m_p_listwidget->setItemWidget(listitem, row);
        if (item.patient()
                && item.patient()->id() == app_selected_patient_id) {
#ifdef DEBUG_SELECTIONS
            qDebug() << Q_FUNC_INFO << "preselecting patient at index" << i;
#endif
            // m_p_listwidget->item(i)->setSelected(true);
            m_p_listwidget->setCurrentItem(listitem);

            // DO NOT just setSelected(); that leaves currentItem() and the
            // (obviously) visible selection out of sync, which leads to
            // major user errors.
            // setCurrentItem() will also select the item;
            // http://doc.qt.io/qt-5/qlistwidget.html#setCurrentItem

            preselected = true;
        }
    }
    menuItemSelectionChanged();
    if (preselected) {
        m_p_listwidget->setFocus();
        // http://stackoverflow.com/questions/23065151/how-to-set-an-item-in-a-qlistwidget-as-initially-highlighted
    } else if (m_search_box) {
        m_search_box->setFocus();
    }

    // Method 2: QListView, QStandardItemModel, custom delegate
    // http://doc.qt.io/qt-5/qlistview.html
    // argh!

    // Stretch not necessary, even if the menu is short (the QListWidget
    // seems to handle this fine).

    afterBuild();
}


QString MenuWindow::subtitle() const
{
    return "";
}


QString MenuWindow::icon() const
{
    return m_icon;
}


void MenuWindow::menuItemSelectionChanged()
{
    // Set the verb buttons

    // WHAT'S BEEN CHOSEN?
    QList<QListWidgetItem*> selected_items = m_p_listwidget->selectedItems();
    if (selected_items.isEmpty()) {
#ifdef DEBUG_SELECTIONS
        qDebug() << Q_FUNC_INFO << "Nothing selected";
#endif
        emit offerView(false);
        emit offerEditDelete(false, false);
        emit offerFinishFlag(false);
        return;
    }
    QListWidgetItem* item = selected_items.at(0);
    const QVariant v = item->data(Qt::UserRole);
    const int i = v.toInt();
    if (i < 0 || i >= m_items.size()) {
        qWarning() << Q_FUNC_INFO << "Selection out of range:" << i
                   << "(vector size:" << m_items.size() << ")";
        return;
    }
    MenuItem& m = m_items[i];
#ifdef DEBUG_SELECTIONS
    qInfo() << "Selected:" << m;
#endif
    TaskPtr task = m.task();
    PatientPtr patient = m.patient();

    if (task) {
        // Notify the header (with its verb buttons). Leave it selected.
        emit offerView(true);
        emit offerEditDelete(task->isEditable(), true);
        emit offerFinishFlag(task->isAnonymous());
    } else if (patient) {
        bool selected = true;
        emit offerView(selected);
        emit offerEditDelete(selected, selected);
        emit offerFinishFlag(true);
    } else {
        emit offerView(false);
        emit offerEditDelete(false, false);
        // ... in case a task was selected before
        emit offerFinishFlag(false);
    }

    // The finish-flag button allows the user to mark either PATIENTS or
    // ANONYMOUS TASKS for removal from the tablet even if the user picks
    // the "copy" style of upload.
}


void MenuWindow::menuItemClicked(QListWidgetItem* item)
{
    // Act on a click

    const QVariant v = item->data(Qt::UserRole);
    const int i = v.toInt();
    if (i < 0 || i >= m_items.size()) {
        qWarning() << Q_FUNC_INFO << "Selection out of range:" << i
                   << "(vector size:" << m_items.size() << ")";
        return;
    }
    MenuItem& m = m_items[i];
    qInfo().noquote().nospace() << "Clicked: " << m.info();
    TaskPtr task = m.task();
    PatientPtr patient = m.patient();

    if (task) {
        // Nothing to do; see menuItemSelectionChanged()
    } else if (patient) {
        // qDebug() << Q_FUNC_INFO << "non-null patient pointer =" << patient
        //          << ", this =" << this;
        bool selected = false;
        if (m_app.selectedPatientId() == patient->id()) {
            // Clicked on currently selected patient; deselect it.
            m_app.setSelectedPatient(dbconst::NONEXISTENT_PK);
            m_p_listwidget->clearSelection();
        } else {
            selected = true;
            m_app.setSelectedPatient(patient->id());
        }
        emit offerView(selected);
        emit offerEditDelete(selected, selected);
    } else {
        // ACT ON IT. And clear the selection.
        m.act(m_app);
        m_p_listwidget->clearSelection();
    }
}


void MenuWindow::lockStateChanged(CamcopsApp::LockState lockstate)
{
    Q_UNUSED(lockstate)
    // mark as unused; http://stackoverflow.com/questions/1486904/how-do-i-best-silence-a-warning-about-unused-variables
    // qDebug() << Q_FUNC_INFO;
    build();  // calls down to derived class
}


bool MenuWindow::event(QEvent* e)
{
    const bool result = OpenableWidget::event(e);  // call parent
    const QEvent::Type type = e->type();
    if (type == QEvent::Type::LanguageChange) {
        rebuild();  // including rebuilding the header
    }
    return result;
}


void MenuWindow::viewItem()
{
    viewTask();
}


void MenuWindow::viewTask()
{
    TaskPtr task = currentTask();
    if (!task) {
        return;
    }
    const bool facsimile_available = task->isEditable();
#ifdef SHOW_PID_TO_DEBUG_STREAM
    const bool with_pid = true;
#else
    const bool with_pid = false;
#endif
    const QString instance_title = task->instanceTitle(with_pid);
    ScrollMessageBox msgbox(
                QMessageBox::Question,
                tr("View task"),
                tr("View in what format?"),
                this);
    QAbstractButton* summary = msgbox.addButton(tr("Summary"), QMessageBox::YesRole);
    QAbstractButton* detail = msgbox.addButton(tr("Detail"), QMessageBox::NoRole);
    msgbox.addButton(TextConst::cancel(), QMessageBox::RejectRole);  // e.g. Cancel
    QAbstractButton* facsimile = nullptr;
    if (facsimile_available) {
        facsimile = msgbox.addButton(tr("Facsimile"), QMessageBox::AcceptRole);
    }
    msgbox.exec();
    QAbstractButton* reply = msgbox.clickedButton();
    if (facsimile_available && reply == facsimile) {
        qInfo() << "View as facsimile:" << instance_title;
        OpenableWidget* widget = task->editor(true);
        if (!widget) {
            complainTaskNotOfferingEditor();
            return;
        }
        m_app.openSubWindow(widget, task);

    } else if (reply == detail) {
        qInfo() << "View detail:" << instance_title;
        QString detail = stringfunc::joinHtmlLines(task->detail());
#ifdef SHOW_TASK_TIMING
        detail += QString("<br><br>Editing time: <b>%1</b> s")
                .arg(task->editingTimeSeconds());
#endif
        uifunc::alert(detail, instance_title);

    } else if (reply == summary) {
        qInfo() << "View summary:" << instance_title;
        uifunc::alert(task->summary(), instance_title);
    }
}


void MenuWindow::editItem()
{
    editTask();
}


void MenuWindow::editTask()
{
    TaskPtr task = currentTask();
    if (!task || !task->isEditable()) {
        return;
    }
    const QString instance_title = task->instanceTitle();
    ScrollMessageBox msgbox(
                QMessageBox::Question,
                tr("Edit"),
                tr("Edit this task?") + "\n\n" +  instance_title,
                this);
    QAbstractButton* yes = msgbox.addButton(tr("Yes, edit"),
                                            QMessageBox::YesRole);
    msgbox.addButton(tr("No, cancel"), QMessageBox::NoRole);
    msgbox.exec();
    if (msgbox.clickedButton() != yes) {
        return;
    }
    editTaskConfirmed(task);
}


void MenuWindow::editTaskConfirmed(const TaskPtr& task)
{
#ifdef SHOW_PID_TO_DEBUG_STREAM
    const bool with_pid = true;
#else
    const bool with_pid = false;
#endif
    const QString instance_title = task->instanceTitle(with_pid);
    qInfo() << "Edit:" << instance_title;
    OpenableWidget* widget = task->editor(false);
    if (!widget) {
        complainTaskNotOfferingEditor();
        return;
    }
    connectQuestionnaireToTask(widget, task.data());
    m_app.openSubWindow(widget, task, true);
}


void MenuWindow::complainTaskNotOfferingEditor()
{
    uifunc::alert(tr("Task has declined to supply an editor!"),
                  tr("Can't edit/view task"));
}


void MenuWindow::connectQuestionnaireToTask(OpenableWidget* widget, Task* task)
{
    if (!widget || !task) {
        qWarning() << Q_FUNC_INFO << "null widget or null task";
        return;
    }
    auto questionnaire = dynamic_cast<Questionnaire*>(widget);
    if (!questionnaire) {
        return;
    }
    questionnairefunc::connectQuestionnaireToTask(questionnaire, task);
}


void MenuWindow::deleteItem()
{
    deleteTask();
}


void MenuWindow::deleteTask()
{
    TaskPtr task = currentTask();
    if (!task) {
        return;
    }
    const QString instance_title_for_user = task->instanceTitle();
    ScrollMessageBox msgbox(
                QMessageBox::Warning,
                tr("Delete"),
                tr("Delete this task?") + "\n\n" +  instance_title_for_user,
                this);
    QAbstractButton* yes = msgbox.addButton(tr("Yes, delete"),
                                            QMessageBox::YesRole);
    msgbox.addButton(tr("No, cancel"), QMessageBox::NoRole);
    msgbox.exec();
    if (msgbox.clickedButton() != yes) {
        return;
    }
    {
        SlowGuiGuard guard = m_app.getSlowGuiGuard(tr("Deleting task"),
                                                   TextConst::pleaseWait());
#ifdef SHOW_PID_TO_DEBUG_STREAM
        const QString& instance_title_for_debug = instance_title_for_user;
#else
        const QString instance_title_for_debug = task->instanceTitle(false);
#endif
        qInfo() << "Delete:" << instance_title_for_debug;
        DbNestableTransaction trans(m_app.db());
        task->deleteFromDatabase();
        rebuild();
    }
}


void MenuWindow::toggleFinishFlag()
{
    TaskPtr task = currentTask();
    PatientPtr patient = currentPatient();
    if (task && task->isAnonymous()) {
        DbNestableTransaction trans(m_app.db());
        task->toggleMoveOffTablet();
        build();
    } else if (patient) {
        patient->toggleMoveOffTablet();
        build();
    }
}


int MenuWindow::currentIndex() const
{
    QListWidgetItem* item = m_p_listwidget->currentItem();
    if (!item) {
        return BAD_INDEX;
    }
    const QVariant v = item->data(Qt::UserRole);
    const int i = v.toInt();
    if (i >= m_items.size() || i <= -1) {
        // Out of bounds; coerce to -1
        return BAD_INDEX;
    }
    return i;
}


TaskPtr MenuWindow::currentTask() const
{
    const int index = currentIndex();
    if (index == BAD_INDEX) {
        return TaskPtr(nullptr);
    }
    const MenuItem& item = m_items[index];
    return item.task();
}


PatientPtr MenuWindow::currentPatient() const
{
    const int index = currentIndex();
#ifdef DEBUG_SELECTIONS
    qDebug() << Q_FUNC_INFO << "index =" << index;
#endif
    if (index == BAD_INDEX) {
        qDebug() << Q_FUNC_INFO << "... bad index";
        return PatientPtr(nullptr);
    }
    const MenuItem& item = m_items[index];
    return item.patient();
}


void MenuWindow::debugLayout()
{
    layoutdumper::dumpWidgetHierarchy(this);
}


void MenuWindow::searchTextChanged(const QString& text)
{
    // qDebug() << "Search text:" << text;
    const bool search_empty = text.isEmpty();
    const QString search_text_lower = text.toLower();
    const int n_items = m_items.size();
    for (int i = 0; i < n_items; ++i) {
        const bool show = search_empty ||
                m_items.at(i).matchesSearch(search_text_lower);
        QListWidgetItem* item = m_p_listwidget->item(i);
        item->setHidden(!show);
    }
}
