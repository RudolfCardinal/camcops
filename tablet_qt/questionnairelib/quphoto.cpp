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

// #define DEBUG_ROTATION
// #define DEBUG_CAMERA

#include "quphoto.h"
#include <QCameraInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include "core/camcopsapp.h"
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "lib/slowguiguard.h"
#include "qobjects/slownonguifunctioncaller.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/aspectratiopixmap.h"
#include "widgets/cameraqcamera.h"
#include "widgets/cameraqml.h"
#include "widgets/imagebutton.h"


QuPhoto::QuPhoto(BlobFieldRefPtr fieldref) :
    m_fieldref(fieldref),
    m_questionnaire(nullptr),
    m_incomplete_optional_label(nullptr),
    m_incomplete_mandatory_label(nullptr),
    m_field_problem_label(nullptr),
    m_image_widget(nullptr),
    m_camera(nullptr),
    m_main_widget(nullptr)
{
    m_have_opengl = openglfunc::isOpenGLPresent();
    const int n_cameras = QCameraInfo::availableCameras().count();
    m_have_camera = m_have_opengl && n_cameras > 0;
    // We check for OpenGL. If it's absent, and we try to create a camera,
    // we can get an instant segfault/crash. Better to fail gracefully.

    qDebug().nospace()
            << "OpenGL present: " << m_have_opengl
            << "; number of cameras: " << n_cameras
            << "; have a camera: " << m_have_camera;

    if (!m_fieldref) {
        qCritical() << "Null fieldref pointer to QuPhoto";
        return;
    }
    if (m_fieldref->mandatory()) {
        qWarning() << "You have set a QuPhoto to be mandatory, but not all "
                      "devices will support cameras!";
    }

    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuPhoto::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuPhoto::fieldValueChanged);
}


// ============================================================================
// Building the main widget
// ============================================================================

QPointer<QWidget> QuPhoto::makeWidget(Questionnaire* questionnaire)
{
    // Layout is:
    //
    // btn_take         optional_problem_markers
    // btn_rot_left     photo_photo_photo_photo_photo_photo
    // btn_rot_right    photo_photo_photo_photo_photo_photo
    // btn_clear        photo_photo_photo_photo_photo_photo
    //                  photo_photo_photo_photo_photo_photo
    //                  photo_photo_photo_photo_photo_photo
    //                  photo_photo_photo_photo_photo_photo

    m_questionnaire = questionnaire;
    const bool read_only = questionnaire->readOnly();
    const Qt::Alignment align = Qt::AlignLeft | Qt::AlignTop;

    QAbstractButton* button_open_camera = nullptr;
    QLabel* no_camera = nullptr;
    if (m_have_camera) {
        button_open_camera = new ImageButton(uiconst::CBS_CAMERA);
        button_open_camera->setEnabled(!read_only && m_have_camera);
        if (!read_only) {
            connect(button_open_camera, &QAbstractButton::clicked,
                    this, &QuPhoto::takePhoto);
        }
    } else {
        no_camera = new QLabel(m_have_opengl ? tr("No camera")
                                             : tr("No OpenGL"));
    }

    auto button_reset = new ImageButton(uiconst::CBS_DELETE);
    button_reset->setEnabled(!read_only);
    if (!read_only) {
        connect(button_reset, &QAbstractButton::clicked,
                this, &QuPhoto::resetFieldToNull);
    }

    auto button_rot_left = new ImageButton(uiconst::CBS_ROTATE_ANTICLOCKWISE);
    button_rot_left->setEnabled(!read_only);
    if (!read_only) {
        connect(button_rot_left, &QAbstractButton::clicked,
                this, &QuPhoto::rotateLeft);
    }

    auto button_rot_right = new ImageButton(uiconst::CBS_ROTATE_CLOCKWISE);
    button_rot_right->setEnabled(!read_only);
    if (!read_only) {
        connect(button_rot_right, &QAbstractButton::clicked,
                this, &QuPhoto::rotateRight);
    }

    auto button_layout = new QVBoxLayout();
    button_layout->setContentsMargins(uiconst::NO_MARGINS);
    if (m_have_camera) {
        button_layout->addWidget(button_open_camera, 0, align);
    } else {
        button_layout->addWidget(no_camera, 0, align);
    }
    button_layout->addWidget(button_rot_left, 0, align);
    button_layout->addWidget(button_rot_right, 0, align);
    button_layout->addWidget(button_reset, 0, align);
    button_layout->addStretch();

    auto button_widget = new QWidget();
    button_widget->setLayout(button_layout);

    m_incomplete_optional_label = uifunc::iconWidget(
                uifunc::iconFilename(uiconst::ICON_FIELD_INCOMPLETE_OPTIONAL));
    m_incomplete_mandatory_label = uifunc::iconWidget(
                uifunc::iconFilename(uiconst::ICON_FIELD_INCOMPLETE_MANDATORY));
    m_field_problem_label = uifunc::iconWidget(
                uifunc::iconFilename(uiconst::ICON_FIELD_PROBLEM));
    m_image_widget = new AspectRatioPixmap();

    auto image_layout = new QVBoxLayout();
    image_layout->setContentsMargins(uiconst::NO_MARGINS);
    image_layout->addWidget(m_incomplete_optional_label, 0, align);
    image_layout->addWidget(m_incomplete_mandatory_label, 0, align);
    image_layout->addWidget(m_field_problem_label, 0, align);
    image_layout->addWidget(m_image_widget, 0, align);
    // image_layout->addStretch();

    auto image_and_marker_widget = new QWidget();
    image_and_marker_widget->setLayout(image_layout);
    image_and_marker_widget->setSizePolicy(QSizePolicy::Expanding,
                                           QSizePolicy::Maximum);

    auto top_layout = new QHBoxLayout();
    top_layout->setContentsMargins(uiconst::NO_MARGINS);
    top_layout->addWidget(button_widget, 0, align);
    top_layout->addWidget(image_and_marker_widget, 0, align);
    top_layout->addStretch();

    m_main_widget = new QWidget();
    m_main_widget->setLayout(top_layout);
    m_main_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    setFromField();

    return m_main_widget;
}


// ============================================================================
// Talking to fields
// ============================================================================

void QuPhoto::setFromField()
{
    fieldValueChanged(m_fieldref.data());
}


FieldRefPtrList QuPhoto::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}


void QuPhoto::fieldValueChanged(const FieldRef* fieldref)
{
    const bool missing = fieldref->missingInput();
    const bool null = fieldref->isNull();
    bool loaded = false;
    if (m_incomplete_mandatory_label) {
        m_incomplete_mandatory_label->setVisible(missing);
    }
    if (m_incomplete_optional_label) {
        m_incomplete_optional_label->setVisible(!missing && null);
    }
    if (m_image_widget) {
        const bool show_image = !missing && !null;
        m_image_widget->setVisible(show_image);
        if (show_image) {
            QPixmap pm = fieldref->pixmap(&loaded);
            m_image_widget->setPixmap(pm);
        } else {
            m_image_widget->clear();
        }
    }
    if (m_field_problem_label) {
        m_field_problem_label->setVisible(!missing && !null && !loaded);
    }
}


void QuPhoto::takePhoto()
{
    if (!m_questionnaire) {
        qWarning() << Q_FUNC_INFO << "no questionnaire";
        return;
    }
    if (!m_have_camera) {
        qWarning() << Q_FUNC_INFO << "no camera";
        return;
    }

    SlowGuiGuard guard = m_questionnaire->app().getSlowGuiGuard();

#ifdef QUPHOTO_USE_CAMERA_QML

#ifdef DEBUG_CAMERA
    qDebug() << "Creating new CameraQml()...";
#endif
    m_camera = new CameraQml();
#ifdef DEBUG_CAMERA
    qDebug() << "... CameraQml() created";
#endif
    connect(m_camera, &CameraQml::cancelled, this, &QuPhoto::cameraCancelled);
    connect(m_camera, &CameraQml::rawImageCaptured,
            this, &QuPhoto::rawImageCaptured);
    connect(m_camera, &CameraQml::imageCaptured,
            this, &QuPhoto::imageCaptured);

#else

    QString stylesheet = m_questionnaire->getSubstitutedCss(
                uiconst::CSS_CAMCOPS_CAMERA);
#ifdef DEBUG_CAMERA
    qDebug() << "Creating new CameraQCamera()...";
#endif
    m_camera = new CameraQCamera(stylesheet);
#ifdef DEBUG_CAMERA
    qDebug() << "... CameraQCamera() created";
#endif
    connect(m_camera, &CameraQCamera::imageCaptured,
            this, &QuPhoto::imageCaptured);
    connect(m_camera, &CameraQCamera::cancelled,
            this, &QuPhoto::cameraCancelled);

#endif

#ifdef DEBUG_CAMERA
    qDebug() << "Asking questionnaire to open camera as subwidget...";
#endif
    m_questionnaire->openSubWidget(m_camera);
#ifdef DEBUG_CAMERA
    qDebug() << "... questionnaire has opened camera as subwidget.";
#endif
}


void QuPhoto::resetFieldToNull()
{
    if (m_fieldref->isNull()) {
        return;
    }
    if (!uifunc::confirm(tr("Delete this photo?"),
                         tr("Confirm deletion"),
                         tr("Yes, delete"),
                         tr("No, cancel"),
                         m_main_widget)) {
        return;
    }

#ifdef DEBUG_CAMERA
    qDebug() << "QuPhoto: setting field value to NULL...";
#endif
    bool changed = m_fieldref->setValue(QVariant());
    // ... skip originator; will call fieldValueChanged
#ifdef DEBUG_CAMERA
    qDebug() << "QuPhoto: ... field value set to NULL.";
#endif
    if (changed) {
        emit elementValueChanged();
    }
}


void QuPhoto::cameraCancelled()
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO;
#endif
    if (!m_camera) {
        return;
    }
    m_camera->finish();  // close the camera
}


void QuPhoto::imageCaptured(const QImage& image)
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO;
#endif
    if (!m_camera) {
        qWarning() << Q_FUNC_INFO << "... no camera!";
        return;
    }
    if (!m_questionnaire) {
        qWarning() << Q_FUNC_INFO << "... no questionnaire!";
        return;
    }
    bool changed = false;
    { // guard block
        SlowGuiGuard guard = m_questionnaire->app().getSlowGuiGuard(
                    tr("Saving image..."),
                    tr("Saving"));
#ifdef DEBUG_CAMERA
        qDebug() << "QuPhoto: setting field value to image...";
#endif
        changed = m_fieldref->setImage(image);
#ifdef DEBUG_CAMERA
        qDebug() << "QuPhoto: ... field value set to image.";
#endif
        m_camera->finish();  // close the camera
    }
    if (changed) {
        emit elementValueChanged();
    }
}


void QuPhoto::rawImageCaptured(const QByteArray& data,
                               const QString& extension_without_dot,
                               const QString& mimetype)
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO;
#endif
    if (!m_camera) {
        qWarning() << Q_FUNC_INFO << "... no camera!";
        return;
    }
    if (!m_questionnaire) {
        qWarning() << Q_FUNC_INFO << "... no questionnaire!";
        return;
    }
    bool changed = false;
    { // guard block
        SlowGuiGuard guard = m_questionnaire->app().getSlowGuiGuard(
                    tr("Saving image..."),
                    tr("Saving"));
#ifdef DEBUG_CAMERA
        qDebug() << "QuPhoto: setting field value to raw image...";
#endif
        changed = m_fieldref->setRawImage(data,
                                          extension_without_dot,
                                          mimetype);
#ifdef DEBUG_CAMERA
        qDebug() << "QuPhoto: ... field value set to raw image.";
#endif
        m_camera->finish();  // close the camera
    }
    if (changed) {
        emit elementValueChanged();
    }
}


void QuPhoto::rotate(const int angle_degrees_clockwise)
{
    if (m_fieldref->isNull()) {
        return;
    }
#ifdef DEBUG_ROTATION
    qDebug() << "QuPhoto: rotating...";
#endif
    {
        SlowNonGuiFunctionCaller slow_caller(
                std::bind(&QuPhoto::rotateWorker, this, angle_degrees_clockwise),
                m_main_widget,
                "Rotating...");
    }
#ifdef DEBUG_ROTATION
    qDebug() << "QuPhoto: ... rotation finished.";
#endif
    emit elementValueChanged();
}


void QuPhoto::rotateWorker(const int angle_degrees_clockwise)
{
    m_fieldref->rotateImage(angle_degrees_clockwise);
}


void QuPhoto::rotateLeft()
{
    // http://doc.qt.io/qt-4.8/qtransform.html#rotate
    rotate(-90);
}


void QuPhoto::rotateRight()
{
    rotate(90);
}
