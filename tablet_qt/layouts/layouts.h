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

#include "common/gui_defines.h"

// Choose which layout system we will use. (GUI_USE_HFW_LAYOUT is better.)

#ifdef GUI_USE_HFW_LAYOUT
#include "layouts/gridlayouthfw.h"
#include "layouts/hboxlayouthfw.h"
#include "layouts/vboxlayouthfw.h"
using GridLayout = GridLayoutHfw;
using HBoxLayout = HBoxLayoutHfw;
using VBoxLayout = VBoxLayoutHfw;
#else
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
using GridLayout = QGridLayout;
using HBoxLayout = QHBoxLayout;
using VBoxLayout = QVBoxLayout;
#endif
