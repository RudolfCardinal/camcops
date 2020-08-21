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

// Adapted from:
// http://omg-it.works/how-to-grab-video-frames-directly-from-qcamera/

#include "cameraframegrabber.h"
#include <QDebug>

// #define DEBUG_FRAMEGRABBER


CameraFrameGrabber::CameraFrameGrabber(QObject* parent) :
    QAbstractVideoSurface(parent)
{
}


QList<QVideoFrame::PixelFormat> CameraFrameGrabber::supportedPixelFormats(
        const QAbstractVideoBuffer::HandleType handle_type) const
{
    Q_UNUSED(handle_type)
    return QList<QVideoFrame::PixelFormat>()
        << QVideoFrame::Format_ARGB32
        << QVideoFrame::Format_ARGB32_Premultiplied
        << QVideoFrame::Format_RGB32
        << QVideoFrame::Format_RGB24
        << QVideoFrame::Format_RGB565
        << QVideoFrame::Format_RGB555
        << QVideoFrame::Format_ARGB8565_Premultiplied
        << QVideoFrame::Format_BGRA32
        << QVideoFrame::Format_BGRA32_Premultiplied
        << QVideoFrame::Format_BGR32
        << QVideoFrame::Format_BGR24
        << QVideoFrame::Format_BGR565
        << QVideoFrame::Format_BGR555
        << QVideoFrame::Format_BGRA5658_Premultiplied
        << QVideoFrame::Format_AYUV444
        << QVideoFrame::Format_AYUV444_Premultiplied
        << QVideoFrame::Format_YUV444
        << QVideoFrame::Format_YUV420P
        << QVideoFrame::Format_YV12
        << QVideoFrame::Format_UYVY
        << QVideoFrame::Format_YUYV
        << QVideoFrame::Format_NV12
        << QVideoFrame::Format_NV21
        << QVideoFrame::Format_IMC1
        << QVideoFrame::Format_IMC2
        << QVideoFrame::Format_IMC3
        << QVideoFrame::Format_IMC4
        << QVideoFrame::Format_Y8
        << QVideoFrame::Format_Y16
        << QVideoFrame::Format_Jpeg
        << QVideoFrame::Format_CameraRaw
        << QVideoFrame::Format_AdobeDng;
}


bool CameraFrameGrabber::present(const QVideoFrame& frame)
{
#ifdef DEBUG_FRAMEGRABBER
    qDebug() << Q_FUNC_INFO;
#endif
    // Android crash (segfault) in this function with message:
    //      attachToContext: invalid current EGLDisplay
    // - https://stackoverflow.com/questions/41040376/checkandupdateeglstate-invalid-current-eglcontext
    // - https://stackoverflow.com/questions/35510252/surfacetexture-updateteximage-fails-because-of-ivalid-context
    if (frame.isValid()) {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QAbstractVideoBuffer::ReadOnly);
        const QImage image(cloneFrame.bits(),
                           cloneFrame.width(),
                           cloneFrame.height(),
                           QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat()));
        emit frameAvailable(image);
        cloneFrame.unmap();
        return true;
    }
    return false;
}
