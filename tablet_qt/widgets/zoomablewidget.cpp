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

// #define DEBUG_PAINTING

#include "zoomablewidget.h"
#include <QDebug>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include "qobjects/sizewatcher.h"
#include "widgets/zoomablegraphicsview.h"

/*

See:

- https://stackoverflow.com/questions/6650219/zooming-function-on-a-qwidget
- https://stackoverflow.com/questions/26811446/qt-scaling-zooming-contents-of-a-qframe-widgets-etc

NOTES

- I spent a bit of effort trying to read the widget's size, and then use
  m_scene->setSceneRect(size). However, it turns out to be much better not to
  use that function at all, at which point sceneRect() returns "the current
  widget size" in a useful way -- for use by e.g.
  ZoomableGraphicsView::fitView().

- On shrike, at least, this gives perfect text rendering when zoomed (not
  a pixelwise zoom).

*/


ZoomableWidget::ZoomableWidget(QWidget* contents,
                               const bool can_scale_smaller_than_viewport,
                               const qreal min_scale,
                               const qreal max_scale,
                               const qreal scale_step_factor,
                               const QSize& minimum_size,
                               QWidget* parent) :
    QWidget(parent),
    m_contents(contents),
    m_minimum_size(minimum_size)
{
    Q_ASSERT(m_contents);

    // We create a graphics scene containing our target widget.
    contents->ensurePolished();
    m_scene = new QGraphicsScene();
    m_scene->addWidget(contents);  // adds it at (0,0); returns QGraphicsProxyWidget*

    // We create a graphics view to show the scene.
    // The view is where we implement zooming.
    m_view = new ZoomableGraphicsView(
                m_scene, can_scale_smaller_than_viewport,
                min_scale, max_scale, scale_step_factor);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Our widget (this) has a layout containing the graphics view.
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_view);

    // We'd like "this" to be as large as possible:
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // No: sp.setHeightForWidth(contents->hasHeightForWidth());
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << "ZoomableWidget size policy:" << sp;
#endif
    setSizePolicy(sp);
}


QSize ZoomableWidget::sizeHint() const
{
    const QSize size = m_contents->sizeHint();
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << size;
#endif
    return size;
}


QSize ZoomableWidget::minimumSizeHint() const
{
    // No: const QSize size = m_contents->minimumSizeHint();
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << m_minimum_size;
#endif
    return m_minimum_size;
}


bool ZoomableWidget::hasHeightForWidth() const
{
    const bool hfw = m_contents->hasHeightForWidth();
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << hfw;
#endif
    return hfw;
}


int ZoomableWidget::heightForWidth(int width) const
{
    const int h = m_contents->heightForWidth(width);
#ifdef DEBUG_PAINTING
    qDebug() << Q_FUNC_INFO << h;
#endif
    return h;
}
