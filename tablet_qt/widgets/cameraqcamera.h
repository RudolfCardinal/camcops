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
#define CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER  // required for viewfinder on Android

/*

SUMMARY OF DECISIONS about camera methods: see CameraQml class.

*/

#include <QCamera>
#include <QCameraImageCapture>
#include <QImage>
#include <QPointer>
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

#ifndef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
#define CAMERA_QCAMERA_USE_QCAMERAVIEWFINDER
#endif


class CameraQCamera : public OpenableWidget
{
    // Widget to take a photo.
    // DEPRECATED at present in favour of QML version; see CameraQml.

    Q_OBJECT
public:
    // ========================================================================
    // Constructor/destructor
    // ========================================================================

    // Construct with stylesheet.
    CameraQCamera(const QString& stylesheet, QWidget* parent = nullptr);

    // Construct with QCameraInfo and stylesheet.
    CameraQCamera(const QCameraInfo& camera_info, const QString& stylesheet,
           QWidget* parent = nullptr);

    // Destructor.
    ~CameraQCamera();

    // ========================================================================
    // Public interface
    // ========================================================================

    // Emit the finished() signal.
    void finish();

    // Return the latest image captured.
    QImage image() const;

    // Choose the preview image's resolution.
    void setPreviewResolution(const QSize& resolution);

    // Choose the captured image's resolution.
    void setMainResolution(const QSize& resolution);

signals:
    // "We've captured this image."
    // - Note that QImage is copy-on-write; passing QImage is efficient.
    void imageCaptured(QImage image);

    // "User chose to cancel."
    void cancelled();

    // ========================================================================
    // Internals
    // ========================================================================
protected:

    // Update the display state for the buttons ("take", "lock", "cancel").
    void updateButtons();

    // Start the camera object.
    void startCamera();

    // Stop the camera object.
    void stopCamera();

    // Choose a camera.
    void setCamera(const QCameraInfo& camera_info);

    // Lock/unlock the camera.
    void toggleLock();

    // Unlock the camera.
    void unlockCamera();

    // Lock the camera settings (including autofocusing).
    void searchAndLockCamera();

    // Sets exposure compensation.
    void setExposureCompensation(int index);

    // void configureImageSettings();

    // Standard Qt overrides.
    void closeEvent(QCloseEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

protected slots:

    // "User has clicked the 'Take' button."
    void takeImage();

    // "Update the UI to reflect the camera's state."
    void updateCameraState(QCamera::State state);

    // "Pop up a message showing a camera error."
    void displayCameraError(QCamera::Error value);

    // "Update our indicators to reflect a change in the camera's lock status."
    void updateLockStatus(QCamera::LockStatus status,
                          QCamera::LockChangeReason reason);

    // "Change the ready-for-capture state."
    void readyForCapture(bool ready);

    // "An image has arrived via a temporary disk file."
    void imageSaved(int id, const QString& filename);

    // "An image has arrived via a buffer."
    void imageAvailable(int id, const QVideoFrame& buffer);

    // "Display an error that occurred during the image capture process."
    void displayCaptureError(int id, QCameraImageCapture::Error error,
                             const QString& error_string);

#ifdef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
    // "An image is available from the video surface viewfinder."
    void handleFrame(QImage image);  // QImage is copy-on-write
#endif

protected:
    QSize m_resolution_preview;  // resolution of our preview image
    QSize m_resolution_main;  // resolution of images we capture
    QSharedPointer<QCamera> m_camera;  // our camera
    QSharedPointer<QCameraImageCapture> m_capture;  // records images
#ifdef CAMERA_QCAMERA_USE_QCAMERAVIEWFINDER
    QPointer<QCameraViewfinder> m_viewfinder;  // our viewfinder
#endif
#ifdef CAMERA_QCAMERA_USE_VIDEO_SURFACE_VIEWFINDER
    QPointer<CameraFrameGrabber> m_framegrabber;  // our video viewfinder
    QPointer<QLabel> m_label_viewfinder;  // label to display viewfinder image
#endif
    QPointer<QPushButton> m_lock_button;  // lock state button; shows e.g. "Focus", "Unlock"
    QPointer<QPushButton> m_button_cancel;  // "Cancel"
    QPointer<QStatusBar> m_status_bar;  // shows status messages
    QPointer<QAbstractButton> m_button_take;  // "Take"

    // QImageEncoderSettings m_image_settings;  // image encoder settings

    bool m_ready;  // ready to capture?
    bool m_capturing_image;  // currently capturing?
    bool m_exiting;  // closing/exiting?
    QImage m_most_recent_image;  // most recently captured image
#ifndef CAMERA_LOAD_FROM_DISK_PROMPTLY
    QSet<QString> m_filenames_for_deletion;  // temporary files to delete
    QString m_most_recent_filename;  // file with most recent image
    enum class CapturedState {  // ways we may have captured an image
        Nothing,
        File,
        Buffer,
    };
    CapturedState m_captured_state;  // what have we captured?
#endif
};
