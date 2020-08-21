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

#define QUPHOTO_USE_CAMERA_QML

#include "db/blobfieldref.h"
#include "lib/openglfunc.h"
#include "questionnairelib/quelement.h"

class AspectRatioPixmap;
class CameraQCamera;
class CameraQml;
class QLabel;
class QWidget;


class QuPhoto : public QuElement
{
    // Allows users to take a photo using the device's camera.

    Q_OBJECT
public:
    // Constructor
    QuPhoto(BlobFieldRefPtr fieldref);

protected:
    // Set widget state (image) from field data.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Rotate image.
    void rotate(int angle_degrees_clockwise);

    // Worker function, called in a separate thread, to rotate the image.
    void rotateWorker(int angle_degrees_clockwise);

protected slots:
    // "The field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref);

    // "Take the photo."
    void takePhoto();

    // "Set photo to blank."
    void resetFieldToNull();

    // "Rotate left 90 degrees."
    void rotateLeft();

    // "Rotate right 90 degrees."
    void rotateRight();

    // "User cancelled taking a photo."
    void cameraCancelled();

    // "Camera sends you this captured raw image."
    void rawImageCaptured(const QByteArray& data,
                          const QString& extension_without_dot,
                          const QString& mimetype);

    // "Camera sends you this captured QImage."
    void imageCaptured(const QImage& image);

protected:
    BlobFieldRefPtr m_fieldref;  // our field
    bool m_have_opengl;  // is OpenGL available?
    bool m_have_camera;  // are any cameras available?

    QPointer<Questionnaire> m_questionnaire;  // our questionnaire
    QPointer<QLabel> m_incomplete_optional_label;  // label for incomplete data
    QPointer<QLabel> m_incomplete_mandatory_label;  // label for incomplete data
    QPointer<QLabel> m_field_problem_label;  // "something wrong" indicator
    QPointer<AspectRatioPixmap> m_image_widget;  // image display widget
#ifdef QUPHOTO_USE_CAMERA_QML
    QPointer<CameraQml> m_camera;  // camera
#else
    QPointer<CameraQCamera> m_camera;  // camera
#endif
    QPointer<QWidget> m_main_widget;  // top-level widget
};
