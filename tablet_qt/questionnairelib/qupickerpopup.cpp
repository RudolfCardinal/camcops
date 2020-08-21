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

#include "qupickerpopup.h"
#include <QDialog>
#include <QHBoxLayout>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "dialogs/nvpchoicedialog.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/clickablelabelwordwrapwide.h"


// const int MAX_LENGTH = 100;


QuPickerPopup::QuPickerPopup(FieldRefPtr fieldref,
                             const NameValueOptions& options) :
    m_fieldref(fieldref),
    m_options(options),
    m_randomize(false),
    m_label(nullptr)
{
    m_options.validateOrDie();
    Q_ASSERT(m_fieldref);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuPickerPopup::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuPickerPopup::fieldValueChanged);
}


QuPickerPopup* QuPickerPopup::setRandomize(const bool randomize)
{
    m_randomize = randomize;
    return this;
}


QuPickerPopup* QuPickerPopup::setPopupTitle(const QString& popup_title)
{
    m_popup_title = popup_title;
    return this;
}


QPointer<QWidget> QuPickerPopup::makeWidget(Questionnaire* questionnaire)
{
    // Randomize?
    if (m_randomize) {
        m_options.shuffle();
    }
    const bool read_only = questionnaire->readOnly();

    m_label = new ClickableLabelWordWrapWide(true);
    m_label->setObjectName(cssconst::PICKER_POPUP);
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (!read_only) {
        connect(m_label.data(), &ClickableLabelWordWrapWide::clicked,
                this, &QuPickerPopup::clicked);
    }
    m_label->setEnabled(!read_only);

    setFromField();
    return QPointer<QWidget>(m_label);
}


void QuPickerPopup::clicked()
{
    if (!m_label) {
        return;
    }
    NvpChoiceDialog dlg(m_label, m_options, m_popup_title);
    QVariant newvalue;
    if (dlg.choose(&newvalue) != QDialog::Accepted) {
        return;  // user pressed cancel, or some such
    }
    const bool changed = m_fieldref->setValue(newvalue);  // Will trigger valueChanged
    if (changed) {
        emit elementValueChanged();
    }
}


void QuPickerPopup::setFromField()
{
    fieldValueChanged(m_fieldref.data());
}


void QuPickerPopup::fieldValueChanged(const FieldRef* fieldref)
{
    if (!m_label) {
        return;
    }
    const int index = m_options.indexFromValue(fieldref->value());
    const bool missing = fieldref->missingInput();
    uifunc::setPropertyMissing(m_label, missing);
    const QString text = m_options.name(index);
    m_label->setText(text);
}


FieldRefPtrList QuPickerPopup::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}
