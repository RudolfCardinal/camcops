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
#include <QDebug>
#include "lib/layoutdumper.h"

class Questionnaire;
class QuElement;

class QVariant;


namespace debugfunc {

// Send a QVariant to the specified debugging stream, abbreviating potentially
// giant things like QByteArray.
void debugConcisely(QDebug debug, const QVariant& value);

// Send a vector of QVariant objects to the specified debugging stream,
// abbreviating potentially giant things like QByteArray.
void debugConcisely(QDebug debug, const QVector<QVariant>& values);

// Dumps generic information about a QObject.
void dumpQObject(QObject* obj);

// Displays a QWidget in a new dialogue box.
//
// - Place it on a green background.
// - Press <D> to dump information about the widget, including its layout and
//   its children, and all their positional information.
// - Press <A> to call QWidget::adjustSize().
void debugWidget(QWidget* widget,
                 bool set_background_by_name = false,
                 bool set_background_by_stylesheet = true,
                 const layoutdumper::DumperConfig& config = layoutdumper::DumperConfig(),
                 bool use_hfw_layout = true,
                 const QString* dialog_stylesheet = nullptr);

}  // namespace debugfunc
