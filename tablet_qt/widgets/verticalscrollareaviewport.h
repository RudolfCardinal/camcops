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
#include <QWidget>


class VerticalScrollAreaViewport : public QWidget
{
    // Class intended as the viewport widget for VerticalScrollArea, to
    // replace the default implementation of QScrollArea, which uses a plain
    // QWidget as its viewport.
    //
    // The main thing is that it has a DIRECT CHILD WIDGET, given to it by
    // the scroll area, not a layout.
    //
    // And the thing we need to avoid is that the child widget gets its height
    // set based on its sizeHint(), without any reference to heightForWidth().
    //
    // HOWEVER, resizeEvent() is not called when we resize the
    // VerticalScrollArea, and setGeometry() is not virtual, so this is not
    // much use. (If called via a QWidget*, as in
    // QAbstractScrollAreaPrivate::layoutChildren(), the base class
    // QWidget::setGeometry() will be called.)

    Q_OBJECT
public:

    // Constructor
    VerticalScrollAreaViewport(QWidget* parent = nullptr);

    // Perform debugging checks. Report to the debugging stream.
    void checkChildSize() const;
};
