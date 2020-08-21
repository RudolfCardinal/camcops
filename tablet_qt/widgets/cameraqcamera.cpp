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

// MODIFIED FROM:
// http://doc.qt.io/qt-5/qtmultimedia-multimediawidgets-camera-camera-cpp.html

/*
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
*/

// #define DEBUG_CAMERA
#define USE_FILE

#include "cameraqcamera.h"
#include <QCameraInfo>
#include <QCameraViewfinder>
#include <QCameraViewfinderSettings>
#include <QCloseEvent>
#include <QFile>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QVBoxLayout>
#include <QVideoFrame>
#include "common/cssconst.h"
#include "common/preprocessor_aid.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "dialogs/scrollmessagebox.h"
#include "graphics/imagefunc.h"
#include "imagebutton.h"
#include "lib/uifunc.h"
#include "qobjects/cameraframegrabber.h"

// NOT IMPLEMENTED (see CameraQml instead): choose camera front/back
// NOT IMPLEMENTED (see CameraQml instead): set preview resolution (from those supported)
// NOT IMPLEMENTED (see CameraQml instead): set main resolution (from those supported)

/*

For examples see
- http://doc.qt.io/qt-5/qtmultimedia-multimediawidgets-camera-example.html
- qt5/qtmultimedia/examples/multimediawidgets/camera/camera.cpp
- qt5/qtmultimedia/examples/multimedia/declarative-camera/...

The "declarative-camera" example is the QML one.
- It's very responsive. It runs on Android properly.

The "multimediawidgets/camera" one is plain CPP.
- Its viewfinder is laggy in the default configuration.
- Its viewfinder doesn't work on Android.

Yet presumably all the QML stuff uses the same underlying CPP code?

Or maybe not?
- https://forum.qt.io/topic/59394/declarative-camera-vs-widget-based-camera-qml-to-c-breakout
- http://doc.qt.io/qt-5/qtqml-cppintegration-interactqmlfromcpp.html
- http://lists.qt-project.org/pipermail/android-development/2015-September/000734.html
- https://stackoverflow.com/questions/40153156/qt-qcamera-not-working-on-android
- https://bugreports.qt.io/browse/QTBUG-38233
- https://bugreports.qt.io/browse/QTBUG-41467
- http://omg-it.works/how-to-grab-video-frames-directly-from-qcamera/
- https://forum.qt.io/topic/47330/android-qcamera-5-4-beta
- https://www.ics.com/blog/combining-qt-widgets-and-qml-qwidgetcreatewindowcontainer

The actual error on Android is:
... warning: The video surface is not compatible with any format supported by the camera

*/


// ============================================================================
// Constructor/destructor
// ============================================================================

CameraQCamera::CameraQCamera(const QString& stylesheet, QWidget* parent) :
    CameraQCamera(QCameraInfo::defaultCamera(), stylesheet, parent)
{
}


CameraQCamera::CameraQCamera(const QCameraInfo& camera_info,
                             const QString& stylesheet,
                             QWidget* parent) :
    OpenableWidget(parent)
{
    m_resolution_preview = QSize(640, 480); // !!! Not implemented, but CameraQCamera superseded by CameraQml
    m_resolution_main = QSize(1024, 768); // !!! Not implemented, but CameraQCamera superseded by CameraQml

    setStyleSheet(stylesheet);

    m_camera.clear();
    m_capture.clear();
    m_ready = false;
    m_capturing_image = false;
    m_exiting = false;
#ifndef CAMERA_LOAD_FROM_DISK_PROMPTLY
    m_captured_state = CapturedState::Nothing;
#endif

    Qt::Alignment align = Qt::AlignLeft | Qt::AlignTop;

    m_button_take = new QPushButton(tr("Take"));
    connect(m_button_take, &QAbstractButton::clicked,
            this, &CameraQCamera::takeImage);

    m_button_cancel = new QPushButton(TextConst::cancel());
    connect(m_button_cancel, &QAbstractButton::clicked,
            this, &CameraQCamera::cancelled);

    m_lock_button = new QPushButton("lock button");

    auto button_layout = new QVBoxLayout();
    button_layout->addWidget(m_button_take, 0, align);
    button_layout->addWidget(m_lock_button, 0, align);
    button_layout->addWidget(m_button_cancel, 0, align);
    button_layout->addStretch();
    auto button_widget = new QWidget();
    button_widget->setLayout(button_layout);

#ifdef CAMERA_QCAMERA_USE_QCAMERAVIEWFINDER
    m_viewfinder = new QCameraViewfinder();
    // m_viewfinder->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
#endif
#ifdef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
    m_label_viewfinder = new QLabel();
#endif

    auto middle_layout = new QHBoxLayout();
    middle_layout->addWidget(button_widget, 0, align);
#ifdef CAMERA_QCAMERA_USE_QCAMERAVIEWFINDER
    middle_layout->addWidget(m_viewfinder, 0, align);
#endif
#ifdef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
    middle_layout->addWidget(m_label_viewfinder, 0, align);
#endif
    middle_layout->addStretch();

    m_status_bar = new QStatusBar();

    auto top_layout = new QVBoxLayout();
    top_layout->addLayout(middle_layout);
    top_layout->addStretch();
    top_layout->addWidget(m_status_bar);

    setLayout(top_layout);

    // Now, since the CSS of the outermost object is ignored within a
    // QStackedWidget...
    auto inner_widget = new QWidget();
    inner_widget->setObjectName(cssconst::CAMERA_INNER_OBJECT);
    inner_widget->setLayout(top_layout);

    // ... we need an outer layout too.
    auto outer_layout = new QVBoxLayout();
    outer_layout->setContentsMargins(uiconst::NO_MARGINS);
    outer_layout->addWidget(inner_widget);
    setLayout(outer_layout);

    setCamera(camera_info);
}


CameraQCamera::~CameraQCamera()
{
#ifndef CAMERA_LOAD_FROM_DISK_PROMPTLY
    // Remove anything that we've saved to disk
    for (auto filename : m_filenames_for_deletion) {
        bool success = QFile::remove(filename);
        qInfo() << "Deleting temporary camera file " << filename
                << (success ? "... success" : "... FAILED!");
    }
#endif
}


// ============================================================================
// Public interface
// ============================================================================

void CameraQCamera::finish()
{
    emit finished();
}


QImage CameraQCamera::image() const
{
    return m_most_recent_image;
#ifndef CAMERA_LOAD_FROM_DISK_PROMPTLY
    QImage img;
    switch (m_captured_state) {
    case CapturedState::Nothing:
        qDebug() << "... no file captured yet";
        break;
    case CapturedState::File:
        qDebug() << "... returning contents of" << m_most_recent_filename;
        qInfo() << "Camera::image: Loading image file...";
        img.load(m_most_recent_filename);
        qInfo() << "Camera::image: ... loaded.";
        break;
    case CapturedState::Buffer:
        qDebug() << "... returning image from buffer";
        img = m_most_recent_image;  // no cost; copy-on-write
        break;
    }
    return img;
#endif
}


void CameraQCamera::setPreviewResolution(const QSize& resolution)
{
    if (m_camera) {
        QCameraViewfinderSettings vf_settings;
        vf_settings.setResolution(resolution);
        vf_settings.setMinimumFrameRate(0);  // "let the backend choose optimally"
        vf_settings.setMaximumFrameRate(0);  // "let the backend choose optimally"
        m_camera->setViewfinderSettings(vf_settings);
    }
#ifdef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
    if (!m_label_viewfinder) {
        m_label_viewfinder->setFixedSize(resolution);
    }
#endif
}


void CameraQCamera::setMainResolution(const QSize& resolution)
{
    Q_UNUSED(resolution)  // NOT IMPLEMENTED - see CameraQml
}


// ============================================================================
// Talking to the camera
// ============================================================================

void CameraQCamera::setCamera(const QCameraInfo& camera_info)
{
    // ------------------------------------------------------------------------
    // QCamera
    // ------------------------------------------------------------------------
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO << "Creating camera with info" << camera_info;
#endif
    m_camera = QSharedPointer<QCamera>(new QCamera(camera_info));
#ifdef DEBUG_CAMERA
    qDebug() << "QCamera::supportedViewfinderResolutions() == "
             << m_camera->supportedViewfinderResolutions();
    qDebug() << Q_FUNC_INFO << "... done";
#endif
    connect(m_camera.data(), &QCamera::stateChanged,
            this, &CameraQCamera::updateCameraState);
    // QCamera::error is overloaded.
    // Disambiguate like this:
    void (QCamera::*camera_error)(QCamera::Error) = &QCamera::error;
    connect(m_camera.data(), camera_error,
            this, &CameraQCamera::displayCameraError);
    // QCamera::lockStatusChanged is overloaded.
    // Disambiguate like this:
    void (QCamera::*camera_lockstatus)(
                QCamera::LockStatus,
                QCamera::LockChangeReason) = &QCamera::lockStatusChanged;
    connect(m_camera.data(), camera_lockstatus,
            this, &CameraQCamera::updateLockStatus);

    // ------------------------------------------------------------------------
    // QCameraImageCapture
    // ------------------------------------------------------------------------
    m_capture = QSharedPointer<QCameraImageCapture>(
                new QCameraImageCapture(m_camera.data()));

    updateCameraState(m_camera->state());

    connect(m_capture.data(), &QCameraImageCapture::readyForCaptureChanged,
            this, &CameraQCamera::readyForCapture);
    connect(m_capture.data(), &QCameraImageCapture::imageSaved,
            this, &CameraQCamera::imageSaved);
    connect(m_capture.data(), &QCameraImageCapture::imageAvailable,
            this, &CameraQCamera::imageAvailable);
    // QCameraImageCapture::error is overloaded.
    // Disambiguate like this:
    void (QCameraImageCapture::*capture_error)(
                int,
                QCameraImageCapture::Error,
                const QString&) = &QCameraImageCapture::error;
    connect(m_capture.data(), capture_error,
            this, &CameraQCamera::displayCaptureError);

#ifdef USE_FILE
    const bool use_buffer = false;
#else
    const bool buffer_supported = m_capture->isCaptureDestinationSupported(
                QCameraImageCapture::CaptureToBuffer);
    const bool use_buffer = buffer_supported;
#endif
    if (use_buffer) {
        qInfo() << Q_FUNC_INFO << "Capturing to buffer";
        m_capture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
        // BUT... it saves a file anyway, to ~/Pictures/IMG_xxx.jpg.
        // Are we better off using explicit files, to avoid nasty leftovers?
        // Yes, we are. QCameraImageCapture::capture() appears always to write
        // to a file (implied by docs, too), and we do NOT want "leftovers".
        // https://stackoverflow.com/questions/43522004/qcameraimagecapture-saves-to-file-instead-of-buffer
        // http://php.wekeepcoding.com/article/10431109/Why+is+QCameraImageCapture+saving+an+image+to+the+hard+drive%3F
    } else {
        qInfo() << Q_FUNC_INFO << "Capturing to file";
        m_capture->setCaptureDestination(QCameraImageCapture::CaptureToFile);
    }

    // ------------------------------------------------------------------------
    // Viewfinder
    // ------------------------------------------------------------------------
#ifdef CAMERA_QCAMERA_USE_QCAMERAVIEWFINDER
    m_camera->setViewfinder(m_viewfinder);
#endif
#ifdef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
    m_framegrabber = new CameraFrameGrabber();
    connect(m_framegrabber.data(), &CameraFrameGrabber::frameAvailable,
            this, &CameraQCamera::handleFrame);
    m_camera->setViewfinder(m_framegrabber);
#endif
    setPreviewResolution(m_resolution_preview);

    // ------------------------------------------------------------------------
    // Main resolution
    // ------------------------------------------------------------------------
    setMainResolution(m_resolution_main);

    // ------------------------------------------------------------------------
    // Set up; let's go.
    // ------------------------------------------------------------------------
    if (m_camera->isCaptureModeSupported(QCamera::CaptureStillImage)) {
        m_camera->setCaptureMode(QCamera::CaptureStillImage);
    } else {
        qWarning() << Q_FUNC_INFO
                   << "Camera does not support QCamera::CaptureStillImage";
    }
    updateLockStatus(m_camera->lockStatus(),
                     QCamera::LockChangeReason::UserRequest);
    readyForCapture(m_capture->isReadyForCapture());
    startCamera();
}


void CameraQCamera::startCamera()
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO;
#endif
    m_camera->start();
}


void CameraQCamera::stopCamera()
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO;
#endif
    m_camera->stop();
}


void CameraQCamera::toggleLock()
{
    switch (m_camera->lockStatus()) {
    case QCamera::Searching:
    case QCamera::Locked:
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
    default:
#endif
        unlockCamera();
        break;
    case QCamera::Unlocked:
        searchAndLockCamera();
        break;
    }
}


void CameraQCamera::unlockCamera()
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO << "calling unlock()";
#endif
    m_camera->unlock();
}


void CameraQCamera::searchAndLockCamera()
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO << "calling searchAndLock()";
#endif
    m_camera->searchAndLock();
}


void CameraQCamera::updateLockStatus(const QCamera::LockStatus status,
                                     const QCamera::LockChangeReason reason)
{
    QColor indicationColor = Qt::black;

    switch (status) {
    case QCamera::Searching:
        indicationColor = Qt::yellow;
        m_status_bar->showMessage(tr("Focusing..."));
        m_lock_button->setText(tr("Focusing..."));
        break;
    case QCamera::Locked:
        indicationColor = Qt::darkGreen;
        m_lock_button->setText(tr("Unlock"));
        m_status_bar->showMessage(tr("Focused"), 2000);
        break;
    case QCamera::Unlocked:
        indicationColor = reason == QCamera::LockFailed ? Qt::red : Qt::black;
        m_lock_button->setText(tr("Focus"));
        if (reason == QCamera::LockFailed) {
            m_status_bar->showMessage(tr("Focus failed"), 2000);
        } else {
            m_status_bar->showMessage(tr("Camera"));
        }
    }

    QPalette palette = m_lock_button->palette();
    palette.setColor(QPalette::ButtonText, indicationColor);
    m_lock_button->setPalette(palette);
    updateButtons();
}


void CameraQCamera::takeImage()
{
    m_capturing_image = true;
    // !!! CameraQCamera::takeImage: implement some sort of wait message -- but superseded by CameraQml
    updateButtons();
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO << "calling capture()";
#endif
    m_capture->capture();  // a bit slow, so update buttons first
}


void CameraQCamera::displayCaptureError(const int id,
                                        const QCameraImageCapture::Error error,
                                        const QString& error_string)
{
    qWarning() << "Capture error:" << id << error << error_string;
    ScrollMessageBox::warning(this, tr("Image capture error"), error_string);
    m_capturing_image = false;
    updateButtons();
}


void CameraQCamera::displayCameraError(const QCamera::Error value)
{
    QString err = m_camera->errorString();
    qWarning() << "Camera error:" << value << err;
    ScrollMessageBox::warning(this, tr("Camera error"), err);
}


void CameraQCamera::updateCameraState(const QCamera::State state)
{
    // !!! CameraQCamera::updateCameraState -- not implemented, but superseded by CameraQml
    // Update the UI to reflect the camera's state
    switch (state) {
    case QCamera::ActiveState:
        /*
        ui->actionStartCamera->setEnabled(false);
        ui->actionStopCamera->setEnabled(true);
        ui->captureWidget->setEnabled(true);
        ui->actionSettings->setEnabled(true);
        */
        break;
    case QCamera::UnloadedState:
    case QCamera::LoadedState:
        /*
        ui->actionStartCamera->setEnabled(true);
        ui->actionStopCamera->setEnabled(false);
        ui->captureWidget->setEnabled(false);
        ui->actionSettings->setEnabled(false);
        */
        break;
    }
    updateButtons();
}


void CameraQCamera::updateButtons()
{
    if (m_button_take) {
        m_button_take->setEnabled(m_ready && !m_capturing_image);
    }
    if (m_lock_button) {
        m_lock_button->setEnabled(!m_capturing_image);
    }
    if (m_button_cancel) {
        m_button_cancel->setEnabled(!m_capturing_image);
    }
}


void CameraQCamera::setExposureCompensation(const int index)
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO;
#endif
    m_camera->exposure()->setExposureCompensation(index * 0.5);
}


void CameraQCamera::readyForCapture(const bool ready)
{
    m_ready = ready;
    updateButtons();
    // If you try to capture when it's not ready, it causes an error;
    // http://doc.qt.io/qt-5/qcameraimagecapture.html

    // Because the viewfinder tends to start out too small, this is a good
    // time:
#ifdef CAMERA_QCAMERA_USE_QCAMERAVIEWFINDER
    m_viewfinder->updateGeometry();
#endif
}

void CameraQCamera::imageSaved(const int id, const QString& filename)
{
    // Image has arrived via a disk file.
    Q_UNUSED(id)
    qDebug() << "Camera image has arrived via temporary file" << filename;
#ifdef CAMERA_LOAD_FROM_DISK_PROMPTLY
    m_most_recent_image.load(filename);
    qDebug() << "Camera image loaded";
    bool success = QFile::remove(filename);
    qDebug() << "Deleting temporary camera file " << filename
             << (success ? "... success" : "... FAILED!");
#else
    m_filenames_for_deletion.insert(filename);
    m_most_recent_filename = filename;
    m_captured_state = CapturedState::File;
#endif
    m_capturing_image = false;
    emit imageCaptured(image());
    if (m_exiting) {
        close();
    } else {
        updateButtons();
    }
}


void CameraQCamera::imageAvailable(const int id, const QVideoFrame& buffer)
{
    // Image has arrived via a buffer.

    // http://stackoverflow.com/questions/27297657/how-to-qvideoframe-to-qimage
    // http://stackoverflow.com/questions/27829830/convert-qvideoframe-to-qimage

    Q_UNUSED(id)
    qInfo() << "Camera::imageAvailable: fetching image from buffer...";
    m_most_recent_image = imagefunc::imageFromVideoFrame(buffer);
    qInfo() << "Camera::imageAvailable: ... fetched.";
#ifndef CAMERA_LOAD_FROM_DISK_PROMPTLY
    m_captured_state = CapturedState::Buffer;
#endif
    m_capturing_image = false;
    emit imageCaptured(image());
    if (m_exiting) {
        close();
    } else {
        updateButtons();
    }
}


void CameraQCamera::closeEvent(QCloseEvent* event)
{
    if (m_capturing_image) {
        setEnabled(false);
        m_exiting = true;
        event->ignore();
    } else {
        event->accept();
    }
}


void CameraQCamera::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        return;
    }

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        searchAndLockCamera();
        event->accept();
        break;
    case Qt::Key_Camera:
        if (m_camera->captureMode() == QCamera::CaptureStillImage) {
            takeImage();
        }
        event->accept();
        break;
    default:
        OpenableWidget::keyPressEvent(event);
    }
}


void CameraQCamera::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) {
        return;
    }

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        unlockCamera();
        break;
    default:
        OpenableWidget::keyReleaseEvent(event);
    }
}


#ifdef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
void CameraQCamera::handleFrame(const QImage image)
{
#ifdef DEBUG_CAMERA
    qDebug() << Q_FUNC_INFO;
#endif
    if (!m_label_viewfinder) {
        return;
    }
    const QPixmap pm = QPixmap::fromImage(image);
    m_label_viewfinder->setPixmap(pm);
}
#endif
