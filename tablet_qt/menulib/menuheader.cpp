/*
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
#include "common/gui_defines.h"
#include "common/uiconstants.h"
#include "dbobjects/patient.h"
#include "lib/uifunc.h"
#include "widgets/horizontalline.h"
#include "widgets/imagebutton.h"
#include "widgets/labelwordwrapwide.h"

#ifdef GUI_USE_HFW_LAYOUT
#include "widgets/hboxlayouthfw.h"
#include "widgets/vboxlayouthfw.h"
#else
#include <QHBoxLayout>
#include <QVBoxLayout>
#endif


MenuHeader::MenuHeader(QWidget* parent,
                       CamcopsApp& app,
                       bool top,
                       const QString& title,
                       const QString& icon_filename,
                       bool debug_allowed)
    : QWidget(parent),
      m_app(app),
      m_icon_whisker_connected(nullptr),
      m_button_needs_upload(nullptr),
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
#ifdef GUI_USE_HFW_LAYOUT
    VBoxLayoutHfw* mainlayout = new VBoxLayoutHfw();
#else
    QVBoxLayout* mainlayout = new QVBoxLayout();
#endif
    setLayout(mainlayout);

    // ------------------------------------------------------------------------
    // Main row
    // ------------------------------------------------------------------------
#ifdef GUI_USE_HFW_LAYOUT
    HBoxLayoutHfw* toprowlayout = new HBoxLayoutHfw();
#else
    QHBoxLayout* toprowlayout = new QHBoxLayout();
#endif
    mainlayout->addLayout(toprowlayout);

    Qt::Alignment button_align = Qt::AlignHCenter | Qt::AlignTop;
    Qt::Alignment text_align = Qt::AlignLeft | Qt::AlignVCenter;

    // Back button (unless top)
    if (!top) {
        QAbstractButton* back = new ImageButton(UiConst::CBS_BACK);
        toprowlayout->addWidget(back, 0, button_align);
        connect(back, &QAbstractButton::clicked,
                this, &MenuHeader::backClicked,
                Qt::UniqueConnection);
    }

    // Icon
    if (!icon_filename.isEmpty()) {
        QLabel* icon = UiFunc::iconWidget(icon_filename, this);
        toprowlayout->addWidget(icon, 0, button_align);
    }

    // Title
    m_title_label = new LabelWordWrapWide(title);
    m_title_label->setAlignment(text_align);
    m_title_label->setObjectName(CssConst::MENU_WINDOW_TITLE);
    toprowlayout->addWidget(m_title_label, 0, text_align);

    // Spacing
    toprowlayout->addStretch();

    // Right-hand icons

    // - Debug
    if (debug_allowed) {
        m_button_debug = new QPushButton("Dump layout");
        connect(m_button_debug, &QAbstractButton::clicked,
                this, &MenuHeader::debugLayout);
        toprowlayout->addWidget(m_button_debug, 0, text_align);
    }

    // - Task verb buttons
    m_button_finish_flag = new ImageButton(UiConst::CBS_FINISHFLAG);
    m_button_view = new ImageButton(UiConst::CBS_ZOOM);
    m_button_edit = new ImageButton(UiConst::CBS_EDIT);
    m_button_delete = new ImageButton(UiConst::CBS_DELETE);
    m_button_add = new ImageButton(UiConst::CBS_ADD);
    toprowlayout->addWidget(m_button_finish_flag, 0, button_align);
    toprowlayout->addWidget(m_button_view, 0, button_align);
    toprowlayout->addWidget(m_button_edit, 0, button_align);
    toprowlayout->addWidget(m_button_delete, 0, button_align);
    toprowlayout->addWidget(m_button_add, 0, button_align);
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

    // - Whisker
    m_icon_whisker_connected = UiFunc::iconWidget(
        UiFunc::iconFilename(UiConst::ICON_WHISKER), this);
    toprowlayout->addWidget(m_icon_whisker_connected);
    toprowlayout->setAlignment(m_icon_whisker_connected, button_align);
    whiskerConnectionStateChanged(m_app.whiskerConnected());

    // - Needs upload
    m_button_needs_upload = new ImageButton(UiConst::ICON_UPLOAD);
    toprowlayout->addWidget(m_button_needs_upload, 0, button_align);
    needsUploadChanged(m_app.needsUpload());
    connect(m_button_needs_upload, &QAbstractButton::clicked,
            &m_app, &CamcopsApp::upload);

    // - Locked/unlocked/privileged
    m_button_locked = new ImageButton(UiConst::CBS_LOCKED);
    m_button_unlocked = new ImageButton(UiConst::CBS_UNLOCKED);
    m_button_privileged = new ImageButton(UiConst::CBS_PRIVILEGED);
    toprowlayout->addWidget(m_button_locked, 0, button_align);
    toprowlayout->addWidget(m_button_unlocked, 0, button_align);
    toprowlayout->addWidget(m_button_privileged, 0, button_align);
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
    HorizontalLine* horizline = new HorizontalLine(UiConst::HEADER_HLINE_WIDTH);
    horizline->setObjectName(CssConst::HEADER_HORIZONTAL_LINE);
    mainlayout->addWidget(horizline);

    // ------------------------------------------------------------------------
    // Selected patient
    // ------------------------------------------------------------------------
    m_patient_info = new LabelWordWrapWide();
    m_patient_info->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_patient_info->setObjectName(CssConst::MENU_HEADER_PATIENT_INFO);
    mainlayout->addWidget(m_patient_info);
    m_no_patient = new LabelWordWrapWide(tr("No patient selected"));
    m_no_patient->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_no_patient->setObjectName(CssConst::MENU_HEADER_NO_PATIENT);
    mainlayout->addWidget(m_no_patient);
    setPatientDetails(m_app.selectedPatient());

    // ========================================================================
    // Incoming signals
    // ========================================================================
    connect(&m_app, &CamcopsApp::whiskerConnectionStateChanged,
            this, &MenuHeader::whiskerConnectionStateChanged,
            Qt::UniqueConnection);
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


void MenuHeader::lockStateChanged(CamcopsApp::LockState lockstate)
{
    m_button_locked->setVisible(lockstate == CamcopsApp::LockState::Locked);
    m_button_unlocked->setVisible(lockstate == CamcopsApp::LockState::Unlocked);
    m_button_privileged->setVisible(lockstate == CamcopsApp::LockState::Privileged);
}


void MenuHeader::whiskerConnectionStateChanged(bool connected)
{
    m_icon_whisker_connected->setVisible(connected);
}


void MenuHeader::needsUploadChanged(bool needs_upload)
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
    bool selected = patient != nullptr;
    QString info;

    if (selected) {
        info = QString("<b>%1, %2</b> (%3, DOB %4); %5")
                .arg(patient->surname().toUpper())
                .arg(patient->forename())
                .arg(QString("%1 y").arg(patient->ageYears()))
                .arg(patient->dobText())
                .arg(patient->shortIdnumSummary());
    }
#ifdef DEBUG_SLOTS
    qDebug() << Q_FUNC_INFO << info << "[patient:" << patient << "]";
#endif
    m_patient_info->setText(info);
    m_no_patient->setVisible(!selected);
    m_patient_info->setVisible(selected);
}


void MenuHeader::offerView(bool offer_view)
{
    m_button_view->setVisible(offer_view);
}


void MenuHeader::offerEditDelete(bool offer_edit, bool offer_delete)
{
    m_button_edit->setVisible(offer_edit);
    m_button_delete->setVisible(offer_delete);
}


void MenuHeader::offerAdd(bool offer_add)
{
    m_button_add->setVisible(offer_add);
}


void MenuHeader::offerFinishFlag(bool offer_finish_flag)
{
    m_button_finish_flag->setVisible(offer_finish_flag);
}
