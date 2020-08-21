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
#include <functional>
#include <QMap>
#include <QString>
#include <QVariant>
#include "questionnairelib/quelement.h"


class QuButton : public QuElement
{
    // Element to offer a button (calling a callback function).

    Q_OBJECT
public:

    using CallbackFunction = std::function<void()>;
    // To pass other arguments, use std::bind to bind them before passing here.
    // For example:
    //
    // - plain function
    //   std::bind(&myFunc, this)
    //
    // - member function
    //   std::bind(&MyClass::myFunc, this)
    //
    // - member function with parameter
    //   std::bind(&MyClass::myFunc, this, "someparam")

    // Constructor: display text label
    QuButton(const QString& label, const CallbackFunction& callback);

    // Constructor: display icon.
    // Args:
    //  icon_filename:
    //      icon filename
    //  filename_is_camcops_stem:
    //      process filename via uifunc::iconFilename(filename)
    //  alter_unpressed_image:
    //      apply a background circle to the plain ("unpressed") image, as well
    //      as the "pressed" state.
    //  callback
    //      the callback function
    QuButton(const QString& icon_filename, bool filename_is_camcops_stem,
             bool alter_unpressed_image, const CallbackFunction& callback);

    // Should the button respond, or just sit there unresponsive?
    // (It will also be inactive in read-only questionnaires, but this allows
    // you to disable it on the fly in live questionnaires.)
    QuButton* setActive(bool active);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;

protected slots:
    // "Our internal button widget was clicked."
    void clicked();

protected:
    QString m_label;  // text for text buttons
    QString m_icon_filename;  // filename for image buttons
    bool m_filename_is_camcops_stem;  // how to interpret our filename
    bool m_alter_unpressed_image;  // see above
    CallbackFunction m_callback;  // callback function
    bool m_active;  // should the button be responsive?
};
