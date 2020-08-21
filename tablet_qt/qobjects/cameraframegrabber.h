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
#include <QAbstractVideoSurface>
#include <QList>
#include <QVideoFrame>

class CameraFrameGrabber : public QAbstractVideoSurface
{
    // Class to use a video surface as a camera's viewfinder, and grab a frame
    // from it.

    Q_OBJECT
public:
    explicit CameraFrameGrabber(QObject* parent = nullptr);
    virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handle_type) const override;
    virtual bool present(const QVideoFrame& frame) override;
signals:
    void frameAvailable(QImage image);  // QImage is copy-on-write
    // ... https://stackoverflow.com/questions/8455887/stack-object-qt-signal-and-parameter-as-reference/18146433
};
