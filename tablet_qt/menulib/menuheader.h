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
#include "core/camcopsapp.h"  // for LockState

class BaseWidget;
class LabelWordWrapWide;
class QAbstractButton;
class QLabel;
class QPushButton;


// A widget for the top part of a CamCOPS menu (with title and control
// buttons).

class MenuHeader : public QWidget  // breaks as BaseWidget
{
    Q_OBJECT
public:
    MenuHeader(QWidget* parent,
               CamcopsApp& app,
               bool top,
               const QString& title,
               const QString& icon_filename = "",
               bool debug_allowed = false);

    // Set the title.
    void setTitle(const QString& title);

    // Set the icon.
    // The parameter is a CamCOPS icon filename stub.
    void setIcon(const QString& icon_filename);

    // Set the menu header colour for a task menu where that task is crippled.
    void setCrippled(bool crippled);

signals:
    // Back button clicked.
    void backClicked();

    // "View" action button clicked.
    void viewClicked();

    // "Edit" action button clicked.
    void editClicked();

    // "Delete" action button clicked.
    void deleteClicked();

    // "Add" action button clicked.
    void addClicked();

    // "Please display a debug dump of this menu's layout."
    void debugLayout();

    // "Finish" flag clicked.
    void finishFlagClicked();

public slots:
    // Should the header offer the "view" button?
    void offerView(bool offer_view = false);

    // Should the header offer the "edit"/"delete" buttons?
    void offerEditDelete(bool offer_edit = false, bool offer_delete = false);

    // Should the header offer the "add" button?
    void offerAdd(bool offer_add = false);

    // Should the header offer the "finish" flag?
    void offerFinishFlag(bool offer_finish_flag = false);

    // "The application's lock state has changed."
    void lockStateChanged(CamcopsApp::LockState lockstate);

    // "The application's need-to-upload state has changed."
    void needsUploadChanged(bool needs_upload);

    // "The application's selected patient has changed."
    void selectedPatientChanged(const Patient* patient);

    // "The details of the selected patient have changed."
    void selectedPatientDetailsChanged(const Patient* patient);

protected:
    // Internal version of setIcon().
    void setIcon(const QString& icon_filename, bool call_show_or_hide);

    // "Update the lines (at the bottom of the header) showing patient info."
    void setPatientDetails(const Patient* patient);

protected:
    CamcopsApp& m_app;
    QPointer<BaseWidget> m_top_bar;
    QPointer<LabelWordWrapWide> m_title_label;
    QPointer<QLabel> m_icon;
    QPointer<QAbstractButton> m_button_needs_upload;
    QPointer<QPushButton> m_button_debug;
    QPointer<QAbstractButton> m_button_finish_flag;
    QPointer<QAbstractButton> m_button_view;
    QPointer<QAbstractButton> m_button_edit;
    QPointer<QAbstractButton> m_button_delete;
    QPointer<QAbstractButton> m_button_add;
    QPointer<QAbstractButton> m_button_locked;
    QPointer<QAbstractButton> m_button_unlocked;
    QPointer<QAbstractButton> m_button_privileged;
    QPointer<QLabel> m_patient_info;
    QPointer<QLabel> m_no_patient;
};
