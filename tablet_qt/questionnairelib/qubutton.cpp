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

#include "qubutton.h"
#include "common/cssconst.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/imagebutton.h"


QuButton::QuButton(const QString& label,
                   const CallbackFunction& callback) :
    m_label(label),
    m_callback(callback),
    m_active(true)
{
}


QuButton::QuButton(const QString& icon_filename,
                   const bool filename_is_camcops_stem,
                   const bool alter_unpressed_image,
                   const CallbackFunction& callback) :
    m_icon_filename(icon_filename),
    m_filename_is_camcops_stem(filename_is_camcops_stem),
    m_alter_unpressed_image(alter_unpressed_image),
    m_callback(callback),
    m_active(true)
{
}


QuButton* QuButton::setActive(const bool active)
{
    m_active = active;
    return this;
}


QPointer<QWidget> QuButton::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = !m_active || questionnaire->readOnly();
    QAbstractButton* button;
    if (!m_label.isEmpty()) {
        // Text
        button = new ClickableLabelWordWrapWide(m_label);
        button->setObjectName(cssconst::BUTTON);
        if (read_only) {
            button->setEnabled(false);  // NB setDisabled and setEnabled are not exact opposites
        }
    } else {
        // Image
        button = new ImageButton(m_icon_filename, m_filename_is_camcops_stem,
                                 m_alter_unpressed_image, read_only);
    }
    if (!read_only) {
        connect(button, &QAbstractButton::clicked,
                this, &QuButton::clicked);
    }
    return QPointer<QWidget>(button);
}


void QuButton::clicked()
{
    m_callback();
}
