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

#include <QPointer>
#include <QWidget>
class QGraphicsScene;
class SizeWatcher;
class ZoomableGraphicsView;


class ZoomableWidget : public QWidget
{
    // Widget that encloses another, and provides a zoomable view onto it.
    //
    // (Compare also the OpenableWidget/ScreenLikeGraphicsView combination used
    // by Task::makeGraphicsWidget for graphics-based tasks -- related, but
    // different.)

    Q_OBJECT
public:

    // Constructor
    // - A scale of 1.0 means "life-sized".
    //
    // - contents: widget to be encapsulated.
    // - can_scale_smaller_than_viewport: can we shrink the contents smaller
    //   than the viewport? Using "true" is unusual!
    // - min_scale: minimum scale permitted (subject to
    //   can_scale_smaller_than_viewport).
    // - max_scale: maximum scale permitted.
    // - scale_step_factor: a single notch of a mouse wheel zooms in/out by
    //   this factor.
    // - minimum_size: how small can this widget go? Since it zooms its
    //   contents, this can be pretty small.
    // - parent: widget's parent (optional)
    ZoomableWidget(QWidget* contents,
                   bool can_scale_smaller_than_viewport = false,
                   qreal min_scale = 0.2,
                   qreal max_scale = 5.0,
                   qreal scale_step_factor = 1.1,
                   const QSize& minimum_size = QSize(300, 300),
                   QWidget* parent = nullptr);

protected:
    // Standard overrides
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int width) const override;

protected:
    QWidget* m_contents;  // the widget we're displaying
    QPointer<QGraphicsScene> m_scene;  // a graphics scene containing the contents
    QPointer<ZoomableGraphicsView> m_view;  // view to display/zoom the scene
    QSize m_minimum_size;  // how small may we be displayed?
};
