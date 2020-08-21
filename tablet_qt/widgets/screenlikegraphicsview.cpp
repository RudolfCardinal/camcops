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

#include "screenlikegraphicsview.h"
#include <QDebug>
#include <QFrame>
#include <QResizeEvent>
#include <QTransform>
#include "common/colourdefs.h"


ScreenLikeGraphicsView::ScreenLikeGraphicsView(QWidget* parent) :
    QGraphicsView(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setBackgroundBrush(QCOLOR_BLACK);
    setFrameShape(QFrame::NoFrame);
}


ScreenLikeGraphicsView::ScreenLikeGraphicsView(QGraphicsScene* scene,
                                               QWidget* parent) :
    ScreenLikeGraphicsView(parent)  // delegating constructor
{
    setScene(scene);  // this is what QGraphicsView(scene, parent) does internally
}


void ScreenLikeGraphicsView::setBackgroundColour(const QColor& colour)
{
    // Without this, it's transparent, so you see the CSS effect for the
    // underlying window or some garbage like that.
    setBackgroundBrush(QBrush(colour, Qt::SolidPattern));
}


void ScreenLikeGraphicsView::resizeEvent(QResizeEvent* event)
{
    /*
    http://doc.qt.io/qt-5.8/qgraphicsview.html
    "Note though, that calling fitInView() from inside resizeEvent() can lead
    to unwanted resize recursion, if the new transformation toggles the
    automatic state of the scrollbars. You can toggle the scrollbar policies to
    always on or always off to prevent this (see horizontalScrollBarPolicy()
    and verticalScrollBarPolicy()).
    */

    Q_UNUSED(event)
    fitView();
}


void ScreenLikeGraphicsView::showEvent(QShowEvent* event)
{
    // http://stackoverflow.com/questions/17028680/qt5-c-qgraphicsview-images-dont-fit-view-frame
    Q_UNUSED(event)
    fitView();
}


void ScreenLikeGraphicsView::fitView()
{
#ifdef SCREENLIKEGRAPHICSVIEW_REDO_FIT_IN_VIEW
    fitInView2(sceneRect(), Qt::KeepAspectRatio);
#else
    fitInView(sceneRect(), Qt::KeepAspectRatio);
#endif
    // A bit of ?hardcoded margin appears, e.g. 1 pixel around the edge.
    // - https://bugreports.qt.io/browse/QTBUG-42331
}


#ifdef SCREENLIKEGRAPHICSVIEW_REDO_FIT_IN_VIEW
// The problem turned out not to be mis-scaling, but that
// viewport() is two pixels smaller than the QGraphicsView.
// That was fixed with setFrameShape(QFrame::NoFrame);

void ScreenLikeGraphicsView::fitInView2(
        const QRectF& rect,
        Qt::AspectRatioMode aspect_ratio_mode)
{
    // Bugfix for default fitInView() implementation
    // - https://bugreports.qt.io/browse/QTBUG-42331
    // - https://github.com/nevion/pyqimageview/blob/master/qimageview/widget.py#L276

    //
    if (!scene() || rect.isNull()) {
        return;
    }
    // self.last_scene_roi = rect
    const QRectF unity = transform().mapRect(QRectF(0, 0, 1, 1));
    scale(1/unity.width(), 1/unity.height());
    QWidget* vp = viewport();
    if (!vp) {
        qWarning() << Q_FUNC_INFO << "No viewport!";
        return;
    }
    const QRect view_rect = vp->rect();
    const QRectF scene_rect = transform().mapRect(rect);
    qreal xratio = view_rect.width() / scene_rect.width();
    qreal yratio = view_rect.height() / scene_rect.height();
    if (aspect_ratio_mode == Qt::KeepAspectRatio) {
        xratio = yratio = qMin(xratio, yratio);
    } else if (aspect_ratio_mode == Qt::KeepAspectRatioByExpanding) {
        xratio = yratio = qMax(xratio, yratio);
    }
    scale(xratio, yratio);
    centerOn(rect.center());
}


void ScreenLikeGraphicsView::fitInView2(qreal x, qreal y, qreal w, qreal h,
                                        Qt::AspectRatioMode aspect_ratio_mode)
{
    fitInView2(QRectF(x, y, w, h), aspect_ratio_mode);
}

#endif
