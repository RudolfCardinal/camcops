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
#include "common/gui_defines.h"


class BaseWidget : public QWidget
{
    // Generic base widget class for CamCOPS. It has two functions, one
    // superseded and #defined out.
    //
    // (1) If GUI_USE_RESIZE_FOR_HEIGHT is defined, this is a widget that knows
    // that its layout implements a height-for-width function and deals with it
    // properly, adjusting the widget's height to the layout (and its
    // contents).
    //
    // - SPECIFICALLY: IT WILL REDUCE ITS HEIGHT (TO FIT THE CONTENTS) AS THE
    //   LAYOUT SPREADS OUT CHILD WIDGETS TO THE RIGHT (in a way that a plain
    //   QWidget won't).
    //
    // - ... and will also set a correct MINIMUM HEIGHT as the width shrinks.
    //
    // - Use this when you want to put a FlowLayout in (e.g. see QuMCQ).
    //
    // - You might also use this when you want a widget containing a layout
    //   containing a LabelWordWrapWide object, or similar (e.g. see
    //   ClickableLabelWordWrapWide -- though that has to re-implement, not
    //   inherit, for Qt inheritance reasons).
    //
    // However, this system is INFERIOR to using a proper layout; see
    // BoxLayoutHfw and its children, for example. We have therefore moved AWAY
    // from GUI_USE_RESIZE_FOR_HEIGHT; see gui_defines.h.
    //
    // (2) This also enables our widgets to support stylesheets; see below.

    Q_OBJECT

public:
    BaseWidget(QWidget* parent = nullptr);
    virtual ~BaseWidget() override;

#ifdef GUI_USE_RESIZE_FOR_HEIGHT
    virtual void resizeEvent(QResizeEvent* event) override;
#endif

    // IF WE WANT CLASSES DERIVED FROM QWidget TO SUPPORT STYLE SHEETS,
    // as we do, we must also support this:
    virtual void paintEvent(QPaintEvent* event) override;
};
