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

// #define DEBUG_SLOTS

#include "menuheader.h"
#include <QAbstractButton>
#include <QLabel>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "dbobjects/patient.h"
#include "layouts/flowlayouthfw.h"
#include "layouts/layouts.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "widgets/basewidget.h"
#include "widgets/horizontalline.h"
#include "widgets/imagebutton.h"
#include "widgets/labelwordwrapwide.h"


MenuHeader::MenuHeader(QWidget* parent,
                       CamcopsApp& app,
                       const bool top,
                       const QString& title,
                       const QString& icon_filename,
                       const bool debug_allowed)
    : QWidget(parent),
      m_app(app),
      m_button_needs_upload(nullptr),  // waste of effort; constructed as nullptr
      m_button_debug(nullptr),
      m_button_view(nullptr),
      m_button_edit(nullptr),
      m_button_delete(nullptr),
      m_button_add(nullptr),
      m_button_locked(nullptr),
      m_button_unlocked(nullptr),
      m_button_privileged(nullptr),
      m_patient_info(nullptr),
      m_no_patient(nullptr)
{
    auto mainlayout = new VBoxLayout();
    setLayout(mainlayout);

    // ------------------------------------------------------------------------
    // Main row
    // ------------------------------------------------------------------------

    // Left
    m_top_bar = new BaseWidget();
    m_top_bar->setSizePolicy(sizehelpers::expandingFixedHFWPolicy());
    auto toprowlayout = new HBoxLayout();
    m_top_bar->setLayout(toprowlayout);
    mainlayout->addWidget(m_top_bar);

    const Qt::Alignment button_align = Qt::AlignHCenter | Qt::AlignTop;
    const Qt::Alignment text_align = Qt::AlignLeft | Qt::AlignVCenter;

    // - Back button (unless top)
    if (!top) {
        QAbstractButton* back = new ImageButton(uiconst::CBS_BACK);
        toprowlayout->addWidget(back, 0, button_align);
        connect(back, &QAbstractButton::clicked,
                this, &MenuHeader::backClicked,
                Qt::UniqueConnection);
    }

    // Spacing
    toprowlayout->addStretch();

    // Centre

    // - Icon for current menu
    m_icon = new QLabel();
    setIcon(icon_filename, false);
    toprowlayout->addWidget(m_icon, 0, button_align);

    // - Title
    m_title_label = new LabelWordWrapWide(title);
    m_title_label->setAlignment(text_align);
    m_title_label->setObjectName(cssconst::MENU_WINDOW_TITLE);
    toprowlayout->addWidget(m_title_label, 0, text_align);

    // Spacing
    toprowlayout->addStretch();

    // Right-hand icons ("verbs")
    auto rh_icons = new FlowLayoutHfw();
    toprowlayout->addLayout(rh_icons);
    rh_icons->setHorizontalAlignmentOfContents(Qt::AlignRight);

    // - Debug
    if (debug_allowed) {
        m_button_debug = new QPushButton(tr("Dump layout"));
        connect(m_button_debug, &QAbstractButton::clicked,
                this, &MenuHeader::debugLayout);
        rh_icons->addWidget(m_button_debug, text_align);
    }

    // - Task verb buttons
    m_button_finish_flag = new ImageButton(uiconst::CBS_FINISHFLAG);
    m_button_view = new ImageButton(uiconst::CBS_ZOOM);
    m_button_edit = new ImageButton(uiconst::CBS_EDIT);
    m_button_delete = new ImageButton(uiconst::CBS_DELETE);
    m_button_add = new ImageButton(uiconst::CBS_ADD);
    rh_icons->addWidget(m_button_finish_flag, button_align);
    rh_icons->addWidget(m_button_view, button_align);
    rh_icons->addWidget(m_button_edit, button_align);
    rh_icons->addWidget(m_button_delete, button_align);
    rh_icons->addWidget(m_button_add, button_align);
    offerFinishFlag();
    offerView();
    offerEditDelete();
    offerAdd();
    connect(m_button_finish_flag, &QAbstractButton::clicked,
            this, &MenuHeader::finishFlagClicked);
    connect(m_button_view, &QAbstractButton::clicked,
            this, &MenuHeader::viewClicked);
    connect(m_button_edit, &QAbstractButton::clicked,
            this, &MenuHeader::editClicked);
    connect(m_button_delete, &QAbstractButton::clicked,
            this, &MenuHeader::deleteClicked);
    connect(m_button_add, &QAbstractButton::clicked,
            this, &MenuHeader::addClicked);

    // - Needs upload ("upload")
    m_button_needs_upload = new ImageButton(uiconst::ICON_UPLOAD);
    rh_icons->addWidget(m_button_needs_upload, button_align);
    needsUploadChanged(m_app.needsUpload());
    connect(m_button_needs_upload, &QAbstractButton::clicked,
            &m_app, &CamcopsApp::upload);

    // - Locked/unlocked/privileged
    m_button_locked = new ImageButton(uiconst::CBS_LOCKED);
    m_button_unlocked = new ImageButton(uiconst::CBS_UNLOCKED);
    m_button_privileged = new ImageButton(uiconst::CBS_PRIVILEGED);
    rh_icons->addWidget(m_button_locked, button_align);
    rh_icons->addWidget(m_button_unlocked, button_align);
    rh_icons->addWidget(m_button_privileged, button_align);
    lockStateChanged(m_app.lockstate());
    connect(m_button_locked, &QAbstractButton::clicked,
            &m_app, &CamcopsApp::unlock);
    connect(m_button_unlocked, &QAbstractButton::clicked,
            &m_app, &CamcopsApp::lock);
    connect(m_button_privileged, &QAbstractButton::clicked,
            &m_app, &CamcopsApp::unlock);

    // ------------------------------------------------------------------------
    // Horizontal line
    // ------------------------------------------------------------------------
    auto horizline = new HorizontalLine(uiconst::HEADER_HLINE_WIDTH);
    horizline->setObjectName(cssconst::HEADER_HORIZONTAL_LINE);
    mainlayout->addWidget(horizline);

    // ------------------------------------------------------------------------
    // Selected patient
    // ------------------------------------------------------------------------
    m_patient_info = new LabelWordWrapWide();
    m_patient_info->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_patient_info->setObjectName(cssconst::MENU_HEADER_PATIENT_INFO);
    mainlayout->addWidget(m_patient_info);
    m_no_patient = new LabelWordWrapWide(tr("No patient selected"));
    m_no_patient->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_no_patient->setObjectName(cssconst::MENU_HEADER_NO_PATIENT);
    mainlayout->addWidget(m_no_patient);
    setPatientDetails(m_app.selectedPatient());

    setCrippled(false);

    // ========================================================================
    // Incoming signals
    // ========================================================================
    connect(&m_app, &CamcopsApp::lockStateChanged,
            this, &MenuHeader::lockStateChanged,
            Qt::UniqueConnection);
    connect(&m_app, &CamcopsApp::selectedPatientChanged,
            this, &MenuHeader::selectedPatientChanged,
            Qt::UniqueConnection);
    connect(&m_app, &CamcopsApp::selectedPatientDetailsChanged,
            this, &MenuHeader::selectedPatientDetailsChanged,
            Qt::UniqueConnection);
    connect(&m_app, &CamcopsApp::needsUploadChanged,
            this, &MenuHeader::needsUploadChanged,
            Qt::UniqueConnection);
}


void MenuHeader::setTitle(const QString& title)
{
    if (!m_title_label) {
        return;
    }
    m_title_label->setText(title);
}


void MenuHeader::setIcon(const QString& icon_filename)
{
    setIcon(icon_filename, true);
}


void MenuHeader::setIcon(const QString& icon_filename, bool call_show_or_hide)
{
    uifunc::setLabelToIcon(m_icon, icon_filename);
    if (call_show_or_hide) {
        if (icon_filename.isEmpty()) {
            m_icon->hide();
        } else {
            m_icon->show();
        }
    }
}


void MenuHeader::setCrippled(const bool crippled)
{
    if (m_top_bar) {
        m_top_bar->setObjectName(crippled ? cssconst::MENU_HEADER_CRIPPLED
                                          : "");
    }
}


void MenuHeader::lockStateChanged(const CamcopsApp::LockState lockstate)
{
    m_button_locked->setVisible(lockstate == CamcopsApp::LockState::Locked);
    m_button_unlocked->setVisible(lockstate == CamcopsApp::LockState::Unlocked);
    m_button_privileged->setVisible(lockstate == CamcopsApp::LockState::Privileged);
}


void MenuHeader::needsUploadChanged(const bool needs_upload)
{
    m_button_needs_upload->setVisible(needs_upload);
}


void MenuHeader::selectedPatientChanged(const Patient* patient)
{
#ifdef DEBUG_SLOTS
    qDebug() << Q_FUNC_INFO << "[this:" << this << "]";
#endif
    setPatientDetails(patient);
}


void MenuHeader::selectedPatientDetailsChanged(const Patient* patient)
{
#ifdef DEBUG_SLOTS
    qDebug() << Q_FUNC_INFO << "[this:" << this << "]";
#endif
    setPatientDetails(patient);
}


void MenuHeader::setPatientDetails(const Patient* patient)
{
    const bool selected = patient != nullptr;
    QString info;

    if (selected) {
        info = patient->oneLineHtmlDetailString();
    }
#ifdef DEBUG_SLOTS
    qDebug() << Q_FUNC_INFO << info << "[patient:" << patient << "]";
#endif
    m_patient_info->setText(info);
    m_no_patient->setVisible(!selected);
    m_patient_info->setVisible(selected);
}


void MenuHeader::offerView(const bool offer_view)
{
    m_button_view->setVisible(offer_view);
}


void MenuHeader::offerEditDelete(const bool offer_edit,
                                 const bool offer_delete)
{
    m_button_edit->setVisible(offer_edit);
    m_button_delete->setVisible(offer_delete);
}


void MenuHeader::offerAdd(const bool offer_add)
{
    m_button_add->setVisible(offer_add);
}


void MenuHeader::offerFinishFlag(const bool offer_finish_flag)
{
    m_button_finish_flag->setVisible(offer_finish_flag);
}
