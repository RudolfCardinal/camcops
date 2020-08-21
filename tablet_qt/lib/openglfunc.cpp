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

// See:
// - https://raw.githubusercontent.com/mltframework/shotcut/master/src/widgets/gltestwidget.h
// - https://raw.githubusercontent.com/mltframework/shotcut/master/src/widgets/gltestwidget.cpp
// - https://forum.qt.io/topic/68217/qopenglwidget-how-to-get-opengl-version-from-os


#include "openglfunc.h"
#include <QDebug>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QSurface>
#include <QtGui/QSurfaceFormat>
#include <QtOpenGL/QGLFormat>
#include <QtWidgets/QOpenGLWidget>

static bool s_opengl_presence_checked = false;
static bool s_opengl_present = false;


namespace openglfunc {


bool isOpenGLPresent()
{
    if (!s_opengl_presence_checked) {
        s_opengl_presence_checked = true;
        s_opengl_present = false;

#ifndef QT_NO_OPENGL

        // - Android supports OpenGL ES 2.0 from Android 2.2 (API level 8).
        // - Android supports OpenGL ES 3.0 from Android 4.3 (API level 18),
        //   though not all devices may support this.
        // See https://developer.android.com/guide/topics/graphics/opengl.

        // For Qt support, see
        // - https://blog.qt.io/blog/2015/09/09/cross-platform-opengl-es-3-apps-with-qt-5-6/
        // - https://doc.qt.io/qt-5/qopenglfunctions.html
        // - https://doc.qt.io/qt-5/qopenglversionprofile.html#details

        QOffscreenSurface surf;
        surf.create();
        const QSurfaceFormat fmt = surf.format();
        qInfo() << "OpenGL surface format:" << fmt;

        QOpenGLContext ctx;
        if (!ctx.create()) {
            qCritical() << "Unable to create OpenGL context";
            return false;
        }
        ctx.makeCurrent(&surf);

        QOpenGLFunctions* glfuncs = ctx.functions();
        glfuncs->initializeOpenGLFunctions();

        // Doesn't work under Android:
        // const QOpenGLFunctions_2_0* v2funcs = ctx.versionFunctions<QOpenGLFunctions_2_0>();
        // const bool opengl_v2 = (v2funcs != nullptr);

        // Deprecated and missing on Android:
        // const bool opengl_v2 = QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0;

        const bool opengl_v2 = fmt.majorVersion() >= 2;

        const bool npot_textures = glfuncs->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
        const bool shaders = glfuncs->hasOpenGLFeature(QOpenGLFunctions::Shaders);
        const bool framebuffers = glfuncs->hasOpenGLFeature(QOpenGLFunctions::Framebuffers);
        qInfo() << "OpenGL v2.0 present:" << opengl_v2;
        qInfo() << "OpenGL has NPOTTextures:" << npot_textures;
        // ... "not powers of two" textures
        qInfo() << "OpenGL has shaders:" << shaders;
        qInfo() << "OpenGL has framebuffers:" << framebuffers;

        s_opengl_present = opengl_v2;  // we don't need the fancy bits

        if (s_opengl_present) {
            qInfo() << "OpenGL v2.0 or higher is present and satisfactory";
        } else {
            qCritical() << "Error: This program requires OpenGL version 2.0+";
        }
#endif

    }
    return s_opengl_present;
}


}  // namespace openglfunc
