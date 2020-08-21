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

#define CAMERA_LOAD_FROM_DISK_PROMPTLY

#define CAMERA_USE_QML
// #define CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER  // required for viewfinder on Android

/*

SUMMARY OF DECISIONS about camera methods:

1.  QCamera with QCameraViewFinder
    - fine under Linux
    - under Android, blank viewfinder, and warning:

      warning: The video surface is not compatible with any format supported by the camera

2.  QCamera with custom CameraFrameGrabber
    - fine under Linux
    - under Android, segfault with:
      attachToContext: invalid current EGLDisplay

3.  QML
    - not only fine under Linux, but significantly better (features +/- speed)
    - declarative-camera demo works under Android

*/

#include <QCamera>
#include <QCameraImageCapture>
#include <QImage>
#include <QPointer>
#include <QtQuickWidgets/QQuickWidget>
#include <QSet>
#include "widgets/openablewidget.h"
class CameraFrameGrabber;
class QAbstractButton;
class QCameraInfo;
class QCameraViewfinder;
class QLabel;
class QQuickWidget;
class QPushButton;
class QStatusBar;
class QVideoFrame;


class CameraQml : public OpenableWidget
{
    // Widget to take a photo, using QML.
    // See resources/camcops/camera_qml/camera.qml, the top-level QML file.

    Q_OBJECT
public:
    // ========================================================================
    // Constructor/destructor
    // ========================================================================

    // Constructor
    CameraQml(QWidget* parent = nullptr);

    // ========================================================================
    // Public interface
    // ========================================================================

    // Close the camera. Emit the "finished" signal.
    void finish();

signals:
    // If possible, we will emit rawImageCaptured, because performance is
    // better. Failing that, we will emit imageCaptured. ONE OR THE OTHER will
    // be emitted.

    // "We've captured an image." High performance.
    void rawImageCaptured(QByteArray data,  // QByteArray is copy-on-write
                          QString extension_without_dot,
                          QString mimetype);

    // "We've captured an image." Lower performance.
    void imageCaptured(QImage image);  // QImage is copy-on-write

    // "User has cancelled the operation."
    void cancelled();

    // ========================================================================
    // Internals
    // ========================================================================

protected:

    // Called when Qt has finished loading the QML.
    // Connects the QML object signals to our slots.
    void qmlFinishedLoading();

    // Delete a temporary camera file.
    void deleteFile(const QString& filename) const;

protected slots:

    // Note that we route signals through the various QML objects to the QML
    // root object, m_qml_view->rootObject(), and connect the root object's
    // signals to our C++ object.

    // "The QML root object's status has changed."
    // Called from m_qml_view's QQuickWidget::statusChanged.
    void qmlStatusChanged(QQuickWidget::Status status);

    // "The camera QML says a temporary file is no longer needed."
    // Called from the fileNoLongerNeeded signal defined in camera.qml.
    void deleteSuperfluousFile(const QString& filename) const;

    // "The camera QML has captured an image via a temporary file."
    // Called from the imageSavedToFile signal defined in camera.qml.
    void cameraHasCapturedImage(const QString& filename);

protected:
    QPointer<QQuickWidget> m_qml_view;  // our QML view widget
};
